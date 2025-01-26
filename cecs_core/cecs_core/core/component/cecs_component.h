#ifndef CECS_COMPONENT_H
#define CECS_COMPONENT_H

#include <stdint.h>
#include <stdbool.h>
#include "../../types/cecs_type_id.h"
#include "../../containers/cecs_union.h"
#include "../../containers/cecs_sparse_set.h"
#include "../../containers/cecs_arena.h"
#include "../../containers/cecs_discard.h"
#include "entity/cecs_entity.h"
#include "cecs_component_storage.h"

typedef uint32_t cecs_world_components_checksum;
cecs_world_components_checksum cecs_world_components_checksum_hash(cecs_world_components_checksum current);
inline cecs_world_components_checksum cecs_world_components_checksum_add(cecs_world_components_checksum current, cecs_component_id component_id) {
    return cecs_world_components_checksum_hash(current + (cecs_world_components_checksum)component_id);
}
inline cecs_world_components_checksum cecs_world_components_checksum_remove(cecs_world_components_checksum current, cecs_component_id component_id) {
    return cecs_world_components_checksum_hash(current - (cecs_world_components_checksum)component_id);
}

typedef cecs_discard cecs_component_discard;

typedef CECS_OPTION_STRUCT(size_t *, cecs_optional_component_size) cecs_optional_component_size;
typedef enum cecs_component_storage_attachment_usage {
    cecs_component_storage_attachment_usage_none = 0,
    cecs_component_storage_attachment_usage_user = 1 << 0,
} cecs_component_storage_attachment_usage;
typedef uint32_t cecs_component_storage_attachment_usage_flags;

typedef struct cecs_component_storage_attachments {
    void *user_attachments;
    size_t attachments_size;
    cecs_component_storage_attachment_usage_flags flags;
} cecs_component_storage_attachments;

typedef struct cecs_world_components {
    cecs_arena storages_arena;
    cecs_arena components_arena;
    cecs_paged_sparse_set component_storages;
    cecs_paged_sparse_set component_storages_attachments;
    cecs_paged_sparse_set component_sizes;
    cecs_world_components_checksum checksum;
    cecs_component_discard discard;
} cecs_world_components;

cecs_world_components cecs_world_components_create(size_t component_type_capacity);

void cecs_world_components_free(cecs_world_components *wc);

static inline size_t cecs_world_components_get_component_storage_count(const cecs_world_components *wc) {
    return cecs_paged_sparse_set_count_of_size(&wc->component_storages, sizeof(cecs_component_storage));
}

cecs_optional_component_size cecs_world_components_get_component_size(const cecs_world_components *wc, cecs_component_id component_id);
static inline size_t cecs_world_components_get_component_size_unchecked(const cecs_world_components *wc, cecs_component_id component_id) {
    return *CECS_OPTION_GET(cecs_optional_component_size, cecs_world_components_get_component_size(wc, component_id));
}

typedef CECS_OPTION_STRUCT(cecs_component_storage *, cecs_optional_component_storage) cecs_optional_component_storage;
cecs_optional_component_storage cecs_world_components_get_component_storage(const cecs_world_components *wc, cecs_component_id component_id);
cecs_component_storage *cecs_world_components_get_component_storage_unchecked(const cecs_world_components *wc, cecs_component_id component_id);

bool cecs_world_components_has_storage(const cecs_world_components *wc, cecs_component_id component_id);

typedef cecs_component_id cecs_indirect_component_id;
typedef struct cecs_component_storage_descriptor {
    bool is_size_known;
    size_t capacity;
    CECS_OPTION_STRUCT(cecs_component_id, cecs_indirect_component_id) indirect_component_id;
} cecs_component_storage_descriptor;

cecs_component_storage cecs_component_storage_descriptor_build(
    cecs_component_storage_descriptor descriptor,
    cecs_world_components *wc,
    size_t component_size
);

cecs_optional_component cecs_world_components_set_component(
    cecs_world_components *wc,
    cecs_entity_id entity_id,
    cecs_component_id component_id,
    void *component,
    size_t size,
    cecs_component_storage_descriptor additional_storage_descriptor
);

cecs_optional_component_array cecs_world_components_set_component_array(
    cecs_world_components* wc,
    cecs_entity_id entity_id,
    cecs_component_id component_id,
    void* components,
    size_t count,
    size_t size,
    cecs_component_storage_descriptor additional_storage_descriptor
);

cecs_optional_component_array cecs_world_components_set_component_copy_array(
    cecs_world_components *wc,
    cecs_entity_id entity_id,
    cecs_component_id component_id,
    void *component_single_src,
    size_t count,
    size_t size,
    cecs_component_storage_descriptor additional_storage_descriptor
);

