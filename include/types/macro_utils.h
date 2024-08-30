#ifndef MACRO_UTILS_H
#define MACRO_UTILS_H

#define EVAL1(...) __VA_ARGS__
#define EVAL2(...) EVAL1(EVAL1(__VA_ARGS__))
#define EVAL4(...) EVAL2(EVAL2(__VA_ARGS__))
#define EVAL8(...) EVAL4(EVAL4(__VA_ARGS__))
#define EVAL16(...) EVAL8(EVAL8(__VA_ARGS__))
#define EVAL32(...) EVAL16(EVAL16(__VA_ARGS__))
#define EVAL64(...) EVAL32(EVAL32(__VA_ARGS__))
#define EVAL128(...) EVAL64(EVAL64(__VA_ARGS__))
#define EVAL256(...) EVAL128(EVAL128(__VA_ARGS__))
#define EVAL512(...) EVAL256(EVAL256(__VA_ARGS__))
#define EVAL1024(...) EVAL512(EVAL512(__VA_ARGS__))
#define EVAL(...) EVAL1024(__VA_ARGS__)

#define FIRST(first, ...) first
#define SECOND(first, second, ...) second

#define IS_PROBE(...) SECOND(__VA_ARGS__, 0)
#define PROBE() ~, 1

#define CAT2(x, ...) x ## __VA_ARGS__
#define CAT3(x, y, ...) x ## y ## __VA_ARGS__

#define _NOT_0 PROBE()
#define NOT(x) IS_PROBE(CAT2(_NOT_, x))
#define BOOL(x) NOT(NOT(x))

#define _IF_0(...)
#define _IF_1(...) __VA_ARGS__
#define _IF(bool) CAT2(_IF_, bool)
#define IF(condition) _IF(BOOL(condition))

#define _IF_0_ELSE(t, f) f
#define _IF_1_ELSE(t, f) t
#define _IF_ELSE(bool) CAT3(_IF_, bool, _ELSE)
#define IF_ELSE(condition) _IF_ELSE(BOOL(condition))

#define PASS(...) __VA_ARGS__
#define EMPTY()
#define COMMA() ,
#define SEMICOLON() ;
#define PLUS() +
#define ZERO() 0
#define ONE() 1

#define DEFER1(f) f EMPTY()
#define DEFER2(f) f EMPTY EMPTY()()
#define DEFER3(f) f EMPTY EMPTY EMPTY()()()
#define DEFER4(f) f EMPTY EMPTY EMPTY EMPTY()()()()

#define CAT2_DEFERRED(x, ...) DEFER1(CAT2)(x, __VA_ARGS__)
#define CAT3_DEFERRED(x, y, ...) DEFER1(CAT3)(x, y, __VA_ARGS__)

#define _END_OF_ARGUMENTS(...) BOOL(FIRST(__VA_ARGS__))
#define HAS_ARGUMENTS(...) BOOL(FIRST(_END_OF_ARGUMENTS __VA_ARGS__)(0))

#define _MAP_() _MAP
#define _MAP(f, separator, first, ...) \
    f(first) \
    IF(HAS_ARGUMENTS(__VA_ARGS__))( \
        separator() DEFER2(_MAP_)()(f, separator, __VA_ARGS__) \
    )
#define MAP(f, separator, ...) \
    IF(HAS_ARGUMENTS(__VA_ARGS__))( \
        EVAL(_MAP(f, separator, __VA_ARGS__)) \
    )

#define _MAP_PAIRS_() _MAP_PAIRS
#define _MAP_PAIRS(f, separator, first, second, ...) \
    f(first, second) \
    IF(HAS_ARGUMENTS(__VA_ARGS__))( \
        separator() DEFER2(_MAP_PAIRS_)()(f, separator, __VA_ARGS__) \
    )
#define MAP_PAIRS(f, separator, ...) \
    IF(HAS_ARGUMENTS(__VA_ARGS__))( \
        EVAL(_MAP_PAIRS(f, separator, __VA_ARGS__)) \
    )

#define _FOLD_L_() _FOLD_L
#define _FOLD_L(f, initial, first, ...) \
    IF_ELSE(HAS_ARGUMENTS(__VA_ARGS__))( \
        DEFER2(_FOLD_L_)()(f, f(initial, first), __VA_ARGS__), \
        f(initial, first) \
    )
#define FOLD_L(f, ...) \
    IF(HAS_ARGUMENTS(__VA_ARGS__))( \
        EVAL(_FOLD_L(f, __VA_ARGS__)) \
    )
    
#define _FOLD_L1(f, initial, ...) \
    IF_ELSE(HAS_ARGUMENTS(__VA_ARGS__))( \
        FOLD_L(f, initial, __VA_ARGS__), \
        initial \
    )
#define FOLD_L1(f, ...) \
    IF(HAS_ARGUMENTS(__VA_ARGS__))( \
        _FOLD_L1(f, __VA_ARGS__) \
    )

#define _REVERSE_() _REVERSE
#define _REVERSE(separator, first, ...) \
    IF(HAS_ARGUMENTS(__VA_ARGS__))( \
        DEFER2(_REVERSE_)()(separator, __VA_ARGS__)separator() \
    ) \
    first 
#define REVERSE(separator, ...) \
    IF(HAS_ARGUMENTS(__VA_ARGS__))( \
        EVAL(_REVERSE(separator, __VA_ARGS__)) \
    )

#define SWAP(a, b) (b, a)
#define COMPOSE(f, g) DEFER1(f)g
#define _FOLD_R_(f, initial, first, ...) \
    f(initial, FOLD_L(COMPOSE(f, SWAP), first, __VA_ARGS__))
#define _FOLD_R(f, initial, first, ...) \
    IF_ELSE(HAS_ARGUMENTS(__VA_ARGS__))( \
        _FOLD_R_(f, initial, REVERSE(COMMA, first, __VA_ARGS__)), \
        f(initial, first) \
    )
#define FOLD_R(f, ...) \
    IF(HAS_ARGUMENTS(__VA_ARGS__))( \
        _FOLD_R(f, __VA_ARGS__) \
    )

#define _FOLD_R1(f, initial, ...) \
    IF_ELSE(HAS_ARGUMENTS(__VA_ARGS__))( \
        FOLD_R(f, initial, __VA_ARGS__), \
        initial \
    )
#define FOLD_R1(f, ...) \
    IF(HAS_ARGUMENTS(__VA_ARGS__))( \
        _FOLD_R1(f, __VA_ARGS__) \
    )


#define CAT(...) FOLD_L(CAT2, __VA_ARGS__)

#endif