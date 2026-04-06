// kernel.c
#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;



uint8_t* vga = (uint8_t*)0x000b8000;
uint16_t character_ptr = 0;

static inline void outb(u16 port, u8 value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(u16 port) {
	uint8_t ret;
	__asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}



static inline void outw(u16 port, u16 value) {
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}



__attribute__((naked, used, section(".text.entry")))
void kernel_entry(void) {
    __asm__ volatile (
        "cli\n"
        "cld\n"
        "mov $0x10, %ax\n"
        "mov %ax, %ds\n"
        "mov %ax, %es\n"
        "mov %ax, %fs\n"
        "mov %ax, %gs\n"
        "mov %ax, %ss\n"
        "mov $0x00090000, %esp\n"
        "call kmain\n"
        "1:\n"
        "hlt\n"
        "jmp 1b\n"
    );
}

void print_char(uint8_t ch) {
	vga[character_ptr] = ch;
	vga[character_ptr + 1] = 0x0f;
	character_ptr += 2;
	move_cursor_forward();
 }
    void printf(char* str) {
        int str_pointer = 0;
     while(str[str_pointer] != 0) {
	         print_char(str[str_pointer]);
	         str_pointer ++;
	}
	
  }

#define VGA_INDEX 0x03d4
#define VGA_DATA 0X3D5

	
	
	void move_cursor_forward(void) {
    uint16_t cursor_pos;

    // Read current cursor position
    outb(VGA_INDEX, 0x0F);
    cursor_pos = inb(VGA_DATA);
    outb(VGA_INDEX, 0x0E);
    cursor_pos |= ((uint16_t)inb(VGA_DATA)) << 8;

    cursor_pos += 1;

    // Write new cursor position
    outb(VGA_INDEX, 0x0F);
    outb(VGA_DATA, (uint8_t)(cursor_pos & 0xFF));
    outb(VGA_INDEX, 0x0E);
    outb(VGA_DATA, (uint8_t)((cursor_pos >> 8) & 0xFF));
}

	






void kmain(void) {

	
	printf("hello world!");

		
    
    while(1) {
		
	}

}








