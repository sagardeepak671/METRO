#!/bin/bash

# run2.sh - Reads test.satoutput and produces test.metromap
# Usage: ./run2.sh test

if [ $# -eq 0 ]; then
    echo "Usage: ./run2.sh <filename>"
    exit 1
fi

filename=$1

# Check if required files exist
if [ ! -f "${filename}.city" ]; then
    echo "Error: ${filename}.city not found"
    exit 1
fi

if [ ! -f "${filename}.satoutput" ]; then
    echo "Error: ${filename}.satoutput not found"
    exit 1
fi

# Run decoder with filename argument
./decoder "$filename"
