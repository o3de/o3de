/*-
 * Copyright (c) 1998, 2002-2008 Kiyoshi Matsui <kmatsui@t3.rim.or.jp>
 * All rights reserved.
 *
 * Some parts of this code are derived from the public domain software
 * DECUS cpp (1984,1985) written by Martin Minow.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 *                          M B C H A R . C
 *      C h a r a c t e r    h a n d l i n g    R o u t i n e s
 *
 * Character handling and multi-byte character handling routines are
 * placed here.
 */

#if PREPROCESSED
#include    "mcpp.h"
#else
#include    "system.h"
#include    "internal.h"
#endif

/*
 * Tables of character types and multi-byte character types.
 *
 * Some of these character attributes will be overwritten by
 *      execution time option '-@post' or '-@old'.
 * Warning on erroneous sequence will be issued from the caller routines:
 * scan_quote(), scan_id() or scan_number().
 */

/* Non-ASCII characters are always checked by mb_read().    */
#define NA      0x4000  /* Non-ASCII characters */

/* Horizontal spaces (' ', '\t' and TOK_SEP)    */
#define HSPA    (SPA | HSP)

short *     char_type;  /* Pointer to one of the following type_*[].    */

#define EJ1     0x100   /* 1st byte of EUC_JP   */
#define EJ2     0x200   /* 2nd byte of EUC_JP   */
#define GB1     0x400   /* 1st byte of GB2312   */
#define GB2     0x800   /* 2nd byte of GB2312   */
#define KS1     0x1000  /* 1st byte of KSC5601  */
#define KS2     0x2000  /* 2nd byte of KSC5601  */

#define EJ12    (EJ1 | EJ2)     /* 1st byte or 2nd byte of EUC_JP   */
#define GB12    (GB1 | GB2)
#define KS12    (KS1 | KS2)
#define EJ1N    (NA | EJ1)
#define EU12N   (NA | EJ12 | GB12 | KS12)
    /* 1st or 2nd byte of EUC_JP, GB2312 or KSC5601, or any other non-ASCII */

