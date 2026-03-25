#ifndef _VTEX_H
#define _VTEX_H

struct region_type
{
    int16_t   minX;
    int16_t   minY;
    int16_t   maxX;
    int16_t   maxY;
    int16_t   hotX;
    int16_t   hotY;
};
typedef struct region_type  region_t;

struct vtex_type
{
    uint16_t  wide;
    uint16_t  high;
    uint32_t  size;
    uint32_t  offset;
    uint32_t  qty;
    region_t *region;
};
typedef struct vtex_type    vtex_t;

#endif
