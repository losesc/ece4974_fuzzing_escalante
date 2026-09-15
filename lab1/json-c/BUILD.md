# Build commands — json-c 0.15 (finding F1)

Target: https://github.com/json-c/json-c @ tag `json-c-0.15-20200726`
Toolchain: Kali Linux, clang/LLVM 21.1.8

## 1. Clone + checkout
```bash
git clone https://github.com/json-c/json-c.git
cd json-c
git checkout json-c-0.15-20200726
```

## 2. Release baseline (verify it works before instrumenting)
```bash
mkdir build-release && cd build-release
cmake -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang \
  -DBUILD_SHARED_LIBS=OFF ..
make -j$(nproc)
./apps/json_parse ../tests/valid.json          # parses correctly
nm -C ./apps/json_parse | grep -c __asan        # 0 — no sanitizer in baseline
cd ..
```

## 3. Instrumented build (ASan + UBSan)
```bash
mkdir build-asan && cd build-asan
cmake -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang \
  -DBUILD_SHARED_LIBS=OFF -DDISABLE_WERROR=ON \
  -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize-address-use-after-scope -g -O1" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined" ..
make -j$(nproc) 2>&1 | tee build.log
cd ..
```

Note: `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` is required because modern cmake
dropped support for the old `cmake_minimum_required` version this release ships.

## 4. Compile the F1 PoC against the instrumented static lib
```bash
B=build-asan
clang -fsanitize=address,undefined -fno-omit-frame-pointer -g -O1 \
  -I"$B" -I"$B/include" -I. \
  trigger.c "$B/libjson-c.a" -o trigger
nm -C ./trigger | grep -c __asan     # 698  (ldd shows 0 — runtime linked statically)
```

## 5. Run
```bash
export ASAN_OPTIONS="detect_leaks=0:print_stacktrace=1"
export UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1"
./trigger > findings/F1_new_array_ext.txt 2>&1
```

## Corpus run (methodology — parser came back clean)
```bash
python3 make_corpus.py            # 137 from tests/test_parse.c + 2 fixtures + 11 edge = 150
# harness compiled the same way as trigger.c above, from drive_json.c
```
