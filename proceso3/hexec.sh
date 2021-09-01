#!/bin/bash
FILE=proceso3.out
make $FILE
if test -f "./$FILE"; then
    valgrind --tool=helgrind ./$FILE
fi
