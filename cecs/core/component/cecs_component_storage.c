#include <memory.h>

#include "cecs_component_storage.h"

const cecs_storage_info cecs_unit_component_storage_info(const cecs_unit_component_storage* self) {
    return (cecs_storage_info) {
        .is_index_stable = true,
            .is_dense = true,
            .is_unit_type_storage = true
    };
}

cecs_optional_component cecs_unit_component_storage_get(const cecs_unit_component_storage* self, cecs_entity_id id, size_t size) {
    return CECS_OPTION_CREATE_NONE_STRUCT(cecs_optional_component);
}

cecs_optional_component cecs_unit_component_storage_set(const cecs_unit_component_storage* self, cecs_arena* a, cecs_entity_id id, void* component, size_t size) {
    return CECS_OPTION_CREATE_NONE_STRUCT(cecs_optional_component);
}

bool cecs_unit_component_storage_remove(const cecs_unit_component_storage* self, cecs_arena* a, cecs_entity_id id, void* out_removed_component, size_t size) {
    memset(out_removed_component, 0, size);
    return false;
}

cecs_unit_component_storage cecs_unit_component_storage_create(void) {
    return (cecs_unit_component_storage) {
        .phantom_data = 0
    };
}

const cecs_storage_info cecs_indirect_component_storage_info(const cecs_indirect_component_storage* self) {
    return (cecs_storage_info) {
        .is_dense = false,
            .is_index_stable = true,
            .is_unit_type_storage = false
    };
}

cecs_optional_component cecs_indirect_component_storage_get(const cecs_indirect_component_storage* self, cecs_entity_id id, size_t size) {
    if (!cecs_displaced_set_contains(&self->component_references, (size_t)id, &(void*){0}, sizeof(void*))) {
        return CECS_OPTION_CREATE_NONE_STRUCT(cecs_optional_component);
    }
    else {
        return CECS_OPTION_CREATE_SOME_STRUCT(cecs_optional_component, *CECS_DISPLACED_SET_GET(void*, &self->component_references, (size_t)id));
    }
}

cecs_optional_component cecs_indirect_component_storage_set(cecs_indirect_component_storage* self, cecs_arena* a, cecs_entity_id id, void* component, size_t size) {
    return CECS_OPTION_CREATE_SOME_STRUCT(
        cecs_optional_component,
        *CECS_DISPLACED_SET_SET(
            void*,
            &self->component_references,
            a,
            (size_t)id,
            &component
        )
    );
}

bool cecs_indirect_component_storage_remove(cecs_indirect_component_storage* self, cecs_arena* a, cecs_entity_id id, void* out_removed_component, size_t size) {
    void* removed_component = NULL;
    if (cecs_displaced_set_remove_out(
        &self->component_references,
        (size_t)id,
        &removed_component,
        sizeof(void*),
        &(void*){0}
    )) {
        memcpy(out_removed_component, removed_component, size);
        return true;
    }
    else {
        memset(out_removed_component, 0, size);
        return false;
    }
}

cecs_indirect_component_storage cecs_indirect_component_storage_create(void) {
    return (cecs_indirect_component_storage) {
        .component_references = cecs_displaced_set_create()
    };
}

const cecs_storage_info cecs_sparse_component_storage_info(const cecs_sparse_component_storage* self) {
    return (cecs_storage_info) {
        .is_index_stable = true,
            .is_dense = false,
            .is_unit_type_storage = false
    };
}

cecs_optional_component cecs_sparse_component_storage_get(const cecs_sparse_component_storage* self, cecs_entity_id id, size_t size) {
    if (!cecs_displaced_set_contains_index(&self->components, (size_t)id)) {
        return CECS_OPTION_CREATE_NONE_STRUCT(cecs_optional_component);
    }
    else {
        return CECS_OPTION_CREATE_SOME_STRUCT(cecs_optional_component, cecs_displaced_set_get(&self->components, (size_t)id, size));
    }
}

cecs_optional_component cecs_sparse_component_storage_set(cecs_sparse_component_storage* self, cecs_arena* a, cecs_entity_id id, void* component, size_t size) {
    return CECS_OPTION_CREATE_SOME_STRUCT(
        cecs_optional_component,
        cecs_displaced_set_set(&self->components, a, (size_t)id, component, size)
    );
}

