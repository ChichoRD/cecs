#include <memory.h>
#include <stdarg.h>
#include "cecs_component_storage.h"

cecs_storage_info cecs_unit_component_storage_info(const cecs_unit_component_storage* self) {
    (void)self;
    return (cecs_storage_info) {
        .is_index_stable = true,
        .is_dense = true,
        .is_unit_type_storage = true,
        .has_array_optimisation = false
    };
}

const cecs_component_storage_functions unit_component_storage_functions = {
    .info = (cecs_info *const)cecs_unit_component_storage_info,
    .get = (cecs_get_component *const)NULL,
    .set = (cecs_set_component *const)NULL,
    .remove = (cecs_remove_component *const)NULL
};

cecs_unit_component_storage cecs_unit_component_storage_create(void) {
    return (cecs_unit_component_storage) {
        .phantom_data = 0
    };
}

cecs_storage_info cecs_indirect_component_storage_info(const cecs_indirect_component_storage* self) {
    (void)self;
    return (cecs_storage_info) {
        .is_dense = false,
        .is_index_stable = true,
        .is_unit_type_storage = false,
        .has_array_optimisation = false
    };
}

cecs_optional_component cecs_indirect_component_storage_get(const cecs_indirect_component_storage* self, cecs_entity_id id, size_t size) {
    (void)size;
    if (!cecs_displaced_set_contains(&self->component_references, (size_t)id, &(void*){0}, sizeof(void*))) {
        return CECS_OPTION_CREATE_NONE_STRUCT(cecs_optional_component);
    } else {
        return CECS_OPTION_CREATE_SOME_STRUCT(cecs_optional_component, *CECS_DISPLACED_SET_GET(void*, &self->component_references, (size_t)id));
    }
}

void *cecs_indirect_component_storage_set(cecs_indirect_component_storage* self, cecs_arena* a, cecs_entity_id id, void* component, size_t size) {
    (void)size;
    return *CECS_DISPLACED_SET_SET(
        void*,
        &self->component_references,
        a,
        (size_t)id,
        &component
    );
}

