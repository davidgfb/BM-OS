/* Customized Ray Tracer for BareMetal OS
*
* Based on Andrew Kensler's business card sized ray tracer
* Breakdown by Fabien Sanglard here: https://fabiensanglard.net/rayTracing_back_of_business_card/
*
* - Converted from C++ to C
* - Uses built-in functions for pow, sqrt, ceil, and rand
*
* Upon execution it will render the image directly to the screen
* The first run will use only one CPU core
* The second run will use all available CPU cores*/

#include "libBareMetal.h"
//#include <stdio.h> // NO soportado

#define R1 1103515245
#define R2 12345
#define R3 (1<<31) // 2^31
#define RAND_MAX 32767

#define TAMAGNO 80

u8 *frame_buffer, *cpu_table;
u16 X, Y;
u32 progress = 0;
u64 next = 1, // For rand()
    lock = 0, TOTALCORES = 0, BSP;

// Custom pow
float bpow(float x, float y) {
	float result = 1.0;
	int neg_exp_flag = 0;

	if (y < 0) {
		neg_exp_flag = 1;
		y *= -1;
	}

	while (y > 0) {
        	if ((int)y % 2 == 1) result *= x; // Si el exponente es impar

	        x *= x;  // Elevar al cuadrado la base
        	y /= 2;  // Dividir el exponente entre 2
    	}

	if (neg_exp_flag) result = 1.0 / result;

	return result;
}

//#include <math.h> // fabs NO incluido

// Implementación manual de fabs
float fabs(float x) {return (x < 0) ? -x : x;}

// Custom sqrt
float bsqrt(float num) {
	if (num < 0) return -1;

	float e = 1e-5, guess = num / 2.0;

	while (fabs(guess - num / guess) >= e) guess = (guess + num / guess) / 2.0;  // Método de Newton

	return guess;
}

// Custom ceil
float bceil(float num) {
	float part = (int)num;

	// Si el número tiene parte decimal, devuelve el siguiente número entero
	return (num > part) ? part + 1 : part;
}

// Custom rand
uint64_t rand() {
	next = (R1 * next + R2) % R3;

	return next % (RAND_MAX + 1);
}

// Vector Structure
typedef struct {float x, y, z;} vector;

// Vector Add
vector v_add(vector a, vector b) {
	vector result = {a.x + b.x, a.y + b.y, a.z + b.z};

	return result;
}

// Vector Multiply/Scale
vector v_mul(vector a, float b) {
	vector result = { a.x * b, a.y * b, a.z * b };

	return result;
}

