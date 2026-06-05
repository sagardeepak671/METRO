#!/bin/bash

# run1.sh - Reads test.city and produces test.satinput
# Usage: ./run1.sh test

if [ $# -eq 0 ]; then
    echo "Usage: ./run1.sh <filename>"
    exit 1
fi

filename=$1

# Check if input file exists
if [ ! -f "${filename}.city" ]; then
    echo "Error: ${filename}.city not found"
    exit 1
fi

# Run encoder with filename argument
./encoder "$filename"
