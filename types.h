#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

#ifdef __GNUC__
#include <inttypes.h>
#endif

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
#ifdef __GNUC__
typedef uint64_t uint64;
#endif /* __GNUC__ */
#ifdef _MSC_VER
typedef unsigned __int64 uint64;
#endif /* _MSC_VER */

typedef char int8;
typedef short int16;
typedef int int32;
#ifdef __GNUC__
typedef int64_t int64;
#endif /* __GNUC__ */
#ifdef _MSC_VER
typedef __int64 int64;
#endif /* _MSC_VER */

#endif /* TYPES_H_INCLUDED */
