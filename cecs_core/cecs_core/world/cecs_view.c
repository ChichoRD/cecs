#include "cecs_view.h"

const cecs_component_registry *cecs_view_unchecked_registry(const cecs_view_unchecked view, const cecs_world_components *const components) {
    const cecs_registry *registry = cecs_world_components_get_registry(components, view.component.id);
    return cecs_registry_get_unchecked(registry);
}
cecs_component_registry *cecs_view_unchecked_registry_mut(const cecs_view_unchecked view, cecs_world_components *const components) {
    cecs_registry *registry = cecs_world_components_get_registry_mut(components, view.component.id);
    return cecs_registry_get_mut_unchecked(registry);
}

const void *cecs_view_unchecked_get(const cecs_view_unchecked view, const cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity) {
    const cecs_component_registry *const registry = cecs_view_unchecked_registry(view, components);
    const cecs_entity stored_entity = cecs_entity_storage_get_entity_exact(entities, entity);

    return cecs_component_storage_get(&registry->storage, cecs_entity_index(stored_entity), registry->component_size);
}
void *cecs_view_unchecked_get_mut(const cecs_view_unchecked view, cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity) {
    const cecs_component_registry *const registry = cecs_view_unchecked_registry_mut(view, components);
    const cecs_entity stored_entity = cecs_entity_storage_get_entity_exact(entities, entity);

    return cecs_component_storage_get_mut(&registry->storage, cecs_entity_index(stored_entity), registry->component_size);
}

bool cecs_view_unchecked_try_get(
    const cecs_view_unchecked view, const cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity, const void **const out_component
) {
    const cecs_component_registry *const registry = cecs_view_unchecked_registry(view, components);
    const cecs_entity stored_entity = cecs_entity_storage_get_entity_exact(entities, entity);
    
    const size_t index = cecs_entity_index(stored_entity);
    if (cecs_component_storage_contains(&registry->storage, index, registry->component_size)) {
        *out_component = cecs_component_storage_get(&registry->storage, index, registry->component_size);
        return true;
    } else {
        *out_component = NULL;
        return false;
    }
}
bool cecs_view_unchecked_try_get_mut(
    const cecs_view_unchecked view, cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity, void **const out_component
) {
    const cecs_component_registry *const registry = cecs_view_unchecked_registry_mut(view, components);
    const cecs_entity stored_entity = cecs_entity_storage_get_entity_exact(entities, entity);
    
    const size_t index = cecs_entity_index(stored_entity);
    if (cecs_component_storage_contains(&registry->storage, index, registry->component_size)) {
        *out_component = cecs_component_storage_get_mut(&registry->storage, index, registry->component_size);
        return true;
    } else {
        *out_component = NULL;
        return false;
    }
}

bool cecs_view_unchecked_contains(const cecs_view_unchecked view, const cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity) {
    const cecs_component_registry *const registry = cecs_view_unchecked_registry(view, components);
    const cecs_entity stored_entity = cecs_entity_storage_get_entity_exact(entities, entity);
    
    const size_t index = cecs_entity_index(stored_entity);
    return cecs_component_storage_contains(&registry->storage, index, registry->component_size);
}

void *cecs_view_unchecked_get_or_insert(
    const cecs_view_unchecked view, cecs_allocator *const allocator, cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity
) {
    cecs_unimplemented_fail("TODO: component groups!");
    cecs_component_registry *const registry = cecs_view_unchecked_registry_mut(view, components);
    const cecs_entity stored_entity = cecs_entity_storage_get_entity_exact(entities, entity);

    return cecs_component_storage_get_or_insert(&registry->storage, allocator, cecs_entity_index(stored_entity), registry->component_size);
}
void *cecs_view_unchecked_insert_expect(
    const cecs_view_unchecked view, cecs_allocator *const allocator, cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity
){
    cecs_unimplemented_fail("TODO: component groups!");
    cecs_component_registry *const registry = cecs_view_unchecked_registry_mut(view, components);
    const cecs_entity stored_entity = cecs_entity_storage_get_entity_exact(entities, entity);

    return cecs_component_storage_insert_expect(&registry->storage, allocator, cecs_entity_index(stored_entity), registry->component_size);
}

bool cecs_view_unchecked_remove(
    const cecs_view_unchecked view, cecs_allocator *const allocator, cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity
) {
    cecs_unimplemented_fail("TODO: component groups!");
    cecs_component_registry *const registry = cecs_view_unchecked_registry_mut(view, components);
    const cecs_entity stored_entity = cecs_entity_storage_get_entity_exact(entities, entity);

    return cecs_component_storage_remove(&registry->storage, allocator, cecs_entity_index(stored_entity), registry->component_size);
}
void cecs_view_unchecked_remove_expect(
    const cecs_view_unchecked view, cecs_allocator *const allocator, cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity
) {
    cecs_unimplemented_fail("TODO: component groups!");
    cecs_component_registry *const registry = cecs_view_unchecked_registry_mut(view, components);
    const cecs_entity stored_entity = cecs_entity_storage_get_entity_exact(entities, entity);

    cecs_component_storage_remove_expect(&registry->storage, allocator, cecs_entity_index(stored_entity), registry->component_size);
}

