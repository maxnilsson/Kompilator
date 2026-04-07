#!/usr/bin/env sh

set -u

VERBOSE=0
MAX_FAIL_LINES=35

while [ "$#" -gt 0 ]; do
  case "$1" in
    -v|--verbose)
      VERBOSE=1
      ;;
    -h|--help)
      echo "Usage: sh run_all_tests.sh [--verbose]"
      echo "  --verbose, -v   Show full compiler output for every file"
      exit 0
      ;;
    *)
      echo "Unknown option: $1"
      echo "Use --help for usage."
      exit 2
      ;;
  esac
  shift
done

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT_DIR"

if [ -t 1 ]; then
  C_RESET='\033[0m'
  C_BOLD='\033[1m'
  C_BLUE='\033[34m'
  C_GREEN='\033[32m'
  C_YELLOW='\033[33m'
  C_RED='\033[31m'
else
  C_RESET=''
  C_BOLD=''
  C_BLUE=''
  C_GREEN=''
  C_YELLOW=''
  C_RED=''
fi

print_header() {
  printf "\n${C_BOLD}${C_BLUE}========================================${C_RESET}\n"
  printf "${C_BOLD}${C_BLUE}%s${C_RESET}\n" "$1"
  printf "${C_BOLD}${C_BLUE}========================================${C_RESET}\n"
}

exit_name() {
  case "$1" in
    0) echo "SUCCESS" ;;
    1) echo "LEXICAL_ERROR" ;;
    2) echo "SYNTAX_ERROR" ;;
    3) echo "AST_ERROR" ;;
    4) echo "SEMANTIC_ERROR" ;;
    139) echo "SEGFAULT" ;;
    *) echo "UNKNOWN" ;;
  esac
}

print_header "Building compiler"
if ! make compiler; then
  printf "${C_RED}${C_BOLD}Build failed. Aborting test run.${C_RESET}\n"
  exit 1
fi

TMP_LIST="$(mktemp)"
TMP_CODES="$(mktemp)"

find test_files -type f -name "*.cpm" | sort > "$TMP_LIST"
TOTAL="$(wc -l < "$TMP_LIST" | tr -d ' ')"

if [ "$TOTAL" -eq 0 ]; then
  printf "${C_YELLOW}No .cpm files found under test_files.${C_RESET}\n"
  rm -f "$TMP_LIST" "$TMP_CODES"
  exit 0
fi

SUCCESS_COUNT=0
ERROR_COUNT=0

print_header "Running ${TOTAL} input files"

idx=0
while IFS= read -r file; do
  idx=$((idx + 1))

  printf "\n${C_BOLD}${C_BLUE}[%d/%d] %s${C_RESET}\n" "$idx" "$TOTAL" "$file"

  output="$(./compiler "$file" 2>&1)"
  code=$?
  code_name="$(exit_name "$code")"

  printf "%s\n" "$code" >> "$TMP_CODES"

  if [ "$code" -eq 0 ]; then
    SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
    printf "${C_GREEN}status: %s (exit %d)${C_RESET}\n" "$code_name" "$code"
  else
    ERROR_COUNT=$((ERROR_COUNT + 1))
    printf "${C_RED}status: %s (exit %d)${C_RESET}\n" "$code_name" "$code"
  fi

  if [ -n "$output" ]; then
    if [ "$VERBOSE" -eq 1 ]; then
      printf "%s\n" "$output" | sed 's/^/  | /'
    elif [ "$code" -ne 0 ]; then
      printf "${C_YELLOW}  showing last %d log lines (use --verbose for full output)${C_RESET}\n" "$MAX_FAIL_LINES"
      printf "%s\n" "$output" | tail -n "$MAX_FAIL_LINES" | sed 's/^/  | /'
    fi
  fi
done < "$TMP_LIST"

print_header "Summary"
printf "${C_BOLD}Total files:${C_RESET}   %d\n" "$TOTAL"
printf "${C_GREEN}${C_BOLD}Exit 0:${C_RESET}       %d\n" "$SUCCESS_COUNT"
printf "${C_RED}${C_BOLD}Nonzero exits:${C_RESET} %d\n" "$ERROR_COUNT"

printf "\n${C_BOLD}Exit code breakdown:${C_RESET}\n"
sort -n "$TMP_CODES" | uniq -c | while IFS= read -r line; do
  set -- $line
  count="$1"
  code="$2"
  printf "  exit %-3s %-16s -> %d file(s)\n" "$code" "$(exit_name "$code")" "$count"
done

printf "\n${C_YELLOW}Done.${C_RESET}\n"

rm -f "$TMP_LIST" "$TMP_CODES"
