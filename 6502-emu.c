#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>

#include "6502.h"
#include "6850.h"

struct termios initial_termios;

void step_delay()
{
	struct timespec req, rem;
	
	req.tv_sec = 0;
	req.tv_nsec = STEP_DURATION;
	
	nanosleep(&req, &rem);
}

void run_cpu(int verbose, int mem_dump)
{
	int cycles = 0;
	int cycles_per_step = (CPU_FREQ / (ONE_SECOND / STEP_DURATION));
	
	for (;;) {
		for (cycles %= cycles_per_step; cycles < cycles_per_step;) {
			if (mem_dump) save_memory(NULL);
			cycles += step_cpu(verbose);
			step_uart();
		}
		step_delay(); // remove this for more speed
	}
}

void restore_stdin()
{
	tcsetattr(0, TCSANOW, &initial_termios);
}

void raw_stdin()
{
	struct termios new_termios;
	
	tcgetattr(0, &initial_termios);
	new_termios = initial_termios;
	cfmakeraw(&new_termios);
	tcsetattr(0, TCSANOW, &new_termios);
	atexit(restore_stdin);
}

int hextoint(char *str) {
	int val;

	if (*str == '$') str++;
	val = strtol(str, NULL, 16);
	return val;
}

int main(int argc, char *argv[])
{
	int a, x, y, sp, sr, pc;
	int verbose, interactive, mem_dump;
	int opt;

	verbose = 0;
	interactive = 0;
	mem_dump = 0;
	a = 0;
	x = 0;
	y = 0;
	sp = 0;
	sr = 0;
	pc = -RST_VEC;  // negative implies indirect
	while ((opt = getopt(argc, argv, "vima:x:y:r:p:s:g:")) != -1) {
		switch (opt) {
		case 'v':
			verbose = 1;
			break;
		case 'i':
			interactive = 1;
			break;
		case 'm':
			mem_dump = 1;
			break;
		case 'a':
			a = hextoint(optarg);
			break;
		case 'x':
			x = hextoint(optarg);
			break;
		case 'y':
			y = hextoint(optarg);
			break;
		case 's':
			sp = hextoint(optarg);
			break;
		case 'p':
			sr = hextoint(optarg);
			break;
		case 'r':
		case 'g':
			pc = hextoint(optarg);
			break;
	    default: /* '?' */
			fprintf(stderr, "Usage: %s [-v] [-i] [-a HEX] [-x HEX] [-y HEX] [-s HEX] [-p HEX] [-g|-r ADDR] file.rom\nThe first 16k of \"file.rom\" is loaded into the last 16k of memory.\n",
	               argv[0]);
	       exit(EXIT_FAILURE);
	   }
	}

	if (optind >= argc) {
	   fprintf(stderr, "Expected argument after options\n");
	   exit(EXIT_FAILURE);
	}
	if (load_rom(argv[optind]) != 0) {
		printf("Error loading \"%s\".\n", argv[optind]);
		return EXIT_FAILURE;
	}
	
	if (interactive) raw_stdin(); // allow individual keystrokes to be detected
	
	init_tables();
	init_uart();
	
	reset_cpu(a, x, y, sp, sr, pc);
	run_cpu(verbose, mem_dump);
	
	return EXIT_SUCCESS;
}
