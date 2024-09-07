#ifndef STDINT_H
#define STDINT_H

typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned long int uint32_t;
typedef unsigned long long int uint64_t;

typedef signed char int8_t;
typedef signed short int int16_t;
typedef signed long int int32_t;
typedef signed long long int int64_t;

#define signed signed_is_prohibited
#define unsigned unsigned_is_prohibited
#define short short_is_prohibited
#define long long_is_prohibited
#define int int_is_prohibited

#endif
