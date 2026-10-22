cd ~/ece4974/lab2
S=seeds_min
D=/usr/local/share/afl/dictionaries/png.dict
E='export ASAN_OPTIONS=abort_on_error=1:halt_on_error=1:symbolize=0:detect_leaks=0; export UBSAN_OPTIONS=abort_on_error=1:halt_on_error=1:symbolize=0'
H6=21600; H8=28800; H24=86400
mkdir -p campaign
tmux new-session -d -s lab2 -n ref   "$E; afl-fuzz -i $S -o campaign/ref  -m none -t 1000 -V $H24 -x $D -- ./fuzz_v2_new; exec zsh"
tmux new-window  -t lab2   -n ndA    "$E; for r in 1 2 3; do afl-fuzz -i $S -o campaign/nd_\$r -m none -t 1000 -V $H6 -- ./fuzz_v2_new; done; exec zsh"
tmux new-window  -t lab2   -n mix    "$E; afl-fuzz -i $S -o campaign/nd_4 -m none -t 1000 -V $H6 -- ./fuzz_v2_new; afl-fuzz -i $S -o campaign/nd_5 -m none -t 1000 -V $H6 -- ./fuzz_v2_new; afl-fuzz -i $S -o campaign/dd_1 -m none -t 1000 -V $H6 -x $D -- ./fuzz_v2_new; exec zsh"
tmux new-window  -t lab2   -n ctrlD  "$E; afl-fuzz -i $S -o campaign/ctrl -m none -t 1000 -V $H8 -x $D -- ./fuzz_v2_old; for r in 2 3 4; do afl-fuzz -i $S -o campaign/dd_\$r -m none -t 1000 -V $H6 -x $D -- ./fuzz_v2_new; done; exec zsh"
