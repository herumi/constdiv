#include <stdio.h>
#include <stdint.h>

namespace kem {
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

void test3329()
{
	puts("test3329");
	for (int x = -32768; x < 32768; x++) {
		int r = x % q;
		int r1 = ref3329(x);
		int r2 = mod3329(x);
		if (!(-q/2 <= r2 && r2 <= q/2)) {
			printf("ERR0 x=%d r2=%d\n", x, r2);
		}
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

} // kem

int main()
{
	kem::test3329();
}
