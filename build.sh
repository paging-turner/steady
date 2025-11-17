#!/usr/bin/env sh

mkdir -p build

clang -g -Wno-switch -o build/steady.out test_list.c
