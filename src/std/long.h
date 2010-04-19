#ifndef __LONG_H__
#define __LONG_H__

#include "vector.h"
#include "list.h"
#include "map.h"

/* typedefs to handle built-in types */
typedef long long_t;

VECTOR_MAKE_TYPE_INTERFACE(long)
LIST_MAKE_TYPE_INTERFACE(long)
HASHMAP_MAKE_TYPE_INTERFACE(long)

#endif // __LONG_H__
