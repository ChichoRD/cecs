#include "world.h"

world world_create(size_t entity_capacity, size_t component_type_capacity, size_t resource_capacity) {
    world w;
    w.entities = world_entities_create(entity_capacity);
    w.components = world_components_create(component_type_capacity);
    w.relations = world_relations_create(entity_capacity);

    const size_t resource_default_size = sizeof(intptr_t) * 4;
    w.resources = world_resources_create(resource_capacity, resource_default_size);
    return w;
}

void* world_get_component(const world* w, entity_id entity_id, component_id component_id) {
    return world_components_get_component_unchecked(&w->components, entity_id, component_id);
}

bool world_try_get_component(const world* w, entity_id entity_id, component_id component_id, void** out_component) {
    optional_component component = world_components_get_component(&w->components, entity_id, component_id);
    if (CECS_OPTION_IS_SOME(optional_component, component)) {
        *out_component = CECS_OPTION_GET(optional_component, component);
        return true;
    }
    else {
        *out_component = NULL;
        return false;
    }
}

entity_flags world_get_entity_flags(const world* w, entity_id entity_id) {
    assert(world_enities_has_entity(&w->entities, entity_id) && "entity with given ID does not exist");
    entity_flags* flags = NULL;
    if (WORLD_TRY_GET_COMPONENT(entity_flags, w, entity_id, &flags)) {
        return *flags;
    }
    else {
        return entity_flags_default();
    }
}

