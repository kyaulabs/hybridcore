#include "blowfish.h"
#include "bf_tab.h"             /* P-box P-array, S-box */

static char bf_mode[4];

/* Each box takes up 4k so be very careful here */
#define BOXES 3

/* #define S(x,i) (bf_S[i][x.w.byte##i]) */
#define S0(x) (bf_S[0][x.w.byte0])
#define S1(x) (bf_S[1][x.w.byte1])
#define S2(x) (bf_S[2][x.w.byte2])
#define S3(x) (bf_S[3][x.w.byte3])
#define bf_F(x) (((S0(x) + S1(x)) ^ S2(x)) + S3(x))
#define ROUND(a,b,n) (a.word ^= bf_F(b) ^ bf_P[n])

/* Keep a set of rotating P & S boxes */
static struct box_t {
  u_32bit_t *P;
  u_32bit_t **S;
  char key[81];
  char keybytes;
  time_t lastuse;
} box[BOXES];

/* static u_32bit_t bf_P[bf_N+2]; */
/* static u_32bit_t bf_S[4][256]; */
static u_32bit_t *bf_P;
static u_32bit_t **bf_S;

static void blowfish_encipher(u_32bit_t *xl, u_32bit_t *xr)
{
  union aword Xl;
  union aword Xr;

  Xl.word = *xl;
  Xr.word = *xr;

  Xl.word ^= bf_P[0];
  ROUND(Xr, Xl, 1);
  ROUND(Xl, Xr, 2);
  ROUND(Xr, Xl, 3);
  ROUND(Xl, Xr, 4);
  ROUND(Xr, Xl, 5);
  ROUND(Xl, Xr, 6);
  ROUND(Xr, Xl, 7);
  ROUND(Xl, Xr, 8);
  ROUND(Xr, Xl, 9);
  ROUND(Xl, Xr, 10);
  ROUND(Xr, Xl, 11);
  ROUND(Xl, Xr, 12);
  ROUND(Xr, Xl, 13);
  ROUND(Xl, Xr, 14);
  ROUND(Xr, Xl, 15);
  ROUND(Xl, Xr, 16);
  Xr.word ^= bf_P[17];

  *xr = Xl.word;
  *xl = Xr.word;
}

