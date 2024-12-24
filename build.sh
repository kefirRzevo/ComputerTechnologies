#!/bin/bash

if [ -z "$1" ]; then
    echo "Usage: $0 <directory>"
    exit 1
fi

if [ ! -d "$1" ]; then
    echo "Error: Directory '$1' not found"
    exit 1
fi

find "$1" -type d -exec sh -c '
    cd "$1" || exit
    for file in *.c; do
        if [ -f "$file" ]; then
            filename=$(basename "$file" .c)
            gcc "$file" -o "$filename"
        fi
    done' sh {} \;