extern inline bool cecs_view_is_valid(const cecs_view view);
extern inline void cecs_view_expect_valid_or_exit(const cecs_view view);
extern inline cecs_view cecs_view_create(const cecs_rwlock_borrow borrow, const cecs_component_type component);
void cecs_view_release(cecs_view *const view, cecs_world_components *const components) {
    cecs_registry *registry = cecs_world_components_get_registry_mut(components, view->view.component.id);
    cecs_registry_release(registry, &view->borrow);
}

const cecs_component_registry *cecs_view_registry(const cecs_view view, const cecs_world_components *const components) {
    cecs_view_expect_valid_or_exit(view);
    const cecs_registry *registry = cecs_world_components_get_registry(components, view.view.component.id);
    return cecs_registry_get_unchecked(registry);
}

const void *cecs_view_get(const cecs_view view, const cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity) {
    cecs_view_expect_valid_or_exit(view);
    return cecs_view_unchecked_get(view.view, components, entities, entity);
}
bool cecs_view_try_get(
    const cecs_view view, const cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity, const void **const out_component
) {
    cecs_view_expect_valid_or_exit(view);
    return cecs_view_unchecked_try_get(view.view, components, entities, entity, out_component);
}
bool cecs_view_contains(const cecs_view view, const cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity) {
    cecs_view_expect_valid_or_exit(view);
    return cecs_view_unchecked_contains(view.view, components, entities, entity);
}


extern inline bool cecs_view_mut_is_valid(const cecs_view_mut view);
extern inline void cecs_view_mut_expect_valid_or_exit(const cecs_view_mut view);
extern inline cecs_view_mut cecs_view_mut_create(const cecs_rwlock_borrow_mut borrow, const cecs_component_type component);

void cecs_view_mut_release(cecs_view_mut *const view, cecs_world_components *const components) {
    cecs_registry *const registry = cecs_world_components_get_registry_mut(components, view->view.component.id);
    cecs_registry_release_mut(registry, &view->borrow);
}

cecs_component_registry *cecs_view_mut_registry(const cecs_view_mut view, cecs_world_components *const components) {
    cecs_view_mut_expect_valid_or_exit(view);
    cecs_registry *registry = cecs_world_components_get_registry_mut(components, view.view.component.id);
    return cecs_registry_get_mut_unchecked(registry);
}


const void *cecs_view_mut_get(const cecs_view_mut view, const cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity) {
    cecs_view_mut_expect_valid_or_exit(view);
    return cecs_view_unchecked_get(view.view, components, entities, entity);
}
void *cecs_view_mut_get_mut(const cecs_view_mut view, cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity) {
    cecs_view_mut_expect_valid_or_exit(view);
    return cecs_view_unchecked_get_mut(view.view, components, entities, entity);
}

bool cecs_view_mut_try_get(
    const cecs_view_mut view, const cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity, const void **const out_component
) {
    cecs_view_mut_expect_valid_or_exit(view);
    return cecs_view_unchecked_try_get(view.view, components, entities, entity, out_component);
}
void *cecs_view_mut_try_get_mut(
    const cecs_view_mut view, cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity, void **const out_component
) {
    cecs_view_mut_expect_valid_or_exit(view);
    return cecs_view_unchecked_try_get_mut(view.view, components, entities, entity, out_component);
}

bool cecs_view_mut_contains(const cecs_view_mut view, const cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity) {
    cecs_view_mut_expect_valid_or_exit(view);
    return cecs_view_unchecked_contains(view.view, components, entities, entity);
}


extern inline const cecs_view_mut *cecs_view_alloc_view(const cecs_view_alloc *const view);
extern inline cecs_view_mut *cecs_view_alloc_view_mut(cecs_view_alloc *const view);
extern inline bool cecs_view_alloc_is_valid(const cecs_view_alloc *const view);
extern inline void cecs_view_alloc_expect_valid_or_exit(const cecs_view_alloc *const view);

cecs_view_alloc cecs_view_alloc_create(cecs_allocator *const allocator, cecs_view_mut *const view) {
    cecs_view_alloc view_alloc = (cecs_view_alloc) {
        .allocator = *allocator,
        .view = *view,
    };
    allocator->type = cecs_internal_allocator_type_none;
    cecs_rwlock_borrow_mut_release(&view->borrow);
    return view_alloc;
}

void cecs_view_alloc_release(cecs_view_alloc *const  view, cecs_world_components *const components) {
    cecs_view_mut_release(&view->view, components);
}

void *cecs_view_alloc_get_or_insert(
    cecs_view_alloc *const view, cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity
) {
    cecs_view_alloc_expect_valid_or_exit(view);
    return cecs_view_unchecked_get_or_insert(view->view.view, &view->allocator, components, entities, entity);
}
void *cecs_view_alloc_insert_expect(
    cecs_view_alloc *const view, cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity
) {
    cecs_view_alloc_expect_valid_or_exit(view);
    return cecs_view_unchecked_insert_expect(view->view.view, &view->allocator, components, entities, entity);
}

bool cecs_view_alloc_remove(
    cecs_view_alloc *const view, cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity
) {
    cecs_view_alloc_expect_valid_or_exit(view);
    return cecs_view_unchecked_remove(view->view.view, &view->allocator, components, entities, entity);
}
void cecs_view_alloc_remove_expect(
    cecs_view_alloc *const view, cecs_world_components *const components, const cecs_entity_storage *const entities, const cecs_entity entity
) {
    cecs_view_alloc_expect_valid_or_exit(view);
    cecs_view_unchecked_remove_expect(view->view.view, &view->allocator, components, entities, entity);
}
