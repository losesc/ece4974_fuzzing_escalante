#include <stdio.h>
#include <json.h>
int main(void){
    struct json_tokener *t = json_tokener_new_ex(-1);  /* calloc(-1, ...) */
    if (!t){ printf("tok NULL\n"); return 0; }
    struct json_object *o = json_tokener_parse_ex(t, "[[[[1]]]]", 9);
    printf("parsed=%p\n", (void*)o);
    if (o) json_object_put(o);
    json_tokener_free(t);
    return 0;
}
