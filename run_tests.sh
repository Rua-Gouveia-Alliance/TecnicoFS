#!/bin/sh

for file in tests/*
do
    if [[ $file != *.c && $file != *.txt ]]; then
        $file
    fi
done
