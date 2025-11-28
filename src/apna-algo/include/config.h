#ifndef APNA_CONFIG_H
#define APNA_CONFIG_H

/* Centralized configuration for the apna-algo component.
 * This header exposes guarded macro defaults for tunable constants
 * referenced across the codebase. Each define is wrapped with
 * #ifndef so existing definitions in other headers or build
 * system can override them.
 */

#include <netinet/in.h>

/* Network defaults (can be overridden at compile time) */
#ifndef HOST_IP
#define HOST_IP "10.0.0.1"
#endif

#ifndef PORT
/* Use numeric literal for convenience; some files declare PORT as a variable. */
#define PORT 38412
#endif

/* Connection / concurrency */
#ifndef MAX_CONNECTIONS
#define MAX_CONNECTIONS 1000
#endif

#ifndef MAX_AMFS
#define MAX_AMFS 5
#endif

/* Buffering and forwarding */
#ifndef BUFFER_SIZE
#define BUFFER_SIZE 1024
#endif

#ifndef MAX_RETRIES
#define MAX_RETRIES 10
#endif

#ifndef RECONNECT_BUFFER_SECONDS
#define RECONNECT_BUFFER_SECONDS 5 /* seconds */
#endif

/* Forward connections table length: default to 2 * MAX_CONNECTIONS */
#ifndef FORWARD_CONNS_ARRAY_LEN
#define FORWARD_CONNS_ARRAY_LEN (2 * MAX_CONNECTIONS)
#endif

/* Scaling defaults. Note: AMF_CAPACITY is a variable in the code; provide
 * a default macro name for build-time defaults but do not conflict
 * with the variable name used in source files.
 */
#ifndef DEFAULT_AMF_CAPACITY
#define DEFAULT_AMF_CAPACITY 20
#endif

// this is in percentaga, actually
#ifndef HEADROOM
#define HEADROOM 20
#endif

#ifndef HEADROOM_PERCENTAGE
#define HEADROOM_PERCENTAGE HEADROOM / 100.F
#endif

/* Derived/default thresholds (kept as macros for clarity) */
#ifndef THRESHOLD_DOWN
#define THRESHOLD_DOWN ((1.0f - HEADROOM_PERCENTAGE) / 6.0f)
#endif

#ifndef THRESHOLD_UP
#define THRESHOLD_UP ((1.0f - HEADROOM_PERCENTAGE) / 2.0f)
#endif

#ifndef DESCALING_INTERVAL_MINUTES
#define DESCALING_INTERVAL_MINUTES 1
#endif

/* File/path defaults */
#ifndef LOGS_DIR
#define LOGS_DIR "logs"
#endif

#ifndef LATENCY_CSV_NAME
#define LATENCY_CSV_NAME "associative-latency.csv"
#endif

#ifndef MIGRATION_CSV_NAME
#define MIGRATION_CSV_NAME "migration-latency.csv"
#endif

#ifndef PID_FILE
#define PID_FILE "logs/tmp.pid"
#endif

/* Timestamp format used for log directory names */
#ifndef TIMESTAMP_FMT
#define TIMESTAMP_FMT "%Y-%m-%d_%H%M%S"
#endif

#endif /* APNA_CONFIG_H */