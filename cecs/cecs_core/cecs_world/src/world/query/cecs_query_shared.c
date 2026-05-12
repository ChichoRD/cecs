#include "cecs_query_shared.h"

// TODO: static helper collect function for each descriptor set
void cecs_query_shared_acquire(
    const cecs_world_components *const components,
    const cecs_query_descriptor_shared *const descriptor,
    cecs_query_result *const out_result
) {
    for (size_t i = 0; i < descriptor->set_count; i++) {
        const cecs_query_descriptor_shared_set *const set = &descriptor->sets[i];
        const cecs_query_access_type access = (cecs_query_access_type)set->access;
        cecs_debugbreak_fail_unless(
            access == cecs_query_access_type_immutable,
            "if access is any other than immutable, call cecs_query_acquire_mut instead of cecs_query_acquire"
        );
    }

    const cecs_component_registry *unused;
    size_t next_view = 0;
    for (size_t i = 0; i < descriptor->set_count; i++) {
        const cecs_query_descriptor_shared_set *const set = &descriptor->sets[i];
        for (size_t j = 0; j < set->component_count; j++) {
            const cecs_component_type component = set->components[j];
            out_result->view_buffer[next_view] = cecs_view_create(
                cecs_world_components_acquire_registry(components, component.id, &unused),
                component
            );
            ++next_view;
        }
    }
}

void cecs_query_shared_acquire_mut(
    cecs_world_components *const components,
    const cecs_query_descriptor_shared *const descriptor,
    cecs_allocator *const allocators,
    cecs_query_result *const out_result
) {
    const cecs_component_registry *unused;
    cecs_component_registry *unused_mut;
    size_t next_view = 0;
    size_t next_view_mut = 0;
    size_t next_view_alloc = 0;
    for (size_t i = 0; i < descriptor->set_count; i++) {
        const cecs_query_descriptor_shared_set *const set = &descriptor->sets[i];
        const cecs_query_access_type access = (cecs_query_access_type)set->access;
        switch (access) {
        case cecs_query_access_type_immutable:
            for (size_t j = 0; j < set->component_count; j++) {
                const cecs_component_type component = set->components[j];
                out_result->view_buffer[next_view] = cecs_view_create(
                    cecs_world_components_acquire_registry(components, component.id, &unused),
                    component
                );
                ++next_view;
            }
            break;
        case cecs_query_access_type_mut:
            for (size_t j = 0; j < set->component_count; j++) {
                const cecs_component_type component = set->components[j];
                out_result->view_mut_buffer[next_view_mut] = cecs_view_mut_create(
                    cecs_world_components_acquire_registry_mut(components, component.id, &unused_mut),
                    component
                );
                ++next_view_mut;
            }
            break;
        case cecs_query_access_type_mut_alloc:
            for (size_t j = 0; j < set->component_count; j++) {
                const cecs_component_type component = set->components[j];
                cecs_view_mut view_mut = cecs_view_mut_create(
                    cecs_world_components_acquire_registry_mut(components, component.id, &unused_mut),
                    component
                );
                out_result->view_alloc_buffer[next_view_alloc] = cecs_view_alloc_create_take(
                    &allocators[next_view_alloc],
                    &view_mut
                );
                ++next_view_alloc;
            }
            break;
        default:
            cecs_debugbreak_unreachable(
                "invalid access type in query descriptor shared set"
            );
        }
    }
}

void cecs_query_shared_release(
    const cecs_world_components *const components,
    const cecs_query_descriptor_shared *const descriptor,
    cecs_query_result *const result
) {
    for (size_t i = 0; i < descriptor->set_count; i++) {
        const cecs_query_descriptor_shared_set *const set = &descriptor->sets[i];
        const cecs_query_access_type access = (cecs_query_access_type)set->access;
        cecs_debugbreak_fail_unless(
            access == cecs_query_access_type_immutable,
            "if access is any other than immutable, call cecs_query_release_mut instead of cecs_query_release"
        );
    }
    size_t next_view = 0;
    for (size_t i = 0; i < descriptor->set_count; i++) {
        const cecs_query_descriptor_shared_set *const set = &descriptor->sets[i];
        for (size_t j = 0; j < set->component_count; j++) {
            cecs_view_release(&result->view_buffer[next_view], components);
            ++next_view;
        }
    }
}

void cecs_query_shared_release_mut(
    cecs_world_components *const components,
    const cecs_query_descriptor_shared *const descriptor,
    cecs_query_result *const result
) {
    size_t next_view = 0;
    size_t next_view_mut = 0;
    size_t next_view_alloc = 0;
    for (size_t i = 0; i < descriptor->set_count; i++) {
        const cecs_query_descriptor_shared_set *const set = &descriptor->sets[i];
        const cecs_query_access_type access = (cecs_query_access_type)set->access;
        switch (access) {
        case cecs_query_access_type_immutable:
            for (size_t j = 0; j < set->component_count; j++) {
                cecs_view_release(&result->view_buffer[next_view], components);
                ++next_view;
            }
            break;
        case cecs_query_access_type_mut:
            for (size_t j = 0; j < set->component_count; j++) {
                cecs_view_mut_release(&result->view_mut_buffer[next_view_mut], components);
                ++next_view_mut;
            }
            break;
        case cecs_query_access_type_mut_alloc:
            for (size_t j = 0; j < set->component_count; j++) {
                cecs_view_alloc_release(&result->view_alloc_buffer[next_view_alloc], components);
                ++next_view_alloc;
            }
            break;
        default:
            cecs_debugbreak_unreachable(
                "invalid access type in query descriptor shared set"
            );
        }
    }
}
