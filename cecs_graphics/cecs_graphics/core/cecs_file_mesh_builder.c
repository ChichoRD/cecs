#include <memory.h>
#include "cecs_file_mesh_builder.h"

cecs_file_mesh_builder_gltf cecs_file_mesh_builder_gltf_create(
    cecs_graphics_world *graphics_world,
    const cecs_mesh_builder_descriptor descriptor,
    cecs_arena *builder_arena,
    const char *path
) {
    const cgltf_options options = {
        .type = cgltf_file_type_invalid,
        .memory = (cgltf_memory_options) {
            .alloc_func = cecs_cgltf_alloc,
            .free_func = cecs_cgltf_free,
            .user_data = builder_arena,
        },
    };
    cgltf_data *data;
    const cgltf_result result = cgltf_parse_file(&options, path, &data);
    if (result != cgltf_result_success) {
        assert(false && "error: failed to parse or locate .gltf file");
        exit(EXIT_FAILURE);
    }
    
    const cgltf_result buffers_result = cgltf_load_buffers(&options, data, path);
    if (buffers_result != cgltf_result_success) {
        assert(false && "error: failed to load buffers");
        exit(EXIT_FAILURE);
    }

    assert(data->meshes_count > 0 && "error: no meshes found in .gltf file");
    cecs_mesh_builder *mesh_builders = cecs_arena_alloc(builder_arena, data->meshes_count * sizeof(cecs_mesh_builder));
    for (size_t i = 0; i < data->meshes_count; i++) {
        mesh_builders[i] = cecs_mesh_builder_create(graphics_world, descriptor, builder_arena);
    }

    return (cecs_file_mesh_builder_gltf){
        .mesh_builders = mesh_builders,
        .data = data,
    };
}

cecs_file_mesh_builder_gltf *cecs_file_mesh_builder_gltf_clear_builders(cecs_file_mesh_builder_gltf *builder) {
    for (size_t i = 0; i < builder->data->meshes_count; i++) {
        cecs_mesh_builder_clear(&builder->mesh_builders[i]);
    }
    return builder;
}

extern inline cecs_arena *cecs_file_mesh_builder_gltf_arena(const cecs_file_mesh_builder_gltf *builder);
cecs_file_mesh_builder_gltf *cecs_file_mesh_builder_gltf_clear_builders_and_data(cecs_file_mesh_builder_gltf *builder) {
    cecs_file_mesh_builder_gltf_clear_builders(builder);
    cecs_cgltf_free(cecs_file_mesh_builder_gltf_arena(builder), builder->data);
    builder->data = NULL;

    return builder;
}


extern inline size_t cecs_file_mesh_builder_gltf_mesh_count(const cecs_file_mesh_builder_gltf *builder);
static size_t cecs_file_mesh_builder_gltf_collect_vertex_accessors(
    cecs_arena *builder_arena,
    const cgltf_mesh *mesh,
    const cgltf_attribute_type attribute_type,
    cecs_dynamic_array *accessors
) {
    size_t total_attribute_count = 0;
    for (size_t i = 0; i < mesh->primitives_count; i++) {
        const cgltf_primitive *primitive = &mesh->primitives[i];
        for (size_t j = 0; j < primitive->attributes_count; j++) {
            const cgltf_attribute *attribute = &primitive->attributes[j];
            if (attribute->type == attribute_type) {
                const cgltf_accessor *accessor = attribute->data;
                assert(accessor->count > 0 && "error: no attributes in accessor");

                total_attribute_count += accessor->count;
                cecs_dynamic_array_add(accessors, builder_arena, &accessor, sizeof(cgltf_accessor *));
            }
        }
    }
    return total_attribute_count;
}

