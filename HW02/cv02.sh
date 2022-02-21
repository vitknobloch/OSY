#!/bin/bash

for FILE in *
do
    if grep >/dev/null "echo" "$FILE"; then
        echo "$FILE" $(stat -c%s "$FILE")B
    fi

done