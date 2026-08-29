/*
 * configed.H
 * Configurations for MCPP using config.h genarated by configure script.
 *
 * WARNING: These settings assume HOST == TARGET.  In case of HOST
 *      differs from TARGET, you must edit this file here and there.
 */

#define     TRUE            1
#define     FALSE           0
#define     DATE            "2008/11"               /* Date of mcpp     */

/*
 *  'Target' means the O.S. and the compiler to which cpp is implemented.
 *  'Host' means the O.S. and the compiler with which cpp is compiled.
 */

#include    "config.h"

#ifndef COMPILER
#define COMPILER            MSC
#endif
#ifndef HOST_COMPILER
#define HOST_COMPILER       COMPILER
#endif

/*
 *      P A R T   1     Configurations for target-operating-system
 *                      and target-compiler.
 */

/*
 * Names of the SYSTEM (i.e. target operating system).  This is needed so that
 * cpp can use appropriate filename conventions.
 */
#define SYS_UNKNOWN         0
#define SYS_UNIX            0x1000
#define SYS_LINUX           0x1800  /* (SYS_LINUX & 0xF000) == SYS_UNIX     */
#define SYS_FREEBSD         0x1A00  /* (SYS_FREEBSD & 0xF000) == SYS_UNIX   */
#define SYS_CYGWIN          0x1C00  /* (SYS_CYGWIN & 0xF000) == SYS_UNIX    */
#define SYS_MAC             0x1E00  /* (SYS_MAC & 0xF000) == SYS_UNIX       */
#define SYS_WIN             0x7000
#define SYS_WIN32           0x7400  /* (SYS_WIN32 & 0xF000) == SYS_WIN      */
#define SYS_MINGW           0x7C00  /* (SYS_MINGW & 0xF000) == SYS_WIN      */

/* COMPILER */
#define COMPILER_UNKNOWN    0
#define MSC                 0x7400  /* Microsoft C, Visual C++      */
#define BORLANDC            0x7440  /* Borland C    */
#define WIN_SYMANTECC       0x7470  /* Symantec for Windows         */
#define LCC                 0x74C0  /* LCC-Win32    */
#define GNUC                0x00E0  /* GNU C (GCC)  */
#define INDEPENDENT         0xFFFF  /* No target, compiler-independent-build*/

#define SYS_FAMILY          (SYSTEM & 0xF000)
#define COMPILER_FAMILY     (COMPILER & 0xF0)
#define HOST_SYS_FAMILY     (HOST_SYSTEM & 0xF000)

/* Default MBCHAR (multi-byte character) encoding.  */
#define EUC_JP          0x10    /* Extended UNIX code of JIS X 0208 */
#define GB2312          0x20    /* EUC-like encoding of Chinese GB 2312-80  */
#define KSC5601         0x30    /* EUC-like encoding of Korean KS C 5601    */
#define SJIS            0x80    /* Shift-JIS encoding of JIS X 0208 */
#define BIGFIVE         0x90    /* Encoding of Taiwanese Big Five   */
#define ISO2022_JP      0x100   /* ISO-2022-JP (ISO-2022-JP1) encoding      */
#define UTF8            0x1000  /* UTF-8 encoding           */

/*
 * MBCHAR means multi-byte character encoding.
 * MBCHAR means the default encoding, and you can change the encoding by
 *      #pragma MCPP setlocale, -e <encoding> option or environment variable
 *      LC_ALL, LC_CTYPE, LANG.
 * MBCHAR == 0 means not to recognize any multi-byte character encoding.
 */

