/*
 * This source file is part of the bstring string library.  This code was
 * written by Paul Hsieh in 2002-2010, and is covered by either the 3-clause 
 * BSD open source license or GPL v2.0. Refer to the accompanying documentation 
 * for details on usage and license.
 */
// Modifications copyright Amazon.com, Inc. or its affiliates

/*
 * bstrlib.c
 *
 * This file is the core module for implementing the bstring functions.
 */

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "bstrlib.h"
#include "../internal_includes/hlslcc_malloc.h"

/* Optionally include a mechanism for debugging memory */

#if defined(MEMORY_DEBUG) || defined(BSTRLIB_MEMORY_DEBUG)
#include "memdbg.h"
#endif

#ifndef bstr__alloc
#define bstr__alloc(x) malloc (x)
#endif

#ifndef bstr__free
#define bstr__free(p) free (p)
#endif

#ifndef bstr__realloc
#define bstr__realloc(p,x) realloc ((p), (x))
#endif

#ifndef bstr__memcpy
#define bstr__memcpy(d,s,l) memcpy ((d), (s), (l))
#endif

#ifndef bstr__memmove
#define bstr__memmove(d,s,l) memmove ((d), (s), (l))
#endif

#ifndef bstr__memset
#define bstr__memset(d,c,l) memset ((d), (c), (l))
#endif

#ifndef bstr__memcmp
#define bstr__memcmp(d,c,l) memcmp ((d), (c), (l))
#endif

#ifndef bstr__memchr
#define bstr__memchr(s,c,l) memchr ((s), (c), (l))
#endif

/* Just a length safe wrapper for memmove. */

#define bBlockCopy(D,S,L) { if ((L) > 0) bstr__memmove ((D),(S),(L)); }

/* Compute the snapped size for a given requested size.  By snapping to powers
   of 2 like this, repeated reallocations are avoided. */
static int snapUpSize (int i) {
    if (i < 8) {
        i = 8;
    } else {
        unsigned int j;
        j = (unsigned int) i;

        j |= (j >>  1);
        j |= (j >>  2);
        j |= (j >>  4);
        j |= (j >>  8);        /* Ok, since int >= 16 bits */
#if (UINT_MAX != 0xffff)
        j |= (j >> 16);        /* For 32 bit int systems */
#if (UINT_MAX > 0xffffffffUL)
        j |= (j >> 32);        /* For 64 bit int systems */
#endif
#endif
        /* Least power of two greater than i */
        j++;
        if ((int) j >= i) i = (int) j;
    }
    return i;
}

/*  int balloc (bstring b, int len)
 *
 *  Increase the size of the memory backing the bstring b to at least len.
 */
int balloc (bstring b, int olen) {
    int len;
    if (b == NULL || b->data == NULL || b->slen < 0 || b->mlen <= 0 || 
        b->mlen < b->slen || olen <= 0) {
        return BSTR_ERR;
    }

    if (olen >= b->mlen) {
        unsigned char * x;

        if ((len = snapUpSize (olen)) <= b->mlen) return BSTR_OK;

        /* Assume probability of a non-moving realloc is 0.125 */
        if (7 * b->mlen < 8 * b->slen) {

            /* If slen is close to mlen in size then use realloc to reduce
               the memory defragmentation */

            reallocStrategy:;

            x = (unsigned char *) bstr__realloc (b->data, (size_t) len);
            if (x == NULL) {

                /* Since we failed, try allocating the tighest possible 
                   allocation */

                if (NULL == (x = (unsigned char *) bstr__realloc (b->data, (size_t) (len = olen)))) {
                    return BSTR_ERR;
                }
            }
        } else {

            /* If slen is not close to mlen then avoid the penalty of copying
               the extra bytes that are allocated, but not considered part of
               the string */

            if (NULL == (x = (unsigned char *) bstr__alloc ((size_t) len))) {

                /* Perhaps there is no available memory for the two 
                   allocations to be in memory at once */

                goto reallocStrategy;

            } else {
                if (b->slen) bstr__memcpy ((char *) x, (char *) b->data, (size_t) b->slen);
                bstr__free (b->data);
            }
        }
        b->data = x;
        b->mlen = len;
        b->data[b->slen] = (unsigned char) '\0';
    }

    return BSTR_OK;
}

/*  int ballocmin (bstring b, int len)
 *
 *  Set the size of the memory backing the bstring b to len or b->slen+1,
 *  whichever is larger.  Note that repeated use of this function can degrade
 *  performance.
 */
int ballocmin (bstring b, int len) {
    unsigned char * s;

    if (b == NULL || b->data == NULL || (b->slen+1) < 0 || b->mlen <= 0 || 
        b->mlen < b->slen || len <= 0) {
        return BSTR_ERR;
    }

    if (len < b->slen + 1) len = b->slen + 1;

    if (len != b->mlen) {
        s = (unsigned char *) bstr__realloc (b->data, (size_t) len);
        if (NULL == s) return BSTR_ERR;
        s[b->slen] = (unsigned char) '\0';
        b->data = s;
        b->mlen = len;
    }

    return BSTR_OK;
}

/*  bstring bfromcstr (const char * str)
 *
 *  Create a bstring which contains the contents of the '\0' terminated char *
 *  buffer str.
 */
bstring bfromcstr (const char * str) {
bstring b;
int i;
size_t j;

    if (str == NULL) return NULL;
    j = (strlen) (str);
    i = snapUpSize ((int) (j + (2 - (j != 0))));
    if (i <= (int) j) return NULL;

    b = (bstring) bstr__alloc (sizeof (struct tagbstring));
    if (NULL == b) return NULL;
    b->slen = (int) j;
    if (NULL == (b->data = (unsigned char *) bstr__alloc (b->mlen = i))) {
        bstr__free (b);
        return NULL;
    }

    bstr__memcpy (b->data, str, j+1);
    return b;
}

/*  bstring bfromcstralloc (int mlen, const char * str)
 *
 *  Create a bstring which contains the contents of the '\0' terminated char *
 *  buffer str.  The memory buffer backing the string is at least len 
 *  characters in length.
 */
bstring bfromcstralloc (int mlen, const char * str) {
bstring b;
int i;
size_t j;

    if (str == NULL) return NULL;
    j = (strlen) (str);
    i = snapUpSize ((int) (j + (2 - (j != 0))));
    if (i <= (int) j) return NULL;

    b = (bstring) bstr__alloc (sizeof (struct tagbstring));
    if (b == NULL) return NULL;
    b->slen = (int) j;
    if (i < mlen) i = mlen;

    if (NULL == (b->data = (unsigned char *) bstr__alloc (b->mlen = i))) {
        bstr__free (b);
        return NULL;
    }

    bstr__memcpy (b->data, str, j+1);
    return b;
}

/*  bstring blk2bstr (const void * blk, int len)
 *
 *  Create a bstring which contains the content of the block blk of length 
 *  len.
 */
bstring blk2bstr (const void * blk, int len) {
bstring b;
int i;

    if (blk == NULL || len < 0) return NULL;
    b = (bstring) bstr__alloc (sizeof (struct tagbstring));
    if (b == NULL) return NULL;
    b->slen = len;

    i = len + (2 - (len != 0));
    i = snapUpSize (i);

    b->mlen = i;

    b->data = (unsigned char *) bstr__alloc ((size_t) b->mlen);
    if (b->data == NULL) {
        bstr__free (b);
        return NULL;
    }

    if (len > 0) bstr__memcpy (b->data, blk, (size_t) len);
    b->data[len] = (unsigned char) '\0';

    return b;
}

/*  char * bstr2cstr (const_bstring s, char z)
 *
 *  Create a '\0' terminated char * buffer which is equal to the contents of 
 *  the bstring s, except that any contained '\0' characters are converted 
 *  to the character in z. This returned value should be freed with a 
 *  bcstrfree () call, by the calling application.
 */
char * bstr2cstr (const_bstring b, char z) {
int i, l;
char * r;

    if (b == NULL || b->slen < 0 || b->data == NULL) return NULL;
    l = b->slen;
    r = (char *) bstr__alloc ((size_t) (l + 1));
    if (r == NULL) return r;

    for (i=0; i < l; i ++) {
        r[i] = (char) ((b->data[i] == '\0') ? z : (char) (b->data[i]));
    }

    r[l] = (unsigned char) '\0';

    return r;
}

/*  int bcstrfree (char * s)
 *
 *  Frees a C-string generated by bstr2cstr ().  This is normally unnecessary
 *  since it just wraps a call to bstr__free (), however, if bstr__alloc () 
 *  and bstr__free () have been redefined as a macros within the bstrlib 
 *  module (via defining them in memdbg.h after defining 
 *  BSTRLIB_MEMORY_DEBUG) with some difference in behaviour from the std 
 *  library functions, then this allows a correct way of freeing the memory 
 *  that allows higher level code to be independent from these macro 
 *  redefinitions.
 */
int bcstrfree (char * s) {
    if (s) {
        bstr__free (s);
        return BSTR_OK;
    }
    return BSTR_ERR;
}

/*  int bconcat (bstring b0, const_bstring b1)
 *
 *  Concatenate the bstring b1 to the bstring b0.
 */
int bconcat (bstring b0, const_bstring b1) {
int len, d;
bstring aux = (bstring) b1;

    if (b0 == NULL || b1 == NULL || b0->data == NULL || b1->data == NULL) return BSTR_ERR;

    d = b0->slen;
    len = b1->slen;
    if ((d | (b0->mlen - d) | len | (d + len)) < 0) return BSTR_ERR;

    if (b0->mlen <= d + len + 1) {
        ptrdiff_t pd = b1->data - b0->data;
        if (0 <= pd && pd < b0->mlen) {
            if (NULL == (aux = bstrcpy (b1))) return BSTR_ERR;
        }
        if (balloc (b0, d + len + 1) != BSTR_OK) {
            if (aux != b1) bdestroy (aux);
            return BSTR_ERR;
        }
    }

    bBlockCopy (&b0->data[d], &aux->data[0], (size_t) len);
    b0->data[d + len] = (unsigned char) '\0';
    b0->slen = d + len;
    if (aux != b1) bdestroy (aux);
    return BSTR_OK;
}

/*  int bconchar (bstring b, char c)
/ *
 *  Concatenate the single character c to the bstring b.
 */
int bconchar (bstring b, char c) {
int d;

    if (b == NULL) return BSTR_ERR;
    d = b->slen;
    if ((d | (b->mlen - d)) < 0 || balloc (b, d + 2) != BSTR_OK) return BSTR_ERR;
    b->data[d] = (unsigned char) c;
    b->data[d + 1] = (unsigned char) '\0';
    b->slen++;
    return BSTR_OK;
}

/*  int bcatcstr (bstring b, const char * s)
 *
 *  Concatenate a char * string to a bstring.
 */
int bcatcstr (bstring b, const char * s) {
char * d;
int i, l;

    if (b == NULL || b->data == NULL || b->slen < 0 || b->mlen < b->slen
     || b->mlen <= 0 || s == NULL) return BSTR_ERR;

    /* Optimistically concatenate directly */
    l = b->mlen - b->slen;
    d = (char *) &b->data[b->slen];
    for (i=0; i < l; i++) {
        if ((*d++ = *s++) == '\0') {
            b->slen += i;
            return BSTR_OK;
        }
    }
    b->slen += i;

    /* Need to explicitely resize and concatenate tail */
    return bcatblk (b, (const void *) s, (int) strlen (s));
}

/*  int bcatblk (bstring b, const void * s, int len)
 *
 *  Concatenate a fixed length buffer to a bstring.
 */
int bcatblk (bstring b, const void * s, int len) {
int nl;

    if (b == NULL || b->data == NULL || b->slen < 0 || b->mlen < b->slen
     || b->mlen <= 0 || s == NULL || len < 0) return BSTR_ERR;

    if (0 > (nl = b->slen + len)) return BSTR_ERR; /* Overflow? */
    if (b->mlen <= nl && 0 > balloc (b, nl + 1)) return BSTR_ERR;

    bBlockCopy (&b->data[b->slen], s, (size_t) len);
    b->slen = nl;
    b->data[nl] = (unsigned char) '\0';
    return BSTR_OK;
}

/*  bstring bstrcpy (const_bstring b)
 *
 *  Create a copy of the bstring b.
 */
bstring bstrcpy (const_bstring b) {
bstring b0;
int i,j;

    /* Attempted to copy an invalid string? */
    if (b == NULL || b->slen < 0 || b->data == NULL) return NULL;

    b0 = (bstring) bstr__alloc (sizeof (struct tagbstring));
    if (b0 == NULL) {
        /* Unable to allocate memory for string header */
        return NULL;
    }

    i = b->slen;
    j = snapUpSize (i + 1);

    b0->data = (unsigned char *) bstr__alloc (j);
    if (b0->data == NULL) {
        j = i + 1;
        b0->data = (unsigned char *) bstr__alloc (j);
        if (b0->data == NULL) {
            /* Unable to allocate memory for string data */
            bstr__free (b0);
            return NULL;
        }
    }

    b0->mlen = j;
    b0->slen = i;

    if (i) bstr__memcpy ((char *) b0->data, (char *) b->data, i);
    b0->data[b0->slen] = (unsigned char) '\0';

    return b0;
}

/*  int bassign (bstring a, const_bstring b)
 *
 *  Overwrite the string a with the contents of string b.
 */
