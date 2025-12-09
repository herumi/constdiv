#include <stdio.h>
#include <stdint.h>

const int q = 3329;

int16_t ref3329(int16_t x)
{
	int t = (x * 20159 + (1<<25)) >> 26;
	t = x - t * q;
	return t;
}

int16_t mod3329(int16_t x)
{
	int t = (x * 5 + (1<<12)) >> 14;
	t = x - t * q;
	if (t > q/2) {
		t -= q;
	}
	return t;
}

int main()
{
	for (int x = -32768; x < 32768; x++) {
		int r = x % q;
		int r1 = ref3329(x);
		int r2 = mod3329(x);
		if ((r - r1) % q) {
			printf("ERR1 x=%d r=%d r1=%d\n", x, r, r1);
		}
		if ((r - r2) % q) {
			printf("ERR2 x=%d r=%d r2=%d\n", x, r, r2);
		}
		if (r1 != r2) {
			printf("ERR3 x=%d r1=%d r2=%d\n", x, r1, r2);
		}
	}
	puts("ok");
}