/*
 * In order to predefine target-dependent macros,
 * several macros are defined here:
 * *_OLD define the macro beginning with an alphabetic letter,
 * *_STD, *_STD?, *_EXT, *_EXT2 define the macro beginning with an '_'.
 * *_STD1 define the macro beginning with '__' and ending with an alpha-
 *          numeric letter.
 * *_STD2 define the macro beginning with '__' and ending with '__'.
 * These may not be defined, if they are not needed.
 * They should not be #defined to no token or to "".
 *
 * SYSTEM_OLD, SYSTEM_STD1, SYSTEM_STD2, SYSTEM_EXT, SYSTEM_EXT2
 *      define the target operating system (by name).
 * SYSTEM_SP_OLD, SYSTEM_SP_STD     define the target-OS specific macro name
 * COMPILER_OLD, COMPILER_STD1, COMPILER_STD2, COMPILER_EXT, COMPILER_EXT2
 *          , COMPILER_SP_OLD, COMPILER_SP_STD
 *      define the target compiler (by name).
 * COMPILER_CPLUS        defines the target C++ compiler.
 * COMPILER_SP1, COMPILER_SP2, COMPILER_SP3
 *      define the compiler-specific macros.
 *
 * <macro>_VAL      specify the value of the <macro>.
 *      If not specified, these values default to "1".
 *      To define the value of no-token, specify as "" rather than no-token.
 * SYSTEM_OLD, SYSTEM_STD?, COMPILER_OLD have the value of "1".
 */

/*
 * target-compiler-dependent definitions:
 *
 * LINE_PREFIX  defines the output line prefix, if not "#line 123".
 *              This should be defined as "# " to represent "# 123" format
 *              ("#line " represents "#line 123" format).
 *
 * ENV_C_INCLUDE_DIR    may be defined to the name of environment-variable for
 *              C include directory.
 * ENV_CPLUS_INCLUDE_DIR    is name of environment-variable for C++ include
 *              directory which exists other than ENV_C_INCLUDE_DIR.
 * ENV_SEP      is the separator (other than space) of include-paths in an
 *              environment-variable.  e.g. the ':' in
 *              "/usr/abc/include:/usr/xyz/include"
 *
 * EMFILE       should be defined to the macro to represent errno of 'too many
 *              open files' if the macro is different from EMFILE.
 *
 * ONE_PASS     should be set TRUE, if COMPILER is "one pass compiler".
 *
 * FNAME_FOLD   means that target-system folds upper and lower cases of
 *              directory and file-name.
 *
 * SEARCH_INIT  specifies the default value of 'search_rule' (in system.c).
 *              'search_rule' holds searching rule of #include "header.h" to
 *              search first before searching user specified or system-
 *              specific include directories.
 *              CURRENT means to search the directory relative to "current
 *              directory" which is current at cpp invocation.
 *              SOURCE means to search the directory relative to that of the
 *              source file (i.e. "includer").
 *              (CURRENT & SOURCE) means to search current directory first
 *              source directory next.
 *              'search_rule' is initialized to SEARCH_INIT.
 */
#define CURRENT             1
#define SOURCE              2

#if     SYS_FAMILY == SYS_UNIX
#define SYSTEM_OLD          "unix"
#define SYSTEM_STD1         "__unix"
#define SYSTEM_STD2         "__unix__"
#endif

#if     SYSTEM == SYS_FREEBSD
#define SYSTEM_EXT          "__FreeBSD__"
#endif

#if     SYSTEM == SYS_LINUX
#define SYSTEM_EXT          "__linux__"
#endif

#if     SYSTEM == SYS_MAC
#define SYSTEM_EXT          "__APPLE__"
#endif

#if     SYSTEM == SYS_CYGWIN
#define SYSTEM_EXT          "__CYGWIN__"
#if     defined (__CYGWIN64__)
#define SYSTEM_EXT2         "__CYGWIN64__"
#else
#define SYSTEM_EXT2         "__CYGWIN32__"
#endif
#ifndef MBCHAR
#define MBCHAR              SJIS
#endif
#endif

#if     SYSTEM == SYS_MINGW
#define SYSTEM_EXT          "__MINGW__"
#if     defined (__MINGW64__)
#define SYSTEM_EXT2         "__MINGW64__"
#else
#define SYSTEM_EXT2         "__MINGW32__"
#endif
#ifndef MBCHAR
#define MBCHAR              SJIS
#endif
#endif

