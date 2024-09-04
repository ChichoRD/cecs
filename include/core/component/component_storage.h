#ifndef COMPONENT_STORAGE_H
#define COMPONENT_STORAGE_H

#include <assert.h>
#include <stdlib.h>
#include "../../containers/list.h"
#include "../../containers/bitset.h"
#include "../../containers/range.h"
#include "../../containers/arena.h"
#include "../../containers/tagged_union.h"
#include "entity/entity.h"

#define COMPONENT(type) CAT2(type, _component)
#define _COMPONENT_IMPLEMENT(component) TYPE_ID_IMPLEMENT_COUNTER(component, component_id_count)
#define COMPONENT_IMPLEMENT(type) _COMPONENT_IMPLEMENT(COMPONENT(type))

#define COMPONENT_ID(type) ((component_id)TYPE_ID(COMPONENT(type)))
#define COMPONENT_ID_ARRAY(...) (component_id[]){ MAP(COMPONENT_ID, COMMA, __VA_ARGS__) }
#define COMPONENT_COUNT(...) (sizeof(COMPONENT_ID_ARRAY(__VA_ARGS__)) / sizeof(component_id))

#define COMPONENT_MASK(type) ((component_mask)(1 << COMPONENT_ID(type)))

#define _COMPONENT_MASK_OR(mask, next_mask) (mask | next_mask)
#define COMPONENTS_MASK(...) (FOLD_L1(_COMPONENT_MASK_OR, ((component_mask)0), MAP(COMPONENT_MASK, COMMA, __VA_ARGS__)))


typedef OPTION_STRUCT(void *, component) optional_component;

typedef void *get_component(const void *self, entity_id id, size_t size);
typedef void *set_component(const void *self, arena *a, entity_id id, void *component, size_t size);
typedef optional_component remove_component(const void *self, arena *a, entity_id id, size_t size);
typedef struct component_storage_header {
    get_component *const get;
    set_component *const set;
    remove_component *const remove;
} component_storage_header;


typedef struct sparse_component_storage {
    component_storage_header header;
    list components;
    exclusive_range entity_id_range;
} sparse_component_storage;

bool sparse_component_storage_is_empty(const sparse_component_storage *self) {
    return exclusive_range_is_empty(self->entity_id_range);
}

void *sparse_component_storage_get(const sparse_component_storage *self, entity_id id, size_t size) {
    assert(exclusive_range_contains(self->entity_id_range, id) && "Entity ID out of bounds");
    size_t list_index = (entity_id)id - self->entity_id_range.start;
    return list_get(&self->components, list_index, size);
}


void *sparse_component_storage_expand(sparse_component_storage *self, arena *a, entity_id id, size_t size) {
    if (sparse_component_storage_is_empty(self)) {
        self->entity_id_range = exclusive_range_singleton(id);
        return list_append_empty(&self->components, a, 1, size);
    } else {
        exclusive_range expanded_range = exclusive_range_from(
            range_union(self->entity_id_range.range, exclusive_range_singleton(id).range)
        );

        range_splitting expansion_ranges = exclusive_range_difference(
            expanded_range,
            self->entity_id_range
        );

        exclusive_range range0 = exclusive_range_from(expansion_ranges.ranges[0]);
        exclusive_range range1 = exclusive_range_from(expansion_ranges.ranges[1]);
        self->entity_id_range = expanded_range;
        if (!exclusive_range_is_empty(range0)) {
            size_t missing_count = exclusive_range_length(range0);
            return list_prepend_empty(&self->components, a, missing_count, size);
        } else if (!exclusive_range_is_empty(range1)) {
            size_t missing_count = exclusive_range_length(range1);
            return list_append_empty(&self->components, a, missing_count, size);
        } else {
            assert(false && "unreachable: should not have called expand with an id within bounds");
            return list_get(&self->components, id - self->entity_id_range.start, size);
        }
    }
}

void *sparse_component_storage_set(sparse_component_storage *self, arena *a, entity_id id, void *component, size_t size) {
    if (exclusive_range_contains(self->entity_id_range, id)) {
        return list_set(&self->components, (id - self->entity_id_range.start), component, size);
    } else {
        return memcpy(sparse_component_storage_expand(self, a, id, size), component, size);
    }
}

optional_component sparse_component_storage_remove(sparse_component_storage *self, arena *a, entity_id id, size_t size) {
    if (exclusive_range_contains(self->entity_id_range, id)) {
        return OPTION_CREATE_SOME_STRUCT(component, list_get(&self->components, (id - self->entity_id_range.start), size));
    } else {
        return OPTION_CREATE_NONE_STRUCT(component);
    }
}

sparse_component_storage sparse_component_storage_create(arena *a, size_t component_capacity, size_t component_size) {
    return (sparse_component_storage) {
        .header = {
            .get = sparse_component_storage_get,
            .set = sparse_component_storage_set,
            .remove = sparse_component_storage_remove,
        },
        .components = list_create(a, component_capacity * component_size),
        .entity_id_range = { 0, 0 },
    };
}

typedef TAGGED_UNION_STRUCT(
    sparse_component_storage,
    sparse_component_storage,
) component_storage_union;

typedef struct component_storage {
    component_storage_header header;
    component_storage_union storage;
    hibitset entity_bitset;
} component_storage;

component_storage component_storage_create_sparse(arena *a, size_t component_capacity, size_t component_size) {
    sparse_component_storage storage = sparse_component_storage_create(a, component_capacity, component_size);
    return (component_storage) {
        .header = storage.header,
        .storage = TAGGED_UNION_CREATE(
            sparse_component_storage,
            storage
        ),
        .entity_bitset = hibitset_create(a),
    };
}

bool component_storage_has(const component_storage *self, entity_id id) {
    return hibitset_has(&self->entity_bitset, id);
}

const list *component_storage_components(const component_storage *self) {
    switch (self->storage.variant) {
        case TAGGED_UNION_VARIANT(sparse_component_storage):
            return &TAGGED_UNION_GET_UNCHECKED(sparse_component_storage, self->storage).components;
        default:
            {
                assert(false && "unreachable: invalid component storage variant");
                exit(EXIT_FAILURE);
                return NULL;
            }
    }
}

void *component_storage_get(const component_storage *self, entity_id id, size_t size) {
    return (self->header.get)(&self->storage, id, size);
}
#define COMPONENT_STORAGE_GET(type, component_storage_ref, entity_id) \
    ((type *)component_storage_get(component_storage_ref, entity_id, sizeof(type)))

void *component_storage_set(component_storage *self, arena *a, entity_id id, void *component, size_t size) {
    hibitset_set(&self->entity_bitset, a, id);
    return (self->header.set)(&self->storage, a, id, component, size);
}
#define COMPONENT_STORAGE_SET(type, component_storage_ref, arena_ref, entity_id, component_ref) \
    ((type *)component_storage_set(component_storage_ref, arena_ref, entity_id, component_ref, sizeof(type)))

optional_component component_storage_remove(component_storage *self, arena *a, entity_id id, size_t size) {
    hibitset_unset(&self->entity_bitset, a, id);
    return (self->header.remove)(&self->storage, a, id, size);
}
#define COMPONENT_STORAGE_REMOVE(type, component_storage_ref, arena_ref, entity_id) \
    ((type *)component_storage_remove(component_storage_ref, arena_ref, entity_id, sizeof(type)))


#endif