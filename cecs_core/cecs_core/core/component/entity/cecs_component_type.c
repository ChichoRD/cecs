#include "cecs_component_type.h"

static cecs_component_id component_id_count = 0;

cecs_component_id *cecs_component_id_counter(void) {
    return &component_id_count;
}
