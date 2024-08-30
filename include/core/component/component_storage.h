#ifndef COMPONENT_STORAGE_H
#define COMPONENT_STORAGE_H

#include <assert.h>
#include "../../containers/list.h"
#include "../../containers/bitset.h"
#include "../../containers/arena.h"
#include "../entity.h"
#include "component.h"

#define COMPONENT_REGISTRATION_ID_TYPE size_t
typedef COMPONENT_REGISTRATION_ID_TYPE component_registration_id;

typedef struct component_storage {
    list components;
    list entity_ids;
    hibitset entity_bitset;
} component_storage;

component_storage component_storage_create(arena *a, size_t component_capacity, size_t component_size) {
    return (component_storage) {
        .components = list_create(a, component_size * component_capacity),
        .entity_ids = list_create(a, sizeof(entity_id) * component_capacity),
        .entity_bitset = hibitset_create(a)
    };
}

bool component_storage_has(const component_storage *cs, entity_id entity_id) {
    //hibitset_bit_range
    return hibitset_is_set(&cs->entity_bitset, entity_id);
}

void *component_storage_get(const component_storage *cs, component_registration_id component_index, size_t component_size) {
    assert(component_index < list_count(&cs->components) && "A component with the given index does not exist");
    return list_get(&cs->components, component_index, component_size);
}
#define COMPONENT_STORAGE_GET(type, component_storage_ref, component_index) \
    ((type *)component_storage_get(component_storage_ref, component_index, sizeof(type)))

component_registration_id component_storage_add(component_storage *cs, arena *a, entity_id entity_id, void *component, size_t component_size) {
    assert(!component_storage_has(cs, entity_id) && "An entity with the given ID already has a component");
    list_add(&cs->components, a, component, component_size);
    list_add(&cs->entity_ids, a, &entity_id, sizeof(entity_id));
    hibitset_set(&cs->entity_bitset, a, entity_id);
    return list_count_of_size(&cs->components, component_size) - 1;
}

component_registration_id component_storage_set(component_storage *cs, arena *a, entity_id entity_id, component_registration_id component_index, void *component, size_t component_size) {
    if (component_storage_has(cs, entity_id)) {
        list_set(&cs->components, component_index, component, component_size);
        // TODO: See if we want this
        //list_set(&cs->entity_ids, component_index, &entity_id, sizeof(entity_id));
        assert(LIST_GET(component_registration_id, &cs->entity_ids, component_index) == entity_id);
        return component_index;
    } else {
        return component_storage_add(cs, a, entity_id, component, component_size);
    }
}

entity_id component_storage_remove(component_storage *cs, arena *a, component_registration_id component_index, size_t component_size) {
    assert(component_index < list_count(&cs->components) && "A component with the given index does not exist");
    list_remove_swap_last(&cs->components, a, component_index, component_size);

    entity_id removed_id = *LIST_GET(entity_id, &cs->entity_ids, component_index);
    list_remove_swap_last(&cs->entity_ids, a, component_index, sizeof(entity_id));

    hibitset_unset(&cs->entity_bitset, a, removed_id);
    return removed_id;
}

#endif