#ifndef CECS_VIEW_H
#define CECS_VIEW_H

#include "cecs_world_components.h"
#include "cecs_entity_storage.h"
#include "registry/cecs_component.h"
#include <cecs_rwlock.h>

typedef struct cecs_view_unchecked {
    cecs_component_type component;
} cecs_view_unchecked;
const cecs_component_registry *cecs_view_unchecked_registry(const cecs_view_unchecked view, const cecs_world_components *const components);
cecs_component_registry *cecs_view_unchecked_registry_mut(const cecs_view_unchecked view, cecs_world_components *const components);

const void *cecs_view_unchecked_get(const cecs_view_unchecked view, const cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity);
void *cecs_view_unchecked_get_mut(const cecs_view_unchecked view, cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity);

bool cecs_view_unchecked_try_get(
    const cecs_view_unchecked view, const cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity, const void **const out_component
);
bool cecs_view_unchecked_try_get_mut(
    const cecs_view_unchecked view, cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity, void **const out_component
);

bool cecs_view_unchecked_contains(const cecs_view_unchecked view, const cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity);

// TODO: maybe change return to bool and void *const  as void **const out 
void *cecs_view_unchecked_get_or_insert(
    const cecs_view_unchecked view, cecs_allocator *const allocator, cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity
);
void *cecs_view_unchecked_insert_expect(
    const cecs_view_unchecked view, cecs_allocator *const allocator, cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity
);

bool cecs_view_unchecked_remove(
    const cecs_view_unchecked view, cecs_allocator *const allocator, cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity
);
void cecs_view_unchecked_remove_expect(
    const cecs_view_unchecked view, cecs_allocator *const allocator, cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity
);


typedef struct cecs_view {
    cecs_rwlock_borrow borrow;
    cecs_view_unchecked view; 
} cecs_view;

inline bool cecs_view_is_valid(const cecs_view view) {
    return cecs_rwlock_borrow_acquired(view.borrow);
}
inline void cecs_view_expect_valid_or_exit(const cecs_view view) {
    cecs_debugbreak_fail_unless(
        cecs_view_is_valid(view),
        "error: cecs_view_expect_valid_or_exit was called with an invalid view"
    );
}
inline cecs_view cecs_view_create(const cecs_rwlock_borrow borrow, const cecs_component_type component) {
    const cecs_view view = (cecs_view) {
        .borrow = borrow,
        .view = (cecs_view_unchecked){
            .component = component,
        },
    };
    cecs_view_expect_valid_or_exit(view);
    return view;
}
void cecs_view_release(cecs_view *const view, const cecs_world_components *const components);

const cecs_component_registry *cecs_view_registry(const cecs_view view, const cecs_world_components *const components);

// FIXME: change *_get in most places to *_find to signify that the procedure might not just be a single pointer indirection
const void *cecs_view_get(const cecs_view view, const cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity);
// FIXME: change *_try_* to *_ok, I like that name better for fallible operations
bool cecs_view_try_get(
    const cecs_view view, const cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity, const void **const out_component
);
bool cecs_view_contains(const cecs_view view, const cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity);


typedef struct cecs_view_mut {
    cecs_rwlock_borrow_mut borrow;
    cecs_view_unchecked view;
} cecs_view_mut;

inline bool cecs_view_mut_is_valid(const cecs_view_mut view) {
    return cecs_rwlock_borrow_mut_acquired(view.borrow);
}
inline void cecs_view_mut_expect_valid_or_exit(const cecs_view_mut view) {
    cecs_debugbreak_fail_unless(
        cecs_view_mut_is_valid(view),
        "error: cecs_view_mut_expect_valid_or_exit was called with an invalid view"
    );
}
inline cecs_view_mut cecs_view_mut_create(const cecs_rwlock_borrow_mut borrow, const cecs_component_type component) {
    const cecs_view_mut view = (cecs_view_mut) {
        .borrow = borrow,
        .view = (cecs_view_unchecked){
            .component = component,
        },
    };
    cecs_view_mut_expect_valid_or_exit(view);
    return view;
}
void cecs_view_mut_release(cecs_view_mut *const view, cecs_world_components *const components);

cecs_component_registry *cecs_view_mut_registry(const cecs_view_mut view, cecs_world_components *const components);

const void *cecs_view_mut_get(const cecs_view_mut view, const cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity);
void *cecs_view_mut_get_mut(const cecs_view_mut view, cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity);

bool cecs_view_mut_try_get(
    const cecs_view_mut view, const cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity, const void **const out_component
);
bool cecs_view_mut_try_get_mut(
    const cecs_view_mut view, cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity, void **const out_component
);

bool cecs_view_mut_contains(const cecs_view_mut view, const cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity);


// TODO: reserve, etc.
typedef struct cecs_view_alloc {
    cecs_allocator allocator;
    cecs_view_mut view;
} cecs_view_alloc;


inline const cecs_view_mut *cecs_view_alloc_view(const cecs_view_alloc *const view) {
    return &view->view;
}
inline cecs_view_mut *cecs_view_alloc_view_mut(cecs_view_alloc *const view) {
    return &view->view;
}
inline bool cecs_view_alloc_is_valid(const cecs_view_alloc *const view) {
    return cecs_view_mut_is_valid(view->view);
}
inline void cecs_view_alloc_expect_valid_or_exit(const cecs_view_alloc *const view) {
    cecs_view_mut_expect_valid_or_exit(view->view);
}

cecs_view_alloc cecs_view_alloc_create_take(cecs_allocator *const allocator, cecs_view_mut *const view);
// FIXME: name *_unchecked or *_implicit to signify that the caller must ensure the returned view is the sole owner
// of the allocator and the view's mutex guard. No other reference must alias these resources!
cecs_view_alloc cecs_view_alloc_create(const cecs_allocator allocator, const cecs_view_mut view);
void cecs_view_alloc_release(cecs_view_alloc *const view, cecs_world_components *const components);

void *cecs_view_alloc_get_or_insert(
    cecs_view_alloc *const view, cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity
);
// TODO: change API to insert_many at once
void *cecs_view_alloc_insert_expect(
    cecs_view_alloc *const view, cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity
);

bool cecs_view_alloc_remove(
    cecs_view_alloc *const view, cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity
);
void cecs_view_alloc_remove_expect(
    cecs_view_alloc *const view, cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity
);


#endif
