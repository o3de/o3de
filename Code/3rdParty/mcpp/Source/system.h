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
 *                          S Y S T E M . H
 *                  S y s t e m   D e p e n d e n t
 *              D e f i n i t i o n s   f o r   M C P P
 *
 * Definitions in this file may be edited to configure MCPP for particular
 * operating systems and compiler configurations.
 *
 * Note:  MCPP assumes the system implement the Ascii character set.
 * If this is not the case, you will have to do some editing here and there.
 */

#define SYSTEM_H

#if     HAVE_CONFIG_H
#include    "configed.h"
#else
#include    "noconfig.h"
#endif

/* Some system has a wrong definition of UCHAR_MAX.  See cpp-test.html#5.1.3 */
#if     UCHARMAX < -255
/* The definition of UCHARMAX (possibly UCHAR_MAX too) is wrong.    */
/* Define it as a signed int value, not as an unsigned value.       */
#undef  UCHARMAX
#define UCHARMAX    ((1 << CHAR_BIT) - 1)
#endif

/*
 *      PART 1 and PART 2 are defined in the above header files.
 */

/*
 *      P A R T   1     Configurations for target-operating-system
 *                      and target-compiler.
 */

/*
 *      P A R T   2     Configurations for host-compiler.
 */

/*
 *      P A R T   3     Configurations for default settings, typedefs and
 *                      translation limits.
 */

/*
 * The variable 'mcpp_mode' specifies the mode of preprocessing as one of 
 *          OLD_PREP, KR, STD, or POST_STD.
 *      Those modes have many differences each other --
 *          i.e. handling of translation phases; handling of some
 *          preprocessing tokens; availability of some directives; way of
 *          macro expansion;
 *          -- as follows.
 *
 * KR       Actual arguments of a macro are expanded (after substitution)
 *          with rest of the replacement text and the subsequent source text.
 *      ## in macro definition has no significance to cpp.  The surrounding
 *          tokens are macro-expanded separately.  Also, # has no significance
 *          to cpp.  The following token is expanded.
 *      Directly or intermediately recursive macro call causes an error.
 *      <backslash><newline> sequence is deleted only in string literals
 *          and in #define directive lines.
 *      sizeof (type) can be used in #if line.
 *      KR corresponds to the "K&R 1st."
 *
 * OLD_PREP     In addition to the KR specifications, this mode has the
 *          following characteristics (and some others).
 *      Converts comment to 0 space instead of 1 space.
 *      Expands the parameter like spelling in string literal as a macro.
 *      Does not check unmatched pair of '"' or '\''.
 *      OLD_PREP corresponts to "Reiser model" cpp.
 *
 * STD      Standard conforming mode.
 *      <backslash><newline> sequence is always deleted after trigraph
 *          conversion and before tokenization.
 *      Digraph sequences are recognized as tokens.
 *      Actual arguments of a macro are expanded separately prior to
 *          re-scanning of the replacement text.
 *      The name in the replacement text once expanded is not re-expanded,
 *          thus preventing recursive death.
 *      ## in macro definition concatenates tokens.  The tokens are not
 *          expanded.  The concatenated token is expanded by rescanning.
 *      # in macro definition stringizes the following argument.  The argument
 *          is not expanded.  \ is inserted before " and \ in or surrounding
 *          the string literal or character constant.
 *      An expanded macro is surrounded by spaces to prevent unintended
 *          token merging.
 *
 * POST_STD     This mode simplifies the behavior of STD mode as follows.
 *      1. Digraph sequences are converted in translation phase 1, as
 *          alternate characters rather than as tokens.
 *      2. A space is inserted as a token separator between any tokens in
 *          a source (except a macro name and the next '(' in macro
 *          definition): thus simplifying tokenization, test of macro
 *          redefinition and macro expansion, especially "stringization".
 *      3. Test of macro redefinition ignores difference of parameter names,
 *          test of which has little utility and not a little overhead.
 *      4. #if expression forbids character constants, which have little
 *          portability, little utility and not a little overhead.
 *      5. Rescanning of a macro expansion is limited in the replacement
 *          text, rest of the source file is not scanned, thus making the
 *          syntax of "function-like" macro call more similar to that of
 *          function call.
 *      6. Argument of #include directive in <header.h> form is an obsolescent
 *          feature.
 *      7. '$' or so are not treated specially in #define directive.
 *      8. Trigraphs, UCN (universal-character name) are not recognized.
 *      9. Multi-byte characters in an identifier are not recognized.
 *
 * The following specifications are available when mode is STD or POST_STD.
 *      preprocessing number token, digraphs,
 *      #pragma (#pragma MCPP put_defines, #pragma MCPP warning
 *          , #pragma MCPP debug) directive,
 *      #error directive,
 *      #if defined operator, #elif directive,
 *      predefined macros __FILE__, __LINE__, __DATE__, __TIME__
 *          , __STDC__, __STDC_VERSION__, __STDC_HOSTED__,
 *      wide character constant, wide character string literal,
 *      _Pragma() operator, variable-arguments macro,
 *      macro as an argument of #include, #line directives,
 *      escape sequences \x[hex-digits], \a, \v,
 *      '+' option (C++ preprocessing),
 *      'S<n>' option (re-defines __STDC__ as <n>, unpredefine some macros),
 *      'V<n>' option (re-defines __STDC_VERSION__ or __cplusplus as <n>),
 *      'h<n>' option (re-defines __STDC_HOSTED__ as <n>).
 * The following specifications are available only in STD mode.
 *      Trigraphs and UCN,
 *      Multi-byte characters in an identifier.
 * The following specifications are available only in KR and OLD_PREP modes.
 *      #assert, #asm, #endasm, #put_defines, #debug and some other older
 *          directives,
 *      argument of #line directive other than decimal-digits.
 */

