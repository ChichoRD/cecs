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
    bool has_array_optimisation : 1;
    bool is_storage_extension : 1;
} cecs_storage_info;


typedef CECS_OPTION_STRUCT(void *, cecs_optional_component) cecs_optional_component;
typedef CECS_OPTION_STRUCT(void *, cecs_optional_component_array) cecs_optional_component_array;

typedef cecs_storage_info cecs_info(const void *self);
typedef cecs_optional_component cecs_get_component(const void *self, cecs_entity_id id, size_t size);
typedef void *cecs_set_component(void *self, cecs_arena *a, cecs_entity_id id, void *component, size_t size);
typedef bool cecs_remove_component(void *self, cecs_arena *a, cecs_entity_id id, void *out_removed_component, size_t size);
typedef struct cecs_component_storage_functions {
    cecs_info *const info;
    cecs_get_component *const get;
    cecs_set_component *const set;
    cecs_remove_component *const remove;
} cecs_component_storage_functions;

typedef size_t cecs_get_component_array(const void *self, cecs_entity_id id, void **out_components, size_t count, size_t size);
typedef void *cecs_set_component_array(void *self, cecs_arena *a, cecs_entity_id id, void *components, size_t count, size_t size);
typedef void *cecs_set_component_copy_array(void *self, cecs_arena *a, cecs_entity_id id, void *component_single_src, size_t count, size_t size);
typedef size_t cecs_remove_component_array(void *self, cecs_arena *a, cecs_entity_id id, void *out_removed_components, size_t count, size_t size);
typedef struct cecs_component_storage_array_functions {
    //cecs_info *const info;
    cecs_get_component_array *const get;
    cecs_set_component_array *const set;
    cecs_set_component_copy_array *const set_copy;
    cecs_remove_component_array *const remove;
} cecs_component_storage_array_functions;


typedef struct cecs_unit_component_storage {
    uint_least8_t phantom_data;
} cecs_unit_component_storage;

cecs_storage_info cecs_unit_component_storage_info(const cecs_unit_component_storage *self);
extern const cecs_component_storage_functions unit_component_storage_functions;

cecs_unit_component_storage cecs_unit_component_storage_create(void);


typedef struct cecs_indirect_component_storgage {
    cecs_displaced_set component_references;
} cecs_indirect_component_storage;

cecs_storage_info cecs_indirect_component_storage_info(const cecs_indirect_component_storage *self);
cecs_optional_component cecs_indirect_component_storage_get(const cecs_indirect_component_storage *self, cecs_entity_id id, size_t size);
void *cecs_indirect_component_storage_set(cecs_indirect_component_storage *self, cecs_arena *a, cecs_entity_id id, void *component, size_t size);
bool cecs_indirect_component_storage_remove(cecs_indirect_component_storage *self, cecs_arena *a, cecs_entity_id id, void *out_removed_component, size_t size);

cecs_indirect_component_storage cecs_indirect_component_storage_create(void);

extern const cecs_component_storage_functions indirect_component_storage_functions;


typedef struct cecs_sparse_component_storage {
    cecs_displaced_set components;
} cecs_sparse_component_storage;

cecs_storage_info cecs_sparse_component_storage_info(const cecs_sparse_component_storage *self);
cecs_optional_component cecs_sparse_component_storage_get(const cecs_sparse_component_storage *self, cecs_entity_id id, size_t size);
void *cecs_sparse_component_storage_set(cecs_sparse_component_storage *self, cecs_arena *a, cecs_entity_id id, void *component, size_t size);
bool cecs_sparse_component_storage_remove(cecs_sparse_component_storage *self, cecs_arena *a, cecs_entity_id id, void *out_removed_component, size_t size);

cecs_sparse_component_storage cecs_sparse_component_storage_create(cecs_arena *a, size_t component_capacity, size_t component_size);
extern const cecs_component_storage_functions sparse_component_storage_functions;

