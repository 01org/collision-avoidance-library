#!/usr/bin/env bash
# Copyright (c) 2017 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -e

SCRIPT_DIR=$(dirname $(realpath ${BASH_SOURCE[0]}))

# Set autopilot
# Supported Autopilots: AP_PX4 and AP_APM
AUTOPILOT=${AUTOPILOT:-"AP_PX4"}

# ArduCopter Variables
APM_CMD="${APM_DIR:+"${APM_DIR}/"}arducopter"
APM_TCP_PORT_1=5760
APM_TCP_PORT_2=5762
APM_PARM_URL=${APM_PARM_URL:-"https://raw.githubusercontent.com/ArduPilot/ardupilot/master/Tools/autotest/default_params/copter.parm"}

# PX4 Variables
PX4_DIR=${PX4_DIR:-"~/px4/Firmware"}
PX4_CMD="make posix_sitl_default jmavsim"
PX4_UDP_PORT_1=14550
PX4_UDP_PORT_2=14540

# Simulation Parameters
WORLD=${WORLD:-"simple_obstacle.sdf"}
LOGDIR=${LOGDIR:-"${SCRIPT_DIR}/logs"}

# Collision Avoidance and Gazebo variables
GZSITL_UDP_PORT=15556
COAV_GCS_UDP_PORT=15557

run_autopilot () {
    # Run SITL Simulator
    if [ "$AUTOPILOT" = "AP_PX4" ]; then
        cd $PX4_DIR
        $PX4_CMD > ${LOGDIR}/sitl.log 2> ${LOGDIR}/sitl_err.log &
        SITLID=$!
        cd - > /dev/null

        # Wait for sitl
        sleep 15
    elif [ "$AUTOPILOT" = "AP_APM" ]; then
        cd $SCRIPT_DIR # sitl must run in the same dir of "eeprom.bin"

        # Fetch copter parameters file
        if [ ! -f copter.parm ]; then
            wget ${APM_PARM_URL}
            wait $!

            if [ ! -f copter.parm ]; then
                echo "Couldn't fetch ardupilot's parm file."
                exit 1
            fi
        fi

        $APM_CMD --model x --defaults ./copter.parm \
            > ${LOGDIR}/sitl.log 2> ${LOGDIR}/sitl_err.log &
        SITLID=$!
        cd - > /dev/null

        # Wait for sitl
        sleep 5
    fi
}

run_gazebo () {
    # Gazebo engine without GUI.
    # The log can be played through `gazebo -p logfile`
    SDFFILE="${SCRIPT_DIR}/worlds/${WORLD}"
    gzserver --record_path $LOGDIR --verbose $SDFFILE \
        > ${LOGDIR}/gzserver.log 2> ${LOGDIR}/gzserver_err.log &
    GZID=$!

    # Wait until gazebo is up and running
    sleep 8
}

run_coav_control() {
    # Run the collision avoidance
    ../build/tools/coav-control/coav-control "$@" \
        > ${LOGDIR}/coav-control.log 2> ${LOGDIR}/coav-control_err.log &
    COAVID=$!

    # Wait until is up and running
    sleep 1
}

simulate () {
    run_coav_control "$@"
    run_autopilot
    run_gazebo

    # Bidirectional bridge between autopilot and gazebo-sitl
    SOCAT_ARG_2="udp:localhost:$GZSITL_UDP_PORT"
    if [ "$AUTOPILOT" = "AP_PX4" ]; then
        SOCAT_ARG_1="udp-listen:$PX4_UDP_PORT_1"
    elif [ "$AUTOPILOT" = "AP_APM" ]; then
        SOCAT_ARG_1="tcp:localhost:$APM_TCP_PORT_1"
    fi

    socat $SOCAT_ARG_1 $SOCAT_ARG_2 &
    GZSITL_SOCATID=$!

    # Wait for gzsitl-sitl connection to be established
    sleep 2

    # Bidirectional bridge between the autopilot and the coav_gcs
    SOCAT_ARG_2="udp:localhost:$COAV_GCS_UDP_PORT"
    if [ "$AUTOPILOT" = "AP_PX4" ]; then
        SOCAT_ARG_1="udp-listen:$PX4_UDP_PORT_2"
    elif [ "$AUTOPILOT" = "AP_APM" ]; then
        SOCAT_ARG_1="tcp:localhost:$APM_TCP_PORT_2"
    fi

    socat $SOCAT_ARG_1 $SOCAT_ARG_2 &
    COAV_SOCATID=$!

    # Wait for child process
    wait
}

silentkill () {
    if [ ! -z $2 ]; then
        kill $2 $1 > /dev/null 2>&1 || true
    else
        kill -KILL $1 > /dev/null 2>&1 || true
    fi
}

cleanup () {
    silentkill $GZSITL_SOCATID # Kill gzsitl socat
    silentkill $COAV_SOCATID # Kill coav_gcs socat
    silentkill $COAVID # Kill coav-control sitl
    silentkill $SITLID # Kill sitl
    silentkill $GZID -INT && sleep 3 # Wait gzserver to save the log
    silentkill $GZID  # Kill gzserver
}

test_dep () {
    command -v $1 > /dev/null 2>&1 || { echo >&2 "Error: command '$1' not found"; exit 1; }
}

check_deps () {
    test_dep gzserver
    test_dep socat
    test_dep wget
    if [ "$AUTOPILOT" = "AP_APM" ]; then
        test_dep $APM_CMD
    fi
}

test_dir () {
    [ -d $1 ] > /dev/null 2>&1 || { echo >&2 "Error: directory '$1' not found"; exit 1; }
}

check_dirs () {
    test_dir $SCRIPT_DIR
    if [ "$AUTOPILOT" = "AP_PX4" ]; then
        test_dir $PX4_DIR
    fi
}

trap cleanup EXIT
trap "exit 0" SIGHUP SIGINT SIGTERM

check_deps
check_dirs

if [ ! -d ${LOGDIR} ]; then
    mkdir ${LOGDIR}
fi

simulate "$@"