bool cecs_indirect_component_storage_remove(cecs_indirect_component_storage* self, cecs_arena* a, cecs_entity_id id, void* out_removed_component, size_t size) {
    (void)a;
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

const cecs_component_storage_functions indirect_component_storage_functions = {
    .info = (cecs_info *const)cecs_indirect_component_storage_info,
    .get = (cecs_get_component *const)cecs_indirect_component_storage_get,
    .set = (cecs_set_component *const)cecs_indirect_component_storage_set,
    .remove = (cecs_remove_component *const)cecs_indirect_component_storage_remove
};

cecs_storage_info cecs_sparse_component_storage_info(const cecs_sparse_component_storage* self) {
    (void)self;
    return (cecs_storage_info) {
        .is_index_stable = true,
        .is_dense = false,
        .is_unit_type_storage = false,
        .has_array_optimisation = true
    };
}

cecs_optional_component cecs_sparse_component_storage_get(const cecs_sparse_component_storage* self, cecs_entity_id id, size_t size) {
    if (!cecs_displaced_set_contains_index(&self->components, (size_t)id)) {
        return CECS_OPTION_CREATE_NONE_STRUCT(cecs_optional_component);
    } else {
        return CECS_OPTION_CREATE_SOME_STRUCT(cecs_optional_component, cecs_displaced_set_get(&self->components, (size_t)id, size));
    }
}

void *cecs_sparse_component_storage_set(cecs_sparse_component_storage* self, cecs_arena* a, cecs_entity_id id, void* component, size_t size) {
    return cecs_displaced_set_set(&self->components, a, (size_t)id, component, size);
}

bool cecs_sparse_component_storage_remove(cecs_sparse_component_storage* self, cecs_arena* a, cecs_entity_id id, void* out_removed_component, size_t size) {
    (void)a;
    if (cecs_displaced_set_contains_index(&self->components, (size_t)id)) {
        memcpy(out_removed_component, cecs_displaced_set_get(&self->components, (size_t)id, size), size);
        return true;
    } else {
        memset(out_removed_component, 0, size);
        return false;
    }
}

cecs_sparse_component_storage cecs_sparse_component_storage_create(cecs_arena* a, size_t component_capacity, size_t component_size) {
    return (cecs_sparse_component_storage) {
        .components = cecs_displaced_set_create_with_capacity(a, component_capacity * component_size),
    };
}

const cecs_component_storage_functions sparse_component_storage_functions = {
    .info = (cecs_info *const)cecs_sparse_component_storage_info,
    .get = (cecs_get_component *const)cecs_sparse_component_storage_get,
    .set = (cecs_set_component *const)cecs_sparse_component_storage_set,
    .remove = (cecs_remove_component *const)cecs_sparse_component_storage_remove
};

size_t cecs_sparse_component_storage_get_array(const cecs_sparse_component_storage *self, cecs_entity_id id, void **out_components, size_t count, size_t size) {
    cecs_inclusive_range range = cecs_inclusive_range_from(cecs_range_intersection(
        cecs_exclusive_range_index_count((cecs_ssize_t)id, (cecs_ssize_t)count).range,
        self->components.index_range.range
    ));
    if (cecs_inclusive_range_is_empty(range)) {
        *out_components = NULL;
        return 0;
    } else {
        *out_components = cecs_displaced_set_get_range(
            &self->components,
            range,
            size
        );
        return cecs_inclusive_range_length(range);
    }
}

void *cecs_sparse_component_storage_set_array(cecs_sparse_component_storage *self, cecs_arena *a, cecs_entity_id id, void *components, size_t count, size_t size) {
    return cecs_displaced_set_set_range(
        &self->components,
        a,
        cecs_inclusive_range_index_count((cecs_ssize_t)id, (cecs_ssize_t)count),
        components,
        size
    );
}

void *cecs_sparse_component_storage_set_copy_array(cecs_sparse_component_storage *self, cecs_arena *a, cecs_entity_id id, void *component_single_src, size_t count, size_t size) {
    return cecs_displaced_set_set_copy_range(
        &self->components,
        a,
        cecs_inclusive_range_index_count((cecs_ssize_t)id, (cecs_ssize_t)count),
        component_single_src,
        size
    );
}

size_t cecs_sparse_component_storage_remove_array(cecs_sparse_component_storage *self, cecs_arena *a, cecs_entity_id id, void *out_removed_components, size_t count, size_t size) {
    cecs_inclusive_range range = cecs_inclusive_range_from(cecs_range_intersection(
        cecs_exclusive_range_index_count((cecs_ssize_t)id, (cecs_ssize_t)count).range,
        self->components.index_range.range
    ));
    if (cecs_inclusive_range_is_empty(range)) {
        memset(out_removed_components, 0, count * size);
        return 0;
    } else {
        size_t removed_count = cecs_inclusive_range_length(range);
        memcpy(out_removed_components, cecs_displaced_set_get_range(&self->components, range, size), removed_count * size);
        return removed_count;
    }
}

const cecs_component_storage_array_functions sparse_component_storage_array_functions = {
    .get = (cecs_get_component_array *const)cecs_sparse_component_storage_get_array,
    .set = (cecs_set_component_array *const)cecs_sparse_component_storage_set_array,
    .set_copy = (cecs_set_component_copy_array *const)cecs_sparse_component_storage_set_copy_array,
    .remove = (cecs_remove_component_array *const)cecs_sparse_component_storage_remove_array
};


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

cecs_component_storage_function_type cecs_component_storage_function_type_from_info(cecs_storage_info info) {
    if (info.is_unit_type_storage) {
        return cecs_component_storage_function_type_none;
    } else if (info.has_array_optimisation) {
        return cecs_component_storage_function_type_array_functions;
    } else if (info.is_storage_extension) {
        return cecs_component_storage_function_type_custom;
    } else {
        return cecs_component_storage_function_type_functions;
    }
}

bool cecs_component_storage_has(const cecs_component_storage *self, cecs_entity_id id) {
    return cecs_hibitset_is_set(&self->entity_bitset, (size_t)id);
}

const cecs_dynamic_array *cecs_component_storage_components(const cecs_component_storage* self) {
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

extern inline cecs_component_storage_functions cecs_component_storage_get_functions(const cecs_component_storage *self);
extern inline cecs_component_storage_array_functions cecs_component_storage_get_array_functions(const cecs_component_storage *self);

cecs_storage_info cecs_component_storage_info(const cecs_component_storage* self) {
    cecs_component_storage_functions storage_functions = cecs_component_storage_get_functions(self);
    assert(storage_functions.info != NULL && "fatal error: component storage functions info must not be NULL");
    return storage_functions.info(&self->storage);
}

cecs_optional_component cecs_component_storage_get(const cecs_component_storage* self, cecs_entity_id id, size_t size) {
    cecs_component_storage_functions storage_functions = cecs_component_storage_get_functions(self);
    if (cecs_component_storage_function_type_from_info(storage_functions.info(&self->storage))
        != cecs_component_storage_function_type_none) {
        return cecs_component_storage_get_functions(self).get(&self->storage, id, size);
    } else {
        return CECS_OPTION_CREATE_NONE_STRUCT(cecs_optional_component);
    }
}
size_t cecs_component_storage_get_array(const cecs_component_storage *self, cecs_entity_id id, void **out_components, size_t count, size_t size) {
    cecs_component_storage_functions storage_functions = cecs_component_storage_get_functions(self);
    switch (cecs_component_storage_function_type_from_info(storage_functions.info(&self->storage))) {
    case cecs_component_storage_function_type_none: {
        *out_components = NULL;
        return 0;
    }
    case cecs_component_storage_function_type_functions: {
        size_t i = 0;
        bool all_found = true;
        while (i < count && all_found) {
            cecs_optional_component component = storage_functions.get(&self->storage, id + i, size);
            if (CECS_OPTION_IS_NONE(cecs_optional_component, component)) {
                all_found = false;
            } else {
                out_components[i] = CECS_OPTION_GET_UNCHECKED(cecs_optional_component, component);
                ++i;
            }
        }
        return i;
    }
    case cecs_component_storage_function_type_array_functions:
        return cecs_component_storage_get_array_functions(self).get(&self->storage, id, out_components, count, size);
    case cecs_component_storage_function_type_custom: {
        // TODO
        assert(false && "unimplemented: custom storage functions");
        exit(EXIT_FAILURE);
        return 0;
    }
    default: {
        assert(false && "unreachable: invalid component storage function type");
        exit(EXIT_FAILURE);
        return 0;
    }
    }
}

void *cecs_component_storage_get_unchecked(const cecs_component_storage *self, cecs_entity_id id, size_t size) {
    return CECS_OPTION_GET(cecs_optional_component, cecs_component_storage_get(self, id, size));
}
void *cecs_component_storage_get_or_null(const cecs_component_storage* self, cecs_entity_id id, size_t size) {
    cecs_optional_component component = cecs_component_storage_get(self, id, size);
    return CECS_OPTION_GET_OR_NULL(cecs_optional_component, component);
}

void *cecs_component_storage_set(cecs_component_storage* self, cecs_arena* a, cecs_entity_id id, void* component, size_t size) {
    cecs_hibitset_set(&self->entity_bitset, a, (size_t)id);

    cecs_component_storage_functions storage_functions = cecs_component_storage_get_functions(self);
    if (cecs_component_storage_function_type_from_info(storage_functions.info(&self->storage))) {
        return storage_functions.set(&self->storage, a, id, component, size);
    } else {
        return component;
    }
}

void *cecs_component_storage_set_array(cecs_component_storage *self, cecs_arena *a, cecs_entity_id id, void *components, size_t count, size_t size) {
    cecs_hibitset_set_range(&self->entity_bitset, a, (size_t)id, count);

    cecs_component_storage_functions storage_functions = cecs_component_storage_get_functions(self);
    switch (cecs_component_storage_function_type_from_info(storage_functions.info(&self->storage))) {
    case cecs_component_storage_function_type_none:
        return components;
    case cecs_component_storage_function_type_functions: {
        if (count == 0) {
            return NULL;
        }

        void *first = set(&self->storage, a, id, components, size);
        for (size_t i = 1; i < count; ++i) {
            storage_functions.set(&self->storage, a, id + i, ((uint8_t *)components) + i * size, size);
        }
        return first;
    }
    case cecs_component_storage_function_type_array_functions:
        return cecs_component_storage_get_array_functions(self).set(&self->storage, a, id, components, count, size);
    case cecs_component_storage_function_type_custom: {
        assert(false && "unimplemented: custom storage functions");
        exit(EXIT_FAILURE);
        return NULL;
    }
    default: {
        assert(false && "unreachable: invalid component storage function type");
        exit(EXIT_FAILURE);
        return NULL;
    }
    }
}

void *cecs_component_storage_set_copy_array(cecs_component_storage *self, cecs_arena *a, cecs_entity_id id, void *component_single_src, size_t count, size_t size) {
    cecs_hibitset_set_range(&self->entity_bitset, a, (size_t)id, count);

    cecs_component_storage_functions storage_functions = cecs_component_storage_get_functions(self);
    switch (cecs_component_storage_function_type_from_info(storage_functions.info(&self->storage))) {
    case cecs_component_storage_function_type_none:
        return component_single_src;
    case cecs_component_storage_function_type_functions: {
        if (count == 0) {
            return NULL;
        }

        void *first = storage_functions.set(&self->storage, a, id, component_single_src, size);
        for (size_t i = 1; i < count; ++i) {
            storage_functions.set(&self->storage, a, id + i, component_single_src, size);
        }
        return first;
    }
    case cecs_component_storage_function_type_array_functions:
        return cecs_component_storage_get_array_functions(self).set_copy(&self->storage, a, id, component_single_src, count, size);
    case cecs_component_storage_function_type_custom: {
        assert(false && "unimplemented: custom storage functions");
        exit(EXIT_FAILURE);
        return NULL;
    }
    default: {
        assert(false && "unreachable: invalid component storage function type");
        exit(EXIT_FAILURE);
        return NULL;
    }
    }
}

bool cecs_component_storage_remove(cecs_component_storage *self, cecs_arena *a, cecs_entity_id id, void *out_removed_component, size_t size) {
    bool was_set = cecs_hibitset_is_set(&self->entity_bitset, (size_t)id);
    cecs_hibitset_unset(&self->entity_bitset, a, (size_t)id);

    cecs_component_storage_functions storage_functions = cecs_component_storage_get_functions(self);
    if (cecs_component_storage_function_type_from_info(storage_functions.info(&self->storage))) {
        return storage_functions.remove(&self->storage, a, id, out_removed_component, size);
    } else {
        memset(out_removed_component, 0, size);
        return was_set;
    }
}

size_t cecs_component_storage_remove_array(cecs_component_storage *self, cecs_arena *a, cecs_entity_id id, void *out_removed_components, size_t count, size_t size) {
    cecs_hibitset_unset_range(&self->entity_bitset, a, (size_t)id, count);

    cecs_component_storage_functions storage_functions = cecs_component_storage_get_functions(self);
    switch (cecs_component_storage_function_type_from_info(storage_functions.info(&self->storage))) {
    case cecs_component_storage_function_type_none: {
        memset(out_removed_components, 0, count * size);
        return 0;
    }
    case cecs_component_storage_function_type_functions: {
        size_t i = 0;
        bool all_removed = true;
        while (i < count && all_removed) {
            if (storage_functions.remove(&self->storage, a, id + i, ((uint8_t *)out_removed_components) + i * size, size)) {
                ++i;
            } else {
                all_removed = false;
            }
        }
        return i;
    }
    case cecs_component_storage_function_type_array_functions:
        return cecs_component_storage_get_array_functions(self).remove(&self->storage, a, id, out_removed_components, count, size);
    case cecs_component_storage_function_type_custom: {
        assert(false && "unimplemented: custom storage functions");
        exit(EXIT_FAILURE);
        return 0;
    }
    default: {
        assert(false && "unreachable: invalid component storage function type");
        exit(EXIT_FAILURE);
        return 0;
    }
    }
}
