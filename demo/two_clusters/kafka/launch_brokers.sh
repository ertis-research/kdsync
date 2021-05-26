#!/bin/bash

# Pass kafka project path using first argument or use default
KAFKA_HOME="${1:-`dirname $0`/kafka-dist}"

CONFIG_FOLDER="`dirname $0`/config"


run_kafka() {
        gnome-terminal --disable-factory -t "$2" -- sh -c "while true; do $1; done; bash;" # sleep 5"
}

# Cluster 0
## Broker 0
run_kafka "$KAFKA_HOME/bin/kafka-server-start.sh $CONFIG_FOLDER/cluster0-broker0.properties" "Cluster 0 broker 0 [9092]" &
## Broker 1
run_kafka "$KAFKA_HOME/bin/kafka-server-start.sh $CONFIG_FOLDER/cluster0-broker1.properties" "Cluster 0 broker 1 [9093]" &


# Cluster 1
## Broker 0
run_kafka "'$KAFKA_HOME/bin/kafka-server-start.sh' '$CONFIG_FOLDER/cluster1-broker0.properties'" "Cluster 1 broker 0 [9094]" &
## Broker 1
run_kafka "'$KAFKA_HOME/bin/kafka-server-start.sh' '$CONFIG_FOLDER/cluster1-broker1.properties'" "Cluster 1 broker 1 [9095]" &


# Run zookeeper
"$KAFKA_HOME/bin/zookeeper-server-start.sh"  "$KAFKA_HOME/config/zookeeper.properties"

wait