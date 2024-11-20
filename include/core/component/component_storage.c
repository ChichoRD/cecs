#include "component_storage.h"

const storage_info unit_component_storage_info(const unit_component_storage* self) {
    return (storage_info) {
        .is_index_stable = true,
            .is_dense = true,
            .is_unit_type_storage = true
    };
}

optional_component unit_component_storage_get(const unit_component_storage* self, entity_id id, size_t size) {
    return CECS_OPTION_CREATE_NONE_STRUCT(optional_component);
}

optional_component unit_component_storage_set(const unit_component_storage* self, cecs_arena* a, entity_id id, void* component, size_t size) {
    return CECS_OPTION_CREATE_NONE_STRUCT(optional_component);
}

bool unit_component_storage_remove(const unit_component_storage* self, cecs_arena* a, entity_id id, void* out_removed_component, size_t size) {
    memset(out_removed_component, 0, size);
    return false;
}

unit_component_storage unit_component_storage_create() {
    return (unit_component_storage) {
        .phantom_data = 0
    };
}

const storage_info indirect_component_storage_info(const indirect_component_storage* self) {
    return (storage_info) {
        .is_dense = false,
            .is_index_stable = true,
            .is_unit_type_storage = false
    };
}

optional_component indirect_component_storage_get(const indirect_component_storage* self, entity_id id, size_t size) {
    if (!cecs_displaced_set_contains(&self->component_references, (size_t)id, &(void*){0}, sizeof(void*))) {
        return CECS_OPTION_CREATE_NONE_STRUCT(optional_component);
    }
    else {
        return CECS_OPTION_CREATE_SOME_STRUCT(optional_component, *CECS_DISPLACED_SET_GET(void*, &self->component_references, (size_t)id));
    }
}

optional_component indirect_component_storage_set(indirect_component_storage* self, cecs_arena* a, entity_id id, void* component, size_t size) {
    return CECS_OPTION_CREATE_SOME_STRUCT(
        optional_component,
        *CECS_DISPLACED_SET_SET(
            void*,
            &self->component_references,
            a,
            (size_t)id,
            &component
        )
    );
}

bool indirect_component_storage_remove(indirect_component_storage* self, cecs_arena* a, entity_id id, void* out_removed_component, size_t size) {
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

indirect_component_storage indirect_component_storage_create() {
    return (indirect_component_storage) {
        .component_references = cecs_displaced_set_create()
    };
}

const storage_info sparse_component_storage_info(const sparse_component_storage* self) {
    return (storage_info) {
        .is_index_stable = true,
            .is_dense = false,
            .is_unit_type_storage = false
    };
}

optional_component sparse_component_storage_get(const sparse_component_storage* self, entity_id id, size_t size) {
    if (!cecs_displaced_set_contains_index(&self->components, (size_t)id)) {
        return CECS_OPTION_CREATE_NONE_STRUCT(optional_component);
    }
    else {
        return CECS_OPTION_CREATE_SOME_STRUCT(optional_component, cecs_displaced_set_get(&self->components, (size_t)id, size));
    }
}

optional_component sparse_component_storage_set(sparse_component_storage* self, cecs_arena* a, entity_id id, void* component, size_t size) {
    return CECS_OPTION_CREATE_SOME_STRUCT(
        optional_component,
        cecs_displaced_set_set(&self->components, a, (size_t)id, component, size)
    );
}

bool sparse_component_storage_remove(sparse_component_storage* self, cecs_arena* a, entity_id id, void* out_removed_component, size_t size) {
    if (cecs_displaced_set_contains_index(&self->components, (size_t)id)) {
        memcpy(out_removed_component, cecs_displaced_set_get(&self->components, (size_t)id, size), size);
        return true;
    }
    else {
        memset(out_removed_component, 0, size);
        return false;
    }
}

sparse_component_storage sparse_component_storage_create(cecs_arena* a, size_t component_capacity, size_t component_size) {
    return (sparse_component_storage) {
        .components = cecs_displaced_set_create_with_capacity(a, component_capacity * component_size),
    };
}

component_storage component_storage_create_sparse(cecs_arena* a, size_t component_capacity, size_t component_size) {
    sparse_component_storage storage = sparse_component_storage_create(a, component_capacity, component_size);
    return (component_storage) {
        .storage = CECS_UNION_CREATE(
            sparse_component_storage,
            component_storage_union,
            storage
        ),
            .entity_bitset = cecs_hibitset_create(a),
    };
}

component_storage component_storage_create_unit(cecs_arena* a) {
    unit_component_storage storage = unit_component_storage_create();
    return (component_storage) {
        .storage = CECS_UNION_CREATE(
            unit_component_storage,
            component_storage_union,
            storage
        ),
            .entity_bitset = cecs_hibitset_create(a),
    };
}

component_storage component_storage_create_indirect(cecs_arena* a, component_storage* other_storage) {
    indirect_component_storage storage = indirect_component_storage_create(&other_storage->storage);
    return (component_storage) {
        .storage = CECS_UNION_CREATE(
            indirect_component_storage,
            component_storage_union,
            storage
        ),
            .entity_bitset = cecs_hibitset_create(a),
    };
}

bool component_storage_has(const component_storage* self, entity_id id) {
    return cecs_hibitset_is_set(&self->entity_bitset, (size_t)id);
}

const cecs_dynamic_array* component_storage_components(const component_storage* self) {
    CECS_UNION_MATCH(self->storage) {
        case CECS_UNION_VARIANT(sparse_component_storage, component_storage_union):
            return &CECS_UNION_GET_UNCHECKED(sparse_component_storage, self->storage).components.elements;
        case CECS_UNION_VARIANT(indirect_component_storage, component_storage_union):
            return &CECS_UNION_GET_UNCHECKED(indirect_component_storage, self->storage).component_references.elements;
        default:
        {
            assert(false && "unreachable: invalid component storage variant");
            exit(EXIT_FAILURE);
            return NULL;
        }
    }
}

const storage_info component_storage_info(const component_storage* self) {
    return component_storage_get_functions(self).info(&self->storage);
}

optional_component component_storage_get(const component_storage* self, entity_id id, size_t size) {
    return component_storage_get_functions(self).get(&self->storage, id, size);
}

void* component_storage_get_unchecked(const component_storage* self, entity_id id, size_t size) {
    return CECS_OPTION_GET(optional_component, component_storage_get(self, id, size));
}

void* component_storage_get_or_null(const component_storage* self, entity_id id, size_t size) {
    optional_component component = component_storage_get(self, id, size);
    return CECS_OPTION_GET_OR_NULL(optional_component, component);
}

optional_component component_storage_set(component_storage* self, cecs_arena* a, entity_id id, void* component, size_t size) {
    cecs_hibitset_set(&self->entity_bitset, a, (size_t)id);
    return component_storage_get_functions(self).set(&self->storage, a, id, component, size);
}

bool component_storage_remove(component_storage* self, cecs_arena* a, entity_id id, void* out_removed_component, size_t size) {
    cecs_hibitset_unset(&self->entity_bitset, a, (size_t)id);
    return component_storage_get_functions(self).remove(&self->storage, a, id, out_removed_component, size);
}
