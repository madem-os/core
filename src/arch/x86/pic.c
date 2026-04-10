#include <stdint.h>

#include "arch/x86/pic.h"
#include "mos-ports.h"

#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1
#define PIC_EOI 0x20

void pic_init(void) {
    write_port_byte(PIC1_COMMAND, 0x11);
    io_wait();
    write_port_byte(PIC2_COMMAND, 0x11);
    io_wait();

    write_port_byte(PIC1_DATA, 0x20);
    io_wait();
    write_port_byte(PIC2_DATA, 0x28);
    io_wait();

    write_port_byte(PIC1_DATA, 0x04);
    io_wait();
    write_port_byte(PIC2_DATA, 0x02);
    io_wait();

    write_port_byte(PIC1_DATA, 0x01);
    io_wait();
    write_port_byte(PIC2_DATA, 0x01);
    io_wait();

    write_port_byte(PIC1_DATA, 0xFF);
    io_wait();
    write_port_byte(PIC2_DATA, 0xFF);
    io_wait();
}

void pic_set_mask(uint8_t irq) {
    uint16_t data_port;
    uint8_t mask;

    if (irq < 8) {
        data_port = PIC1_DATA;
    } else {
        data_port = PIC2_DATA;
        irq -= 8;
    }

    mask = read_port_byte(data_port);
    write_port_byte(data_port, mask | (uint8_t)(1u << irq));
}

void pic_clear_mask(uint8_t irq) {
    uint16_t data_port;
    uint8_t mask;

    if (irq < 8) {
        data_port = PIC1_DATA;
    } else {
        data_port = PIC2_DATA;
        irq -= 8;
    }

    mask = read_port_byte(data_port);
    write_port_byte(data_port, mask & (uint8_t)~(1u << irq));
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        write_port_byte(PIC2_COMMAND, PIC_EOI);
    }

    write_port_byte(PIC1_COMMAND, PIC_EOI);
}
