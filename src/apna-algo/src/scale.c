#include "scale.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "amf.h"
#include "forward.h"
#include "utils.h"

int AMF_CAPACITY = 15;
const float HEADROOM_PERCENTAGE = 0.20f;
const float THRESHOLD_DOWN = (1 - HEADROOM_PERCENTAGE) / 6;
const float THRESHOLD_UP = (1 - HEADROOM_PERCENTAGE) / 2;

pthread_mutex_t amf_state_mutex = PTHREAD_MUTEX_INITIALIZER;
int total_conn_count = 0;

extern char migration_log_filename[256];

// Helper: generate deployment name for AMF index i (0-based)
static void get_deployment_name(int index, char* buf, size_t buf_len) {
    // Human-friendly numbering starts at 1
    snprintf(buf, buf_len, "core5g-amf-%d-deployment", index + 1);
}

void scale_up_check(void) {
    pthread_mutex_lock(&amf_state_mutex);
    int active_count = get_active_amf_count();
    if (active_count == 0) {
        log("INFO", "[scale] scale_up_check: No active AMFs, skipping scale up\n");
        pthread_mutex_unlock(&amf_state_mutex);
        return;
    }

    int threshold = (int)(active_count * AMF_CAPACITY * (1 - HEADROOM_PERCENTAGE));
    log("INFO", "[scale] scale_up_check: total_conn_count=%d, threshold=%d\n",
        total_conn_count, threshold);

    if (total_conn_count >= threshold) {
        for (int i = 0; i < MAX_AMFS; i++) {
            if (!amfs[i].active) {
                log("INFO", "[scale] SCALE UP: Load (%d) exceeds threshold (%d). Deploying AMF %d...\n",
                    total_conn_count, threshold, amfs[i].id);
                amfs[i].active = 1;

                char name_buf[64];
                get_deployment_name(i, name_buf, sizeof(name_buf));

                char* cmd[] = {
                    "kubectl",
                    "-n",
                    "open5gs",
                    "scale",
                    "deployment",
                    name_buf,
                    "--replicas=1",
                    NULL};
                execute_command("kubectl", cmd);
                break;
            }
        }
    } else {
        log("INFO", "[scale] scale_up_check: Load below threshold, no scaling up needed\n");
    }

    pthread_mutex_unlock(&amf_state_mutex);
}