/* The values of 'mcpp_mode'.   */
#define OLD_PREP            1           /* "Reiser" cpp mode    */
#define KR                  2           /* K&R 1st mode         */
#define STD                 3           /* Standard moce        */
#define POST_STD            9           /* Special mode of MCPP */

/*
 * TRIGRAPHS_INIT   Initial value for the -3 option.  If TRUE -3
 *              disables trigraphs, if FALSE -3 enables them.
 * DIGRAPHS_INIT    Initial value for the -2 option.  If TRUE -2
 *              disables digraphs, if FALSE -2 enables them.
 * OK_UCN       Enable recognition of Universal-Character-Name sequence
 *              by -V199901L option.
 * OK_MBIDENT   Enable multi-byte characters in identifier by -V199901L
 *              option.
 * EXPAND_PRAGMA    Enable macro expansion of #pragma line (even in modes
 *              other than C99).
 * expr_t, uexpr_t      Type of maximum integer:
 *              long long (unsigned long long) or longer.
 * EXPR_MAX should be defined to the maximum value of uexpr_t.
 */
#define TRIGRAPHS_INIT      FALSE
#define DIGRAPHS_INIT       FALSE
#ifndef EXPAND_PRAGMA
#define EXPAND_PRAGMA       FALSE
#endif
#define OK_UCN              TRUE
#define OK_MBIDENT          FALSE

#if     HAVE_INTMAX_T
#if     HAVE_STDINT_H
#include    "stdint.h"
#elif   HAVE_INTTYPES_H
#include    "inttypes.h"
#endif
typedef intmax_t    expr_t;
typedef uintmax_t   uexpr_t;
#else
#if     HAVE_LONG_LONG
#if     (HOST_COMPILER == MSC && _MSC_VER < 1500) || HOST_COMPILER == BORLANDC
typedef __int64             expr_t;
typedef unsigned __int64    uexpr_t;
#else
typedef long long           expr_t;
typedef unsigned long long  uexpr_t;
#endif
#else   /* !HAVE_LONG_LONG  */
typedef unsigned long   uexpr_t;
typedef long            expr_t;
#endif  /* HAVE_LONG_LONG   */
#endif  /* HAVE_INTMAX_T    */

#if     HAVE_INTMAX_T
#define EXPR_MAX            UINTMAX_MAX
#elif   HAVE_LONG_LONG
#if     (HOST_COMPILER == MSC && _MSC_VER < 1400) || HOST_COMPILER == BORLANDC
#define EXPR_MAX            0xFFFFFFFFFFFFFFFFui64
#else
#define EXPR_MAX            0xFFFFFFFFFFFFFFFFULL
#endif
#else
#define EXPR_MAX            4294967295UL
#endif

/*
 * Translation limits.
 * The following definitions are used to allocate memory for work buffers.
 *
 * NWORK        Output buffer size.  Set this size according to your compiler-
 *              proper.  Length of string literal should be less than NWORK
 *              - 1.
 *              Nevertheless, when COMPILER == GNUC || COMPILER == MSC, mcpp
 *              uses NMACWORK as output buffer size because GNUC and Visual C
 *              can accept very long line.
 * NBUFF        Input buffer size after line concatenation by <backslash>
 *              <newline>.
 * NMACWORK     Internal work buffer size for macro definition and expansion.
 * IDMAX        The longest identifier length.
 * NMACPARS     The maximum number of #define parameters.
 *              NOTE: Must be NMACPARS <= UCHARMAX.
 * NEXP         The maximum nesting depth of #if expressions.
 * BLK_NEST     The number of nested #if's permitted.
 * INCLUDE_NEST The maximum nesting depth of #include.  This is needed to
 *              prevent infinite recursive inclusion.
 * RESCAN_LIMIT The maximum rescan times of macro expansion in STD or POST_STD
 *              modes.
 * PRESTD_RESCAN_LIMIT  The maximum rescan times of macro expansion in KR or
 *              OLD_PREP modes..
 *
 * NBUFF should not be smaller than NWORK.
 * NMACWORK should not be smaller than NWORK * 2.
 *
 * SBSIZE defines the number of hash-table slots for the macro symbol table.
 * It must be a power of 2.
 *
 * MKDEP_INIT   The initial maximum number of filenames in a dependency line
 *              of output of -M* option.  The maximum number is dynamically
 *              enlarged in execution.
 */