int bassign (bstring a, const_bstring b) {
    if (b == NULL || b->data == NULL || b->slen < 0)
        return BSTR_ERR;
    if (b->slen != 0) {
        if (balloc (a, b->slen) != BSTR_OK) return BSTR_ERR;
        bstr__memmove (a->data, b->data, b->slen);
    } else {
        if (a == NULL || a->data == NULL || a->mlen < a->slen || 
            a->slen < 0 || a->mlen == 0) 
            return BSTR_ERR;
    }
    a->data[b->slen] = (unsigned char) '\0';
    a->slen = b->slen;
    return BSTR_OK;
}

/*  int bassignmidstr (bstring a, const_bstring b, int left, int len)
 *
 *  Overwrite the string a with the middle of contents of string b 
 *  starting from position left and running for a length len.  left and 
 *  len are clamped to the ends of b as with the function bmidstr.
 */
int bassignmidstr (bstring a, const_bstring b, int left, int len) {
    if (b == NULL || b->data == NULL || b->slen < 0)
        return BSTR_ERR;

    if (left < 0) {
        len += left;
        left = 0;
    }

    if (len > b->slen - left) len = b->slen - left;

    if (a == NULL || a->data == NULL || a->mlen < a->slen ||
        a->slen < 0 || a->mlen == 0)
        return BSTR_ERR;

    if (len > 0) {
        if (balloc (a, len) != BSTR_OK) return BSTR_ERR;
        bstr__memmove (a->data, b->data + left, len);
        a->slen = len;
    } else {
        a->slen = 0;
    }
    a->data[a->slen] = (unsigned char) '\0';
    return BSTR_OK;
}

/*  int bassigncstr (bstring a, const char * str)
 *
 *  Overwrite the string a with the contents of char * string str.  Note that 
 *  the bstring a must be a well defined and writable bstring.  If an error 
 *  occurs BSTR_ERR is returned however a may be partially overwritten.
 */
int bassigncstr (bstring a, const char * str) {
int i;
size_t len;
    if (a == NULL || a->data == NULL || a->mlen < a->slen ||
        a->slen < 0 || a->mlen == 0 || NULL == str) 
        return BSTR_ERR;

    for (i=0; i < a->mlen; i++) {
        if ('\0' == (a->data[i] = str[i])) {
            a->slen = i;
            return BSTR_OK;
        }
    }

    a->slen = i;
    len = strlen (str + i);
    if (len > INT_MAX || i + len + 1 > INT_MAX ||
        0 > balloc (a, (int) (i + len + 1))) return BSTR_ERR;
    bBlockCopy (a->data + i, str + i, (size_t) len + 1);
    a->slen += (int) len;
    return BSTR_OK;
}

/*  int bassignblk (bstring a, const void * s, int len)
 *
 *  Overwrite the string a with the contents of the block (s, len).  Note that 
 *  the bstring a must be a well defined and writable bstring.  If an error 
 *  occurs BSTR_ERR is returned and a is not overwritten.
 */
int bassignblk (bstring a, const void * s, int len) {
    if (a == NULL || a->data == NULL || a->mlen < a->slen ||
        a->slen < 0 || a->mlen == 0 || NULL == s || len + 1 < 1) 
        return BSTR_ERR;
    if (len + 1 > a->mlen && 0 > balloc (a, len + 1)) return BSTR_ERR;
    bBlockCopy (a->data, s, (size_t) len);
    a->data[len] = (unsigned char) '\0';
    a->slen = len;
    return BSTR_OK;
}

/*  int btrunc (bstring b, int n)
 *
 *  Truncate the bstring to at most n characters.
 */
int btrunc (bstring b, int n) {
    if (n < 0 || b == NULL || b->data == NULL || b->mlen < b->slen ||
        b->slen < 0 || b->mlen <= 0) return BSTR_ERR;
    if (b->slen > n) {
        b->slen = n;
        b->data[n] = (unsigned char) '\0';
    }
    return BSTR_OK;
}

#define   upcase(c) (toupper ((unsigned char) c))
#define downcase(c) (tolower ((unsigned char) c))
#define   wspace(c) (isspace ((unsigned char) c))

/*  int btoupper (bstring b)
 *
 *  Convert contents of bstring to upper case.
 */
int btoupper (bstring b) {
int i, len;
    if (b == NULL || b->data == NULL || b->mlen < b->slen ||
        b->slen < 0 || b->mlen <= 0) return BSTR_ERR;
    for (i=0, len = b->slen; i < len; i++) {
        b->data[i] = (unsigned char) upcase (b->data[i]);
    }
    return BSTR_OK;
}

/*  int btolower (bstring b)
 *
 *  Convert contents of bstring to lower case.
 */
int btolower (bstring b) {
int i, len;
    if (b == NULL || b->data == NULL || b->mlen < b->slen ||
        b->slen < 0 || b->mlen <= 0) return BSTR_ERR;
    for (i=0, len = b->slen; i < len; i++) {
        b->data[i] = (unsigned char) downcase (b->data[i]);
    }
    return BSTR_OK;
}

/*  int bstricmp (const_bstring b0, const_bstring b1)
 *
 *  Compare two strings without differentiating between case.  The return 
 *  value is the difference of the values of the characters where the two 
 *  strings first differ after lower case transformation, otherwise 0 is 
 *  returned indicating that the strings are equal.  If the lengths are 
 *  different, then a difference from 0 is given, but if the first extra 
 *  character is '\0', then it is taken to be the value UCHAR_MAX+1.
 */
int bstricmp (const_bstring b0, const_bstring b1) {
int i, v, n;

    if (bdata (b0) == NULL || b0->slen < 0 || 
        bdata (b1) == NULL || b1->slen < 0) return SHRT_MIN;
    if ((n = b0->slen) > b1->slen) n = b1->slen;
    else if (b0->slen == b1->slen && b0->data == b1->data) return BSTR_OK;

    for (i = 0; i < n; i ++) {
        v  = (char) downcase (b0->data[i])
           - (char) downcase (b1->data[i]);
        if (0 != v) return v;
    }

    if (b0->slen > n) {
        v = (char) downcase (b0->data[n]);
        if (v) return v;
        return UCHAR_MAX + 1;
    }
    if (b1->slen > n) {
        v = - (char) downcase (b1->data[n]);
        if (v) return v;
        return - (int) (UCHAR_MAX + 1);
    }
    return BSTR_OK;
}

/*  int bstrnicmp (const_bstring b0, const_bstring b1, int n)
 *
 *  Compare two strings without differentiating between case for at most n
 *  characters.  If the position where the two strings first differ is
 *  before the nth position, the return value is the difference of the values
 *  of the characters, otherwise 0 is returned.  If the lengths are different
 *  and less than n characters, then a difference from 0 is given, but if the 
 *  first extra character is '\0', then it is taken to be the value 
 *  UCHAR_MAX+1.
 */
int bstrnicmp (const_bstring b0, const_bstring b1, int n) {
int i, v, m;

    if (bdata (b0) == NULL || b0->slen < 0 || 
        bdata (b1) == NULL || b1->slen < 0 || n < 0) return SHRT_MIN;
    m = n;
    if (m > b0->slen) m = b0->slen;
    if (m > b1->slen) m = b1->slen;

    if (b0->data != b1->data) {
        for (i = 0; i < m; i ++) {
            v  = (char) downcase (b0->data[i]);
            v -= (char) downcase (b1->data[i]);
            if (v != 0) return b0->data[i] - b1->data[i];
        }
    }

    if (n == m || b0->slen == b1->slen) return BSTR_OK;

    if (b0->slen > m) {
        v = (char) downcase (b0->data[m]);
        if (v) return v;
        return UCHAR_MAX + 1;
    }

    v = - (char) downcase (b1->data[m]);
    if (v) return v;
    return - (int) (UCHAR_MAX + 1);
}

/*  int biseqcaseless (const_bstring b0, const_bstring b1)
 *
 *  Compare two strings for equality without differentiating between case.  
 *  If the strings differ other than in case, 0 is returned, if the strings 
 *  are the same, 1 is returned, if there is an error, -1 is returned.  If 
 *  the length of the strings are different, this function is O(1).  '\0' 
 *  termination characters are not treated in any special way.
 */
int biseqcaseless (const_bstring b0, const_bstring b1) {
int i, n;

    if (bdata (b0) == NULL || b0->slen < 0 || 
        bdata (b1) == NULL || b1->slen < 0) return BSTR_ERR;
    if (b0->slen != b1->slen) return BSTR_OK;
    if (b0->data == b1->data || b0->slen == 0) return 1;
    for (i=0, n=b0->slen; i < n; i++) {
        if (b0->data[i] != b1->data[i]) {
            unsigned char c = (unsigned char) downcase (b0->data[i]);
            if (c != (unsigned char) downcase (b1->data[i])) return 0;
        }
    }
    return 1;
}

/*  int bisstemeqcaselessblk (const_bstring b0, const void * blk, int len)
 *
 *  Compare beginning of string b0 with a block of memory of length len 
 *  without differentiating between case for equality.  If the beginning of b0
 *  differs from the memory block other than in case (or if b0 is too short), 
 *  0 is returned, if the strings are the same, 1 is returned, if there is an 
 *  error, -1 is returned.  '\0' characters are not treated in any special 
 *  way.
 */
int bisstemeqcaselessblk (const_bstring b0, const void * blk, int len) {
int i;

    if (bdata (b0) == NULL || b0->slen < 0 || NULL == blk || len < 0)
        return BSTR_ERR;
    if (b0->slen < len) return BSTR_OK;
    if (b0->data == (const unsigned char *) blk || len == 0) return 1;

    for (i = 0; i < len; i ++) {
        if (b0->data[i] != ((const unsigned char *) blk)[i]) {
            if (downcase (b0->data[i]) != 
                downcase (((const unsigned char *) blk)[i])) return 0;
        }
    }
    return 1;
}

/*
 * int bltrimws (bstring b)
 *
 * Delete whitespace contiguous from the left end of the string.
 */
int bltrimws (bstring b) {
int i, len;

    if (b == NULL || b->data == NULL || b->mlen < b->slen ||
        b->slen < 0 || b->mlen <= 0) return BSTR_ERR;

    for (len = b->slen, i = 0; i < len; i++) {
        if (!wspace (b->data[i])) {
            return bdelete (b, 0, i);
        }
    }

    b->data[0] = (unsigned char) '\0';
    b->slen = 0;
    return BSTR_OK;
}

/*
 * int brtrimws (bstring b)
 *
 * Delete whitespace contiguous from the right end of the string.
 */
int brtrimws (bstring b) {
int i;

    if (b == NULL || b->data == NULL || b->mlen < b->slen ||
        b->slen < 0 || b->mlen <= 0) return BSTR_ERR;

    for (i = b->slen - 1; i >= 0; i--) {
        if (!wspace (b->data[i])) {
            if (b->mlen > i) b->data[i+1] = (unsigned char) '\0';
            b->slen = i + 1;
            return BSTR_OK;
        }
    }

    b->data[0] = (unsigned char) '\0';
    b->slen = 0;
    return BSTR_OK;
}

/*
 * int btrimws (bstring b)
 *
 * Delete whitespace contiguous from both ends of the string.
 */
int btrimws (bstring b) {
int i, j;

    if (b == NULL || b->data == NULL || b->mlen < b->slen ||
        b->slen < 0 || b->mlen <= 0) return BSTR_ERR;

    for (i = b->slen - 1; i >= 0; i--) {
        if (!wspace (b->data[i])) {
            if (b->mlen > i) b->data[i+1] = (unsigned char) '\0';
            b->slen = i + 1;
            for (j = 0; wspace (b->data[j]); j++) {}
            return bdelete (b, 0, j);
        }
    }

    b->data[0] = (unsigned char) '\0';
    b->slen = 0;
    return BSTR_OK;
}

/*  int biseq (const_bstring b0, const_bstring b1)
 *
 *  Compare the string b0 and b1.  If the strings differ, 0 is returned, if 
 *  the strings are the same, 1 is returned, if there is an error, -1 is 
 *  returned.  If the length of the strings are different, this function is
 *  O(1).  '\0' termination characters are not treated in any special way.
 */
int biseq (const_bstring b0, const_bstring b1) {
    if (b0 == NULL || b1 == NULL || b0->data == NULL || b1->data == NULL ||
        b0->slen < 0 || b1->slen < 0) return BSTR_ERR;
    if (b0->slen != b1->slen) return BSTR_OK;
    if (b0->data == b1->data || b0->slen == 0) return 1;
    return !bstr__memcmp (b0->data, b1->data, b0->slen);
}

/*  int bisstemeqblk (const_bstring b0, const void * blk, int len)
 *
 *  Compare beginning of string b0 with a block of memory of length len for 
 *  equality.  If the beginning of b0 differs from the memory block (or if b0 
 *  is too short), 0 is returned, if the strings are the same, 1 is returned, 
 *  if there is an error, -1 is returned.  '\0' characters are not treated in 
 *  any special way.
 */
int bisstemeqblk (const_bstring b0, const void * blk, int len) {
int i;

    if (bdata (b0) == NULL || b0->slen < 0 || NULL == blk || len < 0)
        return BSTR_ERR;
    if (b0->slen < len) return BSTR_OK;
    if (b0->data == (const unsigned char *) blk || len == 0) return 1;

    for (i = 0; i < len; i ++) {
        if (b0->data[i] != ((const unsigned char *) blk)[i]) return BSTR_OK;
    }
    return 1;
}

