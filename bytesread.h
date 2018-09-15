#include <stdint.h>
#include <stdio.h>
union byte4uint32 {
    uint8_t b[4];
    uint32_t i;
};

union byte8uint64 {
    uint8_t b[8];
    uint64_t i;
};

union byte8double {
    uint8_t b[8];
    double d;
};

uint32_t read_2bytes_to_uint32(const uint8_t* data);
uint32_t read_3bytes_to_uint32(const uint8_t* data);
uint32_t read_4bytes_to_uint32(const uint8_t* data);
uint64_t read_8bytes_to_uint64(const uint8_t* data);
double read_8bytes_to_double(const uint8_t* data);
