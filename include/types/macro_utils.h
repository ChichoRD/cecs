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
#define TAIL(first, ...) __VA_ARGS__

#define IS_PROBE(...) SECOND(__VA_ARGS__, 0)
#define PROBE() ~, 1

#define CAT2(x, ...) x ## __VA_ARGS__
#define CAT3(x, y, ...) x ## y ## __VA_ARGS__

#define _NOT_0 PROBE()
#define NOT(x) IS_PROBE(CAT2(_NOT_, x))
#define BOOL(x) NOT(NOT(x))

#define _AND_0(...) 0
#define _AND_1(...) __VA_ARGS__
#define _AND2(b0, b1) CAT2(_AND_, b0)(b1)
#define AND2(condition0, condition1) _AND2(BOOL(condition0), BOOL(condition1))

#define _OR_0(...) __VA_ARGS__
#define _OR_1(...) 1
#define _OR2(b0, b1) CAT2(_OR_, b0)(b1)
#define OR2(condition0, condition1) _OR2(BOOL(condition0), BOOL(condition1))

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

#define AND(condition0, condition1, ...) FOLD_L(AND2, condition0, condition1, __VA_ARGS__)
#define OR(condition0, condition1, ...) FOLD_L(OR2, condition0, condition1, __VA_ARGS__)

#define SWAP(a, b) (b, a)
#define COMPOSE2(f, g) DEFER1(f)g
#define COMPOSE(...) FOLD_L(COMPOSE2, __VA_ARGS__)
#define _FOLD_R_(f, initial, first, ...) \
    f(initial, FOLD_L(COMPOSE2(f, SWAP), first, __VA_ARGS__))
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

#define _FILTER_() _FILTER
#define _FILTER(f, separator, first, ...) \
    IF(f(first))( \
        first IF(HAS_ARGUMENTS(__VA_ARGS__))(separator()) \
    ) \
    IF(HAS_ARGUMENTS(__VA_ARGS__))( \
        DEFER2(_FILTER_)()(f, separator, __VA_ARGS__) \
    )
#define FILTER(f, separator, ...) \
    IF(HAS_ARGUMENTS(__VA_ARGS__))( \
        EVAL(_FILTER(f, separator, __VA_ARGS__)) \
    )

#define COMPARE_0(x) x
#define COMPARE_1(x) x
#define _COMPARE(x, y) \
    IS_PROBE( \
        CAT2(COMPARE_, x)(CAT2(COMPARE_, y))(PROBE()) \
    )
#define IS_COMPARABLE(x) IS_PROBE(CAT2(COMPARE_, x)(PROBE()))

#define NOT_EQUAL(x, y) \
    IF_ELSE(AND2(IS_COMPARABLE(x), IS_COMPARABLE(y)))( \
        _COMPARE(x, y), \
        1 \
    )
#define EQUAL(x, y) NOT(NOT_EQUAL(x, y))
#define IS_BOOL(x) OR2(EQUAL(x, 0), EQUAL(x, 1))

#define _MATCH_() _MATCH
#define _MATCH(obj, default, first, second, ...) \
    IF_ELSE(OR2(AND2(IS_BOOL(first), first), AND2(IS_BOOL(first(obj)), first(obj))))( \
        second, \
        IF_ELSE(HAS_ARGUMENTS(__VA_ARGS__))( \
            DEFER3(_MATCH_)()(obj, default, __VA_ARGS__), \
            default \
        ) \
    ) 

#define MATCH(obj, default, ...) \
    IF_ELSE(HAS_ARGUMENTS(__VA_ARGS__))( \
        EVAL(_MATCH(obj, default, __VA_ARGS__)), \
        default \
    )

#define COMPARE_int(x) x
#define COMPARE_char(x) x
#define COMPARE_float(x) x
#define COMPARE_double(x) x
#define SIZE_OF_PRIMITIVE(type) \
    MATCH( \
        type, \
        -1, \
        \
        EQUAL(type, char), \
        1, \
        \
        EQUAL(type, int), \
        4, \
        \
        EQUAL(type, float), \
        4, \
        \
        EQUAL(type, double), \
        8, \
        \
    )

#define REP0(separator, ...)
#define REP1(separator, ...) __VA_ARGS__ separator()
#define REP2(separator, ...) REP1(separator, __VA_ARGS__) __VA_ARGS__ separator()
#define REP3(separator, ...) REP2(separator, __VA_ARGS__) __VA_ARGS__ separator()
#define REP4(separator, ...) REP3(separator, __VA_ARGS__) __VA_ARGS__ separator()
#define REP5(separator, ...) REP4(separator, __VA_ARGS__) __VA_ARGS__ separator()
#define REP6(separator, ...) REP5(separator, __VA_ARGS__) __VA_ARGS__ separator()
#define REP7(separator, ...) REP6(separator, __VA_ARGS__) __VA_ARGS__ separator()
#define REP8(separator, ...) REP7(separator, __VA_ARGS__) __VA_ARGS__ separator()
#define REP9(separator, ...) REP8(separator, __VA_ARGS__) __VA_ARGS__ separator()
#define REP10(separator, ...) REP9(separator, __VA_ARGS__) __VA_ARGS__

#define REMOVE_TRAILING_COMMAS(...) MAP(PASS, COMMA, __VA_ARGS__)
#define REP(separator, c, d, u, ...) \
    CAT2(REP, c)(separator, REP10(separator, REP10(separator, __VA_ARGS__))) \
    CAT2(REP, d)(separator, REP10(separator, __VA_ARGS__)) \
    CAT2(REP, u)(separator, __VA_ARGS__)

#define __DO_() _DO_
#define _DO_(separator, f, first, ...) \
    f \
    IF(HAS_ARGUMENTS(__VA_ARGS__))( \
        separator() DEFER2(__DO_)()(separator, f, __VA_ARGS__) \
    )

#define _DO(separator, f, ...) \
    IF(HAS_ARGUMENTS(__VA_ARGS__))( \
        EVAL(_DO_(separator, f, __VA_ARGS__)) \
    )
#define DO(separator, f, ...) _DO(separator, f, REP(COMMA, __VA_ARGS__, 1))


#endif