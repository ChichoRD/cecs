#ifndef CECS_MACRO_UTILS_H
#define CECS_MACRO_UTILS_H

#define CECS_EVAL1(...) __VA_ARGS__
#define CECS_EVAL2(...) CECS_EVAL1(CECS_EVAL1(__VA_ARGS__))
#define CECS_EVAL4(...) CECS_EVAL2(CECS_EVAL2(__VA_ARGS__))
#define CECS_EVAL8(...) CECS_EVAL4(CECS_EVAL4(__VA_ARGS__))
#define CECS_EVAL16(...) CECS_EVAL8(CECS_EVAL8(__VA_ARGS__))
#define CECS_EVAL32(...) CECS_EVAL16(CECS_EVAL16(__VA_ARGS__))
#define CECS_EVAL64(...) CECS_EVAL32(CECS_EVAL32(__VA_ARGS__))
#define CECS_EVAL128(...) CECS_EVAL64(CECS_EVAL64(__VA_ARGS__))
#define CECS_EVAL256(...) CECS_EVAL128(CECS_EVAL128(__VA_ARGS__))
#define CECS_EVAL512(...) CECS_EVAL256(CECS_EVAL256(__VA_ARGS__))
#define CECS_EVAL1024(...) CECS_EVAL512(CECS_EVAL512(__VA_ARGS__))
#define CECS_EVAL(...) CECS_EVAL1024(__VA_ARGS__)

#define CECS_FIRST(first, ...) first
#define CECS_SECOND(first, second, ...) second
#define CECS_TAIL(first, ...) __VA_ARGS__

#define CECS_IS_PROBE(...) CECS_SECOND(__VA_ARGS__, 0)
#define CECS_PROBE() ~, 1

#define CECS_CAT2(x, ...) x ## __VA_ARGS__
#define CECS_CAT3(x, y, ...) x ## y ## __VA_ARGS__

#define CECS_NOT_0 CECS_PROBE()
#define CECS_NOT(x) CECS_IS_PROBE(CECS_CAT2(CECS_NOT_, x))
#define CECS_BOOL(x) CECS_NOT(CECS_NOT(x))

#define CECS_AND_0(...) 0
#define CECS_AND_1(...) __VA_ARGS__
#define _CECS_AND2(b0, b1) CECS_CAT2(CECS_AND_, b0)(b1)
#define CECS_AND2(condition0, condition1) _CECS_AND2(CECS_BOOL(condition0), CECS_BOOL(condition1))

#define CECS_OR_0(...) __VA_ARGS__
#define CECS_OR_1(...) 1
#define C_ECS_OR2(b0, b1) CECS_CAT2(CECS_OR_, b0)(b1)
#define CECS_OR2(condition0, condition1) _CECS_OR2(CECS_BOOL(condition0), CECS_BOOL(condition1))

#define CECS_IF_0(...)
#define CECS_IF_1(...) __VA_ARGS__
#define _CECS_IF(bool) CECS_CAT2(CECS_IF_, bool)
#define CECS_IF(condition) _CECS_IF(CECS_BOOL(condition))

#define CECS_IF_0_ELSE(t, f) f
#define CECS_IF_1_ELSE(t, f) t
#define _CECS_IF_ELSE(bool) CECS_CAT3(CECS_IF_, bool, _ELSE)
#define CECS_IF_ELSE(condition) _CECS_IF_ELSE(CECS_BOOL(condition))

#define CECS_PASS(...) __VA_ARGS__
#define CECS_EMPTY()
#define CECS_COMMA() ,
#define CECS_SEMICOLON() ;
#define CECS_PLUS() +
#define CECS_ZERO() 0
#define CECS_ONE() 1

#define CECS_DEFER1(f) f CECS_EMPTY()
#define CECS_DEFER2(f) f CECS_EMPTY CECS_EMPTY()()
#define CECS_DEFER3(f) f CECS_EMPTY CECS_EMPTY CECS_EMPTY()()()
#define CECS_DEFER4(f) f CECS_EMPTY CECS_EMPTY CECS_EMPTY CECS_EMPTY()()()()

