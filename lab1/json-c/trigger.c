#include <limits.h>
#include <json.h>
int main(void) {
    struct json_object *a = json_object_new_array_ext(INT_MIN + 1);
    if (a) json_object_put(a);
    return 0;
}
