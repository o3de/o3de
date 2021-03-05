/*
 * This source file is part of the bstring string library.  This code was
 * written by Paul Hsieh in 2002-2010, and is covered by either the 3-clause 
 * BSD open source license or GPL v2.0. Refer to the accompanying documentation 
 * for details on usage and license.
 */
// Modifications copyright Amazon.com, Inc. or its affiliates

/*
 * bstraux.c
 *
 * This file is not necessarily part of the core bstring library itself, but
 * is just an auxilliary module which includes miscellaneous or trivial 
 * functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include "bstrlib.h"
#include "bstraux.h"

/*  bstring bTail (bstring b, int n)
 *
 *  Return with a string of the last n characters of b.
 */
bstring bTail (bstring b, int n) {
    if (b == NULL || n < 0 || (b->mlen < b->slen && b->mlen > 0)) return NULL;
    if (n >= b->slen) return bstrcpy (b);
    return bmidstr (b, b->slen - n, n);
}

/*  bstring bHead (bstring b, int n)
 *
 *  Return with a string of the first n characters of b.
 */
bstring bHead (bstring b, int n) {
    if (b == NULL || n < 0 || (b->mlen < b->slen && b->mlen > 0)) return NULL;
    if (n >= b->slen) return bstrcpy (b);
    return bmidstr (b, 0, n);
}

/*  int bFill (bstring a, char c, int len)
 *
 *  Fill a given bstring with the character in parameter c, for a length n.
 */
int bFill (bstring b, char c, int len) {
    if (b == NULL || len < 0 || (b->mlen < b->slen && b->mlen > 0)) return -__LINE__;
    b->slen = 0;
    return bsetstr (b, len, NULL, c);
}

/*  int bReplicate (bstring b, int n)
 *
 *  Replicate the contents of b end to end n times and replace it in b.
 */
int bReplicate (bstring b, int n) {
    return bpattern (b, n * b->slen);
}

/*  int bReverse (bstring b)
 *
 *  Reverse the contents of b in place.
 */
int bReverse (bstring b) {
int i, n, m;
unsigned char t;

    if (b == NULL || b->slen < 0 || b->mlen < b->slen) return -__LINE__;
    n = b->slen;
    if (2 <= n) {
        m = ((unsigned)n) >> 1;
        n--;
        for (i=0; i < m; i++) {
            t = b->data[n - i];
            b->data[n - i] = b->data[i];
            b->data[i] = t;
        }
    }
    return 0;
}

/*  int bInsertChrs (bstring b, int pos, int len, unsigned char c, unsigned char fill)
 *
 *  Insert a repeated sequence of a given character into the string at 
 *  position pos for a length len.
 */
int bInsertChrs (bstring b, int pos, int len, unsigned char c, unsigned char fill) {
    if (b == NULL || b->slen < 0 || b->mlen < b->slen || pos < 0 || len <= 0) return -__LINE__;

    if (pos > b->slen 
     && 0 > bsetstr (b, pos, NULL, fill)) return -__LINE__;

    if (0 > balloc (b, b->slen + len)) return -__LINE__;
    if (pos < b->slen) memmove (b->data + pos + len, b->data + pos, b->slen - pos);
    memset (b->data + pos, c, len);
    b->slen += len;
    b->data[b->slen] = (unsigned char) '\0';
    return BSTR_OK;
}

/*  int bJustifyLeft (bstring b, int space)
 *
 *  Left justify a string.
 */
int bJustifyLeft (bstring b, int space) {
int j, i, s, t;
unsigned char c = (unsigned char) space;

    if (b == NULL || b->slen < 0 || b->mlen < b->slen) return -__LINE__;
    if (space != (int) c) return BSTR_OK;

    for (s=j=i=0; i < b->slen; i++) {
        t = s;
        s = c != (b->data[j] = b->data[i]);
        j += (t|s);
    }
    if (j > 0 && b->data[j-1] == c) j--;

    b->data[j] = (unsigned char) '\0';
    b->slen = j;
    return BSTR_OK;
}

/*  int bJustifyRight (bstring b, int width, int space)
 *
 *  Right justify a string to within a given width.
 */
int bJustifyRight (bstring b, int width, int space) {
int ret;
    if (width <= 0) return -__LINE__;
    if (0 > (ret = bJustifyLeft (b, space))) return ret;
    if (b->slen <= width)
        return bInsertChrs (b, 0, width - b->slen, (unsigned char) space, (unsigned char) space);
    return BSTR_OK;
}

/*  int bJustifyCenter (bstring b, int width, int space)
 *
 *  Center a string's non-white space characters to within a given width by
 *  inserting whitespaces at the beginning.
 */
int bJustifyCenter (bstring b, int width, int space) {
int ret;
    if (width <= 0) return -__LINE__;
    if (0 > (ret = bJustifyLeft (b, space))) return ret;
    if (b->slen <= width)
        return bInsertChrs (b, 0, (width - b->slen + 1) >> 1, (unsigned char) space, (unsigned char) space);
    return BSTR_OK;
}