#define CECS_END_OF_ARGUMENTS(...) CECS_BOOL(CECS_FIRST(__VA_ARGS__))
#define CECS_HAS_ARGUMENTS(...) CECS_BOOL(CECS_FIRST(CECS_END_OF_ARGUMENTS __VA_ARGS__)(0))

#define _CECS_MAP_() _CECS_MAP
#define _CECS_MAP(f, separator, first, ...) \
    f(first) \
    CECS_IF(CECS_HAS_ARGUMENTS(__VA_ARGS__))( \
        separator() CECS_DEFER2(_CECS_MAP_)()(f, separator, __VA_ARGS__) \
    )
#define CECS_MAP(f, separator, ...) \
    CECS_IF(CECS_HAS_ARGUMENTS(__VA_ARGS__))( \
        CECS_EVAL(_CECS_MAP(f, separator, __VA_ARGS__)) \
    )

#define _CECS_MAP_CONST1_() _CECS_MAP_CONST1
#define _CECS_MAP_CONST1(f, constant_arg, separator, first, ...) \
    f(constant_arg, first) \
    CECS_IF(CECS_HAS_ARGUMENTS(__VA_ARGS__))( \
        separator() CECS_DEFER2(_CECS_MAP_CONST1_)()(f, constant_arg, separator, __VA_ARGS__) \
    )
#define CECS_MAP_CONST1(f, constant_arg, separator, ...) \
    CECS_IF(CECS_HAS_ARGUMENTS(__VA_ARGS__))( \
        CECS_EVAL(_CECS_MAP_CONST1(f, constant_arg, separator, __VA_ARGS__)) \
    )

#define _CECS_MAP_CONST2_() _CECS_MAP_CONST2
#define _CECS_MAP_CONST2(f, constant_arg0, constant_arg1, separator, first, ...) \
    f(constant_arg0, constant_arg1, first) \
    CECS_IF(CECS_HAS_ARGUMENTS(__VA_ARGS__))( \
        separator() CECS_DEFER2(_CECS_MAP_CONST2_)()(f, constant_arg0, constant_arg1, separator, __VA_ARGS__) \
    )
#define CECS_MAP_CONST2(f, constant_arg0, constant_arg1, separator, ...) \
    CECS_IF(CECS_HAS_ARGUMENTS(__VA_ARGS__))( \
        CECS_EVAL(_CECS_MAP_CONST2(f, constant_arg0, constant_arg1, separator, __VA_ARGS__)) \
    )

#define _CECS_MAP_PAIRS_() _CECS_MAP_PAIRS
#define _CECS_MAP_PAIRS(f, separator, first, second, ...) \
    f(first, second) \
    CECS_IF(CECS_HAS_ARGUMENTS(__VA_ARGS__))( \
        separator() CECS_DEFER2(_CECS_MAP_PAIRS_)()(f, separator, __VA_ARGS__) \
    )
#define CECS_MAP_PAIRS(f, separator, ...) \
    CECS_IF(CECS_HAS_ARGUMENTS(__VA_ARGS__))( \
        CECS_EVAL(_CECS_MAP_PAIRS(f, separator, __VA_ARGS__)) \
    )

#define _CECS_FOLD_L_() _CECS_FOLD_L
#define _CECS_FOLD_L(f, initial, first, ...) \
    CECS_IF_ELSE(CECS_HAS_ARGUMENTS(__VA_ARGS__))( \
        CECS_DEFER2(_CECS_FOLD_L_)()(f, f(initial, first), __VA_ARGS__), \
        f(initial, first) \
    )
#define CECS_FOLD_L(f, ...) \
    CECS_IF(CECS_HAS_ARGUMENTS(__VA_ARGS__))( \
        CECS_EVAL(_CECS_FOLD_L(f, __VA_ARGS__)) \
    )
    
#define _CECS_FOLD_L1(f, initial, ...) \
    CECS_IF_ELSE(CECS_HAS_ARGUMENTS(__VA_ARGS__))( \
        CECS_FOLD_L(f, initial, __VA_ARGS__), \
        initial \
    )
#define CECS_FOLD_L1(f, ...) \
    CECS_IF(CECS_HAS_ARGUMENTS(__VA_ARGS__))( \
        _CECS_FOLD_L1(f, __VA_ARGS__) \
    )