static void blowfish_decipher(u_32bit_t *xl, u_32bit_t *xr)
{
  union aword Xl;
  union aword Xr;

  Xl.word = *xl;
  Xr.word = *xr;

  Xl.word ^= bf_P[17];
  ROUND(Xr, Xl, 16);
  ROUND(Xl, Xr, 15);
  ROUND(Xr, Xl, 14);
  ROUND(Xl, Xr, 13);
  ROUND(Xr, Xl, 12);
  ROUND(Xl, Xr, 11);
  ROUND(Xr, Xl, 10);
  ROUND(Xl, Xr, 9);
  ROUND(Xr, Xl, 8);
  ROUND(Xl, Xr, 7);
  ROUND(Xr, Xl, 6);
  ROUND(Xl, Xr, 5);
  ROUND(Xr, Xl, 4);
  ROUND(Xl, Xr, 3);
  ROUND(Xr, Xl, 2);
  ROUND(Xl, Xr, 1);
  Xr.word ^= bf_P[0];

  *xl = Xr.word;
  *xr = Xl.word;
}
static void blowfish_init(u_8bit_t *key, int keybytes)
{
  int i, j, bx;
  time_t lowest;
  u_32bit_t data;
  u_32bit_t datal;
  u_32bit_t datar;
  union aword temp;

  /* drummer: Fixes crash if key is longer than 80 char. This may cause the key
   *          to not end with \00 but that's no problem.
   */
  if (keybytes > 80)
    keybytes = 80;

  /* Is buffer already allocated for this? */
  for (i = 0; i < BOXES; i++)
    if (box[i].P != NULL) {
      if ((box[i].keybytes == keybytes) &&
          (!strncmp((char *) (box[i].key), (char *) key, keybytes))) {
        /* Match! */
        box[i].lastuse = now;
        bf_P = box[i].P;
        bf_S = box[i].S;
        return;
      }
    }
  /* No pre-allocated buffer: make new one */
  /* Set 'bx' to empty buffer */
  bx = -1;
  for (i = 0; i < BOXES; i++) {
    if (box[i].P == NULL) {
      bx = i;
      i = BOXES + 1;
    }
  }
  if (bx < 0) {
    /* Find oldest */
    lowest = now;
    for (i = 0; i < BOXES; i++)
      if (box[i].lastuse <= lowest) {
        lowest = box[i].lastuse;
        bx = i;
      }
    nfree(box[bx].P);
    for (i = 0; i < 4; i++)
      nfree(box[bx].S[i]);
    nfree(box[bx].S);
  }
  /* Initialize new buffer */
  /* uh... this is over 4k */
  box[bx].P = nmalloc((bf_N + 2) * sizeof(u_32bit_t));
  box[bx].S = nmalloc(4 * sizeof(u_32bit_t *));
  for (i = 0; i < 4; i++)
    box[bx].S[i] = nmalloc(256 * sizeof(u_32bit_t));
  bf_P = box[bx].P;
  bf_S = box[bx].S;
  box[bx].keybytes = keybytes;
  strncpy(box[bx].key, (char *) key, keybytes);
  box[bx].key[keybytes] = 0;
  box[bx].lastuse = now;
  /* Robey: Reset blowfish boxes to initial state
   * (I guess normally it just keeps scrambling them, but here it's
   * important to get the same encrypted result each time)
   */
  for (i = 0; i < bf_N + 2; i++)
    bf_P[i] = initbf_P[i];
  for (i = 0; i < 4; i++)
    for (j = 0; j < 256; j++)
      bf_S[i][j] = initbf_S[i][j];

  j = 0;
  if (keybytes > 0) {           /* drummer: fixes crash if key=="" */
    for (i = 0; i < bf_N + 2; ++i) {
      temp.word = 0;
      temp.w.byte0 = key[j];
      temp.w.byte1 = key[(j + 1) % keybytes];
      temp.w.byte2 = key[(j + 2) % keybytes];
      temp.w.byte3 = key[(j + 3) % keybytes];
      data = temp.word;
      bf_P[i] = bf_P[i] ^ data;
      j = (j + 4) % keybytes;
    }
  }
  datal = 0x00000000;
  datar = 0x00000000;
  for (i = 0; i < bf_N + 2; i += 2) {
    blowfish_encipher(&datal, &datar);
    bf_P[i] = datal;
    bf_P[i + 1] = datar;
  }
  for (i = 0; i < 4; ++i) {
    for (j = 0; j < 256; j += 2) {
      blowfish_encipher(&datal, &datar);
      bf_S[i][j] = datal;
      bf_S[i][j + 1] = datar;
    }
  }
}

/* Of course, if you change either of these, then your userfile will
 * no longer be able to be shared. :)
 */
#define SALT1  0xdeadd061
#define SALT2  0x23f6b095

/* Convert 64-bit encrypted password to text for userfile */
static char *base64 =
            "./0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static char *cbcbase64 =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

static int base64dec(char c)
{
  int i;

  for (i = 0; i < 64; i++)
    if (base64[i] == c)
      return i;
  return 0;
}

static int cbcbase64dec(char c)
{
  char *i = strchr(cbcbase64, c);
  return i ? (int)(i - cbcbase64) : -1;
}


/* Returned string must be freed when done with it!
 */
static char *encrypt_string_ecb(char *key, char *str)
{
  u_32bit_t left, right;
  unsigned char *p;
  char *s, *dest, *d;
  int i;

  /* Pad fake string with 8 bytes to make sure there's enough */
  s = nmalloc(strlen(str) + 9);
  strcpy(s, str);
  if ((!key) || (!key[0]))
    return s;
  p = (unsigned char *) s;
  dest = nmalloc((strlen(str) + 9) * 2);
  while (*p)
    p++;
  for (i = 0; i < 8; i++)
    *p++ = 0;
  blowfish_init((unsigned char *) key, strlen(key));
  p = (unsigned char *) s;
  d = dest;
  while (*p) {
    left = ((*p++) << 24);
    left += ((*p++) << 16);
    left += ((*p++) << 8);
    left += (*p++);
    right = ((*p++) << 24);
    right += ((*p++) << 16);
    right += ((*p++) << 8);
    right += (*p++);
    blowfish_encipher(&left, &right);
    for (i = 0; i < 6; i++) {
      *d++ = base64[right & 0x3f];
      right = (right >> 6);
    }
    for (i = 0; i < 6; i++) {
      *d++ = base64[left & 0x3f];
      left = (left >> 6);
    }
  }
  *d = 0;
  nfree(s);
  return dest;
}