#ifndef IDMAX
#define IDMAX               0x400
#endif
#ifndef NMACPARS
#define NMACPARS            0xFF
#endif
#ifndef NEXP
#define NEXP                0x100
#endif
#ifndef BLK_NEST
#define BLK_NEST            0x100
#endif
#ifndef INCLUDE_NEST
#define INCLUDE_NEST        0x100
#endif
#ifndef RESCAN_LIMIT
#define RESCAN_LIMIT        0x40
#endif
#ifndef PRESTD_RESCAN_LIMIT
#define PRESTD_RESCAN_LIMIT 0x100
#endif
#ifndef NBUFF
#define NBUFF               0x10000     /* Must be NWORK <= NBUFF   */
#endif
#ifndef NWORK
#define NWORK               NBUFF       /* 0x1000, 0x4000, 0x10000, ..  */
#endif
#ifndef NMACWORK
#define NMACWORK            (NWORK * 4) /* Must be NWORK * 2 <= NMACWORK    */
#endif
#ifndef SBSIZE
#define SBSIZE              0x400
#endif
#ifndef MKDEP_INIT
#define MKDEP_INIT          0x100
#endif

#if     UCHARMAX < NMACPARS
    #error  "NMACPARS should not be greater than UCHARMAX"
#endif

#if     NBUFF < NWORK
    #error   "NBUFF must be same or greater than NWORK"
#endif
#if     NMACWORK < NWORK * 2
    #error   "NMACWORK must be same or greater than NWORK * 2"
#endif

#define SBMASK  (SBSIZE - 1)
#if     (SBSIZE ^ SBMASK) != ((SBSIZE * 2) - 1)
    #error  "SBSIZE must be a power of 2 !"
#endif

/*
 * Translation limits required by the Standard.
 *
 * *90MIN   limits specified by C90.
 * *99MIN   limits specified by C99.
 * *_CPLUS_MIN  limits recommended by C++ (ISO 1998/07 Standard).
 *
 * SLEN*MIN     Characters in a logical source line
 *              and characters in a string literal or wide string literal
 *              (after concatenation).
 * IDLEN*MIN    Significant initial characters in an internal identifier
 *              or a macro name.
 * NMACPARS*MIN     Parameters in one macro definition.
 *              Arguments in one macro invocation.
 * EXP_NEST*MIN     Nesting levels of parenthesized expressions in a full
 *              expression.
 * BLK_NEST*MIN     Nesting levels of conditional inclusion.
 * INCLUDE_NEST*MIN     Nesting levels for #include files.
 * NMACRO*MIN   Macro identifiers simultaneously defined in one translation
 *              unit.
 */
#define SLEN90MIN           0x1FD
#define IDLEN90MIN          0x1F
#define NMACPARS90MIN       0x1F
#define EXP_NEST90MIN       0x20
#define BLK_NEST90MIN       8
#define INCLUDE_NEST90MIN   8
#define NMACRO90MIN         0x400

#define SLEN99MIN           0xFFF
#define IDLEN99MIN          0x3F
#define NMACPARS99MIN       0x7F
#define EXP_NEST99MIN       0x3F
#define BLK_NEST99MIN       0x3F
#define INCLUDE_NEST99MIN   0xF
#define NMACRO99MIN         0xFFF

#define SLEN_CPLUS_MIN      0x10000
#define IDLEN_CPLUS_MIN     0x400
#define NMACPARS_CPLUS_MIN  0x100
#define EXP_NEST_CPLUS_MIN  0x100
#define BLK_NEST_CPLUS_MIN  0x100
#define INCLUDE_NEST_CPLUS_MIN  0x100
#define NMACRO_CPLUS_MIN    0x10000

/* LINE99LIMIT  means the line number limit of C99  */
#define LINE99LIMIT         0x7FFFFFFF

/*
 * STDC     This macro is used for the predefined __STDC__.
 * STDC_VERSION     is used for the value of __STDC_VERSION__.
 * STDC_HOSTED      is used for the value of __STDC_HOSTED__.
 */
#if     IDMAX < IDLEN90MIN || NBUFF < SLEN90MIN + 3
            || NWORK < SLEN90MIN + 2 || NMACPARS < NMACPARS90MIN
            || NEXP < EXP_NEST90MIN || BLK_NEST < BLK_NEST90MIN
#define STDC                0
#endif
#ifndef STDC
#define STDC                1       /* 1 : for ISO 9899:1990 or later   */
#endif

#ifndef STDC_VERSION
#define STDC_VERSION        0L      /* 199409L  : For conforming
            implementation to ISO 9899:1990 / Amendment 1:1995
            199901L : For C99   */
#endif
#ifndef STDC_HOSTED
#define STDC_HOSTED         1       /* 1 : for hosted implementation,
            0 : for free-standing implementation (C99 specification)    */
#endif

/*
 * CPLUS specifies the default value of the pre-defined macro __cplusplus
 * for C++ processing.
 * The value can be changed by -V<n> option.
 */
#define CPLUS               1       /* 199711L for C++ Standard     */

