/*
Copyright (c) 2017 landave

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef KERNEL_H_
#define KERNEL_H_

#include <string>
#include "TSHasherContext.h"

const char* TSHasherContext::KERNEL_CODE = R"(
#if VENDOR_ID == (1 << 0)
#define IS_AMD
#elif VENDOR_ID == (1 << 5)
#define IS_NV
#else
#define IS_GENERIC
#endif

#ifdef IS_AMD
#pragma OPENCL EXTENSION cl_amd_media_ops  : enable
#pragma OPENCL EXTENSION cl_amd_media_ops2 : enable
#endif

// the following basic SHA1 implementation has been
// taken from the hashcat project
// see https://hashcat.net/hashcat/
// and https://github.com/hashcat/hashcat

typedef enum sha1_constants
{
  SHA1M_A=0x67452301,
  SHA1M_B=0xefcdab89,
  SHA1M_C=0x98badcfe,
  SHA1M_D=0x10325476,
  SHA1M_E=0xc3d2e1f0,

  SHA1C00=0x5a827999,
  SHA1C01=0x6ed9eba1,
  SHA1C02=0x8f1bbcdc,
  SHA1C03=0xca62c1d6u

} sha1_constants_t;

#ifdef IS_NV
#define SHA1_F0(x,y,z)  ((z) ^ ((x) & ((y) ^ (z))))
#define SHA1_F1(x,y,z)  ((x) ^ (y) ^ (z))
#define SHA1_F2(x,y,z)  (((x) & (y)) | ((z) & ((x) ^ (y))))
#define SHA1_F0o(x,y,z) (bitselect ((z), (y), (x)))
#define SHA1_F2o(x,y,z) (bitselect ((x), (y), ((x) ^ (z))))
#endif

#ifdef IS_AMD
#define SHA1_F0(x,y,z)  ((z) ^ ((x) & ((y) ^ (z))))
#define SHA1_F1(x,y,z)  ((x) ^ (y) ^ (z))
#define SHA1_F2(x,y,z)  (((x) & (y)) | ((z) & ((x) ^ (y))))
#define SHA1_F0o(x,y,z) (bitselect ((z), (y), (x)))
#define SHA1_F2o(x,y,z) (bitselect ((x), (y), ((x) ^ (z))))
#endif

#ifdef IS_GENERIC
#define SHA1_F0(x,y,z)  ((z) ^ ((x) & ((y) ^ (z))))
#define SHA1_F1(x,y,z)  ((x) ^ (y) ^ (z))
#define SHA1_F2(x,y,z)  (((x) & (y)) | ((z) & ((x) ^ (y))))
#define SHA1_F0o(x,y,z) (SHA1_F0 ((x), (y), (z)))
#define SHA1_F2o(x,y,z) (SHA1_F2 ((x), (y), (z)))
#endif


#define SHA1_STEP(f,a,b,c,d,e,x)    \
{                                   \
  e += K;                           \
  e += x;                           \
  e += f (b, c, d);                 \
  e += rotate (a,  5u);             \
  b  = rotate (b, 30u);             \
}

typedef uchar  u8;
typedef ushort u16;
typedef uint   u32;
typedef ulong  u64;


void sha1_64 (u32 block[16], u32 digest[5])
{

  u32 a = digest[0];
  u32 b = digest[1];
  u32 c = digest[2];
  u32 d = digest[3];
  u32 e = digest[4];

  u32 w0_t = block[ 0];
  u32 w1_t = block[ 1];
  u32 w2_t = block[ 2];
  u32 w3_t = block[ 3];
  u32 w4_t = block[ 4];
  u32 w5_t = block[ 5];
  u32 w6_t = block[ 6];
  u32 w7_t = block[ 7];
  u32 w8_t = block[ 8];
  u32 w9_t = block[ 9];
  u32 wa_t = block[10];
  u32 wb_t = block[11];
  u32 wc_t = block[12];
  u32 wd_t = block[13];
  u32 we_t = block[14];
  u32 wf_t = block[15];

  #undef K
  #define K SHA1C00

  SHA1_STEP (SHA1_F0o, a, b, c, d, e, w0_t);
  SHA1_STEP (SHA1_F0o, e, a, b, c, d, w1_t);
  SHA1_STEP (SHA1_F0o, d, e, a, b, c, w2_t);
  SHA1_STEP (SHA1_F0o, c, d, e, a, b, w3_t);
  SHA1_STEP (SHA1_F0o, b, c, d, e, a, w4_t);
  SHA1_STEP (SHA1_F0o, a, b, c, d, e, w5_t);
  SHA1_STEP (SHA1_F0o, e, a, b, c, d, w6_t);
  SHA1_STEP (SHA1_F0o, d, e, a, b, c, w7_t);
  SHA1_STEP (SHA1_F0o, c, d, e, a, b, w8_t);
  SHA1_STEP (SHA1_F0o, b, c, d, e, a, w9_t);
  SHA1_STEP (SHA1_F0o, a, b, c, d, e, wa_t);
  SHA1_STEP (SHA1_F0o, e, a, b, c, d, wb_t);
  SHA1_STEP (SHA1_F0o, d, e, a, b, c, wc_t);
  SHA1_STEP (SHA1_F0o, c, d, e, a, b, wd_t);
  SHA1_STEP (SHA1_F0o, b, c, d, e, a, we_t);
  SHA1_STEP (SHA1_F0o, a, b, c, d, e, wf_t);
 
  w0_t = rotate ((wd_t ^ w8_t ^ w2_t ^ w0_t), 1u);
  SHA1_STEP (SHA1_F0o, e, a, b, c, d, w0_t);
  w1_t = rotate ((we_t ^ w9_t ^ w3_t ^ w1_t), 1u);
  SHA1_STEP (SHA1_F0o, d, e, a, b, c, w1_t);
  w2_t = rotate ((wf_t ^ wa_t ^ w4_t ^ w2_t), 1u);
  SHA1_STEP (SHA1_F0o, c, d, e, a, b, w2_t);
  w3_t = rotate ((w0_t ^ wb_t ^ w5_t ^ w3_t), 1u);
  SHA1_STEP (SHA1_F0o, b, c, d, e, a, w3_t);

  #undef K
  #define K SHA1C01

  w4_t = rotate ((w1_t ^ wc_t ^ w6_t ^ w4_t), 1u);
  SHA1_STEP (SHA1_F1, a, b, c, d, e, w4_t);
  w5_t = rotate ((w2_t ^ wd_t ^ w7_t ^ w5_t), 1u);
  SHA1_STEP (SHA1_F1, e, a, b, c, d, w5_t);
  w6_t = rotate ((w3_t ^ we_t ^ w8_t ^ w6_t), 1u);
  SHA1_STEP (SHA1_F1, d, e, a, b, c, w6_t);
  w7_t = rotate ((w4_t ^ wf_t ^ w9_t ^ w7_t), 1u);
  SHA1_STEP (SHA1_F1, c, d, e, a, b, w7_t);
  w8_t = rotate ((w5_t ^ w0_t ^ wa_t ^ w8_t), 1u);
  SHA1_STEP (SHA1_F1, b, c, d, e, a, w8_t);
  w9_t = rotate ((w6_t ^ w1_t ^ wb_t ^ w9_t), 1u);
  SHA1_STEP (SHA1_F1, a, b, c, d, e, w9_t);
  wa_t = rotate ((w7_t ^ w2_t ^ wc_t ^ wa_t), 1u);
  SHA1_STEP (SHA1_F1, e, a, b, c, d, wa_t);
  wb_t = rotate ((w8_t ^ w3_t ^ wd_t ^ wb_t), 1u);
  SHA1_STEP (SHA1_F1, d, e, a, b, c, wb_t);
  wc_t = rotate ((w9_t ^ w4_t ^ we_t ^ wc_t), 1u);
  SHA1_STEP (SHA1_F1, c, d, e, a, b, wc_t);
  wd_t = rotate ((wa_t ^ w5_t ^ wf_t ^ wd_t), 1u);
  SHA1_STEP (SHA1_F1, b, c, d, e, a, wd_t);
  we_t = rotate ((wb_t ^ w6_t ^ w0_t ^ we_t), 1u);
  SHA1_STEP (SHA1_F1, a, b, c, d, e, we_t);
  wf_t = rotate ((wc_t ^ w7_t ^ w1_t ^ wf_t), 1u);
  SHA1_STEP (SHA1_F1, e, a, b, c, d, wf_t);
  w0_t = rotate ((wd_t ^ w8_t ^ w2_t ^ w0_t), 1u);
  SHA1_STEP (SHA1_F1, d, e, a, b, c, w0_t);
  w1_t = rotate ((we_t ^ w9_t ^ w3_t ^ w1_t), 1u);
  SHA1_STEP (SHA1_F1, c, d, e, a, b, w1_t);
  w2_t = rotate ((wf_t ^ wa_t ^ w4_t ^ w2_t), 1u);
  SHA1_STEP (SHA1_F1, b, c, d, e, a, w2_t);
  w3_t = rotate ((w0_t ^ wb_t ^ w5_t ^ w3_t), 1u);
  SHA1_STEP (SHA1_F1, a, b, c, d, e, w3_t);
  w4_t = rotate ((w1_t ^ wc_t ^ w6_t ^ w4_t), 1u);
  SHA1_STEP (SHA1_F1, e, a, b, c, d, w4_t);
  w5_t = rotate ((w2_t ^ wd_t ^ w7_t ^ w5_t), 1u);
  SHA1_STEP (SHA1_F1, d, e, a, b, c, w5_t);
  w6_t = rotate ((w3_t ^ we_t ^ w8_t ^ w6_t), 1u);
  SHA1_STEP (SHA1_F1, c, d, e, a, b, w6_t);
  w7_t = rotate ((w4_t ^ wf_t ^ w9_t ^ w7_t), 1u);
  SHA1_STEP (SHA1_F1, b, c, d, e, a, w7_t);

  #undef K
  #define K SHA1C02

  w8_t = rotate ((w5_t ^ w0_t ^ wa_t ^ w8_t), 1u);
  SHA1_STEP (SHA1_F2o, a, b, c, d, e, w8_t);
  w9_t = rotate ((w6_t ^ w1_t ^ wb_t ^ w9_t), 1u);
  SHA1_STEP (SHA1_F2o, e, a, b, c, d, w9_t);
  wa_t = rotate ((w7_t ^ w2_t ^ wc_t ^ wa_t), 1u);
  SHA1_STEP (SHA1_F2o, d, e, a, b, c, wa_t);
  wb_t = rotate ((w8_t ^ w3_t ^ wd_t ^ wb_t), 1u);
  SHA1_STEP (SHA1_F2o, c, d, e, a, b, wb_t);
  wc_t = rotate ((w9_t ^ w4_t ^ we_t ^ wc_t), 1u);
  SHA1_STEP (SHA1_F2o, b, c, d, e, a, wc_t);
  wd_t = rotate ((wa_t ^ w5_t ^ wf_t ^ wd_t), 1u);
  SHA1_STEP (SHA1_F2o, a, b, c, d, e, wd_t);
  we_t = rotate ((wb_t ^ w6_t ^ w0_t ^ we_t), 1u);
  SHA1_STEP (SHA1_F2o, e, a, b, c, d, we_t);
  wf_t = rotate ((wc_t ^ w7_t ^ w1_t ^ wf_t), 1u);
  SHA1_STEP (SHA1_F2o, d, e, a, b, c, wf_t);
  w0_t = rotate ((wd_t ^ w8_t ^ w2_t ^ w0_t), 1u);
  SHA1_STEP (SHA1_F2o, c, d, e, a, b, w0_t);
  w1_t = rotate ((we_t ^ w9_t ^ w3_t ^ w1_t), 1u);
  SHA1_STEP (SHA1_F2o, b, c, d, e, a, w1_t);
  w2_t = rotate ((wf_t ^ wa_t ^ w4_t ^ w2_t), 1u);
  SHA1_STEP (SHA1_F2o, a, b, c, d, e, w2_t);
  w3_t = rotate ((w0_t ^ wb_t ^ w5_t ^ w3_t), 1u);
  SHA1_STEP (SHA1_F2o, e, a, b, c, d, w3_t);
  w4_t = rotate ((w1_t ^ wc_t ^ w6_t ^ w4_t), 1u);
  SHA1_STEP (SHA1_F2o, d, e, a, b, c, w4_t);
  w5_t = rotate ((w2_t ^ wd_t ^ w7_t ^ w5_t), 1u);
  SHA1_STEP (SHA1_F2o, c, d, e, a, b, w5_t);
  w6_t = rotate ((w3_t ^ we_t ^ w8_t ^ w6_t), 1u);
  SHA1_STEP (SHA1_F2o, b, c, d, e, a, w6_t);
  w7_t = rotate ((w4_t ^ wf_t ^ w9_t ^ w7_t), 1u);
  SHA1_STEP (SHA1_F2o, a, b, c, d, e, w7_t);
  w8_t = rotate ((w5_t ^ w0_t ^ wa_t ^ w8_t), 1u);
  SHA1_STEP (SHA1_F2o, e, a, b, c, d, w8_t);
  w9_t = rotate ((w6_t ^ w1_t ^ wb_t ^ w9_t), 1u);
  SHA1_STEP (SHA1_F2o, d, e, a, b, c, w9_t);
  wa_t = rotate ((w7_t ^ w2_t ^ wc_t ^ wa_t), 1u);
  SHA1_STEP (SHA1_F2o, c, d, e, a, b, wa_t);
  wb_t = rotate ((w8_t ^ w3_t ^ wd_t ^ wb_t), 1u);
  SHA1_STEP (SHA1_F2o, b, c, d, e, a, wb_t);

  #undef K
  #define K SHA1C03

  wc_t = rotate ((w9_t ^ w4_t ^ we_t ^ wc_t), 1u);
  SHA1_STEP (SHA1_F1, a, b, c, d, e, wc_t);
  wd_t = rotate ((wa_t ^ w5_t ^ wf_t ^ wd_t), 1u);
  SHA1_STEP (SHA1_F1, e, a, b, c, d, wd_t);
  we_t = rotate ((wb_t ^ w6_t ^ w0_t ^ we_t), 1u);
  SHA1_STEP (SHA1_F1, d, e, a, b, c, we_t);
  wf_t = rotate ((wc_t ^ w7_t ^ w1_t ^ wf_t), 1u);
  SHA1_STEP (SHA1_F1, c, d, e, a, b, wf_t);
  w0_t = rotate ((wd_t ^ w8_t ^ w2_t ^ w0_t), 1u);
  SHA1_STEP (SHA1_F1, b, c, d, e, a, w0_t);
  w1_t = rotate ((we_t ^ w9_t ^ w3_t ^ w1_t), 1u);
  SHA1_STEP (SHA1_F1, a, b, c, d, e, w1_t);
  w2_t = rotate ((wf_t ^ wa_t ^ w4_t ^ w2_t), 1u);
  SHA1_STEP (SHA1_F1, e, a, b, c, d, w2_t);
  w3_t = rotate ((w0_t ^ wb_t ^ w5_t ^ w3_t), 1u);
  SHA1_STEP (SHA1_F1, d, e, a, b, c, w3_t);
  w4_t = rotate ((w1_t ^ wc_t ^ w6_t ^ w4_t), 1u);
  SHA1_STEP (SHA1_F1, c, d, e, a, b, w4_t);
  w5_t = rotate ((w2_t ^ wd_t ^ w7_t ^ w5_t), 1u);
  SHA1_STEP (SHA1_F1, b, c, d, e, a, w5_t);
  w6_t = rotate ((w3_t ^ we_t ^ w8_t ^ w6_t), 1u);
  SHA1_STEP (SHA1_F1, a, b, c, d, e, w6_t);
  w7_t = rotate ((w4_t ^ wf_t ^ w9_t ^ w7_t), 1u);
  SHA1_STEP (SHA1_F1, e, a, b, c, d, w7_t);
  w8_t = rotate ((w5_t ^ w0_t ^ wa_t ^ w8_t), 1u);
  SHA1_STEP (SHA1_F1, d, e, a, b, c, w8_t);
  w9_t = rotate ((w6_t ^ w1_t ^ wb_t ^ w9_t), 1u);
  SHA1_STEP (SHA1_F1, c, d, e, a, b, w9_t);
  wa_t = rotate ((w7_t ^ w2_t ^ wc_t ^ wa_t), 1u);
  SHA1_STEP (SHA1_F1, b, c, d, e, a, wa_t);
  wb_t = rotate ((w8_t ^ w3_t ^ wd_t ^ wb_t), 1u);
  SHA1_STEP (SHA1_F1, a, b, c, d, e, wb_t);
  wc_t = rotate ((w9_t ^ w4_t ^ we_t ^ wc_t), 1u);
  SHA1_STEP (SHA1_F1, e, a, b, c, d, wc_t);
  wd_t = rotate ((wa_t ^ w5_t ^ wf_t ^ wd_t), 1u);
  SHA1_STEP (SHA1_F1, d, e, a, b, c, wd_t);
  we_t = rotate ((wb_t ^ w6_t ^ w0_t ^ we_t), 1u);
  SHA1_STEP (SHA1_F1, c, d, e, a, b, we_t);
  wf_t = rotate ((wc_t ^ w7_t ^ w1_t ^ wf_t), 1u);
  SHA1_STEP (SHA1_F1, b, c, d, e, a, wf_t);

  digest[0] += a;
  digest[1] += b;
  digest[2] += c;
  digest[3] += d;
  digest[4] += e;
}

ulong countertostring(char* dst, ulong c) {
  if (c==0) { *dst = '0'; return 1; }
  char* dst0 = dst;
  char* dst1 = dst;
  ulong counterlength=0;
  while (c) {
    uchar currentdigit = c%10;
    *dst1 = '0'+currentdigit;
    dst1++;
    counterlength++;
    c = c/10;
  }
  // invert string
  
  dst1--;
  while (dst0 < dst1) {
    uchar tmp = *dst0;
    *dst0 = *dst1;
    *dst1 = tmp;
    dst0++;
    dst1--;
  }
  
  return counterlength;
}


inline uint swap_uint(uint val) {
  #ifdef IS_NV
    u32 r;
    asm ("prmt.b32 %0, %1, 0, 0x0123;" : "=r"(r) : "r"(val));
    return r;
  #else
    return (as_uint (as_uchar4 (val).s3210));
  #endif
}
    
inline ulong swap_ulong(ulong val)
{
    #ifdef IS_NV
      u32 il;
      u32 ir;

      asm ("mov.b64 {%0, %1}, %2;" : "=r"(il), "=r"(ir) : "l"(val));

      u32 tl;
      u32 tr;

      asm ("prmt.b32 %0, %1, 0, 0x0123;" : "=r"(tl) : "r"(il));
      asm ("prmt.b32 %0, %1, 0, 0x0123;" : "=r"(tr) : "r"(ir));

      u64 r;

      asm ("mov.b64 %0, {%1, %2};" : "=l"(r) : "r"(tr), "r"(tl));

      return r;
    #else
      return (as_ulong (as_uchar8 (val).s76543210));
    #endif  
}

inline uchar getDifficulty(uchar* hash) {
  // we push all difficulty levels up to 8 to zero
  // this increases performance
  if (hash[0]!=0) { return 0; }

  
  uchar zerobytes = 1;
  while (zerobytes < 20 && hash[zerobytes] == 0) {
    zerobytes++;
  }
  uchar zerobits = 0;
  if (zerobytes < 20) {
    uchar lastbyte = hash[zerobytes];
    while (!(lastbyte & 1)) {
      zerobits++;
      lastbyte >>= 1;
    }
  }

  return 8 * zerobytes + zerobits;
}

// increases a decimal string counter
// counterend should point to the least significant digit of the counter
inline void increaseStringCounter(char *counterend) {
  bool add = 1;
  
  while (add) {
    uchar currentdigit = *counterend - '0';
    add = currentdigit == 9;
    *counterend = (currentdigit+1)%10 + '0';
    counterend--;
  }
}

// increases a decimal string counter
// counterend should point to the least significant digit of the counter
inline void increaseStringCounterSwapped(char *str, int endpos) {
  bool add = 1;
  
  while (add) {
    uchar currentdigit = str[endpos] - '0';
    add = currentdigit == 9;
    str[endpos] = (currentdigit+1)%10 + '0';
    endpos--;
  }
}

inline void compute_targetdigest(u32 targetdigest[5], uchar targetdifficulty) {
  int i=0;
  for (; i<targetdifficulty/32; i++) {
    targetdigest[i] = 0xFFFFFFFF;
  }
  if (i < 5) { targetdigest[i] = (((u32)1)<<(targetdifficulty%32))-1; i++; };
  for (; i<5; i++) {
    targetdigest[i] = 0;
  }
}
)"

R"(
__kernel void TeamSpeakHasher (const ulong startcounter,
                               const uint iterations,
                               const uchar targetdifficulty,
                               const __global uchar* identity,
                               const uint identity_length,
                               __global uchar *results)
{
  const int gid = get_global_id(0);
  const uint identity_length_snd_block = identity_length-64;

  u32 hashstring[16];
  
  for (int i=0; i < 64; i++) {
    ((uchar*)hashstring)[i] = identity[i];
  }

  //we hash the first block 
  u32 digest1[5];
  digest1[0] = SHA1M_A;
  digest1[1] = SHA1M_B;
  digest1[2] = SHA1M_C;
  digest1[3] = SHA1M_D;
  digest1[4] = SHA1M_E;
  for (int j = 0; j<16; j++) {
    hashstring[j] = swap_uint(((u32*)hashstring)[j]);
  }
  sha1_64(hashstring, digest1);

  for (int i=0; i<identity_length_snd_block; i++) {
    ((uchar*)hashstring)[i] = identity[i+64];
  }

  for (int i=identity_length_snd_block; i<64; i++) {
    ((uchar*)hashstring)[i] = 0;
  }

  for (int j = 0; j<identity_length_snd_block/4; j++) {
    hashstring[j] = swap_uint(hashstring[j]);
  }

  ulong currentcounter = startcounter+gid*iterations;
  u32 digest2[5];

  ulong counterlen = countertostring(((uchar*)hashstring)+identity_length_snd_block,
                                     currentcounter);
  ((uchar*)hashstring)[identity_length_snd_block+counterlen] = 0x80;

  *((ulong*)(((uchar*)hashstring)+56)) = swap_ulong(8*((ulong)identity_length+counterlen));

  // we bring the length once to 32bit big endian form
  hashstring[14] = swap_uint(hashstring[14]);
  hashstring[15] = swap_uint(hashstring[15]);

  const int swapendianness_start = identity_length_snd_block/4;

  u32 targetdigest[5];
  compute_targetdigest(targetdigest, targetdifficulty);
  //we swap it now, to avoid swapping in the loop
  for (int j=0; j<5; j++) { targetdigest[j] = swap_uint(targetdigest[j]); }

  bool target_found = false;

  for (ulong it = 0; it<iterations; it++) {
    for (int j=0; j<5; j++) { digest2[j] = digest1[j]; }

    for (int j = swapendianness_start; j<14; j++) {
      hashstring[j] = swap_uint(hashstring[j]);
    }

    
    sha1_64(hashstring, digest2);
    // we only consider security levels 32 or larger
    // (this gives us a slight performance boost)
    if (digest2[0] == 0) {
      target_found |= 0 == ((digest2[0] & targetdigest[0]) |
                            (digest2[1] & targetdigest[1]) |
                            (digest2[2] & targetdigest[2]) |
                            (digest2[3] & targetdigest[3]) |
                            (digest2[4] & targetdigest[4]));
    }

    //we swap the endianness back to be able to increase the counter
    for (int j = swapendianness_start; j<14; j++) {
      hashstring[j] = swap_uint(hashstring[j]);
    }

    increaseStringCounter(((uchar*)hashstring)+identity_length_snd_block+counterlen-1);
  }
  results[gid] = target_found;
}
)"


R"(
__kernel void TeamSpeakHasher2 (const ulong startcounter,
                                const uint iterations,
                                const uchar targetdifficulty,
                                const __global uchar* identity,
                                const uint identity_length,
                                __global uchar *results)
{
  const int gid = get_global_id(0);
  const uint identity_length_snd_block = identity_length-64;

  u32 hashstring[32];
  
  for (int i=0; i<64; i++) {
    ((uchar*)hashstring)[i] = identity[i];
  }

  // we hash the first block 
  u32 digest1[5];
  digest1[0] = SHA1M_A;
  digest1[1] = SHA1M_B;
  digest1[2] = SHA1M_C;
  digest1[3] = SHA1M_D;
  digest1[4] = SHA1M_E;
  for (int j = 0; j<16; j++) {
    hashstring[j] = swap_uint(((u32*)hashstring)[j]);
  }
  sha1_64(hashstring, digest1);

  for (int i=0; i<identity_length_snd_block; i++) {
    ((uchar*)hashstring)[i] = identity[i+64];
  }

  for (int i=identity_length_snd_block; i<128; i++) {
    ((uchar*)hashstring)[i] = 0;
  }
   
  for (int j = 0; j<identity_length_snd_block/4; j++) {
    hashstring[j] = swap_uint(hashstring[j]);
  }

  ulong currentcounter = startcounter+gid*iterations;
  u32 digest2[5];

  ulong counterlen = countertostring(((uchar*)hashstring)+identity_length_snd_block,
                                     currentcounter);
  ((uchar*)hashstring)[identity_length_snd_block+counterlen] = 0x80;

  *((ulong*)(((uchar*)hashstring)+64+56)) = swap_ulong(8*((ulong)identity_length+counterlen));

  // we bring the first word of the second block to 32bit big endian form
  // note that this can only be a zero word, or 0x80,0x00,0x00,0x00
  hashstring[16] = swap_uint(hashstring[16]);

  // we bring the length once to 32bit big endian form
  hashstring[16+14] = swap_uint(hashstring[16+14]);
  hashstring[16+15] = swap_uint(hashstring[16+15]);

  const int swapendianness_start = identity_length_snd_block/4;
  u32 targetdigest[5];
  compute_targetdigest(targetdigest, targetdifficulty);
  // we swap it now, to avoid swapping in the loop
  for (int j=0; j<5; j++) { targetdigest[j] = swap_uint(targetdigest[j]); }

  bool target_found = false;

  for (ulong it = 0; it<iterations; it++) {
    for (int j=0; j<5; j++) { digest2[j] = digest1[j]; }

    for (int j = swapendianness_start; j<16; j++) {
      hashstring[j] = swap_uint(hashstring[j]);
    }

    sha1_64(hashstring, digest2);
    sha1_64(((u32*)hashstring)+16, digest2);

    // we only consider security levels 32 or larger
    // (this gives us a slight performance boost)
    if (digest2[0]==0) {
      target_found |= 0 == ((digest2[0] & targetdigest[0]) |
                            (digest2[1] & targetdigest[1]) |
                            (digest2[2] & targetdigest[2]) |
                            (digest2[3] & targetdigest[3]) |
                            (digest2[4] & targetdigest[4]));
    }

    // we swap the endianness back to be able to increase the counter
    for (int j = swapendianness_start; j<16; j++) {
      hashstring[j] = swap_uint(hashstring[j]);
    }

    increaseStringCounter(((uchar*)hashstring)+identity_length_snd_block+counterlen-1);
  }
  results[gid] = target_found;
}
)";

#endif
