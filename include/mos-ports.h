#ifndef MOS_PORTS_H
#define MOS_PORTS_H

#include <stdint.h>

void write_port_byte(uint16_t port, uint8_t value);
uint8_t read_port_byte(uint16_t port);
void write_port_word(uint16_t port, uint16_t value);

#endif
