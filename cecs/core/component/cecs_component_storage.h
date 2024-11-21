#ifndef CECS_COMPONENT_STORAGE_H
#define CECS_COMPONENT_STORAGE_H

#include <assert.h>
#include <stdlib.h>
#include "../../containers/cecs_dynamic_array.h"
#include "../../containers/cecs_displaced_set.h"
#include "../../containers/cecs_bitset.h"
#include "../../containers/cecs_arena.h"
#include "../../containers/cecs_union.h"
#include "entity/cecs_component_type.h"
#include "entity/cecs_entity.h"


typedef struct cecs_storage_info {
    bool is_index_stable : 1;
    bool is_dense : 1;
    bool is_unit_type_storage : 1;
} cecs_storage_info;


typedef CECS_OPTION_STRUCT(void *, cecs_optional_component) cecs_optional_component;

typedef cecs_storage_info cecs_info(const void *self);
typedef cecs_optional_component cecs_get_component(const void *self, cecs_entity_id id, size_t size);
typedef cecs_optional_component cecs_set_component(void *self, cecs_arena *a, cecs_entity_id id, void *component, size_t size);
typedef bool cecs_remove_component(void *self, cecs_arena *a, cecs_entity_id id, void *out_removed_component, size_t size);
typedef struct cecs_component_storage_functions {
    cecs_info *const info;
    cecs_get_component *const get;
    cecs_set_component *const set;
    cecs_remove_component *const remove;
} cecs_component_storage_functions;


typedef struct cecs_unit_component_storage {
    uint_least8_t phantom_data;
} cecs_unit_component_storage;

const cecs_storage_info cecs_unit_component_storage_info(const cecs_unit_component_storage *self);

cecs_optional_component cecs_unit_component_storage_get(const cecs_unit_component_storage *self, cecs_entity_id id, size_t size);

cecs_optional_component cecs_unit_component_storage_set(const cecs_unit_component_storage *self, cecs_arena *a, cecs_entity_id id, void *component, size_t size);

bool cecs_unit_component_storage_remove(const cecs_unit_component_storage *self, cecs_arena *a, cecs_entity_id id, void *out_removed_component, size_t size);

static const cecs_component_storage_functions unit_component_storage_functions = {
    .info = (cecs_info *const)cecs_unit_component_storage_info,
    .get = (cecs_get_component *const)cecs_unit_component_storage_get,
    .set = (cecs_set_component *const)cecs_unit_component_storage_set,
    .remove = (cecs_remove_component *const)cecs_unit_component_storage_remove
};

cecs_unit_component_storage cecs_unit_component_storage_create(void);


typedef struct cecs_indirect_component_storgage {
    cecs_displaced_set component_references;
} cecs_indirect_component_storage;

const cecs_storage_info cecs_indirect_component_storage_info(const cecs_indirect_component_storage *self);

cecs_optional_component cecs_indirect_component_storage_get(const cecs_indirect_component_storage *self, cecs_entity_id id, size_t size);

cecs_optional_component cecs_indirect_component_storage_set(cecs_indirect_component_storage *self, cecs_arena *a, cecs_entity_id id, void *component, size_t size);

bool cecs_indirect_component_storage_remove(cecs_indirect_component_storage *self, cecs_arena *a, cecs_entity_id id, void *out_removed_component, size_t size);

cecs_indirect_component_storage cecs_indirect_component_storage_create(void);

static const cecs_component_storage_functions indirect_component_storage_functions = {
    .info = (cecs_info *const)cecs_indirect_component_storage_info,
    .get = (cecs_get_component *const)cecs_indirect_component_storage_get,
    .set = (cecs_set_component *const)cecs_indirect_component_storage_set,
    .remove = (cecs_remove_component *const)cecs_indirect_component_storage_remove
};


typedef struct cecs_sparse_component_storage {
    cecs_displaced_set components;
} cecs_sparse_component_storage;

const cecs_storage_info cecs_sparse_component_storage_info(const cecs_sparse_component_storage *self);

cecs_optional_component cecs_sparse_component_storage_get(const cecs_sparse_component_storage *self, cecs_entity_id id, size_t size);