static void cecs_file_mesh_builder_gltf_unpack_attributes(
    const cgltf_accessor *accessor,
    cecs_arena *builder_arena,
    uint8_t *destination,
    const size_t attribute_count,
    const size_t attribute_size,
    const cecs_attribute_copy_options copy_options
) {
    const cgltf_size accessor_attribute_count = accessor->count;
    const cgltf_size accessor_attribute_size = cgltf_num_components(accessor->type) * cgltf_component_size(accessor->component_type);

    const cgltf_size accessor_float_count = accessor_attribute_count * cgltf_num_components(accessor->type);
    const size_t destination_float_count = attribute_count * attribute_size / sizeof(float);
    switch (copy_options) {
    case cecs_attribute_copy_expect_exact: {
        assert(accessor_attribute_size == attribute_size && "error: attribute size mismatch, with copy option expect exact");
        const cgltf_size unpacked_floats = cgltf_accessor_unpack_floats(accessor, destination, destination_float_count);
        assert(unpacked_floats == destination_float_count && "error: unpacked floats mismatch");
        break;
    }
    case cecs_attribute_copy_expect_larger_copy_padded: {
        assert(accessor_attribute_size >= attribute_size && "error: attribute size mismatch, with copy option expect larger");
        uint8_t *unpacked = cecs_arena_alloc(builder_arena, accessor_attribute_count * accessor_attribute_size);
        const cgltf_size unpacked_floats = cgltf_accessor_unpack_floats(accessor, unpacked, accessor_float_count);
        assert(unpacked_floats == accessor_float_count && "error: unpacked floats mismatch");

        for (size_t i = 0; i < attribute_count; i++) {
            memcpy(
                destination + i * attribute_size,
                unpacked + i * accessor_attribute_size,
                attribute_size
            );
        }
        break;
    }
    case cecs_attribute_copy_expect_larger_copy_start: {
        assert(accessor_attribute_size >= attribute_size && "error: attribute size mismatch, with copy option expect larger");
        const cgltf_size unpacked_floats = cgltf_accessor_unpack_floats(accessor, destination, destination_float_count);
        assert(unpacked_floats == destination_float_count && "error: unpacked floats mismatch");
        break;
    }
    case cecs_attribute_copy_expect_smaller_zero_fill_padded: {
        assert(accessor_attribute_size <= attribute_size && "error: attribute size mismatch, with copy option expect smaller");
        const cgltf_size unpacked_floats = cgltf_accessor_unpack_floats(accessor, destination, destination_float_count);
        assert(unpacked_floats == accessor_float_count && "error: unpacked floats mismatch");
        for (ptrdiff_t i = accessor_attribute_count - 1; i >= 0; i--) {
            memmove(
                destination + i * attribute_size,
                destination + i * accessor_attribute_size,
                accessor_attribute_size
            );
        }
        break;
    }
    case cecs_attribute_copy_expect_smaller_zero_fill: {
        assert(accessor_attribute_size <= attribute_size && "error: attribute size mismatch, with copy option expect smaller");
        // arena guarantees zeroed memory
        const cgltf_size unpacked_floats = cgltf_accessor_unpack_floats(accessor, destination, destination_float_count);
        assert(unpacked_floats == accessor_float_count && "error: unpacked floats mismatch");
        break;
    }
    default: {
        assert(false && "error: unknown copy option");
        exit(EXIT_FAILURE);
    }
    }
}

bool cecs_file_mesh_builder_gltf_set_vertex_attribute(
    cecs_file_mesh_builder_gltf *builder,
    const cgltf_attribute_type attribute_type,
    const cecs_vertex_attribute_id attribute_id,
    const size_t attribute_size,
    const size_t builder_index,
    const cecs_attribute_copy_options copy_options
) {
    assert(builder_index < cecs_file_mesh_builder_gltf_mesh_count(builder) && "error: builder index out of range");
    const cgltf_mesh *mesh = &builder->data->meshes[builder_index];
    
    assert(mesh->primitives_count > 0 && "error: no primitives in mesh");
    cecs_arena *builder_arena = cecs_file_mesh_builder_gltf_arena(builder);

    cecs_dynamic_array accessors = cecs_dynamic_array_create();
    size_t total_attribute_count = cecs_file_mesh_builder_gltf_collect_vertex_accessors(builder_arena, mesh, attribute_type, &accessors);
    if (total_attribute_count == 0) {
        return false;
    }

    uint8_t *attributes = cecs_arena_alloc(builder_arena, total_attribute_count * attribute_size);
    uint8_t *const attributes_start = attributes;

    const size_t accessor_count = cecs_dynamic_array_count_of_size(&accessors, sizeof(cgltf_accessor *));
    for (size_t i = 0; i < accessor_count; i++) {
        const cgltf_accessor *accessor = *(const cgltf_accessor **)cecs_dynamic_array_get(&accessors, i, sizeof(cgltf_accessor *));

        cecs_file_mesh_builder_gltf_unpack_attributes(
            accessor,
            builder_arena,
            attributes,
            accessor->count,
            attribute_size,
            copy_options
        );
        attributes += accessor->count * attribute_size;
    }

    cecs_mesh_builder_set_vertex_attribute(
        &builder->mesh_builders[builder_index],
        attribute_id,
        attributes_start,
        total_attribute_count,
        attribute_size
    );
    return true;
}

