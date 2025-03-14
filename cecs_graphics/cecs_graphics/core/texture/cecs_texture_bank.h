#ifndef CECS_TEXTURE_BANK_H
#define CECS_TEXTURE_BANK_H

#include "../cecs_graphics_world.h"
#include "../component/cecs_texture.h"


cecs_texture_bank *cecs_texture_bank_find_allocated_or_null(
    cecs_graphics_world *world,
    cecs_arena *iteration_arena,
    const cecs_texture_bank_id_descriptor bank_descriptor,
    const uint_fast8_t required_slots_count,
    cecs_entity_id *out_bank_entity_id,
    uint_fast8_t *out_first_slot_index
);
cecs_texture_bank *cecs_texture_bank_allocate(
    cecs_graphics_world *world,
    WGPUDevice device,
    const cecs_texture_bank_id_descriptor bank_descriptor,
    const uint32_t sample_count,
    cecs_entity_id *out_bank_entity_id
);


cecs_texture_bank_status cecs_texture_bank_use(
    cecs_texture_bank *bank,
    const uint_fast8_t first_slot_index,
    const uint_fast8_t slot_count
);
cecs_texture_bank_status cecs_texture_bank_release(
    cecs_texture_bank *bank,
    const uint_fast8_t first_slot_index,
    const uint_fast8_t slot_count
);
cecs_texture_bank *cecs_texture_bank_use_and_relocate(
    cecs_graphics_world *world,
    cecs_texture_bank *bank,
    cecs_texture_bank_id_descriptor bank_descriptor,
    const cecs_entity_id bank_entity_id,
    const uint_fast8_t first_slot_index,
    const uint_fast8_t slot_count
);
cecs_texture_bank *cecs_texture_bank_release_and_relocate(
    cecs_graphics_world *world,
    cecs_texture_bank *bank,
    cecs_texture_bank_id_descriptor bank_descriptor,
    const cecs_entity_id bank_entity_id,
    const uint_fast8_t first_slot_index,
    const uint_fast8_t slot_count
);

typedef struct cecs_texture_in_bank_bundle {
    cecs_texture_in_bank_reference reference;
    cecs_texture_subrect2_f32 subrect;
    cecs_texture_in_bank_range range;
} cecs_texture_in_bank_bundle;

#endif