cecs_optional_component cecs_sparse_component_storage_set(cecs_sparse_component_storage *self, cecs_arena *a, cecs_entity_id id, void *component, size_t size);

bool cecs_sparse_component_storage_remove(cecs_sparse_component_storage *self, cecs_arena *a, cecs_entity_id id, void *out_removed_component, size_t size);

cecs_sparse_component_storage cecs_sparse_component_storage_create(cecs_arena *a, size_t component_capacity, size_t component_size);

static const cecs_component_storage_functions sparse_component_storage_functions = {
    .info = (cecs_info *const)cecs_sparse_component_storage_info,
    .get = (cecs_get_component *const)cecs_sparse_component_storage_get,
    .set = (cecs_set_component *const)cecs_sparse_component_storage_set,
    .remove = (cecs_remove_component *const)cecs_sparse_component_storage_remove
};

// TODO: add more storage types

typedef CECS_UNION_STRUCT(
    cecs_component_storage_union,
    cecs_sparse_component_storage,
    cecs_sparse_component_storage,
    cecs_unit_component_storage,
    cecs_unit_component_storage,
    cecs_indirect_component_storage,
    cecs_indirect_component_storage
) cecs_component_storage_union;

typedef struct cecs_component_storage {
    cecs_component_storage_union storage;
    cecs_hibitset entity_bitset;
} cecs_component_storage;

cecs_component_storage cecs_component_storage_create_sparse(cecs_arena *a, size_t component_capacity, size_t component_size);

cecs_component_storage cecs_component_storage_create_unit(cecs_arena *a);

cecs_component_storage cecs_component_storage_create_indirect(cecs_arena *a, cecs_component_storage *other_storage);

bool cecs_component_storage_has(const cecs_component_storage *self, cecs_entity_id id);

const cecs_dynamic_array *cecs_component_storage_components(const cecs_component_storage *self);

inline cecs_component_storage_functions cecs_component_storage_get_functions(const cecs_component_storage *self) {
    CECS_UNION_MATCH(self->storage) {
        case CECS_UNION_VARIANT(cecs_sparse_component_storage, cecs_component_storage_union):
            return sparse_component_storage_functions;
        case CECS_UNION_VARIANT(cecs_unit_component_storage, cecs_component_storage_union):
            return unit_component_storage_functions;
        case CECS_UNION_VARIANT(cecs_indirect_component_storage, cecs_component_storage_union):
            return indirect_component_storage_functions;
        default:
            {
                assert(false && "unreachable: invalid component storage variant");
                exit(EXIT_FAILURE);
                return (cecs_component_storage_functions){0};
            }
    }
}

const cecs_storage_info cecs_component_storage_info(const cecs_component_storage *self);

cecs_optional_component cecs_component_storage_get(const cecs_component_storage *self, cecs_entity_id id, size_t size);
#define CECS_COMPONENT_STORAGE_GET(type, component_storage_ref, entity_id) \
    ((type *)cecs_component_storage_get(component_storage_ref, entity_id, sizeof(type)))

void *cecs_component_storage_get_unchecked(const cecs_component_storage *self, cecs_entity_id id, size_t size);

void *cecs_component_storage_get_or_null(const cecs_component_storage *self, cecs_entity_id id, size_t size);

cecs_optional_component cecs_component_storage_set(cecs_component_storage *self, cecs_arena *a, cecs_entity_id id, void *component, size_t size);
#define CECS_COMPONENT_STORAGE_SET(type, component_storage_ref, arena_ref, entity_id, component_ref) \
    ((type *)cecs_component_storage_set(component_storage_ref, arena_ref, entity_id, component_ref, sizeof(type)))

bool cecs_component_storage_remove(cecs_component_storage *self, cecs_arena *a, cecs_entity_id id, void *out_removed_component, size_t size);
#define CECS_COMPONENT_STORAGE_REMOVE(type, component_storage_ref, arena_ref, entity_id) \
    ((type *)cecs_component_storage_remove(component_storage_ref, arena_ref, entity_id, sizeof(type)))


#endif