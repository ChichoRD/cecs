#include "cecs_texture_bank.h"

cecs_texture_bank *cecs_texture_bank_find_allocated_or_null(
    cecs_graphics_world *world,
    cecs_arena *iteration_arena,
    const cecs_texture_bank_id_descriptor bank_descriptor,
    const uint_fast8_t required_slots_count,
    cecs_entity_id *out_bank_entity_id,
    uint_fast8_t *out_first_slot_index
) {
    cecs_texture_bank *bank = NULL;
    const cecs_component_id texture_bank_id = cecs_component_id_from_texture_resource_id_descriptor(bank_descriptor);
    CECS_COMPONENT_ITERATION_HANDLE_STRUCT(cecs_texture_bank) handle;
    cecs_component_iterator it = CECS_COMPONENT_ITERATOR_CREATE_GROUPED(&world->world.components, iteration_arena, 
        CECS_COMPONENT_GROUP_FROM_IDS(
            cecs_component_access_inmmutable, cecs_component_group_search_all, CECS_RELATION_ID(cecs_texture_bank, texture_bank_id)
        )
    );
    for (
        cecs_component_iterator_begin_iter(&it, iteration_arena);
        !cecs_component_iterator_done(&it);
        cecs_component_iterator_next(&it)
    ) {
        const cecs_entity_id entity = cecs_component_iterator_current(&it, (void **)&handle);
        bank = handle.cecs_texture_bank_component;
        assert(!cecs_texture_bank_is_full(bank) && "fatal error: texture bank is full, tag mismatch");

        const uint_fast8_t slot_index = cecs_texture_bank_first_free_slot_index(bank, required_slots_count);

        extern inline uint_fast8_t cecs_texture_bank_slot_count(const cecs_texture_bank *bank);
        if (slot_index < cecs_texture_bank_slot_count(handle.cecs_texture_bank_component)) {
            *out_bank_entity_id = entity;
            *out_first_slot_index = slot_index;
            break;
        }
    }
    cecs_component_iterator_end_iter(&it);
    return bank;
}
cecs_texture_bank *cecs_texture_bank_allocate(
    cecs_graphics_world *world,
    WGPUDevice device,
    const cecs_texture_bank_id_descriptor bank_descriptor,
    const uint32_t sample_count,
    cecs_entity_id *out_bank_entity_id
) {
    cecs_texture_bank bank = cecs_texture_bank_create(device, bank_descriptor, sample_count);

    *out_bank_entity_id = cecs_world_add_entity(&world->world);
    return cecs_world_set_component_relation(
        &world->world,
        *out_bank_entity_id,
        CECS_COMPONENT_ID(cecs_texture_bank),
        &bank,
        sizeof(cecs_texture_bank),
        cecs_component_id_from_texture_resource_id_descriptor(bank_descriptor)
    );
}

cecs_texture_bank_status cecs_texture_bank_use(
    cecs_texture_bank *bank,
    const uint_fast8_t first_slot_index,
    const uint_fast8_t slot_count
) {
    const cecs_texture_bank_slot_mask mask = cecs_texture_bank_slot_mask_from_range(first_slot_index, slot_count);
    bank->used_slots_mask |= mask;
    return cecs_texture_bank_is_full(bank) ? cecs_texture_bank_status_full : cecs_texture_bank_status_free;
}
cecs_texture_bank_status cecs_texture_bank_release(
    cecs_texture_bank *bank,
    const uint_fast8_t first_slot_index,
    const uint_fast8_t slot_count
) {
    const cecs_texture_bank_slot_mask mask = cecs_texture_bank_slot_mask_from_range(first_slot_index, slot_count);
    bank->used_slots_mask &= ~mask;
    return cecs_texture_bank_is_empty(bank) ? cecs_texture_bank_status_free : cecs_texture_bank_status_free;
}
cecs_texture_bank *cecs_texture_bank_use_and_relocate(
    cecs_graphics_world *world,
    cecs_texture_bank *bank,
    cecs_texture_bank_id_descriptor bank_descriptor,
    const cecs_entity_id bank_entity_id,
    const uint_fast8_t first_slot_index,
    const uint_fast8_t slot_count
) {
    cecs_texture_bank *new_bank = bank;
    if (cecs_texture_bank_use(bank, first_slot_index, slot_count) == cecs_texture_bank_status_full) {
        cecs_world_remove_component_relation(
            &world->world,
            bank_entity_id,
            CECS_COMPONENT_ID(cecs_texture_bank),
            bank,
            cecs_component_id_from_texture_resource_id_descriptor(bank_descriptor)
        );

        bank_descriptor.flags.slots_full = cecs_texture_bank_status_full;
        new_bank = cecs_world_set_component_relation(
            &world->world,
            bank_entity_id,
            CECS_COMPONENT_ID(cecs_texture_bank),
            bank,
            sizeof(cecs_texture_bank),
            cecs_component_id_from_texture_resource_id_descriptor(bank_descriptor)
        );
        *bank = (cecs_texture_bank){0};
    }
    return new_bank;
}
cecs_texture_bank *cecs_texture_bank_release_and_relocate(
    cecs_graphics_world *world,
    cecs_texture_bank *bank,
    cecs_texture_bank_id_descriptor bank_descriptor,
    const cecs_entity_id bank_entity_id,
    const uint_fast8_t first_slot_index,
    const uint_fast8_t slot_count
) {
    cecs_texture_bank *new_bank = bank;
    if (cecs_texture_bank_release(bank, first_slot_index, slot_count) == cecs_texture_bank_status_free) {
        cecs_world_remove_component_relation(
            &world->world,
            bank_entity_id,
            CECS_COMPONENT_ID(cecs_texture_bank),
            bank,
            cecs_component_id_from_texture_resource_id_descriptor(bank_descriptor)
        );

        bank_descriptor.flags.slots_full = cecs_texture_bank_status_free;
        new_bank = cecs_world_set_component_relation(
            &world->world,
            bank_entity_id,
            CECS_COMPONENT_ID(cecs_texture_bank),
            bank,
            sizeof(cecs_texture_bank),
            cecs_component_id_from_texture_resource_id_descriptor(bank_descriptor)
        );
        *bank = (cecs_texture_bank){0};
    }
    return new_bank;
}