size_t cecs_file_mesh_builder_gltf_set_all_vertex_attributes(
    cecs_file_mesh_builder_gltf *builder,
    const cgltf_attribute_type attribute_type,
    const cecs_vertex_index_id attribute_id,
    const size_t attribute_size,
    const cecs_attribute_copy_options copy_options
) {
    size_t set_count = 0;
    for (size_t i = 0; i < cecs_file_mesh_builder_gltf_mesh_count(builder); i++) {
        if (cecs_file_mesh_builder_gltf_set_vertex_attribute(builder, attribute_type, attribute_id, attribute_size, i, copy_options)) {
            set_count++;
        }
    }
    return set_count;
}

bool cecs_file_mesh_builder_gltf_clear_vertex_attribute(
    cecs_file_mesh_builder_gltf *builder,
    const cecs_vertex_attribute_id attribute_id,
    const size_t builder_index
) {
    assert(builder_index < cecs_file_mesh_builder_gltf_mesh_count(builder) && "error: builder index out of range");
    cecs_mesh_builder_clear_vertex_attribute(&builder->mesh_builders[builder_index], attribute_id);
    
    // TODO: return bool from clear_vertex_attribute
    return true;
}

static size_t cecs_file_mesh_builder_gltf_collect_index_accessors(
    cecs_arena *builder_arena,
    const cgltf_mesh *mesh,
    cecs_dynamic_array *accessors
) {
    size_t total_index_count = 0;
    for (size_t i = 0; i < mesh->primitives_count; i++) {
        const cgltf_primitive *primitive = &mesh->primitives[i];
        const cgltf_accessor *index_accessor = primitive->indices;
        if (index_accessor != NULL) {
            assert(index_accessor->count > 0 && "error: no indices in accessor");
            total_index_count += index_accessor->count;
            cecs_dynamic_array_add(accessors, builder_arena, &index_accessor, sizeof(cgltf_accessor *));
        }
    }
    return total_index_count;
}