#if     SYS_FAMILY == SYS_UNIX
#ifndef MBCHAR
#define MBCHAR              EUC_JP      /* UTF8 if you like */
#endif
#endif

#if     COMPILER == GNUC
#define COMPILER_EXT        "__GNUC__"
#define COMPILER_EXT_VAL    GCC_MAJOR_VERSION
#define COMPILER_EXT2       "__GNUC_MINOR__"
#define COMPILER_EXT2_VAL   GCC_MINOR_VERSION
#define COMPILER_CPLUS      "__GNUG__"
#define COMPILER_CPLUS_VAL  GCC_MAJOR_VERSION
#ifndef ENV_C_INCLUDE_DIR
#define ENV_C_INCLUDE_DIR   "C_INCLUDE_PATH"
#define ENV_CPLUS_INCLUDE_DIR   "CPLUS_INCLUDE_PATH"
#endif
/*
 * __SIZE_TYPE__, __PTRDIFF_TYPE__ and __WCHAR_TYPE__ are the predefines of
 * GCC and undocumented in GCC 2.
 * On GCC V.3.*, V.4.*, these macros are known by mcpp_g*_predef_*.h files.
 */
#if     __GNUC__ == 2
#define COMPILER_SP1        "__SIZE_TYPE__"
#define COMPILER_SP2        "__PTRDIFF_TYPE__"
#define COMPILER_SP3        "__WCHAR_TYPE__"
#endif

#define CMP_NAME            "GCC"
#endif  /* COMPILER == GNUC */

#if     COMPILER == INDEPENDENT
/* specifications of compiler-independent-build */
#define LINE_PREFIX         "#line "
#define STD_LINE_PREFIX     TRUE    /* Output #line by C source format      */
#define HAVE_DIGRAPHS       TRUE    /* Output digraphs as it is     */
#define SEARCH_INIT         SOURCE  /* Include directory relative to source */
#define SJIS_IS_ESCAPE_FREE         TRUE    /* Do not treat SJIS specially  */
#define BIGFIVE_IS_ESCAPE_FREE      TRUE    /* Do not treat specially       */
#define ISO2022_JP_IS_ESCAPE_FREE   TRUE    /* Do not treat specially       */
#define TARGET_HAVE_LONG_LONG       TRUE    /* dummy    */
#define STDC_VERSION        199409L /* Initial value of __STDC_VERSION__    */
#endif

/*
 * defaults
 */

#ifdef  SYSTEM_EXT
#ifndef SYSTEM_EXT_VAL
#define SYSTEM_EXT_VAL      "1"
#endif
#endif
#ifdef  SYSTEM_EXT2
#ifndef SYSTEM_EXT2_VAL
#define SYSTEM_EXT2_VAL     "1"
#endif
#endif
#ifdef  COMPILER_STD1
#ifndef COMPILER_STD1_VAL
#define COMPILER_STD1_VAL   "1"
#endif
#endif
#ifdef  COMPILER_STD2
#ifndef COMPILER_STD2_VAL
#define COMPILER_STD2_VAL   "1"
#endif
#endif
#ifdef  COMPILER_EXT
#ifndef COMPILER_EXT_VAL
#define COMPILER_EXT_VAL    "1"
#endif
#endif
#ifdef  COMPILER_EXT2
#ifndef COMPILER_EXT2_VAL
#define COMPILER_EXT2_VAL   "1"
#endif
#endif
#ifdef  COMPILER_CPLUS
#ifndef COMPILER_CPLUS_VAL
#define COMPILER_CPLUS_VAL  "1"
#endif
#endif

#ifndef ENV_C_INCLUDE_DIR
#define ENV_C_INCLUDE_DIR   "INCLUDE"
#endif
#ifndef ENV_CPLUS_INCLUDE_DIR
#define ENV_CPLUS_INCLUDE_DIR   "CPLUS_INCLUDE"
#endif