/*  int biseqcstr (const_bstring b, const char *s)
 *
 *  Compare the bstring b and char * string s.  The C string s must be '\0' 
 *  terminated at exactly the length of the bstring b, and the contents 
 *  between the two must be identical with the bstring b with no '\0' 
 *  characters for the two contents to be considered equal.  This is 
 *  equivalent to the condition that their current contents will be always be 
 *  equal when comparing them in the same format after converting one or the 
 *  other.  If the strings are equal 1 is returned, if they are unequal 0 is 
 *  returned and if there is a detectable error BSTR_ERR is returned.
 */
int biseqcstr (const_bstring b, const char * s) {
int i;
    if (b == NULL || s == NULL || b->data == NULL || b->slen < 0) return BSTR_ERR;
    for (i=0; i < b->slen; i++) {
        if (s[i] == '\0' || b->data[i] != (unsigned char) s[i]) return BSTR_OK;
    }
    return s[i] == '\0';
}

/*  int biseqcstrcaseless (const_bstring b, const char *s)
 *
 *  Compare the bstring b and char * string s.  The C string s must be '\0' 
 *  terminated at exactly the length of the bstring b, and the contents 
 *  between the two must be identical except for case with the bstring b with 
 *  no '\0' characters for the two contents to be considered equal.  This is 
 *  equivalent to the condition that their current contents will be always be 
 *  equal ignoring case when comparing them in the same format after 
 *  converting one or the other.  If the strings are equal, except for case, 
 *  1 is returned, if they are unequal regardless of case 0 is returned and 
 *  if there is a detectable error BSTR_ERR is returned.
 */
int biseqcstrcaseless (const_bstring b, const char * s) {
int i;
    if (b == NULL || s == NULL || b->data == NULL || b->slen < 0) return BSTR_ERR;
    for (i=0; i < b->slen; i++) {
        if (s[i] == '\0' || 
            (b->data[i] != (unsigned char) s[i] && 
             downcase (b->data[i]) != (unsigned char) downcase (s[i])))
            return BSTR_OK;
    }
    return s[i] == '\0';
}

/*  int bstrcmp (const_bstring b0, const_bstring b1)
 *
 *  Compare the string b0 and b1.  If there is an error, SHRT_MIN is returned, 
 *  otherwise a value less than or greater than zero, indicating that the 
 *  string pointed to by b0 is lexicographically less than or greater than 
 *  the string pointed to by b1 is returned.  If the the string lengths are 
 *  unequal but the characters up until the length of the shorter are equal 
 *  then a value less than, or greater than zero, indicating that the string 
 *  pointed to by b0 is shorter or longer than the string pointed to by b1 is 
 *  returned.  0 is returned if and only if the two strings are the same.  If 
 *  the length of the strings are different, this function is O(n).  Like its
 *  standard C library counter part strcmp, the comparison does not proceed 
 *  past any '\0' termination characters encountered.
 */
int bstrcmp (const_bstring b0, const_bstring b1) {
int i, v, n;

    if (b0 == NULL || b1 == NULL || b0->data == NULL || b1->data == NULL ||
        b0->slen < 0 || b1->slen < 0) return SHRT_MIN;
    n = b0->slen; if (n > b1->slen) n = b1->slen;
    if (b0->slen == b1->slen && (b0->data == b1->data || b0->slen == 0))
        return BSTR_OK;

    for (i = 0; i < n; i ++) {
        v = ((char) b0->data[i]) - ((char) b1->data[i]);
        if (v != 0) return v;
        if (b0->data[i] == (unsigned char) '\0') return BSTR_OK;
    }

    if (b0->slen > n) return 1;
    if (b1->slen > n) return -1;
    return BSTR_OK;
}

/*  int bstrncmp (const_bstring b0, const_bstring b1, int n)
 *
 *  Compare the string b0 and b1 for at most n characters.  If there is an 
 *  error, SHRT_MIN is returned, otherwise a value is returned as if b0 and 
 *  b1 were first truncated to at most n characters then bstrcmp was called
 *  with these new strings are paremeters.  If the length of the strings are 
 *  different, this function is O(n).  Like its standard C library counter 
 *  part strcmp, the comparison does not proceed past any '\0' termination 
 *  characters encountered.
 */
int bstrncmp (const_bstring b0, const_bstring b1, int n) {
int i, v, m;

    if (b0 == NULL || b1 == NULL || b0->data == NULL || b1->data == NULL ||
        b0->slen < 0 || b1->slen < 0) return SHRT_MIN;
    m = n;
    if (m > b0->slen) m = b0->slen;
    if (m > b1->slen) m = b1->slen;

    if (b0->data != b1->data) {
        for (i = 0; i < m; i ++) {
            v = ((char) b0->data[i]) - ((char) b1->data[i]);
            if (v != 0) return v;
            if (b0->data[i] == (unsigned char) '\0') return BSTR_OK;
        }
    }

    if (n == m || b0->slen == b1->slen) return BSTR_OK;

    if (b0->slen > m) return 1;
    return -1;
}

/*  bstring bmidstr (const_bstring b, int left, int len)
 *
 *  Create a bstring which is the substring of b starting from position left
 *  and running for a length len (clamped by the end of the bstring b.)  If
 *  b is detectably invalid, then NULL is returned.  The section described 
 *  by (left, len) is clamped to the boundaries of b.
 */
bstring bmidstr (const_bstring b, int left, int len) {

    if (b == NULL || b->slen < 0 || b->data == NULL) return NULL;

    if (left < 0) {
        len += left;
        left = 0;
    }

    if (len > b->slen - left) len = b->slen - left;

    if (len <= 0) return bfromcstr ("");
    return blk2bstr (b->data + left, len);
}

/*  int bdelete (bstring b, int pos, int len)
 *
 *  Removes characters from pos to pos+len-1 inclusive and shifts the tail of 
 *  the bstring starting from pos+len to pos.  len must be positive for this 
 *  call to have any effect.  The section of the string described by (pos, 
 *  len) is clamped to boundaries of the bstring b.
 */
int bdelete (bstring b, int pos, int len) {
    /* Clamp to left side of bstring */
    if (pos < 0) {
        len += pos;
        pos = 0;
    }

    if (len < 0 || b == NULL || b->data == NULL || b->slen < 0 || 
        b->mlen < b->slen || b->mlen <= 0) 
        return BSTR_ERR;
    if (len > 0 && pos < b->slen) {
        if (pos + len >= b->slen) {
            b->slen = pos;
        } else {
            bBlockCopy ((char *) (b->data + pos),
                        (char *) (b->data + pos + len), 
                        b->slen - (pos+len));
            b->slen -= len;
        }
        b->data[b->slen] = (unsigned char) '\0';
    }
    return BSTR_OK;
}

/*  int bdestroy (bstring b)
 *
 *  Free up the bstring.  Note that if b is detectably invalid or not writable
 *  then no action is performed and BSTR_ERR is returned.  Like a freed memory
 *  allocation, dereferences, writes or any other action on b after it has 
 *  been bdestroyed is undefined.
 */
int bdestroy (bstring b) {
    if (b == NULL || b->slen < 0 || b->mlen <= 0 || b->mlen < b->slen ||
        b->data == NULL)
        return BSTR_ERR;

    bstr__free (b->data);

    /* In case there is any stale usage, there is one more chance to 
       notice this error. */

    b->slen = -1;
    b->mlen = -__LINE__;
    b->data = NULL;

    bstr__free (b);
    return BSTR_OK;
}

/*  int binstr (const_bstring b1, int pos, const_bstring b2)
 *
 *  Search for the bstring b2 in b1 starting from position pos, and searching 
 *  forward.  If it is found then return with the first position where it is 
 *  found, otherwise return BSTR_ERR.  Note that this is just a brute force 
 *  string searcher that does not attempt clever things like the Boyer-Moore 
 *  search algorithm.  Because of this there are many degenerate cases where 
 *  this can take much longer than it needs to.
 */
int binstr (const_bstring b1, int pos, const_bstring b2) {
int j, ii, ll, lf;
unsigned char * d0;
unsigned char c0;
register unsigned char * d1;
register unsigned char c1;
register int i;

    if (b1 == NULL || b1->data == NULL || b1->slen < 0 ||
        b2 == NULL || b2->data == NULL || b2->slen < 0) return BSTR_ERR;
    if (b1->slen == pos) return (b2->slen == 0)?pos:BSTR_ERR;
    if (b1->slen < pos || pos < 0) return BSTR_ERR;
    if (b2->slen == 0) return pos;

    /* No space to find such a string? */
    if ((lf = b1->slen - b2->slen + 1) <= pos) return BSTR_ERR;

    /* An obvious alias case */
    if (b1->data == b2->data && pos == 0) return 0;

    i = pos;

    d0 = b2->data;
    d1 = b1->data;
    ll = b2->slen;

    /* Peel off the b2->slen == 1 case */
    c0 = d0[0];
    if (1 == ll) {
        for (;i < lf; i++) if (c0 == d1[i]) return i;
        return BSTR_ERR;
    }

    c1 = c0;
    j = 0;
    lf = b1->slen - 1;

    ii = -1;
    if (i < lf) do {
        /* Unrolled current character test */
        if (c1 != d1[i]) {
            if (c1 != d1[1+i]) {
                i += 2;
                continue;
            }
            i++;
        }

        /* Take note if this is the start of a potential match */
        if (0 == j) ii = i;

        /* Shift the test character down by one */
        j++;
        i++;

        /* If this isn't past the last character continue */
        if (j < ll) {
            c1 = d0[j];
            continue;
        }

        N0:;

        /* If no characters mismatched, then we matched */
        if (i == ii+j) return ii;

        /* Shift back to the beginning */
        i -= j;
        j  = 0;
        c1 = c0;
    } while (i < lf);

    /* Deal with last case if unrolling caused a misalignment */
    if (i == lf && ll == j+1 && c1 == d1[i]) goto N0;

    return BSTR_ERR;
}

/*  int binstrr (const_bstring b1, int pos, const_bstring b2)
 *
 *  Search for the bstring b2 in b1 starting from position pos, and searching 
 *  backward.  If it is found then return with the first position where it is 
 *  found, otherwise return BSTR_ERR.  Note that this is just a brute force 
 *  string searcher that does not attempt clever things like the Boyer-Moore 
 *  search algorithm.  Because of this there are many degenerate cases where 
 *  this can take much longer than it needs to.
 */
int binstrr (const_bstring b1, int pos, const_bstring b2) {
int j, i, l;
unsigned char * d0, * d1;

    if (b1 == NULL || b1->data == NULL || b1->slen < 0 ||
        b2 == NULL || b2->data == NULL || b2->slen < 0) return BSTR_ERR;
    if (b1->slen == pos && b2->slen == 0) return pos;
    if (b1->slen < pos || pos < 0) return BSTR_ERR;
    if (b2->slen == 0) return pos;

    /* Obvious alias case */
    if (b1->data == b2->data && pos == 0 && b2->slen <= b1->slen) return 0;

    i = pos;
    if ((l = b1->slen - b2->slen) < 0) return BSTR_ERR;

    /* If no space to find such a string then snap back */
    if (l + 1 <= i) i = l;
    j = 0;

    d0 = b2->data;
    d1 = b1->data;
    l  = b2->slen;

    for (;;) {
        if (d0[j] == d1[i + j]) {
            j ++;
            if (j >= l) return i;
        } else {
            i --;
            if (i < 0) break;
            j=0;
        }
    }

    return BSTR_ERR;
}

/*  int binstrcaseless (const_bstring b1, int pos, const_bstring b2)
 *
 *  Search for the bstring b2 in b1 starting from position pos, and searching 
 *  forward but without regard to case.  If it is found then return with the 
 *  first position where it is found, otherwise return BSTR_ERR.  Note that 
 *  this is just a brute force string searcher that does not attempt clever 
 *  things like the Boyer-Moore search algorithm.  Because of this there are 
 *  many degenerate cases where this can take much longer than it needs to.
 */
int binstrcaseless (const_bstring b1, int pos, const_bstring b2) {
int j, i, l, ll;
unsigned char * d0, * d1;

    if (b1 == NULL || b1->data == NULL || b1->slen < 0 ||
        b2 == NULL || b2->data == NULL || b2->slen < 0) return BSTR_ERR;
    if (b1->slen == pos) return (b2->slen == 0)?pos:BSTR_ERR;
    if (b1->slen < pos || pos < 0) return BSTR_ERR;
    if (b2->slen == 0) return pos;

    l = b1->slen - b2->slen + 1;

    /* No space to find such a string? */
    if (l <= pos) return BSTR_ERR;

    /* An obvious alias case */
    if (b1->data == b2->data && pos == 0) return BSTR_OK;

    i = pos;
    j = 0;

    d0 = b2->data;
    d1 = b1->data;
    ll = b2->slen;

    for (;;) {
        if (d0[j] == d1[i + j] || downcase (d0[j]) == downcase (d1[i + j])) {
            j ++;
            if (j >= ll) return i;
        } else {
            i ++;
            if (i >= l) break;
            j=0;
        }
    }

    return BSTR_ERR;
}