/*  int bJustifyMargin (bstring b, int width, int space)
 *
 *  Stretch a string to flush against left and right margins by evenly
 *  distributing additional white space between words.  If the line is too
 *  long to be margin justified, it is left justified.
 */
int bJustifyMargin (bstring b, int width, int space) {
struct bstrList * sl;
int i, l, c;

    if (b == NULL || b->slen < 0 || b->mlen == 0 || b->mlen < b->slen) return -__LINE__;
    if (NULL == (sl = bsplit (b, (unsigned char) space))) return -__LINE__;
    for (l=c=i=0; i < sl->qty; i++) {
        if (sl->entry[i]->slen > 0) {
            c ++;
            l += sl->entry[i]->slen;
        }
    }

    if (l + c >= width || c < 2) {
        bstrListDestroy (sl);
        return bJustifyLeft (b, space);
    }

    b->slen = 0;
    for (i=0; i < sl->qty; i++) {
        if (sl->entry[i]->slen > 0) {
            if (b->slen > 0) {
                int s = (width - l + (c / 2)) / c;
                bInsertChrs (b, b->slen, s, (unsigned char) space, (unsigned char) space);
                l += s;
            }
            bconcat (b, sl->entry[i]);
            c--;
            if (c <= 0) break;
        }
    }

    bstrListDestroy (sl);
    return BSTR_OK;
}

static size_t readNothing (void *buff, size_t elsize, size_t nelem, void *parm) {
    buff = buff;
    elsize = elsize;
    nelem = nelem;
    parm = parm;
    return 0; /* Immediately indicate EOF. */
}

/*  struct bStream * bsFromBstr (const_bstring b);
 *
 *  Create a bStream whose contents are a copy of the bstring passed in.
 *  This allows the use of all the bStream APIs with bstrings.
 */
struct bStream * bsFromBstr (const_bstring b) {
struct bStream * s = bsopen ((bNread) readNothing, NULL);
    bsunread (s, b); /* Push the bstring data into the empty bStream. */
    return s;
}

static size_t readRef (void *buff, size_t elsize, size_t nelem, void *parm) {
struct tagbstring * t = (struct tagbstring *) parm;
size_t tsz = elsize * nelem;

    if (tsz > (size_t) t->slen) tsz = (size_t) t->slen;
    if (tsz > 0) {
        memcpy (buff, t->data, tsz);
        t->slen -= (int) tsz;
        t->data += tsz;
        return tsz / elsize;
    }
    return 0;
}

/*  The "by reference" version of the above function.  This function puts
 *  a number of restrictions on the call site (the passed in struct 
 *  tagbstring *will* be modified by this function, and the source data
 *  must remain alive and constant for the lifetime of the bStream).  
 *  Hence it is not presented as an extern.
 */
static struct bStream * bsFromBstrRef (struct tagbstring * t) {
    if (!t) return NULL;
    return bsopen ((bNread) readRef, t);
}

/*  char * bStr2NetStr (const_bstring b)
 *
 *  Convert a bstring to a netstring.  See 
 *  http://cr.yp.to/proto/netstrings.txt for a description of netstrings.
 *  Note: 1) The value returned should be freed with a call to bcstrfree() at 
 *           the point when it will no longer be referenced to avoid a memory 
 *           leak.
 *        2) If the returned value is non-NULL, then it also '\0' terminated
 *           in the character position one past the "," terminator.
 */
char * bStr2NetStr (const_bstring b) {
char strnum[sizeof (b->slen) * 3 + 1];
bstring s;
unsigned char * buff;

    if (b == NULL || b->data == NULL || b->slen < 0) return NULL;
    sprintf (strnum, "%d:", b->slen);
    if (NULL == (s = bfromcstr (strnum))
     || bconcat (s, b) == BSTR_ERR || bconchar (s, (char) ',') == BSTR_ERR) {
        bdestroy (s);
        return NULL;
    }
    buff = s->data;
    bcstrfree ((char *) s);
    return (char *) buff;
}

/*  bstring bNetStr2Bstr (const char * buf)
 *
 *  Convert a netstring to a bstring.  See 
 *  http://cr.yp.to/proto/netstrings.txt for a description of netstrings.
 *  Note that the terminating "," *must* be present, however a following '\0'
 *  is *not* required.
 */
bstring bNetStr2Bstr (const char * buff) {
int i, x;
bstring b;
    if (buff == NULL) return NULL;
    x = 0;
    for (i=0; buff[i] != ':'; i++) {
        unsigned int v = buff[i] - '0';
        if (v > 9 || x > ((INT_MAX - (signed int)v) / 10)) return NULL;
        x = (x * 10) + v;
    }

    /* This thing has to be properly terminated */
    if (buff[i + 1 + x] != ',') return NULL;

    if (NULL == (b = bfromcstr (""))) return NULL;
    if (balloc (b, x + 1) != BSTR_OK)  {
        bdestroy (b);
        return NULL;
    }
    memcpy (b->data, buff + i + 1, x);
    b->data[x] = (unsigned char) '\0';
    b->slen = x;
    return b;
}

