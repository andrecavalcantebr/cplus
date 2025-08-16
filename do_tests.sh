#!/bin/bash

for f in ./tests/*.cplus.h; do
  echo "==== $f ===="
  ./parser/parser "$f"
  echo
 done