// Vector Dot Product
float v_dot(vector a, vector b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Vector Cross-Product
vector v_cross(vector a, vector b) {
	vector result = {a.y * b.z - a.z * b.y,
			 a.z * b.x - a.x * b.z,
			 a.x * b.y - a.y * b.x};

	return result;
}

// Vector Normalize
vector v_norm(vector a) {
	float mag = bsqrt(v_dot(a, a));

	if (mag == 0) {
        	// Manejar el caso de vector cero: no se puede normalizar
        	vector zero_vector = {0.0, 0.0, 0.0};

        	return zero_vector;
    	}

	return v_mul(a, 1 / mag);
}

// Vector Initialization
vector v_init(float a, float b, float c) {
	vector result = {a, b, c};

	return result;
}

// Array of spheres (displaying 'Hi!')
int G[] = {280336, 279040, 279040, 279056, 509456, 278544, 278544, 279056,
	   278544};

// Random generator, return a float within range [0-1]
float R() {return (float)rand() / (RAND_MAX + 1.0);}

// Tracer
// Return 0 if no hit was found but ray goes up
// Return 1 if no hit was found but ray goes down
// Return 2 if a hit was found (and also return distance t and bouncing ray n)
int T(vector o, vector d, float *t, vector *n) {
	*t = 1e9;
	int m = 0;
	float p = -o.z / d.z;

	if (p > 0.01) {
		*t = p;
		*n = v_init(0, 0, 1);
		m = 1;
	}

	for (int k = 0; k < 19; k++) {
		for (int j = 0; j < 9; j++) {
			if (G[j] & (1 << k)) {
				vector p = v_add(o, v_init(-k, 0, -j - 4));
				float b = v_dot(p, d), c = v_dot(p, p) - 1,
				      q = b * b - c;

				if (q > 0) {
					float s = -b - bsqrt(q);

					if (s < *t && s > 0.01) {
						*t = s;
						*n = v_norm(v_add(p, v_mul(d, s)));
						m = 2;
					}
				}
			}
		}
	}

	return m;
}

char int_A_Char(int num) {
	int ud = num % 10;

	return '0' + ud;
}

// Sampler
// Return the pixel color for a ray passing by point o (origin) and d (direction)
vector S(vector o, vector d) {
	float t;
	vector n;
	int m = T(o, d, &t, &n);

	/*
	char cadena[50] = "";
	float res = 0.0; // antes de if
	*/
	// Generate a sky color if no sphere is hit and the ray goes up
	if (!m)	{
		float res = bpow(1 - d.z, 4);

		/*
		cadena[0] = int_A_Char((int)res);
		cadena[1] = '\0';
		*/
		//render()->S()
		//b_output(cadena, TAMAGNO); //se vuelve loco
		//sprintf(cadena, "%f", res); // NO soportado, nº -> char
		return v_mul(v_init(0.7, 0.6, 1), res);
	}

	vector h = v_add(o, v_mul(d, t)),
	       l = v_norm(v_add(v_init(9 + R(), 9 + R(), 16), v_mul(h, -1))),
	       r = v_add(d, v_mul(n, -2 * v_dot(n, d)));

	float b = v_dot(l, n);

	if (b < 0 || T(h, l, &t, &n)) b = 0;

	float p = bpow(v_dot(l, r) * (b > 0), 99);

	// Generate a floor color if no sphere is hit and the ray goes down
	if (m & 1) {
		h = v_mul(h, 0.2);

		return v_mul(((int)(bceil(h.x) + bceil(h.y)) & 1) ? v_init(3, 1, 1) : v_init(3, 3, 3), b / 5.0 + 0.1);
	}

	// A sphere was hit.
	return v_add(v_init(p, p, p), v_mul(S(h, r), 0.5));
}

int render() {
	b_system(SMP_LOCK, lock, 0);

	int y = progress++; // Starting line number

	b_system(SMP_UNLOCK, lock, 0);

	vector g = v_norm(v_init(5, -28, 7)), // Camera direction (-/+ = Right/Left, ?/? , Down/Up)
	       a = v_mul(v_norm(v_cross(v_init(0, 0, -1), g)), 0.002), // Camera up vector
	       b = v_mul(v_norm(v_cross(g, a)), 0.002),
	       c = v_add(v_add(v_mul(a, -256), v_mul(b, -256)), g);

	for (; y<Y; y+=TOTALCORES) // Increment by the number of CPUs
		for (int x = 0; x<X; x++) {
			int offset = (y * X + x) * 4; // Calculate the offset into video memory for this line

			vector p = v_init(13, 13, 13); // Reuse the vector class to store the RGB values of a pixel

			for (int r = 64; r--;) {
				vector t = v_add(v_mul(a, (R() - 0.5) * 99), v_mul(b, (R() - 0.5) * 99)),
	             		       ray_dir = v_norm(v_add(v_mul(t, -1), v_mul(v_add(v_add(v_mul(a, R() + x), v_mul(b, y + R())), c), 16)));

				p = v_add(v_mul(S(v_add(v_init(17, 16, 8), t), ray_dir), 3.5), p);
			}

			frame_buffer[offset++] = (int)p.z; // Output RGB values directly to video memory
			frame_buffer[offset++] = (int)p.y;
			frame_buffer[offset++] = (int)p.x;
			frame_buffer[offset++] = 0;
		}
}

void cls() {
	//memset(frame_buffer, 0x40, X * Y * 4); //NO soportado
	u8 pixel = 0x40;

	for (int bytes = 0; bytes < (X * Y * 4); bytes++) frame_buffer[bytes] = pixel;
}

void wait_for_keypress() {
    u8 c = 0;
    do {c = b_input();} while (c == 0);
    cls(); // Limpia la pantalla
}

int main() {
	frame_buffer = (u8 *)b_system(SCREEN_LFB_GET, 0, 0); // Frame buffer address from kernel
	cpu_table = (u8 *)0x5100;
	X = b_system(SCREEN_X_GET, 0, 0); // Screen X
	Y = b_system(SCREEN_Y_GET, 0, 0); // Screen Y

	//u8 tcore;
	u8 c;
	int busy;

	b_output("\nraytrace - First run will be using 1 CPU core\nPress any key to continue", TAMAGNO);
	wait_for_keypress();
	/*
	c = 0;

	do {c = b_input();} while (c == 0);

	cls();
	*/
	TOTALCORES = 1;

	render();

	TOTALCORES = b_system(SMP_NUMCORES, 0, 0); // Total cores in the system
	BSP = b_system(SMP_ID, 0, 0); // ID of the BSP

	b_output("\nRender complete. Second run will use all CPU cores\nPress any key to continue", TAMAGNO);
	wait_for_keypress();
	/*
	c = 0;

	do {c = b_input();} while (c == 0);

	cls();
	*/
	for (u8 t=0; t<TOTALCORES; t++) {
		u8 tcore = cpu_table[t]; // Location of the Active CPU IDs

		if (tcore != BSP) b_system(SMP_SET, (u64)render, tcore); // Have each AP render
	}

	render(); // Have the BSP render as well

	// Wait for all other cores to be finished
	do {busy = b_system(SMP_BUSY, 0, 0);} while (busy == 1);

	b_output("\nRender complete. Press any key to exit", TAMAGNO);
	wait_for_keypress();
	/*
	c = 0;

	do {c = b_input();} while (c == 0);

	cls();
	*/
	return 0;
}
