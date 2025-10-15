#include "scale.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "amf.h"
#include "forward.h"
#include "utils.h"

int AMF_CAPACITY = 10;
const float HEADROOM_PERCENTAGE = 0.16f;
const float THRESHOLD_DOWN = (1 - HEADROOM_PERCENTAGE) / 6;
const float THRESHOLD_UP = (1 - HEADROOM_PERCENTAGE) / 2;

pthread_mutex_t amf_state_mutex = PTHREAD_MUTEX_INITIALIZER;
int total_conn_count = 0;

// Helper: generate deployment name for AMF index i (0-based)
static void get_deployment_name(int index, char *buf, size_t buf_len) {
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

                char *cmd[] = {
                    "kubectl",
                    "-n",
                    "open5gs",
                    "scale",
                    "deployment",
                    name_buf,
                    "--replicas=1",
                    NULL
                };
                execute_command("kubectl", cmd);
                break;
            }
        }
    } else {
        log("INFO", "[scale] scale_up_check: Load below threshold, no scaling up needed\n");
    }

    pthread_mutex_unlock(&amf_state_mutex);
}

void *descaling_thread_func(void *arg) {
    (void)arg;
    while (1) {
        sleep(DESCALING_INTERVAL_MINUTES * 60);

        pthread_mutex_lock(&amf_state_mutex);
        int active_count = get_active_amf_count();
        if (active_count <= 1) {
            log("INFO", "[scale] descaling_thread_func: Only %d active AMF(s), skipping scale down\n",
                active_count);
            pthread_mutex_unlock(&amf_state_mutex);
            continue;
        }

        for (int i = 0; i < MAX_AMFS; i++) {
            AMF *old_amf = &amfs[i];
            if (!old_amf->active) continue;

            float util = old_amf->connections > 0
                          ? (float)old_amf->connections / AMF_CAPACITY
                          : 0.0f;
            log("INFO", "[scale] descaling_thread_func: Checking AMF %d utilization: %.2f\n",
                old_amf->id, util);

            if (util < THRESHOLD_DOWN) {
                AMF *new_amf = NULL;
                int min_load = AMF_CAPACITY;
                for (int j = 0; j < MAX_AMFS; j++) {
                    AMF *cand = &amfs[j];
                    if (!cand->active || cand == old_amf) continue;
                    float cand_util = (float)cand->connections / AMF_CAPACITY;
                    if (cand_util >= THRESHOLD_DOWN &&
                        cand_util < THRESHOLD_UP &&
                        cand->connections < min_load) {
                        min_load = cand->connections;
                        new_amf = cand;
                    }
                }

                if (new_amf) {
                    log("INFO", "[scale] SCALE DOWN: Migrating from AMF %d to AMF %d\n",
                        old_amf->id, new_amf->id);

                    pthread_mutex_lock(&old_amf->lock);
                    pthread_mutex_lock(&new_amf->lock);

                    pthread_mutex_lock(&live_threads_mutex);
                    for (int t = 0; t < MAX_CONNECTIONS; t++) {
                        if (live_threads[t] == NULL || !live_threads[t]->is_active)
                            continue;

                        forward_info_t *thread_info = live_threads[t];
                        if (*(thread_info->current_amf) == old_amf) {
                            int old_sock = *(thread_info->destination_socket);
                            int new_sock = connect_to_amf(new_amf);

                            if (new_sock > 0) {
                                *(thread_info->current_amf) = new_amf;
                                *(thread_info->destination_socket) = new_sock;
                                log("INFO", "[scale] Migrated connection (gNB sock %d) to new AMF socket %d\n",
                                    thread_info->source_socket, new_sock);
                                close(old_sock);
                            } else {
                                fprintf(stderr,
                                        "[scale] Migration failed for gNB sock %d: could not connect to new AMF.\n",
                                        thread_info->source_socket);
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

                    char *cmd[] = {
                        "kubectl",
                        "-n",
                        "open5gs",
                        "scale",
                        "deployment",
                        name_buf,
                        "--replicas=0",
                        NULL
                    };
                    execute_command("kubectl", cmd);

                    pthread_mutex_unlock(&new_amf->lock);
                    pthread_mutex_unlock(&old_amf->lock);
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
