#include "cecs_query.h"
#include <cecs_error.h>
#include <relations/cecs_ordering.h>

bool cecs_query_term_match(
    const cecs_query_match_value match,
    const cecs_component_registry *const *const registries,
    const size_t registry_count,
    const cecs_entity_index entity
) {
    const cecs_query_match_type match_type = (cecs_query_match_type)match;
    switch (match_type) {
    case cecs_query_match_type_all: {
        for (size_t i = 0; i < registry_count; i++) {
            if (!cecs_component_storage_contains(&registries[i]->storage, entity, registries[i]->component_size)) {
                return false;
            }
        }
        return true;
    }
    case cecs_query_match_type_any: {
        for (size_t i = 0; i < registry_count; i++) {
            if (cecs_component_storage_contains(&registries[i]->storage, entity, registries[i]->component_size)) {
                return true;
            }
        }
        return false;
    }
    case cecs_query_match_type_none_of: {
        for (size_t i = 0; i < registry_count; i++) {
            if (cecs_component_storage_contains(&registries[i]->storage, entity, registries[i]->component_size)) {
                return false;
            }
        }
        return true;
    }
    default:
        cecs_debugbreak_unreachable("invalid query match type");
    }
}
size_t cecs_query_term_count_min(const cecs_query_term term, const cecs_component_registry *const *const registries, const size_t registry_count) {
    size_t min = SIZE_MAX;
    for (size_t i = 0; i < term.view_count && i < registry_count; i++) {
        const cecs_component_registry *const registry = registries[i];
        const size_t count = cecs_component_storage_count(&registry->storage);
        if (count < min) {
            min = count;
        }
    }
    return min;
}

size_t cecs_query_term_find_storages(
    const cecs_query_term term, 
    const cecs_world_components *const components,
    const cecs_component_registry **const out_registries,
    const size_t out_registry_count
) {
    static_assert(
        cecs_query_access_shared <= cecs_query_access_mut,
        "static error: cecs_query_term_find_storages requires that the value of cecs_query_access_mut is greater than the value of cecs_query_access_shared"
    );
#if cecs_query_access_shared != 0
    cecs_debugbreak_fail_unless(
        term.access >= cecs_query_access_shared,
        "cecs_query_term_find_storages may only be used to find storages for query terms with shared access\n"
        "note: use cecs_query_term_find_storages_mut to find storages for query terms with mutable access"
    );
#endif
    const size_t storages_max = cecs_min(term.view_count, out_registry_count);
    for (size_t i = 0; i < storages_max; ++i) {
        const cecs_view_unchecked view = term.views[i];
        const cecs_component_registry *const registry = cecs_view_unchecked_registry(view, components);
        out_registries[i] = registry;
    }
    return storages_max;
}
size_t cecs_query_term_find_storages_mut(
    const cecs_query_term term,
    cecs_world_components *const components,
    cecs_component_registry **const out_registries,
    const size_t out_registry_count
) {
    static_assert(
        cecs_query_access_shared <= cecs_query_access_mut,
        "static error: cecs_query_term_find_storages_mut requires that the value of cecs_query_access_mut is greater than the value of cecs_query_access_shared"
    );
#if cecs_query_access_mut != 0
    cecs_debugbreak_fail_unless(
        term.access >= cecs_query_access_mut,
        "cecs_query_term_find_storages_mut may only be used to find storages for query terms with mutable access\n"
        "note: use cecs_query_term_find_storages to find storages for query terms with shared access"
    );
#endif
    const size_t storages_max = cecs_min(term.view_count, out_registry_count);
    for (size_t i = 0; i < storages_max; ++i) {
        const cecs_view_unchecked view = term.views[i];
        cecs_component_registry *const registry = cecs_view_unchecked_registry_mut(view, components);
        out_registries[i] = registry;
    }
    return storages_max;
}


size_t cecs_query_find_storages(
    const cecs_query query,
    const cecs_world_components *const components,
    const cecs_component_registry **const out_registries,
    const size_t out_registry_count
) {
    size_t out_offset = 0;
    size_t out_count = out_registry_count;
    for (size_t i = 0; i < query.term_count; ++i) {
        const cecs_query_term term = query.terms[i];
        const size_t found_count = cecs_query_term_find_storages(term, components, out_registries + out_offset, out_count);
        out_offset += found_count;
        out_count -= found_count;
    }
    return out_offset;
}
size_t cecs_query_find_storages_mut(
    const cecs_query query,
    cecs_world_components *const components,
    cecs_query_registry *const out_registries,
    const size_t out_registry_count
) {
    size_t out_offset = 0;
    size_t out_count = out_registry_count;
    for (size_t i = 0; i < query.term_count; ++i) {
        const cecs_query_term term = query.terms[i];
        size_t found_count;
        if (term.access == cecs_query_access_mut) {
            found_count = cecs_query_term_find_storages_mut(term, components, &out_registries[out_offset].registry_mut, out_count);
        } else {
            found_count = cecs_query_term_find_storages(term, components, &out_registries[out_offset].registry, out_count);
        }
        out_offset += found_count;
        out_count -= found_count;
    }
    return out_offset;
}

bool cecs_query_match(
    const cecs_query query,
    const cecs_component_registry **const registries,
    const size_t registry_count,
    const cecs_entity_index entity
) {
    size_t term_storages_offset = 0;
    for (size_t i = 0; i < query.term_count; ++i) {
        const cecs_query_term term = query.terms[i];
        cecs_debugbreak_fail_unless(
            term.view_count <= registry_count - term_storages_offset,
            "invalid query: not enough registries provided to match all query terms"
        );

        if (!cecs_query_term_match(term.match, registries + term_storages_offset, term.view_count, entity)) {
            return false;
        }
        term_storages_offset += term.view_count;
    }
    return true;
}
size_t cecs_query_count_min(const cecs_query query, const cecs_component_registry **const registries, const size_t registry_count) {
    size_t min = SIZE_MAX;
    size_t term_storages_offset = 0;
    for (size_t i = 0; i < query.term_count; ++i) {
        const cecs_query_term term = query.terms[i];
        cecs_debugbreak_fail_unless(
            term.view_count <= registry_count - term_storages_offset,
            "invalid query: not enough registries provided to count min for all query terms"
        );

        const size_t count = cecs_query_term_count_min(term, registries + term_storages_offset, term.view_count);
        if (count < min) {
            min = count;
        }
        term_storages_offset += term.view_count;
    }
    return min;
}