static char b64ETable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*  bstring bBase64Encode (const_bstring b)
 *
 *  Generate a base64 encoding.  See: RFC1341
 */
bstring bBase64Encode (const_bstring b) {
int i, c0, c1, c2, c3;
bstring out;

    if (b == NULL || b->slen < 0 || b->data == NULL) return NULL;

    out = bfromcstr ("");
    for (i=0; i + 2 < b->slen; i += 3) {
        if (i && ((i % 57) == 0)) {
            if (bconchar (out, (char) '\015') < 0 || bconchar (out, (char) '\012') < 0) {
                bdestroy (out);
                return NULL;
            }
        }
        c0 = b->data[i] >> 2;
        c1 = ((b->data[i] << 4) |
              (b->data[i+1] >> 4)) & 0x3F;
        c2 = ((b->data[i+1] << 2) |
              (b->data[i+2] >> 6)) & 0x3F;
        c3 = b->data[i+2] & 0x3F;
        if (bconchar (out, b64ETable[c0]) < 0 ||
            bconchar (out, b64ETable[c1]) < 0 ||
            bconchar (out, b64ETable[c2]) < 0 ||
            bconchar (out, b64ETable[c3]) < 0) {
            bdestroy (out);
            return NULL;
        }
    }

    if (i && ((i % 57) == 0)) {
        if (bconchar (out, (char) '\015') < 0 || bconchar (out, (char) '\012') < 0) {
            bdestroy (out);
            return NULL;
        }
    }

    switch (i + 2 - b->slen) {
        case 0:    c0 = b->data[i] >> 2;
                c1 = ((b->data[i] << 4) |
                      (b->data[i+1] >> 4)) & 0x3F;
                c2 = (b->data[i+1] << 2) & 0x3F;
            if (bconchar (out, b64ETable[c0]) < 0 ||
                bconchar (out, b64ETable[c1]) < 0 ||
                bconchar (out, b64ETable[c2]) < 0 ||
                bconchar (out, (char) '=') < 0) {
                bdestroy (out);
                return NULL;
            }
            break;
        case 1:    c0 =  b->data[i] >> 2;
                c1 = (b->data[i] << 4) & 0x3F;
            if (bconchar (out, b64ETable[c0]) < 0 ||
                bconchar (out, b64ETable[c1]) < 0 ||
                bconchar (out, (char) '=') < 0 ||
                bconchar (out, (char) '=') < 0) {
                bdestroy (out);
                return NULL;
            }
            break;
        case 2: break;
    }

    return out;
}

#define B64_PAD (-2)
#define B64_ERR (-1)

static int base64DecodeSymbol (unsigned char alpha) {
   if      ((alpha >= 'A') && (alpha <= 'Z')) return (int)(alpha - 'A');
   else if ((alpha >= 'a') && (alpha <= 'z'))
        return 26 + (int)(alpha - 'a');
   else if ((alpha >= '0') && (alpha <= '9'))
        return 52 + (int)(alpha - '0');
   else if (alpha == '+') return 62;
   else if (alpha == '/') return 63;
   else if (alpha == '=') return B64_PAD;
   else                   return B64_ERR;
}

/*  bstring bBase64DecodeEx (const_bstring b, int * boolTruncError)
 *
 *  Decode a base64 block of data.  All MIME headers are assumed to have been
 *  removed.  See: RFC1341
 */
