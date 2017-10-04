#!/bin/sh

# Adjust this script like shown below and remove "LD_LIBRARY_PATH=./lib" if you want to move it to another place
# cd "/home/YOURUSERNAME/Little Navconnect"
# LD_LIBRARY_PATH="/home/YOURUSERNAME/Little Navconnect/lib"

# Use subshell to keep LD_LIBRARY_PATH local
(
export LD_LIBRARY_PATH=./lib
./littlenavconnect
)
