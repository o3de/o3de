// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#ifndef HASH_H_
#define HASH_H_

/*
--------------------------------------------------------------------
mix -- mix 3 64-bit values reversibly.
mix() takes 48 machine instructions, but only 24 cycles on a superscalar
  machine (like Intel's new MMX architecture).  It requires 4 64-bit
  registers for 4::2 parallelism.
All 1-bit deltas, all 2-bit deltas, all deltas composed of top bits of
  (a,b,c), and all deltas of bottom bits were tested.  All deltas were
  tested both on random keys and on keys that were nearly all zero.
  These deltas all cause every bit of c to change between 1/3 and 2/3
  of the time (well, only 113/400 to 287/400 of the time for some
  2-bit delta).  These deltas all cause at least 80 bits to change
  among (a,b,c) when the mix is run either forward or backward (yes it
  is reversible).
This implies that a hash using mix64 has no funnels.  There may be
  characteristics with 3-bit deltas or bigger, I didn't test for
  those.
--------------------------------------------------------------------
*/
#define mix64(a,b,c) \
{ \
  a -= b; a -= c; a ^= (c>>43); \
  b -= c; b -= a; b ^= (a<<9); \
  c -= a; c -= b; c ^= (b>>8); \
  a -= b; a -= c; a ^= (c>>38); \
  b -= c; b -= a; b ^= (a<<23); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>35); \
  b -= c; b -= a; b ^= (a<<49); \
  c -= a; c -= b; c ^= (b>>11); \
  a -= b; a -= c; a ^= (c>>12); \
  b -= c; b -= a; b ^= (a<<18); \
  c -= a; c -= b; c ^= (b>>22); \
}

/*
--------------------------------------------------------------------
hash64() -- hash a variable-length key into a 64-bit value
  k     : the key (the unaligned variable-length array of bytes)
  len   : the length of the key, counting by bytes
  level : can be any 8-byte value
Returns a 64-bit value.  Every bit of the key affects every bit of
the return value.  No funnels.  Every 1-bit and 2-bit delta achieves
avalanche.  About 41+5len instructions.

The best hash table sizes are powers of 2.  There is no need to do
mod a prime (mod is sooo slow!).  If you need less than 64 bits,
use a bitmask.  For example, if you need only 10 bits, do
  h = (h & hashmask(10));
In which case, the hash table should have hashsize(10) elements.

If you are hashing n strings (ub1 **)k, do it like this:
  for (i=0, h=0; i<n; ++i) h = hash( k[i], len[i], h);

By Bob Jenkins, Jan 4 1997.  bob_jenkins@burtleburtle.net.  You may
use this code any way you wish, private, educational, or commercial,
but I would appreciate if you give me credit.

See http://burtleburtle.net/bob/hash/evahash.html
Use for hash table lookup, or anything where one collision in 2^^64
is acceptable.  Do NOT use for cryptographic purposes.
--------------------------------------------------------------------
*/

static uint64_t hash64( const uint8_t *k, uint32_t length, uint64_t initval )
{
  uint64_t a,b,c,len;

  /* Set up the internal state */
  len = length;
  a = b = initval;                         /* the previous hash value */
  c = 0x9e3779b97f4a7c13LL; /* the golden ratio; an arbitrary value */

  /*---------------------------------------- handle most of the key */
  while (len >= 24)
  {
    a += (k[0]        +((uint64_t)k[ 1]<< 8)+((uint64_t)k[ 2]<<16)+((uint64_t)k[ 3]<<24)
     +((uint64_t)k[4 ]<<32)+((uint64_t)k[ 5]<<40)+((uint64_t)k[ 6]<<48)+((uint64_t)k[ 7]<<56));
    b += (k[8]        +((uint64_t)k[ 9]<< 8)+((uint64_t)k[10]<<16)+((uint64_t)k[11]<<24)
     +((uint64_t)k[12]<<32)+((uint64_t)k[13]<<40)+((uint64_t)k[14]<<48)+((uint64_t)k[15]<<56));
    c += (k[16]       +((uint64_t)k[17]<< 8)+((uint64_t)k[18]<<16)+((uint64_t)k[19]<<24)
     +((uint64_t)k[20]<<32)+((uint64_t)k[21]<<40)+((uint64_t)k[22]<<48)+((uint64_t)k[23]<<56));
    mix64(a,b,c);
    k += 24; len -= 24;
  }

  /*------------------------------------- handle the last 23 bytes */
  c += length;
  switch(len)              /* all the case statements fall through */
  {
  case 23: c+=((uint64_t)k[22]<<56);
  case 22: c+=((uint64_t)k[21]<<48);
  case 21: c+=((uint64_t)k[20]<<40);
  case 20: c+=((uint64_t)k[19]<<32);
  case 19: c+=((uint64_t)k[18]<<24);
  case 18: c+=((uint64_t)k[17]<<16);
  case 17: c+=((uint64_t)k[16]<<8);
    /* the first byte of c is reserved for the length */
  case 16: b+=((uint64_t)k[15]<<56);
  case 15: b+=((uint64_t)k[14]<<48);
  case 14: b+=((uint64_t)k[13]<<40);
  case 13: b+=((uint64_t)k[12]<<32);
  case 12: b+=((uint64_t)k[11]<<24);
  case 11: b+=((uint64_t)k[10]<<16);
  case 10: b+=((uint64_t)k[ 9]<<8);
  case  9: b+=((uint64_t)k[ 8]);
  case  8: a+=((uint64_t)k[ 7]<<56);
  case  7: a+=((uint64_t)k[ 6]<<48);
  case  6: a+=((uint64_t)k[ 5]<<40);
  case  5: a+=((uint64_t)k[ 4]<<32);
  case  4: a+=((uint64_t)k[ 3]<<24);
  case  3: a+=((uint64_t)k[ 2]<<16);
  case  2: a+=((uint64_t)k[ 1]<<8);
  case  1: a+=((uint64_t)k[ 0]);
    /* case 0: nothing left to add */
  }
  mix64(a,b,c);
  /*-------------------------------------------- report the result */
  return c;
}

#endif
