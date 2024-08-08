#ifndef COMPONENT_H
#define COMPONENT_H

#include <stdint.h>
#include "type_id.h"

typedef uint32_t component_id;
static component_id component_id_count = 0;

#define COMPONENT_DEFINE(type) TYPE_ID_DEFINE_COUNTER(type##_component, component_id_count)
#define COMPONENT_ID(type) (component_id)TYPE_ID(type##_component)


// Source: https://www.reddit.com/r/C_Programming/comments/3dfu5w/comment/ct4remi/?utm_source=share&utm_medium=web3x&utm_name=web3xcss&utm_term=1&utm_content=share_button
#define _L0(F, x) F(x)
#define _L1(F, x, ...) F(x) _L0(F, __VA_ARGS__)
#define _L2(F, x, ...) F(x) _L1(F, __VA_ARGS__)
#define _L3(F, x, ...) F(x) _L2(F, __VA_ARGS__)
#define _L4(F, x, ...) F(x) _L3(F, __VA_ARGS__)
#define _L5(F, x, ...) F(x) _L4(F, __VA_ARGS__)
#define _L6(F, x, ...) F(x) _L5(F, __VA_ARGS__)
#define _L7(F, x, ...) F(x) _L6(F, __VA_ARGS__)
#define _L8(F, x, ...) F(x) _L7(F, __VA_ARGS__)
#define _L9(F, x, ...) F(x) _L8(F, __VA_ARGS__)
#define _L10(F, x, ...) F(x) _L9(F, __VA_ARGS__)
#define _L11(F, x, ...) F(x) _L10(F, __VA_ARGS__)
#define _L12(F, x, ...) F(x) _L11(F, __VA_ARGS__)
#define _L13(F, x, ...) F(x) _L12(F, __VA_ARGS__)
#define _L14(F, x, ...) F(x) _L13(F, __VA_ARGS__)
#define _L15(F, x, ...) F(x) _L14(F, __VA_ARGS__)

#define _GET_MACRO(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, NAME, ...) NAME
#define FOR_EACH(action, ...) _GET_MACRO(__VA_ARGS__, _L15, _L14, _L13, _L12, _L11, _L10, _L9, _L8, _L7, _L6, _L5, _L4, _L3, _L2, _L1, _L0)(action, __VA_ARGS__)


#define _COMPONENT_ID_COMMA(type) COMPONENT_ID(type),
#define COMPONENT_IDS(...) FOR_EACH(_COMPONENT_ID_COMMA, __VA_ARGS__)

#endif