bstring bBase64DecodeEx (const_bstring b, int * boolTruncError) {
int i, v;
unsigned char c0, c1, c2;
bstring out;

    if (b == NULL || b->slen < 0 || b->data == NULL) return NULL;
    if (boolTruncError) *boolTruncError = 0;
    out = bfromcstr ("");
    i = 0;
    for (;;) {
        do {
            if (i >= b->slen) return out;
            if (b->data[i] == '=') {    /* Bad "too early" truncation */
                if (boolTruncError) {
                    *boolTruncError = 1;
                    return out;
                }
                bdestroy (out);
                return NULL;
            }
            v = base64DecodeSymbol (b->data[i]);
            i++;
        } while (v < 0);
        c0 = (unsigned char) (v << 2);
        do {
            if (i >= b->slen || b->data[i] == '=') {    /* Bad "too early" truncation */
                if (boolTruncError) {
                    *boolTruncError = 1;
                    return out;
                }
                bdestroy (out);
                return NULL;
            }
            v = base64DecodeSymbol (b->data[i]);
            i++;
        } while (v < 0);
        c0 |= (unsigned char) (v >> 4);
        c1  = (unsigned char) (v << 4);
        do {
            if (i >= b->slen) {
                if (boolTruncError) {
                    *boolTruncError = 1;
                    return out;
                }
                bdestroy (out);
                return NULL;
            }
            if (b->data[i] == '=') {
                i++;
                if (i >= b->slen || b->data[i] != '=' || bconchar (out, c0) < 0) {
                    if (boolTruncError) {
                        *boolTruncError = 1;
                        return out;
                    }
                    bdestroy (out); /* Missing "=" at the end. */
                    return NULL;
                }
                return out;
            }
            v = base64DecodeSymbol (b->data[i]);
            i++;
        } while (v < 0);
        c1 |= (unsigned char) (v >> 2);
        c2  = (unsigned char) (v << 6);
        do {
            if (i >= b->slen) {
                if (boolTruncError) {
                    *boolTruncError = 1;
                    return out;
                }
                bdestroy (out);
                return NULL;
            }
            if (b->data[i] == '=') {
                if (bconchar (out, c0) < 0 || bconchar (out, c1) < 0) {
                    if (boolTruncError) {
                        *boolTruncError = 1;
                        return out;
                    }
                    bdestroy (out);
                    return NULL;
                }
                if (boolTruncError) *boolTruncError = 0;
                return out;
            }
            v = base64DecodeSymbol (b->data[i]);
            i++;
        } while (v < 0);
        c2 |= (unsigned char) (v);
        if (bconchar (out, c0) < 0 ||
            bconchar (out, c1) < 0 ||
            bconchar (out, c2) < 0) {
            if (boolTruncError) {
                *boolTruncError = -1;
                return out;
            }
            bdestroy (out);
            return NULL;
        }
    }
}

#define UU_DECODE_BYTE(b) (((b) == (signed int)'`') ? 0 : (b) - (signed int)' ')

struct bUuInOut {
    bstring src, dst;
    int * badlines;
};

#define UU_MAX_LINELEN 45

static int bUuDecLine (void * parm, int ofs, int len) {
struct bUuInOut * io = (struct bUuInOut *) parm;
bstring s = io->src;
bstring t = io->dst;
int i, llen, otlen, ret, c0, c1, c2, c3, d0, d1, d2, d3;

    if (len == 0) return 0;
    llen = UU_DECODE_BYTE (s->data[ofs]);
    ret = 0;

    otlen = t->slen;

    if (((unsigned) llen) > UU_MAX_LINELEN) { ret = -__LINE__; 
        goto bl;
    }

    llen += t->slen;

    for (i=1; i < s->slen && t->slen < llen;i += 4) {
        unsigned char outoctet[3];
        c0 = UU_DECODE_BYTE (d0 = (int) bchare (s, i+ofs+0, ' ' - 1));
        c1 = UU_DECODE_BYTE (d1 = (int) bchare (s, i+ofs+1, ' ' - 1));
        c2 = UU_DECODE_BYTE (d2 = (int) bchare (s, i+ofs+2, ' ' - 1));
        c3 = UU_DECODE_BYTE (d3 = (int) bchare (s, i+ofs+3, ' ' - 1));

        if (((unsigned) (c0|c1) >= 0x40)) { if (!ret) ret = -__LINE__;
            if (d0 > 0x60 || (d0 < (' ' - 1) && !isspace (d0)) ||
                d1 > 0x60 || (d1 < (' ' - 1) && !isspace (d1))) {
                t->slen = otlen;
                goto bl;
            }
            c0 = c1 = 0;
        }
        outoctet[0] = (unsigned char) ((c0 << 2) | ((unsigned) c1 >> 4));
        if (t->slen+1 >= llen) {
            if (0 > bconchar (t, (char) outoctet[0])) return -__LINE__;
            break;
        }
        if ((unsigned) c2 >= 0x40) { if (!ret) ret = -__LINE__;
            if (d2 > 0x60 || (d2 < (' ' - 1) && !isspace (d2))) {
                t->slen = otlen;
                goto bl;
            }
            c2 = 0;
        }
        outoctet[1] = (unsigned char) ((c1 << 4) | ((unsigned) c2 >> 2));
        if (t->slen+2 >= llen) {
            if (0 > bcatblk (t, outoctet, 2)) return -__LINE__;
            break;
        }
        if ((unsigned) c3 >= 0x40) { if (!ret) ret = -__LINE__;
            if (d3 > 0x60 || (d3 < (' ' - 1) && !isspace (d3))) {
                t->slen = otlen;
                goto bl;
            }
            c3 = 0;
        }
        outoctet[2] = (unsigned char) ((c2 << 6) | ((unsigned) c3));
        if (0 > bcatblk (t, outoctet, 3)) return -__LINE__;
    }
    if (t->slen < llen) { if (0 == ret) ret = -__LINE__;
        t->slen = otlen;
    }
    bl:;
    if (ret && io->badlines) {
        (*io->badlines)++;
        return 0;
    }
    return ret;
}