static short    type_euc[ UCHARMAX + 1] = {
/*
 * For EUC_JP, GB2312, KSC5601 or other similar multi-byte char encodings.
 */

/* Character type codes */
/*   0,     1,     2,     3,     4,     5,     6,     7,                    */
/*   8,     9,     A,     B,     C,     D,     E,     F,       Hex          */

   000,   000,   000,   000,   000,   000,   000,   000,    /* 00           */
   000,   HSPA,  SPA,   SPA,   SPA,   SPA,   000,   000,    /* 08           */
   000,   000,   000,   000,   000,   000,   000,   000,    /* 10           */
    /* 0x17-0x1A and 0x1F will be cleared in some modes by chk_opts()       */
   000,   LET,   LET,   000,   000,   000,   000,   HSPA,   /* 18           */
   HSPA,  PUNC,  QUO,   PUNC,  000,   PUNC,  PUNC,  QUO,    /* 20  !"#$%&'  */
   PUNC,  PUNC,  PUNC,  PUNC,  PUNC,  PUNC,  DOT,   PUNC,   /* 28 ()*+,-./  */
   DIG,   DIG,   DIG,   DIG,   DIG,   DIG,   DIG,   DIG,    /* 30 01234567  */
   DIG,   DIG,   PUNC,  PUNC,  PUNC,  PUNC,  PUNC,  PUNC,   /* 38 89:;<=>?  */

   000,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 40 @ABCDEFG  */
   LET,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 48 HIJKLMNO  */
   LET,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 50 PQRSTUVW  */
   LET,   LET,   LET,   PUNC,  000,   PUNC,  PUNC,  LET,    /* 58 XYZ[\]^_  */
   000,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 60 `abcdefg  */
   LET,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 68 hijklmno  */
   LET,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 70 pqrstuvw  */
   LET,   LET,   LET,   PUNC,  PUNC,  PUNC,  PUNC,  000,    /* 78 xyz{|}~   */

   NA,    NA,    NA,    NA,    NA,    NA,    NA,    NA,     /*   80 .. 87   */
   NA,    NA,    NA,    NA,    NA,    NA,    EJ1N,  NA,     /*   88 .. 8F   */
   NA,    NA,    NA,    NA,    NA,    NA,    NA,    NA,     /*   90 .. 97   */
   NA,    NA,    NA,    NA,    NA,    NA,    NA,    NA,     /*   98 .. 9F   */
   NA,    EU12N, EU12N, EU12N, EU12N, EU12N, EU12N, EU12N,  /*   A0 .. A7   */
   EU12N, EU12N, EU12N, EU12N, EU12N, EU12N, EU12N, EU12N,  /*   A8 .. AF   */
   EU12N, EU12N, EU12N, EU12N, EU12N, EU12N, EU12N, EU12N,  /*   B0 .. B7   */
   EU12N, EU12N, EU12N, EU12N, EU12N, EU12N, EU12N, EU12N,  /*   B8 .. BF   */
   EU12N, EU12N, EU12N, EU12N, EU12N, EU12N, EU12N, EU12N,  /*   C0 .. C7   */
   EU12N, EU12N, EU12N, EU12N, EU12N, EU12N, EU12N, EU12N,  /*   C8 .. CF   */
   EU12N, EU12N, EU12N, EU12N, EU12N, EU12N, EU12N, EU12N,  /*   D0 .. D7   */
   EU12N, EU12N, EU12N, EU12N, EU12N, EU12N, EU12N, EU12N,  /*   D8 .. DF   */
   EU12N, EU12N, EU12N, EU12N, EU12N, EU12N, EU12N, EU12N,  /*   E0 .. E7   */
   EU12N, EU12N, EU12N, EU12N, EU12N, EU12N, EU12N, EU12N,  /*   E8 .. EF   */
   EU12N, EU12N, EU12N, EU12N, EU12N, EU12N, EU12N, EU12N,  /*   F0 .. F7   */
   EU12N, EU12N, EU12N, EU12N, EU12N, EU12N, EU12N, NA,     /*   F8 .. FF   */
};

static short    type_bsl[ UCHARMAX + 1] = {
/*
 * For SJIS, BIGFIVE or other similar encodings which may have '\\' value as
 * the second byte of multi-byte character.
 */

#define SJ1     0x100   /* 1st byte of SJIS     */
#define SJ2     0x200   /* 2nd byte of SJIS     */
#define BF1     0x400   /* 1st byte of BIGFIVE  */
#define BF2     0x800   /* 2nd byte of BIGFIVE  */

#define SB2     (SJ2 | BF2)
#define SJ2N    (NA | SJ2)
#define SB2N    (NA | SJ2 | BF2)
#define SJ12N   (NA | SJ1 | SJ2)
#define BF12N   (NA | BF1 | BF2)
#define SB12N   (NA | SJ1 | SJ2 | BF1 | BF2)
#define S2B12N  (NA | SJ2 | BF1 | BF2)

#define LSB2    (LET | SB2)
#define PSB2    (PUNC| SB2)

/* Character type codes */
/*   0,     1,     2,     3,     4,     5,     6,     7,                    */
/*   8,     9,     A,     B,     C,     D,     E,     F,       Hex          */

   000,   000,   000,   000,   000,   000,   000,   000,    /* 00           */
   000,   HSPA,  SPA,   SPA,   SPA,   SPA,   000,   000,    /* 08           */
   000,   000,   000,   000,   000,   000,   000,   000,    /* 10           */
    /* 0x17-0x1A and 0x1F will be cleared in some modes by chk_opts()       */
   000,   LET,   LET,   000,   000,   000,   000,   HSPA,   /* 18           */
   HSPA,  PUNC,  QUO,   PUNC,  000,   PUNC,  PUNC,  QUO,    /* 20  !"#$%&'  */
   PUNC,  PUNC,  PUNC,  PUNC,  PUNC,  PUNC,  DOT,   PUNC,   /* 28 ()*+,-./  */
   DIG,   DIG,   DIG,   DIG,   DIG,   DIG,   DIG,   DIG,    /* 30 01234567  */
   DIG,   DIG,   PUNC,  PUNC,  PUNC,  PUNC,  PUNC,  PUNC,   /* 38 89:;<=>?  */

   SB2,   LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,   /* 40 @ABCDEFG  */
   LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,   /* 48 HIJKLMNO  */
   LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,   /* 50 PQRSTUVW  */
   LSB2,  LSB2,  LSB2,  PSB2,  SB2,   PSB2,  PSB2,  LSB2,   /* 58 XYZ[\]^_  */
   SB2,   LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,   /* 60 `abcdefg  */
   LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,   /* 68 hijklmno  */
   LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,   /* 70 pqrstuvw  */
   LSB2,  LSB2,  LSB2,  PSB2,  PSB2,  PSB2,  PSB2,  000,    /* 78 xyz{|}~   */

   SB2N,  SJ12N, SJ12N, SJ12N, SJ12N, SJ12N, SJ12N, SJ12N,  /*   80 .. 87   */
   SJ12N, SJ12N, SJ12N, SJ12N, SJ12N, SJ12N, SJ12N, SJ12N,  /*   88 .. 8F   */
   SJ12N, SJ12N, SJ12N, SJ12N, SJ12N, SJ12N, SJ12N, SJ12N,  /*   90 .. 97   */
   SJ12N, SJ12N, SJ12N, SJ12N, SJ12N, SJ12N, SJ12N, SJ12N,  /*   98 .. 9F   */
   SJ2N,  S2B12N,S2B12N,S2B12N,S2B12N,S2B12N,S2B12N,S2B12N, /*   A0 .. A7   */
   S2B12N,S2B12N,S2B12N,S2B12N,S2B12N,S2B12N,S2B12N,S2B12N, /*   A8 .. AF   */
   S2B12N,S2B12N,S2B12N,S2B12N,S2B12N,S2B12N,S2B12N,S2B12N, /*   B0 .. B7   */
   S2B12N,S2B12N,S2B12N,S2B12N,S2B12N,S2B12N,S2B12N,S2B12N, /*   B8 .. BF   */
   S2B12N,S2B12N,S2B12N,S2B12N,S2B12N,S2B12N,S2B12N,S2B12N, /*   C0 .. C7   */
   S2B12N,S2B12N,S2B12N,S2B12N,S2B12N,S2B12N,S2B12N,S2B12N, /*   C8 .. CF   */
   S2B12N,S2B12N,S2B12N,S2B12N,S2B12N,S2B12N,S2B12N,S2B12N, /*   D0 .. D7   */
   S2B12N,S2B12N,S2B12N,S2B12N,S2B12N,S2B12N,S2B12N,S2B12N, /*   D8 .. DF   */
   SB12N, SB12N, SB12N, SB12N, SB12N, SB12N, SB12N, SB12N,  /*   E0 .. E7   */
   SB12N, SB12N, SB12N, SB12N, SB12N, SB12N, SB12N, SB12N,  /*   E8 .. EF   */
   SB12N, SB12N, SB12N, SB12N, SB12N, SB12N, SB12N, SB12N,  /*   F0 .. F7   */
   SB12N, SB12N, SB12N, SB12N, SB12N, BF12N, BF12N, NA,     /*   F8 .. FF   */
};

