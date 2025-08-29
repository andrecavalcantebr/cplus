#!/bin/bash
# do_tests.sh
# Uso:
#   ./do_tests.sh               # roda passed e failed
#   ./do_tests.sh -p            # só passed
#   ./do_tests.sh -f            # só failed
#   ./do_tests.sh -p -f         # explicitamente ambos
#   ./do_tests.sh -p -o out.txt # redireciona saída para arquivo
#   ./do_tests.sh -h            # ajuda

shopt -s nullglob

run_passed=0
run_failed=0
out_file=""

print_help() {
  cat <<'EOF'
Uso:
  do_tests.sh [opções]

Opções:
  -p            Rodar apenas testes "passed"
  -f            Rodar apenas testes "failed"
  -o <arquivo>  Redirecionar toda a saída (stdout+stderr) para <arquivo>
  -h            Mostrar esta ajuda

Sem -p/-f, o script roda ambos os grupos (passed e failed).
EOF
}

# Parse das opções
while getopts ":pfo:h" opt; do
  case "$opt" in
    p) run_passed=1 ;;
    f) run_failed=1 ;;
    o) out_file="$OPTARG" ;;
    h) print_help; exit 0 ;;
    \?) echo "Opção inválida: -$OPTARG" >&2; print_help; exit 2 ;;
    :)  echo "Faltou argumento para -$OPTARG" >&2; print_help; exit 2 ;;
  esac
done

# Padrão: se nenhuma das flags foi passada, roda ambos
if [ $run_passed -eq 0 ] && [ $run_failed -eq 0 ]; then
  run_passed=1
  run_failed=1
fi

# Redirecionamento opcional
if [ -n "$out_file" ]; then
  exec >"$out_file" 2>&1
fi

# Funções auxiliares
run_passed_tests() {
  echo "=== Running PASSED tests ==="
  local found=0
  for f in ./tests/passed/*.cplus.h ./tests/passed/*.cplus; do
    found=1
    echo "==== $f ===="
    ./parser/parser -f "$f"
    echo
  done
  [ $found -eq 1 ] || echo "(nenhum arquivo em ./tests/passed)"
}

run_failed_tests() {
  echo "=== Running FAILED tests ==="
  local found=0
  for f in ./tests/failed/*.cplus.h ./tests/failed/*.cplus; do
    found=1
    echo "==== $f ===="
    if ./parser/parser -f "$f"; then
      echo "❌ Expected FAIL, but got SUCCESS"
    else
      echo "✅ Correctly failed"
    fi
    echo
  done
  [ $found -eq 1 ] || echo "(nenhum arquivo em ./tests/failed)"
}

# Execução
[ $run_passed -eq 1 ] && run_passed_tests
[ $run_failed -eq 1 ] && run_failed_tests