/* Returned string must be freed when done with it!
 */
static char *encrypt_string_cbc(char *key, char *str)
{
  u_32bit_t left, right, prevleft = 0, prevright = 0;
  unsigned char *p;
  char *s, *dest;
  int i, slen;

  /* Pad fake string with 8 bytes to make sure there's enough
   * and prepend with 8 byte IV */
  slen = strlen(str) + 8;
  s = nmalloc(slen + 9);
  for (i = 0; i < 8; ++i) {
    s[i] = (char) (random() % 256);
  }
  strcpy(s + 8, str);
  if ((!key) || (!key[0]))
    return s;
  p = (unsigned char *) s + slen;
  while (slen % 8) {
    ++slen;
    *p++ = 0;
  }
  *p = 0;

  blowfish_init((unsigned char *) key, strlen(key));

  p = (unsigned char *) s;
  /* p[0] can be 0 as it's random IV so check if at start of IV */
  while (*p || (p == (unsigned char *)s)) {
    left = ((*p++) << 24);
    left |= ((*p++) << 16);
    left |= ((*p++) << 8);
    left |= (*p++);
    right = ((*p++) << 24);
    right |= ((*p++) << 16);
    right |= ((*p++) << 8);
    right |= (*p++);

    /* XOR with previous encrypted block */
    left ^= prevleft;
    right ^= prevright;

    blowfish_encipher(&left, &right);

    /* save this encrypted block to use for next */
    prevleft = left;
    prevright = right;

    /* turn back into chars */
    for (i = 0; i < 32; i += 8) {
        *--p = (unsigned char) ((right >> i) & 0xff);
    }
    for (i = 0; i < 32; i += 8) {
        *--p = (unsigned char) ((left >> i) & 0xff);
    }
    p += 8;
  }

  /* base64 encoded string won't be longer than double the size,
   * plus 2 for the * prefix and NULL suffix */
  dest = nmalloc(slen * 2 + 2);
  dest[0] = '*';

  /* base64 encode */
  /* go on til slen - #possible pads */
  p = (unsigned char *) dest + 1;
  for (i = 0; i < slen - 2; i += 3) {
    *p++ = cbcbase64[((unsigned char) s[i]) >> 2];
    *p++ = cbcbase64[((s[i] & 0x03) << 4) | (((unsigned char)s[i + 1]) >> 4)];
    *p++ = cbcbase64[((s[i + 1] & 0x0f) << 2) | (((unsigned char)s[i + 2]) >> 6)];
    *p++ = cbcbase64[s[i + 2] & 0x3f];
  }
  if (slen - i == 2) {
    *p++ = cbcbase64[((unsigned char) s[i]) >> 2];
    *p++ = cbcbase64[((s[i] & 0x03) << 4) | (((unsigned char)s[i + 1]) >> 4)];
    *p++ = cbcbase64[(s[i + 1] & 0x0f) << 2];
    *p++ = '=';
  } else if (slen - i == 1) {
    *p++ = cbcbase64[((unsigned char) s[i]) >> 2];
    *p++ = cbcbase64[(s[i] & 0x03) << 4];
    *p++ = '=';
    *p++ = '=';
  }
  *p = 0;

  nfree(s);

  return dest;
}
/* Returned string must be freed when done with it!
 */
static char *encrypt_string(char *key, char *str)
{
  if (!egg_strncasecmp(key, "ecb:", 4)) {
    return encrypt_string_ecb(key + 4, str);

  } else if (!egg_strncasecmp(key, "cbc:", 4)) {
    return encrypt_string_cbc(key + 4, str);

  } else if (!egg_strncasecmp(bf_mode, "ecb", sizeof bf_mode)) {
    return encrypt_string_ecb(key, str);

  } else if (!egg_strncasecmp(bf_mode, "cbc", sizeof bf_mode)) {
    return encrypt_string_cbc(key, str);

  }

  /* else ECB for now, change at v1.9.0! */
  return encrypt_string_ecb(key, str);
}

/* Returned string must be freed when done with it!
 */
