#include "uint128_t.hpp"

const uint64_t c_ = 0x124924925;
const uint32_t a_=35;

extern "C" {

uint32_t div7org(uint32_t x)
{
	return x / 7;
}

uint32_t div19org(uint32_t x)
{
	return x / 19;
}

uint32_t div7org2(uint32_t x)
{
	uint64_t v = x * (c_ & 0xffffffff);
	v >>= 32;
	uint32_t xL = uint32_t(x) - uint32_t(v);
	xL >>= 1;
	xL += uint32_t(v);
	xL >>= 2;
	return xL;
}

uint32_t mod7org(uint32_t x)
{
	return x % 7;
}

uint32_t mod7org2(uint32_t x)
{
	uint64_t v = x * (c_ & 0xffffffff);
	v >>= 32;
	uint32_t xL = uint32_t(x) - uint32_t(v);
	xL >>= 1;
	xL += uint32_t(v);
	xL >>= 2;
	return x - 7 * xL;
}


uint32_t div7a(uint32_t x)
{
	const uint64_t cs = c_ << (64 - a_);
	uint64_t H;
	mulUnit1(&H, x, cs);
	return uint32_t(H);
}

uint32_t div7b(uint32_t x)
{
	uint64_t v = x * (c_ & 0xffffffff);
	v >>= 32;
	v += x;
	v >>= a_-32;
	return uint32_t(v);
}

} // extern "C"