/*  bstring bUuDecodeEx (const_bstring src, int * badlines)
 *
 *  Performs a UUDecode of a block of data.  If there are errors in the
 *  decoding, they are counted up and returned in "badlines", if badlines is
 *  not NULL. It is assumed that the "begin" and "end" lines have already 
 *  been stripped off.  The potential security problem of writing the 
 *  filename in the begin line is something that is beyond the scope of a 
 *  portable library.
 */

#ifdef _MSC_VER
#pragma warning(disable:4204)
#endif

bstring bUuDecodeEx (const_bstring src, int * badlines) {
struct tagbstring t;
struct bStream * s;
struct bStream * d;
bstring b;

    if (!src) return NULL;
    t = *src; /* Short lifetime alias to header of src */
    s = bsFromBstrRef (&t); /* t is undefined after this */
    if (!s) return NULL;
    d = bsUuDecode (s, badlines);
    b = bfromcstralloc (256, "");
    if (NULL == b || 0 > bsread (b, d, INT_MAX)) {
        bdestroy (b);
        bsclose (d);
        bsclose (s);
        return NULL;
    }
    return b;
}

struct bsUuCtx {
    struct bUuInOut io;
    struct bStream * sInp;
};

static size_t bsUuDecodePart (void *buff, size_t elsize, size_t nelem, void *parm) {
static struct tagbstring eol = bsStatic ("\r\n");
struct bsUuCtx * luuCtx = (struct bsUuCtx *) parm;
size_t tsz;
int l, lret;

    if (NULL == buff || NULL == parm) return 0;
    tsz = elsize * nelem;

    CheckInternalBuffer:;
    /* If internal buffer has sufficient data, just output it */
    if (((size_t) luuCtx->io.dst->slen) > tsz) {
        memcpy (buff, luuCtx->io.dst->data, tsz);
        bdelete (luuCtx->io.dst, 0, (int) tsz);
        return nelem;
    }

    DecodeMore:;
    if (0 <= (l = binchr (luuCtx->io.src, 0, &eol))) {
        int ol = 0;
        struct tagbstring t;
        bstring s = luuCtx->io.src;
        luuCtx->io.src = &t;

        do {
            if (l > ol) {
                bmid2tbstr (t, s, ol, l - ol);
                lret = bUuDecLine (&luuCtx->io, 0, t.slen);
                if (0 > lret) {
                    luuCtx->io.src = s;
                    goto Done;
                }
            }
            ol = l + 1;
            if (((size_t) luuCtx->io.dst->slen) > tsz) break;
            l = binchr (s, ol, &eol);
        } while (BSTR_ERR != l);
        bdelete (s, 0, ol);
        luuCtx->io.src = s;
        goto CheckInternalBuffer;
    }

    if (BSTR_ERR != bsreada (luuCtx->io.src, luuCtx->sInp, bsbufflength (luuCtx->sInp, BSTR_BS_BUFF_LENGTH_GET))) {
        goto DecodeMore;
    }

    bUuDecLine (&luuCtx->io, 0, luuCtx->io.src->slen);

    Done:;
    /* Output any lingering data that has been translated */
    if (((size_t) luuCtx->io.dst->slen) > 0) {
        if (((size_t) luuCtx->io.dst->slen) > tsz) goto CheckInternalBuffer;
        memcpy (buff, luuCtx->io.dst->data, luuCtx->io.dst->slen);
        tsz = luuCtx->io.dst->slen / elsize;
        luuCtx->io.dst->slen = 0;
        if (tsz > 0) return tsz;
    }

    /* Deallocate once EOF becomes triggered */
    bdestroy (luuCtx->io.dst);
    bdestroy (luuCtx->io.src);
    free (luuCtx);
    return 0;
}

/*  bStream * bsUuDecode (struct bStream * sInp, int * badlines)
 *
 *  Creates a bStream which performs the UUDecode of an an input stream.  If
 *  there are errors in the decoding, they are counted up and returned in 
 *  "badlines", if badlines is not NULL. It is assumed that the "begin" and 
 *  "end" lines have already been stripped off.  The potential security 
 *  problem of writing the filename in the begin line is something that is 
 *  beyond the scope of a portable library.
 */

struct bStream * bsUuDecode (struct bStream * sInp, int * badlines) {
struct bsUuCtx * luuCtx = (struct bsUuCtx *) malloc (sizeof (struct bsUuCtx));
struct bStream * sOut;

    if (NULL == luuCtx) return NULL;

    luuCtx->io.src = bfromcstr ("");
    luuCtx->io.dst = bfromcstr ("");
    if (NULL == luuCtx->io.dst || NULL == luuCtx->io.src) {
        CleanUpFailureToAllocate:;
        bdestroy (luuCtx->io.dst);
        bdestroy (luuCtx->io.src);
        free (luuCtx);
        return NULL;
    }
    luuCtx->io.badlines = badlines;
    if (badlines) *badlines = 0;

    luuCtx->sInp = sInp;

    sOut = bsopen ((bNread) bsUuDecodePart, luuCtx);
    if (NULL == sOut) goto CleanUpFailureToAllocate;
    return sOut;
}

