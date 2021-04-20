/*
 * This source file is part of the bstring string library.  This code was
 * written by Paul Hsieh in 2002-2010, and is covered by either the 3-clause 
 * BSD open source license or GPL v2.0. Refer to the accompanying documentation 
 * for details on usage and license.
 */
// Modifications copyright Amazon.com, Inc. or its affiliates

/*
 * bstraux.h
 *
 * This file is not a necessary part of the core bstring library itself, but
 * is just an auxilliary module which includes miscellaneous or trivial 
 * functions.
 */

#ifndef BSTRAUX_INCLUDE
#define BSTRAUX_INCLUDE

#include <time.h>
#include "bstrlib.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Safety mechanisms */
#define bstrDeclare(b)               bstring (b) = NULL; 
#define bstrFree(b)                  {if ((b) != NULL && (b)->slen >= 0 && (b)->mlen >= (b)->slen) { bdestroy (b); (b) = NULL; }}

/* Backward compatibilty with previous versions of Bstrlib */
#define bAssign(a,b)                 ((bassign)((a), (b)))
#define bSubs(b,pos,len,a,c)         ((breplace)((b),(pos),(len),(a),(unsigned char)(c)))
#define bStrchr(b,c)                 ((bstrchr)((b), (c)))
#define bStrchrFast(b,c)             ((bstrchr)((b), (c)))
#define bCatCstr(b,s)                ((bcatcstr)((b), (s)))
#define bCatBlk(b,s,len)             ((bcatblk)((b),(s),(len)))
#define bCatStatic(b,s)              bCatBlk ((b), ("" s ""), sizeof (s) - 1)
#define bTrunc(b,n)                  ((btrunc)((b), (n)))
#define bReplaceAll(b,find,repl,pos) ((bfindreplace)((b),(find),(repl),(pos)))
#define bUppercase(b)                ((btoupper)(b))
#define bLowercase(b)                ((btolower)(b))
#define bCaselessCmp(a,b)            ((bstricmp)((a), (b)))
#define bCaselessNCmp(a,b,n)         ((bstrnicmp)((a), (b), (n)))
#define bBase64Decode(b)             (bBase64DecodeEx ((b), NULL))
#define bUuDecode(b)                 (bUuDecodeEx ((b), NULL))

/* Unusual functions */
extern struct bStream * bsFromBstr (const_bstring b);
extern bstring bTail (bstring b, int n);
extern bstring bHead (bstring b, int n);
extern int bSetCstrChar (bstring a, int pos, char c);
extern int bSetChar (bstring b, int pos, char c);
extern int bFill (bstring a, char c, int len);
extern int bReplicate (bstring b, int n);
extern int bReverse (bstring b);
extern int bInsertChrs (bstring b, int pos, int len, unsigned char c, unsigned char fill);
extern bstring bStrfTime (const char * fmt, const struct tm * timeptr);
#define bAscTime(t) (bStrfTime ("%c\n", (t)))
#define bCTime(t)   ((t) ? bAscTime (localtime (t)) : NULL)

/* Spacing formatting */
extern int bJustifyLeft (bstring b, int space);
extern int bJustifyRight (bstring b, int width, int space);
extern int bJustifyMargin (bstring b, int width, int space);
extern int bJustifyCenter (bstring b, int width, int space);

/* Esoteric standards specific functions */
extern char * bStr2NetStr (const_bstring b);
extern bstring bNetStr2Bstr (const char * buf);
extern bstring bBase64Encode (const_bstring b);
extern bstring bBase64DecodeEx (const_bstring b, int * boolTruncError);
extern struct bStream * bsUuDecode (struct bStream * sInp, int * badlines);
extern bstring bUuDecodeEx (const_bstring src, int * badlines);
extern bstring bUuEncode (const_bstring src);
extern bstring bYEncode (const_bstring src);
extern bstring bYDecode (const_bstring src);

/* Writable stream */
typedef int (* bNwrite) (const void * buf, size_t elsize, size_t nelem, void * parm);

struct bwriteStream * bwsOpen (bNwrite writeFn, void * parm);
int bwsWriteBstr (struct bwriteStream * stream, const_bstring b);
int bwsWriteBlk (struct bwriteStream * stream, void * blk, int len);
int bwsWriteFlush (struct bwriteStream * stream);
int bwsIsEOF (const struct bwriteStream * stream);
int bwsBuffLength (struct bwriteStream * stream, int sz);
void * bwsClose (struct bwriteStream * stream);

/* Security functions */
#define bSecureDestroy(b) {                                                \
bstring bstr__tmp = (b);                                                \
    if (bstr__tmp && bstr__tmp->mlen > 0 && bstr__tmp->data) {          \
        (void) memset (bstr__tmp->data, 0, (size_t) bstr__tmp->mlen);   \
        bdestroy (bstr__tmp);                                           \
    }                                                                   \
}
#define bSecureWriteProtect(t) {                                                  \
    if ((t).mlen >= 0) {                                                          \
        if ((t).mlen > (t).slen)) {                                               \
            (void) memset ((t).data + (t).slen, 0, (size_t) (t).mlen - (t).slen); \
        }                                                                         \
        (t).mlen = -1;                                                            \
    }                                                                             \
}
extern bstring bSecureInput (int maxlen, int termchar, 
                             bNgetc vgetchar, void * vgcCtx);

#ifdef __cplusplus
}
#endif

#endif
