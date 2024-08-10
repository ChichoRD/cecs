#ifndef VIEW_H
#define VIEW_H

#include <assert.h>

#define VIEW(type) type##_view

#define VIEW_STRUCT(type, indirection) \
    struct VIEW(type) { \
        size_t count; \
        type indirection*elements; \
    }

#define VIEW_IMPLEMENT(type) \
    struct VIEW(type) type##_view_create(type *elements, size_t count) { \
        struct VIEW(type) view; \
        view.count = count; \
        view.elements = elements; \
        return view; \
    } \
    type *type##_view_get(struct VIEW(type) view, size_t index) { \
        assert(index < view.count); \
        return &view.elements[index]; \
    } \
    struct VIEW(type) type##_view_slice(struct VIEW(type) view, size_t start, size_t end) { \
        assert(end <= view.count); \
        return type##_view_create(view.elements + start, end - start); \
    }

#define VIEW_CREATE(type, elements, count) VIEW(type)_create(elements, count)
#define VIEW_GET(type, view, index) VIEW(type)_get(view, index)
#define VIEW_SLICE(type, view, start, end) VIEW(type)_slice(view, start, end)

#endif