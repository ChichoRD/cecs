#include "cecs_world.h"

cecs_world cecs_world_create(size_t entity_capacity, size_t component_type_capacity, size_t resource_capacity) {
    cecs_world w;
    w.entities = cecs_world_entities_create(entity_capacity);
    w.components = cecs_world_components_create(component_type_capacity);
    w.relations = cecs_world_relations_create(entity_capacity);

    const size_t resource_default_size = sizeof(intptr_t) * 4;
    w.resources = cecs_world_resources_create(resource_capacity, resource_default_size);
    return w;
}

void* cecs_world_get_component(const cecs_world* w, cecs_entity_id entity_id, cecs_component_id component_id) {
    return cecs_world_components_get_component_unchecked(&w->components, entity_id, component_id);
}

bool cecs_world_try_get_component(const cecs_world* w, cecs_entity_id entity_id, cecs_component_id component_id, void** out_component) {
    cecs_optional_component component = cecs_world_components_get_component(&w->components, entity_id, component_id);
    if (CECS_OPTION_IS_SOME(cecs_optional_component, component)) {
        *out_component = CECS_OPTION_GET(cecs_optional_component, component);
        return true;
    }
    else {
        *out_component = NULL;
        return false;
    }
}

cecs_entity_flags cecs_world_get_entity_flags(const cecs_world* w, cecs_entity_id entity_id) {
    assert(cecs_world_enities_has_entity(&w->entities, entity_id) && "entity with given ID does not exist");
    cecs_entity_flags* flags = NULL;
    if (CECS_WORLD_TRY_GET_COMPONENT(cecs_entity_flags, w, entity_id, &flags)) {
        return *flags;
    }
    else {
        return cecs_entity_flags_default();
    }
}

