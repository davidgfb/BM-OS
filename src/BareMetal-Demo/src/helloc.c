// helloc.c -- Output a 'hello world' message

// gcc -c -m64 -nostdlib -nostartfiles -nodefaultlibs -o helloc.o helloc.c
// gcc -c -m64 -nostdlib -nostartfiles -nodefaultlibs -o libBareMetal.o libBareMetal.c
// ld -T c.ld -o helloc.app helloc.o libBareMetal.o

#define TAMAGNO 50

#include "libBareMetal.h"

/*
#include <stdint.h>  // u64
#include <string.h>  // NO incluido strlen
*/

int main(void) {
	char cadena[TAMAGNO] = "\nHola, gnapa!";
	//uint64_t tamagno = strlen(cadena);

	b_output(cadena, TAMAGNO);
	return 0;
}
