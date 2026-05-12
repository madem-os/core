#include <stdint.h>
#include <stddef.h>


uint8_t page_table[131072] = {0};

uint8_t modify_bit(uint8_t byte, uint8_t select) {
    if (select == 0) {
        select = 1;
    }
    else {
    select = select >> 1;
    }
    byte = byte + select;
    return byte;
}

uint8_t modify_pages_status8(uint8_t byte, uint8_t status, uint8_t amt) {
    if (status == 0) {
        for (int i = 0; i < amt; i++) {
            byte = byte >> 1;
        }
    }
    else if (status == 1) {
        amt--;
        byte = modify_bit(byte, amt);
    }
    return byte;
} 


uint32_t modify_page_tbl(uint32_t pointer, uint8_t status, uint32_t amt, uint8_t dir) {
    uint8_t byte;
    uint32_t inc;
    uint8_t amt8 = amt % 8;
    amt = amt / 8;
    if (dir == 1) inc = 0xffffffff;
    else if (dir == 0) inc = 1;
    for (int i = 0; i < amt; i++) {
        byte = page_table[pointer];
        byte = modify_pages_status8(byte, status, 8);
        page_table[pointer] = byte;
        pointer += inc;
    }
    byte = page_table[pointer];
    byte = modify_pages_status8(byte, status, amt8);
    page_table[pointer] = byte;
    pointer += inc;
    return pointer;
}