#!/bin/bash
FILE=proceso2.out
make $FILE
if test -f "./$FILE"; then
    valgrind --tool=helgrind ./$FILE
fi
