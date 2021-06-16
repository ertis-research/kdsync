#!/bin/bash

# Pass kafka project path using first argument or use default
KAFKA_HOME="${1:-`dirname $0`/kafka-dist}"

KDSYNC_SUFFIX="-global"

TOPIC="${2:-test}"

cd $KAFKA_HOME

open_terminal() {
        gnome-terminal --disable-factory -t "$2" -- sh -c "while true; do $1; done; bash;" # sleep 5"
}

# open_terminal "bin/kafka-console-consumer.sh --whitelist '$TOPIC|$TOPIC$KDSYNC_SUFFIX' --bootstrap-server localhost:9092" \
#         "Cluster 0 | Consumer <- $TOPIC" &

open_terminal "bin/kafka-console-consumer.sh --topic '$TOPIC' --bootstrap-server localhost:9092" \
        "Cluster 0 | Consumer <- $TOPIC" &

open_terminal "bin/kafka-console-consumer.sh --topic '$TOPIC$KDSYNC_SUFFIX' --bootstrap-server localhost:9092" \
        "Cluster 0 | Consumer <- $TOPIC$KDSYNC_SUFFIX" &


open_terminal "bin/kafka-console-producer.sh --topic '$TOPIC' --bootstrap-server localhost:9092" \
        "Cluster 0 | Producer -> $TOPIC" &


# open_terminal "bin/kafka-console-consumer.sh --whitelist '$TOPIC|$TOPIC$KDSYNC_SUFFIX' --bootstrap-server localhost:9094" \
#         "Cluster 1 | Consumer <- $TOPIC" &

open_terminal "bin/kafka-console-consumer.sh --topic '$TOPIC' --bootstrap-server localhost:9094" \
        "Cluster 1 | Consumer <- $TOPIC" &

open_terminal "bin/kafka-console-consumer.sh --topic '$TOPIC$KDSYNC_SUFFIX' --bootstrap-server localhost:9094" \
        "Cluster 1 | Consumer <- $TOPIC$KDSYNC_SUFFIX" &


open_terminal "bin/kafka-console-producer.sh --topic '$TOPIC' --bootstrap-server localhost:9094" \
        "Cluster 1 | Producer -> $TOPIC" &

wait