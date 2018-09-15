#include "bytesread.h"

uint32_t read_2bytes_to_uint32(const uint8_t* data)
{
    union byte4uint32 u;
    u.b[3] = 0;
    u.b[2] = 0;
    u.b[1] = *data;
    u.b[0] = *(data+1);
    return u.i;
}

uint32_t read_3bytes_to_uint32(const uint8_t* data)
{
    union byte4uint32 u;
    u.b[3] = 0;
    u.b[2] = *data;
    u.b[1] = *(data+1);
    u.b[0] = *(data+2);
    return u.i;
}

uint32_t read_4bytes_to_uint32(const uint8_t* data)
{
    union byte4uint32 u;
    u.b[3] = *data;
    u.b[2] = *(data+1);
    u.b[1] = *(data+2);
    u.b[0] = *(data+3);
    return u.i;
}

uint64_t read_8bytes_to_uint64(const uint8_t* data)
{
    union byte8uint64 u;
    u.b[7] = *data;
    u.b[6] = *(data+1);
    u.b[5] = *(data+2);
    u.b[4] = *(data+3);
    u.b[3] = *(data+4);
    u.b[2] = *(data+5);
    u.b[1] = *(data+6);
    u.b[0] = *(data+7);

    return u.i;
}

double read_8bytes_to_double(const uint8_t* data)
{
    union byte8double u;
    u.b[7] = *data;
    u.b[6] = *(data+1);
    u.b[5] = *(data+2);
    u.b[4] = *(data+3);
    u.b[3] = *(data+4);
    u.b[2] = *(data+5);
    u.b[1] = *(data+6);
    u.b[0] = *(data+7);

    return u.d;
}