void* cecs_world_set_component(cecs_world* w, cecs_entity_id id, cecs_component_id component_id, void* component, size_t size) {
    assert(cecs_world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    assert(
        !cecs_world_get_entity_flags(w, id).is_inmutable
        && "entity with given ID is inmutable and its components may not be set"
    );

    return cecs_world_components_set_component_unchecked(
        &w->components,
        id,
        component_id,
        component,
        size,
        (cecs_component_storage_descriptor) {
        .capacity = 1,
            .is_size_known = true,
            .indirect_component_id = { 0 }
    }
    );
}

bool cecs_world_remove_component(cecs_world* w, cecs_entity_id id, cecs_component_id component_id, void* out_removed_component) {
    assert(cecs_world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    assert(
        !cecs_world_get_entity_flags(w, id).is_inmutable
        && "entity with given ID is inmutable and components may not be removed from it"
    );

    return cecs_world_components_remove_component(&w->components, id, component_id, out_removed_component);
}

cecs_entity_flags* cecs_world_get_or_set_default_entity_flags(cecs_world* w, cecs_entity_id id) {
    assert(cecs_world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    cecs_entity_flags* flags = NULL;
    if (CECS_WORLD_TRY_GET_COMPONENT(cecs_entity_flags, w, id, &flags)) {
        return flags;
    }
    else {
        return cecs_world_set_entity_flags(w, id, cecs_entity_flags_default());
    }
}

bool cecs_world_has_tag(const cecs_world* w, cecs_entity_id id, cecs_tag_id tag_id) {
    if (!cecs_world_enities_has_entity(&w->entities, id))
        return false;

    return cecs_world_components_has_component(&w->components, id, tag_id);
}

cecs_tag_id cecs_world_add_tag(cecs_world* w, cecs_entity_id id, cecs_tag_id tag_id) {
    assert(cecs_world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    assert(
        !cecs_world_get_entity_flags(w, id).is_inmutable
        && "entity with given ID is inmutable and tags may not be added to it"
    );

    cecs_world_components_set_component(
        &w->components,
        id,
        tag_id,
        NULL,
        0,
        (cecs_component_storage_descriptor) {
        .capacity = 1,
            .is_size_known = true,
            .indirect_component_id = { 0 }
    }
    );
    return tag_id;
}

cecs_tag_id cecs_world_remove_tag(cecs_world* w, cecs_entity_id id, cecs_tag_id tag_id) {
    assert(cecs_world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    assert(
        !cecs_world_get_entity_flags(w, id).is_inmutable
        && "entity with given ID is inmutable and tags may not be removed from it"
    );

    cecs_world_components_remove_component(&w->components, id, tag_id, &(cecs_optional_component){0});
    return tag_id;
}

cecs_entity_id cecs_world_add_entity(cecs_world* w) {
    cecs_entity_id e = cecs_world_entities_add_entity(&w->entities);
#if CECS_WORLD_FLAG_ALL_ENTITIES
    cecs_world_set_entity_flags(w, e, cecs_entity_flags_default());
#endif
    return e;
}

cecs_entity_id cecs_world_clear_entity(cecs_world* w, cecs_entity_id entity_id) {
    assert(
        !cecs_world_get_entity_flags(w, entity_id).is_inmutable
        && "entity with given ID is inmutable and cannot be cleared"
    );

    for (
        cecs_world_components_entity_iterator it = cecs_world_components_entity_iterator_create(&w->components, entity_id);
        !cecs_world_components_entity_iterator_done(&it);
        cecs_world_components_entity_iterator_next(&it)
        ) {
        cecs_sized_component_storage storage = cecs_world_components_entity_iterator_current(&it);
        cecs_component_storage_remove(
            storage.storage,
            &w->components.components_arena,
            entity_id,
            w->components.discard.handle,
            storage.component_size
        );
    }
    return entity_id;
}

cecs_entity_id cecs_world_copy_entity_onto(cecs_world* w, cecs_entity_id destination, cecs_entity_id source) {
    assert(
        !cecs_world_get_entity_flags(w, destination).is_inmutable
        && "entity with given ID is inmutable and components may not be copied onto it"
    );

    for (
        cecs_world_components_entity_iterator it = cecs_world_components_entity_iterator_create(&w->components, source);
        !cecs_world_components_entity_iterator_done(&it);
        cecs_world_components_entity_iterator_next(&it)
        ) {
        cecs_sized_component_storage storage = cecs_world_components_entity_iterator_current(&it);
        cecs_component_storage_set(
            storage.storage,
            &w->components.components_arena,
            destination,
            cecs_component_storage_info(storage.storage).is_unit_type_storage
            ? NULL
            : CECS_OPTION_GET(cecs_optional_component, cecs_component_storage_get(
                storage.storage,
                source,
                storage.component_size
            )),
            storage.component_size
        );
    }

    return destination;
}

cecs_entity_id cecs_world_copy_entity(cecs_world* w, cecs_entity_id destination, cecs_entity_id source) {
    cecs_world_clear_entity(w, destination);
    return cecs_world_copy_entity_onto(w, destination, source);
}

cecs_entity_id cecs_world_add_entity_copy(cecs_world* w, cecs_entity_id source) {
    return cecs_world_copy_entity_onto(w, cecs_world_add_entity(w), source);
}

void* cecs_world_copy_entity_onto_and_grab(cecs_world* w, cecs_entity_id destination, cecs_entity_id source, cecs_component_id grab_component_id) {
    assert(
        !cecs_world_get_entity_flags(w, destination).is_inmutable
        && "entity with given ID is inmutable and components may not be copied onto it"
    );

    void* grabbed = NULL;
    for (
        cecs_world_components_entity_iterator it = cecs_world_components_entity_iterator_create(&w->components, source);
        !cecs_world_components_entity_iterator_done(&it);
        cecs_world_components_entity_iterator_next(&it)
        ) {
        cecs_sized_component_storage storage = cecs_world_components_entity_iterator_current(&it);
        cecs_optional_component copied_component = cecs_component_storage_set(
            storage.storage,
            &w->components.components_arena,
            destination,
            cecs_component_storage_info(storage.storage).is_unit_type_storage
            ? NULL
            : CECS_OPTION_GET(cecs_optional_component, cecs_component_storage_get(
                storage.storage,
                source,
                storage.component_size
            )),
            storage.component_size
        );

        if (CECS_OPTION_IS_SOME(cecs_optional_component, copied_component) && grab_component_id == storage.component_id) {
            grabbed = CECS_OPTION_GET(cecs_optional_component, copied_component);
        }
    }

    return grabbed;
}

void* cecs_world_copy_entity_and_grab(cecs_world* w, cecs_entity_id destination, cecs_entity_id source, cecs_component_id grab_component_id) {
    cecs_world_clear_entity(w, destination);
    return cecs_world_copy_entity_onto_and_grab(w, destination, source, grab_component_id);
}

void* cecs_world_set_component_relation(cecs_world* w, cecs_entity_id id, cecs_component_id component_id, void* component, size_t size, cecs_tag_id tag_id) {
    assert(cecs_world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    cecs_entity_id extra_id = cecs_world_add_entity(w);
    cecs_entity_id associated_id = cecs_world_relations_increment_associated_id_or_set(
        &w->relations,
        id,
        (cecs_relation_target) {
        tag_id
    },
        extra_id
    );
    if (associated_id != extra_id) {
        cecs_world_entities_remove_entity(&w->entities, extra_id);
    }
    else {
        cecs_world_set_component(
            w,
            associated_id,
            CECS_TYPE_ID(CECS_COMPONENT(cecs_relation_entity_reference)),
            &(cecs_relation_entity_reference){id},
            sizeof(cecs_relation_entity_reference)
        );
        *cecs_world_get_or_set_default_entity_flags(w, id) = (cecs_entity_flags){ .is_permanent = true };
    }

    void* component_source =
#if CECS_WORLD_UNIQUE_RELATION_COMPONENTS
        cecs_world_set_component(
            w,
            id,
            component_id,
            component,
            size
        );
#else
        component;
#endif

    return cecs_world_components_set_component_unchecked(
        &w->components,
        id,
        cecs_relation_id_create(cecs_relation_id_descriptor_create_tag(component_id, tag_id)),
        cecs_world_set_component(
            w,
            associated_id,
            component_id,
            component_source,
            size
        ),
        size,
        (cecs_component_storage_descriptor) {
        .capacity = 1,
            .indirect_component_id = CECS_OPTION_CREATE_SOME(cecs_indirect_component_id, component_id),
            .is_size_known = true,
    }
    );
}

void* cecs_world_get_component_relation(const cecs_world* w, cecs_entity_id id, cecs_component_id component_id, cecs_tag_id tag_id) {
    assert(cecs_world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    return cecs_world_get_component(w, id, cecs_relation_id_create(cecs_relation_id_descriptor_create_tag(component_id, tag_id)));
}

bool cecs_world_remove_component_relation(cecs_world* w, cecs_entity_id id, cecs_component_id component_id, void* out_removed_component, cecs_tag_id tag_id) {
    assert(cecs_world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    cecs_entity_id associated_id;
    if (!cecs_world_relations_remove_associated_id(&w->relations, id, (cecs_relation_target) { tag_id }, & associated_id)) {
        return false;
    }
    else if (!cecs_world_relations_entity_has_associated_id(&w->relations, id, (cecs_relation_target) { tag_id })) {
        *cecs_world_get_or_set_default_entity_flags(w, id) = (cecs_entity_flags){ .is_permanent = false };
        cecs_world_clear_entity(w, associated_id);
    }

    return cecs_world_remove_component(
        w,
        id,
        cecs_relation_id_create(cecs_relation_id_descriptor_create_tag(component_id, tag_id)),
        out_removed_component
    )
#if CECS_WORLD_UNIQUE_RELATION_COMPONENTS
        && cecs_world_remove_component(w, id, component_id, out_removed_component)
#endif
        && cecs_world_remove_component(
            w,
            associated_id,
            component_id,
            out_removed_component
        );
}

cecs_tag_id cecs_world_add_tag_relation(cecs_world* w, cecs_entity_id id, cecs_tag_id tag, cecs_tag_id target_tag_id) {
    assert(cecs_world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    cecs_entity_id extra_id = cecs_world_add_entity(w);
    cecs_entity_id associated_id = cecs_world_relations_increment_associated_id_or_set(
        &w->relations,
        id,
        (cecs_relation_target) {
        target_tag_id
    },
        extra_id
    );
    if (associated_id != extra_id) {
        cecs_world_entities_remove_entity(&w->entities, extra_id);
    }
    else {
        cecs_world_set_component(
            w,
            associated_id,
            CECS_TYPE_ID(CECS_COMPONENT(cecs_relation_entity_reference)),
            &(cecs_relation_entity_reference){id},
            sizeof(cecs_relation_entity_reference)
        );
        *cecs_world_get_or_set_default_entity_flags(w, id) = (cecs_entity_flags){ .is_permanent = true };
    }

    cecs_tag_id tag_source =
#if CECS_WORLD_UNIQUE_RELATION_COMPONENTS
        cecs_world_add_tag(
            w,
            id,
            tag
        );
#else
        tag;
#endif

    return cecs_world_add_tag(
        w,
        id,
        cecs_relation_id_create(cecs_relation_id_descriptor_create_tag(
            cecs_world_add_tag(
                w,
                associated_id,
                tag_source
            ),
            target_tag_id
        ))
    );
}

bool cecs_world_remove_tag_relation(cecs_world* w, cecs_entity_id id, cecs_tag_id tag, cecs_tag_id target_tag_id) {
    assert(cecs_world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    cecs_entity_id associated_id;
    if (!cecs_world_relations_remove_associated_id(&w->relations, id, (cecs_relation_target) { target_tag_id }, & associated_id)) {
        return false;
    }
    else if (!cecs_world_relations_entity_has_associated_id(&w->relations, id, (cecs_relation_target) { target_tag_id })) {
        *cecs_world_get_or_set_default_entity_flags(w, id) = (cecs_entity_flags){ .is_permanent = false };
        cecs_world_clear_entity(w, associated_id);
    }

    return cecs_world_remove_tag(
        w,
        id,
        cecs_relation_id_create(cecs_relation_id_descriptor_create_tag(tag, target_tag_id))
    )
#if CECS_WORLD_UNIQUE_RELATION_COMPONENTS
        && cecs_world_remove_tag(w, id, tag)
#endif
        && cecs_world_remove_tag(
            w,
            associated_id,
            tag
        );
}

cecs_associated_entity_ids_iterator cecs_world_get_associated_ids(cecs_world* w, cecs_entity_id id) {
    assert(cecs_world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    return cecs_world_relations_get_associated_ids(&w->relations, id);
}

cecs_resource_handle cecs_world_set_resource(cecs_world* w, cecs_resource_id id, void* resource, size_t size) {
    return cecs_world_resources_set_resource(&w->resources, id, resource, size);
}

cecs_resource_handle cecs_world_get_resource(const cecs_world* w, cecs_resource_id id) {
    return cecs_world_resources_get_resource(&w->resources, id);
}

bool cecs_world_remove_resource(cecs_world* w, cecs_resource_id id) {
    return cecs_world_resources_remove_resource(&w->resources, id);
}

bool cecs_world_remove_resource_out(cecs_world* w, cecs_resource_id id, cecs_resource_handle out_resource, size_t size) {
    return cecs_world_resources_remove_resource_out(&w->resources, id, out_resource, size);
}

cecs_entity_id cecs_world_remove_entity(cecs_world* w, cecs_entity_id entity_id) {
    assert(
        !cecs_world_get_entity_flags(w, entity_id).is_permanent
        && "entity with given ID is permanent and cannot be removed"
    );

    cecs_world_set_entity_flags(w, entity_id, cecs_entity_flags_default());
    cecs_world_clear_entity(w, entity_id);
    return cecs_world_entities_remove_entity(&w->entities, entity_id);
}

void cecs_world_free(cecs_world* w) {
    cecs_world_entities_free(&w->entities);
    cecs_world_components_free(&w->components);
    cecs_world_relations_free(&w->relations);
    cecs_world_resources_free(&w->resources);
    w->resources = (cecs_world_resources){ 0 };
    w->relations = (cecs_world_relations){ 0 };
    w->components = (cecs_world_components){ 0 };
    w->entities = (cecs_world_entities){ 0 };
}