static char *decrypt_string_ecb(char *key, char *str)
{
  u_32bit_t left, right;
  char *p, *s, *dest, *d;
  int i;

  /* Pad encoded string with 0 bits in case it's bogus */
  s = nmalloc(strlen(str) + 12);
  strcpy(s, str);
  if ((!key) || (!key[0]))
    return s;
  p = s;
  dest = nmalloc(strlen(str) + 12);
  while (*p)
    p++;
  for (i = 0; i < 12; i++)
    *p++ = 0;
  blowfish_init((unsigned char *) key, strlen(key));
  p = s;
  d = dest;
  while (*p) {
    right = 0L;
    left = 0L;
    for (i = 0; i < 6; i++)
      right |= (base64dec(*p++)) << (i * 6);
    for (i = 0; i < 6; i++)
      left |= (base64dec(*p++)) << (i * 6);
    blowfish_decipher(&left, &right);
    for (i = 0; i < 4; i++)
      *d++ = (left & (0xff << ((3 - i) * 8))) >> ((3 - i) * 8);
    for (i = 0; i < 4; i++)
      *d++ = (right & (0xff << ((3 - i) * 8))) >> ((3 - i) * 8);
  }
  *d = 0;
  nfree(s);
  return dest;
}

/* Returned string must be freed when done with it!
 */
static char *decrypt_string_cbc(char *key, char *str)
{
  u_32bit_t left, right, prevleft = 0, prevright = 0, prevencleft, prevencright;
  unsigned char *p;
  char *s, *dest;
  int i, slen, dlen;

  slen = strlen(str);
  s = nmalloc(slen + 1);
  strcpy(s, str);
  s[slen] = 0;
  if ((!key) || (!key[0]) || (slen % 4))
    return s;

  blowfish_init((unsigned char *) key, strlen(key));

  /* base64 decode */
  dlen = (slen >> 2) * 3;
  dest = nmalloc(dlen + 1);
  p = (unsigned char *) dest;
  /* '=' will/should return 64 */
  for (i = 0; i < slen; i += 4) {
    int s1 = cbcbase64dec(s[i]);
    int s2 = cbcbase64dec(s[i + 1]);
    int s3 = cbcbase64dec(s[i + 2]);
    int s4 = cbcbase64dec(s[i + 3]);
    if (s1 < 0 || s1 == 64 || s2 < 0 || s2 == 64 || s3 < 0 || s4 < 0)
      return s;
    *p++ = (unsigned char) ((s1 << 2) | (s2 >> 4));
    if (s3 != 64) {
      *p++ = (unsigned char) ((s2 << 4) | (s3 >> 2));
      if (s4 != 64)
        *p++ = (unsigned char) ((s3 << 6) | s4);
      else
        --dlen;
    } else
      dlen -= 2;
  }
  *p = 0;

  /* see if multiple of 8 bytes */
  if (dlen % 8)
    return s;

  p = (unsigned char *) dest;
  while ((p - (unsigned char *)dest) < dlen) {
    left = ((*p++) << 24);
    left |= ((*p++) << 16);
    left |= ((*p++) << 8);
    left |= (*p++);
    right = ((*p++) << 24);
    right |= ((*p++) << 16);
    right |= ((*p++) << 8);
    right |= (*p++);

    /* Save encoded block to xor next decrypted block with */
    prevencleft = left;
    prevencright = right;

    blowfish_decipher(&left, &right);

    /* XOR decrypted block with previous encoded block */
    left ^= prevleft;
    right ^= prevright;
    prevleft = prevencleft;
    prevright = prevencright;

    /* turn back into chars */
    for (i = 0; i < 32; i += 8) {
        *--p = (unsigned char) ((right >> i) & 0xff);
    }
    for (i = 0; i < 32; i += 8) {
        *--p = (unsigned char) ((left >> i) & 0xff);
    }
    p += 8;
  }

  /* cut off IV */
  strcpy(s, dest + 8);
  s[dlen - 8] = 0;
  nfree(dest);

  return s;
}

/* Returned string must be freed when done with it!
 */
static char *decrypt_string(char *key, char *str)
{
  if (!egg_strncasecmp(key, "ecb:", 4)) {
    if (str[0] == '*') {
      /* ecb strings shouldn't start with * */
      return decrypt_string_cbc(key + 4, str + 1);
    }
    /* else */
    return decrypt_string_ecb(key + 4, str);

  } else if (!egg_strncasecmp(key, "cbc:", 4)) {
    if (str[0] != '*') {
      /* cbc strings should start with * */
      return decrypt_string_ecb(key + 4, str);
    }
    /* else */
    return decrypt_string_cbc(key + 4, str + 1);

  } else if (str[0] != '*') {
      return decrypt_string_ecb(key, str);
  }

  /* else */
  return decrypt_string_cbc(key, str + 1);
}