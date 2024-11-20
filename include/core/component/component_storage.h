#ifndef COMPONENT_STORAGE_H
#define COMPONENT_STORAGE_H

#include <assert.h>
#include <stdlib.h>
#include "../../containers/list.h"
#include "../../containers/displaced_set.h"
#include "../../containers/cecs_bitset.h"
#include "../../containers/range.h"
#include "../../containers/cecs_arena.h"
#include "../../containers/tagged_union.h"
#include "entity/component_type.h"
#include "entity/entity.h"


typedef struct storage_info {
    bool is_index_stable : 1;
    bool is_dense : 1;
    bool is_unit_type_storage : 1;
} storage_info;


typedef OPTION_STRUCT(void *, optional_component) optional_component;

typedef storage_info info(const void *self);
typedef optional_component get_component(const void *self, entity_id id, size_t size);
typedef optional_component set_component(void *self, cecs_arena *a, entity_id id, void *component, size_t size);
typedef bool remove_component(void *self, cecs_arena *a, entity_id id, void *out_removed_component, size_t size);
typedef struct component_storage_functions {
    info *const info;
    get_component *const get;
    set_component *const set;
    remove_component *const remove;
} component_storage_functions;


typedef struct unit_component_storage {
    uint_least8_t phantom_data;
} unit_component_storage;

const storage_info unit_component_storage_info(const unit_component_storage *self);

optional_component unit_component_storage_get(const unit_component_storage *self, entity_id id, size_t size);

optional_component unit_component_storage_set(const unit_component_storage *self, cecs_arena *a, entity_id id, void *component, size_t size);

bool unit_component_storage_remove(const unit_component_storage *self, cecs_arena *a, entity_id id, void *out_removed_component, size_t size);

static const component_storage_functions unit_component_storage_functions = {
    .info = (info *const)unit_component_storage_info,
    .get = (get_component *const)unit_component_storage_get,
    .set = (set_component *const)unit_component_storage_set,
    .remove = (remove_component *const)unit_component_storage_remove
};

unit_component_storage unit_component_storage_create();


typedef struct indirect_component_storgage {
    displaced_set component_references;
} indirect_component_storage;

const storage_info indirect_component_storage_info(const indirect_component_storage *self);

optional_component indirect_component_storage_get(const indirect_component_storage *self, entity_id id, size_t size);

optional_component indirect_component_storage_set(indirect_component_storage *self, cecs_arena *a, entity_id id, void *component, size_t size);

bool indirect_component_storage_remove(indirect_component_storage *self, cecs_arena *a, entity_id id, void *out_removed_component, size_t size);

indirect_component_storage indirect_component_storage_create();

static const component_storage_functions indirect_component_storage_functions = {
    .info = (info *const)indirect_component_storage_info,
    .get = (get_component *const)indirect_component_storage_get,
    .set = (set_component *const)indirect_component_storage_set,
    .remove = (remove_component *const)indirect_component_storage_remove
};


typedef struct sparse_component_storage {
    displaced_set components;
} sparse_component_storage;

const storage_info sparse_component_storage_info(const sparse_component_storage *self);

optional_component sparse_component_storage_get(const sparse_component_storage *self, entity_id id, size_t size);

optional_component sparse_component_storage_set(sparse_component_storage *self, cecs_arena *a, entity_id id, void *component, size_t size);

bool sparse_component_storage_remove(sparse_component_storage *self, cecs_arena *a, entity_id id, void *out_removed_component, size_t size);

sparse_component_storage sparse_component_storage_create(cecs_arena *a, size_t component_capacity, size_t component_size);

static const component_storage_functions sparse_component_storage_functions = {
    .info = (info *const)sparse_component_storage_info,
    .get = (get_component *const)sparse_component_storage_get,
    .set = (set_component *const)sparse_component_storage_set,
    .remove = (remove_component *const)sparse_component_storage_remove
};

// TODO: add more storage types

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
    cecs_hibitset entity_bitset;
} component_storage;

component_storage component_storage_create_sparse(cecs_arena *a, size_t component_capacity, size_t component_size);

component_storage component_storage_create_unit(cecs_arena *a);

component_storage component_storage_create_indirect(cecs_arena *a, component_storage *other_storage);

bool component_storage_has(const component_storage *self, entity_id id);

const list *component_storage_components(const component_storage *self);

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

const storage_info component_storage_info(const component_storage *self);

optional_component component_storage_get(const component_storage *self, entity_id id, size_t size);
#define COMPONENT_STORAGE_GET(type, component_storage_ref, entity_id) \
    ((type *)component_storage_get(component_storage_ref, entity_id, sizeof(type)))

void *component_storage_get_unchecked(const component_storage *self, entity_id id, size_t size);

void *component_storage_get_or_null(const component_storage *self, entity_id id, size_t size);

optional_component component_storage_set(component_storage *self, cecs_arena *a, entity_id id, void *component, size_t size);
#define COMPONENT_STORAGE_SET(type, component_storage_ref, arena_ref, entity_id, component_ref) \
    ((type *)component_storage_set(component_storage_ref, arena_ref, entity_id, component_ref, sizeof(type)))

bool component_storage_remove(component_storage *self, cecs_arena *a, entity_id id, void *out_removed_component, size_t size);
#define COMPONENT_STORAGE_REMOVE(type, component_storage_ref, arena_ref, entity_id) \
    ((type *)component_storage_remove(component_storage_ref, arena_ref, entity_id, sizeof(type)))


#endif