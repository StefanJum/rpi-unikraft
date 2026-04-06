#!/bin/bash

files=(
    "unikraft/wrk-1.txt"
    "unikraft/wrk-10.txt"
    "linux/wrk-1.txt"
    "linux/wrk-10.txt"
)

args=(
    "r"
    "rt"
    "l"
)

for file in "${files[@]}"; do
    for arg in "${args[@]}"; do
        python3 parse-wrk.py "$file" "$arg"
    done
done
