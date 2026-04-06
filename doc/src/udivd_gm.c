u32 udivd(u32 x) {
  // c and a are constants determined by d.
  u32 cL = c & 0xffffffff;
  u32 y = (u64(x) * cL) >> 32;
  u32 t = ((x - y) >> 1) + y;
  return t >> (a - 33);
}
