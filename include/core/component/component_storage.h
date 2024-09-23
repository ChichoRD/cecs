#ifndef COMPONENT_STORAGE_H
#define COMPONENT_STORAGE_H

#include <assert.h>
#include <stdlib.h>
#include "../../containers/list.h"
#include "../../containers/displaced_set.h"
#include "../../containers/bitset.h"
#include "../../containers/range.h"
#include "../../containers/arena.h"
#include "../../containers/tagged_union.h"
#include "../../types/wide_ptr.h"
#include "component_type.h"
#include "entity/entity.h"


typedef struct storage_info {
    bool is_index_stable : 1;
    bool is_dense : 1;
    bool is_unit_type_storage : 1;
} storage_info;


typedef OPTION_STRUCT(void *, optional_component) optional_component;

typedef storage_info info(const void *self);
typedef optional_component get_component(const void *self, entity_id id, size_t size);
typedef optional_component set_component(void *self, arena *a, entity_id id, void *component, size_t size);
typedef bool remove_component(void *self, arena *a, entity_id id, void *out_removed_component, size_t size);
typedef struct component_storage_functions {
    info *const info;
    get_component *const get;
    set_component *const set;
    remove_component *const remove;
} component_storage_functions;

typedef union component_storage_ptr {
    wide_ptr raw_storage;
    struct {
        void *storage;
        component_storage_functions *functions;
    };
} component_storage_ptr;


typedef struct unit_component_storage {
    uint_least8_t phantom_data;
} unit_component_storage;

const storage_info unit_component_storage_info(const unit_component_storage *self) {
    return (storage_info) {
        .is_index_stable = true,
        .is_dense = true,
        .is_unit_type_storage = true
    };
}

optional_component unit_component_storage_get(const unit_component_storage *self, entity_id id, size_t size) {
    return OPTION_CREATE_NONE_STRUCT(optional_component);
}

optional_component unit_component_storage_set(const unit_component_storage *self, arena *a, entity_id id, void *component, size_t size) {
    return OPTION_CREATE_NONE_STRUCT(optional_component);
}

bool unit_component_storage_remove(const unit_component_storage *self, arena *a, entity_id id, void *out_removed_component, size_t size) {
    memset(out_removed_component, 0, size);
    return false;
}

static const component_storage_functions unit_component_storage_functions = {
    .info = unit_component_storage_info,
    .get = unit_component_storage_get,
    .set = unit_component_storage_set,
    .remove = unit_component_storage_remove
};

unit_component_storage unit_component_storage_create() {
    return (unit_component_storage) {
        .phantom_data = 0
    };
}


typedef struct indirect_component_storgage {
    displaced_set component_references;
} indirect_component_storage;

const storage_info indirect_component_storage_info(const indirect_component_storage *self) {
    return (storage_info){
        .is_dense = false,
        .is_index_stable = true,
        .is_unit_type_storage = false
    };
}

optional_component indirect_component_storage_get(const indirect_component_storage *self, entity_id id, size_t size) {
    if (!displaced_set_contains(&self->component_references, id, &(void *){0}, sizeof(void *))) {
        return OPTION_CREATE_NONE_STRUCT(optional_component);
    } else {
        return OPTION_CREATE_SOME_STRUCT(optional_component, *DISPLACED_SET_GET(void *, &self->component_references, id));
    }
}

optional_component indirect_component_storage_set(const indirect_component_storage *self, arena *a, entity_id id, void *component, size_t size) {
    return OPTION_CREATE_SOME_STRUCT(
        optional_component,
        *DISPLACED_SET_SET(
            void *,
            &self->component_references,
            a,
            id,
            &component
        )
    );
}

bool indirect_component_storage_remove(const indirect_component_storage *self, arena *a, entity_id id, void *out_removed_component, size_t size) {
    void *removed_component = NULL;
    if (displaced_set_remove_out(
        &self->component_references,
        id,
        &removed_component,
        sizeof(void *),
        &(void *){0}
    )) {
        memcpy(out_removed_component, removed_component, size);
        return true;
    } else {
        memset(out_removed_component, 0, size);
        return false;
    }
}

indirect_component_storage indirect_component_storage_create() {
    return (indirect_component_storage) {
        .component_references = displaced_set_create()
    };
}

static const component_storage_functions indirect_component_storage_functions = {
    .info = indirect_component_storage_info,
    .get = indirect_component_storage_get,
    .set = indirect_component_storage_set,
    .remove = indirect_component_storage_remove
};


typedef struct sparse_component_storage {
    displaced_set components;
} sparse_component_storage;

const storage_info sparse_component_storage_info(const sparse_component_storage *self) {
    return (storage_info) {
        .is_index_stable = true,
        .is_dense = false,
        .is_unit_type_storage = false
    };
}

optional_component sparse_component_storage_get(const sparse_component_storage *self, entity_id id, size_t size) {
    if(!displaced_set_contains_index(&self->components, id)) {
        return OPTION_CREATE_NONE_STRUCT(optional_component);
    } else {
        return OPTION_CREATE_SOME_STRUCT(optional_component, displaced_set_get(&self->components, id, size));
    }
}

optional_component sparse_component_storage_set(sparse_component_storage *self, arena *a, entity_id id, void *component, size_t size) {
    return OPTION_CREATE_SOME_STRUCT(
        optional_component,
        displaced_set_set(&self->components, a, id, component, size)
    );
}

