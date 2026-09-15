#include <stdio.h>
#include <json.h>
#include "arraylist.h"
int main(void){
    struct array_list *a = array_list_new(NULL);
    array_list_add(a, (void*)1);
    int r = array_list_del_idx(a, 1, (size_t)-1);   /* stop = 1 + SIZE_MAX = 0, guard bypassed */
    printf("del_idx returned %d\n", r);
    array_list_free(a);
    return 0;
}