#ifndef ENV_SEP
#if     SYS_FAMILY == SYS_WIN
#define ENV_SEP             ';'
#else
#define ENV_SEP             ':'
#endif
#endif

#ifndef ONE_PASS
#define ONE_PASS            FALSE
#endif

/*
 * CHARBIT, UCHARMAX are respectively CHAR_BIT, UCHAR_MAX of target compiler.
 * CHARBIT should be defined to the number of bits per character.
 * It is needed only for processing of multi-byte character constants.
 * UCHARMAX should be defined to the maximum value of type unsigned char
 * or maximum value of unsigned int which is converted from type (signed)
 * char.
 *
 * LONGMAX      should be defined to the LONG_MAX in <limits.h>.
 * ULONGMAX     should be defined to the ULONG_MAX in <limits.h>.
 */

/* _POSIX_* only to get PATH_MAX    */
#define _POSIX_             1
#define _POSIX_SOURCE       1
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE     1
#define _POSIX_C_SOURCE_defined     1
#endif
#include    "limits.h"
#undef  _POSIX_
#undef  _POSIX_SOURCE
#ifdef  _POSIX_C_SOURCE_defined
#undef  _POSIX_C_SOURCE
#undef  _POSIX_C_SOURCE_defined
#endif
#define CHARBIT             CHAR_BIT
#define UCHARMAX            UCHAR_MAX
#define USHRTMAX            USHRT_MAX
#define LONGMAX             LONG_MAX
#define ULONGMAX            ULONG_MAX

/*
 * Define MBCHAR (multi-byte character encoding) to SJIS, EUC_JP or other.
 */
#ifndef MBCHAR
#define MBCHAR              0
#endif

/*
 * SJIS_IS_ESCAPE_FREE means the compiler does not escape '0x5c' ('\\') in
 * shift-JIS encoded multi-byte character.  SJIS_IS_ESCAPE_FREE == FALSE 
 * enables cpp to insert * '\\' before '\\' of the 2nd byte of SJIS code in
 * literal.  This insertion is for the compiler-proper which can't recognize
 * SJIS literal.
 * BIGFIVE_IS_ESCAPE_FREE means similar case on BIGFIVE encoding.
 * ISO2022_JP_IS_ESCAPE_FREE means similar case on ISO2022_JP encoding.
 */
#ifndef SJIS_IS_ESCAPE_FREE
#define SJIS_IS_ESCAPE_FREE     FALSE   /* or TRUE following your compiler  */
#endif
#ifndef BIGFIVE_IS_ESCAPE_FREE
#define BIGFIVE_IS_ESCAPE_FREE  FALSE   /* or TRUE following your compiler  */
#endif
#ifndef ISO2022_JP_IS_ESCAPE_FREE
#define ISO2022_JP_IS_ESCAPE_FREE   FALSE   /* or TRUE following compiler   */
#endif

/*
 *      P A R T   2     Configurations for host-compiler.
 *
 *      WARNING: In case of HOST_COMPILER differs from COMPILER, you must
 *          edit here and there of this part.
 */

#define HOST_HAVE_STPCPY        HAVE_STPCPY

/*
 * Declaration of standard library functions and macros.
 */

/* stdin, stdout, stderr, FILE, NULL, fgets(), fputs() and other functions. */
#include    "stdio.h"

/* PATHMAX is the maximum length of path-list on the host system.   */
#ifdef  PATH_MAX
#define PATHMAX     PATH_MAX        /* Posix macro  */
#else
#define PATHMAX     FILENAME_MAX
#endif

/* islower(), isupper(), toupper(), isdigit(), isxdigit(), iscntrl()        */
#include    "ctype.h"

/* errno    */
#include    "errno.h"

#include    "string.h"
#include    "stdlib.h"
#include    "time.h"
#include    "setjmp.h"

/* For debugging malloc systems by kmatsui  */
#if     KMMALLOC && _MEM_DEBUG
#include        "xalloc.h"
#endif