void* world_set_component(world* w, entity_id id, component_id component_id, void* component, size_t size) {
    assert(world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    assert(
        !world_get_entity_flags(w, id).is_inmutable
        && "entity with given ID is inmutable and its components may not be set"
    );

    return world_components_set_component_unchecked(
        &w->components,
        id,
        component_id,
        component,
        size,
        (component_storage_descriptor) {
        .capacity = 1,
            .is_size_known = true,
            .indirect_component_id = { 0 }
    }
    );
}

const bool world_remove_component(world* w, entity_id id, component_id component_id, void* out_removed_component) {
    assert(world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    assert(
        !world_get_entity_flags(w, id).is_inmutable
        && "entity with given ID is inmutable and components may not be removed from it"
    );

    return world_components_remove_component(&w->components, id, component_id, out_removed_component);
}

entity_flags* world_get_or_set_default_entity_flags(world* w, entity_id id) {
    assert(world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    entity_flags* flags = NULL;
    if (WORLD_TRY_GET_COMPONENT(entity_flags, w, id, &flags)) {
        return flags;
    }
    else {
        return world_set_entity_flags(w, id, entity_flags_default());
    }
}

bool world_has_tag(const world* w, entity_id id, tag_id tag_id) {
    if (!world_enities_has_entity(&w->entities, id))
        return false;

    return world_components_has_component(&w->components, id, tag_id);
}

tag_id world_add_tag(world* w, entity_id id, tag_id tag_id) {
    assert(world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    assert(
        !world_get_entity_flags(w, id).is_inmutable
        && "entity with given ID is inmutable and tags may not be added to it"
    );

    world_components_set_component(
        &w->components,
        id,
        tag_id,
        NULL,
        0,
        (component_storage_descriptor) {
        .capacity = 1,
            .is_size_known = true,
            .indirect_component_id = { 0 }
    }
    );
    return tag_id;
}

tag_id world_remove_tag(world* w, entity_id id, tag_id tag_id) {
    assert(world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    assert(
        !world_get_entity_flags(w, id).is_inmutable
        && "entity with given ID is inmutable and tags may not be removed from it"
    );

    world_components_remove_component(&w->components, id, tag_id, &(optional_component){0});
    return tag_id;
}

entity_id world_add_entity(world* w) {
    entity_id e = world_entities_add_entity(&w->entities);
#if WORLD_FLAG_ALL_ENTITIES
    world_set_entity_flags(w, e, entity_flags_default());
#endif
    return e;
}

entity_id world_clear_entity(world* w, entity_id entity_id) {
    assert(
        !world_get_entity_flags(w, entity_id).is_inmutable
        && "entity with given ID is inmutable and cannot be cleared"
    );

    for (
        world_components_entity_iterator it = world_components_entity_iterator_create(&w->components, entity_id);
        !world_components_entity_iterator_done(&it);
        world_components_entity_iterator_next(&it)
        ) {
        sized_component_storage storage = world_components_entity_iterator_current(&it);
        component_storage_remove(
            storage.storage,
            &w->components.components_arena,
            entity_id,
            w->components.discard.handle,
            storage.component_size
        );
    }
    return entity_id;
}

entity_id world_copy_entity_onto(world* w, entity_id destination, entity_id source) {
    assert(
        !world_get_entity_flags(w, destination).is_inmutable
        && "entity with given ID is inmutable and components may not be copied onto it"
    );

    for (
        world_components_entity_iterator it = world_components_entity_iterator_create(&w->components, source);
        !world_components_entity_iterator_done(&it);
        world_components_entity_iterator_next(&it)
        ) {
        sized_component_storage storage = world_components_entity_iterator_current(&it);
        component_storage_set(
            storage.storage,
            &w->components.components_arena,
            destination,
            component_storage_info(storage.storage).is_unit_type_storage
            ? NULL
            : CECS_OPTION_GET(optional_component, component_storage_get(
                storage.storage,
                source,
                storage.component_size
            )),
            storage.component_size
        );
    }

    return destination;
}

entity_id world_copy_entity(world* w, entity_id destination, entity_id source) {
    world_clear_entity(w, destination);
    return world_copy_entity_onto(w, destination, source);
}

entity_id world_add_entity_copy(world* w, entity_id source) {
    return world_copy_entity_onto(w, world_add_entity(w), source);
}

void* world_copy_entity_onto_and_grab(world* w, entity_id destination, entity_id source, component_id grab_component_id) {
    assert(
        !world_get_entity_flags(w, destination).is_inmutable
        && "entity with given ID is inmutable and components may not be copied onto it"
    );

    void* grabbed = NULL;
    for (
        world_components_entity_iterator it = world_components_entity_iterator_create(&w->components, source);
        !world_components_entity_iterator_done(&it);
        world_components_entity_iterator_next(&it)
        ) {
        sized_component_storage storage = world_components_entity_iterator_current(&it);
        optional_component copied_component = component_storage_set(
            storage.storage,
            &w->components.components_arena,
            destination,
            component_storage_info(storage.storage).is_unit_type_storage
            ? NULL
            : CECS_OPTION_GET(optional_component, component_storage_get(
                storage.storage,
                source,
                storage.component_size
            )),
            storage.component_size
        );

        if (CECS_OPTION_IS_SOME(optional_component, copied_component) && grab_component_id == storage.component_id) {
            grabbed = CECS_OPTION_GET(optional_component, copied_component);
        }
    }

    return grabbed;
}

void* world_copy_entity_and_grab(world* w, entity_id destination, entity_id source, component_id grab_component_id) {
    world_clear_entity(w, destination);
    return world_copy_entity_onto_and_grab(w, destination, source, grab_component_id);
}

void* world_set_component_relation(world* w, entity_id id, component_id component_id, void* component, size_t size, tag_id tag_id) {
    assert(world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    entity_id extra_id = world_add_entity(w);
    entity_id associated_id = world_relations_increment_associated_id_or_set(
        &w->relations,
        id,
        (relation_target) {
        tag_id
    },
        extra_id
    );
    if (associated_id != extra_id) {
        world_entities_remove_entity(&w->entities, extra_id);
    }
    else {
        world_set_component(
            w,
            associated_id,
            TYPE_ID(COMPONENT(relation_entity_reference)),
            &(relation_entity_reference){id},
            sizeof(relation_entity_reference)
        );
        *world_get_or_set_default_entity_flags(w, id) = (entity_flags){ .is_permanent = true };
    }

    void* component_source =
#if WORLD_UNIQUE_RELATION_COMPONENTS
        world_set_component(
            w,
            id,
            component_id,
            component,
            size
        );
#else
        component;
#endif

    return world_components_set_component_unchecked(
        &w->components,
        id,
        relation_id_create(relation_id_descriptor_create_tag(component_id, tag_id)),
        world_set_component(
            w,
            associated_id,
            component_id,
            component_source,
            size
        ),
        size,
        (component_storage_descriptor) {
        .capacity = 1,
            .indirect_component_id = CECS_OPTION_CREATE_SOME(indirect_component_id, component_id),
            .is_size_known = true,
    }
    );
}

void* world_get_component_relation(const world* w, entity_id id, component_id component_id, tag_id tag_id) {
    assert(world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    return world_get_component(w, id, relation_id_create(relation_id_descriptor_create_tag(component_id, tag_id)));
}

bool world_remove_component_relation(world* w, entity_id id, component_id component_id, void* out_removed_component, tag_id tag_id) {
    assert(world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    entity_id associated_id;
    if (!world_relations_remove_associated_id(&w->relations, id, (relation_target) { tag_id }, & associated_id)) {
        return false;
    }
    else if (!world_relations_entity_has_associated_id(&w->relations, id, (relation_target) { tag_id })) {
        *world_get_or_set_default_entity_flags(w, id) = (entity_flags){ .is_permanent = false };
        world_clear_entity(w, associated_id);
    }

    return world_remove_component(
        w,
        id,
        relation_id_create(relation_id_descriptor_create_tag(component_id, tag_id)),
        out_removed_component
    )
#if WORLD_UNIQUE_RELATION_COMPONENTS
        && world_remove_component(w, id, component_id, out_removed_component)
#endif
        && world_remove_component(
            w,
            associated_id,
            component_id,
            out_removed_component
        );
}

tag_id world_add_tag_relation(world* w, entity_id id, tag_id tag, tag_id target_tag_id) {
    assert(world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    entity_id extra_id = world_add_entity(w);
    entity_id associated_id = world_relations_increment_associated_id_or_set(
        &w->relations,
        id,
        (relation_target) {
        target_tag_id
    },
        extra_id
    );
    if (associated_id != extra_id) {
        world_entities_remove_entity(&w->entities, extra_id);
    }
    else {
        world_set_component(
            w,
            associated_id,
            TYPE_ID(COMPONENT(relation_entity_reference)),
            &(relation_entity_reference){id},
            sizeof(relation_entity_reference)
        );
        *world_get_or_set_default_entity_flags(w, id) = (entity_flags){ .is_permanent = true };
    }

    tag_id tag_source =
#if WORLD_UNIQUE_RELATION_COMPONENTS
        world_add_tag(
            w,
            id,
            tag
        );
#else
        tag;
#endif

    return world_add_tag(
        w,
        id,
        relation_id_create(relation_id_descriptor_create_tag(
            world_add_tag(
                w,
                associated_id,
                tag_source
            ),
            target_tag_id
        ))
    );
}

bool world_remove_tag_relation(world* w, entity_id id, tag_id tag, tag_id target_tag_id) {
    assert(world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    entity_id associated_id;
    if (!world_relations_remove_associated_id(&w->relations, id, (relation_target) { target_tag_id }, & associated_id)) {
        return false;
    }
    else if (!world_relations_entity_has_associated_id(&w->relations, id, (relation_target) { target_tag_id })) {
        *world_get_or_set_default_entity_flags(w, id) = (entity_flags){ .is_permanent = false };
        world_clear_entity(w, associated_id);
    }

    return world_remove_tag(
        w,
        id,
        relation_id_create(relation_id_descriptor_create_tag(tag, target_tag_id))
    )
#if WORLD_UNIQUE_RELATION_COMPONENTS
        && world_remove_tag(w, id, tag)
#endif
        && world_remove_tag(
            w,
            associated_id,
            tag
        );
}

associated_entity_ids_iterator world_get_associated_ids(world* w, entity_id id) {
    assert(world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    return world_relations_get_associated_ids(&w->relations, id);
}

resource_handle world_set_resource(world* w, resource_id id, void* resource, size_t size) {
    return world_resources_set_resource(&w->resources, id, resource, size);
}

resource_handle world_get_resource(const world* w, resource_id id) {
    return world_resources_get_resource(&w->resources, id);
}

bool world_remove_resource(world* w, resource_id id) {
    return world_resources_remove_resource(&w->resources, id);
}

bool world_remove_resource_out(world* w, resource_id id, resource_handle out_resource, size_t size) {
    return world_resources_remove_resource_out(&w->resources, id, out_resource, size);
}

entity_id world_remove_entity(world* w, entity_id entity_id) {
    assert(
        !world_get_entity_flags(w, entity_id).is_permanent
        && "entity with given ID is permanent and cannot be removed"
    );

    world_set_entity_flags(w, entity_id, entity_flags_default());
    world_clear_entity(w, entity_id);
    return world_entities_remove_entity(&w->entities, entity_id);
}

void world_free(world* w) {
    world_entities_free(&w->entities);
    world_components_free(&w->components);
    world_relations_free(&w->relations);
    world_resources_free(&w->resources);
    w->resources = (world_resources){ 0 };
    w->relations = (world_relations){ 0 };
    w->components = (world_components){ 0 };
    w->entities = (world_entities){ 0 };
}
