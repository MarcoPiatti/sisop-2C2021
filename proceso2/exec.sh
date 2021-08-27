#!/bin/bash
FILE=proceso
make $FILE
if test -f "./$FILE"; then
    ./$FILE
fi