static inline void *cecs_world_components_set_component_unchecked(
    cecs_world_components *wc,
    cecs_entity_id entity_id,
    cecs_component_id component_id,
    void *component,
    size_t size,
    cecs_component_storage_descriptor additional_storage_descriptor
) {
    return CECS_OPTION_GET(
        cecs_optional_component,
        cecs_world_components_set_component(wc, entity_id, component_id, component, size, additional_storage_descriptor)
    );
}

static inline void *cecs_world_components_set_component_array_unchecked(
    cecs_world_components *wc,
    cecs_entity_id entity_id,
    cecs_component_id component_id,
    void *components,
    size_t count,
    size_t size,
    cecs_component_storage_descriptor additional_storage_descriptor
) {
    return CECS_OPTION_GET(
        cecs_optional_component_array,
        cecs_world_components_set_component_array(wc, entity_id, component_id, components, count, size, additional_storage_descriptor)
    );
}

static inline void *cecs_world_components_set_component_copy_array_unchecked(
    cecs_world_components *wc,
    cecs_entity_id entity_id,
    cecs_component_id component_id,
    void *component_single_src,
    size_t count,
    size_t size,
    cecs_component_storage_descriptor additional_storage_descriptor
) {
    return CECS_OPTION_GET(
        cecs_optional_component_array,
        cecs_world_components_set_component_copy_array(wc, entity_id, component_id, component_single_src, count, size, additional_storage_descriptor)
    );
}

bool cecs_world_components_has_component(const cecs_world_components *wc, cecs_entity_id entity_id, cecs_component_id component_id);

cecs_optional_component cecs_world_components_get_component(
    const cecs_world_components *wc,
    cecs_entity_id entity_id, 
    cecs_component_id component_id
);
size_t cecs_world_components_get_component_array(
    const cecs_world_components* wc,
    cecs_entity_id entity_id,
    cecs_component_id component_id,
    void** out_components,
    size_t count
);

static inline void *cecs_world_components_get_component_unchecked(
    const cecs_world_components *wc,
    cecs_entity_id entity_id,
    cecs_component_id component_id
) {
    return CECS_OPTION_GET(cecs_optional_component, cecs_world_components_get_component(wc, entity_id, component_id));
}

bool cecs_world_components_remove_component(
    cecs_world_components *wc,
    cecs_entity_id entity_id,
    cecs_component_id component_id,
    void *out_removed_component
);

size_t cecs_world_components_remove_component_array(
    cecs_world_components *wc,
    cecs_entity_id entity_id,
    cecs_component_id component_id,
    void *out_removed_components,
    size_t count
);

const cecs_component_storage_attachments *cecs_world_components_set_component_storage_attachments(
    cecs_world_components *wc,
    cecs_component_id component_id,
    void *attachments,
    size_t size,
    cecs_component_storage_attachment_usage_flags flags
);

bool cecs_world_components_has_component_storage_attachments(const cecs_world_components *wc, cecs_component_id component_id);
const cecs_component_storage_attachments *cecs_world_components_get_component_storage_attachments_unchecked(const cecs_world_components *wc, cecs_component_id component_id);

const cecs_component_storage_attachments *cecs_world_components_get_or_set_component_storage_attachments(
    cecs_world_components *wc,
    cecs_component_id component_id,
    void *attachments,
    size_t size,
    cecs_component_storage_attachment_usage_flags flags
);

bool cecs_world_components_remove_component_storage_attachments(
    cecs_world_components *wc,
    cecs_component_id component_id,
    cecs_component_storage_attachments *out_removed_attachments
);


typedef struct cecs_sized_component_storage {
    cecs_component_storage *storage;
    size_t component_size;
    cecs_component_id component_id;
} cecs_sized_component_storage;

typedef struct cecs_world_components_iterator {
    const cecs_world_components *const components;
    size_t storage_raw_index;
} cecs_world_components_iterator;

cecs_world_components_iterator cecs_world_components_iterator_create(const cecs_world_components *components);

bool cecs_world_components_iterator_done(const cecs_world_components_iterator *it);

size_t cecs_world_components_iterator_next(cecs_world_components_iterator *it);

cecs_sized_component_storage cecs_world_components_iterator_current(const cecs_world_components_iterator *it);


typedef struct cecs_world_components_entity_iterator {
    cecs_world_components_iterator it;
    const cecs_entity_id entity_id;
} cecs_world_components_entity_iterator;


cecs_world_components_entity_iterator cecs_world_components_entity_iterator_create(const cecs_world_components *components, cecs_entity_id entity_id);

bool cecs_world_components_entity_iterator_done(const cecs_world_components_entity_iterator *it);

size_t cecs_world_components_entity_iterator_next(cecs_world_components_entity_iterator *it);

cecs_sized_component_storage cecs_world_components_entity_iterator_current(const cecs_world_components_entity_iterator *it);

#endif