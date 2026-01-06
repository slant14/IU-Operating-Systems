#!/bin/bash
set -euo pipefail

mkdir -p /tmp/ex2
gcc -O2 -Wall -Wextra pager.c -o pager
gcc -O2 -Wall -Wextra mmu.c -o mmu

run_case() {
  local pages="$1"
  local frames="$2"
  local algo="$3"

  : > /tmp/ex2/pagetable

  ./pager "$pages" "$frames" "$algo" &
  local pid_pager=$!

  sleep 0.1

  local commands="R0 R1 R0 W1 R0 R1 R0 W1 R0 R2 R0 W2 R0 R2 R0 W2 R0 R3 R0 W3 R0 R3 R0 W3 R0 R4 R0 W4 R0 R4 R0 W4"
  ./mmu "$pages" $commands "$pid_pager"

  wait "$pid_pager" || true
}

run_case 5  3 random
run_case 10 3 nfu
run_case 10 6 aging
run_case 15 5 aging

