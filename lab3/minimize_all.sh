#!/usr/bin/env bash
# afl-tmin EVERY reproducible crash, then re-bucket the minimized file.
NAME=$1; BIN=$2; T=tri_$NAME; mkdir -p $T/min_all
key() { grep -E '^ *#[0-9]+ ' | head -3 | sed -E 's/^ *#[0-9]+ 0x[0-9a-f]+ in //; s/:[0-9]+$//; s#/home/[^ ]*/##; s/ \(.*\)$//' | tr '\n' '|'; }
echo "file,orig_bytes,min_bytes,orig_bucket,min_bucket" > $T/min_all.csv
awk -F, '$3=="yes"{print $1","$4}' $T/funnel.csv | while IFS=, read f b; do
  ASAN_OPTIONS=abort_on_error=1:symbolize=0:detect_leaks=0 afl-tmin -i $T/all/$f -o $T/min_all/$f -m none -t 2000 -- $BIN >/dev/null 2>&1
  nb=$(ASAN_OPTIONS=symbolize=1:detect_leaks=0 $BIN < $T/min_all/$f 2>&1 | key | md5sum | cut -c1-8)
  echo "$f,$(stat -c%s $T/all/$f),$(stat -c%s $T/min_all/$f),$b,$nb" >> $T/min_all.csv
done
awk -F, 'NR>1{n++; o+=$2; m+=$3; if($4!=$5)c++} END{printf "%s: %d minimized, avg %d -> %d bytes, %d changed bucket\n","'$NAME'",n,o/n,m/n,c}' $T/min_all.csv
