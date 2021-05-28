#!/bin/bash

# This script will launch two gnome-shell terminals, each one with custom 
# parameters and configuration files. Each shell is a kdsync network.
# If first parameter "debug" is issued, kdsync instances will be launched through 
# gdb.


if [ "${1^^}" = "DEBUG" ]; then
    echo -e "\e[33mGDB DEBUG ENABLED\e[39m"
    DEBUG_PREFIX="gdb -ex=r --args"
fi

run_terminal() { # args, derecho_cfg_file, title
    export DERECHO_CONF_FILE=$2
    SCRIPT_DIR=`dirname $0`
    LAUNCH_COMMAND="$DEBUG_PREFIX \"`dirname $0`/../../../bin/kdsync\" $1"
    echo "Launch command: "$LAUNCH_COMMAND
    gnome-terminal --disable-factory -t "$3 - ($DERECHO_CONF_FILE)" -- sh -c "echo 'Running kdsync...';
    $LAUNCH_COMMAND; bash;"
}

SLEEP_TIME=5

run_terminal "localhost:9092 test" "`dirname $0`/config/derecho_0.cfg" "localhost:9092" &
sleep $SLEEP_TIME
run_terminal "localhost:9094 test" "`dirname $0`/config/derecho_1.cfg" "localhost:9094" &

wait