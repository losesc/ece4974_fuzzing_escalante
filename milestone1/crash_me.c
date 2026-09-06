#include <stdio.h>
#include <string.h>
#include <unistd.h>
int main(void) {
    char b[64] = {0};
    int n = read(0, b, sizeof(b) - 1);
    if (n >= 3 && b[0]=='A' && b[1]=='F' && b[2]=='L') {
        volatile char *p = NULL;
        *p = 'x';
    }
    return 0;
}