#define UU_ENCODE_BYTE(b) (char) (((b) == 0) ? '`' : ((b) + ' '))

/*  bstring bUuEncode (const_bstring src)
 *
 *  Performs a UUEncode of a block of data.  The "begin" and "end" lines are 
 *  not appended.
 */
bstring bUuEncode (const_bstring src) {
bstring out;
int i, j, jm;
unsigned int c0, c1, c2;
    if (src == NULL || src->slen < 0 || src->data == NULL) return NULL;
    if ((out = bfromcstr ("")) == NULL) return NULL;
    for (i=0; i < src->slen; i += UU_MAX_LINELEN) {
        if ((jm = i + UU_MAX_LINELEN) > src->slen) jm = src->slen;
        if (bconchar (out, UU_ENCODE_BYTE (jm - i)) < 0) {
            bstrFree (out);
            break;
        }
        for (j = i; j < jm; j += 3) {
            c0 = (unsigned int) bchar (src, j    );
            c1 = (unsigned int) bchar (src, j + 1);
            c2 = (unsigned int) bchar (src, j + 2);
            if (bconchar (out, UU_ENCODE_BYTE ( (c0 & 0xFC) >> 2)) < 0 ||
                bconchar (out, UU_ENCODE_BYTE (((c0 & 0x03) << 4) | ((c1 & 0xF0) >> 4))) < 0 ||
                bconchar (out, UU_ENCODE_BYTE (((c1 & 0x0F) << 2) | ((c2 & 0xC0) >> 6))) < 0 ||
                bconchar (out, UU_ENCODE_BYTE ( (c2 & 0x3F))) < 0) {
                bstrFree (out);
                goto End;
            }
        }
        if (bconchar (out, (char) '\r') < 0 || bconchar (out, (char) '\n') < 0) {
            bstrFree (out);
            break;
        }
    }
    End:;
    return out;
}

/*  bstring bYEncode (const_bstring src)
 *
 *  Performs a YEncode of a block of data.  No header or tail info is 
 *  appended.  See: http://www.yenc.org/whatis.htm and 
 *  http://www.yenc.org/yenc-draft.1.3.txt
 */
bstring bYEncode (const_bstring src) {
int i;
bstring out;
unsigned char c;

    if (src == NULL || src->slen < 0 || src->data == NULL) return NULL;
    if ((out = bfromcstr ("")) == NULL) return NULL;
    for (i=0; i < src->slen; i++) {
        c = (unsigned char)(src->data[i] + 42);
        if (c == '=' || c == '\0' || c == '\r' || c == '\n') {
            if (0 > bconchar (out, (char) '=')) {
                bdestroy (out);
                return NULL;
            }
            c += (unsigned char) 64;
        }
        if (0 > bconchar (out, c)) {
            bdestroy (out);
            return NULL;
        }
    }
    return out;
}

/*  bstring bYDecode (const_bstring src)
 *
 *  Performs a YDecode of a block of data.  See: 
 *  http://www.yenc.org/whatis.htm and http://www.yenc.org/yenc-draft.1.3.txt
 */
#define MAX_OB_LEN (64)

bstring bYDecode (const_bstring src) {
int i;
bstring out;
unsigned char c;
unsigned char octetbuff[MAX_OB_LEN];
int obl;

    if (src == NULL || src->slen < 0 || src->data == NULL) return NULL;
    if ((out = bfromcstr ("")) == NULL) return NULL;

    obl = 0;

    for (i=0; i < src->slen; i++) {
        if ('=' == (c = src->data[i])) { /* The = escape mode */
            i++;
            if (i >= src->slen) {
                bdestroy (out);
                return NULL;
            }
            c = (unsigned char) (src->data[i] - 64);
        } else {
            if ('\0' == c) {
                bdestroy (out);
                return NULL;
            }

            /* Extraneous CR/LFs are to be ignored. */
            if (c == '\r' || c == '\n') continue;
        }

        octetbuff[obl] = (unsigned char) ((int) c - 42);
        obl++;

        if (obl >= MAX_OB_LEN) {
            if (0 > bcatblk (out, octetbuff, obl)) {
                bdestroy (out);
                return NULL;
            }
            obl = 0;
        }
    }

    if (0 > bcatblk (out, octetbuff, obl)) {
        bdestroy (out);
        out = NULL;
    }
    return out;
}

/*  bstring bStrfTime (const char * fmt, const struct tm * timeptr)
 *
 *  Takes a format string that is compatible with strftime and a struct tm
 *  pointer, formats the time according to the format string and outputs
 *  the bstring as a result. Note that if there is an early generation of a 
 *  '\0' character, the bstring will be truncated to this end point.
 */
