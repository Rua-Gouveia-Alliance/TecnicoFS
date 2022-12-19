#!/bin/bash

# Defining colors
RES=$(tput sgr0)
RED=$(tput setaf 1)$(tput bold)
GREEN=$(tput setaf 2)$(tput bold)
FAILED="${RED}TEST FAILED$RES"
PASSED="${GREEN}TEST PASSED$RES"
COUNT=0
MAX=1000

while [ $COUNT -lt $MAX ]
do
	{ ./run_tests.sh 2>&1 >/dev/null | grep -q "Aborted"; } && { echo "$FAILED" ; exit; }
   ((COUNT=COUNT+1))
done
echo "$PASSED"