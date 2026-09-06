#include <stdlib.h>
int main(int argc, char **argv) {
    int a[10] = {0};
    int i = argc + 9;        // 10 at runtime, opaque to the optimizer
    return a[i];
}