bstring bStrfTime (const char * fmt, const struct tm * timeptr) {
#if defined (__TURBOC__) && !defined (__BORLANDC__)
static struct tagbstring ns = bsStatic ("bStrfTime Not supported");
    fmt = fmt;
    timeptr = timeptr;
    return &ns;
#else
bstring buff;
int n;
size_t r;

    if (fmt == NULL) return NULL;

    /* Since the length is not determinable beforehand, a search is
       performed using the truncating "strftime" call on increasing 
       potential sizes for the output result. */

    if ((n = (int) (2*strlen (fmt))) < 16) n = 16;
    buff = bfromcstralloc (n+2, "");

    for (;;) {
        if (BSTR_OK != balloc (buff, n + 2)) {
            bdestroy (buff);
            return NULL;
        }

        r = strftime ((char *) buff->data, n + 1, fmt, timeptr);

        if (r > 0) {
            buff->slen = (int) r;
            break;
        }

        n += n;
    }

    return buff;
#endif
}

/*  int bSetCstrChar (bstring a, int pos, char c)
 *
 *  Sets the character at position pos to the character c in the bstring a.
 *  If the character c is NUL ('\0') then the string is truncated at this
 *  point.  Note: this does not enable any other '\0' character in the bstring
 *  as terminator indicator for the string.  pos must be in the position 
 *  between 0 and b->slen inclusive, otherwise BSTR_ERR will be returned.
 */
int bSetCstrChar (bstring b, int pos, char c) {
    if (NULL == b || b->mlen <= 0 || b->slen < 0 || b->mlen < b->slen)
        return BSTR_ERR;
    if (pos < 0 || pos > b->slen) return BSTR_ERR;

    if (pos == b->slen) {
        if ('\0' != c) return bconchar (b, c);
        return 0;
    }

    b->data[pos] = (unsigned char) c;
    if ('\0' == c) b->slen = pos;

    return 0;
}

/*  int bSetChar (bstring b, int pos, char c)
 *
 *  Sets the character at position pos to the character c in the bstring a.
 *  The string is not truncated if the character c is NUL ('\0').  pos must
 *  be in the position between 0 and b->slen inclusive, otherwise BSTR_ERR
 *  will be returned.
 */
int bSetChar (bstring b, int pos, char c) {
    if (NULL == b || b->mlen <= 0 || b->slen < 0 || b->mlen < b->slen)
        return BSTR_ERR;
    if (pos < 0 || pos > b->slen) return BSTR_ERR;

    if (pos == b->slen) {
        return bconchar (b, c);
    }

    b->data[pos] = (unsigned char) c;
    return 0;
}

#define INIT_SECURE_INPUT_LENGTH (256)

/*  bstring bSecureInput (int maxlen, int termchar, 
 *                        bNgetc vgetchar, void * vgcCtx)
 *
 *  Read input from an abstracted input interface, for a length of at most
 *  maxlen characters.  If maxlen <= 0, then there is no length limit put
 *  on the input.  The result is terminated early if vgetchar() return EOF
 *  or the user specified value termchar.
 *
 */
bstring bSecureInput (int maxlen, int termchar, bNgetc vgetchar, void * vgcCtx) {
int i, m, c;
bstring b, t;

    if (!vgetchar) return NULL;

    b = bfromcstralloc (INIT_SECURE_INPUT_LENGTH, "");
    if ((c = UCHAR_MAX + 1) == termchar) c++;

    for (i=0; ; i++) {
        if (termchar == c || (maxlen > 0 && i >= maxlen)) c = EOF;
        else c = vgetchar (vgcCtx);

        if (EOF == c) break;

        if (i+1 >= b->mlen) {

            /* Double size, but deal with unusual case of numeric
               overflows */

            if ((m = b->mlen << 1)   <= b->mlen &&
                (m = b->mlen + 1024) <= b->mlen &&
                (m = b->mlen + 16)   <= b->mlen &&
                (m = b->mlen + 1)    <= b->mlen) t = NULL;
            else t = bfromcstralloc (m, "");

            if (t) memcpy (t->data, b->data, i);
            bSecureDestroy (b); /* Cleanse previous buffer */
            b = t;
            if (!b) return b;
        }

        b->data[i] = (unsigned char) c;
    }

    b->slen = i;
    b->data[i] = (unsigned char) '\0';
    return b;
}

#define BWS_BUFF_SZ (1024)

struct bwriteStream {
    bstring buff;    /* Buffer for underwrites                   */
    void * parm;     /* The stream handle for core stream        */
    bNwrite writeFn; /* fwrite work-a-like fnptr for core stream */
    int isEOF;       /* track stream's EOF state                 */
    int minBuffSz;
};

/*  struct bwriteStream * bwsOpen (bNwrite writeFn, void * parm)
 *
 *  Wrap a given open stream (described by a fwrite work-a-like function 
 *  pointer and stream handle) into an open bwriteStream suitable for write
 *  streaming functions.
 */
struct bwriteStream * bwsOpen (bNwrite writeFn, void * parm) {
struct bwriteStream * ws;