/*  int binstrrcaseless (const_bstring b1, int pos, const_bstring b2)
 *
 *  Search for the bstring b2 in b1 starting from position pos, and searching 
 *  backward but without regard to case.  If it is found then return with the 
 *  first position where it is found, otherwise return BSTR_ERR.  Note that 
 *  this is just a brute force string searcher that does not attempt clever 
 *  things like the Boyer-Moore search algorithm.  Because of this there are 
 *  many degenerate cases where this can take much longer than it needs to.
 */
int binstrrcaseless (const_bstring b1, int pos, const_bstring b2) {
int j, i, l;
unsigned char * d0, * d1;

    if (b1 == NULL || b1->data == NULL || b1->slen < 0 ||
        b2 == NULL || b2->data == NULL || b2->slen < 0) return BSTR_ERR;
    if (b1->slen == pos && b2->slen == 0) return pos;
    if (b1->slen < pos || pos < 0) return BSTR_ERR;
    if (b2->slen == 0) return pos;

    /* Obvious alias case */
    if (b1->data == b2->data && pos == 0 && b2->slen <= b1->slen) return BSTR_OK;

    i = pos;
    if ((l = b1->slen - b2->slen) < 0) return BSTR_ERR;

    /* If no space to find such a string then snap back */
    if (l + 1 <= i) i = l;
    j = 0;

    d0 = b2->data;
    d1 = b1->data;
    l  = b2->slen;

    for (;;) {
        if (d0[j] == d1[i + j] || downcase (d0[j]) == downcase (d1[i + j])) {
            j ++;
            if (j >= l) return i;
        } else {
            i --;
            if (i < 0) break;
            j=0;
        }
    }

    return BSTR_ERR;
}


/*  int bstrchrp (const_bstring b, int c, int pos)
 *
 *  Search for the character c in b forwards from the position pos 
 *  (inclusive).
 */
int bstrchrp (const_bstring b, int c, int pos) {
unsigned char * p;

    if (b == NULL || b->data == NULL || b->slen <= pos || pos < 0) return BSTR_ERR;
    p = (unsigned char *) bstr__memchr ((b->data + pos), (unsigned char) c, (b->slen - pos));
    if (p) return (int) (p - b->data);
    return BSTR_ERR;
}

/*  int bstrrchrp (const_bstring b, int c, int pos)
 *
 *  Search for the character c in b backwards from the position pos in string 
 *  (inclusive).
 */
int bstrrchrp (const_bstring b, int c, int pos) {
int i;
 
    if (b == NULL || b->data == NULL || b->slen <= pos || pos < 0) return BSTR_ERR;
    for (i=pos; i >= 0; i--) {
        if (b->data[i] == (unsigned char) c) return i;
    }
    return BSTR_ERR;
}

#if !defined (BSTRLIB_AGGRESSIVE_MEMORY_FOR_SPEED_TRADEOFF)
#define LONG_LOG_BITS_QTY (3)
#define LONG_BITS_QTY (1 << LONG_LOG_BITS_QTY)
#define LONG_TYPE unsigned char

#define CFCLEN ((1 << CHAR_BIT) / LONG_BITS_QTY)
struct charField { LONG_TYPE content[CFCLEN]; };
#define testInCharField(cf,c) ((cf)->content[(c) >> LONG_LOG_BITS_QTY] & (((long)1) << ((c) & (LONG_BITS_QTY-1))))
#define setInCharField(cf,idx) { \
    unsigned int c = (unsigned int) (idx); \
    (cf)->content[c >> LONG_LOG_BITS_QTY] |= (LONG_TYPE) (1ul << (c & (LONG_BITS_QTY-1))); \
}

#else

#define CFCLEN (1 << CHAR_BIT)
struct charField { unsigned char content[CFCLEN]; };
#define testInCharField(cf,c) ((cf)->content[(unsigned char) (c)])
#define setInCharField(cf,idx) (cf)->content[(unsigned int) (idx)] = ~0

#endif

/* Convert a bstring to charField */
static int buildCharField (struct charField * cf, const_bstring b) {
int i;
    if (b == NULL || b->data == NULL || b->slen <= 0) return BSTR_ERR;
    memset ((void *) cf->content, 0, sizeof (struct charField));
    for (i=0; i < b->slen; i++) {
        setInCharField (cf, b->data[i]);
    }
    return BSTR_OK;
}

static void invertCharField (struct charField * cf) {
int i;
    for (i=0; i < CFCLEN; i++) cf->content[i] = ~cf->content[i];
}

/* Inner engine for binchr */
static int binchrCF (const unsigned char * data, int len, int pos, const struct charField * cf) {
int i;
    for (i=pos; i < len; i++) {
        unsigned char c = (unsigned char) data[i];
        if (testInCharField (cf, c)) return i;
    }
    return BSTR_ERR;
}

/*  int binchr (const_bstring b0, int pos, const_bstring b1);
 *
 *  Search for the first position in b0 starting from pos or after, in which 
 *  one of the characters in b1 is found and return it.  If such a position 
 *  does not exist in b0, then BSTR_ERR is returned.
 */
int binchr (const_bstring b0, int pos, const_bstring b1) {
struct charField chrs;
    if (pos < 0 || b0 == NULL || b0->data == NULL ||
        b0->slen <= pos) return BSTR_ERR;
    if (1 == b1->slen) return bstrchrp (b0, b1->data[0], pos);
    if (0 > buildCharField (&chrs, b1)) return BSTR_ERR;
    return binchrCF (b0->data, b0->slen, pos, &chrs);
}

/* Inner engine for binchrr */
static int binchrrCF (const unsigned char * data, int pos, const struct charField * cf) {
int i;
    for (i=pos; i >= 0; i--) {
        unsigned int c = (unsigned int) data[i];
        if (testInCharField (cf, c)) return i;
    }
    return BSTR_ERR;
}

/*  int binchrr (const_bstring b0, int pos, const_bstring b1);
 *
 *  Search for the last position in b0 no greater than pos, in which one of 
 *  the characters in b1 is found and return it.  If such a position does not 
 *  exist in b0, then BSTR_ERR is returned.
 */
int binchrr (const_bstring b0, int pos, const_bstring b1) {
struct charField chrs;
    if (pos < 0 || b0 == NULL || b0->data == NULL || b1 == NULL ||
        b0->slen < pos) return BSTR_ERR;
    if (pos == b0->slen) pos--;
    if (1 == b1->slen) return bstrrchrp (b0, b1->data[0], pos);
    if (0 > buildCharField (&chrs, b1)) return BSTR_ERR;
    return binchrrCF (b0->data, pos, &chrs);
}

/*  int bninchr (const_bstring b0, int pos, const_bstring b1);
 *
 *  Search for the first position in b0 starting from pos or after, in which 
 *  none of the characters in b1 is found and return it.  If such a position 
 *  does not exist in b0, then BSTR_ERR is returned.
 */
int bninchr (const_bstring b0, int pos, const_bstring b1) {
struct charField chrs;
    if (pos < 0 || b0 == NULL || b0->data == NULL || 
        b0->slen <= pos) return BSTR_ERR;
    if (buildCharField (&chrs, b1) < 0) return BSTR_ERR;
    invertCharField (&chrs);
    return binchrCF (b0->data, b0->slen, pos, &chrs);
}

/*  int bninchrr (const_bstring b0, int pos, const_bstring b1);
 *
 *  Search for the last position in b0 no greater than pos, in which none of 
 *  the characters in b1 is found and return it.  If such a position does not 
 *  exist in b0, then BSTR_ERR is returned.
 */
int bninchrr (const_bstring b0, int pos, const_bstring b1) {
struct charField chrs;
    if (pos < 0 || b0 == NULL || b0->data == NULL || 
        b0->slen < pos) return BSTR_ERR;
    if (pos == b0->slen) pos--;
    if (buildCharField (&chrs, b1) < 0) return BSTR_ERR;
    invertCharField (&chrs);
    return binchrrCF (b0->data, pos, &chrs);
}

/*  int bsetstr (bstring b0, int pos, bstring b1, unsigned char fill)
 *
 *  Overwrite the string b0 starting at position pos with the string b1. If 
 *  the position pos is past the end of b0, then the character "fill" is 
 *  appended as necessary to make up the gap between the end of b0 and pos.
 *  If b1 is NULL, it behaves as if it were a 0-length string.
 */
int bsetstr (bstring b0, int pos, const_bstring b1, unsigned char fill) {
int d, newlen;
ptrdiff_t pd;
bstring aux = (bstring) b1;

    if (pos < 0 || b0 == NULL || b0->slen < 0 || NULL == b0->data || 
        b0->mlen < b0->slen || b0->mlen <= 0) return BSTR_ERR;
    if (b1 != NULL && (b1->slen < 0 || b1->data == NULL)) return BSTR_ERR;

    d = pos;

    /* Aliasing case */
    if (NULL != aux) {
        if ((pd = (ptrdiff_t) (b1->data - b0->data)) >= 0 && pd < (ptrdiff_t) b0->mlen) {
            if (NULL == (aux = bstrcpy (b1))) return BSTR_ERR;
        }
        d += aux->slen;
    }

    /* Increase memory size if necessary */
    if (balloc (b0, d + 1) != BSTR_OK) {
        if (aux != b1) bdestroy (aux);
        return BSTR_ERR;
    }

    newlen = b0->slen;

    /* Fill in "fill" character as necessary */
    if (pos > newlen) {
        bstr__memset (b0->data + b0->slen, (int) fill, (size_t) (pos - b0->slen));
        newlen = pos;
    }

    /* Copy b1 to position pos in b0. */
    if (aux != NULL) {
        bBlockCopy ((char *) (b0->data + pos), (char *) aux->data, aux->slen);
        if (aux != b1) bdestroy (aux);
    }

    /* Indicate the potentially increased size of b0 */
    if (d > newlen) newlen = d;

    b0->slen = newlen;
    b0->data[newlen] = (unsigned char) '\0';

    return BSTR_OK;
}

/*  int binsert (bstring b1, int pos, bstring b2, unsigned char fill)
 *
 *  Inserts the string b2 into b1 at position pos.  If the position pos is 
 *  past the end of b1, then the character "fill" is appended as necessary to 
 *  make up the gap between the end of b1 and pos.  Unlike bsetstr, binsert
 *  does not allow b2 to be NULL.
 */
int binsert (bstring b1, int pos, const_bstring b2, unsigned char fill) {
int d, l;
ptrdiff_t pd;
bstring aux = (bstring) b2;

    if (pos < 0 || b1 == NULL || b2 == NULL || b1->slen < 0 || 
        b2->slen < 0 || b1->mlen < b1->slen || b1->mlen <= 0) return BSTR_ERR;

    /* Aliasing case */
    if ((pd = (ptrdiff_t) (b2->data - b1->data)) >= 0 && pd < (ptrdiff_t) b1->mlen) {
        if (NULL == (aux = bstrcpy (b2))) return BSTR_ERR;
    }

    /* Compute the two possible end pointers */
    d = b1->slen + aux->slen;
    l = pos + aux->slen;
    if ((d|l) < 0) return BSTR_ERR;

    if (l > d) {
        /* Inserting past the end of the string */
        if (balloc (b1, l + 1) != BSTR_OK) {
            if (aux != b2) bdestroy (aux);
            return BSTR_ERR;
        }
        bstr__memset (b1->data + b1->slen, (int) fill, (size_t) (pos - b1->slen));
        b1->slen = l;
    } else {
        /* Inserting in the middle of the string */
        if (balloc (b1, d + 1) != BSTR_OK) {
            if (aux != b2) bdestroy (aux);
            return BSTR_ERR;
        }
        bBlockCopy (b1->data + l, b1->data + pos, d - l);
        b1->slen = d;
    }
    bBlockCopy (b1->data + pos, aux->data, aux->slen);
    b1->data[b1->slen] = (unsigned char) '\0';
    if (aux != b2) bdestroy (aux);
    return BSTR_OK;
}

/*  int breplace (bstring b1, int pos, int len, bstring b2, 
 *                unsigned char fill)
 *
 *  Replace a section of a string from pos for a length len with the string b2.
 *  fill is used is pos > b1->slen.
 */
int breplace (bstring b1, int pos, int len, const_bstring b2, 
              unsigned char fill) {
int pl, ret;
ptrdiff_t pd;
bstring aux = (bstring) b2;

    if (pos < 0 || len < 0 || (pl = pos + len) < 0 || b1 == NULL || 
        b2 == NULL || b1->data == NULL || b2->data == NULL || 
        b1->slen < 0 || b2->slen < 0 || b1->mlen < b1->slen ||
        b1->mlen <= 0) return BSTR_ERR;

    /* Straddles the end? */
    if (pl >= b1->slen) {
        if ((ret = bsetstr (b1, pos, b2, fill)) < 0) return ret;
        if (pos + b2->slen < b1->slen) {
            b1->slen = pos + b2->slen;
            b1->data[b1->slen] = (unsigned char) '\0';
        }
        return ret;
    }

    /* Aliasing case */
    if ((pd = (ptrdiff_t) (b2->data - b1->data)) >= 0 && pd < (ptrdiff_t) b1->slen) {
        if (NULL == (aux = bstrcpy (b2))) return BSTR_ERR;
    }

    if (aux->slen > len) {
        if (balloc (b1, b1->slen + aux->slen - len) != BSTR_OK) {
            if (aux != b2) bdestroy (aux);
            return BSTR_ERR;
        }
    }

    if (aux->slen != len) bstr__memmove (b1->data + pos + aux->slen, b1->data + pos + len, b1->slen - (pos + len));
    bstr__memcpy (b1->data + pos, aux->data, aux->slen);
    b1->slen += aux->slen - len;
    b1->data[b1->slen] = (unsigned char) '\0';
    if (aux != b2) bdestroy (aux);
    return BSTR_OK;
}

