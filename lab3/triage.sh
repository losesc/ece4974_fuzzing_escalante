#!/usr/bin/env bash
# Lab 3 triage funnel: collect -> reproduce -> minimize -> dedup (top-3 stack hash).
#   ./triage.sh <name> <replay_binary> <crash_dir>...
set -uo pipefail
NAME=$1; BIN=$2; shift 2
OUT=tri_$NAME; rm -rf "$OUT"; mkdir -p "$OUT"/{all,min,reports}
export ASAN_OPTIONS="abort_on_error=1:halt_on_error=1:symbolize=1:detect_leaks=0"
export UBSAN_OPTIONS="abort_on_error=1:halt_on_error=1:symbolize=1:print_stacktrace=1"
i=0; for d in "$@"; do i=$((i+1)); for f in "$d"/id:*; do [ -f "$f" ] && cp "$f" "$OUT/all/s${i}_$(basename "$f" | cut -d, -f1)"; done; done
RAW=$(ls "$OUT/all" | wc -l)
key() { grep -E '^ *#[0-9]+ ' | head -3 | sed -E 's/^ *#[0-9]+ 0x[0-9a-f]+ in //; s/:[0-9]+$//; s#/home/[^ ]*/##; s/ \(.*\)$//' | tr '\n' '|'; }
echo "file,exit,repro,bucket,class,top3" > "$OUT/funnel.csv"
for f in "$OUT"/all/*; do
  ok=1; for t in 1 2 3; do "$BIN" < "$f" > "$OUT/reports/$(basename "$f").txt" 2>&1; rc=$?; [ $rc -ge 128 ] || ok=0; done
  rep="$OUT/reports/$(basename "$f").txt"
  if [ $ok = 1 ]; then
    k=$(key < "$rep"); b=$(printf %s "$k" | md5sum | cut -c1-8)
    cls=$(grep -m1 -oE 'AddressSanitizer: [a-z-]+|runtime error: [^:]+' "$rep" | head -1)
    echo "$(basename "$f"),$rc,yes,$b,\"$cls\",\"$k\"" >> "$OUT/funnel.csv"
  else echo "$(basename "$f"),$rc,no,,," >> "$OUT/funnel.csv"; fi
done 2>/dev/null
REPRO=$(grep -c ',yes,' "$OUT/funnel.csv")
for b in $(awk -F, '$3=="yes"{print $4}' "$OUT/funnel.csv" | sort -u); do
  rep=$(awk -F, -v b=$b '$4==b{print $1}' "$OUT/funnel.csv" | while read n; do echo "$(stat -c%s "$OUT/all/$n") $n"; done | sort -n | head -1 | cut -d' ' -f2)
  ASAN_OPTIONS="${ASAN_OPTIONS/symbolize=1/symbolize=0}" afl-tmin -i "$OUT/all/$rep" -o "$OUT/min/$b.png" -m none -t 2000 -- "$BIN" >/dev/null 2>&1
  "$BIN" < "$OUT/min/$b.png" > "$OUT/min/$b.asan.txt" 2>&1
  same=$([ "$(key < "$OUT/min/$b.asan.txt" | md5sum | cut -c1-8)" = "$b" ] && echo same || echo CHANGED)
  echo "$b $rep $(stat -c%s "$OUT/all/$rep")B -> $(stat -c%s "$OUT/min/$b.png")B bucket:$same" >> "$OUT/minimize.txt"
done 2>/dev/null
UNIQ=$(awk -F, '$3=="yes"{print $4}' "$OUT/funnel.csv" | sort -u | wc -l)
{ echo "== $NAME: $RAW raw -> $REPRO reproducible ($(( RAW ? 100*REPRO/RAW : 0 ))%) -> $UNIQ unique (top-3 stack)"
  echo; echo "count bucket   class / top-3 frames"
  awk -F, '$3=="yes"{print $4" "$5" "$6}' "$OUT/funnel.csv" | sort | uniq -c | sort -rn
  echo; echo "minimized reproducers:"; cat "$OUT/minimize.txt" 2>/dev/null; } | tee "$OUT/summary.txt"
