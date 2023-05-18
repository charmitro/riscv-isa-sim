require_extension('D');
require_extension(EXT_ZFA);
require_fp;
uint64_t a = FRS1_D.v;

uint32_t sign = signF64UI(a);
uint32_t exp  = expF64UI(a);
uint64_t frac = fracF64UI(a);

bool inexact = false;
bool invalid = false;

/* detect the non-normal cases */
bool is_subnorm    = (exp == 0 && frac != 0);
bool is_zero       = (exp == 0 && frac == 0);
bool is_nan_or_inf = (exp == 0x7ff);

/* Restore implicit bit */
frac |= 1ull << 52;
/* Undo the offset-binary */
int true_exp  = exp - 1023;

/* Detect trivial cases for normal numbers */
/* Bits will be 'multiplied' out */
bool is_too_large = true_exp >= 84;
/* Bits will be 'divided' out */
bool is_too_small = true_exp < 0;

if (is_zero) {
  frac = 0;
} else if (is_subnorm) {
  inexact = true;
  frac    = 0;
} else if (is_nan_or_inf) {
  invalid = true;
  frac   = 0;
} else if (is_too_large) {
  invalid = true;
  frac    =  0;
} else if (is_too_small) {
  inexact = true;
  frac    = 0;
} else {
  /* perform the exponentation on the fixed-point
     mantissa and extract the integer part */
  uint128_t fixedpoint = (uint128_t)frac << true_exp;
  frac                 = fixedpoint >> 52;
  uint64_t mantissa    = fixedpoint & UINT64_C(0xFFFFFFFFFFFFF);

  /* apply the sign bit */
  if (sign)
    frac = -frac;

  /* raise FP exception flags, honoring the precedence
     of nV > nX */
  if (true_exp > 31)
    invalid = true;
  else if (mantissa != 0)
    inexact = true;

}

WRITE_RD(sext32(frac));
STATE.fflags->write(STATE.fflags->read() |
		    (inexact ? softfloat_flag_inexact : 0) |
		    (invalid ? softfloat_flag_invalid : 0));