void* descaling_thread_func(void* arg) {
    (void)arg;
    while (1) {
        sleep(DESCALING_INTERVAL_MINUTES * 60);

        pthread_mutex_lock(&amf_state_mutex);
        int active_count = get_active_amf_count();
        if (active_count <= 1) {
            for (int i = 0; i < MAX_AMFS; i++) {
                AMF* old_amf = &amfs[i];
                if (!old_amf->active) continue;

                float util = old_amf->connections > 0
                                 ? (float)old_amf->connections / AMF_CAPACITY
                                 : 0.0f;
                log("INFO", "[scale] descaling_thread_func: Checking AMF %d utilization: %.2f\n",
                    old_amf->id, util);
            }
            log("INFO", "[scale] descaling_thread_func: Only %d active AMF(s), skipping scale down\n",
                active_count);
            pthread_mutex_unlock(&amf_state_mutex);
            continue;
        }

        for (int i = 0; i < MAX_AMFS; i++) {
            AMF* old_amf = &amfs[i];
            if (!old_amf->active) continue;

            float util = old_amf->connections > 0
                             ? (float)old_amf->connections / AMF_CAPACITY
                             : 0.0f;
            log("INFO", "[scale] descaling_thread_func: Checking AMF %d utilization: %.2f\n",
                old_amf->id, util);

            if (util <= THRESHOLD_DOWN) {
                AMF* new_amf = NULL;
                int min_load = AMF_CAPACITY;
                for (int j = 0; j < MAX_AMFS; j++) {
                    AMF* cand = &amfs[j];
                    if (!cand->active || cand == old_amf) continue;
                    float cand_util = (float)cand->connections / AMF_CAPACITY;
                    if (cand_util > THRESHOLD_DOWN &&
                        cand_util <= THRESHOLD_UP &&
                        cand->connections < min_load) {
                        min_load = cand->connections;
                        new_amf = cand;
                    }
                }

                if (new_amf) {
                    log("INFO", "[scale] SCALE DOWN: Migrating from AMF %d to AMF %d\n",
                        old_amf->id, new_amf->id);

                    // Start migration timer
                    struct timeval mig_start, mig_end;
                    gettimeofday(&mig_start, NULL);

                    pthread_mutex_lock(&old_amf->lock);
                    pthread_mutex_lock(&new_amf->lock);

                    pthread_mutex_lock(&live_threads_mutex);
                    for (int t = 0; t < FORWARD_CONNS_ARRAY_LEN; t++) {
                        if (live_threads[t] == NULL || !live_threads[t]->is_active)
                            continue;

                        forward_info_t* thread_info = live_threads[t];
                        if (*(thread_info->current_amf) == old_amf && thread_info->from_gnb) {
                            int old_sock = *(thread_info->destination_socket);
                            int new_sock = connect_to_amf(new_amf);

                            if (new_sock > 0) {
                                *(thread_info->current_amf) = new_amf;
                                *(thread_info->destination_socket) = new_sock;
                                log("INFO", "[scale] Migrated connection (gNB sock %d) to new AMF socket %d\n",
                                    thread_info->source_socket, new_sock);
                                close(old_sock);  // OLD AMF2GNB SCOCKET WILL GET DISRUPTED
                                log("DEBUG", "[scale] Closed old AMF socket %d after migration\n", old_sock);
                                // creat an amf2gnb thread where source is new_sock and dest is gnb socket

                                pthread_t amf2gnb_thread;
                                forward_info_t* amf2gnb_info = malloc(sizeof(forward_info_t));
                                if (!amf2gnb_info) {
                                    log("INFO", "[forward] handle_gnb_connection: Memory allocation failed for amf_to_gnb\n");
                                    continue;
                                }
                                amf2gnb_info->source_socket = new_sock;
                                amf2gnb_info->destination_socket = malloc(sizeof(int));
                                *(amf2gnb_info->destination_socket) = thread_info->source_socket;
                                amf2gnb_info->current_amf = malloc(sizeof(AMF*));
                                *(amf2gnb_info->current_amf) = new_amf;
                                amf2gnb_info->is_active = 1;
                                amf2gnb_info->live_thread_index = -1;
                                amf2gnb_info->from_gnb = false;

                                pthread_mutex_unlock(&live_threads_mutex);         // Unlock before registering thread
                                int slot = forward_register_thread(amf2gnb_info);  // this will lock the mutex again
                                pthread_mutex_lock(&live_threads_mutex);           // Re-lock after registering thread
                                if (slot != -1) {
                                    pthread_mutex_unlock(&old_amf->lock);  // unlock before creating thread
                                    pthread_mutex_unlock(&new_amf->lock);  // unlock before creating thread
                                    if (pthread_create(&amf2gnb_thread, NULL, forward_messages, amf2gnb_info) == 0) {
                                        pthread_detach(amf2gnb_thread);
                                        log("INFO", "[scale] Started AMF->gNB thread for migrated connection (gNB sock %d)\n",
                                            thread_info->source_socket);
                                    }
                                    pthread_mutex_lock(&old_amf->lock);  // relock after creating thread
                                    pthread_mutex_lock(&new_amf->lock);  // relock after creating thread
                                } else {
                                    log("INFO", "[scale] Could not register AMF->gNB thread for migrated connection (gNB sock %d)\n",
                                        thread_info->source_socket);
                                }
                            } else {
                                fprintf(stderr,
                                        "[scale] Migration failed for gNB sock %d: could not connect to new AMF.\n",
                                        thread_info->source_socket);
                            }
                            // Record migration end time and write CSV entry per migrated connection
                            gettimeofday(&mig_end, NULL);
                            double mig_ms = (mig_end.tv_sec - mig_start.tv_sec) * 1000.0 +
                                            (mig_end.tv_usec - mig_start.tv_usec) / 1000.0;
                            double mig_us = mig_ms * 1000;

                            // Prepare CSV fields: timestamp, gnb_ip, old_amf_ip, new_amf_ip, migration_us, migration_ms
                            char timestamp_str[32];
                            time_t now = time(NULL);
                            struct tm tm_now;
                            localtime_r(&now, &tm_now);
                            strftime(timestamp_str, sizeof(timestamp_str), "%Y-%m-%d %H:%M:%S", &tm_now);

                            char gnb_ip[64] = "unknown";
                            char old_amf_ip[64] = "unknown";
                            char new_amf_ip[64] = "unknown";
                            // Use helper to get gNB ip:port
                            get_ip_port(thread_info->source_socket, gnb_ip, sizeof(gnb_ip));
                            // AMF struct contains ip fields
                            if (old_amf->ip) snprintf(old_amf_ip, sizeof(old_amf_ip), "%s", old_amf->ip);
                            if (new_amf->ip) snprintf(new_amf_ip, sizeof(new_amf_ip), "%s", new_amf->ip);

                            FILE* mig_file = fopen(migration_log_filename, "a");
                            if (mig_file) {
                                fprintf(mig_file, "%s,%s,%s,%s,%.3lf,%.3lf\n",
                                        timestamp_str, gnb_ip, old_amf_ip, new_amf_ip, mig_us, mig_ms);
                                fclose(mig_file);
                            } else {
                                log_perror("[scale] Failed to open migration log");
                            }
                        }
                    }
                    pthread_mutex_unlock(&live_threads_mutex);

                    new_amf->connections += old_amf->connections;
                    old_amf->connections = 0;
                    old_amf->active = 0;

                    log("INFO", "[scale] Descaling: Scaling down AMF %d deployment to 0 replicas\n",
                        old_amf->id);

                    char name_buf[64];
                    get_deployment_name(i, name_buf, sizeof(name_buf));

                    char* cmd[] = {
                        "kubectl",
                        "-n",
                        "open5gs",
                        "scale",
                        "deployment",
                        name_buf,
                        "--replicas=0",
                        NULL};
                    execute_command("kubectl", cmd);

                    pthread_mutex_unlock(&new_amf->lock);
                    pthread_mutex_unlock(&old_amf->lock);

                    // till here migration latency to be calculated

                    break;
                } else {
                    log("INFO", "[scale] No suitable AMF found for migration from AMF %d\n",
                        old_amf->id);
                }
            }
        }

        pthread_mutex_unlock(&amf_state_mutex);
    }
    return NULL;
}