/*  
 *  findreplaceengine is used to implement bfindreplace and 
 *  bfindreplacecaseless. It works by breaking the three cases of
 *  expansion, reduction and replacement, and solving each of these
 *  in the most efficient way possible.
 */

typedef int (*instr_fnptr) (const_bstring s1, int pos, const_bstring s2);

#define INITIAL_STATIC_FIND_INDEX_COUNT 32

static int findreplaceengine (bstring b, const_bstring find, const_bstring repl, int pos, instr_fnptr instr) {
int i, ret, slen, mlen, delta, acc;
int * d;
int static_d[INITIAL_STATIC_FIND_INDEX_COUNT+1]; /* This +1 is unnecessary, but it shuts up LINT. */
ptrdiff_t pd;
bstring auxf = (bstring) find;
bstring auxr = (bstring) repl;

    if (b == NULL || b->data == NULL || find == NULL ||
        find->data == NULL || repl == NULL || repl->data == NULL || 
        pos < 0 || find->slen <= 0 || b->mlen < 0 || b->slen > b->mlen || 
        b->mlen <= 0 || b->slen < 0 || repl->slen < 0) return BSTR_ERR;
    if (pos > b->slen - find->slen) return BSTR_OK;

    /* Alias with find string */
    pd = (ptrdiff_t) (find->data - b->data);
    if ((ptrdiff_t) (pos - find->slen) < pd && pd < (ptrdiff_t) b->slen) {
        if (NULL == (auxf = bstrcpy (find))) return BSTR_ERR;
    }

    /* Alias with repl string */
    pd = (ptrdiff_t) (repl->data - b->data);
    if ((ptrdiff_t) (pos - repl->slen) < pd && pd < (ptrdiff_t) b->slen) {
        if (NULL == (auxr = bstrcpy (repl))) {
            if (auxf != find) bdestroy (auxf);
            return BSTR_ERR;
        }
    }

    delta = auxf->slen - auxr->slen;

    /* in-place replacement since find and replace strings are of equal 
       length */
    if (delta == 0) {
        while ((pos = instr (b, pos, auxf)) >= 0) {
            bstr__memcpy (b->data + pos, auxr->data, auxr->slen);
            pos += auxf->slen;
        }
        if (auxf != find) bdestroy (auxf);
        if (auxr != repl) bdestroy (auxr);
        return BSTR_OK;
    }

    /* shrinking replacement since auxf->slen > auxr->slen */
    if (delta > 0) {
        acc = 0;

        while ((i = instr (b, pos, auxf)) >= 0) {
            if (acc && i > pos)
                bstr__memmove (b->data + pos - acc, b->data + pos, i - pos);
            if (auxr->slen)
                bstr__memcpy (b->data + i - acc, auxr->data, auxr->slen);
            acc += delta;
            pos = i + auxf->slen;
        }

        if (acc) {
            i = b->slen;
            if (i > pos)
                bstr__memmove (b->data + pos - acc, b->data + pos, i - pos);
            b->slen -= acc;
            b->data[b->slen] = (unsigned char) '\0';
        }

        if (auxf != find) bdestroy (auxf);
        if (auxr != repl) bdestroy (auxr);
        return BSTR_OK;
    }

    /* expanding replacement since find->slen < repl->slen.  Its a lot 
       more complicated.  This works by first finding all the matches and 
       storing them to a growable array, then doing at most one resize of
       the destination bstring and then performing the direct memory transfers
       of the string segment pieces to form the final result. The growable 
       array of matches uses a deferred doubling reallocing strategy.  What 
       this means is that it starts as a reasonably fixed sized auto array in 
       the hopes that many if not most cases will never need to grow this 
       array.  But it switches as soon as the bounds of the array will be 
       exceeded.  An extra find result is always appended to this array that
       corresponds to the end of the destination string, so slen is checked
       against mlen - 1 rather than mlen before resizing.
    */

    mlen = INITIAL_STATIC_FIND_INDEX_COUNT;
    d = (int *) static_d; /* Avoid malloc for trivial/initial cases */
    acc = slen = 0;

    while ((pos = instr (b, pos, auxf)) >= 0) {
        if (slen >= mlen - 1) {
            int sl, *t;

            mlen += mlen;
            sl = sizeof (int *) * mlen;
            if (static_d == d) d = NULL; /* static_d cannot be realloced */
            if (mlen <= 0 || sl < mlen || NULL == (t = (int *) bstr__realloc (d, sl))) {
                ret = BSTR_ERR;
                goto done;
            }
            if (NULL == d) bstr__memcpy (t, static_d, sizeof (static_d));
            d = t;
        }
        d[slen] = pos;
        slen++;
        acc -= delta;
        pos += auxf->slen;
        if (pos < 0 || acc < 0) {
            ret = BSTR_ERR;
            goto done;
        }
    }
    
    /* slen <= INITIAL_STATIC_INDEX_COUNT-1 or mlen-1 here. */
    d[slen] = b->slen;

    if (BSTR_OK == (ret = balloc (b, b->slen + acc + 1))) {
        b->slen += acc;
        for (i = slen-1; i >= 0; i--) {
            int s, l;
            s = d[i] + auxf->slen;
            l = d[i+1] - s; /* d[slen] may be accessed here. */
            if (l) {
                bstr__memmove (b->data + s + acc, b->data + s, l);
            }
            if (auxr->slen) {
                bstr__memmove (b->data + s + acc - auxr->slen, 
                               auxr->data, auxr->slen);
            }
            acc += delta;        
        }
        b->data[b->slen] = (unsigned char) '\0';
    }

    done:;
    if (static_d == d) d = NULL;
    bstr__free (d);
    if (auxf != find) bdestroy (auxf);
    if (auxr != repl) bdestroy (auxr);
    return ret;
}

/*  int bfindreplace (bstring b, const_bstring find, const_bstring repl, 
 *                    int pos)
 *
 *  Replace all occurrences of a find string with a replace string after a
 *  given point in a bstring.
 */
int bfindreplace (bstring b, const_bstring find, const_bstring repl, int pos) {
    return findreplaceengine (b, find, repl, pos, binstr);
}

/*  int bfindreplacecaseless (bstring b, const_bstring find, const_bstring repl, 
 *                    int pos)
 *
 *  Replace all occurrences of a find string, ignoring case, with a replace 
 *  string after a given point in a bstring.
 */
int bfindreplacecaseless (bstring b, const_bstring find, const_bstring repl, int pos) {
    return findreplaceengine (b, find, repl, pos, binstrcaseless);
}

/*  int binsertch (bstring b, int pos, int len, unsigned char fill)
 *
 *  Inserts the character fill repeatedly into b at position pos for a 
 *  length len.  If the position pos is past the end of b, then the 
 *  character "fill" is appended as necessary to make up the gap between the 
 *  end of b and the position pos + len.
 */
int binsertch (bstring b, int pos, int len, unsigned char fill) {
int d, l, i;

    if (pos < 0 || b == NULL || b->slen < 0 || b->mlen < b->slen ||
        b->mlen <= 0 || len < 0) return BSTR_ERR;

    /* Compute the two possible end pointers */
    d = b->slen + len;
    l = pos + len;
    if ((d|l) < 0) return BSTR_ERR;

    if (l > d) {
        /* Inserting past the end of the string */
        if (balloc (b, l + 1) != BSTR_OK) return BSTR_ERR;
        pos = b->slen;
        b->slen = l;
    } else {
        /* Inserting in the middle of the string */
        if (balloc (b, d + 1) != BSTR_OK) return BSTR_ERR;
        for (i = d - 1; i >= l; i--) {
            b->data[i] = b->data[i - len];
        }
        b->slen = d;
    }

    for (i=pos; i < l; i++) b->data[i] = fill;
    b->data[b->slen] = (unsigned char) '\0';
    return BSTR_OK;
}

/*  int bpattern (bstring b, int len)
 *
 *  Replicate the bstring, b in place, end to end repeatedly until it 
 *  surpasses len characters, then chop the result to exactly len characters. 
 *  This function operates in-place.  The function will return with BSTR_ERR 
 *  if b is NULL or of length 0, otherwise BSTR_OK is returned.
 */
int bpattern (bstring b, int len) {
int i, d;

    d = blength (b);
    if (d <= 0 || len < 0 || balloc (b, len + 1) != BSTR_OK) return BSTR_ERR;
    if (len > 0) {
        if (d == 1) return bsetstr (b, len, NULL, b->data[0]);
        for (i = d; i < len; i++) b->data[i] = b->data[i - d];
    }
    b->data[len] = (unsigned char) '\0';
    b->slen = len;
    return BSTR_OK;
}

#define BS_BUFF_SZ (1024)

/*  int breada (bstring b, bNread readPtr, void * parm)
 *
 *  Use a finite buffer fread-like function readPtr to concatenate to the 
 *  bstring b the entire contents of file-like source data in a roughly 
 *  efficient way.
 */
int breada (bstring b, bNread readPtr, void * parm) {
int i, l, n;

    if (b == NULL || b->mlen <= 0 || b->slen < 0 || b->mlen < b->slen ||
        b->mlen <= 0 || readPtr == NULL) return BSTR_ERR;

    i = b->slen;
    for (n=i+16; ; n += ((n < BS_BUFF_SZ) ? n : BS_BUFF_SZ)) {
        if (BSTR_OK != balloc (b, n + 1)) return BSTR_ERR;
        l = (int) readPtr ((void *) (b->data + i), 1, n - i, parm);
        i += l;
        b->slen = i;
        if (i < n) break;
    }

    b->data[i] = (unsigned char) '\0';
    return BSTR_OK;
}

/*  bstring bread (bNread readPtr, void * parm)
 *
 *  Use a finite buffer fread-like function readPtr to create a bstring 
 *  filled with the entire contents of file-like source data in a roughly 
 *  efficient way.
 */
bstring bread (bNread readPtr, void * parm) {
bstring buff;

    if (0 > breada (buff = bfromcstr (""), readPtr, parm)) {
        bdestroy (buff);
        return NULL;
    }
    return buff;
}

/*  int bassigngets (bstring b, bNgetc getcPtr, void * parm, char terminator)
 *
 *  Use an fgetc-like single character stream reading function (getcPtr) to 
 *  obtain a sequence of characters which are concatenated to the end of the
 *  bstring b.  The stream read is terminated by the passed in terminator 
 *  parameter.
 *
 *  If getcPtr returns with a negative number, or the terminator character 
 *  (which is appended) is read, then the stream reading is halted and the 
 *  function returns with a partial result in b.  If there is an empty partial
 *  result, 1 is returned.  If no characters are read, or there is some other 
 *  detectable error, BSTR_ERR is returned.
 */
int bassigngets (bstring b, bNgetc getcPtr, void * parm, char terminator) {
int c, d, e;

    if (b == NULL || b->mlen <= 0 || b->slen < 0 || b->mlen < b->slen ||
        b->mlen <= 0 || getcPtr == NULL) return BSTR_ERR;
    d = 0;
    e = b->mlen - 2;

    while ((c = getcPtr (parm)) >= 0) {
        if (d > e) {
            b->slen = d;
            if (balloc (b, d + 2) != BSTR_OK) return BSTR_ERR;
            e = b->mlen - 2;
        }
        b->data[d] = (unsigned char) c;
        d++;
        if (c == terminator) break;
    }

    b->data[d] = (unsigned char) '\0';
    b->slen = d;

    return d == 0 && c < 0;
}

/*  int bgetsa (bstring b, bNgetc getcPtr, void * parm, char terminator)
 *
 *  Use an fgetc-like single character stream reading function (getcPtr) to 
 *  obtain a sequence of characters which are concatenated to the end of the
 *  bstring b.  The stream read is terminated by the passed in terminator 
 *  parameter.
 *
 *  If getcPtr returns with a negative number, or the terminator character 
 *  (which is appended) is read, then the stream reading is halted and the 
 *  function returns with a partial result concatentated to b.  If there is 
 *  an empty partial result, 1 is returned.  If no characters are read, or 
 *  there is some other detectable error, BSTR_ERR is returned.
 */
int bgetsa (bstring b, bNgetc getcPtr, void * parm, char terminator) {
int c, d, e;

    if (b == NULL || b->mlen <= 0 || b->slen < 0 || b->mlen < b->slen ||
        b->mlen <= 0 || getcPtr == NULL) return BSTR_ERR;
    d = b->slen;
    e = b->mlen - 2;

    while ((c = getcPtr (parm)) >= 0) {
        if (d > e) {
            b->slen = d;
            if (balloc (b, d + 2) != BSTR_OK) return BSTR_ERR;
            e = b->mlen - 2;
        }
        b->data[d] = (unsigned char) c;
        d++;
        if (c == terminator) break;
    }

    b->data[d] = (unsigned char) '\0';
    b->slen = d;

    return d == 0 && c < 0;
}