/*
 * For ISO2022_JP multi-byte character encoding.
 */

#define IS1     0x100   /* 1st byte of shift-sequence   */
#define IS2     0x200   /* 2nd byte of shift-sequence   */
#define IS3     0x400   /* 3rd byte of shift-sequence   */
#define IS4     0x800   /* 4th byte of shift-sequence   */
#define IJP     0x1000  /* 1st or 2nd byte of ISO-2022-JP (ISO-2022-JP1)    */

#define PIJP    (PUNC | IJP)
#define QIJP    (QUO | IJP)
#define DTJP    (DOT | IJP)
#define DGJP    (DIG | IJP)
#define LIJP    (LET | IJP)

#define JPS2    (IJP | IS2)
#define PJPS23  (PIJP | IS2 | IS3)
#define LJPS3   (LIJP | IS3)
#define LJPS4   (LIJP | IS4)

static short    type_iso2022_jp[ UCHARMAX + 1] = {

/* Character type codes */
/*   0,     1,     2,     3,     4,     5,     6,     7,                    */
/*   8,     9,     A,     B,     C,     D,     E,     F,       Hex          */

   000,   000,   000,   000,   000,   000,   000,   000,    /* 00           */
   000,   HSPA,  SPA,   SPA,   SPA,   SPA,   000,   000,    /* 08           */
   000,   000,   000,   000,   000,   000,   000,   000,    /* 10           */
    /* 0x17-0x1A and 0x1F will be cleared in some modes by chk_opts()       */
   000,   LET,   LET,   IS1,   000,   000,   000,   HSPA,   /* 18           */
   HSPA,  PIJP,  QIJP,  PIJP,  JPS2,  PIJP,  PIJP,  QIJP,   /* 20  !"#$%&'  */
   PJPS23,PIJP,  PIJP,  PIJP,  PIJP,  PIJP,  DTJP,  PIJP,   /* 28 ()*+,-./  */
   DGJP,  DGJP,  DGJP,  DGJP,  DGJP,  DGJP,  DGJP,  DGJP,   /* 30 01234567  */
   DGJP,  DGJP,  PIJP,  PIJP,  PIJP,  PIJP,  PIJP,  PIJP,   /* 38 89:;<=>?  */

   IJP,   LIJP,  LJPS3, LIJP,  LJPS4, LIJP,  LIJP,  LIJP,   /* 40 @ABCDEFG  */
   LIJP,  LIJP,  LIJP,  LIJP,  LIJP,  LIJP,  LIJP,  LIJP,   /* 48 HIJKLMNO  */
   LIJP,  LIJP,  LIJP,  LIJP,  LIJP,  LIJP,  LIJP,  LIJP,   /* 50 PQRSTUVW  */
   LIJP,  LIJP,  LIJP,  PIJP,  IJP,   PIJP,  PIJP,  LIJP,   /* 58 XYZ[\]^_  */
   IJP,   LIJP,  LIJP,  LIJP,  LIJP,  LIJP,  LIJP,  LIJP,   /* 60 `abcdefg  */
   LIJP,  LIJP,  LIJP,  LIJP,  LIJP,  LIJP,  LIJP,  LIJP,   /* 68 hijklmno  */
   LIJP,  LIJP,  LIJP,  LIJP,  LIJP,  LIJP,  LIJP,  LIJP,   /* 70 pqrstuvw  */
   LIJP,  LIJP,  LIJP,  PIJP,  PIJP,  PIJP,  PIJP,  000,    /* 78 xyz{|}~   */

   NA,    NA,    NA,    NA,    NA,    NA,    NA,    NA,     /*   80 .. 87   */
   NA,    NA,    NA,    NA,    NA,    NA,    NA,    NA,     /*   88 .. 8F   */
   NA,    NA,    NA,    NA,    NA,    NA,    NA,    NA,     /*   90 .. 97   */
   NA,    NA,    NA,    NA,    NA,    NA,    NA,    NA,     /*   98 .. 9F   */
   NA,    NA,    NA,    NA,    NA,    NA,    NA,    NA,     /*   A0 .. A7   */
   NA,    NA,    NA,    NA,    NA,    NA,    NA,    NA,     /*   A8 .. AF   */
   NA,    NA,    NA,    NA,    NA,    NA,    NA,    NA,     /*   B0 .. B7   */
   NA,    NA,    NA,    NA,    NA,    NA,    NA,    NA,     /*   B8 .. BF   */
   NA,    NA,    NA,    NA,    NA,    NA,    NA,    NA,     /*   C0 .. C7   */
   NA,    NA,    NA,    NA,    NA,    NA,    NA,    NA,     /*   C8 .. CF   */
   NA,    NA,    NA,    NA,    NA,    NA,    NA,    NA,     /*   D0 .. D7   */
   NA,    NA,    NA,    NA,    NA,    NA,    NA,    NA,     /*   D8 .. DF   */
   NA,    NA,    NA,    NA,    NA,    NA,    NA,    NA,     /*   E0 .. E7   */
   NA,    NA,    NA,    NA,    NA,    NA,    NA,    NA,     /*   E8 .. EF   */
   NA,    NA,    NA,    NA,    NA,    NA,    NA,    NA,     /*   F0 .. F7   */
   NA,    NA,    NA,    NA,    NA,    NA,    NA,    NA,     /*   F8 .. FF   */
};

