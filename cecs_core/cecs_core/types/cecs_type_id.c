#include "cecs_type_id.h"

static cecs_type_id cecs_type_id_count = 0;

cecs_type_id *cecs_type_id_counter(void) {
    return &cecs_type_id_count;
}