/*  bstring bgets (bNgetc getcPtr, void * parm, char terminator)
 *
 *  Use an fgetc-like single character stream reading function (getcPtr) to 
 *  obtain a sequence of characters which are concatenated into a bstring.  
 *  The stream read is terminated by the passed in terminator function.
 *
 *  If getcPtr returns with a negative number, or the terminator character 
 *  (which is appended) is read, then the stream reading is halted and the 
 *  result obtained thus far is returned.  If no characters are read, or 
 *  there is some other detectable error, NULL is returned.
 */
bstring bgets (bNgetc getcPtr, void * parm, char terminator) {
bstring buff;

    if (0 > bgetsa (buff = bfromcstr (""), getcPtr, parm, terminator) || 0 >= buff->slen) {
        bdestroy (buff);
        buff = NULL;
    }
    return buff;
}

struct bStream {
    bstring buff;        /* Buffer for over-reads */
    void * parm;        /* The stream handle for core stream */
    bNread readFnPtr;    /* fread compatible fnptr for core stream */
    int isEOF;        /* track file's EOF state */
    int maxBuffSz;
};

/*  struct bStream * bsopen (bNread readPtr, void * parm)
 *
 *  Wrap a given open stream (described by a fread compatible function 
 *  pointer and stream handle) into an open bStream suitable for the bstring 
 *  library streaming functions.
 */
struct bStream * bsopen (bNread readPtr, void * parm) {
struct bStream * s;

    if (readPtr == NULL) return NULL;
    s = (struct bStream *) bstr__alloc (sizeof (struct bStream));
    if (s == NULL) return NULL;
    s->parm = parm;
    s->buff = bfromcstr ("");
    s->readFnPtr = readPtr;
    s->maxBuffSz = BS_BUFF_SZ;
    s->isEOF = 0;
    return s;
}

/*  int bsbufflength (struct bStream * s, int sz)
 *
 *  Set the length of the buffer used by the bStream.  If sz is zero, the 
 *  length is not set.  This function returns with the previous length.
 */
int bsbufflength (struct bStream * s, int sz) {
int oldSz;
    if (s == NULL || sz < 0) return BSTR_ERR;
    oldSz = s->maxBuffSz;
    if (sz > 0) s->maxBuffSz = sz;
    return oldSz;
}

int bseof (const struct bStream * s) {
    if (s == NULL || s->readFnPtr == NULL) return BSTR_ERR;
    return s->isEOF && (s->buff->slen == 0);
}

/*  void * bsclose (struct bStream * s)
 *
 *  Close the bStream, and return the handle to the stream that was originally
 *  used to open the given stream.
 */
void * bsclose (struct bStream * s) {
void * parm;
    if (s == NULL) return NULL;
    s->readFnPtr = NULL;
    if (s->buff) bdestroy (s->buff);
    s->buff = NULL;
    parm = s->parm;
    s->parm = NULL;
    s->isEOF = 1;
    bstr__free (s);
    return parm;
}

/*  int bsreadlna (bstring r, struct bStream * s, char terminator)
 *
 *  Read a bstring terminated by the terminator character or the end of the
 *  stream from the bStream (s) and return it into the parameter r.  This 
 *  function may read additional characters from the core stream that are not 
 *  returned, but will be retained for subsequent read operations.
 */
int bsreadlna (bstring r, struct bStream * s, char terminator) {
int i, l, ret, rlo;
char * b;
struct tagbstring x;

    if (s == NULL || s->buff == NULL || r == NULL || r->mlen <= 0 ||
        r->slen < 0 || r->mlen < r->slen) return BSTR_ERR;
    l = s->buff->slen;
    if (BSTR_OK != balloc (s->buff, s->maxBuffSz + 1)) return BSTR_ERR;
    b = (char *) s->buff->data;
    x.data = (unsigned char *) b;

    /* First check if the current buffer holds the terminator */
    b[l] = terminator; /* Set sentinel */
    for (i=0; b[i] != terminator; i++) ;
    if (i < l) {
        x.slen = i + 1;
        ret = bconcat (r, &x);
        s->buff->slen = l;
        if (BSTR_OK == ret) bdelete (s->buff, 0, i + 1);
        return BSTR_OK;
    }

    rlo = r->slen;

    /* If not then just concatenate the entire buffer to the output */
    x.slen = l;
    if (BSTR_OK != bconcat (r, &x)) return BSTR_ERR;

    /* Perform direct in-place reads into the destination to allow for
       the minimum of data-copies */
    for (;;) {
        if (BSTR_OK != balloc (r, r->slen + s->maxBuffSz + 1)) return BSTR_ERR;
        b = (char *) (r->data + r->slen);
        l = (int) s->readFnPtr (b, 1, s->maxBuffSz, s->parm);
        if (l <= 0) {
            r->data[r->slen] = (unsigned char) '\0';
            s->buff->slen = 0;
            s->isEOF = 1;
            /* If nothing was read return with an error message */
            return BSTR_ERR & -(r->slen == rlo);
        }
        b[l] = terminator; /* Set sentinel */
        for (i=0; b[i] != terminator; i++) ;
        if (i < l) break;
        r->slen += l;
    }

    /* Terminator found, push over-read back to buffer */
    i++;
    r->slen += i;
    s->buff->slen = l - i;
    bstr__memcpy (s->buff->data, b + i, l - i);
    r->data[r->slen] = (unsigned char) '\0';
    return BSTR_OK;
}

/*  int bsreadlnsa (bstring r, struct bStream * s, bstring term)
 *
 *  Read a bstring terminated by any character in the term string or the end 
 *  of the stream from the bStream (s) and return it into the parameter r.  
 *  This function may read additional characters from the core stream that 
 *  are not returned, but will be retained for subsequent read operations.
 */
int bsreadlnsa (bstring r, struct bStream * s, const_bstring term) {
int i, l, ret, rlo;
unsigned char * b;
struct tagbstring x;
struct charField cf;

    if (s == NULL || s->buff == NULL || r == NULL || term == NULL ||
        term->data == NULL || r->mlen <= 0 || r->slen < 0 ||
        r->mlen < r->slen) return BSTR_ERR;
    if (term->slen == 1) return bsreadlna (r, s, term->data[0]);
    if (term->slen < 1 || buildCharField (&cf, term)) return BSTR_ERR;

    l = s->buff->slen;
    if (BSTR_OK != balloc (s->buff, s->maxBuffSz + 1)) return BSTR_ERR;
    b = (unsigned char *) s->buff->data;
    x.data = b;

    /* First check if the current buffer holds the terminator */
    b[l] = term->data[0]; /* Set sentinel */
    for (i=0; !testInCharField (&cf, b[i]); i++) ;
    if (i < l) {
        x.slen = i + 1;
        ret = bconcat (r, &x);
        s->buff->slen = l;
        if (BSTR_OK == ret) bdelete (s->buff, 0, i + 1);
        return BSTR_OK;
    }

    rlo = r->slen;

    /* If not then just concatenate the entire buffer to the output */
    x.slen = l;
    if (BSTR_OK != bconcat (r, &x)) return BSTR_ERR;

    /* Perform direct in-place reads into the destination to allow for
       the minimum of data-copies */
    for (;;) {
        if (BSTR_OK != balloc (r, r->slen + s->maxBuffSz + 1)) return BSTR_ERR;
        b = (unsigned char *) (r->data + r->slen);
        l = (int) s->readFnPtr (b, 1, s->maxBuffSz, s->parm);
        if (l <= 0) {
            r->data[r->slen] = (unsigned char) '\0';
            s->buff->slen = 0;
            s->isEOF = 1;
            /* If nothing was read return with an error message */
            return BSTR_ERR & -(r->slen == rlo);
        }

        b[l] = term->data[0]; /* Set sentinel */
        for (i=0; !testInCharField (&cf, b[i]); i++) ;
        if (i < l) break;
        r->slen += l;
    }

    /* Terminator found, push over-read back to buffer */
    i++;
    r->slen += i;
    s->buff->slen = l - i;
    bstr__memcpy (s->buff->data, b + i, l - i);
    r->data[r->slen] = (unsigned char) '\0';
    return BSTR_OK;
}

/*  int bsreada (bstring r, struct bStream * s, int n)
 *
 *  Read a bstring of length n (or, if it is fewer, as many bytes as is 
 *  remaining) from the bStream.  This function may read additional 
 *  characters from the core stream that are not returned, but will be 
 *  retained for subsequent read operations.  This function will not read
 *  additional characters from the core stream beyond virtual stream pointer.
 */
int bsreada (bstring r, struct bStream * s, int n) {
int l, ret, orslen;
char * b;
struct tagbstring x;

    if (s == NULL || s->buff == NULL || r == NULL || r->mlen <= 0
     || r->slen < 0 || r->mlen < r->slen || n <= 0) return BSTR_ERR;

    n += r->slen;
    if (n <= 0) return BSTR_ERR;

    l = s->buff->slen;

    orslen = r->slen;

    if (0 == l) {
        if (s->isEOF) return BSTR_ERR;
        if (r->mlen > n) {
            l = (int) s->readFnPtr (r->data + r->slen, 1, n - r->slen, s->parm);
            if (0 >= l || l > n - r->slen) {
                s->isEOF = 1;
                return BSTR_ERR;
            }
            r->slen += l;
            r->data[r->slen] = (unsigned char) '\0';
            return 0;
        }
    }

    if (BSTR_OK != balloc (s->buff, s->maxBuffSz + 1)) return BSTR_ERR;
    b = (char *) s->buff->data;
    x.data = (unsigned char *) b;

    do {
        if (l + r->slen >= n) {
            x.slen = n - r->slen;
            ret = bconcat (r, &x);
            s->buff->slen = l;
            if (BSTR_OK == ret) bdelete (s->buff, 0, x.slen);
            return BSTR_ERR & -(r->slen == orslen);
        }

        x.slen = l;
        if (BSTR_OK != bconcat (r, &x)) break;

        l = n - r->slen;
        if (l > s->maxBuffSz) l = s->maxBuffSz;

        l = (int) s->readFnPtr (b, 1, l, s->parm);

    } while (l > 0);
    if (l < 0) l = 0;
    if (l == 0) s->isEOF = 1;
    s->buff->slen = l;
    return BSTR_ERR & -(r->slen == orslen);
}

/*  int bsreadln (bstring r, struct bStream * s, char terminator)
 *
 *  Read a bstring terminated by the terminator character or the end of the
 *  stream from the bStream (s) and return it into the parameter r.  This 
 *  function may read additional characters from the core stream that are not 
 *  returned, but will be retained for subsequent read operations.
 */
int bsreadln (bstring r, struct bStream * s, char terminator) {
    if (s == NULL || s->buff == NULL || r == NULL || r->mlen <= 0)
        return BSTR_ERR;
    if (BSTR_OK != balloc (s->buff, s->maxBuffSz + 1)) return BSTR_ERR;
    r->slen = 0;
    return bsreadlna (r, s, terminator);
}

/*  int bsreadlns (bstring r, struct bStream * s, bstring term)
 *
 *  Read a bstring terminated by any character in the term string or the end 
 *  of the stream from the bStream (s) and return it into the parameter r.  
 *  This function may read additional characters from the core stream that 
 *  are not returned, but will be retained for subsequent read operations.
 */
int bsreadlns (bstring r, struct bStream * s, const_bstring term) {
    if (s == NULL || s->buff == NULL || r == NULL || term == NULL 
     || term->data == NULL || r->mlen <= 0) return BSTR_ERR;
    if (term->slen == 1) return bsreadln (r, s, term->data[0]);
    if (term->slen < 1) return BSTR_ERR;
    if (BSTR_OK != balloc (s->buff, s->maxBuffSz + 1)) return BSTR_ERR;
    r->slen = 0;
    return bsreadlnsa (r, s, term);
}

/*  int bsread (bstring r, struct bStream * s, int n)
 *
 *  Read a bstring of length n (or, if it is fewer, as many bytes as is 
 *  remaining) from the bStream.  This function may read additional 
 *  characters from the core stream that are not returned, but will be 
 *  retained for subsequent read operations.  This function will not read
 *  additional characters from the core stream beyond virtual stream pointer.
 */
int bsread (bstring r, struct bStream * s, int n) {
    if (s == NULL || s->buff == NULL || r == NULL || r->mlen <= 0
     || n <= 0) return BSTR_ERR;
    if (BSTR_OK != balloc (s->buff, s->maxBuffSz + 1)) return BSTR_ERR;
    r->slen = 0;
    return bsreada (r, s, n);
}

/*  int bsunread (struct bStream * s, const_bstring b)
 *
 *  Insert a bstring into the bStream at the current position.  These 
 *  characters will be read prior to those that actually come from the core 
 *  stream.
 */
int bsunread (struct bStream * s, const_bstring b) {
    if (s == NULL || s->buff == NULL) return BSTR_ERR;
    return binsert (s->buff, 0, b, (unsigned char) '?');
}

/*  int bspeek (bstring r, const struct bStream * s)
 *
 *  Return the currently buffered characters from the bStream that will be 
 *  read prior to reads from the core stream.
 */
int bspeek (bstring r, const struct bStream * s) {
    if (s == NULL || s->buff == NULL) return BSTR_ERR;
    return bassign (r, s->buff);
}

/*  bstring bjoin (const struct bstrList * bl, const_bstring sep);
 *
 *  Join the entries of a bstrList into one bstring by sequentially 
 *  concatenating them with the sep string in between.  If there is an error 
 *  NULL is returned, otherwise a bstring with the correct result is returned.
 */
