#!/bin/bash
FILE=proceso1.out
make $FILE
if test -f "./$FILE"; then
    valgrind --tool=helgrind ./$FILE
fi
