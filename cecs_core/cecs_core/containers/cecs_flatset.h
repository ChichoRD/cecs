#ifndef CECS_FLATSET_H
#define CECS_FLATSET_H

#include <stdint.h>

#ifndef CECS_FLATSET_HASH_TYPE
#define CECS_FLATSET_HASH_TYPE_DEFAULT size_t
#define CECS_FLATSET_HASH_TYPE CECS_FLATSET_HASH_TYPE_DEFAULT
#endif
typedef CECS_FLATSET_HASH_TYPE cecs_flatset_hash_type;

typedef struct cecs_flatbucket14_index_pair {
    uint8_t index0 : 4;
    uint8_t index1 : 4;
} cecs_flatbucket14_index_pair;
typedef union cecs_flatbucket14_index_pair_u {
    cecs_flatbucket14_index_pair pair;
    uint8_t value;
} cecs_flatbucket14_index_pair_u;

typedef struct cecs_flatbucket14 {
    uint8_t flags : 4;
    uint8_t count : 4;
    cecs_flatbucket14_index_pair_u hash_to_indices[7];
    uint8_t values[];
} cecs_flatbucket14;



#endif