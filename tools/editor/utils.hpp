#ifndef UTILS_HPP
#define UTILS_HPP

#include "jansson.h"

#define BIG2LE_16(x) (((x) & 0x00FF) << 8) | (((x) >> 8) & 0x00FF)
#define BIG2LE_32(x) (BIG2LE_16((x) & 0x0000FFFF) << 16) |                     \
        BIG2LE_16(((x) >> 16) & 0x0000FFFF)

#endif /* !UTILS_HPP */
