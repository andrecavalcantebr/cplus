#!/bin/bash

# Se tiver parâmetro, redireciona tudo (stdout+stderr) para o arquivo dado
if [ $# -gt 0 ]; then
  exec >"$1" 2>&1
fi

for f in ./tests/*.cplus.h; do
  echo "==== $f ===="
  ./parser/parser -f "$f"
  echo
done
