#!/bin/bash

export LD_LIBRARY_PATH=~/Qt/5.9.5/gcc_64/lib

cd ~/Projekte/build-littlenavconnect-debug
~/Projekte/build-littlenavconnect-debug/littlenavconnect $@