size_t cecs_sparse_component_storage_get_array(const cecs_sparse_component_storage *self, cecs_entity_id id, void **out_components, size_t count, size_t size);
void *cecs_sparse_component_storage_set_array(cecs_sparse_component_storage *self, cecs_arena *a, cecs_entity_id id, void *components, size_t count, size_t size);
void *cecs_sparse_component_storage_set_copy_array(cecs_sparse_component_storage *self, cecs_arena *a, cecs_entity_id id, void *component_single_src, size_t count, size_t size);
size_t cecs_sparse_component_storage_remove_array(
    cecs_sparse_component_storage *self,
    cecs_arena *a,
    cecs_entity_id id,
    void *out_removed_components,
    size_t count,
    size_t size
);

extern const cecs_component_storage_array_functions sparse_component_storage_array_functions;

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
    cecs_hibitset entity_bitset;
    cecs_component_storage_union storage;
} cecs_component_storage;

cecs_component_storage cecs_component_storage_create_sparse(cecs_arena *a, size_t component_capacity, size_t component_size);
cecs_component_storage cecs_component_storage_create_unit(cecs_arena *a);
cecs_component_storage cecs_component_storage_create_indirect(cecs_arena *a);

typedef enum cecs_component_storage_function_type {
    cecs_component_storage_function_type_none,
    cecs_component_storage_function_type_functions,
    cecs_component_storage_function_type_array_functions,
    cecs_component_storage_function_type_custom
} cecs_component_storage_function_type;

cecs_component_storage_function_type cecs_component_storage_function_type_from_info(cecs_storage_info info);

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
    // TODO: Add extension for user defined storage types
    default: {
        assert(false && "unreachable: invalid component storage variant");
        exit(EXIT_FAILURE);
        return (cecs_component_storage_functions){0};
    }
    }
}

inline cecs_component_storage_array_functions cecs_component_storage_get_array_functions(const cecs_component_storage *self) {
    CECS_UNION_MATCH(self->storage) {
    case CECS_UNION_VARIANT(cecs_sparse_component_storage, cecs_component_storage_union):
        return sparse_component_storage_array_functions;
    default: {
        assert(
            false &&
            "error: invalid component storage variant, "
            "check cecs_storage_info.has_array_optimisation to see if the storage supports array operations"
        );
        exit(EXIT_FAILURE);
        return (cecs_component_storage_array_functions){0};
    }
    }
}

cecs_storage_info cecs_component_storage_info(const cecs_component_storage *self);
cecs_optional_component cecs_component_storage_get(const cecs_component_storage *self, cecs_entity_id id, size_t size);
#define CECS_COMPONENT_STORAGE_GET(type, component_storage_ref, entity_id) \
    ((type *)cecs_component_storage_get(component_storage_ref, entity_id, sizeof(type)))
size_t cecs_component_storage_get_array(const cecs_component_storage *self, cecs_entity_id id, void **out_components, size_t count, size_t size);

void *cecs_component_storage_get_unchecked(const cecs_component_storage *self, cecs_entity_id id, size_t size);
void *cecs_component_storage_get_or_null(const cecs_component_storage *self, cecs_entity_id id, size_t size);

cecs_optional_component cecs_component_storage_set(cecs_component_storage *self, cecs_arena *a, cecs_entity_id id, void *component, size_t size);
#define CECS_COMPONENT_STORAGE_SET(type, component_storage_ref, arena_ref, entity_id, component_ref) \
    ((type *)cecs_component_storage_set(component_storage_ref, arena_ref, entity_id, component_ref, sizeof(type)))
cecs_optional_component_array cecs_component_storage_set_array(cecs_component_storage *self, cecs_arena *a, cecs_entity_id id, void *components, size_t count, size_t size);
cecs_optional_component_array cecs_component_storage_set_copy_array(cecs_component_storage *self, cecs_arena *a, cecs_entity_id id, void *component_single_src, size_t count, size_t size);

bool cecs_component_storage_remove(cecs_component_storage *self, cecs_arena *a, cecs_entity_id id, void *out_removed_component, size_t size);
#define CECS_COMPONENT_STORAGE_REMOVE(type, component_storage_ref, arena_ref, entity_id) \
    ((type *)cecs_component_storage_remove(component_storage_ref, arena_ref, entity_id, sizeof(type)))
size_t cecs_component_storage_remove_array(
    cecs_component_storage *self,
    cecs_arena *a,
    cecs_entity_id id,
    void *out_removed_components,
    size_t count,
    size_t size
);

#endif