static void cecs_file_mesh_builder_gltf_unpack_indices(
    const cgltf_accessor *accessor,
    cecs_arena *builder_arena,
    uint8_t *destination,
    const size_t index_count,
    const size_t index_size,
    const cecs_attribute_copy_options copy_options
) {
    assert(cgltf_num_components(accessor->type) == 1 && "error: index accessor component count mismatch");
    
    const cgltf_size accessor_index_count = accessor->count;
    const cgltf_size accessor_index_size = cgltf_component_size(accessor->component_type);

    switch (copy_options) {
    case cecs_attribute_copy_expect_exact: {
        assert(accessor_index_size == index_size && "error: index size mismatch, with copy option expect exact");
        const cgltf_size unpacked_indices = cgltf_accessor_unpack_indices(accessor, destination, index_size, index_count);
        assert(unpacked_indices == index_count && "error: unpacked indeices mismatch");
        break;
    }
    case cecs_attribute_copy_expect_larger_copy_padded: {
        assert(accessor_index_size >= index_size && "error: index size mismatch, with copy option expect larger");
        uint8_t *unpacked = cecs_arena_alloc(builder_arena, accessor_index_count * accessor_index_size);
        const cgltf_size unpacked_indices = cgltf_accessor_unpack_indices(accessor, unpacked, accessor_index_size, accessor_index_count);
        assert(unpacked_indices == accessor_index_count && "error: unpacked indices mismatch");

        for (size_t i = 0; i < index_count; i++) {
            memcpy(
                destination + i * index_size,
                unpacked + i * accessor_index_size,
                index_size
            );
        }
        break;
    }
    case cecs_attribute_copy_expect_larger_copy_start: {
        assert(accessor_index_size >= index_size && "error: index size mismatch, with copy option expect larger");
        const cgltf_size index_unpack_count = index_count * index_size / accessor_index_size;
        const cgltf_size unpacked_indices = cgltf_accessor_unpack_indices(accessor, destination, accessor_index_size, index_unpack_count);
        assert(unpacked_indices == index_unpack_count && "error: unpacked indices mismatch");
        break;
    }
    case cecs_attribute_copy_expect_smaller_zero_fill_padded: {
        assert(accessor_index_size <= index_size && "error: index size mismatch, with copy option expect smaller");
        const cgltf_size unpacked_indices = cgltf_accessor_unpack_indices(accessor, destination, index_size, index_count);
        assert(unpacked_indices == index_count && "error: unpacked indices mismatch");
        break;
    }
    case cecs_attribute_copy_expect_smaller_zero_fill: {
        assert(false && "error: copy option not supported for indices, use expect smaller zero fill padded");
        exit(EXIT_FAILURE);
        break;
    }
    default: {
        assert(false && "error: unknown copy option");
        exit(EXIT_FAILURE);
    }
    }
}

bool cecs_file_mesh_builder_gltf_set_indices(
    cecs_file_mesh_builder_gltf *builder,
    const size_t builder_index,
    const cecs_attribute_copy_options copy_options
) {
    assert(builder_index < cecs_file_mesh_builder_gltf_mesh_count(builder) && "error: builder index out of range");
    const cgltf_mesh *mesh = &builder->data->meshes[builder_index];
    
    assert(mesh->primitives_count > 0 && "error: no primitives in mesh");
    cecs_arena *builder_arena = cecs_file_mesh_builder_gltf_arena(builder);

    cecs_dynamic_array accessors = cecs_dynamic_array_create();
    const size_t index_count = cecs_file_mesh_builder_gltf_collect_index_accessors(builder_arena, mesh, &accessors);
    if (index_count == 0) {
        return false;
    }

    const size_t index_size = cecs_index_format_info_from(builder->mesh_builders[builder_index].descriptor.index_format).size;
    uint8_t *indices = cecs_arena_alloc(builder_arena, index_count * index_size);
    uint8_t *const indices_start = indices;

    const size_t accessor_count = cecs_dynamic_array_count_of_size(&accessors, sizeof(cgltf_accessor *));
    for (size_t i = 0; i < accessor_count; i++) {
        const cgltf_accessor *accessor = *(const cgltf_accessor **)cecs_dynamic_array_get(&accessors, i, sizeof(cgltf_accessor *));

        cecs_file_mesh_builder_gltf_unpack_indices(
            accessor,
            builder_arena,
            indices,
            accessor->count,
            index_size,
            copy_options
        );
        indices += accessor->count * index_size;
    }

    cecs_mesh_builder_set_indices(
        &builder->mesh_builders[builder_index],
        indices_start,
        index_count
    );
    return true;
}

size_t cecs_file_mesh_builder_gltf_set_all_indices(cecs_file_mesh_builder_gltf *builder, const cecs_attribute_copy_options copy_options) {
    size_t set_count = 0;
    for (size_t i = 0; i < cecs_file_mesh_builder_gltf_mesh_count(builder); i++) {
        if (cecs_file_mesh_builder_gltf_set_indices(builder, i, copy_options)) {
            set_count++;
        }
    }
    return set_count;
}

bool cecs_file_mesh_builder_gltf_clear_indices(cecs_file_mesh_builder_gltf *builder, const size_t builder_index) {
    assert(builder_index < cecs_file_mesh_builder_gltf_mesh_count(builder) && "error: builder index out of range");
    cecs_mesh_builder_clear_indices(&builder->mesh_builders[builder_index]);
    return true;
}