bstring bjoin (const struct bstrList * bl, const_bstring sep) {
bstring b;
int i, c, v;

    if (bl == NULL || bl->qty < 0) return NULL;
    if (sep != NULL && (sep->slen < 0 || sep->data == NULL)) return NULL;

    for (i = 0, c = 1; i < bl->qty; i++) {
        v = bl->entry[i]->slen;
        if (v < 0) return NULL;    /* Invalid input */
        c += v;
        if (c < 0) return NULL;    /* Wrap around ?? */
    }

    if (sep != NULL) c += (bl->qty - 1) * sep->slen;

    b = (bstring) bstr__alloc (sizeof (struct tagbstring));
    if (NULL == b) return NULL; /* Out of memory */
    b->data = (unsigned char *) bstr__alloc (c);
    if (b->data == NULL) {
        bstr__free (b);
        return NULL;
    }

    b->mlen = c;
    b->slen = c-1;

    for (i = 0, c = 0; i < bl->qty; i++) {
        if (i > 0 && sep != NULL) {
            bstr__memcpy (b->data + c, sep->data, sep->slen);
            c += sep->slen;
        }
        v = bl->entry[i]->slen;
        bstr__memcpy (b->data + c, bl->entry[i]->data, v);
        c += v;
    }
    b->data[c] = (unsigned char) '\0';
    return b;
}

#define BSSSC_BUFF_LEN (256)

/*  int bssplitscb (struct bStream * s, const_bstring splitStr, 
 *    int (* cb) (void * parm, int ofs, const_bstring entry), void * parm)
 *
 *  Iterate the set of disjoint sequential substrings read from a stream 
 *  divided by any of the characters in splitStr.  An empty splitStr causes 
 *  the whole stream to be iterated once.
 *
 *  Note: At the point of calling the cb function, the bStream pointer is 
 *  pointed exactly at the position right after having read the split 
 *  character.  The cb function can act on the stream by causing the bStream
 *  pointer to move, and bssplitscb will continue by starting the next split
 *  at the position of the pointer after the return from cb.
 *
 *  However, if the cb causes the bStream s to be destroyed then the cb must
 *  return with a negative value, otherwise bssplitscb will continue in an 
 *  undefined manner.
 */
int bssplitscb (struct bStream * s, const_bstring splitStr, 
    int (* cb) (void * parm, int ofs, const_bstring entry), void * parm) {
struct charField chrs;
bstring buff;
int i, p, ret;

    if (cb == NULL || s == NULL || s->readFnPtr == NULL 
     || splitStr == NULL || splitStr->slen < 0) return BSTR_ERR;

    if (NULL == (buff = bfromcstr (""))) return BSTR_ERR;

    if (splitStr->slen == 0) {
        while (bsreada (buff, s, BSSSC_BUFF_LEN) >= 0) ;
        if ((ret = cb (parm, 0, buff)) > 0) 
            ret = 0;
    } else {
        buildCharField (&chrs, splitStr);
        ret = p = i = 0;
        for (;;) {
            if (i >= buff->slen) {
                bsreada (buff, s, BSSSC_BUFF_LEN);
                if (i >= buff->slen) {
                    if (0 < (ret = cb (parm, p, buff))) ret = 0;
                    break;
                }
            }
            if (testInCharField (&chrs, buff->data[i])) {
                struct tagbstring t;
                unsigned char c;

                blk2tbstr (t, buff->data + i + 1, buff->slen - (i + 1));
                if ((ret = bsunread (s, &t)) < 0) break;
                buff->slen = i;
                c = buff->data[i];
                buff->data[i] = (unsigned char) '\0';
                if ((ret = cb (parm, p, buff)) < 0) break;
                buff->data[i] = c;
                buff->slen = 0;
                p += i + 1;
                i = -1;
            }
            i++;
        }
    }

    bdestroy (buff);
    return ret;
}

/*  int bssplitstrcb (struct bStream * s, const_bstring splitStr, 
 *    int (* cb) (void * parm, int ofs, const_bstring entry), void * parm)
 *
 *  Iterate the set of disjoint sequential substrings read from a stream 
 *  divided by the entire substring splitStr.  An empty splitStr causes 
 *  each character of the stream to be iterated.
 *
 *  Note: At the point of calling the cb function, the bStream pointer is 
 *  pointed exactly at the position right after having read the split 
 *  character.  The cb function can act on the stream by causing the bStream
 *  pointer to move, and bssplitscb will continue by starting the next split
 *  at the position of the pointer after the return from cb.
 *
 *  However, if the cb causes the bStream s to be destroyed then the cb must
 *  return with a negative value, otherwise bssplitscb will continue in an 
 *  undefined manner.
 */
int bssplitstrcb (struct bStream * s, const_bstring splitStr, 
    int (* cb) (void * parm, int ofs, const_bstring entry), void * parm) {
bstring buff;
int i, p, ret;

    if (cb == NULL || s == NULL || s->readFnPtr == NULL 
     || splitStr == NULL || splitStr->slen < 0) return BSTR_ERR;

    if (splitStr->slen == 1) return bssplitscb (s, splitStr, cb, parm);

    if (NULL == (buff = bfromcstr (""))) return BSTR_ERR;

    if (splitStr->slen == 0) {
        for (i=0; bsreada (buff, s, BSSSC_BUFF_LEN) >= 0; i++) {
            if ((ret = cb (parm, 0, buff)) < 0) {
                bdestroy (buff);
                return ret;
            }
            buff->slen = 0;
        }
        return BSTR_OK;
    } else {
        ret = p = i = 0;
        for (i=p=0;;) {
            if ((ret = binstr (buff, 0, splitStr)) >= 0) {
                struct tagbstring t;
                blk2tbstr (t, buff->data, ret);
                i = ret + splitStr->slen;
                if ((ret = cb (parm, p, &t)) < 0) break;
                p += i;
                bdelete (buff, 0, i);
            } else {
                bsreada (buff, s, BSSSC_BUFF_LEN);
                if (bseof (s)) {
                    if ((ret = cb (parm, p, buff)) > 0) ret = 0;
                    break;
                }
            }
        }
    }

    bdestroy (buff);
    return ret;
}

/*  int bstrListCreate (void)
 *
 *  Create a bstrList.
 */
struct bstrList * bstrListCreate (void) {
struct bstrList * sl = (struct bstrList *) bstr__alloc (sizeof (struct bstrList));
    if (sl) {
        sl->entry = (bstring *) bstr__alloc (1*sizeof (bstring));
        if (!sl->entry) {
            bstr__free (sl);
            sl = NULL;
        } else {
            sl->qty = 0;
            sl->mlen = 1;
        }
    }
    return sl;
}

/*  int bstrListDestroy (struct bstrList * sl)
 *
 *  Destroy a bstrList that has been created by bsplit, bsplits or bstrListCreate.
 */
int bstrListDestroy (struct bstrList * sl) {
int i;
    if (sl == NULL || sl->qty < 0) return BSTR_ERR;
    for (i=0; i < sl->qty; i++) {
        if (sl->entry[i]) {
            bdestroy (sl->entry[i]);
            sl->entry[i] = NULL;
        }
    }
    sl->qty  = -1;
    sl->mlen = -1;
    bstr__free (sl->entry);
    sl->entry = NULL;
    bstr__free (sl);
    return BSTR_OK;
}

/*  int bstrListAlloc (struct bstrList * sl, int msz)
 *
 *  Ensure that there is memory for at least msz number of entries for the
 *  list.
 */
int bstrListAlloc (struct bstrList * sl, int msz) {
bstring * l;
int smsz;
size_t nsz;
    if (!sl || msz <= 0 || !sl->entry || sl->qty < 0 || sl->mlen <= 0 || sl->qty > sl->mlen) return BSTR_ERR;
    if (sl->mlen >= msz) return BSTR_OK;
    smsz = snapUpSize (msz);
    nsz = ((size_t) smsz) * sizeof (bstring);
    if (nsz < (size_t) smsz) return BSTR_ERR;
    l = (bstring *) bstr__realloc (sl->entry, nsz);
    if (!l) {
        smsz = msz;
        nsz = ((size_t) smsz) * sizeof (bstring);
        l = (bstring *) bstr__realloc (sl->entry, nsz);
        if (!l) return BSTR_ERR;
    }
    sl->mlen = smsz;
    sl->entry = l;
    return BSTR_OK;
}

/*  int bstrListAllocMin (struct bstrList * sl, int msz)
 *
 *  Try to allocate the minimum amount of memory for the list to include at
 *  least msz entries or sl->qty whichever is greater.
 */
int bstrListAllocMin (struct bstrList * sl, int msz) {
bstring * l;
size_t nsz;
    if (!sl || msz <= 0 || !sl->entry || sl->qty < 0 || sl->mlen <= 0 || sl->qty > sl->mlen) return BSTR_ERR;
    if (msz < sl->qty) msz = sl->qty;
    if (sl->mlen == msz) return BSTR_OK;
    nsz = ((size_t) msz) * sizeof (bstring);
    if (nsz < (size_t) msz) return BSTR_ERR;
    l = (bstring *) bstr__realloc (sl->entry, nsz);
    if (!l) return BSTR_ERR;
    sl->mlen = msz;
    sl->entry = l;
    return BSTR_OK;
}

/*  int bsplitcb (const_bstring str, unsigned char splitChar, int pos,
 *    int (* cb) (void * parm, int ofs, int len), void * parm)
 *
 *  Iterate the set of disjoint sequential substrings over str divided by the
 *  character in splitChar.
 *
 *  Note: Non-destructive modification of str from within the cb function 
 *  while performing this split is not undefined.  bsplitcb behaves in 
 *  sequential lock step with calls to cb.  I.e., after returning from a cb 
 *  that return a non-negative integer, bsplitcb continues from the position 
 *  1 character after the last detected split character and it will halt 
 *  immediately if the length of str falls below this point.  However, if the 
 *  cb function destroys str, then it *must* return with a negative value, 
 *  otherwise bsplitcb will continue in an undefined manner.
 */
int bsplitcb (const_bstring str, unsigned char splitChar, int pos,
    int (* cb) (void * parm, int ofs, int len), void * parm) {
int i, p, ret;

    if (cb == NULL || str == NULL || pos < 0 || pos > str->slen) 
        return BSTR_ERR;

    p = pos;
    do {
        for (i=p; i < str->slen; i++) {
            if (str->data[i] == splitChar) break;
        }
        if ((ret = cb (parm, p, i - p)) < 0) return ret;
        p = i + 1;
    } while (p <= str->slen);
    return BSTR_OK;
}

/*  int bsplitscb (const_bstring str, const_bstring splitStr, int pos,
 *    int (* cb) (void * parm, int ofs, int len), void * parm)
 *
 *  Iterate the set of disjoint sequential substrings over str divided by any 
 *  of the characters in splitStr.  An empty splitStr causes the whole str to
 *  be iterated once.
 *
 *  Note: Non-destructive modification of str from within the cb function 
 *  while performing this split is not undefined.  bsplitscb behaves in 
 *  sequential lock step with calls to cb.  I.e., after returning from a cb 
 *  that return a non-negative integer, bsplitscb continues from the position 
 *  1 character after the last detected split character and it will halt 
 *  immediately if the length of str falls below this point.  However, if the 
 *  cb function destroys str, then it *must* return with a negative value, 
 *  otherwise bsplitscb will continue in an undefined manner.
 */
int bsplitscb (const_bstring str, const_bstring splitStr, int pos,
    int (* cb) (void * parm, int ofs, int len), void * parm) {
struct charField chrs;
int i, p, ret;

    if (cb == NULL || str == NULL || pos < 0 || pos > str->slen 
     || splitStr == NULL || splitStr->slen < 0) return BSTR_ERR;
    if (splitStr->slen == 0) {
        if ((ret = cb (parm, 0, str->slen)) > 0) ret = 0;
        return ret;
    }

    if (splitStr->slen == 1) 
        return bsplitcb (str, splitStr->data[0], pos, cb, parm);

    buildCharField (&chrs, splitStr);

    p = pos;
    do {
        for (i=p; i < str->slen; i++) {
            if (testInCharField (&chrs, str->data[i])) break;
        }
        if ((ret = cb (parm, p, i - p)) < 0) return ret;
        p = i + 1;
    } while (p <= str->slen);
    return BSTR_OK;
}

/*  int bsplitstrcb (const_bstring str, const_bstring splitStr, int pos,
 *    int (* cb) (void * parm, int ofs, int len), void * parm)
 *
 *  Iterate the set of disjoint sequential substrings over str divided by the 
 *  substring splitStr.  An empty splitStr causes the whole str to be 
 *  iterated once.
 *
 *  Note: Non-destructive modification of str from within the cb function 
 *  while performing this split is not undefined.  bsplitstrcb behaves in 
 *  sequential lock step with calls to cb.  I.e., after returning from a cb 
 *  that return a non-negative integer, bsplitscb continues from the position 
 *  1 character after the last detected split character and it will halt 
 *  immediately if the length of str falls below this point.  However, if the 
 *  cb function destroys str, then it *must* return with a negative value, 
 *  otherwise bsplitscb will continue in an undefined manner.
 */
