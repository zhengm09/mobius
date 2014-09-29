#!/bin/bash

# How long should we run (in seconds)?
TIME=7200

# Number of routers
ROUTERS=20

# Number of total clients
CLIENTS=400

# Number of bulk clients
BULK_CLIENTS=320

tools/exp.pl stop
sleep 1
tools/exp.pl start ${ROUTERS} ${CLIENTS} ${BULK_CLIENTS} &
sleep ${TIME}
tools/exp.pl stop
tools/exp.pl data
