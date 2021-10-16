#!/bin/bash

# Script for internal debugging/startup

CONF=release

cd ${APROJECTS}/build-littlenavconnect-${CONF}

export LD_LIBRARY_PATH=~/Qt/5.15.2/gcc_64/lib:${APROJECTS}/build-littlenavconnect-${CONF}

${APROJECTS}/build-littlenavconnect-${CONF}/littlenavconnect $@
