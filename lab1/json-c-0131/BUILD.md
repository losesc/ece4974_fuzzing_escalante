# Build commands - json-c 0.13.1 ( F2, F3)

Target: https://github.com/json-c/json-c @ tag `json-c-0.13.1-20180305`
Toolchain: Kali Linux, clang/LLVM 21.1.8

## 1. Clone + checkout
```bash
git clone https://github.com/json-c/json-c.git json-c-0131
cd json-c-0131
git checkout json-c-0.13.1-20180305
```

## 2. Instrumented build (ASan + UBSan)
```bash
mkdir build-asan && cd build-asan
cmake -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang \
  -DBUILD_SHARED_LIBS=OFF -DDISABLE_WERROR=ON \
  -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize-address-use-after-scope -g -O1 -Wno-implicit-const-int-float-conversion -Wno-deprecated-non-prototype -Wno-unused-but-set-variable" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined" ..
make -j$(nproc) 2>&1 | tee build.log
cd ..
```

Two flags are specific to this older release
- `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` - modern cmake dropped the old
  `cmake_minimum_required` version this release declares.
- `-Wno-implicit-const-int-float-conversion` (and the two others) - 0.13.x
  appends `-Werror` *after* CMAKE_C_FLAGS, so `-DDISABLE_WERROR=ON` alone is not
  enough; the specific warnings clang 21 raises must be silenced so the appended
  `-Werror` has nothing to promote.

## 3. Compile the PoCs against the instrumented static lib
```bash
B=build-asan
clang -fsanitize=address,undefined -fno-omit-frame-pointer -g -O1 \
  -I"$B" -I"$B/include" -I. poc_F2_del_idx.c "$B/libjson-c.a" -o poc_F2
clang -fsanitize=address,undefined -fno-omit-frame-pointer -g -O1 \
  -I"$B" -I"$B/include" -I. poc_F3_neg_depth.c "$B/libjson-c.a" -o poc_F3
nm -C ./poc_F2 | grep -c __asan     # 698  (ldd shows 0)
```

## 4. Run
```bash
export ASAN_OPTIONS="detect_leaks=0:print_stacktrace=1"
export UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1"
./poc_F2 > findings/F2_array_list_del_idx.txt 2>&1
./poc_F3 > findings/F3_tokener_negative_depth.txt 2>&1
```
