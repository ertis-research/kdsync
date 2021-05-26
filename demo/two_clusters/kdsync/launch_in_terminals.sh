#!/bin/bash

run_terminal() { # args, derecho_cfg_file, title
    export DERECHO_CONF_FILE=$2
    gnome-terminal --disable-factory -t "$3 - ($DERECHO_CONF_FILE)" -- sh -c "echo 'Running kdsync...';
    '`dirname $0`/../../../bin/kdsync' "$1"; bash;"
}

SLEEP_TIME=5

run_terminal "localhost:9092 test" "`dirname $0`/config/derecho_0.cfg" "localhost:9092" &
sleep $SLEEP_TIME
run_terminal "localhost:9094 test" "`dirname $0`/config/derecho_1.cfg" "localhost:9094" &

wait