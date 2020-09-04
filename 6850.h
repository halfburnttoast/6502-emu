#include <stdbool.h>

//#define CTRL_ADDR 0xA000
#define CTRL_ADDR 0x8005
//#define DATA_ADDR 0xA001
#define DATA_ADDR 0x8001
#define READ_ADDR 0x8002
#define RAND_ADDR 0x8003
#define IO_ADDR 0x8004

struct UartStatusBits{
	bool RDRF:1; // bit 0
	bool TDRE:1;
	bool DCD:1;
	bool CTS:1;
	bool FE:1;
	bool OVRN:1;
	bool PE:1;
	bool IRQ:1; // bit 7
};

union UartStatusReg {
	struct UartStatusBits bits;
	uint8_t byte;
};

union UartStatusReg uart_SR;

uint8_t incoming_char;

void init_uart();

void step_uart();
