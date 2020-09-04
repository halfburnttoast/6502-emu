#include <stdio.h>
#include <unistd.h>
#include <sys/poll.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#include "6502-emu.h"
#include "6502.h"
#include "6850.h"

extern int g_force_uppercase;
int n;

void init_uart() {
	memory[DATA_ADDR] = 0;
	
	uart_SR.byte = 0;
	uart_SR.bits.TDRE = 1; // we are always ready to output data
	
	uart_SR.bits.RDRF = 0;
	incoming_char = 0;

    srand(time(0));
}

int stdin_ready() {
	struct pollfd fds;
	fds.fd = 0; // stdin
	fds.events = POLLIN;
	return poll(&fds, 1, 0) == 1; // timeout = 0
}

void step_uart() {
	if (write_addr == &memory[DATA_ADDR]) {
		putchar(memory[DATA_ADDR]);
		if (memory[DATA_ADDR] == '\b') printf(" \b");
		fflush(stdout);
		write_addr = NULL;
	} else if (read_addr == &memory[READ_ADDR]) {
		uart_SR.bits.RDRF = 0;
		read_addr = NULL;
		memory[READ_ADDR] = incoming_char; // modified for TPC
		incoming_char = 0;					// TPC COM architecture needs this to be cleared every loop
	} else if(read_addr == &memory[IO_ADDR]) {
		read_addr = NULL;
		printf("IO: %02X\n", memory[IO_ADDR]);
	} else if (read_addr == &memory[RAND_ADDR]) {
        memory[RAND_ADDR] = rand() % 0xFF;
        read_addr = NULL;
    }
	
	/* update input register if empty */
	if ((n++ % 30) == 0) { // polling stdin every cycle is performance intensive. This is a bit of a dirty hack.
		if (!uart_SR.bits.RDRF && stdin_ready()) { // the real hardware has no buffer. Remote the RDRF check for more accurate emulation.
			if (read(0, &incoming_char, 1) != 1) {
				printf("Warning: read() returned 0\n");
			}
			if (incoming_char == 0x18) { // CTRL+X
				printf("\r\n");
				exit(0);
			}
			if (incoming_char == 0x7F) { // Backspace
				incoming_char = '\b';
			}
			uart_SR.bits.RDRF = 1;
		}
	}

    if(g_force_uppercase) {
        memory[READ_ADDR] = toupper(incoming_char);
    } else {
        memory[READ_ADDR] = incoming_char;
    }
	memory[CTRL_ADDR] = uart_SR.byte;	// CTRL_ADDR is not used for TPC architecture, but must be cleared
										// for polling to resume.
}