    if (NULL == writeFn) return NULL;
    ws = (struct bwriteStream *) malloc (sizeof (struct bwriteStream));
    if (ws) {
        if (NULL == (ws->buff = bfromcstr (""))) {
            free (ws);
            ws = NULL;
        } else {
            ws->parm = parm;
            ws->writeFn = writeFn;
            ws->isEOF = 0;
            ws->minBuffSz = BWS_BUFF_SZ;
        }
    }
    return ws;
}

#define internal_bwswriteout(ws,b) { \
    if ((b)->slen > 0) { \
        if (1 != (ws->writeFn ((b)->data, (b)->slen, 1, ws->parm))) { \
            ws->isEOF = 1; \
            return BSTR_ERR; \
        } \
    } \
}

/*  int bwsWriteFlush (struct bwriteStream * ws)
 *
 *  Force any pending data to be written to the core stream.
 */
int bwsWriteFlush (struct bwriteStream * ws) {
    if (NULL == ws || ws->isEOF || 0 >= ws->minBuffSz || 
        NULL == ws->writeFn || NULL == ws->buff) return BSTR_ERR;
    internal_bwswriteout (ws, ws->buff);
    ws->buff->slen = 0;
    return 0;
}

/*  int bwsWriteBstr (struct bwriteStream * ws, const_bstring b)
 *
 *  Send a bstring to a bwriteStream.  If the stream is at EOF BSTR_ERR is
 *  returned.  Note that there is no deterministic way to determine the exact
 *  cut off point where the core stream stopped accepting data.
 */
int bwsWriteBstr (struct bwriteStream * ws, const_bstring b) {
struct tagbstring t;
int l;

    if (NULL == ws || NULL == b || NULL == ws->buff ||
        ws->isEOF || 0 >= ws->minBuffSz || NULL == ws->writeFn)
        return BSTR_ERR;

    /* Buffer prepacking optimization */
    if (b->slen > 0 && ws->buff->mlen - ws->buff->slen > b->slen) {
        static struct tagbstring empty = bsStatic ("");
        if (0 > bconcat (ws->buff, b)) return BSTR_ERR;
        return bwsWriteBstr (ws, &empty);
    }

    if (0 > (l = ws->minBuffSz - ws->buff->slen)) {
        internal_bwswriteout (ws, ws->buff);
        ws->buff->slen = 0;
        l = ws->minBuffSz;
    }

    if (b->slen < l) return bconcat (ws->buff, b);

    if (0 > bcatblk (ws->buff, b->data, l)) return BSTR_ERR;
    internal_bwswriteout (ws, ws->buff);
    ws->buff->slen = 0;

    bmid2tbstr (t, (bstring) b, l, b->slen);

    if (t.slen >= ws->minBuffSz) {
        internal_bwswriteout (ws, &t);
        return 0;
    }

    return bassign (ws->buff, &t);
}

/*  int bwsWriteBlk (struct bwriteStream * ws, void * blk, int len)
 *
 *  Send a block of data a bwriteStream.  If the stream is at EOF BSTR_ERR is 
 *  returned.
 */
int bwsWriteBlk (struct bwriteStream * ws, void * blk, int len) {
struct tagbstring t;
    if (NULL == blk || len < 0) return BSTR_ERR;
    blk2tbstr (t, blk, len);
    return bwsWriteBstr (ws, &t);
}

/*  int bwsIsEOF (const struct bwriteStream * ws)
 *
 *  Returns 0 if the stream is currently writable, 1 if the core stream has 
 *  responded by not accepting the previous attempted write.
 */
int bwsIsEOF (const struct bwriteStream * ws) {
    if (NULL == ws || NULL == ws->buff || 0 > ws->minBuffSz || 
        NULL == ws->writeFn) return BSTR_ERR;
    return ws->isEOF;
}

/*  int bwsBuffLength (struct bwriteStream * ws, int sz)
 *
 *  Set the length of the buffer used by the bwsStream.  If sz is zero, the 
 *  length is not set.  This function returns with the previous length.
 */
int bwsBuffLength (struct bwriteStream * ws, int sz) {
int oldSz;
    if (ws == NULL || sz < 0) return BSTR_ERR;
    oldSz = ws->minBuffSz;
    if (sz > 0) ws->minBuffSz = sz;
    return oldSz;
}

/*  void * bwsClose (struct bwriteStream * s)
 *
 *  Close the bwriteStream, and return the handle to the stream that was 
 *  originally used to open the given stream.  Note that even if the stream
 *  is at EOF it still needs to be closed with a call to bwsClose.
 */
void * bwsClose (struct bwriteStream * ws) {
void * parm;
    if (NULL == ws || NULL == ws->buff || 0 >= ws->minBuffSz || 
        NULL == ws->writeFn) return NULL;
    bwsWriteFlush (ws);
    parm = ws->parm;
    ws->parm = NULL;
    ws->minBuffSz = -1;
    ws->writeFn = NULL;
    bstrFree (ws->buff);
    free (ws);
    return parm;
}