bool sparse_component_storage_remove(sparse_component_storage *self, arena *a, entity_id id, void *out_removed_component, size_t size) {
    if (displaced_set_contains_index(&self->components, id)) {
        memcpy(out_removed_component, displaced_set_get(&self->components, id, size), size);
        return true;
    } else {
        memset(out_removed_component, 0, size);
        return false;
    }
}

sparse_component_storage sparse_component_storage_create(arena *a, size_t component_capacity, size_t component_size) {
    return (sparse_component_storage) {
        .components = displaced_set_create_with_capacity(a, component_capacity * component_size),
    };
}

static const component_storage_functions sparse_component_storage_functions = {
    .info = sparse_component_storage_info,
    .get = sparse_component_storage_get,
    .set = sparse_component_storage_set,
    .remove = sparse_component_storage_remove
};


typedef TAGGED_UNION_STRUCT(
    component_storage_union,
    sparse_component_storage,
    sparse_component_storage,
    unit_component_storage,
    unit_component_storage,
    indirect_component_storage,
    indirect_component_storage
) component_storage_union;

typedef struct component_storage {
    component_storage_union storage;
    hibitset entity_bitset;
} component_storage;

component_storage component_storage_create_sparse(arena *a, size_t component_capacity, size_t component_size) {
    sparse_component_storage storage = sparse_component_storage_create(a, component_capacity, component_size);
    return (component_storage) {
        .storage = TAGGED_UNION_CREATE(
            sparse_component_storage,
            component_storage_union,
            storage
        ),
        .entity_bitset = hibitset_create(a),
    };
}

component_storage component_storage_create_unit(arena *a) {
    unit_component_storage storage = unit_component_storage_create();
    return (component_storage) {
        .storage = TAGGED_UNION_CREATE(
            unit_component_storage,
            component_storage_union,
            storage
        ),
        .entity_bitset = hibitset_create(a),
    };
}

component_storage component_storage_create_indirect(arena *a, component_storage *other_storage) {
    indirect_component_storage storage = indirect_component_storage_create(&other_storage->storage);
    return (component_storage) {
        .storage = TAGGED_UNION_CREATE(
            indirect_component_storage,
            component_storage_union,
            storage
        ),
        .entity_bitset = hibitset_create(a),
    };
}

bool component_storage_has(const component_storage *self, entity_id id) {
    return hibitset_is_set(&self->entity_bitset, id);
}

const list *component_storage_components(const component_storage *self) {
    TAGGED_UNION_MATCH(self->storage) {
        case TAGGED_UNION_VARIANT(sparse_component_storage, component_storage_union):
            return &TAGGED_UNION_GET_UNCHECKED(sparse_component_storage, self->storage).components.elements;
        case TAGGED_UNION_VARIANT(indirect_component_storage, component_storage_union):
            return &TAGGED_UNION_GET_UNCHECKED(indirect_component_storage, self->storage).component_references.elements;
        default:
            {
                assert(false && "unreachable: invalid component storage variant");
                exit(EXIT_FAILURE);
                return NULL;
            }
    }
}

inline component_storage_functions component_storage_get_functions(const component_storage *self) {
    TAGGED_UNION_MATCH(self->storage) {
        case TAGGED_UNION_VARIANT(sparse_component_storage, component_storage_union):
            return sparse_component_storage_functions;
        case TAGGED_UNION_VARIANT(unit_component_storage, component_storage_union):
            return unit_component_storage_functions;
        case TAGGED_UNION_VARIANT(indirect_component_storage, component_storage_union):
            return indirect_component_storage_functions;
        default:
            {
                assert(false && "unreachable: invalid component storage variant");
                exit(EXIT_FAILURE);
                return (component_storage_functions){0};
            }
    }
}

const storage_info component_storage_info(const component_storage *self) {
    return component_storage_get_functions(self).info(&self->storage);
}

optional_component component_storage_get(const component_storage *self, entity_id id, size_t size) {
    return component_storage_get_functions(self).get(&self->storage, id, size);
}
#define COMPONENT_STORAGE_GET(type, component_storage_ref, entity_id) \
    ((type *)component_storage_get(component_storage_ref, entity_id, sizeof(type)))

void *component_storage_get_unchecked(const component_storage *self, entity_id id, size_t size) {
    return OPTION_GET(optional_component, component_storage_get(self, id, size));
}

void *component_storage_get_or_null(const component_storage *self, entity_id id, size_t size) {
    optional_component component = component_storage_get(self, id, size);
    return OPTION_GET_OR_NULL(optional_component, component);
}

optional_component component_storage_set(component_storage *self, arena *a, entity_id id, void *component, size_t size) {
    hibitset_set(&self->entity_bitset, a, id);
    return component_storage_get_functions(self).set(&self->storage, a, id, component, size);
}
#define COMPONENT_STORAGE_SET(type, component_storage_ref, arena_ref, entity_id, component_ref) \
    ((type *)component_storage_set(component_storage_ref, arena_ref, entity_id, component_ref, sizeof(type)))

bool component_storage_remove(component_storage *self, arena *a, entity_id id, void *out_removed_component, size_t size) {
    hibitset_unset(&self->entity_bitset, a, id);
    return component_storage_get_functions(self).remove(&self->storage, a, id, out_removed_component, size);
}
#define COMPONENT_STORAGE_REMOVE(type, component_storage_ref, arena_ref, entity_id) \
    ((type *)component_storage_remove(component_storage_ref, arena_ref, entity_id, sizeof(type)))


#endif