#define _CECS_REVERSE_() _CECS_REVERSE
#define _CECS_REVERSE(separator, first, ...) \
    CECS_IF(CECS_HAS_ARGUMENTS(__VA_ARGS__))( \
        CECS_DEFER2(_CECS_REVERSE_)()(separator, __VA_ARGS__)separator() \
    ) \
    first 
#define CECS_REVERSE(separator, ...) \
    CECS_IF(CECS_HAS_ARGUMENTS(__VA_ARGS__))( \
        CECS_EVAL(_CECS_REVERSE(separator, __VA_ARGS__)) \
    )

#define CECS_AND(condition0, condition1, ...) CECS_FOLD_L(CECS_AND2, condition0, condition1, __VA_ARGS__)
#define CECS_OR(condition0, condition1, ...) CECS_FOLD_L(CECS_OR2, condition0, condition1, __VA_ARGS__)

#define CECS_SWAP(a, b) (b, a)
#define CECS_COMPOSE2(f, g) CECS_DEFER1(f)g
#define CECS_COMPOSE(...) CECS_FOLD_L(CECS_COMPOSE2, __VA_ARGS__)
#define _CECS_FOLD_R_(f, initial, first, ...) \
    f(initial, CECS_FOLD_L(CECS_COMPOSE2(f, CECS_SWAP), first, __VA_ARGS__))
#define _CECS_FOLD_R(f, initial, first, ...) \
    CECS_IF_ELSE(CECS_HAS_ARGUMENTS(__VA_ARGS__))( \
        _CECS_FOLD_R_(f, initial, CECS_REVERSE(CECS_COMMA, first, __VA_ARGS__)), \
        f(initial, first) \
    )
#define CECS_FOLD_R(f, ...) \
    CECS_IF(CECS_HAS_ARGUMENTS(__VA_ARGS__))( \
        _CECS_FOLD_R(f, __VA_ARGS__) \
    )

#define _CECS_FOLD_R1(f, initial, ...) \
    CECS_IF_ELSE(CECS_HAS_ARGUMENTS(__VA_ARGS__))( \
        CECS_FOLD_R(f, initial, __VA_ARGS__), \
        initial \
    )
#define CECS_FOLD_R1(f, ...) \
    CECS_IF(CECS_HAS_ARGUMENTS(__VA_ARGS__))( \
        _CECS_FOLD_R1(f, __VA_ARGS__) \
    )

#define CECS_CAT(...) CECS_FOLD_L(CECS_CAT2, __VA_ARGS__)

#define _CECS_FILTER_() _CECS_FILTER
#define _CECS_FILTER(f, separator, first, ...) \
    CECS_IF(f(first))( \
        first CECS_IF(CECS_HAS_ARGUMENTS(__VA_ARGS__))(separator()) \
    ) \
    CECS_IF(CECS_HAS_ARGUMENTS(__VA_ARGS__))( \
        CECS_DEFER2(_CECS_FILTER_)()(f, separator, __VA_ARGS__) \
    )
#define CECS_FILTER(f, separator, ...) \
    CECS_IF(CECS_HAS_ARGUMENTS(__VA_ARGS__))( \
        CECS_EVAL(_CECS_FILTER(f, separator, __VA_ARGS__)) \
    )

#define CECS_COMPARE_0(x) x
#define CECS_COMPARE_1(x) x
#define CECS_COMPARE(x, y) \
    CECS_IS_PROBE( \
        CECS_CAT2(CECS_COMPARE_, x)(CECS_CAT2(CECS_COMPARE_, y))(CECS_PROBE()) \
    )
#define CECS_IS_COMPARABLE(x) CECS_IS_PROBE(CECS_CAT2(CECS_COMPARE_, x)(CECS_PROBE()))

#define CECS_NOT_EQUAL(x, y) \
    CECS_IF_ELSE(CECS_AND2(CECS_IS_COMPARABLE(x), CECS_IS_COMPARABLE(y)))( \
        CECS_COMPARE(x, y), \
        1 \
    )