int bsplitstrcb (const_bstring str, const_bstring splitStr, int pos,
    int (* cb) (void * parm, int ofs, int len), void * parm) {
int i, p, ret;

    if (cb == NULL || str == NULL || pos < 0 || pos > str->slen 
     || splitStr == NULL || splitStr->slen < 0) return BSTR_ERR;

    if (0 == splitStr->slen) {
        for (i=pos; i < str->slen; i++) {
            if ((ret = cb (parm, i, 1)) < 0) return ret;
        }
        return BSTR_OK;
    }

    if (splitStr->slen == 1) 
        return bsplitcb (str, splitStr->data[0], pos, cb, parm);

    for (i=p=pos; i <= str->slen - splitStr->slen; i++) {
        if (0 == bstr__memcmp (splitStr->data, str->data + i, splitStr->slen)) {
            if ((ret = cb (parm, p, i - p)) < 0) return ret;
            i += splitStr->slen;
            p = i;
        }
    }
    if ((ret = cb (parm, p, str->slen - p)) < 0) return ret;
    return BSTR_OK;
}

struct genBstrList {
    bstring b;
    struct bstrList * bl;
};

static int bscb (void * parm, int ofs, int len) {
struct genBstrList * g = (struct genBstrList *) parm;
    if (g->bl->qty >= g->bl->mlen) {
        int mlen = g->bl->mlen * 2;
        bstring * tbl;

        while (g->bl->qty >= mlen) {
            if (mlen < g->bl->mlen) return BSTR_ERR;
            mlen += mlen;
        }

        tbl = (bstring *) bstr__realloc (g->bl->entry, sizeof (bstring) * mlen);
        if (tbl == NULL) return BSTR_ERR;

        g->bl->entry = tbl;
        g->bl->mlen = mlen;
    }

    g->bl->entry[g->bl->qty] = bmidstr (g->b, ofs, len);
    g->bl->qty++;
    return BSTR_OK;
}

/*  struct bstrList * bsplit (const_bstring str, unsigned char splitChar)
 *
 *  Create an array of sequential substrings from str divided by the character
 *  splitChar.  
 */
struct bstrList * bsplit (const_bstring str, unsigned char splitChar) {
struct genBstrList g;

    if (str == NULL || str->data == NULL || str->slen < 0) return NULL;

    g.bl = (struct bstrList *) bstr__alloc (sizeof (struct bstrList));
    if (g.bl == NULL) return NULL;
    g.bl->mlen = 4;
    g.bl->entry = (bstring *) bstr__alloc (g.bl->mlen * sizeof (bstring));
    if (NULL == g.bl->entry) {
        bstr__free (g.bl);
        return NULL;
    }

    g.b = (bstring) str;
    g.bl->qty = 0;
    if (bsplitcb (str, splitChar, 0, bscb, &g) < 0) {
        bstrListDestroy (g.bl);
        return NULL;
    }
    return g.bl;
}

/*  struct bstrList * bsplitstr (const_bstring str, const_bstring splitStr)
 *
 *  Create an array of sequential substrings from str divided by the entire
 *  substring splitStr.
 */
struct bstrList * bsplitstr (const_bstring str, const_bstring splitStr) {
struct genBstrList g;

    if (str == NULL || str->data == NULL || str->slen < 0) return NULL;

    g.bl = (struct bstrList *) bstr__alloc (sizeof (struct bstrList));
    if (g.bl == NULL) return NULL;
    g.bl->mlen = 4;
    g.bl->entry = (bstring *) bstr__alloc (g.bl->mlen * sizeof (bstring));
    if (NULL == g.bl->entry) {
        bstr__free (g.bl);
        return NULL;
    }

    g.b = (bstring) str;
    g.bl->qty = 0;
    if (bsplitstrcb (str, splitStr, 0, bscb, &g) < 0) {
        bstrListDestroy (g.bl);
        return NULL;
    }
    return g.bl;
}

/*  struct bstrList * bsplits (const_bstring str, bstring splitStr)
 *
 *  Create an array of sequential substrings from str divided by any of the 
 *  characters in splitStr.  An empty splitStr causes a single entry bstrList
 *  containing a copy of str to be returned.
 */
struct bstrList * bsplits (const_bstring str, const_bstring splitStr) {
struct genBstrList g;

    if (     str == NULL ||      str->slen < 0 ||      str->data == NULL ||
        splitStr == NULL || splitStr->slen < 0 || splitStr->data == NULL)
        return NULL;

    g.bl = (struct bstrList *) bstr__alloc (sizeof (struct bstrList));
    if (g.bl == NULL) return NULL;
    g.bl->mlen = 4;
    g.bl->entry = (bstring *) bstr__alloc (g.bl->mlen * sizeof (bstring));
    if (NULL == g.bl->entry) {
        bstr__free (g.bl);
        return NULL;
    }
    g.b = (bstring) str;
    g.bl->qty = 0;

    if (bsplitscb (str, splitStr, 0, bscb, &g) < 0) {
        bstrListDestroy (g.bl);
        return NULL;
    }
    return g.bl;
}

#if defined (__TURBOC__) && !defined (__BORLANDC__)
# ifndef BSTRLIB_NOVSNP
#  define BSTRLIB_NOVSNP
# endif
#endif

/* Give WATCOM C/C++, MSVC some latitude for their non-support of vsnprintf */
#if defined(__WATCOMC__) || defined(_MSC_VER)
#define exvsnprintf(r,b,n,f,a) {r = _vsnprintf (b,n,f,a);}
#else
#ifdef BSTRLIB_NOVSNP
/* This is just a hack.  If you are using a system without a vsnprintf, it is 
   not recommended that bformat be used at all. */
#define exvsnprintf(r,b,n,f,a) {vsprintf (b,f,a); r = -1;}
#define START_VSNBUFF (256)
#else

#ifdef __GNUC__
/* Something is making gcc complain about this prototype not being here, so 
   I've just gone ahead and put it in. */
//extern int vsnprintf (char *buf, size_t count, const char *format, va_list arg);
#endif

#define exvsnprintf(r,b,n,f,a) {r = vsnprintf (b,n,f,a);}
#endif
#endif

#if !defined (BSTRLIB_NOVSNP)

#ifndef START_VSNBUFF
#define START_VSNBUFF (16)
#endif

/* On IRIX vsnprintf returns n-1 when the operation would overflow the target 
   buffer, WATCOM and MSVC both return -1, while C99 requires that the 
   returned value be exactly what the length would be if the buffer would be
   large enough.  This leads to the idea that if the return value is larger 
   than n, then changing n to the return value will reduce the number of
   iterations required. */

/*  int bformata (bstring b, const char * fmt, ...)
 *
 *  After the first parameter, it takes the same parameters as printf (), but 
 *  rather than outputting results to stdio, it appends the results to 
 *  a bstring which contains what would have been output. Note that if there 
 *  is an early generation of a '\0' character, the bstring will be truncated 
 *  to this end point.
 */
int bformata (bstring b, const char * fmt, ...) {
va_list arglist;
bstring buff;
int n, r;

    if (b == NULL || fmt == NULL || b->data == NULL || b->mlen <= 0 
     || b->slen < 0 || b->slen > b->mlen) return BSTR_ERR;

    /* Since the length is not determinable beforehand, a search is
       performed using the truncating "vsnprintf" call (to avoid buffer
       overflows) on increasing potential sizes for the output result. */

    if ((n = (int) (2*strlen (fmt))) < START_VSNBUFF) n = START_VSNBUFF;
    if (NULL == (buff = bfromcstralloc (n + 2, ""))) {
        n = 1;
        if (NULL == (buff = bfromcstralloc (n + 2, ""))) return BSTR_ERR;
    }

    for (;;) {
        va_start (arglist, fmt);
        exvsnprintf (r, (char *) buff->data, n + 1, fmt, arglist);
        va_end (arglist);

        buff->data[n] = (unsigned char) '\0';
        buff->slen = (int) (strlen) ((char *) buff->data);

        if (buff->slen < n) break;

        if (r > n) n = r; else n += n;

        if (BSTR_OK != balloc (buff, n + 2)) {
            bdestroy (buff);
            return BSTR_ERR;
        }
    }

    r = bconcat (b, buff);
    bdestroy (buff);
    return r;
}

/*  int bassignformat (bstring b, const char * fmt, ...)
 *
 *  After the first parameter, it takes the same parameters as printf (), but 
 *  rather than outputting results to stdio, it outputs the results to 
 *  the bstring parameter b. Note that if there is an early generation of a 
 *  '\0' character, the bstring will be truncated to this end point.
 */
int bassignformat (bstring b, const char * fmt, ...) {
va_list arglist;
bstring buff;
int n, r;

    if (b == NULL || fmt == NULL || b->data == NULL || b->mlen <= 0 
     || b->slen < 0 || b->slen > b->mlen) return BSTR_ERR;

    /* Since the length is not determinable beforehand, a search is
       performed using the truncating "vsnprintf" call (to avoid buffer
       overflows) on increasing potential sizes for the output result. */

    if ((n = (int) (2*strlen (fmt))) < START_VSNBUFF) n = START_VSNBUFF;
    if (NULL == (buff = bfromcstralloc (n + 2, ""))) {
        n = 1;
        if (NULL == (buff = bfromcstralloc (n + 2, ""))) return BSTR_ERR;
    }

    for (;;) {
        va_start (arglist, fmt);
        exvsnprintf (r, (char *) buff->data, n + 1, fmt, arglist);
        va_end (arglist);

        buff->data[n] = (unsigned char) '\0';
        buff->slen = (int) (strlen) ((char *) buff->data);

        if (buff->slen < n) break;

        if (r > n) n = r; else n += n;

        if (BSTR_OK != balloc (buff, n + 2)) {
            bdestroy (buff);
            return BSTR_ERR;
        }
    }

    r = bassign (b, buff);
    bdestroy (buff);
    return r;
}

/*  bstring bformat (const char * fmt, ...)
 *
 *  Takes the same parameters as printf (), but rather than outputting results
 *  to stdio, it forms a bstring which contains what would have been output.
 *  Note that if there is an early generation of a '\0' character, the 
 *  bstring will be truncated to this end point.
 */
bstring bformat (const char * fmt, ...) {
va_list arglist;
bstring buff;
int n, r;

    if (fmt == NULL) return NULL;

    /* Since the length is not determinable beforehand, a search is
       performed using the truncating "vsnprintf" call (to avoid buffer
       overflows) on increasing potential sizes for the output result. */

    if ((n = (int) (2*strlen (fmt))) < START_VSNBUFF) n = START_VSNBUFF;
    if (NULL == (buff = bfromcstralloc (n + 2, ""))) {
        n = 1;
        if (NULL == (buff = bfromcstralloc (n + 2, ""))) return NULL;
    }

    for (;;) {
        va_start (arglist, fmt);
        exvsnprintf (r, (char *) buff->data, n + 1, fmt, arglist);
        va_end (arglist);

        buff->data[n] = (unsigned char) '\0';
        buff->slen = (int) (strlen) ((char *) buff->data);

        if (buff->slen < n) break;

        if (r > n) n = r; else n += n;

        if (BSTR_OK != balloc (buff, n + 2)) {
            bdestroy (buff);
            return NULL;
        }
    }

    return buff;
}

/*  int bvcformata (bstring b, int count, const char * fmt, va_list arglist)
 *
 *  The bvcformata function formats data under control of the format control 
 *  string fmt and attempts to append the result to b.  The fmt parameter is 
 *  the same as that of the printf function.  The variable argument list is 
 *  replaced with arglist, which has been initialized by the va_start macro.
 *  The size of the appended output is upper bounded by count.  If the 
 *  required output exceeds count, the string b is not augmented with any 
 *  contents and a value below BSTR_ERR is returned.  If a value below -count 
 *  is returned then it is recommended that the negative of this value be 
 *  used as an update to the count in a subsequent pass.  On other errors, 
 *  such as running out of memory, parameter errors or numeric wrap around 
 *  BSTR_ERR is returned.  BSTR_OK is returned when the output is successfully 
 *  generated and appended to b.
 *
 *  Note: There is no sanity checking of arglist, and this function is
 *  destructive of the contents of b from the b->slen point onward.  If there 
 *  is an early generation of a '\0' character, the bstring will be truncated 
 *  to this end point.
 */
int bvcformata (bstring b, int count, const char * fmt, va_list arg) {
int n, r, l;

    if (b == NULL || fmt == NULL || count <= 0 || b->data == NULL
     || b->mlen <= 0 || b->slen < 0 || b->slen > b->mlen) return BSTR_ERR;

    if (count > (n = b->slen + count) + 2) return BSTR_ERR;
    if (BSTR_OK != balloc (b, n + 2)) return BSTR_ERR;

    exvsnprintf (r, (char *) b->data + b->slen, count + 2, fmt, arg);

    /* Did the operation complete successfully within bounds? */
    for (l = b->slen; l <= n; l++) {
        if ('\0' == b->data[l]) {
            b->slen = l;
            return BSTR_OK;
        }
    }

    /* Abort, since the buffer was not large enough.  The return value 
       tries to help set what the retry length should be. */

    b->data[b->slen] = '\0';
    if (r > count + 1) {    /* Does r specify a particular target length? */
        n = r;
    } else {
        n = count + count;    /* If not, just double the size of count */
        if (count > n) n = INT_MAX;
    }
    n = -n;

    if (n > BSTR_ERR-1) n = BSTR_ERR-1;
    return n;
}

#endif
