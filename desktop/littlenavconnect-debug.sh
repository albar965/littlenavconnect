#!/bin/bash

# Script for internal debugging/startup
if [ -f ~/.projects ]; then
  source ~/.projects
fi

CONF=debug

cd ${APROJECTS}/build-littlenavconnect-${CONF}

export LD_LIBRARY_PATH=~/Qt/${QT_VERSION}/gcc_64/lib:${APROJECTS}/build-littlenavconnect-${CONF}

${APROJECTS}/build-littlenavconnect-${CONF}/littlenavconnect --replay-gui "$@"