#define CECS_EQUAL(x, y) CECS_NOT(CECS_NOT_EQUAL(x, y))
#define CECS_IS_BOOL(x) CECS_OR2(CECS_EQUAL(x, 0), CECS_EQUAL(x, 1))

#define _CECS_MATCH_() _CECS_MATCH
#define _CECS_MATCH(obj, default, first, second, ...) \
    CECS_IF_ELSE(CECS_OR2(CECS_AND2(CECS_IS_BOOL(first), first), CECS_AND2(CECS_IS_BOOL(first(obj)), first(obj))))( \
        second, \
        CECS_IF_ELSE(CECS_HAS_ARGUMENTS(__VA_ARGS__))( \
            CECS_DEFER3(_CECS_MATCH_)()(obj, default, __VA_ARGS__), \
            default \
        ) \
    ) 

#define CECS_MATCH(obj, default, ...) \
    CECS_IF_ELSE(CECS_HAS_ARGUMENTS(__VA_ARGS__))( \
        CECS_EVAL(_CECS_MATCH(obj, default, __VA_ARGS__)), \
        default \
    )

#define CECS_COMPARE_int(x) x
#define CECS_COMPARE_char(x) x
#define CECS_COMPARE_float(x) x
#define CECS_COMPARE_double(x) x
#define CECS_SIZE_OF_PRIMITIVE(type) \
    CECS_MATCH( \
        type, \
        -1, \
        \
        CECS_EQUAL(type, char), \
        1, \
        \
        CECS_EQUAL(type, int), \
        4, \
        \
        CECS_EQUAL(type, float), \
        4, \
        \
        CECS_EQUAL(type, double), \
        8, \
        \
    )

#define CECS_REP0(separator, ...)
#define CECS_REP1(separator, ...) __VA_ARGS__ separator()
#define CECS_REP2(separator, ...) CECS_REP1(separator, __VA_ARGS__) __VA_ARGS__ separator()
#define CECS_REP3(separator, ...) CECS_REP2(separator, __VA_ARGS__) __VA_ARGS__ separator()
#define CECS_REP4(separator, ...) CECS_REP3(separator, __VA_ARGS__) __VA_ARGS__ separator()
#define CECS_REP5(separator, ...) CECS_REP4(separator, __VA_ARGS__) __VA_ARGS__ separator()
#define CECS_REP6(separator, ...) CECS_REP5(separator, __VA_ARGS__) __VA_ARGS__ separator()
#define CECS_REP7(separator, ...) CECS_REP6(separator, __VA_ARGS__) __VA_ARGS__ separator()
#define CECS_REP8(separator, ...) CECS_REP7(separator, __VA_ARGS__) __VA_ARGS__ separator()
#define CECS_REP9(separator, ...) CECS_REP8(separator, __VA_ARGS__) __VA_ARGS__ separator()
#define CECS_REP10(separator, ...) CECS_REP9(separator, __VA_ARGS__) __VA_ARGS__

#define CECS_REMOVE_TRAILING_COMMAS(...) CECS_MAP(CECS_PASS, CECS_COMMA, __VA_ARGS__)
#define CECS_REP(separator, c, d, u, ...) \
    CECS_CAT2(CECS_REP, c)(separator, CECS_REP10(separator, CECS_REP10(separator, __VA_ARGS__))) \
    CECS_CAT2(CECS_REP, d)(separator, CECS_REP10(separator, __VA_ARGS__)) \
    CECS_CAT2(CECS_REP, u)(separator, __VA_ARGS__)

#define __CECS_DO_() _CECS_DO_
#define _CECS_DO_(separator, f, first, ...) \
    f \
    CECS_IF(CECS_HAS_ARGUMENTS(__VA_ARGS__))( \
        separator() CECS_DEFER2(__CECS_DO_)()(separator, f, __VA_ARGS__) \
    )

#define _CECS_DO(separator, f, ...) \
    CECS_IF(CECS_HAS_ARGUMENTS(__VA_ARGS__))( \
        CECS_EVAL(_CECS_DO_(separator, f, __VA_ARGS__)) \
    )
#define CECS_DO(separator, f, ...) _CECS_DO(separator, f, CECS_REP(CECS_COMMA, __VA_ARGS__, 1))

#endif