/*
 * For UTF8 multi-byte character encoding.
 */

#define U2_1    0x100       /* 1st byte of 2-byte encoding of UTF8  */
#define U3_1    0x200       /* 1st byte of 3-byte encoding of UTF8  */
#define U4_1    0x400       /* 1st byte of 4-byte encoding of UTF8  */
#define UCONT   0x800   /* Continuation of a 2, 3, or 4 byte UTF8 sequence  */
#define U2_1N   (NA | U2_1)
#define U3_1N   (NA | U3_1)
#define U4_1N   (NA | U4_1)
#define UCONTN  (NA | UCONT)

static short    type_utf8[ UCHARMAX + 1] = {

/* Character type codes */
/*   0,     1,     2,     3,     4,     5,     6,     7,                    */
/*   8,     9,     A,     B,     C,     D,     E,     F,       Hex          */

   000,   000,   000,   000,   000,   000,   000,   000,    /* 00           */
   000,   HSPA,  SPA,   SPA,   SPA,   SPA,   000,   000,    /* 08           */
   000,   000,   000,   000,   000,   000,   000,   000,    /* 10           */
    /* 0x17-0x1A and 0x1F will be cleared in some modes by chk_opts()       */
   000,   LET,   LET,   000,   000,   000,   000,   HSPA,   /* 18           */
   HSPA,  PUNC,  QUO,   PUNC,  000,   PUNC,  PUNC,  QUO,    /* 20  !"#$%&'  */
   PUNC,  PUNC,  PUNC,  PUNC,  PUNC,  PUNC,  DOT,   PUNC,   /* 28 ()*+,-./  */
   DIG,   DIG,   DIG,   DIG,   DIG,   DIG,   DIG,   DIG,    /* 30 01234567  */
   DIG,   DIG,   PUNC,  PUNC,  PUNC,  PUNC,  PUNC,  PUNC,   /* 38 89:;<=>?  */

   000,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 40 @ABCDEFG  */
   LET,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 48 HIJKLMNO  */
   LET,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 50 PQRSTUVW  */
   LET,   LET,   LET,   PUNC,  000,   PUNC,  PUNC,  LET,    /* 58 XYZ[\]^_  */
   000,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 60 `abcdefg  */
   LET,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 68 hijklmno  */
   LET,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 70 pqrstuvw  */
   LET,   LET,   LET,   PUNC,  PUNC,  PUNC,  PUNC,  000,    /* 78 xyz{|}~   */

   UCONTN,UCONTN,UCONTN,UCONTN,UCONTN,UCONTN,UCONTN,UCONTN, /*   80 .. 87   */
   UCONTN,UCONTN,UCONTN,UCONTN,UCONTN,UCONTN,UCONTN,UCONTN, /*   88 .. 8F   */
   UCONTN,UCONTN,UCONTN,UCONTN,UCONTN,UCONTN,UCONTN,UCONTN, /*   90 .. 97   */
   UCONTN,UCONTN,UCONTN,UCONTN,UCONTN,UCONTN,UCONTN,UCONTN, /*   98 .. 9F   */
   UCONTN,UCONTN,UCONTN,UCONTN,UCONTN,UCONTN,UCONTN,UCONTN, /*   A0 .. A7   */
   UCONTN,UCONTN,UCONTN,UCONTN,UCONTN,UCONTN,UCONTN,UCONTN, /*   A8 .. AF   */
   UCONTN,UCONTN,UCONTN,UCONTN,UCONTN,UCONTN,UCONTN,UCONTN, /*   B0 .. B7   */
   UCONTN,UCONTN,UCONTN,UCONTN,UCONTN,UCONTN,UCONTN,UCONTN, /*   B8 .. BF   */
   NA,    NA,    U2_1N, U2_1N, U2_1N, U2_1N, U2_1N, U2_1N,  /*   C0 .. C7   */
   U2_1N, U2_1N, U2_1N, U2_1N, U2_1N, U2_1N, U2_1N, U2_1N,  /*   C8 .. CF   */
   U2_1N, U2_1N, U2_1N, U2_1N, U2_1N, U2_1N, U2_1N, U2_1N,  /*   D0 .. D7   */
   U2_1N, U2_1N, U2_1N, U2_1N, U2_1N, U2_1N, U2_1N, U2_1N,  /*   D8 .. DF   */
   U3_1N, U3_1N, U3_1N, U3_1N, U3_1N, U3_1N, U3_1N, U3_1N,  /*   E0 .. E7   */
   U3_1N, U3_1N, U3_1N, U3_1N, U3_1N, U3_1N, U3_1N, U3_1N,  /*   E8 .. EF   */
   U4_1N, U4_1N, U4_1N, U4_1N, U4_1N, NA,    NA,    NA,     /*   F0 .. F7   */
   NA,    NA,    NA,    NA,    NA,    NA,    NA,    NA,     /*   F8 .. FF   */
};

#define SETLOCALE       2       /* #pragma setlocale (not __setlocale)  */

#define NUM_ENCODING    8
#define NUM_ALIAS       6

/* Names of encoding recognized.  Table for search_encoding().  */
static const char * const   encoding_name[ NUM_ENCODING][ NUM_ALIAS] = {
    /* Visual C full, Visual C short
        , 4 miscellaneous  */
    { "english",    "c"
        , "c",      "en",   "latin",    "iso8859"},
    { "",     ""
        , "eucjp",  "euc",  "ujis",     ""},
    { "chinesesimplified",  "chs"
        , "gb2312", "cngb",     "euccn",    ""},
    { "korean",   "kor"
        , "ksc5601",    "ksx1001",  "wansung",  "euckr"},
    { "japanese", "jpn"
        , "sjis",   "shiftjis", "mskanji",  ""},
    { "chinesetraditional", "cht"
        , "bigfive",    "big5", "cnbig5",   "euctw"},
    { "",     ""
        , "iso2022jp",  "iso2022jp1",   "jis",  ""},
    { "",     ""
        , "utf8",   "utf",      "",     ""},
};

static int      mbstart;
static int      mb2;

static size_t   mb_read_2byte( int c1, char ** in_pp, char ** out_pp);
                /* For 2-byte encodings of mbchar   */
static const char *     search_encoding( char * norm, int alias);
                /* Search encoding_name[][] table   */
static void     strip_bar( char * string);
                /* Remove '_', '-' or '.' in the string */
static void     conv_case( char * name, char * lim, int upper);
                /* Convert to upper/lower case      */
static size_t   mb_read_iso2022_jp( int c1, char ** in_pp, char ** out_pp);
                /* For ISO2022_JP encoding          */
static size_t   mb_read_utf8( int c1, char ** in_pp, char ** out_pp);
                /* For UTF8 mbchar encoding         */

#define NAMLEN          20
#define UPPER           1               /* To upper */
#define LOWER           0               /* To lower */


const char *    set_encoding(
    char *  name,       /* Name of encoding specified   */
    char *  env,        /* Name of environment variable */
    int     pragma
        /* 2: #pragma setlocale, 1: #pragma __setlocale, 0: not #pragma */
)
/*
 * Search the encoding specified and re-initialize mbchar settings.
 */
{
    const char *    unknown_encoding
            = "Unknown encoding: %s%.0ld%.0s";          /* _W1_ */
    const char *    too_long
            = "Too long encoding name: %s%.0ld%.0s";    /* _E_  */
    const char *    loc = "";
    int     alias;
    char    norm[ NAMLEN];
            /*
             * Normalized name (removed 'xxxxx.', stripped '_', '-', '.'
             * and lowered.
             */

    if (strlen( name) >= NAMLEN) {
        if ((env || pragma) && (warn_level & 1)) {
            cwarn( too_long, name, 0L, NULL);
        } else {
            mcpp_fprintf( ERR, too_long, name);
            mcpp_fputc( '\n', ERR);
        }
    }
    strcpy( norm, name);
    if (norm[ 5] == '.')
        memmove( norm, norm + 5, strlen( norm + 5) + 1);
        /* Remove initial 'xxxxx.' as 'ja_JP.', 'en_US.' or any other   */
    conv_case( norm, norm + strlen( norm), LOWER);
    strip_bar( norm);

    if (strlen( name) == 0) {                       /* ""       */
        mbchar = MBCHAR;    /* Restore to the default encoding  */
    } else if (memcmp( norm, "iso8859", 7) == 0     /* iso8859* */
            || memcmp( norm, "latin", 5) == 0       /* latin*   */
            || memcmp( norm, "en", 2) == 0) {       /* en*      */
        mbchar = 0;                 /* No multi-byte character  */
    } else {
        alias = 2;
#if COMPILER == MSC
        if (pragma == SETLOCALE)        /* #pragma setlocale    */
            alias = 0;
#endif
        loc = search_encoding( norm, alias);        /* Search the name  */
    }
    if (loc == NULL) {
        if ((env || pragma) && (warn_level & 1)) {
            cwarn( unknown_encoding, name, 0L, NULL);
        } else {                        /* -m option            */
            mcpp_fprintf( ERR, unknown_encoding, name);
            mcpp_fputc( '\n', ERR);
        }
    } else {
        mb_init();                      /* Re-initialize        */
    }
    return  loc;
}

static const char * search_encoding(
    char *  norm,           /* The name of encoding specified   */
    int     alias           /* The number of alias to start searching   */
)
{
    const char *    loc;
    int             lo, al;

    for (lo = 0; lo < NUM_ENCODING; lo++) {
        for (al = alias ; al < NUM_ALIAS; al++) {
            loc = encoding_name[ lo][ al];
            if (str_eq( loc, norm)) {
                switch (lo) {
                case 0  :   mbchar = 0;             break;
                case 1  :   mbchar = EUC_JP;        break;
                case 2  :   mbchar = GB2312;        break;
                case 3  :   mbchar = KSC5601;       break;
                case 4  :   mbchar = SJIS;          break;
                case 5  :   mbchar = BIGFIVE;       break;
                case 6  :   mbchar = ISO2022_JP;    break;
                case 7  :   mbchar = UTF8;          break;
                }
                return  loc;
            }
        }
    }
    return  NULL;
}

static void strip_bar(
    char *  string
)
/*
 * Strip '_', '-' or '.' in the string.
 */
{
    char *  cp = string;

    while (*cp != EOS) {
        if (*cp == '_' || *cp == '-' || *cp == '.')
            memmove( cp, cp + 1, strlen( cp));
        else
            cp++;
    }
}

static void     conv_case(
    char *  name,                       /* (diretory) Name          */
    char *  lim,                        /* End of (directory) name  */
    int     upper                       /* TRUE if to upper         */
)
/* Convert a string to upper-case letters or lower-case letters in-place    */
{
    int     c;
    char *  sp;

    for (sp = name; sp < lim; sp++) {
        c = *sp & UCHARMAX;
#if MBCHAR
        if ((char_type[ c] & mbstart)) {
            char    tmp[ PATHMAX+1];
            char *  tp = tmp;
            *tp++ = *sp++;
            mb_read( c, &sp, &tp);
        } else
#endif
        {
            if (upper)
                *sp = toupper( c);
            else
                *sp = tolower( c);
        }
    }
}

void    mb_init( void)
/*
 * Initialize multi-byte character settings.
 * First called prior to setting the 'mcpp_mode'.
 * Will be called again each time the multibyte character encoding is changed.
 */
{
    /*
     * Select the character classification table, select the multi-byte
     * character reading routine and decide whether multi-byte character
     * may contain the byte of value 0x5c.
     */
    switch (mbchar) {
    case 0      :
    case EUC_JP     :
    case GB2312     :
    case KSC5601    :
        char_type = type_euc;
        bsl_in_mbchar = FALSE;
        mb_read = mb_read_2byte;
        break;
    case SJIS   :
    case BIGFIVE    :
        char_type = type_bsl;
        bsl_in_mbchar = TRUE;
        mb_read = mb_read_2byte;
        break;
    case ISO2022_JP :
        char_type = type_iso2022_jp;
        bsl_in_mbchar = TRUE;
        mb_read = mb_read_iso2022_jp;
        break;
    case UTF8   :
        char_type = type_utf8;
        bsl_in_mbchar = FALSE;
        mb_read = mb_read_utf8;
        break;
    }

    /* Set the bit patterns for character classification.   */
    switch (mbchar) {
    case 0      :
        mbstart = 0;
        break;
    case EUC_JP :
        mbstart = EJ1;
        mb2 = EJ2;
        break;
    case GB2312 :
        mbstart = GB1;
        mb2 = GB2;
        break;
    case KSC5601:
        mbstart = KS1;
        mb2 = KS2;
        break;
    case SJIS   :
        mbstart = SJ1;
        mb2 = SJ2;
        break;
    case BIGFIVE:
        mbstart = BF1;
        mb2 = BF2;
        break;
    case ISO2022_JP :
        mbstart = IS1;
        break;
    case UTF8   :
        mbstart = (U2_1 | U3_1 | U4_1);
        break;
    }
    switch (mbchar) {
    case 0      :
        mbchk = 0;
        break;
    case EUC_JP :
    case GB2312 :
    case KSC5601:
    case SJIS   :
    case BIGFIVE:
    case UTF8   :
        mbchk = NA;
        break;
    case ISO2022_JP :
        mbchk = (IS1 | NA);
        break;
    }

    /*
     * Set special handling for some encodings to supplement some compiler's
     * deficiency.
     */
    switch (mbchar) {
    case SJIS   :
#if ! SJIS_IS_ESCAPE_FREE
        bsl_need_escape = TRUE;
#endif
        break;
    case BIGFIVE:
#if ! BIGFIVE_IS_ESCAPE_FREE
        bsl_need_escape = TRUE;
#endif
        break;
    case ISO2022_JP :
#if ! ISO2022_JP_IS_ESCAPE_FREE
        bsl_need_escape = TRUE;
#endif
        break;
    default :
        bsl_need_escape = FALSE;
        break;
    }

    /*
     * Modify magic characters in character type table.
     * char_type[] table should be rewritten in accordance with the 'mcpp_mode'
     * whenever the encoding is changed.
     */
    if (mcpp_mode) {                /* If mcpp_mode is already set  */
        char_type[ DEF_MAGIC] = standard ? LET : 0;
        char_type[ IN_SRC] = (mcpp_mode == STD) ? LET : 0;
        char_type[ TOK_SEP] = (mcpp_mode == STD || mcpp_mode == OLD_PREP)
                ? HSPA: 0;          /* TOK_SEP equals to COM_SEP    */
    }
}

static size_t   mb_read_2byte(
    int     c1,         /* The 1st byte of mbchar sequence (already read)   */
    char ** in_pp,              /* Pointer to input     */
    char ** out_pp              /* Pointer to output    */
)
/*
 * Multi-byte character reading routine for 2-byte encodings.
 */
{
    int     error = FALSE;
    size_t  len = 0;    /* Number of multi-byte characters read.    */
    char *  in_p = *in_pp;
    char *  out_p = *out_pp;

    if (! (char_type[ c1 & UCHARMAX] & mbstart))
        return  MB_ERROR;           /* Not a multi-byte character   */

    do {
        if (! (char_type[ (*out_p++ = *in_p++) & UCHARMAX] & mb2)) {
            error = TRUE;
            break;
        }
        len++;
    } while (char_type[ (*out_p++ = *in_p++) & UCHARMAX] & mbstart);
    *in_pp = --in_p;
    *(--out_p) = EOS;
    *out_pp = out_p;
    return  error ? (len | MB_ERROR) : len;
}

static size_t   mb_read_iso2022_jp(
    int     c1, /* The 1st byte of the sequence already read (always 0x1b). */
    char ** in_pp,
    char ** out_pp
)
/*
 * Multi-byte character reading routine for ISO2022_JP.
 */
{
    int     error = FALSE;
    size_t  len = 0;
    char *  in_p = *in_pp;
    char *  out_p = *out_pp;
    int     c2, c3, c4;

    if (! (char_type[ c1 & UCHARMAX] & mbstart))
        return  MB_ERROR;

    do {

        *out_p++ = c2 = *in_p++;
        if (! (char_type[ c2 & UCHARMAX] & IS2)) {
            error = TRUE;
            break;
        }
        *out_p++ = c3 = *in_p++;
        if (! (char_type[ c3 & UCHARMAX] & IS3)) {
            error = TRUE;
            break;
        }

        switch (c2) {
        case 0x24   :
            switch (c3) {
            case 0x42   :   /* 0x1b 0x24 0x42:  JIS X 0208-1983 */
                break;
            case 0x28   :
                *out_p++ = c4 = *in_p++;
                if (! (char_type[ c4 & UCHARMAX] & IS4))
                    error = TRUE;
                /* else:    0x1b 0x24 0x28 0x44:    JIS X 0212  */
                break;
            default :
                error = TRUE;
            }
            break;
        case 0x28   :
            switch (c3) {
            case 0x42   :   /* 0x1b 0x28 0x42:  ASCII   */
                c1 = *out_p++ = *in_p++ & UCHARMAX;
                continue;
            default :
                error = TRUE;
            }
            break;
        }
        if (error)
            break;

        while (char_type[ c1 = *out_p++ = (*in_p++ & UCHARMAX)] & IJP) {
            if (! (char_type[ *out_p++ = (*in_p++ & UCHARMAX)] & IJP)) {
                error = TRUE;
                break;
            }
            len++;          /* String of multi-byte characters  */
        }
        if (error)
            break;

    } while (char_type[ c1] & IS1);     /* 0x1b:    start of shift-sequence */

    *in_pp = --in_p;
    *(--out_p) = EOS;
    *out_pp = out_p;
    return  error ? (len | MB_ERROR) : len;
}

static size_t   mb_read_utf8(
    int     c1,
    char ** in_pp,
    char ** out_pp
)
/*
 * Multi-byte character reading routine for UTF8.
 */
{
    int     error = FALSE;
    size_t  len = 0;
    char *  in_p = *in_pp;
    char *  out_p = *out_pp;

    if (! (char_type[ c1 & UCHARMAX] & mbstart))
        return  MB_ERROR;

    do {
        unsigned int    codepoint;
        int             i, bytes;

        if ((char_type[ c1 & UCHARMAX] & U4_1) == U4_1)
            bytes = 4;                          /* 4-byte character */
        else if ((char_type[ c1 & UCHARMAX] & U3_1) == U3_1)
            bytes = 3;                          /* 3-byte character */
        else if ((char_type[ c1 & UCHARMAX] & U2_1) == U2_1)
            bytes = 2;                          /* 2-byte character */

        /* Must ensure that the sequence is not reserved as a surrogate */
        codepoint = ((2 << (6-bytes)) - 1) & c1;    /* mask off top bits    */

        /* All bytes left in the sequence must be in 0x80 - 0xBF    */
        for (i = bytes - 1; i && !error; i--) {
            codepoint = (codepoint << 6) + ((*in_p) & 0x3fU);
            if (! (char_type[ (*out_p++ = *in_p++) & UCHARMAX] & UCONT))
                error = TRUE;
        }

        /* Check for overlong/underlong sequences */
        if ((bytes == 2 && (codepoint < 0x80 || codepoint > 0x7FF))
            || (bytes == 3 && (codepoint < 0x800 || codepoint > 0xFFFF))
            || (bytes == 4 && (codepoint < 0x10000 || codepoint > 0x10FFFF)))
            error = TRUE;
        if ((codepoint >= 0xD800 && codepoint <= 0xDFFF)
            /* Check for reserved surrogate codepoints */
                || (codepoint >= 0xFFFE && codepoint <= 0xFFFF))
                /* Illegal  */
            error = TRUE;
#if 0
        printf( "codepoint:0x%x\n", codepoint);
#endif
        if (error)
            break;
        len++;
    } while (char_type[ (*out_p++ = c1 = *in_p++) & UCHARMAX] & mbstart);
                        /* Start of the next multi-byte character   */
    *in_pp = --in_p;
    *(--out_p) = EOS;
    *out_pp = out_p;
    return  error ? (len | MB_ERROR) : len;
}

uexpr_t     mb_eval(
    char ** seq_pp
)
/*
 * Evaluate the value of a multi-byte character.
 * This routine does not check the legality of the sequence.
 * This routine is called from eval_char().
 * This routine is never called in POST_STD mode.
 */
{
    char *      seq = *seq_pp;
    uexpr_t     val = 0;
    int         c, c1;

    if (! (char_type[ c = *seq++ & UCHARMAX] & mbstart)) {
        *seq_pp = seq;
        return  c;                  /* Not a multi-byte character   */
    }

    switch (mbchar) {
    case EUC_JP :
    case GB2312 :
    case KSC5601:
    case SJIS   :
    case BIGFIVE:
        val = (c << 8) + (*seq++ & UCHARMAX);
        /* Evaluate the 2-byte sequence */
        break;
    case ISO2022_JP :
        if (char_type[ c & UCHARMAX] & IS1) {   /* Skip shift-sequence  */
            if (char_type[ c = *seq++ & UCHARMAX] & IS2) {
                if (char_type[ c1 = *seq++ & UCHARMAX] & IS3) {
                    if (c1 == 0x28)
                        seq++;
                    if (c == 0x28 && c1 == 0x42) {  /* Shift-out sequence   */
                        val = 0;
                        break;
                    }
                    c = *seq++ & UCHARMAX;
                }
            }
        }
        val = (c << 8) + (*seq++ & UCHARMAX);       /* Evaluate the 2-bytes */
        break;
    case UTF8   :   /* Evaluate the sequence of 2, 3 or 4 bytes as it is    */
        val = (c << 8) + (*seq++ & UCHARMAX);
        if (char_type[ c & UCHARMAX] & U3_1) {
            val = (val << 8) + (*seq++ & UCHARMAX);
        } else if (char_type[ c & UCHARMAX] & U4_1) {
            val = (val << 8) + (*seq++ & UCHARMAX);
            val = (val << 8) + (*seq++ & UCHARMAX);
        }
        break;
    }

    *seq_pp = seq;
    return  val;
}

int  last_is_mbchar(
    const char *  in,               /* Input physical line          */
    int     len                     /* Length of the line minus 2   */
)
/*
 * Return 2, if the last char of the line is second byte of SJIS or BIGFIVE,
 * else return 0.
 */
{
    const char *    cp = in + len;
    const char * const      endp = in + len;    /* -> the char befor '\n'   */

    if ((mbchar & (SJIS | BIGFIVE)) == 0)
        return  0;
    while (in <= --cp) {                    /* Search backwardly    */
        if ((char_type[ *cp & UCHARMAX] & mbstart) == 0)
            break;                  /* Not the first byte of MBCHAR */
    }
    if ((endp - cp) & 1)
        return  0;
    else
        return  2;
}