bool cecs_sparse_component_storage_remove(cecs_sparse_component_storage* self, cecs_arena* a, cecs_entity_id id, void* out_removed_component, size_t size) {
    if (cecs_displaced_set_contains_index(&self->components, (size_t)id)) {
        memcpy(out_removed_component, cecs_displaced_set_get(&self->components, (size_t)id, size), size);
        return true;
    }
    else {
        memset(out_removed_component, 0, size);
        return false;
    }
}

cecs_sparse_component_storage cecs_sparse_component_storage_create(cecs_arena* a, size_t component_capacity, size_t component_size) {
    return (cecs_sparse_component_storage) {
        .components = cecs_displaced_set_create_with_capacity(a, component_capacity * component_size),
    };
}

cecs_component_storage cecs_component_storage_create_sparse(cecs_arena* a, size_t component_capacity, size_t component_size) {
    cecs_sparse_component_storage storage = cecs_sparse_component_storage_create(a, component_capacity, component_size);
    return (cecs_component_storage) {
        .storage = CECS_UNION_CREATE(
            cecs_sparse_component_storage,
            cecs_component_storage_union,
            storage
        ),
            .entity_bitset = cecs_hibitset_create(a),
    };
}

cecs_component_storage cecs_component_storage_create_unit(cecs_arena* a) {
    cecs_unit_component_storage storage = cecs_unit_component_storage_create();
    return (cecs_component_storage) {
        .storage = CECS_UNION_CREATE(
            cecs_unit_component_storage,
            cecs_component_storage_union,
            storage
        ),
            .entity_bitset = cecs_hibitset_create(a),
    };
}

cecs_component_storage cecs_component_storage_create_indirect(cecs_arena* a) {
    cecs_indirect_component_storage storage = cecs_indirect_component_storage_create();
    return (cecs_component_storage) {
        .storage = CECS_UNION_CREATE(
            cecs_indirect_component_storage,
            cecs_component_storage_union,
            storage
        ),
        .entity_bitset = cecs_hibitset_create(a),
    };
}

bool cecs_component_storage_has(const cecs_component_storage* self, cecs_entity_id id) {
    return cecs_hibitset_is_set(&self->entity_bitset, (size_t)id);
}

const cecs_dynamic_array* cecs_component_storage_components(const cecs_component_storage* self) {
    CECS_UNION_MATCH(self->storage) {
        case CECS_UNION_VARIANT(cecs_sparse_component_storage, cecs_component_storage_union):
            return &CECS_UNION_GET_UNCHECKED(cecs_sparse_component_storage, self->storage).components.elements;
        case CECS_UNION_VARIANT(cecs_indirect_component_storage, cecs_component_storage_union):
            return &CECS_UNION_GET_UNCHECKED(cecs_indirect_component_storage, self->storage).component_references.elements;
        default:
        {
            assert(false && "unreachable: invalid component storage variant");
            exit(EXIT_FAILURE);
            return NULL;
        }
    }
}

const cecs_storage_info cecs_component_storage_info(const cecs_component_storage* self) {
    return cecs_component_storage_get_functions(self).info(&self->storage);
}

cecs_optional_component cecs_component_storage_get(const cecs_component_storage* self, cecs_entity_id id, size_t size) {
    return cecs_component_storage_get_functions(self).get(&self->storage, id, size);
}

void* cecs_component_storage_get_unchecked(const cecs_component_storage* self, cecs_entity_id id, size_t size) {
    return CECS_OPTION_GET(cecs_optional_component, cecs_component_storage_get(self, id, size));
}

void* cecs_component_storage_get_or_null(const cecs_component_storage* self, cecs_entity_id id, size_t size) {
    cecs_optional_component component = cecs_component_storage_get(self, id, size);
    return CECS_OPTION_GET_OR_NULL(cecs_optional_component, component);
}

cecs_optional_component cecs_component_storage_set(cecs_component_storage* self, cecs_arena* a, cecs_entity_id id, void* component, size_t size) {
    cecs_hibitset_set(&self->entity_bitset, a, (size_t)id);
    return cecs_component_storage_get_functions(self).set(&self->storage, a, id, component, size);
}

bool cecs_component_storage_remove(cecs_component_storage* self, cecs_arena* a, cecs_entity_id id, void* out_removed_component, size_t size) {
    cecs_hibitset_unset(&self->entity_bitset, a, (size_t)id);
    return cecs_component_storage_get_functions(self).remove(&self->storage, a, id, out_removed_component, size);
}
