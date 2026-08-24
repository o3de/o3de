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
 *                          S Y S T E M . C
 *          S y s t e m   D e p e n d e n t   R o u t i n e s
 *
 * Routines dependent on O.S., compiler or compiler-driver.
 * To port MCPP for the systems not yet ported, you must
 *      1. specify the constants in "configed.H" or "noconfig.H",
 *      2. append the system-dependent routines in this file.
 */
#if PREPROCESSED
#include    "mcpp.H"
#else
#include    "system.H"
#include    "internal.H"
#endif

#if     HOST_SYS_FAMILY == SYS_UNIX
#include    "unistd.h"              /* For getcwd(), readlink() */
#elif   HOST_COMPILER == MSC || HOST_COMPILER == LCC
#include    "direct.h"
#define getcwd( buf, size)  _getcwd( buf, size)
#elif   HOST_COMPILER == BORLANDC
#include    "dir.h"
#endif

#include    "sys/types.h"
#include    "sys/stat.h"                        /* For stat()       */
#if     ! defined( S_ISREG)
#define S_ISREG( mode)  (mode & S_IFREG)
#define S_ISDIR( mode)  (mode & S_IFDIR)
#endif
#if     HOST_COMPILER == MSC
#define S_IFREG     _S_IFREG
#define S_IFDIR     _S_IFDIR
#define stat( path, stbuf)  _stat( path, stbuf)
#endif

/* Function to compare path-list    */
#if     FNAME_FOLD
#if     HOST_COMPILER == GNUC   /* CYGWIN, MINGW, MAC   */
#include    <strings.h>         /* POSIX 1, 2001        */
#define str_case_eq( str1, str2)    (strcasecmp( str1, str2) == 0)
#else   /* MSC, BORLANDC, LCC   */
#if     HOST_COMPILER == MSC
#define stricmp( str1, str2)        _stricmp( str1, str2)
#endif
#define str_case_eq( str1, str2)    (stricmp( str1, str2) == 0)
#endif
#else   /* ! FNAME_FOLD */
#define str_case_eq( str1, str2)    (strcmp( str1, str2) == 0)
#endif

/*
 * PATH_DELIM is defined for the O.S. which has single byte path-delimiter.
 * Note: '\\' or any other character identical to second byte of MBCHAR should
 * not be used for PATH_DELIM for convenience of path-list parsing.
 */
#if SYS_FAMILY == SYS_UNIX || SYS_FAMILY == SYS_WIN || SYSTEM == SYS_UNKNOWN
#define PATH_DELIM      '/'
#define SPECIAL_PATH_DELIM  FALSE
#else   /* Any other path-delimiter, define PATH_DELIM by yourself  */
#define SPECIAL_PATH_DELIM  TRUE    /* Any path-delimiter other than '/'    */
#endif

/*
 * OBJEXT is the suffix to denote "object" file.
 */
#ifndef OBJEXT
#if     SYS_FAMILY == SYS_UNIX || HOST_COMPILER == GNUC
#define OBJEXT     "o"
#elif   SYS_FAMILY == SYS_WIN
#define OBJEXT     "obj"
#elif   1
/* Add here appropriate definitions for other systems.  */
#endif
#endif

static void     version( void);
                /* Print version message            */
static void     usage( int opt);
                /* Putout usage of MCPP             */
static void     set_opt_list( char * optlist);
                /* Set list of legal option chars   */
static int      parse_warn_level( const char * mcpp_optarg, int opt);
                /* Parse warning level option       */
static void     def_a_macro( int opt, char * def);
                /* Do a -D option                   */
static void     chk_opts( int sflag, int trad);
                /* Check consistency of options     */
#if COMPILER != GNUC
static void     init_cpu_macro( int gval, int sse);
                /* Predefine CPU-dependent macros   */
#endif
static void     init_predefines( void);
                /* Set and unset predefined macros  */
static void     init_std_defines( void);
                /* Predefine Standard macros        */
static void     set_limit( void);
                /* Set minimum translation limits   */
static void     set_pragma_op( void);
                /* Set the _Pragma() operator       */
static void     put_info( FILEINFO * sharp_file);
                /* Print compiler-specific-inf      */
static char *   set_files( int argc, char ** argv, char ** in_pp
        , char ** out_pp);
                /* Set input, output, diagnostic    */
static void     set_sys_dirs( int set_cplus_dir);
                /* Set system-specific include dirs */
static void     set_env_dirs( void);
                /* Set user-defined include dirs    */
static void     parse_env( const char * env);
                /* Parse environment variables      */
static void     set_a_dir( const char * dirname);
                /* Append an include directory      */
static char *   norm_dir( const char * dirname, int framework);
                /* Normalize include directory path */
static char *   norm_path( const char * dir, const char * fname, int inf
        , int hmap);    /* Normalize pathname to compare    */
#if SYS_FAMILY == SYS_UNIX
static void     deref_syml( char * slbuf1, char * slbuf2, char * chk_start);
                /* Dereference symbolic linked directory and file   */
#endif
#if COMPILER == GNUC
static void     init_gcc_macro( void);
                /* Predefine GCC-specific macros    */
static void     chk_env( void);
                /* Check the environment variables  */
#elif   COMPILER == MSC
static void     init_msc_macro( void);
                /* Predefine Visual C-specific macros       */
#endif
static void     def_macros( void);
                /* Define macros specified by -D    */
static void     undef_macros( void);
                /* Undefine macros specified by -U  */
static char *   md_init( const char * filename, char * output);
                /* Initialize makefile dependency   */
static char *   md_quote( char * output);
                /* 'Quote' special characters       */
static int      open_include( char * filename, int searchlocal, int next);
                /* Open the file to include         */
static int      has_directory( const char * source, char * directory);
                /* Get directory part of fname      */
static int      is_full_path( const char * path);
                /* The path is absolute path list ? */
static int      search_dir( char * filename, int searchlocal, int next);
                /* Search the include directories   */
static int      open_file( const char ** dirp, const char * src_dir
        , const char * filename, int local, int include_opt, int sys_frame);
                /* Open a source file       */
static const char *     set_fname( const char * filename);
                /* Remember the source filename     */
#if SYSTEM == SYS_MAC
#if COMPILER == GNUC
static char *   search_header_map( const char * hmap_file
        , const char * filename, char * pathlist);
                /* Search header map file for a header  */
static unsigned hmap_hash( const char * fname);
                /* Get hash value for the fname     */
#endif
static void     init_framework( void);
                /* Initialize framework[]           */
static int      search_framework( char * filename);
                /* Search "Framework" directories   */
static int      search_subdir( char * fullname, char * cp, char * frame
        , char * fname, int sys_frame);
                /* Search "Headers" and other dirs  */
#endif  /* SYSTEM == SYS_MAC    */
#if 0   /* This function is only for debugging use  */
static int      chk_dirp( const char ** dirp);
                /* Check validity of dirp arg for open_file()   */
#endif
static void     cur_file( FILEINFO * file, FILEINFO * sharp_file, int marker);
                /* Output current source file name  */
#if SYS_FAMILY == SYS_WIN
static char *   bsl2sl( char * filename);
                /* Convert \ to / in path-list      */
#endif
static int      is_junk( void);
                /* The directive has trailing junk? */
static void     do_once( const char * fullname);
                /* Process #pragma once             */
static int      included( const char * fullname);
                /* The file has been once included? */
static void     push_or_pop( int direction);
                /* Push or pop a macro definition   */
static int      do_prestd_directive( void);
                /* Process pre-Standard directives  */
static void     do_preprocessed( void);
                /* Process preprocessed file        */
static int      do_debug( int set);
                /* #pragma MCPP debug, #debug       */
static void     dump_path( void);
                /* Print include search path        */
static void     do_asm( int asm_start);
                /* Process #asm, #endasm            */
static int      mcpp_getopt( int argc, char * const * argv, const char * opts);
                /* getopt() to prevent linking of glibc getopt  */

/* for mcpp_getopt()    */
static int      mcpp_optind = 1;
static int      mcpp_opterr = 1;
static int      mcpp_optopt;
static char *   mcpp_optarg;

static int      mb_changed = FALSE;     /* Flag of -e option        */
static char     cur_work_dir[ PATHMAX + 1];     /* Current working directory*/

/*
 * incdir[] stores the -I directories (and the system-specific #include <...>
 * directories).  This is set by set_a_dir().  A trailing PATH_DELIM is
 * appended if absent.
 */
static const char **    incdir;         /* Include directories      */
static const char **    incend;         /* -> active end of incdir  */
static int          max_inc;            /* Number of incdir[]       */

typedef struct inc_list {       /* List of directories or files     */
    char *      name;           /* Filename or directory-name       */
    size_t      len;                    /* Length of 'name'         */
} INC_LIST;

/*
 * fnamelist[] stores the souce file names opened by #include directive for
 * debugging information.
 */
static INC_LIST *   fnamelist;          /* Source file names        */
static INC_LIST *   fname_end;          /* -> active end of fnamelist   */
static int          max_fnamelist;      /* Number of fnamelist[]    */

/* once_list[] stores the #pragma once file names.  */
static INC_LIST *   once_list;          /* Once opened file         */
static INC_LIST *   once_end;           /* -> active end of once_list   */
static int          max_once;           /* Number of once_list[]    */

#define INIT_NUM_INCLUDE    32          /* Initial number of incdir[]   */
#define INIT_NUM_FNAMELIST  256         /* Initial number of fnamelist[]    */
#define INIT_NUM_ONCE       64          /* Initial number of once_list[]    */

/*
 * 'search_rule' holds searching rule of #include "header.h" to search first
 * before searching user specified or system-specific include directories.
 * 'search_rule' is initialized to SEARCH_INIT.  It can be changed by -I1, -I2
 * or -I3 option.  -I1 specifies CURRENT, -I2 SOURCE and -I3 both.
 */

static int      search_rule = SEARCH_INIT;  /* Rule to search include file  */

static int      nflag = FALSE;          /* Flag of -N (-undef) option       */
static long     std_val = -1L;  /* Value of __STDC_VERSION__ or __cplusplus */

#define MAX_DEF   256
#define MAX_UNDEF (MAX_DEF/4)
static char *   def_list[ MAX_DEF];     /* Macros to be defined     */
static char *   undef_list[ MAX_UNDEF]; /* Macros to be undefined   */
static int      def_cnt;                /* Count of def_list        */
static int      undef_cnt;              /* Count of undef_list      */

/* Values of mkdep. */
#define MD_MKDEP        1   /* Output source file dependency line   */
#define MD_SYSHEADER    2   /* Print also system-header names       */
#define MD_FILE         4   /* Output to the file named *.d         */
#define MD_PHONY        8   /* Print also phony targets for each header */
#define MD_QUOTE        16  /* 'Quote' $ and space in target name   */

static FILE *   mkdep_fp;                       /* For -Mx option   */
static char *   mkdep_target;
    /* For -MT TARGET option and for GCC's queer environment variables.     */
static char *   mkdep_mf;               /* Argument of -MF option   */
static char *   mkdep_md;               /* Argument of -MD option   */
static char *   mkdep_mq;               /* Argument of -MQ option   */
static char *   mkdep_mt;               /* Argument of -MT option   */

/* sharp_filename is filename for #line line, used only in cur_file()   */
static char *   sharp_filename = NULL;
static char *   argv0;      /* argv[ 0] for usage() and version()   */
static int      ansi;           /* __STRICT_ANSI__ flag for GNUC    */
static int      compat_mode;
                /* "Compatible" mode of recursive macro expansion   */
#define MAX_ARCH_LEN    16
static char     arch[ MAX_ARCH_LEN];    /* -arch or -m64, -m32 options      */

#if COMPILER == GNUC
#define N_QUOTE_DIR     8
/* quote_dir[]:     Include directories for "header" specified by -iquote   */
/* quote_dir_end:   Active end of quote_dir */
static const char *     quote_dir[ N_QUOTE_DIR];
static const char **    quote_dir_end = quote_dir;
/* sys_dirp indicates the first directory to search for system headers.     */
static const char **    sys_dirp = NULL;        /* System header directory  */
static const char *     sysroot = NULL; /* Logical root directory of header */
static int      i_split = FALSE;                /* For -I- option   */
static int      gcc_work_dir = FALSE;           /* For -fworking-directory  */
static int      gcc_maj_ver;                    /* __GNUC__         */
static int      gcc_min_ver;                    /* __GNUC_MINOR__   */
static int      dDflag = FALSE;         /* Flag of -dD option       */
static int      dMflag = FALSE;         /* Flag of -dM option       */
#endif

#if COMPILER == GNUC || COMPILER == MSC
/*
 * preinclude points to the file specified by -include (-Fl for MSC) option,
 * which is included prior to the main input file.
 */
#define         NPREINCLUDE 8
static char *   preinclude[ NPREINCLUDE];       /* File to pre-include      */
static char **  preinc_end = preinclude;    /* -> active end of preinclude  */
#endif

#if COMPILER == MSC
static int      wchar_t_modified = FALSE;   /* -Zc:wchar_t flag     */
#endif

#if COMPILER == LCC
static const char *     optim_name = "__LCCOPTIMLEVEL";
#endif

#if SYSTEM == SYS_CYGWIN
static int      no_cygwin = FALSE;          /* -mno-cygwin          */

#elif   SYSTEM == SYS_MAC
#define         MAX_FRAMEWORK   8
static char *   framework[ MAX_FRAMEWORK];  /* Framework directories*/
static int      num_framework;          /* Current number of framework[]    */
static int      sys_framework;          /* System framework dir     */
static const char **    to_search_framework;
                        /* Search framework[] next to the directory */
static int      in_import;          /* #import rather than #include */
#endif

#define NO_DIR  FALSE
#if NO_DIR
/* Unofficial feature to strip directory part of include file   */
static int      no_dir;
#endif

#if MCPP_LIB
void    init_system( void)
/* Initialize static variables  */
{
    if (sharp_filename)
        free( sharp_filename);
    sharp_filename = NULL;
    incend = incdir = NULL;
    fnamelist = once_list = NULL;
    search_rule = SEARCH_INIT;
    mb_changed = nflag = ansi = compat_mode = FALSE;
    mkdep_fp = NULL;
    mkdep_target = mkdep_mf = mkdep_md = mkdep_mq = mkdep_mt = NULL;
    std_val = -1L;
    def_cnt = undef_cnt = 0;
    mcpp_optind = mcpp_opterr = 1;
#if COMPILER == GNUC
    sys_dirp = NULL;
    sysroot = NULL;
    gcc_work_dir = i_split = FALSE;
    quote_dir_end = quote_dir;
    dDflag = dMflag = FALSE;
#endif
#if COMPILER == MSC
    wchar_t_modified = FALSE;
#endif
#if COMPILER == GNUC || COMPILER == MSC
    preinc_end = preinclude;
#endif
#if SYSTEM == SYS_CYGWIN
    no_cygwin = FALSE;
#elif   SYSTEM == SYS_MAC
    num_framework = sys_framework = 0;
    to_search_framework = NULL;
#endif
#if NO_DIR
    no_dir = FALSE;
#endif
}

#endif

#define OPTLISTLEN  80

void    do_options(
    int         argc,
    char **     argv,
    char **     in_pp,                      /* Input file name      */
    char **     out_pp                      /* Output file name     */
)
/*
 * Process command line arguments, called only at MCPP startup.
 */
{
    char        optlist[ OPTLISTLEN];       /* List of option letter*/
    const char *    warning = "warning: -%c%s option is ignored\n";
    int         opt;
    int         unset_sys_dirs;
        /* Unset system-specific and site-specific include directories ?    */
    int         set_cplus_dir;  /* Set C++ include directory ? (for GCC)*/
    int         show_path;          /* Show include directory list  */
    DEFBUF *    defp;
    VAL_SIGN *  valp;
    int         sflag;                      /* -S option or similar */
    int         trad;                       /* -traditional         */
    int         old_mode;                   /* backup of 'mcpp_mode'*/
    int         gval, sse;
    char *      cp;
    int         i;
#if COMPILER == GNUC
#define NSYSDIR   8
    /* System include directory specified by -isystem   */
    char *      sysdir[ NSYSDIR] = { NULL, };
    char **     sysdir_end = sysdir;
    int         integrated_cpp; /* Flag of cc1 which integrates cpp in it   */
#elif   COMPILER == LCC
    const char *    debug_name = "__LCCDEBUGLEVEL";
#endif

    argv0 = argv[ 0];
    nflag = unset_sys_dirs = show_path = sflag = trad = FALSE;
    arch[ 0] = 0;
    gval = sse = 0;
    set_cplus_dir = TRUE;

    /* Get current directory for -I option and #pragma once */
    getcwd( cur_work_dir, PATHMAX);
#if SYS_FAMILY == SYS_WIN
    bsl2sl( cur_work_dir);
#endif
    sprintf( cur_work_dir + strlen( cur_work_dir), "%c%c", PATH_DELIM, EOS);
        /* Append trailing path-delimiter   */

#if COMPILER == GNUC
    defp = look_id( "__GNUC__");    /* Already defined by init_defines()    */
    gcc_maj_ver = atoi( defp->repl);
    defp = look_id( "__GNUC_MINOR__");
    gcc_min_ver = atoi( defp->repl);
    integrated_cpp = ((gcc_maj_ver == 3 && gcc_min_ver >= 3)
            || gcc_maj_ver == 4);
#endif
#if COMPILER == GNUC || COMPILER == MSC
    option_flags.dollar_in_name = TRUE;
    /* GCC and Visual C allows '$' in name by default   */
#endif

    set_opt_list( optlist);

opt_search: ;
    while (mcpp_optind < argc
            && (opt = mcpp_getopt( argc, argv, optlist)) != EOF) {

        switch (opt) {          /* Command line option character    */

#if COMPILER == GNUC
        case '$':                       /* Forbid '$' in identifier */
            option_flags.dollar_in_name = FALSE;
            break;
#endif

        case '+':
#if COMPILER == GNUC
plus:
#endif
            if (cplus_val || sflag) {
                mcpp_fputs( "warning: -+ option is ignored\n", ERR);
                break;
            }
            cplus_val = CPLUS;
            break;
#if COMPILER == GNUC
        case '-':
            if (memcmp( mcpp_optarg, "sysroot", 7) == 0) {
                if (mcpp_optarg[ 7] == '=')             /* --sysroot=DIR    */
                    sysroot = mcpp_optarg + 8;
                else if (mcpp_optarg[ 7] == EOS)        /* --sysroot DIR    */
                    sysroot = argv[ mcpp_optind++];
                else
                    usage( opt);
                break;
            } else {
                usage( opt);
            }
#endif
        case '2':                   /* Reverse digraphs recognition */
            option_flags.dig = ! option_flags.dig;
            break;
        case '3':                   /* Reverse trigraph recogniion  */
            option_flags.trig = ! option_flags.trig;
            break;

        case '@':                   /* Special preprocessing mode   */
            old_mode = mcpp_mode;
            if (str_eq( mcpp_optarg, "post")
                    || str_eq( mcpp_optarg, "poststd"))
                mcpp_mode = POST_STD;   /* 'post-Standard' mode     */
            else if (str_eq( mcpp_optarg, "old")
                    || str_eq( mcpp_optarg, "oldprep"))
                mcpp_mode = OLD_PREP;   /* 'old-Preprocessor' mode  */
            else if (str_eq( mcpp_optarg, "kr"))
                mcpp_mode = KR;         /* 'K&R 1st' mode           */
            else if (str_eq( mcpp_optarg, "std"))
                mcpp_mode = STD;        /* 'Standard' mode (default)*/
            else if (str_eq( mcpp_optarg, "compat")) {
                compat_mode = TRUE;     /* 'compatible' mode        */
                mcpp_mode = STD;
            }
            else
                usage( opt);
            standard = (mcpp_mode == STD || mcpp_mode == POST_STD);
            if (old_mode != STD && old_mode != mcpp_mode)
                mcpp_fprintf( ERR, "Mode is redefined to: %s\n", mcpp_optarg);
            break;

#if COMPILER == GNUC
        case 'A':       /* Ignore -A system(gnu), -A cpu(vax) or so */
            break;
        case 'a':
            if (str_eq( mcpp_optarg, "nsi")) {      /* -ansi                */
                look_and_install( "__STRICT_ANSI__", DEF_NOARGS_PREDEF, null
                        , "1");
                ansi = TRUE;
                break;
            } else if (memcmp( mcpp_optarg, "uxbase", 6) == 0) {
                mcpp_optind++;
                break;  /* Ignore '-auxbase some' or such nonsence  */
#if SYSTEM == SYS_MAC
            } else if (str_eq( mcpp_optarg, "rch")) {   /* -arch    */
                strcpy( arch, argv[ mcpp_optind++]);
                if (str_eq( arch, "ppc") || str_eq( arch, "ppc7400")
                        || str_eq( arch, "ppc64")
                        || str_eq( arch, "i386") || str_eq( arch, "i686")
                        || str_eq( arch, "x86_64") || str_eq( arch, "amd64")) {
                    if (str_eq( arch, "i686"))
                        strcpy( arch, "i386");
                    else if (str_eq( arch, "amd64"))
                        strcpy( arch, "x86_64");
                    else if (str_eq( arch, "ppc7400"))
                        strcpy( arch, "ppc");
                    break;
                }   /* Else usage() */
#endif
            }
            usage( opt);
#elif   COMPILER == MSC
        case 'a':
            if (memcmp( mcpp_optarg, "rch", 3) == 0) {
                if (str_eq( mcpp_optarg + 3, ":SSE")        /* -arch:SSE    */
                        || str_eq( mcpp_optarg + 3, ":sse"))
                    sse = 1;
                else if (str_eq( mcpp_optarg + 3, ":SSE2")  /* -arch:SSE2   */
                        || str_eq( mcpp_optarg + 3, ":sse2"))
                    sse = 2;
                /* Else ignore  */
            } else {
                usage( opt);
            }
            break;

        case 'A':
            option_flags.lang_asm = TRUE;   /* "assembler" source   */
            break;
#else
        case 'a':
            option_flags.lang_asm = TRUE;   /* "assembler" source   */
            break;
#endif

#if ! STD_LINE_PREFIX
        case 'b':
            std_line_prefix = TRUE; /* Putout line and file infor-  */
            break;                  /*   mation in C source style.  */
#endif

        case 'C':                           /* Keep comments        */
            option_flags.c = TRUE;
            break;

#if COMPILER == GNUC
        case 'c':
            if (! integrated_cpp)
                usage( opt);
            break;                  /* Else ignore this option      */
        case 'd':
            if (str_eq( mcpp_optarg, "M")) {                /* -dM          */
                dMflag = TRUE;
                no_output++;
            } else if (str_eq( mcpp_optarg, "D")) {         /* -dD          */
                dDflag = TRUE;
            } else if (str_eq( mcpp_optarg, "igraphs")) {   /* -digraphs    */
                option_flags.dig = TRUE;
            } else if (str_eq( mcpp_optarg, "umpbase")) {   /* -dumpbase    */
                ;                                           /* Ignore       */
            } else {
                usage( opt);
            }
            break;
#endif  /* COMPILER == GNUC */

        case 'D':                           /* Define symbol        */
            if (def_cnt >= MAX_DEF) {
                mcpp_fputs( "Too many -D options.\n", ERR);
                longjmp( error_exit, -1);
            }
            def_list[ def_cnt++] = mcpp_optarg;
            break;

        case 'e':
            /* Change the default MBCHAR encoding   */
            if (set_encoding( mcpp_optarg, FALSE, 0) == NULL)
                usage( opt);
            mb_changed = TRUE;
            break;

#if COMPILER == GNUC
        case 'E':
            if (! integrated_cpp)
                usage( opt);
            break;                          /* Ignore this option   */
        case 'f':
            if (memcmp( mcpp_optarg, "input-charset=", 14) == 0) {
                /* Treat -finput-charset= as the same option as -e  */
                if (set_encoding( mcpp_optarg + 14, FALSE, 0) == NULL)
                    usage( opt);
                mb_changed = TRUE;
            } else if (str_eq( mcpp_optarg, "working-directory")) {
                gcc_work_dir = TRUE;
            } else if (str_eq( mcpp_optarg, "no-working-directory")) {
                gcc_work_dir = FALSE;
            } else if (str_eq( mcpp_optarg, "stack-protector")) {
                look_and_install( "__SSP__", DEF_NOARGS_PREDEF, null, "1");
            } else if (str_eq( mcpp_optarg, "stack-protector-all")) {
                look_and_install( "__SSP_ALL__", DEF_NOARGS_PREDEF, null, "2");
            } else if (str_eq( mcpp_optarg, "exceptions")) {
                look_and_install( "__EXCEPTIONS", DEF_NOARGS_PREDEF, null
                        , "1");
            } else if (str_eq( mcpp_optarg, "no-exceptions")) {
                undef_list[ undef_cnt++] = "__EXCEPTIONS";
            } else if (str_eq( mcpp_optarg, "PIC")
                    || str_eq( mcpp_optarg, "pic")
                    || str_eq( mcpp_optarg, "PIE")
                    || str_eq( mcpp_optarg, "pie")) {
                look_and_install( "__PIC__", DEF_NOARGS_PREDEF, null, "1");
                look_and_install( "__pic__", DEF_NOARGS_PREDEF, null, "1");
            } else if (str_eq( mcpp_optarg, "no-dollars-in-identifiers")) {
                option_flags.dollar_in_name = FALSE;
            } else if (str_eq( mcpp_optarg, "no-show-column")) {
                ;                           /* Ignore this option   */
            } else if (! integrated_cpp) {
                usage( opt);
            }
            break;

        case 'g':
            if (!isdigit( *mcpp_optarg)
                    && str_eq( argv[ mcpp_optind - 2], "-g"))
                /* Neither '-g 0' nor '-ggdb' -- No argument    */
                mcpp_optind--;
            break;                          /* Ignore the option    */
#elif COMPILER == LCC
        case 'g':               /* Define __LCCDEBUGLEVEL as <n>    */
            if (*(mcpp_optarg + 1) == EOS && isdigit( *mcpp_optarg)) {
                defp = look_id( debug_name);
                strcpy( defp->repl, mcpp_optarg);
            } else {
                usage( opt);
            }
            break;
#elif COMPILER == MSC
        case 'G':
            if (*(mcpp_optarg + 1) == EOS) {    /* -Gx              */
                switch (*mcpp_optarg) {
                case '3':   case '4':   case '5':   case '6':
                    gval = *mcpp_optarg;
                    break;
                case 'B':                   /* -GB                  */
                    gval = '6';
                    break;
                case 'R':
                    look_and_install( "_CPPRTTI", DEF_NOARGS_PREDEF, null
                            , "1");
                    break;
                case 'X':
                    look_and_install( "_CPPUNWIND", DEF_NOARGS_PREDEF, null
                            , "1");
                    break;
                case 'Z':
                    look_and_install( "__MSVC_RUNTIME_CHECKS"
                            , DEF_NOARGS_PREDEF, null, "1");
                    break;
                default :
                    mcpp_fprintf( ERR, warning, opt, mcpp_optarg);
                }
            } else {
                usage( opt);
            }
            break;
#endif

#if SYSTEM == SYS_MAC
        case 'F':
            framework[ num_framework++] = mcpp_optarg;
            break;
#endif

        case 'h':
            if (*(mcpp_optarg + 1) == EOS && isdigit( *mcpp_optarg))
                /* a digit  */
                look_and_install( "__STDC_HOSTED__", DEF_NOARGS_PREDEF, null
                        , mcpp_optarg);
            else
                usage( opt);
            break;

#if COMPILER == MSC
        case 'X':
                unset_sys_dirs = TRUE;
                break;
#endif
        case 'I':                           /* Include directory    */
            if (str_eq( mcpp_optarg, "-")) {        /* -I-                  */
#if COMPILER == GNUC
                sys_dirp = incend;  /* Split include directories    */
                i_split = TRUE;
#else
                unset_sys_dirs = TRUE;
                        /* Unset pre-specified include directories  */
#endif
            } else if (*(mcpp_optarg + 1) == EOS && isdigit( *mcpp_optarg)
                    && (i = *mcpp_optarg - '0') != 0
                    && (i & ~(CURRENT | SOURCE)) == 0) {
                search_rule = i;            /* -I1, -I2 or -I3      */
            } else {                        /* Not '-' nor a digit  */
                set_a_dir( mcpp_optarg);    /* User-defined dir     */
            }
            break;

#if COMPILER == MSC
        case 'F':
            if (str_eq( mcpp_optarg, "l")) {        /* -Fl          */
                if (preinc_end >= &preinclude[ NPREINCLUDE]) {
                    mcpp_fputs( "Too many -Fl options.\n", ERR);
                    longjmp( error_exit, -1);
                }
                *preinc_end++ = argv[ mcpp_optind++];
            } else {
                usage( opt);
            }
            break;
#endif

#if COMPILER == GNUC
        case 'i':
            if (str_eq( mcpp_optarg, "nclude")) {   /* -include     */
                if (preinc_end >= &preinclude[ NPREINCLUDE]) {
                    mcpp_fputs( "Too many -include options.\n", ERR);
                    longjmp( error_exit, -1);
                }
                *preinc_end++ = argv[ mcpp_optind++];
            } else if (str_eq( mcpp_optarg, "system")) {    /* -isystem     */
                if (sysdir_end >= &sysdir[ NSYSDIR]) {
                    mcpp_fputs( "Too many -isystem options.\n", ERR);
                    longjmp( error_exit, -1);
                }
                *sysdir_end++ = argv[ mcpp_optind++];
                /* Add the directory before system include directory*/
            } else if (str_eq( mcpp_optarg, "quote")) {     /* -iquote      */
                if (quote_dir_end >= &quote_dir[ N_QUOTE_DIR]) {
                    mcpp_fputs( "Too many -iquote options.\n", ERR);
                    longjmp( error_exit, -1);
                }
                *quote_dir_end++ = argv[ mcpp_optind++];
                /* Add the directory for #include "header"          */
            } else if (memcmp( mcpp_optarg, "sysroot", 7) == 0) {
                if (mcpp_optarg[ 7] == '=')     /* -isysroot=DIR    */
                    sysroot = mcpp_optarg + 8;
                else if (mcpp_optarg[ 7] == EOS)        /* -isysroot DIR    */
                    sysroot = argv[ mcpp_optind++];
                else
                    usage( opt);
            } else if (str_eq( mcpp_optarg, "prefix")       /* -iprefix     */
                    || str_eq( mcpp_optarg, "withprefix")   /* -iwithprefix */
                    || str_eq( mcpp_optarg, "withprefixbefore")
                                            /* -iwithprefixbefore   */
                    || str_eq( mcpp_optarg, "dirafter")     /* -idirafter   */
                    || str_eq( mcpp_optarg, "multilib")) {  /* -imultilib   */
                mcpp_optind++;              /* Skip the argument    */
                /* Ignore these options */
            } else {
                usage( opt);
            }
            break;
#endif

        case 'j':
            option_flags.no_source_line = TRUE;
            break;  /* Do not output the source line in diagnostics */

#if COMPILER == MSC
        case 'J':
            look_and_install( "_CHAR_UNSIGNED", DEF_NOARGS_PREDEF, null, "1");
            break;
#endif

        case 'K':
            mcpp_debug |= MACRO_CALL;
            /*
             * Putout macro expansion informations embedded in comments.
             * Same with '#pragma MCPP debug macro_call'.
             */
            /* Enable white spaces preservation, too    */
            /* Fall through */
        case 'k':
            option_flags.k = TRUE;
            /* Keep white spaces of input lines as they are */
            break;

#if COMPILER == GNUC
        case 'l':
            if (memcmp( mcpp_optarg, "ang-", 4) != 0) {
                usage( opt);
            } else if (str_eq( mcpp_optarg + 4, "c")) {     /* -lang-c  */
                ;                           /* Ignore this option   */
            } else if (str_eq( mcpp_optarg + 4, "c99")      /* -lang-c99*/
                        || str_eq( mcpp_optarg + 4, "c9x")) {   /* -lang-c9x*/
                if (! sflag) {
                    stdc_val = 1;           /* Define __STDC__ to 1 */
                    std_val = 199901L;
                    sflag = TRUE;
                }
            } else if (str_eq( mcpp_optarg + 4, "c89")) {   /* -lang-c89*/
                if (! sflag) {
                    stdc_val = 1;           /* Define __STDC__ to 1 */
                    sflag = TRUE;
                }
            } else if (str_eq( mcpp_optarg + 4, "c++")) {   /* -lang-c++*/
                goto  plus;
            } else if (str_eq( mcpp_optarg + 4, "asm")) {   /* -lang-asm*/
                option_flags.lang_asm = TRUE;
            } else {
                usage( opt);
            }
            break;
#endif  /* COMPILER == GNUC */

        case 'M':           /* Output source file dependency line   */
            if (str_eq( mcpp_optarg, "M")) {                /* -MM  */
                ;
            } else if (str_eq( mcpp_optarg, "D")) {         /* -MD  */
                mkdep |= (MD_SYSHEADER | MD_FILE);
            } else if (str_eq( mcpp_optarg, "MD")) {         /* -MMD */
                mkdep |= MD_FILE;
            } else if (str_eq( mcpp_optarg, "P")) {          /* -MP  */
                mkdep |= MD_PHONY;
            } else if (str_eq( mcpp_optarg, "Q")) {  /* -MQ target   */
                mkdep |= MD_QUOTE;
                mkdep_mq = argv[ mcpp_optind++];
            } else if (str_eq( mcpp_optarg, "T")) {  /* -MT target   */
                mkdep_mt = argv[ mcpp_optind++];
            } else if (str_eq( mcpp_optarg, "F")) {  /* -MF file     */
                mkdep_mf = argv[ mcpp_optind++];
            } else if (argv[ mcpp_optind - 1] == mcpp_optarg) {     /* -M   */
                mkdep |= MD_SYSHEADER;
                mcpp_optind--;
            } else {
                usage( opt);
            }
            if (str_eq( mcpp_optarg, "D") || str_eq( mcpp_optarg, "MD")) {
                cp = argv[ mcpp_optind];
                if (cp && *cp != '-')           /* -MD (-MMD) file  */
                    mkdep_md = argv[ mcpp_optind++];
            }
            mkdep |= MD_MKDEP;
            break;

#if SYS_FAMILY == SYS_UNIX
        case 'm':
            if (str_eq( mcpp_optarg, "64")) {               /* -m64 */
                if (str_eq( CPU, "i386"))
                    strcpy( arch, "x86_64");
                else if (str_eq( CPU, "ppc"))
                    strcpy( arch, "ppc64");
                /* Else ignore  */
                break;
            } else if (str_eq( mcpp_optarg, "32")) {        /* -m32 */
                if (str_eq( CPU, "x86_64"))
                    strcpy( arch, "i386");
                else if (str_eq( CPU, "ppc64"))
                    strcpy( arch, "ppc");
                /* Else ignore  */
                break;
            } else if (str_eq( mcpp_optarg, "mmx")) {   /* -mmmx    */
                look_and_install( "__MMX__", DEF_NOARGS_PREDEF, null, "1");
                break;
            } else if (str_eq( mcpp_optarg, "no-mmx")) {    /* -mno-mmx     */
                undef_list[ undef_cnt++] = "__MMX__";
                break;
            }
#endif  /* SYS_FAMILY == UNIX   */
#if COMPILER == GNUC
#if SYSTEM == SYS_CYGWIN
            if (str_eq( mcpp_optarg, "no-cygwin")) {    /* -mno-cygwin      */
                no_cygwin = TRUE;
                break;
            }
#endif
            if (! integrated_cpp)
                usage( opt);
            break;

        case 'u':
            if (! str_eq( mcpp_optarg, "ndef"))     /* -undef       */
                usage( opt);                /* Else fall through    */
#endif  /* COMPILER == GNUC */

#if COMPILER == MSC
        case 'u':
#endif
        case 'N':
            /* No predefines:   remove "unix", "__unix__" and friends.  */
            nflag = TRUE;
            break;

#if COMPILER == GNUC || NO_DIR
        case 'n':
#if NO_DIR
            if (str_eq( mcpp_optarg, "odir")) {     /* -nodir       */
                no_dir = TRUE;
            }
#endif
#if COMPILER == GNUC
            if (str_eq( mcpp_optarg, "ostdinc")) {  /* -nostdinc    */
                unset_sys_dirs = TRUE;  /* Unset pre-specified directories  */
            } else if (str_eq( mcpp_optarg, "ostdinc++")) { /* -nostdinc++  */
                set_cplus_dir = FALSE;  /* Unset C++-specific directories   */
            } else if (str_eq( mcpp_optarg, "oprecomp")) {  /* -noprecomp   */
                mcpp_fprintf( ERR, warning, opt, mcpp_optarg);
                break;
            }
#endif
            else {
                usage( opt);
            }
            break;
#endif

#if COMPILER == GNUC
        case 'O':
            if (integrated_cpp) {
                if (*mcpp_optarg == '-')            /* No argument  */
                    mcpp_optind--;
                else if ((isdigit( *mcpp_optarg) && *mcpp_optarg != '0')
                        || *mcpp_optarg == 's' || *mcpp_optarg == 'z')
                                            /* -O1, -O2 -Os, -Oz    */
                    look_and_install( "__OPTIMIZE__", DEF_NOARGS_PREDEF, null
                            , "1");
                else if (! isdigit( *mcpp_optarg))
                    usage( opt);
                /* Else -O0: ignore */
            } else {
                usage( opt);
            }
            break;                  /* Else ignore -Ox option       */
#elif COMPILER == LCC
        case 'O':                   /* Define __LCCOPTIMLEVEL as 1  */
            defp = look_id( optim_name);
            strcpy( defp->repl, "1");
            break;
#endif

        case 'o':
            *out_pp = mcpp_optarg;          /* Output file name     */
            break;

        case 'P':                           /* No #line output      */
            option_flags.p = TRUE;
            break;

#if COMPILER == GNUC
        case 'p':
            if (str_eq( mcpp_optarg, "edantic")     /* -pedantic    */
                    || str_eq( mcpp_optarg, "edantic-errors")) {
                                            /* -pedantic-errors     */
                /* This option does not imply -ansi */
                if (warn_level == -1)
                    warn_level = 0;
                warn_level |= (1 | 2 | 4);
                if (! sflag && ! cplus_val) {
                    stdc_val = 1;
                    sflag = TRUE;
                }
            } else {
                usage( opt);
            }
            break;
        case 'q':
            if (str_eq( mcpp_optarg, "uiet"))
                /* -quiet: GCC's undocumented, yet frequently specified opt */
                break;                      /* Ignore the option    */
            else
                usage( opt);
            break;
#endif  /* COMPILER == GNUC */

        case 'Q':
            option_flags.q = TRUE;
            break;

#if COMPILER == MSC
        case 'R':               /* -RTC1, -RTCc, -RTCs, -RTCu, etc. */
            if (memcmp( mcpp_optarg, "TC", 2) == 0
                    && *(mcpp_optarg + 2) != EOS)
                look_and_install( "__MSVC_RUNTIME_CHECKS", DEF_NOARGS_PREDEF
                        , null, "1");
            else
                usage( opt);
            break;
#endif

        case 'S':
            if (cplus_val || sflag) {   /* C++ or the second time   */
                mcpp_fprintf( ERR, warning, opt, mcpp_optarg);
                break;
            }
            i = *mcpp_optarg;
            if (! isdigit( i) || *(mcpp_optarg + 1) != EOS)
                usage( opt);
            stdc_val = i - '0';
            sflag = TRUE;
            break;

#if COMPILER == GNUC
        case 'r':
            if (str_eq( mcpp_optarg, "emap"))
                mcpp_fprintf( ERR, warning, opt, mcpp_optarg);
                                            /* Ignore -remap option */
            else
                usage( opt);
            break;

        case 's':
            if (memcmp( mcpp_optarg, "td=", 3) == 0
                    && strlen( mcpp_optarg) > 3) {  /* -std=STANDARD*/
                cp = mcpp_optarg + 3;
                if (str_eq( cp, "c89")              /* std=c89      */
                        || str_eq( cp, "c90")       /* std=c90      */
                        || str_eq( cp, "gnu89")     /* std=gnu89    */
                        || str_eq( cp, "iso9899:1990")) {
                    std_val = 0L;               /* C90 + extensions */
                } else if (str_eq( cp, "c99")       /* std=c99      */
                        || str_eq( cp, "c9x")       /* std=c9x      */
                        || str_eq( cp, "gnu99")     /* std=gnu99    */
                        || str_eq( cp, "gnu9x")     /* std=gnu9x    */
                        || str_eq( cp, "iso9899:1999")
                        || str_eq( cp, "iso9899:199x")) {
                    std_val = 199901L;
                } else if (str_eq( cp, "c++98")) {  /* std=c++98    */
                    cplus_val = std_val = 199711L;
                } else if (memcmp( cp, "iso9899:", 8) == 0
                        && strlen( cp) >= 14) { /* std=iso9899:199409, etc. */
                    mcpp_optarg = cp + 8;
                    look_and_install( "__STRICT_ANSI__", DEF_NOARGS_PREDEF
                            , null, "1");
                    ansi = TRUE;
                    goto Version;
                } else if (memcmp( cp, "iso14882", 8) == 0) {
                    cp += 8;
                    ansi = TRUE;
                    if (cp && *cp == ':' && strlen( cp) >= 7) {
                                    /* std=iso14882:199711, etc.    */
                        cplus_val = CPLUS;
                        mcpp_optarg = cp + 1;
                        goto Version;
                    } else {
                        goto plus;
                    }
                } else {
                    usage( opt);
                }
                if (! cplus_val && memcmp( cp, "gnu", 3) != 0) {
                    /* 'std=gnu*' does not imply -ansi  */
                    look_and_install( "__STRICT_ANSI__", DEF_NOARGS_PREDEF
                            , null, "1");
                    ansi = TRUE;
                }
                stdc_val = 1;
                sflag = TRUE;
            } else {
                usage( opt);
            }
            break;

        case 't':
            if (str_eq( mcpp_optarg, "raditional")
                    || str_eq( mcpp_optarg, "raditional-cpp")) {
                                /* -traditional, -traditional-cpp   */
                trad = TRUE;
                mcpp_mode = OLD_PREP;
            } else if (str_eq( mcpp_optarg, "rigraphs")) {
                option_flags.trig = TRUE;           /* -trigraphs   */
            } else {
                usage( opt);
            }
            break;
#endif  /* COMPILER == GNUC */

#if COMPILER == MSC
        case 'T':
            if (strlen( mcpp_optarg) > 1)
                usage( opt);
            i = tolower( *mcpp_optarg);             /* Fold case    */
            if (i == 'c') {
                ;                           /* Ignore this option   */
            } else if (i == 'p') {
                cplus_val = CPLUS;
            } else {
                usage( opt);
            }
            break;
#endif

        case 'U':                           /* Undefine macro       */
            if (undef_cnt >= MAX_UNDEF) {
                mcpp_fputs( "Too many -U options.\n", ERR);
                longjmp( error_exit, -1);
            }
            undef_list[ undef_cnt++] = mcpp_optarg;
            break;

        case 'V':
#if COMPILER == GNUC
Version:
#endif
            valp = eval_num( mcpp_optarg);
            if (valp->sign == VAL_ERROR)
                usage( opt);
            std_val = (long) valp->val;
            break;

        case 'v':
            option_flags.v = TRUE;
            show_path = TRUE;
            break;

        case 'W':                           /* warning level        */
            if (warn_level == -1)           /* Have to initialize   */
                warn_level = 0;
#if COMPILER == GNUC
            if (argv[ mcpp_optind - 1] == mcpp_optarg) {    /* No argument  */
                /*
                 * Note: -W without argument is not officially supported.
                 *  It may cause an error.
                 */
                warn_level |= (1 | 2 | 4 | 16);
                mcpp_optind--;
            } else if (str_eq( mcpp_optarg, "comment")
                        || str_eq( mcpp_optarg, "comments")
                        || str_eq( mcpp_optarg, "sign-compare")) {
                warn_level |= 1;
            } else if (str_eq( mcpp_optarg, "undef")) {
                warn_level |= 4;
            } else if (str_eq( mcpp_optarg, "all")) {
                warn_level |= (1 | 16);     /* Convert -Wall to -W17*/
            } else if (str_eq( mcpp_optarg, "trigraphs")) {
                warn_level |= 16;
            }
#endif  /* COMPILER == GNUC */
#if COMPILER == MSC
            if (str_eq( mcpp_optarg, "all")) {
                warn_level |= (1 | 16);     /* Convert -Wall to -W17*/
            } else if (str_eq( mcpp_optarg, "L")) {
                option_flags.no_source_line = TRUE;
                                        /* Single-line diagnostic   */
            }
#endif
            if (isdigit( *mcpp_optarg)) {
                warn_level |= parse_warn_level( mcpp_optarg, opt);
                if (warn_level > 31 || warn_level < 0)
                    usage( opt);
            }
            if (warn_level == 0)
                warn_level = 0xFF;          /* Remember this option */
            /* Else ignore the option   */
            break;

#if COMPILER == GNUC || COMPILER == MSC
        case 'w':                           /* Same as -W0          */
            warn_level = 0xFF;              /* Remenber this option */
            break;
#endif

#if COMPILER == GNUC
        case 'x':
            if (str_eq( mcpp_optarg, "c")) {
                break;                      /* -x c -- ignore this  */
            } else if (str_eq( mcpp_optarg, "c++")) {
                goto plus;
            } else if (str_eq( mcpp_optarg, "assembler-with-cpp")) {
                option_flags.lang_asm = TRUE;
                break;
            } else {
                usage( opt);
            }
            break;
#endif

#if COMPILER == MSC
        case 'Z':
            if (str_eq( mcpp_optarg, "c:wchar_t")) {        /* -Zc:wchar_t  */
                look_and_install( "_NATIVE_WCHAR_T_DEFINED", DEF_NOARGS_PREDEF
                        , null, "1");
                look_and_install( "_WCHAR_T_DEFINED", DEF_NOARGS_PREDEF, null
                        , "1");
                wchar_t_modified = TRUE;
            } else if (str_eq( mcpp_optarg, "c:wchar_t-")) {/* -Zc:wchar_t- */
                wchar_t_modified = TRUE;        /* Do not define the macros */
            } else if (str_eq( mcpp_optarg, "l")) {
                look_and_install( "_VC_NODEFAULTLIB", DEF_NOARGS_PREDEF, null
                        , "1");
            } else if (str_eq( mcpp_optarg, "a")) {         /* -Za  */
                undefine( "_MSC_EXTENSIONS");
                option_flags.dollar_in_name = FALSE;
            } else if (str_eq( mcpp_optarg, "e")) {
                /* Ignore -Ze silently  */
                break;
            } else if (*(mcpp_optarg + 1) == EOS) {
                /* -Z followed by one char  */
                mcpp_fprintf( ERR, warning, opt, mcpp_optarg);
                /* Ignore the option with warning   */
            } else {
                usage( opt);
            }
            break;
#endif

        case 'z':
            option_flags.z = TRUE;  /* No output of included file   */
            break;

        default:                            /* What is this one?    */
            usage( opt);
            break;
        }                               /* Switch on all options    */

    }                                   /* For all arguments        */

    if (mcpp_optind < argc && set_files( argc, argv, in_pp, out_pp) != NULL)
        goto  opt_search;       /* More options after the filename  */

    /* Check consistency of specified options, set some variables   */
    chk_opts( sflag, trad);

    if (warn_level == -1)               /* No -W option             */
        warn_level = 1;                 /* Default warning level    */
    else if (warn_level == 0xFF)
        warn_level = 0;                 /* -W0 has high precedence  */

#if SYSTEM == SYS_MAC
    set_a_dir( NULL);                       /* Initialize incdir[]  */
    to_search_framework = incend;
                        /* Search framework[] next to the directory */
#endif

#if COMPILER == GNUC && SYSTEM == SYS_MAC
    if (arch[ 0]) {             /* -arch option has been specified  */
        if (((str_eq( CPU, "i386") || str_eq( CPU, "x86_64"))
                && (! str_eq( arch, "i386") && ! str_eq( arch, "x86_64")))
            || ((str_eq( CPU, "ppc") || str_eq( CPU, "ppc64"))
                && (! str_eq( arch, "ppc") && ! str_eq( arch, "ppc64")))) {
            mcpp_fprintf( ERR, "Wrong argument of -arch option: %s\n", arch);
            longjmp( error_exit, -1);
        }
    }
#endif
    if (! arch[ 0]) {
        /* None of -arch, -m32 or -m64 options has been specified.  */
        /* The CPU-specific-macros will be defined in init_cpu_macro(). */
        strcpy( arch, CPU);
    }
#if COMPILER != GNUC
    init_cpu_macro( gval, sse);
#endif

#if COMPILER == GNUC
    if (sysdir < sysdir_end) {
        char **     dp = sysdir;
        if (! sys_dirp || sys_dirp == incdir)
            sys_dirp = dp;
        while (dp < sysdir_end)
            set_a_dir( *dp++);
    }
    if (*in_pp && str_eq( (*in_pp) + strlen( *in_pp) - 2, ".S"))
        option_flags.lang_asm = TRUE;   /* Input file name is *.S   */
    if (option_flags.lang_asm) {
        look_and_install( "__ASSEMBLER__", DEF_NOARGS_PREDEF, null, "1");
        option_flags.dollar_in_name = FALSE;        /* Disable '$' in name  */
    }
    if (! sys_dirp || sys_dirp == incdir)
        sys_dirp = incend;
#endif
#if SYSTEM == SYS_MAC
    init_framework();                   /* After setting sys_dirp   */
#endif
    set_env_dirs();
    if (! unset_sys_dirs)
        set_sys_dirs( set_cplus_dir);

    if (mkdep_mf) {                         /* -MF overrides -MD    */
        mkdep_fp = fopen( mkdep_mf, "w");
    } else if (mkdep_md) {
        mkdep_fp = fopen( mkdep_md, "w");
    }
    if (mkdep_mq)                           /* -MQ overrides -MT    */
        mkdep_target = mkdep_mq;
    else if (mkdep_mt)
        mkdep_target = mkdep_mt;

    /* Normalize the path-list  */
    if (*in_pp && ! str_eq( *in_pp, "-")) {
        char *  tmp = norm_path( null, *in_pp, FALSE, FALSE);
        if (tmp)                        /* The file exists          */
            *in_pp = tmp;
            /* Else mcpp_main() will diagnose *in_pp and exit   */
    }
    if (! (mcpp_debug & MACRO_CALL)) {
        /* -K option alters behavior of -v option   */
        if (option_flags.v)
            version();
        if (show_path) {
            fp_debug = stderr;
            dump_path();
            fp_debug = stdout;
        }
    }
}

static void version( void)
/*
 * Print version message.
 */
{
    const char *    mes[] = {

#if     MCPP_LIB
/* Write messages here, for example, "MySomeTool with ".    */
#endif

#ifdef  VERSION_MSG
        "MCPP V.2.7.2 (2008/11) "
#else
        "MCPP V.", VERSION, " (", DATE, ") "
#endif
#if     COMPILER == INDEPENDENT
            , "compiler-independent-build "
#else
#ifdef  CMP_NAME
            , "for ", CMP_NAME, " "
#endif
#endif
            , "compiled by "
#ifdef  VERSION_MSG
            , VERSION_MSG
#else
#ifdef  HOST_CMP_NAME
            , HOST_CMP_NAME
#if     HOST_COMPILER == GNUC
            , " V.", GCC_MAJOR_VERSION, ".", GCC_MINOR_VERSION
#endif
#endif
#endif
            , "\n",
            NULL
        };

    const char **   mpp = mes;
    while (*mpp)
        mcpp_fputs( *mpp++, ERR);
}

static void usage(
    int     opt
)
/*
 * Print usage.
 */
{
    const char *     mes[] = {

"Usage:  ",
"mcpp",
" [-<opts> [-<opts>]] [<infile> [-<opts>] [<outfile>] [-<opts>]]\n",
"    <infile> defaults to stdin and <outfile> defaults to stdout.\n",

"\nCommonly used options:\n",

"-@MODE      Specify preprocessing mode. MODE should be one of these 4:\n",
"    -@std               Standard conforming mode. (default)\n",
"    -@poststd, -@post   special 'post-Standard' mode.\n",
"    -@kr                K&R 1st mode.\n",
"    -@oldprep, -@old    'old_preprocessor' mode (i.e. 'Reiser model' cpp).\n",

#if COMPILER == MSC
"-arch:SSE, -arch:SSE2   Define the macro _M_IX86_FP as 1, 2 respectively.\n",
#endif
#if SYSTEM == SYS_MAC && COMPILER == GNUC
"-arch <arch>        Change the target to <arch> (i386, x86_64, ppc, ppc64).\n",
#endif

#if ! STD_LINE_PREFIX
"-b          Output #line lines in C source style.\n",
#endif

"-C          Output also comments.\n",
"-D <macro>[=<value>]    Define <macro> as <value> (default:1).\n",
"-D <macro(args)>[=<replace>]    Define <macro(args)> as <replace>.\n",
"-e <encoding>   Change the default multi-byte character encoding to one of:\n",
"            euc_jp, gb2312, ksc5601, big5, sjis, iso2022_jp, utf8.\n",

#if SYSTEM == SYS_MAC
"-F <framework>      Add <framework> to top of framework directory list.\n",
#endif
#if COMPILER == GNUC
"-finput-charset=<encoding>      Same as -e <encoding>.\n",
"            (Don't insert spaces around '=').\n",
#endif
#if COMPILER == MSC
"-Fl <file>  Include the <file> prior to the main input file.\n",
"-G<n>       Define the macro _M_IX86 according to <n>.\n",
#endif
#if COMPILER == LCC
"-g <n>      Define the macro __LCCDEBUGLEVEL as <n>.\n",
#endif

"-I <directory>      Add <directory> to the #include search list.\n",

#if COMPILER == GNUC
"-isysroot <dir>     Change root of system header directory to <dir>.\n",
"-include <file>     Include the <file> prior to the main input file.\n",
#else
"-I-         Unset system or site specific include directories.\n",
#endif
#if COMPILER == MSC
"-J          Define the macro _CHAR_UNSIGNED as 1.\n",
#endif

"-j          Don't output the source line in diagnostics.\n",
"-M, -MM, -MD, -MMD, -MP, -MQ target, -MT target, -MF file\n",
"            Output source file dependency line for makefile.\n",
#if SYS_FAMILY == SYS_UNIX
"-m32        Change target CPU from x86_64, ppc64 to i386, ppc, respectively.\n",
"-m64        Change target CPU from i386, ppc to x86_64, ppc64, respectively.\n",
#endif
"-N          Don't predefine any non-standard macros.\n",

#if COMPILER == GNUC
"-nostdinc   Unset system or site specific include directories.\n",
#endif
#if COMPILER == LCC
"-O          Define the macro __LCCOPTIMLEVEL as 1.\n",
#endif

"-o <file>   Output to <file>.\n",
"-P          Don't output #line lines.\n",
"-Q          Output diagnostics to \"mcpp.err\" (default:stderr).\n",
#if COMPILER == MSC
"-RTC*       Define the macro __MSVC_RUNTIME_CHECKS as 1.\n",
#endif
#if COMPILER == GNUC
"-traditional, -traditional-cpp      Same as -@oldprep.\n",
#endif
"-U <macro>  Undefine <macro>.\n",

#if COMPILER == GNUC
"-undef      Same as -N.\n",
#endif
#if COMPILER == MSC
"-u          Same as -N.\n",
#endif

"-v          Show version and include directories of mcpp.\n",
"-W <level>  Set warning level to <level> (OR of {0,1,2,4,8,16}, default:1).\n",

#if COMPILER == MSC
"-WL         Same as -j.\n",
#endif
#if COMPILER == MSC || COMPILER == GNUC
"-w          Same as -W0.\n",
#endif
#if COMPILER == MSC
"-X          Same as -I-.\n",
"-Zc:wchar_t     Define _NATIVE_WCHAR_T_DEFINED and _WCHAR_T_DEFINED as 1.\n",
"-Zl         Define the macro _VC_NODEFAULTLIB as 1.\n",
#endif

"-z          Don't output the included file, only defining macros.\n",

"\nOptions available with -@std (default) or -@poststd options:\n",

"-+          Process C++ source.\n",

#if DIGRAPHS_INIT
"-2          Disable digraphs.\n",
#else
"-2          Enable digraphs.\n",
#endif
#if COMPILER == GNUC
"-digraphs   Enable digraphs.\n",
#endif

"-h <n>      Re-define the pre-defined macro __STDC_HOSTED__ as <n>.\n",

#if COMPILER == GNUC
"-lang-c89   Same as -S1.\n",
"-lang-c++   Same as -+.\n",
"-pedantic, -pedantic-errors     Same as -W7.\n",
"-S <n>      Redefine __STDC__ to <n>.\n",
#else
"-S <n>      Redefine __STDC__ to <n>, undefine old style macros.\n",
#endif

#if COMPILER == GNUC
"-std=<STANDARD>     Specify the standard to which the code should conform.\n",
"            <STANDARD> may be one of: c90, c99, iso9899:1990, iso14882, etc.\n",
"            iso9899:<n>, iso14882:<n> : Same as -V <n> (long in decimals).\n",
#endif
#if COMPILER == MSC
"-Tp         Same as -+.\n",
#endif

"-V <n>      Redefine __STDC_VERSION__ or __cplusplus to <n>.\n",
"            C with -V199901L specifies C99 specs.\n",
"            C++ with -V199901L specifies C99 compatible specs.\n",

#if COMPILER == GNUC
"-x c++      Same as -+.\n",
#endif

"\nOptions available with only -@std (default) option:\n",

"-@compat    Expand recursive macro more than Standard.\n",
#if TRIGRAPHS_INIT
"-3          Disable trigraphs.\n",
#else
"-3          Enable trigraphs.\n",
#endif
"-K          Output macro annotations embedding in comments.\n",
#if COMPILER == GNUC
"-trigraphs  Enable trigraphs.\n",
#endif

"\nOptions available with -@std (default), -@kr or -@oldprep options:\n",

#if COMPILER == GNUC
"-lang-asm   Same as -x assembler-with-cpp.\n",
"-x assembler-with-cpp   Process \"assembler\" source.\n",
#elif   COMPILER == MSC
"-A          Process \"assembler\" source.\n",
#else
"-a          Process \"assembler\" source.\n",
#endif

"-k          Keep white spaces of input lines as they are.\n",

"\nFor further details see mcpp-manual.html.\n",
        NULL,
    };

    const char *    illegopt = "Incorrect option -%c%s\n";
    const char * const *    mpp = mes;

    if (opt != '?')
        mcpp_fprintf( ERR, illegopt, opt, mcpp_optarg ? mcpp_optarg : null);
    version();
#if MCPP_LIB
    mes[ 1] = argv0;
#endif
    while (*mpp)
        mcpp_fputs( *mpp++, ERR);
    longjmp( error_exit, -1);
}

static void set_opt_list(
    char *  optlist
)
/*
 * Set list of legal option characters.
 */
{
    const char *    list[] = {

#if ! STD_LINE_PREFIX
    "b",
#endif

#if SYS_FAMILY == SYS_UNIX
    "m:",
#endif

#if COMPILER == GNUC
    "$A:a:cd:Ef:g:i:l:r:s:t:u:O:p:q:wx:",
#elif COMPILER == MSC
    "Aa:F:G:JR:T:XZ:uw",
#elif   COMPILER == LCC
    "g:O",
#endif

#if COMPILER != GNUC && COMPILER != MSC
    "a",
#endif
#if SYSTEM == SYS_MAC
    "F:-:",
#endif

    NULL
    };

    const char * const *    lp = & list[ 0];

    strcpy( optlist, "23+@:e:h:jkn:o:vzCD:I:KM:NPQS:U:V:W:");
                                                /* Default options  */
    while (*lp)
        strcat( optlist, *lp++);
    if (strlen( optlist) >= OPTLISTLEN)
        cfatal( "Bug: Too long option list", NULL, 0L, NULL);       /* _F_  */
}

static int  parse_warn_level(
    const char *    mcpp_optarg,
    int     opt
)
/*
 * Parse warn level option.
 * Warning level option is specified as '19' or '1|2|16' or even '3|16'.
 * Even spaces are allowed as ' 1 | 2|16 '.
 */
{
    const char *    cp = mcpp_optarg;
    int             w, i;

    w = i = 0;
    while( *cp != EOS) {
        while( *cp == ' ')
            cp++;                           /* Skip spaces          */
        if (! isdigit( *cp))
            break;                          /* Error    */
        while (isdigit( *cp)) {
            i *= 10;
            i += (*cp++ - '0');
        }
        while (*cp == ' ')
            cp++;
        if (*cp == '|') {       /* Only digits or '|' are allowed   */
            w |= i;                         /* Take OR of the args  */
            i = 0;
            cp++;
        }
    }
    if (*cp != EOS) {               /* Not ending with digit        */
        mcpp_fprintf( ERR, "Illegal warning level option \"%s\"\n"
                , mcpp_optarg);
        usage( opt);
    }
    w |= i;                                 /* Take the last arg    */
    return  w;
}

static void def_a_macro(
    int     opt,                            /* 'D'  */
    char *  def                         /* Argument of -D option    */
)
/*
 * Define a macro specified by -D option.
 * The macro maybe either object-like or function-like (with parameter).
 */
{
    DEFBUF *    defp;
    char *      definition;             /* Argument of -D option    */
    char *      cp;
    int         i;

    /* Convert trigraphs for the environment which need trigraphs   */
    if (mcpp_mode == STD && option_flags.trig)
        cnv_trigraph( def);
    if (mcpp_mode == POST_STD && option_flags.dig)
        cnv_digraph( def);  /* Convert prior to installing macro    */
    definition = xmalloc( strlen( def) + 4);
    strcpy( definition, def);
    if ((cp = strchr( definition, '=')) != NULL) {
        *cp = ' ';                          /* Remove the '='       */
        cp = "\n";                          /* Append <newline>     */
    } else {
        cp = " 1\n";                        /* With definition "1"  */
    }
    strcat( definition, cp);
    cp = definition;
    while ((char_type[ *cp & UCHARMAX] & SPA) == 0)
        cp++;
    i = *cp;
    *cp = EOS;
    if ((defp = look_id( definition)) != NULL)      /* Pre-defined  */
        undefine( definition);
    *cp = i;
    /* Now, save the definition.    */
    unget_string( definition, NULL);
    if (do_define( FALSE, 0) == NULL)       /* Define a macro       */
        usage( opt);
    *cp = EOS;
    if (str_eq( definition, "__STDC__")) {
        defp = look_id( definition);
        defp->nargs = DEF_NOARGS_STANDARD;
                                /* Restore Standard-predefinedness  */
    }
    free( definition);
    skip_nl();                      /* Clear the appended <newline> */
}

static void     chk_opts(
    int     sflag,      /* Flag of Standard or post-Standard mode   */
    int     trad                    /* -traditional (GCC only)      */
)
/*
 * Check consistency between the specified options.
 * Set default value of some variables for each 'mcpp_mode'.
 */
{
    int     incompat = FALSE;

    switch (mcpp_mode) {
    case STD    :
    case POST_STD   :
        if (trad)
            incompat = TRUE;
        if (! stdc_val)
            stdc_val = STDC;
        break;
    case KR :
    case OLD_PREP   :
#if COMPILER == GNUC
        if (sflag || cplus_val || ansi || std_val != -1L)
#else
        if (sflag || cplus_val || std_val != -1L)
#endif
            incompat = TRUE;
        if (option_flags.dig) {
            if (option_flags.dig != DIGRAPHS_INIT)
                incompat = TRUE;
            else
                option_flags.dig = 0;
        }
        break;
    }

    if (mcpp_mode == POST_STD
            && (option_flags.lang_asm || compat_mode || option_flags.k))
        incompat = TRUE;
    if (mcpp_mode != STD && option_flags.trig) {
        if (option_flags.trig != TRIGRAPHS_INIT)
            incompat = TRUE;
        else
            option_flags.trig = FALSE;
    }
    if (mcpp_mode != STD && (mcpp_debug & MACRO_CALL))
        incompat = TRUE;
    if ((mcpp_debug & MACRO_CALL)
                && (option_flags.lang_asm || option_flags.c)) {
            mcpp_fputs( "Disabled -K option.\n", ERR);
            mcpp_debug &= ~MACRO_CALL;
            /* -a and -C options do not co-exist with -K    */
    }
    if (incompat) {
        mcpp_fputs( "Incompatible options are specified.\n", ERR);
        usage( '?');
    }

    standard = (mcpp_mode == STD || mcpp_mode == POST_STD);
    /* Modify magic characters in character type table. */
    if (! standard)
        char_type[ DEF_MAGIC] = 0;
    if (mcpp_mode != STD)
        char_type[ IN_SRC] = 0;
    if (mcpp_mode == POST_STD || mcpp_mode == KR)
        char_type[ TOK_SEP] = 0;    /* TOK_SEP equals to COM_SEP    */
    if (mcpp_mode != STD)
        char_type[ MAC_INF] = 0;

    expand_init( compat_mode, ansi);
                /* Set function pointer to macro expansion routine  */
}

#if COMPILER != GNUC

static void init_cpu_macro (
    int     gval,               /* Argument of -G option for MSC    */
    int     sse                 /* Argument of -sse: option for MSC */
)
/*
 * Set predefined macros for CPU.
 * This routine is called from do_options() only once.
 * GCC-specific-build defines these macro by init_gcc_macro().
 */
{
    const char *    cpu_macro[][ 7] = {
#if SYS_FAMILY == SYS_UNIX
            { "__i386__"
#if SYSTEM == SYS_CYGWIN
                , "_X86_"
#endif
                , NULL,},
            { "__x86_64__", "__amd64__", NULL,},
            { "__ppc__", "__powerpc__", NULL,},
            { "__ppc64__", "__powerpc64__", NULL,}
#elif   SYS_FAMILY == SYS_WIN
            { "_WIN32", "__WIN32__", "_X86_"
#if SYSTEM == SYS_MINGW
                , "__i386__"
#endif
                , NULL,},
            { "_WIN32", "_WIN64", "__WIN64__", "_M_AMD64", "_AMD64_", "_X64_"
                , NULL,},       /* "_WIN32" is defined even on Windows 64   */
            { NULL,},                               /* Not supported  */
            { NULL,}                                /* Not supported  */
#endif
        };
    const char **   macro;
    int             index;

    if (str_eq( arch, "i386"))
        index = 0;
    else if (str_eq( arch, "x86_64"))
        index = 1;
    else if (str_eq( arch, "ppc"))
        index = 2;
    else if (str_eq( arch, "ppc64"))
        index = 3;
    else
        index = 9;

    if (index == 9) {                               /* Unknown CPU  */
        look_and_install( "__" CPU "__", DEF_NOARGS_PREDEF, null, "1");
        return;
    }
    macro = cpu_macro[ index];
    while (*macro)
        look_and_install( *macro++, DEF_NOARGS_PREDEF, null, "1");
#if SYS_FAMILY == SYS_WIN
    if (index == 0) {
        char    val[] = "600";
        if (gval)
            val[ 0] = gval;
        look_and_install( "_M_IX86", DEF_NOARGS_PREDEF, null, val);
        val[ 0] = '0' + sse;
        val[ 1] = '\0';
        look_and_install( "_M_IX86_FP", DEF_NOARGS_PREDEF, null, val);
    }
#endif
}
#endif  /* COMPILER != GNUC */

static void init_predefines( void)
/*
 * Set or unset predefined macros.
 */
{
    char    tmp[ 16];

    if (std_val != -1L) {               /* Version is specified     */
        if (cplus_val)
            cplus_val = std_val;        /* Value of __cplusplus     */
        else
            stdc_ver = std_val;     /* Value of __STDC_VERSION__    */
    } else {
        if (! cplus_val)
            stdc_ver = stdc_val ? STDC_VERSION : 0L;
    }

    if (nflag) {
        un_predefine( TRUE);
    } else if (stdc_val || cplus_val) {
#if COMPILER != GNUC
        un_predefine( FALSE);           /* Undefine "unix" or so    */
#endif
    }
    sprintf( tmp, "%ldL", cplus_val ? cplus_val : stdc_ver);
    if (cplus_val) {
        look_and_install( "__cplusplus", DEF_NOARGS_STANDARD, null, tmp);
    } else {
        if (stdc_ver)
            look_and_install( "__STDC_VERSION__", DEF_NOARGS_STANDARD, null
                    , tmp);
#ifdef  COMPILER_CPLUS
        if (! nflag)        /* Undefine pre-defined macro for C++   */
            undefine( COMPILER_CPLUS);
#endif
    }
    set_limit();
    stdc2 = cplus_val || stdc_ver >= 199901L;
    stdc3 = (cplus_val >= 199901L) || (stdc_ver >= 199901L);
        /* (cplus_val >= 199901L) makes C++ C99-compatible specs    */
    if (standard)
        init_std_defines();
    if (stdc3)
        set_pragma_op();
}

static void init_std_defines( void)
/*
 * For STD and POST_STD modes.
 * The magic pre-defines are initialized with magic argument counts.
 * expand_macro() notices this and calls the appropriate routine.
 * DEF_NOARGS is one greater than the first "magic" definition.
 * 'DEF_NOARGS - n' are reserved for pre-defined macros.
 * __STDC_VERSION__ and __cplusplus are defined by chk_opts() and set_cplus().
 */
{
    char    tmp[ 16];
    char    timestr[ 14];
    time_t  tvec;
    char *  tstring;

    look_and_install( "__LINE__", DEF_NOARGS_DYNAMIC - 1, null, "-1234567890");
    /* Room for 11 chars (10 for long and 1 for '-' in case of wrap round.  */
    look_and_install( "__FILE__", DEF_NOARGS_DYNAMIC - 2, null, null);
                                            /* Should be stuffed    */

    /* Define __DATE__, __TIME__ as present date and time.          */
    time( &tvec);
    tstring = ctime( &tvec);
    sprintf( timestr, "\"%.3s %c%c %.4s\"",
        tstring + 4,
        *(tstring + 8) == '0' ? ' ' : *(tstring + 8),
        *(tstring + 9),
        tstring + 20);
    look_and_install( "__DATE__", DEF_NOARGS_DYNAMIC, null, timestr);
    sprintf( timestr, "\"%.8s\"", tstring + 11);
    look_and_install( "__TIME__", DEF_NOARGS_DYNAMIC, null, timestr);

    if (! look_id( "__STDC_HOSTED__")) {
        /*
         * Some compilers, e.g. GCC older than 3.3, define this macro by
         * -D option.
         */
        sprintf( tmp, "%d", STDC_HOSTED);
        look_and_install( "__STDC_HOSTED__", DEF_NOARGS_PREDEF, null, tmp);
    }
#if COMPILER != GNUC        /* GCC do not undefine __STDC__ on C++  */
    if (cplus_val)
        return;
#endif
    /* Define __STDC__ as 1 or such for Standard conforming compiler.   */
    if (! look_id( "__STDC__")) {
        sprintf( tmp, "%d", stdc_val);
        look_and_install( "__STDC__", DEF_NOARGS_STANDARD, null, tmp);
    }
}

#define LINE90LIMIT         32767
#define LINE_CPLUS_LIMIT    32767

static void set_limit( void)
/*
 * Set the minimum translation limits specified by the Standards.
 */
{
    if (cplus_val) {            /* Specified by C++ 1998 Standard   */
        std_limits.str_len = SLEN_CPLUS_MIN;
        std_limits.id_len = IDLEN_CPLUS_MIN;
        std_limits.n_mac_pars = NMACPARS_CPLUS_MIN;
        std_limits.exp_nest = EXP_NEST_CPLUS_MIN;
        std_limits.blk_nest = BLK_NEST_CPLUS_MIN;
        std_limits.inc_nest = INCLUDE_NEST_CPLUS_MIN;
        std_limits.n_macro = NMACRO_CPLUS_MIN;
        std_limits.line_num = LINE_CPLUS_LIMIT;
    } else if (stdc_ver >= 199901L) {       /* Specified by C 1999 Standard */
        std_limits.str_len = SLEN99MIN;
        std_limits.id_len = IDLEN99MIN;
        std_limits.n_mac_pars = NMACPARS99MIN;
        std_limits.exp_nest = EXP_NEST99MIN;
        std_limits.blk_nest = BLK_NEST99MIN;
        std_limits.inc_nest = INCLUDE_NEST99MIN;
        std_limits.n_macro = NMACRO99MIN;
        std_limits.line_num = LINE99LIMIT;
    } else if (standard) {          /* Specified by C 1990 Standard */
        std_limits.str_len = SLEN90MIN;
        std_limits.id_len = IDLEN90MIN;
        std_limits.n_mac_pars = NMACPARS90MIN;
        std_limits.exp_nest = EXP_NEST90MIN;
        std_limits.blk_nest = BLK_NEST90MIN;
        std_limits.inc_nest = INCLUDE_NEST90MIN;
        std_limits.n_macro = NMACRO90MIN;
        std_limits.line_num = LINE90LIMIT;
    }
    /* Else pre-Standard mode   */
}

static void set_pragma_op( void)
/*
 *      #define _Pragma(a)  _Pragma ( a )
 * Define _Pragma() operator as a special macro so as to be searched
 * easily.  The unusual 'DEF_PRAGMA' is a marker of this psuedo
 * macro.
 */
{
    char *  name = "_Pragma";
    char    tmp[ 16];

    sprintf( tmp, "%c%s ( %c%c )", DEF_MAGIC, name, MAC_PARM, 1);
                                                /* Replacement text */
    look_and_install( name, DEF_PRAGMA, "a", tmp);
}

void    init_sys_macro( void)
/*
 * Define system-specific macros and some Standard required macros
 * and undefine macros specified by -U options.
 */
{
    /* This order is important. */
    def_macros();               /* Define macros specified by -D    */
#if COMPILER == GNUC
    chk_env();
#endif
    init_predefines();                  /* Define predefined macros */
#if COMPILER == GNUC
    init_gcc_macro();
#elif   COMPILER == MSC
    init_msc_macro();
#endif
    undef_macros();             /* Undefine macros specified by -U  */
    if (mcpp_debug & MACRO_CALL)
        dump_def( FALSE, TRUE);     /* Finally putout current macro names   */
}

void    at_start( void)
/*
 * Do the commands prior to processing main source file after do_options().
 */
{
    char *  env;
    FILEINFO *      file_saved = infile;

    /*
     * Set multi-byte character encoding according to environment variables
     * LC_ALL, LC_CTYPE and LANG -- with preference in this order.
     */
    if (! mb_changed) {                     /* -m option precedes   */
        if ((env = getenv( "LC_ALL")) != NULL)
            set_encoding( env, "LC_ALL", 0);
        else if ((env = getenv( "LC_CTYPE")) != NULL)
            set_encoding( env, "LC_CTYPE", 0);
        else if ((env = getenv( "LANG")) != NULL)
            set_encoding( env, "LANG", 0);
    }

#if COMPILER == GNUC || COMPILER == MSC
    if (option_flags.dollar_in_name)
        char_type[ 0x24] |= LET;    /* Enable '$' in identifiers    */
    /*
     * Do the -include (-Fl for MSC) options in the specified order.
     * Note: This functionality is implemented as nested #includes
     *   which results the same effect as sequential #includes.
     */
    {
        char **         preinc;
        /*
         * Note: Here, 'infile' is the main input file, which is pseudo-
         * parent file of the files to pre-include.  So, we must
         * temporarily set the infile's directory to the current directory
         * in order to preinclude the files relative to it.
         */
        preinc = preinc_end;
        while (preinclude <= --preinc && *preinc != NULL)
            open_file( &null, NULL, *preinc, TRUE, TRUE, FALSE);
    }
#endif

    put_info( file_saved);
}

static void put_info(
    FILEINFO *  sharp_file
)
/*
 * Putout compiler-specific information.
 */
{
    if (no_output || option_flags.p)
        return;
    sharp_file->line++;
    sharp( sharp_file, 0);
    sharp_file->line--;
#if COMPILER == GNUC
    if (gcc_work_dir)
        mcpp_fprintf( OUT, "%s%ld \"%s%c\"\n"
                , std_line_prefix ? "#line " : LINE_PREFIX
                , 1, cur_work_dir, '/');
        /* Putout the current directory as a #line line as: */
        /* '# 1 "/abs-path/cur_dir//"'.                     */
    mcpp_fprintf( OUT, "%s%ld \"<built-in>\"\n"
                , std_line_prefix ? "#line " : LINE_PREFIX , 1);
    mcpp_fprintf( OUT, "%s%ld \"<command line>\"\n"
                , std_line_prefix ? "#line " : LINE_PREFIX , 1);
    mcpp_fprintf( OUT, "%s%ld \"%s\"%s\n"
            , std_line_prefix ? "#line " : LINE_PREFIX, 1, cur_fullname
            , ! str_eq( cur_fullname, sharp_file->full_fname) ? " 1" : null);
            /* Suffix " 1" for the file specified by -include   */
#endif
}

static char *   set_files(
    int     argc,
    char ** argv,
    char ** in_pp,
    char ** out_pp
)
/*
 * Set input and/or output files.
 */
{
    char *      cp;

    if (*in_pp == NULL) {                           /* Input file   */
        cp = argv[ mcpp_optind++];
#if SYS_FAMILY == SYS_WIN
        cp = bsl2sl( cp);
#endif
        *in_pp = cp;
    }
    if (mcpp_optind < argc && argv[ mcpp_optind][ 0] != '-'
            && *out_pp == NULL) {
        cp = argv[ mcpp_optind++];
#if SYS_FAMILY == SYS_WIN
        cp = bsl2sl( cp);
#endif
        *out_pp = cp;                               /* Output file  */
    }
    if (mcpp_optind >= argc)
        return  NULL;           /* Exhausted command line arguments */
    if (argv[ mcpp_optind][ 0] == '-')
        return  argv[ mcpp_optind];                 /* More options */
    cfatal( "Excessive file argument \"%s\"", argv[ mcpp_optind], 0L , NULL);
    return  NULL;
}

static void set_env_dirs( void)
/*
 * Add to include path those specified by environment variables.
 */
{
    const char *    env;

    if (cplus_val) {
        if ((env = getenv( ENV_CPLUS_INCLUDE_DIR)) != NULL)
            parse_env( env);
    }
    if ((env = getenv( ENV_C_INCLUDE_DIR)) != NULL)
        parse_env( env);
}

static void parse_env(
    const char *    env
)
/*
 * Parse environmental variable and append the path to include-dir-list.
 */
{
    char *  save;
    char *  save_start;
    char *  p;
    int     sep;

    save = save_start = save_string( env);
    while (*save) {
        p = save;
        while (*p && *p != ENV_SEP)
            p++;
        if (p != save)  {                   /* Variable separator   */
            sep = *p;
            *p = EOS;
            set_a_dir( save);
            if (sep == EOS)
                break;
            save = ++p;
        }
        while (*save == ENV_SEP)
            ++save;
    }
    free( save_start);
}

static void set_sys_dirs(
    int     set_cplus_dir       /* Set C++ include-directory too    */
)
/*
 * Set site-specific and system-specific directories to the include directory
 * list.
 */
{
    if (cplus_val && set_cplus_dir) {
#ifdef  CPLUS_INCLUDE_DIR1
        set_a_dir( CPLUS_INCLUDE_DIR1);
#endif
#ifdef  CPLUS_INCLUDE_DIR2
        set_a_dir( CPLUS_INCLUDE_DIR2);
#endif
#ifdef  CPLUS_INCLUDE_DIR3
        set_a_dir( CPLUS_INCLUDE_DIR3);
#endif
#ifdef  CPLUS_INCLUDE_DIR4
        set_a_dir( CPLUS_INCLUDE_DIR4);
#endif
    }

#if SYS_FAMILY == SYS_UNIX
    set_a_dir( "/usr/local/include");
#endif

#ifdef  C_INCLUDE_DIR1
    set_a_dir( C_INCLUDE_DIR1);
#endif
#ifdef  C_INCLUDE_DIR2
    set_a_dir( C_INCLUDE_DIR2);
#endif

#if SYS_FAMILY == SYS_UNIX
#if SYSTEM == SYS_CYGWIN
    if (no_cygwin)                          /* -mno-cygwin          */
        set_a_dir( "/usr/include/mingw");
    else
        set_a_dir( "/usr/include");
#else
    set_a_dir( "/usr/include"); /* Should be placed after C_INCLUDE_DIR?    */
#endif
#endif
}

static void set_a_dir(
    const char *    dirname                 /* The path-name        */
)
/*
 * Append an include directory.
 * This routine is called from the following routines (in this order).
 * 1. do_options() by -I option.
 * 2. do_options() by -isystem option (for GNUC).
 * 3. set_env_dirs() by environment variables.
 * 4. set_sys_dirs() by CPLUS_INCLUDE_DIR?, C_INCLUDE_DIR? and system-
 *    specifics (unless -I- or -nostdinc option is specified).
 * Ignore non-existent directory.
 * Note that this routine should be called only in initializing steps,
 *      because increase of include dirs causes reallocation of incdir[].
 * Note: a trailing PATH-DELIM is appended by norm_path().
 */
{
    char *  norm_name;
    const char **   ip;

    if (incdir == NULL) {               /* Should be initialized    */
        max_inc = INIT_NUM_INCLUDE;
        incdir = (const char **) xmalloc( sizeof (char *) * max_inc);
        incend = &incdir[ 0];
    } else if (incend - incdir >= max_inc) {        /* Buffer full  */
#if SYSTEM == SYS_MAC
        size_t  framework_pos = to_search_framework - incdir;
#endif
#if COMPILER == GNUC
        size_t  sys_pos = 0;
        if (sys_dirp)
            sys_pos = sys_dirp - incdir;
#endif
        incdir = (const char **) xrealloc( (void *) incdir
                , sizeof (char *) * max_inc * 2);
        incend = &incdir[ max_inc];
#if COMPILER == GNUC
        if (sys_pos)
            sys_dirp = &incdir[ sys_pos];
#endif
#if SYSTEM == SYS_MAC
        to_search_framework = &incdir[ framework_pos];
#endif
        max_inc *= 2;
    }

    if (dirname == NULL)
        return;                     /* Only to initialize incdir[]  */
    norm_name = norm_dir( dirname, FALSE);
    if (! norm_name)                        /* Non-existent         */
        return;
    for (ip = incdir; ip < incend; ip++) {
        if (str_case_eq( *ip, norm_name)) {
            if (option_flags.v && ! (mcpp_debug & MACRO_CALL))
                mcpp_fprintf( ERR, "Duplicate directory \"%s\" is ignored\n"
                        , norm_name);
            free( norm_name);               /* Already registered   */
            return;
        }
    }
    /* Register new directory   */
    *incend++ = norm_name;
}

#if SYSTEM == SYS_MAC && COMPILER == GNUC
/* Definitions for "header map" file of Xcode / Apple-GCC.          */
/* These definitions were taken from cpplib.h of Apple-GCC-4.0.1    */

#define HMAP_SAME_ENDIANNESS_MAGIC      \
        (((((('h' << 8) | 'm') << 8) | 'a') << 8) | 'p')

typedef unsigned short  uint16;
typedef unsigned int    uint32;

struct hmap_bucket
{
    uint32  key;            /* Offset (into strings) of key             */
    struct {
        uint32  prefix;     /* Offset (into strings) of value prefix    */
        uint32  suffix;     /* Offset (into strings) of value suffix    */
    } value;                /* Value (prefix- and suffix-strings)       */
};

struct hmap_header_map
{
    uint32  magic;          /* Magic word, also indicates byte order    */
    uint16  version;        /* Version number -- currently 1            */
    uint16  _reserved;      /* Reserved for future use -- zero for now  */
    uint32  strings_offset; /* Offset to start of string pool           */
    uint32  count;          /* Number of entries in the string table    */
    uint32  capacity;       /* Number of buckets (always a power of 2)  */
    uint32  max_value_length;
                        /* Length of longest result path (excl. '\0')   */
    struct hmap_bucket  buckets[1];
                        /* Inline array of 'capacity' maptable buckets  */
    /* Strings follow the buckets, at strings_offset.  */
};
#endif

static char *   norm_dir(
    const char *    dirname,        /* Directory path to normalize  */
    int             framework       /* Setting a framework directory*/
)
/*
 * Normalize include directory path.
 * Handle -isysroot option for GCC, including framework directory for SYS_MAC.
 */
{
    char *  norm_name;

#if COMPILER == GNUC
    if (sysroot && sys_dirp) {
        /* Logical system root specified and dirname is system header dir   */
        char    delim[ 2] = { EOS, EOS};
        char *  dir;
#if SYSTEM == SYS_MAC
        if (! framework && memcmp( dirname, "/usr/", 5) != 0)
            return  NULL;           /* /Developer/usr/lib/gcc/      */
#endif
        if (dirname[ 0] != PATH_DELIM)
            delim[ 0] = PATH_DELIM;
        dir = xmalloc( strlen( sysroot) + strlen( dirname) + 2);
        sprintf( dir, "%s%s%s", sysroot, delim, dirname);
        dirname = dir;
    }
#endif
#if SYSTEM == SYS_MAC && COMPILER == GNUC
    if (strlen( dirname) > 5
            && str_case_eq( dirname + strlen( dirname) - 5, ".hmap")) {
        /* "header map" file (not an include directory) */
        norm_name = norm_path( null, dirname, FALSE, TRUE);
        if (! norm_name && option_flags.v)
            mcpp_fprintf( ERR, "Invalid header map file \"%s\" is ignored\n"
                    , dirname);
    } else
#endif
    {
        norm_name = norm_path( dirname, NULL, FALSE, FALSE);
                            /* Normalize the pathname to compare    */
        if (! norm_name && option_flags.v && ! (mcpp_debug & MACRO_CALL))
            mcpp_fprintf( ERR, "Non-existent directory \"%s\" is ignored\n"
                    , dirname);
    }
#if COMPILER == GNUC
    if (sysroot && sys_dirp)
        free( dirname);
#endif

    return  norm_name;
}

static char *   norm_path(
    const char *    dir,        /* Include directory (maybe "", never NULL) */
    const char *    fname,
        /* Filename (possibly has directory part, or maybe NULL)    */
    int     inf,    /* If TRUE, output some infs when (mcpp_debug & PATH)   */
    int     hmap            /* "header map" file of Apple-GCC       */
)
/*
 * Normalize the pathname removing redundant components such as
 * "foo/../", "./" and trailing "/.".
 * Append trailing "/" if 'fname' is NULL.
 * Change relative path to absolute path.
 * Dereference a symbolic linked file (or directory) to a real directory/file.
 * Return a malloc'ed buffer, if the directory/file exists.
 * Return NULL, if the specified directory/file does not exist or 'dir' is
 * not a directory or 'fname' is not a regular file.
 * This routine is called from set_a_dir(), init_gcc_macro(), do_once() and
 * open_file().
 */
{
    char *  norm_name;                  /* The path-list converted  */
    char *  start;
    char *  cp1;
    char *  cp2;
    char *  abs_path;
    int     len;                            /* Should not be size_t */
    size_t  start_pos = 0;
    char    slbuf1[ PATHMAX+1];             /* Working buffer       */
#if SYS_FAMILY == SYS_UNIX
    char    slbuf2[ PATHMAX+1]; /* Working buffer for dereferencing */
#endif
#if SYSTEM == SYS_CYGWIN || SYSTEM == SYS_MINGW
    static char *   root_dir;
                /* System's root directory in Windows file system   */
    static size_t   root_dir_len;
#if SYSTEM == SYS_CYGWIN
    static char *   cygdrive = "/cygdrive/";    /* Prefix for drive letter  */
#else
    static char *   mingw_dir;          /* "/mingw" dir in Windows  */
    static size_t   mingw_dir_len;
#endif
#endif
#if HOST_COMPILER == MSC
    struct _stat    st_buf;
#else
    struct stat     st_buf;
#endif

    if (! dir || (*dir && is_full_path( fname)))
        cfatal( "Bug: Wrong argument to norm_path()"        /* _F_  */
                , NULL, 0L, NULL);
    inf = inf && (mcpp_debug & PATH);       /* Output information   */

    strcpy( slbuf1, dir);                   /* Include directory    */
    len = strlen( slbuf1);
    if (fname && len && slbuf1[ len - 1] != PATH_DELIM) {
        slbuf1[ len] = PATH_DELIM;          /* Append PATH_DELIM    */
        slbuf1[ ++len] = EOS;
    } else if (! fname && len && slbuf1[ len - 1] == PATH_DELIM) {
        /* stat() of some systems do not like trailing '/'  */
        slbuf1[ --len] = EOS;
    }
    if (fname)
        strcat( slbuf1, fname);
    if (stat( slbuf1, & st_buf) != 0        /* Non-existent         */
            || (! fname && ! S_ISDIR( st_buf.st_mode))
                /* Not a directory though 'fname' is not specified  */
            || (fname && ! S_ISREG( st_buf.st_mode)))
                /* Not a regular file though 'fname' is specified   */
        return  NULL;
#if SYSTEM == SYS_MAC && COMPILER == GNUC
    if (hmap) {                         /* Dirty "header map" file  */
        struct hmap_header_map  hmap;
        size_t      cnt;
        FILE *      fp;
        fp = fopen( fname, "r");
        cnt = fread( & hmap, sizeof (struct hmap_header_map), 1, fp);
        fclose( fp);
        if (cnt == 0 || hmap.magic != HMAP_SAME_ENDIANNESS_MAGIC)
            return  NULL;
    }
#endif
    if (! fname) {
        slbuf1[ len] = PATH_DELIM;          /* Append PATH_DELIM    */
        slbuf1[ ++len] = EOS;
    }
#if SYS_FAMILY == SYS_UNIX
    /* Dereference symbolic linked directory or file, if any    */
    slbuf1[ len] = EOS;     /* Truncate PATH_DELIM and 'fname' part, if any */
    slbuf2[ 0] = EOS;
    if (*dir && ! fname) {      /* Registering include directory    */
        /* Symbolic link check of directories are required  */
        deref_syml( slbuf1, slbuf2, slbuf1);
    } else if (fname) {                             /* Regular file */
        len = strlen( slbuf1);
        strcat( slbuf1, fname);
        deref_syml( slbuf1, slbuf2, slbuf1 + len);
                                /* Symbolic link check of directory */
        if ((len = readlink( slbuf1, slbuf2, PATHMAX)) > 0) {
            /* Dereference symbolic linked file (not directory) */
            *(slbuf2 + len) = EOS;
            cp1 = slbuf1;
            if (slbuf2[ 0] != PATH_DELIM) {     /* Relative path    */
                cp2 = strrchr( slbuf1, PATH_DELIM);
                if (cp2)        /* Append to the source directory   */
                    cp1 = cp2 + 1;
            }
            strcpy( cp1, slbuf2);
        }
    }
    if (inf) {
        if (slbuf2[ 0])
            mcpp_fprintf( DBG, "Dereferenced \"%s%s\" to \"%s\"\n"
                    , dir, fname ? fname : null, slbuf1);
    }
#endif
    len = strlen( slbuf1);
    start = norm_name = xmalloc( len + 1);  /* Need a new buffer    */
    strcpy( norm_name, slbuf1);
#if SYS_FAMILY == SYS_WIN
    bsl2sl( norm_name);
#endif
#if SPECIAL_PATH_DELIM                  /* ':' ?    */
    for (cp1 = norm_name; *cp1 != EOS; cp1++) {
        if (*cp1 == PATH_DELIM)
            *cp1 = '/';
    }
#endif
    cp1 = norm_name;

#if SYSTEM == SYS_CYGWIN
    /* Convert to "/cygdirve/x/dir" style of absolute path-list     */
    if (len >= 8 && (memcmp( cp1, "/usr/bin", 8) == 0
                    || memcmp( cp1, "/usr/lib", 8) == 0)) {
        memmove( cp1, cp1 + 4, len - 4 + 1);    /* Remove "/usr"    */
        len -= 4;
    }
    if (*cp1 == '/' && (len < 10 || memcmp( cp1, cygdrive, 10) != 0)) {
        /* /dir, not /cygdrive/     */
        if (! root_dir_len) {           /* Should be initialized    */
            /* Convert "X:\DIR-list" to "/cygdrive/x/dir-list"      */
            root_dir = xmalloc( strlen( CYGWIN_ROOT_DIRECTORY) + 1);
            strcpy( root_dir, CYGWIN_ROOT_DIRECTORY);
            *(root_dir + 1) = *root_dir;        /* "x:/" to " x/"   */
            cp1 = xmalloc( strlen( cygdrive) + strlen( root_dir));
            strcpy( cp1, cygdrive);
            strcat( cp1, root_dir + 1);
            free( root_dir);
            root_dir = cp1;
            root_dir_len = strlen( root_dir);
        }
        cp1 = xmalloc( root_dir_len + len + 1);
        strcpy( cp1, root_dir);
        strcat( cp1, norm_name);        /* Convert to absolute path */
        free( norm_name);
        norm_name = start = cp1;
        len += root_dir_len;
    }
#endif

#if SYSTEM == SYS_MINGW
    /* Handle the mess of MinGW's path-list */
    /* Convert to "x:/dir" style of absolute path-list  */
    if (*cp1 == PATH_DELIM && isalpha( *(cp1 + 1))
            && *(cp1 + 2) == PATH_DELIM) {          /* /c/, /d/, etc*/
        *cp1 = *(cp1 + 1);
        *(cp1 + 1) = ':';               /* Convert to c:/, d:/, etc */
    } else if (memcmp( cp1, "/mingw", 6) == 0) {
        if (! mingw_dir_len) {          /* Should be initialized    */
            mingw_dir_len = strlen( MINGW_DIRECTORY);
            mingw_dir = xmalloc( mingw_dir_len + 1);
            strcpy( mingw_dir, MINGW_DIRECTORY);
        }
        cp1 = xmalloc( mingw_dir_len + len + 1);
        strcpy( cp1, mingw_dir);
        strcat( cp1, norm_name + 6);    /* Convert to absolute path */
        free( norm_name);
        norm_name = start = cp1;
        len += mingw_dir_len;
    } else if (memcmp( cp1, "/usr", 4) == 0) {
        memmove( cp1, cp1 + 4, len - 4 + 1);    /* Remove "/usr"    */
        len -= 4;
    }
    if (*cp1 == '/') {                  /* /dir or /                */
        if (! root_dir_len) {           /* Should be initialized    */
            root_dir_len = strlen( MSYS_ROOT_DIRECTORY);
            root_dir = xmalloc( root_dir_len + 1);
            strcpy( root_dir, MSYS_ROOT_DIRECTORY);
        }
        cp1 = xmalloc( root_dir_len + len + 1);
        strcpy( cp1, root_dir);
        strcat( cp1, norm_name);        /* Convert to absolute path */
        free( norm_name);
        norm_name = start = cp1;
        len += root_dir_len;
    }
#endif

#if SYS_FAMILY == SYS_WIN
    if (*(cp1 + 1) == ':')
        start = cp1 += 2;               /* Next to the drive letter */
    start_pos = 2;
#endif
    if (len == 1 && *norm_name == '/')              /* Only "/"     */
        return  norm_name;

    if (strncmp( cp1, "./", 2) == 0)    /* Remove beginning "./"    */
        memmove( cp1, cp1 + 2, strlen( cp1 + 2) + 1);       /* +1 for EOS   */
    if (*start != '/') {    /* Relative path to current directory   */
        /* Make absolute path   */
        abs_path = xmalloc( len + strlen( cur_work_dir) + 1);
        cp1 = stpcpy( abs_path, cur_work_dir);
        strcpy( cp1, start);
        free( norm_name);
        norm_name = abs_path;
        start = cp1 = norm_name + start_pos;
    }

    while ((cp1 = strstr( cp1, "/./")) != NULL)
        memmove( cp1, cp1 + 2, strlen( cp1 + 2) + 1);
                                        /* Remove "/." of "/./"     */
    cp1 = start;
    /* Remove redundant "foo/../"   */
    while ((cp1 = strstr( cp1, "/../")) != NULL) {
        *cp1 = EOS;
        if ((cp2 = strrchr( start, '/')) != NULL) {
            if (*(cp1 - 1) != '.') {
                memmove( cp2 + 1, cp1 + 4, strlen( cp1 + 4) + 1);
                                        /* Remove "foo/../"         */
                cp1 = cp2;
            } else {                                /* Impossible   */
                break;
            }
        } else {                                    /* Impossible   */
            break;
        }
    }

#if SPECIAL_PATH_DELIM
    for (cp1 = start; *cp1 != EOS; cp1++) {
        if (*cp1 == '/')
            *cp1 = PATH_DELIM;
    }
#endif
    if (inf) {
        char    debug_buf[ PATHMAX+1];
        strcpy( debug_buf, dir);
        strcat( debug_buf, fname ? fname : null);
#if SYS_FAMILY == SYS_WIN
        bsl2sl( debug_buf);
#endif
        if (! str_eq( debug_buf, norm_name))
            mcpp_fprintf( DBG, "Normalized the path \"%s\" to \"%s\"\n"
                    , debug_buf, norm_name);
    }

    return  norm_name;
}

#if SYS_FAMILY == SYS_UNIX

static void     deref_syml(
    char *      slbuf1,                     /* Original path-list   */
    char *      slbuf2,                     /* Working buffer       */
    char *      chk_start                   /* Pointer into slbuf1  */
)
/* Dereference symbolic linked directory    */
{
    char *      cp2;
    int         len;                /* Should be int, not size_t    */

    while ((chk_start = strchr( chk_start, PATH_DELIM)) != NULL) {
        *chk_start = EOS;
        if ((len = readlink( slbuf1, slbuf2, PATHMAX)) > 0) {
            /* Dereference symbolic linked directory    */
            cp2 = strrchr( slbuf1, PATH_DELIM); /* Previous delimiter       */
            *chk_start = PATH_DELIM;
            strcpy( slbuf2 + len, chk_start);
            if (slbuf2[ 0] == PATH_DELIM) {     /* Absolute path    */
                strcpy( slbuf1, slbuf2);
                chk_start = slbuf1 + len + 1;
            } else {
                if (cp2)
                    chk_start = cp2 + 1;
                else
                    chk_start = slbuf1;
                strcpy( chk_start, slbuf2);     /* Rewrite the path */
                chk_start += len;
            }
        } else {
            *chk_start++ = PATH_DELIM;
        }
    }
}
#endif

#if COMPILER == GNUC

static void init_gcc_macro( void)
/*
 * Predefine GCC macros.
 * This routine should be called after opening output file in order to putout
 * macro informations by -K option into the file.
 * Also this routine should be called before undef_macros() in order to
 * permit undefining a macro by -U option.
 */
{
    char        fname[ BUFSIZ];
    char        lbuf[ BUFSIZ];
    char *      include_dir;    /* The version-specific include directory   */
    char *      tmp;
    FILE *      fp;
    DEFBUF *    defp;
    const char *    cp;
    char *      tp;
    int         i;

    if (nflag)                                  /* -undef option    */
        goto  undef_special;

    tmp = xmalloc( strlen( INC_DIR) + strlen( "/mingw/mcpp-gcc-")
            + strlen( arch) + 1);
#if SYSTEM == SYS_CYGWIN
    if (no_cygwin) {
        sprintf( tmp, "%s/mingw/mcpp-gcc-%s", INC_DIR, arch);
    } else {
        sprintf( tmp, "%s/mcpp-gcc-%s", INC_DIR, arch);
    }
#else
    sprintf( tmp, "%s/mcpp-gcc-%s", INC_DIR, arch);
#endif
    include_dir = norm_path( tmp, NULL, TRUE, FALSE);
    free( tmp);

    for (i = 0; i <= 1; i++) {
        int         nargs;

        if ((mcpp_mode == POST_STD || ansi) && i == 0)
            continue;   /* POST_STD or __STRICT_ANSI__ does not     */
                        /*      predefine non-conforming macros     */
        /* The predefined macro file    */
        cp = i ? "std" : "old";
        sprintf( fname, "%sg%s%d%d_predef_%s.h"
                , include_dir, cplus_val ? "xx" : "cc"
                , gcc_maj_ver, gcc_min_ver, cp);
            /* Note that norm_path() append a PATH_DELIM.   */
        if ((fp = fopen( fname, "r")) == NULL) {
            mcpp_fprintf( ERR, "The mode for %s has not been installed.\n"
                    , arch);
            longjmp( error_exit, -1);
        }
        nargs = i ? 0 : DEF_NOARGS_PREDEF_OLD;
            /* g*_predef_std.h has DEF_NOARGS_PREDEF or non-negative args   */
            /* while g*_predef_old.h has only DEF_NOARGS_PREDEF_OLD args    */
        while (fgets( lbuf, BUFSIZ, fp) != NULL) {
            unget_string( lbuf, "gcc_predefine");
            if (skip_ws() == '#'
                && scan_token( skip_ws(), (tp = work_buf, &tp), work_end)
                        == NAM
                    && str_eq( work_buf, "define")) {
                defp = do_define( TRUE, nargs);     /* Ignore re-definition */
            }
            skip_nl();
        }
    }
    free( include_dir);

undef_special:
    if (look_id( "__OPTIMIZE__"))       /* -O option is specified   */
        undefine( "__NO_INLINE__");
}

static void chk_env( void)
/*
 * Check the environment variables to specify output of dependency lines.
 */
{
    char *  env;
    char *  cp;

    /* Output of dependency lines   */
    if ((env = getenv( "DEPENDENCIES_OUTPUT")) == NULL) {
        if ((env = getenv( "SUNPRO_DEPENDENCIES")) == NULL)
            return;
        else
            mkdep |= MD_SYSHEADER;
    }
    mkdep |= MD_MKDEP;
    if ((cp = strchr( env, ' ')) != NULL) {
        *cp++ = EOS;
        while (*cp == ' ')
            cp++;
    }
    if (! mkdep_fp)                 /* Command line option precedes */
        mkdep_fp = fopen( env, "a");
    if (! mkdep_target)
        mkdep_target = cp;
}

#elif   COMPILER == MSC

static void init_msc_macro( void)
/*
 * Define a few MSC-specific predefined macros.
 */
{
    DEFBUF *    defp;
    int         i;

    defp = look_id( "_MSC_VER");
    i = atoi( defp->repl);
    if (i >= 1400) {                        /* _MSC_VER >= 1400     */
        look_and_install( "_MT", DEF_NOARGS_PREDEF, null, "1");
        if (cplus_val && ! wchar_t_modified) {
            /* -Zc:wchar_t- was not specified   */
            look_and_install( "_NATIVE_WCHAR_T_DEFINED", DEF_NOARGS_PREDEF
                    , null, "1");
            look_and_install( "_WCHAR_T_DEFINED", DEF_NOARGS_PREDEF, null
                    , "1");
        }
    }
}

#endif

static void     def_macros( void)
/*
 * Define macros specified by -D option.
 * This routine should be called before undef_macros().
 */
{
    int         i;

    for (i = 0; i < def_cnt; i++)
        def_a_macro( 'D', def_list[ i]);
}

static void     undef_macros( void)
/*
 * Undefine macros specified by -U option.
 * This routine should be called after init_predefine().
 */
{
    char *      name;
    int         i;

    for (i = 0; i < undef_cnt; i++) {
        name = undef_list[ i];
        if (look_id( name) != NULL)
            undefine( name);
        else if (warn_level & 8)
            mcpp_fprintf( ERR, "\"%s\" wasn't defined\n", name);
    }
}

void    put_depend(
    const char *    filename
)
/*
 * Append a header name to the source file dependency line.
 */
{
#define MAX_OUT_LEN     76      /* Maximum length of output line    */
#define MKDEP_INITLEN   (MKDEP_INIT * 0x100)
#define MKDEP_MAX       (MKDEP_INIT * 0x10)
#define MKDEP_MAXLEN    (MKDEP_INITLEN * 0x10)

    static char *   output = NULL;          /* File names           */
    static size_t * pos = NULL;             /* Offset to filenames  */
    static int      pos_num;                /* Index of pos[]       */
    static char *   out_p;                  /* Pointer to output[]  */
    static size_t   mkdep_len;              /* Size of output[]     */
    static size_t   pos_max;                /* Size of pos[]        */
    static FILE *   fp;         /* Path to output dependency line   */
    static size_t   llen;       /* Length of current physical output line   */
    size_t *        pos_p;                  /* Index into pos[]     */
    size_t          fnamlen;                /* Length of filename   */

    if (fp == NULL) {   /* Main source file.  Have to initialize.   */
#if MCPP_LIB
        if (output != NULL) {
            free( output);
            free( pos);
        }
#endif
        output = xmalloc( mkdep_len = MKDEP_INITLEN);
        pos = (size_t *) xmalloc( (pos_max = MKDEP_INIT) * sizeof (size_t));
        out_p = md_init( filename, output);
        fp = mkdep_fp;
        llen = strlen( output);
        pos_num = 0;            /* Initialize for MCPP_LIB build    */
    } else if (filename == NULL) {              /* End of input     */
        out_p = stpcpy( out_p, "\n\n");
        if (mkdep & MD_PHONY) {
            /* Output the phony target line for each recorded header files. */
            char *  cp;
            int     c;

            if (strlen( output) * 2 + (pos_num * 2) >= MKDEP_MAXLEN) {
                cerror( "Too long dependency line"          /* _E_  */
                        , NULL, 0L, NULL);
                if (fp == fp_out)
                    mcpp_fputs( output, OUT);
                else
                    fputs( output, fp);
                return;
            } else if (strlen( output) * 2 + (pos_num * 2) >= mkdep_len) {
                /* Enlarge the buffer   */
                size_t  len = out_p - output;
                output = xrealloc( output, mkdep_len *= 2);
                out_p = output + len;
            }
            pos_num--;
            for (pos_p = &pos[ 0]; pos_p <= &pos[ pos_num]; pos_p++) {
                if (pos_p == &pos[ pos_num]) {      /* End of output    */
                    for (cp = output + *pos_p; *cp != '\n'; cp++)
                        ;
                    c = '\n';                       /* Append newline   */
                } else {
                    cp = output + *(pos_p + 1) - 1;
                    while( *cp == ' ' || *cp == '\\' || *cp == '\n')
                        cp--;               /* Remove trailing spaces   */
                    c = *(++cp);
                }
                *cp = EOS;
                out_p = stpcpy( out_p, output + *pos_p);
                out_p = stpcpy( out_p, ":\n\n");
                *cp = c;
            }
        }
        if (fp == fp_out) { /* To the same path with normal preprocessing   */
            mcpp_fputs( output, OUT);
        } else {        /* To the file specified by -MF, -MD, -MMD options  */
            fputs( output, fp);
            fclose( fp);
        }
        fp = NULL;      /* Clear for the next call in MCPP_LIB build        */
        return;
    }

    fnamlen = strlen( filename);
    /* Check the recorded filename  */
    for (pos_p = pos; pos_p < &pos[ pos_num]; pos_p++) {
        if (memcmp( output + *pos_p, filename, fnamlen) == 0)
            return;                 /* Already recorded filename    */
    }
    /* Any new header.  Append its name to output.  */
    if (llen + fnamlen > MAX_OUT_LEN) {         /* Line is long     */
        out_p = stpcpy( out_p, " \\\n ");       /* Fold it          */
        llen = 1;
    }
    llen += fnamlen + 1;
    if (pos_num >= MKDEP_MAX
            || out_p + fnamlen + 1 >= output + MKDEP_MAXLEN)
        cfatal( "Too long dependency line: %s", output, 0L, NULL);
    /* Need to enlarge the buffer   */
    if (pos_num >= pos_max) {
        pos = (size_t *) xrealloc( (char *) pos
                , (pos_max *= 2) * sizeof (size_t *));
    }
    if (output + mkdep_len <= out_p + fnamlen + 1) {
        size_t  len = out_p - output;
        output = xrealloc( output, mkdep_len *= 2);
        out_p = output + len;
    }
    *out_p++ = ' ';
    pos[ pos_num++] = out_p - output;       /* Remember the offset  */
            /* Don't use pointer, since 'output' may be reallocated later.  */
    out_p = stpcpy( out_p, filename);
}

static char *   md_init(
    const char *    filename,   /* The source file name             */
    char *  output              /* Output to dependency file        */
)
/*
 * Initialize output file and target.
 */
{
    char    prefix[ PATHMAX];
    char *  cp = NULL;
    size_t  len;
    char *  out_p;
    const char *    target = filename;
    const char *    cp0;

    if (! mkdep_target || ! mkdep_fp) {         /* Make target name */
#ifdef  PATH_DELIM
        if ((cp0 = strrchr( target, PATH_DELIM)) != NULL)
            target = cp0 + 1;
#endif
        if ((cp0 = strrchr( target, '.')) == NULL)
            len = strlen( target);
        else
            len = (size_t) (cp0 - target);
        memcpy( prefix, target, len);
        cp = prefix + len;
        *cp++ = '.';
    }

    if (! mkdep_fp) {   /* Unless already opened by -MF, -MD, -MMD options  */
        if (mkdep & MD_FILE) {
            strcpy( cp, "d");
            mkdep_fp = fopen( prefix, "w");
        } else {
            mkdep_fp = fp_out;  /* Output dependency line to normal output  */
            no_output++;                /* Without normal output    */
        }
    }

    if (mkdep_target) {         /* -MT or -MQ option is specified   */
        if (mkdep & MD_QUOTE) {         /* 'Quote' $, \t and space  */
            out_p = md_quote( output);
        } else {
            out_p = stpcpy( output, mkdep_target);
        }
    } else {
        strcpy( cp, OBJEXT);
        out_p = stpcpy( output, prefix);
    }

    *out_p++ = ':';
    *out_p = EOS;
    return  out_p;
}

static char *   md_quote(
    char *  output
)
/*
 * 'Quote' $, tab and space.
 * This function was written referring to GCC V.3.2 source.
 */
{
    char *  p;
    char *  q;

    for (p = mkdep_target; *p; p++, output++) {
        switch (*p) {
        case ' ':
        case '\t':
            /* GNU-make treats backslash-space sequence peculiarly  */
            for (q = p - 1; mkdep_target <= q && *q == '\\'; q--)
                *output++ = '\\';
            *output++ = '\\';
            break;
        case '$':
            *output++ = '$';
            break;
        default:
            break;
        }
        *output = *p;
    }
    *output = EOS;
    return  output;
}

static const char *     toolong_fname =
        "Too long header name \"%s%.0ld%s\"";               /* _F_  */
static const char *     excess_token =
        "Excessive token sequence \"%s\"";          /* _E_, _W1_    */

int     do_include(
    int     next        /* TRUE if the directive is #include_next   */
)
/*
 * Process the #include line.
 * There are three variations:
 *      #include "file"         search somewhere relative to the
 *                              current (or source) directory, if not
 *                              found, treat as #include <file>.
 *      #include <file>         Search in an implementation-dependent
 *                              list of places.
 *      #include macro-call     Expand the macro call, it must be one of
 *                              "file" or <file>, process as such.
 * On success : return TRUE;
 * On failure of syntax : return FALSE;
 * On failure of file opening : return FALSE.
 * do_include() always absorbs the line (including the <newline>).
 */
{
    const char * const  no_name = "No header name";         /* _E_  */
    char    header[ PATHMAX + 16];
    int     token_type;
    char *  fname;
    char *  filename;
    int     delim;                          /* " or <, >            */

    if ((delim = skip_ws()) == '\n') {      /* No argument          */
        cerror( no_name, NULL, 0L, NULL);
        return  FALSE;
    }
    fname = infile->bptr - 1;       /* Current token for diagnosis  */

    if (standard && (char_type[ delim] & LET)) {    /* Maybe macro  */
        int     c;
        char    *hp;

        hp = header;
        *hp = EOS;
        c = delim;
        while (get_unexpandable( c, FALSE) != NO_TOKEN) {
                                /* Expand any macros in the line    */
            if (header + PATHMAX < hp + (int) (workp - work_buf))
                cfatal( toolong_fname, header, 0L, work_buf);
            hp = stpcpy( hp, work_buf);
            while (char_type[ c = get_ch()] & HSP)
                *hp++ = c;
        }
        *hp = EOS;                          /* Ensure to terminate  */
        if (macro_line == MACRO_ERROR)      /* Unterminated macro   */
            return  FALSE;                  /*   already diagnosed. */
        unget_string( header, NULL);        /* To re-read           */
        delim = skip_ws();
        if (delim == '\n') {
            cerror( no_name, NULL, 0L, NULL);       /* Expanded to  */
            return  FALSE;                          /*   0 token.   */
        }
    }

    token_type = scan_token( delim, (workp = work_buf, &workp)
            , work_buf + PATHMAX);
    if (token_type == STR)                  /* String literal form  */
        goto  found_name;
    else if (token_type == OPE && openum == OP_LT)          /* '<'  */
        workp = scan_quote( delim, work_buf, work_buf + PATHMAX, TRUE);
                                        /* Re-construct or diagnose */
    else                                    /* Any other token in-  */
        goto  not_header;                   /*   cluding <=, <<, <% */

    if (workp == NULL)                      /* Missing closing '>'  */
        goto  syntax_error;

found_name:
    *--workp = EOS;                         /* Remove the closing and   */
    fname = save_string( &work_buf[ 1]);    /*  the starting delimiter. */

    if (skip_ws() != '\n') {
        if (standard) {
            cerror( excess_token, infile->bptr-1, 0L, NULL);
            skip_nl();
            goto  error;
        } else if (mcpp_mode == OLD_PREP) {
            skip_nl();
        } else {
            if (warn_level & 1)
                cwarn( excess_token, infile->bptr-1, 0L, NULL);
            skip_nl();
        }
    }

#if SYS_FAMILY == SYS_WIN
    bsl2sl( fname);
#endif
    filename = fname;
#if NO_DIR                              /* Unofficial feature           */
    if (no_dir) {                       /* Strip directory components   */
        char    src_dir[ PATHMAX] = { EOS, };
        if (has_directory( fname, src_dir))
            filename = fname + strlen( src_dir);
        delim = '"';    /* Even a system header is handled as a local one   */
    }
#endif
    if (open_include( filename, (delim == '"'), next)) {
        /* 'fname' should not be free()ed, it is used as file->         */
        /*      real_fname and has been registered into fnamelist[]     */
        return  TRUE;
    }

    cerror( "Can't open include file \"%s\"", filename, 0L, NULL);  /* _E_  */
error:
    free( fname);
    return  FALSE;

not_header:
    cerror( "Not a header name \"%s\"", fname, 0L, NULL);   /* _E_  */
syntax_error:
    skip_nl();
    return  FALSE;
}

static int  open_include(
    char *  filename,               /* File name to include         */
    int     searchlocal,            /* TRUE if #include "file"      */
    int     next                    /* TRUE if #include_next        */
)
/*
 * Open an include file.  This routine is only called from do_include() above.
 * It searches the list of directories via search_dir() and opens the file
 * via open_file(), linking it into the list of active files.
 * Returns TRUE if the file was opened, FALSE if it fails.
 */
{
    char    src_dir[ PATHMAX] = { EOS, };   /* Directory part of includer   */
    int     full_path;              /* Filename is full-path-list   */
    int     has_dir = FALSE;        /* Includer has directory part  */
    int     has_dir_src = FALSE;
    int     has_dir_fname = FALSE;

    full_path = is_full_path( filename);

    if (!full_path && searchlocal && (search_rule & SOURCE)) {
        has_dir_src  = has_directory( infile->src_dir, src_dir);
        has_dir_fname = has_directory( infile->real_fname
                , src_dir + strlen( src_dir));
        /* Get directory part of the parent file of the file to include.*/
        /* Note that infile->dirp of main input file is set to "" and   */
        /* remains the same even if -include options are processed.     */
        has_dir = has_dir_src || has_dir_fname
                || (**(infile->dirp) != EOS);
    }
    if (mcpp_debug & PATH)
        mcpp_fprintf( DBG, "filename: %s\n", filename);

#if COMPILER == GNUC
    if (! full_path) {
        if (i_split                     /* -I- option is specified  */
                || next)                        /* or #include_next */
        goto  search_dirs;
    }
#endif

    if ((searchlocal && ((search_rule & CURRENT) || !has_dir)) || full_path) {
        /*
         * Look in local directory first.
         * Try to open filename relative to the "current directory".
         */
        if (open_file( &null, NULL, filename, searchlocal && !full_path
                , FALSE, FALSE))
            return  TRUE;
        if (full_path)
            return  FALSE;
    }

    if (searchlocal && (search_rule & SOURCE) && has_dir) {
        /*
         * Look in local directory of source file.
         * Try to open filename relative to the "source directory".
         */
        if (open_file( infile->dirp, src_dir, filename, TRUE, FALSE, FALSE))
            return  TRUE;
    }

#if COMPILER == MSC
    if (searchlocal) {
        /* Visual C searches ancestor source's directory, too.  */
        FILEINFO *  file = infile;
        while ((file = file->parent) != NULL) {
            /* Search each parent includer's directory  */
            if (open_file( file->dirp, src_dir, filename, TRUE, FALSE, FALSE))
                return  TRUE;
        }
    }
#endif
#if COMPILER == GNUC
search_dirs:
    if (searchlocal) {
        /* Search the directories specified by -iquote option, if any.  */
        const char **   qdir;
        for (qdir = quote_dir; qdir < quote_dir_end; qdir++) {
            if (open_file( qdir, NULL, filename, FALSE, FALSE, FALSE))
                return  TRUE;
        }
    }
#endif
    /* Search the include directories   */
    if (search_dir( filename, searchlocal, next))
        return  TRUE;

    return  FALSE;
}

static int  has_directory(
    const char *    source,         /* Filename to examine          */
    char *  directory               /* Put directory stuff here     */
)
/*
 * If a directory is found in the 'source' filename string (i.e. "includer"),
 * the directory part of the string is copied to 'directory' and
 * has_directory() returns TRUE.
 * Else, nothing is copied and it returns FALSE.
 */
{
    const char *    sp;
    size_t  len;

    if (! source)
        return  FALSE;
    if ((sp = strrchr( source, PATH_DELIM)) == NULL) {
        return  FALSE;
    } else {
        len = (size_t)(sp - source) + 1;    /* With path-delimiter  */
        memcpy( directory, source, len);
        directory[ len] = EOS;
        return  TRUE;
    }
}

static int  is_full_path(
    const char *    path
)
/*
 * Check whether the path is a full (absolute) path list or not.
 */
{
    if (! path)
        return  FALSE;
#if SYS_FAMILY == SYS_UNIX
    if (path[0] == PATH_DELIM)
#elif   SYS_FAMILY == SYS_WIN
    if ((path[1] == ':' && (path[2] == PATH_DELIM || path[2] == '\\'))   /* "C:/path"    */
            || path[0] == PATH_DELIM)       /* Root dir of current drive    */
#elif   1
/* For other systems you should write code here.    */
    if (path[0] == PATH_DELIM)
#endif
        return  TRUE;
    else
        return  FALSE;
}

static int  search_dir(
    char *  filename,               /* File name to include         */
    int     searchlocal,            /* #include "header.h"          */
    int     next                    /* TRUE if #include_next        */
)
/*
 * Look in any directories specified by -I command line arguments,
 * specified by environment variable, then in the builtin search list.
 */
{
    const char **   incptr;                 /* -> inlcude directory */

    incptr = incdir;
#if COMPILER == GNUC
    if (next && **inc_dirp != EOS)
        incptr = inc_dirp + 1;
        /* In case of include_next search after the includer's directory    */
#endif

    for ( ; incptr < incend; incptr++) {
        if (strlen( *incptr) + strlen( filename) >= PATHMAX)
            cfatal( toolong_fname, *incptr, 0L, filename);  /* _F_  */
#if SYSTEM == SYS_MAC
        if (incptr == to_search_framework && ! searchlocal) {
                                /* Now search the framework dirs    */
            if (search_framework( filename)) {          /* Found    */
                if (in_import)  /* "#import"ed file is once only    */
                    do_once( infile->full_fname);
                return  TRUE;
            }
            /* Else continue to search incptr   */
        }
#endif
        if (open_file( incptr, NULL, filename, FALSE, FALSE, FALSE))
            /* Now infile has been renewed  */
            return  TRUE;
    }

    return  FALSE;
}

static int  open_file(
    const char **   dirp,           /* Pointer to include directory */
    const char *    src_dir,        /* Source directory of includer */
    const char *    filename,       /* Filename (possibly has directory)    */
    int         local,                      /* #include "file"      */
    int         include_opt,        /* Specified by -include option */
    int         sys_frame           /* System framework header (for SYS_MAC)*/
)
/*
 * Open a file, add it to the linked list of open files, close the includer
 * if nessesary and truncate the includer's buffer.
 * This is called from open_include() and at_start().
 */
{
    char        dir_fname[ PATHMAX] = { EOS, };
#if HOST_COMPILER == BORLANDC
    /* Borland's fopen() does not set errno.    */
    static int  max_open = FOPEN_MAX - 5;
#else
    static int  max_open;
#endif
    int         len;
    FILEINFO *  file = infile;
    FILE *      fp;
    char *      fullname;
    const char *    fname;

    errno = 0;      /* Clear errno possibly set by path searching   */
#if SYSTEM == SYS_MAC && COMPILER == GNUC
    if (strlen( *dirp) > 5
            && str_case_eq( *dirp + strlen( *dirp) - 5, ".hmap")) {
        /* Search header map file for a header  */
        if (! search_header_map( *dirp, filename, dir_fname))
            return  NULL;
        fname = dir_fname;                  /* Found a path-list    */
        dirp = &null;
        goto  search;
    } else
#endif
    {
        if (mcpp_debug & PATH)
            mcpp_fprintf( DBG, "Searching %s%s%s\n", *dirp
                    , src_dir ? src_dir : null, filename);
    }
    /* src_dir is usually NULL.  This is specified to   */
    /* search the source directory of the includer.     */
    if (src_dir && *src_dir != EOS) {
        strcpy( dir_fname, src_dir);
        strcat( dir_fname, filename);
        fname = dir_fname;
    } else {
        fname = filename;
    }
search:
    fullname = norm_path( *dirp, fname, TRUE, FALSE);
                                    /* Convert to absolute path     */
    if (! fullname)                 /* Non-existent or directory    */
        return  FALSE;
    if (standard && included( fullname))        /* Once included    */
        goto  true;

    if ((max_open != 0 && max_open <= include_nest)
                            /* Exceed the known limit of open files */
            || ((fp = fopen( fullname, "r")) == NULL && errno == EMFILE)) {
                            /* Reached the limit for the first time */
        if (mcpp_debug & PATH) {
#if HOST_COMPILER == BORLANDC
            if (include_nest == FOPEN_MAX - 5)
#else
            if (max_open == 0)
#endif
                mcpp_fprintf( DBG,
    "#include nest reached at the maximum of system: %d, returned errno: %d\n"
                    , include_nest, errno);
        }
        /*
         * Table of open files is full.
         * Remember the file position and close the includer.
         * The state will be restored by get_line() on end of the included.
         */
        file->pos = ftell( file->fp);
        fclose( file->fp);
        /* In case of failure, re-open the includer */
        if ((fp = fopen( fullname, "r")) == NULL) {
            file->fp = fopen( cur_fullname, "r");
            fseek( file->fp, file->pos, SEEK_SET);
            goto  false;
        }
        if (max_open == 0)      /* Remember the limit of the system */
            max_open = include_nest;
    } else if (fp == NULL)                  /* No read permission   */
        goto  false;
    /* Truncate buffer of the includer to save memory   */
    len = (int) (file->bptr - file->buffer);
    if (len) {
        file->buffer = xrealloc( file->buffer, len + 1);
        file->bptr = file->buffer + len;
    }

    if (! include_opt)
        sharp( NULL, 0);    /* Print includer's line num and fname  */
    add_file( fp, src_dir, filename, fullname, include_opt);
    /* Add file-info to the linked list.  'infile' has been just renewed    */
    /*
     * Remember the directory for #include_next.
     * Note: inc_dirp is restored to the parent includer's directory
     *   by get_ch() when the current includer is finished.
     */
    if (g_report_include)
        g_report_include(fp, src_dir, filename, fullname);
    infile->dirp = inc_dirp = dirp;
#if 0   /* This part is only for debugging  */
    chk_dirp( dirp);
#endif
#if COMPILER == GNUC
    if ((**dirp != EOS && sys_dirp <= dirp && dirp <= incend)
#if SYSTEM == SYS_MAC
            || sys_frame
#endif
            )
        infile->sys_header = TRUE;      /* Found in a system header dir     */
    else
        infile->sys_header = FALSE;
#endif
    cur_fullname = fullname;

    if (option_flags.z) {
        no_output++;        /* Don't output the included file       */
        if (include_nest == 2)
            mcpp_fprintf( OUT, "#include \"%s\"\n", fullname);
            /* Output #include line instead, if it is in main source file   */
    } else if (! include_opt) {     /* Do not sharp() on -include   */
        src_line = 1;                   /* Working on line 1 now    */
        sharp( NULL, 1);    /* Print out the included file name     */
    }
    src_line = 0;                       /* To read the first line   */

    if (mkdep && ((mkdep & MD_SYSHEADER) || ! infile->sys_header))
        put_depend( fullname);          /* Output dependency line   */

true:
    return  TRUE;
false:
    free( fullname);
    return  FALSE;
}

void    add_file(
    FILE *      fp,                         /* Open file pointer    */
    const char *    src_dir,                /* Directory of source  */
    const char *    filename,               /* Name of the file     */
    const char *    fullname,               /* Full path list       */
    int         include_opt         /* File specified by -include option    */
)
/*
 * Initialize tables for this open file.  This is called from open_file()
 * (for #include files), and from the entry to MCPP to open the main input
 * file.  It calls a common routine get_file() to build the FILEINFO
 * structure which is used to read characters.
 */
{
    FILEINFO *      file;
    const char *    too_many_include_nest =
            "More than %.0s%ld nesting of #include";    /* _F_ _W4_ */

    filename = set_fname( filename);    /* Search or append to fnamelist[]  */
    fullname = set_fname( fullname);    /* Search or append to fnamelist[]  */
    file = get_file( filename, src_dir, fullname, (size_t) NBUFF, include_opt);
                                        /* file == infile           */
    file->fp = fp;                      /* Better remember FILE *   */
    cur_fname = filename;

    if (include_nest >= INCLUDE_NEST)   /* Probably recursive #include      */
        cfatal( too_many_include_nest, NULL, (long) INCLUDE_NEST, NULL);
    if (standard && (warn_level & 4)
            && include_nest == std_limits.inc_nest + 1)
        cwarn( too_many_include_nest, NULL, (long) std_limits.inc_nest, NULL);
    include_nest++;
}

static const char *     set_fname(
    const char *    filename
)
/*
 * Register the source filename to fnamelist[].
 * Search fnamelist[] for filename or append filename to fnamelist[].
 * Returns the pointer.
 * file->real_fname and file->full_fname points into fnamelist[].
 */
{
    INC_LIST *  fnamep;
    size_t      fnamelen;

    if (fnamelist == NULL) {            /* Should be initialized    */
        max_fnamelist = INIT_NUM_FNAMELIST;
        fnamelist = (INC_LIST *) xmalloc( sizeof (INC_LIST) * max_fnamelist);
        fname_end = &fnamelist[ 0];
    } else if (fname_end - fnamelist >= max_fnamelist) {
                                /* Buffer full: double the elements */
        fnamelist = (INC_LIST *) xrealloc( (void *) fnamelist
                , sizeof (INC_LIST) * max_fnamelist * 2);
        fname_end = &fnamelist[ max_fnamelist];
        max_fnamelist *= 2;
    }

    /* Register the filename in fnamelist[] */
    fnamelen = strlen( filename);
    for (fnamep = fnamelist; fnamep < fname_end; fnamep++) {
        if (fnamep->len == fnamelen && str_case_eq( fnamep->name, filename))
            return  filename;           /* Already registered       */
    }
    fname_end->name = xmalloc( fnamelen + 1);
    filename = strcpy( fname_end->name, filename);
                                /* Global pointer for get_file()    */
    fname_end->len = fnamelen;
    fname_end++;

    return  filename;
}

#if SYSTEM == SYS_MAC
#if COMPILER == GNUC

/* Routines to search "header map" file of Xcode / Apple-GCC.       */
/* search_header_map() and hmap_hash() were written referring to    */
/* c-incpath.c of Apple-GCC-4.0.1.                                  */

static char *   search_header_map(
    const char *    hmap_file,              /* Header map file      */
    const char *    filename,               /* Filename to search   */
    char *          pathlist        /* Buffer for a found pathlist  */
)
/*
 * Search a table in "header map" file for a header.
 */
{
    struct stat     stat_buf;
    FILE *          fp;
    size_t          fsize;
    const char *    contents;
    struct hmap_header_map *    hmap;
    struct hmap_bucket *        buckets;
    const char *    strings;
    uint32          mask;
    uint32          key_offs;
    uint32          i;

    stat( hmap_file, &stat_buf);            /* Get size of the file */
    fsize = stat_buf.st_size;
    contents = xmalloc( fsize + 1);
    fp = fopen( hmap_file, "r");
    fread( contents, fsize, 1, fp);     /* Read whole of the file at once   */
    hmap = (struct hmap_header_map *) contents;

    strings = ((const char *) hmap) + hmap->strings_offset;
    buckets = hmap->buckets;
    mask = hmap->capacity - 1;
    i = hmap_hash( filename) & mask;
    while ((key_offs = buckets[ i].key) != 0) {
        if (str_case_eq( filename, strings + key_offs)) {
            /* The names match.  Make path-list.    */
            char *  cp = stpcpy( pathlist, strings + buckets[ i].value.prefix);
            strcpy( cp, strings + buckets[ i].value.suffix);
            break;
        }
        i = ++i & mask;
    }
    free( contents);
    return  key_offs ? pathlist : NULL;
}

static unsigned hmap_hash(
    const char *    fname                   /* header name          */
)
/*
 * Get hash value for the fname.
 */
{
    const char *    sp;
    unsigned        hash_code = 0;

    for (sp = fname; *sp; sp++)
        hash_code += tolower( *sp & 0xFF) * 13;
    return  hash_code;
}
#endif  /* COMPILER == GNUC */

static void     init_framework( void)
/*
 * Initialize framework[].
 */
{
    char *  framework_dir;
    /* Some frameworks may have been already specified by -F option.    */
    sys_framework = num_framework;      /* These are system frameworks  */
#ifdef  FRAMEWORK1
    framework_dir = norm_dir( FRAMEWORK1, TRUE);
    if (framework_dir)
        framework[ num_framework++] = framework_dir;
#endif
#ifdef  FRAMEWORK2
    framework_dir = norm_dir( FRAMEWORK2, TRUE);
    if (framework_dir)
        framework[ num_framework++] = framework_dir;
#endif
#ifdef  FRAMEWORK3
    framework_dir = norm_dir( FRAMEWORK3, TRUE);
    if (framework_dir)
        framework[ num_framework++] = framework_dir;
#endif
    if (num_framework >= MAX_FRAMEWORK) {
        mcpp_fputs( "Too many Framework directories.", ERR);
        longjmp( error_exit, -1);
    }
}

static const char *     dot_frame = ".framework";

static int      search_framework(
    char *  filename
)
/*
 * Search "Framework" directories.
 * 'frame/header.h' is converted to
 * '/System/Library/Frameworks/frame.framework/Headers/header.h',
 * '/System/Library/Frameworks/frame.framework/PrivateHeaders/header.h',
 * and so on.
 */
{
    char        fullname[ PATHMAX + 1];
    FILEINFO *  file;
    char *      frame, * fname, * cp1, * cp2;
    int         sys_frame = FALSE;
    int         i;

    cp1 = cp2 = strchr( filename, PATH_DELIM);
    /*
     * 'filename' should be <frame/header> format or sometimes
     *      <frame/dir/header>.
     * e.g.: <Foundation/Foundation.h>, <CarbonCore/OSUtils.h>
     *      or <IOKit/pwr_mgt/IOPMLib.h>.
     */
    if (! cp1)
        return  FALSE;
    *cp1 = EOS;
    frame = filename;
    fname = cp1 + 1;

    /* Search framework[] directories   */
    for (i = 0; i < num_framework; i++) {
        cp1 = stpcpy( fullname, framework[ i]);
                    /* 'fullname' e.g.: /System/Library/Frameworks/ */
        if (search_subdir( fullname, cp1, frame, fname, sys_framework <= i))
            return  TRUE;
    }

    /*
     * Search subframework dirs searching its possible parent framework
     * starting from current file's directory to its ancestors.
     * Header file in subframework directories should be included only
     * by its parent or sibling framework headers.
     */
    for (i = sys_framework; i < num_framework; i++) {
        size_t  frame_len, fname_len;
        frame_len = strlen( framework[ i]);
        fname_len = strlen( infile->real_fname);
        if (fname_len <= frame_len)
            continue;
        if (memcmp( framework[ i], infile->real_fname, frame_len) == 0) {
            sys_frame = TRUE;
            break;
        }
    }
    for (file = infile; file; file = file->parent) {
        const char *    dot;
        size_t  len;

        if (! file->fp)
            continue;
        dot = strstr( file->real_fname, dot_frame);
        if (! dot)
            continue;
        len = dot - file->real_fname + strlen( dot_frame) + 1;
        memcpy( fullname, file->real_fname, len);
        cp1 = fullname + len;
        cp1 = stpcpy( cp1, "Frameworks/");
        /* 'fullname' e.g.:                                             */
        /* /System/Library/Frameworks/Foundation.framework/Frameworks/  */
        if (search_subdir( fullname, cp1, frame, fname, sys_frame))
            return  TRUE;
    }

    *cp2 = PATH_DELIM;      /* Restore original include file format */

    return  FALSE;
}

static int      search_subdir(
    char *  fullname,               /* Buffer for path-list to open */
    char *  cp,                     /* Latter half of 'fullname'    */
    char *  frame,                  /* 'frame' of <frame/header>    */
    char *  fname,                  /* 'header' of <frame/header>   */
                /* or sometimes 'dir/header' of <frame/dir/header>  */
    int     sys_frame               /* System framework header ?    */
)
/*
 * Make path-list and try to open.
 */
{
    static const char *     subdir[] = { "Headers", "PrivateHeaders", NULL};
    int     j, n;

    cp += sprintf( cp, "%s%s%c", frame, dot_frame, PATH_DELIM);
    for (j = 0; subdir[ j] != NULL; j++) {
        n = sprintf( cp, "%s%c%s", subdir[ j], PATH_DELIM, fname);
        /*
         * 'fullname' is for example:
         * /System/Library/Frameworks/Foundation.framework/Headers/
         *      Foundation.h,
         * /System/Library/Frameworks/Foundation.framework/Frameworks/
         *      CarbonCore.framework/Headers/OSUtils.h,
         * or /System/Library/Frameworks/IOKit.framework/Headers/
         *      pwr_mgt/IOPMLib.h.
         * Pass this as one filename argument to open_file() rather than
         * deviding to directory part and file part.  The first argument to
         * open_file() which is a pointer to the directory part is remembered
         * by FILEINFO struct.  But, 'fullname' is over-written each time,
         * and the former path-list is lost soon.  Therefore, it cannot be
         * passed as the first argument.  In addition, though the first
         * argument to open_file() is needed for #include_next, this directive
         * has no meaning in framework.
         */
        if ((cp - fullname) + n > PATHMAX)
            cfatal( "Too long framework path", NULL, 0L, NULL); /* _F_  */
        if (open_file( &null, NULL, fullname, FALSE, FALSE, sys_frame))
            return  TRUE;
    }
    return  FALSE;
}

#endif  /* SYSTEM == SYS_MAC    */

#if 0   /* This part is only for debugging  */
static int  chk_dirp(
    const char **   dirp
)
/*
 * Check the validity of include directory specified for open_file().
 * Called only from open_file().
 */
{
    const char **   ip;

    if (dirp == &null)
        return  TRUE;

    for (ip = incdir; ip < incend; ip++)
        if (dirp == ip)
            break;
    if (ip == incend) {
#if COMPILER == MSC
        FILEINFO *  pfile = infile->parent;
        if (pfile) {
            while ((pfile = pfile->parent) != NULL) {
                /* Search each parent includer's directory  */
                if (dirp == pfile->dirp)
                    break;
            }
        }
        if (! pfile)
#endif
#if COMPILER == GNUC
        const char **   qdir;
        for (qdir = quote_dir; qdir < quote_dir_end; qdir++) {
            if (dirp == qdir)
                break;
        }
        if (qdir == quote_dir_end)
#endif
        {
            cfatal( "Bug: *dirp:%s is invalid", *dirp, 0L, NULL);
            return  FALSE;
        }
    }
    return  TRUE;
}
#endif

char* mincs_ifnotnull(char* p1, char* p2)
{
    if (!p1) return p2;
    if (!p2) return p1;
    return p1 < p2 ? p1 : p2;
}

/* extract directory part of a file path */
static char g_space[1024];
char * dir_of(const char* path)
{
    const char * lastsep = mincs_ifnotnull(strrchr(path, PATH_DELIM),
                                           strrchr(path, '\\'));
    if (!lastsep) return NULL;
    if (lastsep - path >= 1024)
        cfatal("path too long encountered. %s", path, 0L, NULL);

    strncpy(g_space, path, lastsep - path + 1);
    g_space[lastsep - path + 1] = 0;
    return g_space;
}

void    sharp(
    FILEINFO *  sharp_file,
    int         flag        /* Flag to append to the line for GCC   */
)
/*
 * Output a line number line.
 * 'file' is 'sharp_file' if specified,
 * else (i.e. 'sharp_file' is NULL) 'infile'.
 */
{
    static FILEINFO *   sh_file;
    static int  sh_line;
    FILEINFO *  file;
    int         line;

    file = sharp_file ? sharp_file : infile;
    if (! file)
        return;
    while (! file->fp)
        file = file->parent;
    line = sharp_file ? sharp_file->line : src_line;
    if (no_output || option_flags.p || file == NULL
            || (file == sh_file && line == sh_line))
        goto  sharp_exit;
    sh_file = file;
    sh_line = line;
    if (keep_comments)
        mcpp_fputc( '\n', OUT);         /* Ensure to be on line top */
    if (std_line_prefix)
        mcpp_fprintf( OUT, "#line %ld", line);
    else
        mcpp_fprintf( OUT, "%s%ld", LINE_PREFIX, line);
    cur_file( file, sharp_file, flag);
    mcpp_fputc( '\n', OUT);
sharp_exit:
    wrong_line = FALSE;
}

static void cur_file(
    FILEINFO *  file,                   /* infile or sharp_file     */
    FILEINFO *  sharp_file,             /* The 'file' or NULL       */
    int         flag                    /* Flag to append for GCC   */
)
/*
 * Output current source file name and line number.
 * Called only from sharp() above.
 */
{
    const char *    name;

    if (mcpp_debug & MACRO_CALL) {  /* In macro notification mode   */
        if (sharp_file)                         /* Main input file  */
            name = file->full_fname;
        else                /* Output full-path-list, normalized    */
            name = cur_fullname;
    } else {                /* Usually, the path not "normalized"   */
        if (sharp_file) {                       /* Main input file  */
            name = file->filename;
        } else if (str_eq( file->filename, file->real_fname)) {
            name = file->full_fname; // O3DE: absolute paths required
        } else {            /* Changed by '#line fname' directive   */
            if (!is_full_path(file->filename)) {     // O3DE: this block does the best it can to get real full paths.
                name = dir_of(cur_fullname);  // extract first part of path
                if (!name)  // no directory found in cur_fullname (weird case)
                    name = file->filename;
                else {
                    sprintf( work_buf, "%s%s", name, file->filename);
                    FILE* reachable = fopen(work_buf, "r");
                    if (reachable) {
                        name = norm_path(null, work_buf, FALSE, FALSE);
                        fclose(reachable);
                    } else {  // non reachable file encountered
                        name = file->filename;  // input verbatim
                    }
                }
                if (strlen(name) == 0)
                    name = file->filename;  // as is
            } else {  // absolute path already
                name = file->filename;
            }
        }
    }
    if (sharp_filename == NULL || ! str_eq( name, sharp_filename)) {
        if (sharp_filename != NULL)
            free( sharp_filename);
        sharp_filename = save_string( name);
    }
    mcpp_fprintf( OUT, " \"%s\"", name);
#if COMPILER == GNUC
    if (! std_line_prefix) {
        if (flag) {
            mcpp_fputc( ' ', OUT);
            mcpp_fputc( '0' + flag, OUT);
        }
        if (file->sys_header)
            mcpp_fputs( " 3", OUT);
    }
#endif
}

#if SYS_FAMILY == SYS_WIN

static char *   bsl2sl(
    char * filename
)
/*
 * Convert '\\' in the path-list to '/'.
 */
{
    static int  diagnosed = FALSE;
    char *  cp;

    cp = filename;

    while (*cp) {
        if (bsl_in_mbchar) {
            int     c;
            c = *cp & UCHARMAX;
            if (char_type[ c] & mbchk) {    /* First byte of MBCHAR */
                char    tmp[ PATHMAX];
                char *  tp = tmp;
                *tp++ = *cp++;
                mb_read( c, &cp, &tp);
                            /* Read over the multi-byte characters  */
                continue;
            }
        }
        if (*cp == '\\') {
            *cp++ = PATH_DELIM;
            if (!diagnosed && (warn_level & 2) && (warn_level != -1)) {
                            /* Backslash in source program          */
                cwarn( "Converted \\ to %s", "/", 0L, NULL);        /* _W2_ */
                    diagnosed = TRUE;       /* Diagnose only once   */
            }
        } else {
            cp++;
        }
    }

    return  filename;
}

#endif  /* SYS_FAMILY == SYS_WIN    */

static const char * const   unknown_arg =
        "Unknown argument \"%s\"";      /*_W1_*/
static const char * const   not_ident =
        "Not an identifier \"%s\"";     /*_W1_*/

static int  is_junk( void)
/*
 * Check the trailing junk in a directive line.
 * This routine is never called in OLD_PREP mode.
 */
{
    int     c;

    c = skip_ws();
    unget_ch();
    if (c != '\n') {                        /* Trailing junk        */
        if (warn_level & 1)
            cwarn( unknown_arg, infile->bptr, 0L, NULL);
        return TRUE;
    } else {
        return FALSE;
    }
}

#define PUSH    1
#define POP    -1

#define __SETLOCALE     1       /* #pragma __setlocale( "encoding") */
#define SETLOCALE       2       /* #pragma setlocale( "encoding")   */

void    do_pragma( void)
/*
 * Process the #pragma lines.
 * 1. Process the sub-directive for MCPP.
 * 2. Pass the line to the compiler-proper.
 *      #pragma MCPP put_defines, #pragma MCPP preprocess,
 *      #pragma MCPP preprocessed and #pragma once are, however, not put
 *      out so as not to duplicate output when re-preprocessed.
 * When EXPAND_PRAGMA == TRUE and (__STDC_VERSION__ >= 199901L or
 * __cplusplus >= 199901L), the line is subject to macro expansion unless
 * the next to 'pragma' token is one of 'STDC', 'GCC' or 'MCPP'.
 */
{
    int         c;
    int         warn = FALSE;               /* Necessity of warning */
    int         token_type;
    char *      bp;                         /* Pointer to argument  */
    char *      tp;
    FILEINFO *  file;

    wrong_line = TRUE;                      /* In case of error     */
    c = skip_ws();
    bp = infile->bptr - 1;  /* Remember token to pass to compiler   */
    if (c == '\n') {
        if (warn_level & 1)
            cwarn( "No sub-directive", NULL, 0L, NULL);     /* _W1_ */
        unget_ch();
        return;
    }
    token_type = scan_token( c, (tp = work_buf, &tp), work_end);
#if EXPAND_PRAGMA
#if COMPILER == MSC
    if (token_type == NAM
            && !str_eq( identifier, "STDC") && !str_eq( identifier, "MCPP")) {
#else
    if (stdc3 && token_type == NAM
            && !str_eq( identifier, "STDC") && !str_eq( identifier, "MCPP")) {
#endif
        DEFBUF *        defp;
        char *          mp;
        char *          mp_end;
        LINE_COL        line_col = { 0L, 0};

        bp = mp = xmalloc( (size_t)(NMACWORK + IDMAX));
                                    /* Buffer for macro expansion   */
        mp_end = mp + NMACWORK;
        tp = stpcpy( mp, identifier);
        do {                /* Expand all the macros in the line    */
            int     has_pragma;
            if (token_type == NAM && (defp = is_macro( &tp)) != NULL) {
                tp = expand_macro( defp, bp, mp_end, line_col, & has_pragma);
                if (has_pragma)
                cerror( "_Pragma operator found in #pragma line"    /* _E_  */
                            , NULL, 0L, NULL);
                if (! stdc3 && (warn_level & 2))
                    cwarn(
                "\"%s\" is macro expanded in other than C99 mode"   /* _W2_ */
                            , identifier, 0L, NULL);
            }
            token_type = scan_token( c = get_ch(), (bp = tp, &tp), mp_end);
        } while (c != '\n');
        unget_string( mp, NULL);                    /* To re-read   */
        free( mp);
        c = skip_ws();
        bp = infile->bptr - 1;
        token_type = scan_token( c, (tp = work_buf, &tp), work_end);
    }
#endif
    if (token_type != NAM) {
        if (warn_level & 1)
            cwarn( not_ident, work_buf, 0L, NULL);
        goto  skip_nl;
    } else if (str_eq( identifier, "once")) {   /* #pragma once     */
       if (! is_junk()) {
            file = infile;
            while (file->fp == NULL)
                file = file->parent;
            do_once( file->full_fname);
            goto  skip_nl;
        }
    } else if (str_eq( identifier, "MCPP")) {
        if (scan_token( skip_ws(), (tp = work_buf, &tp), work_end) != NAM) {
            if (warn_level & 1)
                cwarn( not_ident, work_buf, 0L, NULL);
        }
        if (str_eq( identifier, "put_defines")) {
            if (! is_junk())
                dump_def( TRUE, FALSE); /* #pragma MCPP put_defines */
        } else if (str_eq( identifier, "preprocess")) {
            if (! is_junk())            /* #pragma MCPP preprocess  */
                mcpp_fputs( "#pragma MCPP preprocessed\n", OUT);
                    /* Just putout the directive    */
        } else if (str_eq( identifier, "preprocessed")) {
            if (! is_junk()) {          /* #pragma MCPP preprocessed*/
                skip_nl();
                do_preprocessed();
                return;
            }
        } else if (str_eq( identifier, "warning")) {
                                        /* #pragma MCPP warning     */
            cwarn( infile->buffer, NULL, 0L, NULL);
        } else if (str_eq( identifier, "push_macro")) {
            push_or_pop( PUSH);         /* #pragma MCPP push_macro  */
        } else if (str_eq( identifier, "pop_macro")) {
            push_or_pop( POP);          /* #pragma MCPP pop_macro   */
        } else if (str_eq( identifier, "debug")) {
            do_debug( TRUE);            /* #pragma MCPP debug       */
        } else if (str_eq( identifier, "end_debug")) {
            do_debug( FALSE);           /* #pragma MCPP end_debug   */
        } else {
            warn = TRUE;
        }
        if (warn && (warn_level & 1))
            cwarn( unknown_arg, identifier, 0L, NULL);
        goto  skip_nl;                  /* Do not putout the line   */
#if COMPILER == GNUC
    /* The #pragma lines for GCC is skipped not to confuse cc1.     */
    } else if (str_eq( identifier, "GCC")) {    /* #pragma GCC *    */
        if (scan_token( skip_ws(), (tp = work_buf, &tp), work_end) == NAM) {
            if (str_eq( identifier, "poison")
                    || str_eq( identifier, "dependency")) {
                if (warn_level & 2)
                    cwarn( "Skipped the #pragma line"       /*_W2_  */
                            , NULL, 0L, NULL);
                goto skip_nl;
            } else if (str_eq( identifier, "system_header")) {
                infile->sys_header = TRUE;      /* Mark as a system header  */
                goto skip_nl;
            }
        }
#endif

#if COMPILER == MSC
    } else if (str_eq( identifier, "setlocale")) {
        if (skip_ws() == '('
                && scan_token( skip_ws(), (tp = work_buf, &tp), work_end)
                    == STR
                && skip_ws() == ')') {
            if (! is_junk()) {
                work_buf[ 0] = *(tp - 1) = '\0';
                set_encoding( work_buf + 1, NULL, SETLOCALE);
                work_buf[ 0] = *(tp - 1) = '"';
            }   /* else warned by is_junk() */
        } else {
            warn = TRUE;
        }
#else   /* COMPILER != MSC  */
    } else if (str_eq( identifier, "__setlocale")) {
        if (skip_ws() == '('
                && scan_token( skip_ws(), (tp = work_buf, &tp), work_end)
                        == STR
                && skip_ws() == ')') {
            if (! is_junk()) {              /* #pragma __setlocale  */
                work_buf[ 0] = *(tp - 1) = '\0';
                set_encoding( work_buf + 1, NULL, __SETLOCALE);
                work_buf[ 0] = *(tp - 1) = '"';
            }   /* else warned by is_junk() */
        } else {
            warn = TRUE;
        }
#endif

#if COMPILER == MSC
    } else if (str_eq( identifier, "push_macro")) {
        push_or_pop( PUSH);
        goto  skip_nl;
    } else if (str_eq( identifier, "pop_macro")) {
        push_or_pop( POP);
        goto  skip_nl;
#endif

#if COMPILER == LCC
    } else if (str_eq( identifier, "optimize")
                && (skip_ws() == '(')
                && (char_type[ (c = skip_ws()) & UCHARMAX] == DIG)
                && (skip_ws() == ')')) {
        char    tmp[ 2];

        tmp[ 0] = c;
        tmp[ 1] = EOS;
        look_and_install( optim_name, DEF_NOARGS_PREDEF, null, tmp);
#endif

#if COMPILER == COMPILER_UNKNOWN
    /*
     * Write here any compiler-specific #pragma sub-directive which should
     * be processed by preprocessor.
     */
#endif
    }

    if (warn) {
        if (warn_level & 1)
            cwarn( unknown_arg, identifier, 0L, NULL);
        goto  skip_nl;                  /* Do not putout the line   */
    }

    sharp( NULL, 0);    /* Synchronize line number before output    */
    if (! no_output) {
        mcpp_fputs( "#pragma ", OUT);
        mcpp_fputs( bp, OUT);           /* Line is put out          */
    }
skip_nl: /* Don't use skip_nl() which skips to the newline in source file */
    while (get_ch() != '\n')
        ;
}

static void do_once(
    const char *    fullname        /* Full-path-list of the header */
)
/*
 * Process #pragma once so as not to re-include the file later.
 * This directive has been imported from GCC V.1.* / cpp as an extension.
 */
{
    if (once_list == NULL) {                /* Should initialize    */
        max_once = INIT_NUM_ONCE;
        once_list = (INC_LIST *) xmalloc( sizeof (INC_LIST) * max_once);
        once_end = &once_list[ 0];
    } else if (once_end - once_list >= max_once) {
                                            /* Double the elements  */
        once_list = (INC_LIST *) xrealloc( (void *) once_list
                , sizeof (INC_LIST) * max_once * 2);
        once_end = &once_list[ max_once];
        max_once *= 2;
    }
    once_end->name = fullname;
    once_end->len = strlen( fullname);
    once_end++;
}

static int  included(
    const char *    fullname
)
/*
 * Has the file been once included ?
 * This routine is only called from open_file().
 */
{
    INC_LIST *  inc;
    size_t      fnamelen;

    if (once_list == NULL)              /* No once file registered  */
        return  FALSE;
    fnamelen = strlen( fullname);
    for (inc = once_list; inc < once_end; inc++) {
        if (inc->len == fnamelen && str_case_eq( inc->name, fullname)) {
            /* Already included */
            if (mcpp_debug & PATH)
                mcpp_fprintf( DBG, "Once included \"%s\"\n", fullname);
            return  TRUE;
        }
    }
    return  FALSE;                          /* Not yet included     */
}

static void push_or_pop(
    int     direction
)
/* Process #pragma MCPP push_macro( "MACRO"),
 * #pragma MCPP pop_macro( "MACRO") for other compilers than Visual C,
 * and #pragma push_macro( "MACRO"), #pragma pop_macro( "MACRO") for Visual C.
 * Note:1. "push" count is set in defp->push.
 *      2. pushed definitions are inserted immediatly after the current
 *          definition of the same name.
 *      3. the definitions of a same name macro can be pushed multiple times.
 */
{
    char *          tp;
    DEFBUF **       prevp;
    DEFBUF *        defp;
    DEFBUF *        dp;
    int             cmp;
    size_t          s_name, s_def;

    if (skip_ws() == '('
            && scan_token( skip_ws(), (tp = work_buf, &tp), work_end) == STR
            && skip_ws() == ')') {          /* Correct syntax       */

        if (is_junk())
            return;
        s_name = strlen( work_buf) - 2;
        *(work_buf + s_name + 1) = '\0';
        memcpy( identifier, work_buf + 1, s_name + 1);
                                            /* Remove enclosing '"' */
        prevp = look_prev( identifier, &cmp);
        if (cmp == 0) { /* Current definition or pushed definition exists   */
            defp = *prevp;
            if (direction == PUSH) {/* #pragma push_macro( "MACRO") */
                if (defp->push) {           /* No current definition*/
                    if (warn_level & 1)
                        cwarn( "\"%s\" is already pushed"   /* _W1_ */
                                , identifier, 0L, NULL);
                    return;
                }
                /* Else the current definition exists.  Push it     */
                s_def = sizeof (DEFBUF) + 3 + s_name
                        + strlen( defp->repl) + strlen( defp->fname);
                if (mcpp_mode == STD)
                    s_def += strlen( defp->parmnames);
                dp = (DEFBUF *) xmalloc( s_def);
                memcpy( dp, defp, s_def);   /* Copy the definition  */
                dp->link = *prevp;          /* Insert to linked-list*/
                *prevp = dp;                /*      the pushed def  */
                prevp = &dp->link;          /* Next link to search  */
            } else {                /* #pragma pop_macro( "MACRO")  */
                if (defp->push == 0) {      /* Current definition   */
                    if (defp->link == NULL
                            || ! str_eq( identifier, defp->link->name)) {
                        if (warn_level & 1)
                            cwarn( "\"%s\" has not been pushed"     /* _W1_ */
                                    , identifier, 0L, NULL);
                        return;
                    } else {
                        *prevp = defp->link;
                                /* Link the previous and the next   */
                        free( defp);
                            /* Delete the definition to enable popped def   */
                    }
                }   /* Else no current definition exists    */
            }
            while ((defp = *prevp) != NULL) {
                /* Increment or decrement "push" count of all pushed defs   */
                if ((cmp = memcmp( defp->name, identifier, s_name)) > 0)
                    break;
                defp->push += direction;        /* Increment or decrement   */
                prevp = &defp->link;
            }
        } else {    /* No current definition nor pushed definition  */
            if (warn_level & 1)
                cwarn( "\"%s\" has not been defined"        /* _W1_ */
                        , identifier, 0L, NULL);
        }
    } else {        /* Wrong syntax */
        if (warn_level & 1)
            cwarn( "Bad %s syntax", direction == PUSH       /* _W1_ */
                    ? "push_macro" : "pop_macro", 0L, NULL);
    }
}

static void do_asm(
    int     asm_start                       /* #asm ?               */
)
/*
 * #asm, #endasm
 * Originally written for OS-9/09 Microware C.
 */
{
    if (! compiling)
        return;
    if (asm_start == (in_asm != 0L)) {
        if (in_asm)
            cerror( "In #asm block started at line %.0s%ld" /* _E_  */
                    , NULL, in_asm, NULL);
        else
            cerror( "Without #asm", NULL, 0L, NULL);        /* _E_  */
        skip_nl();
        unget_ch();
        return;
    }
    in_asm = asm_start ? src_line : 0L;
}

void    do_old( void)
/*
 * Process the out-of-standard directives.
 * GCC permits #include_next and #warning even in STANDARD mode.
 */
{
    static const char * const   unknown
            = "Unknown #directive \"%s\"%.0ld%s";       /* _E_ _W8_ */
    static const char * const   ext
            = "%s is not allowed by Standard%.0ld%s";   /* _W2_ _W8_*/

#if COMPILER == GNUC
    if (str_eq( identifier, "include_next")) {
        if ((compiling && (warn_level & 2))
                || (! compiling && (warn_level & 8)))
            cwarn( ext, "#include_next", 0L
                    , compiling ? NULL : " (in skipped block)");
        if (! compiling)
            return;
        in_include = TRUE;
        do_include( TRUE);
        in_include = FALSE;
        return;
    } else if (str_eq( identifier, "warning")) {
        if ((compiling && (warn_level & 2))
                || (! compiling && (warn_level & 8)))
            cwarn( ext, "#warning", 0L
                    , compiling ? NULL : " (in skipped block)");
        if (! compiling)
            return;
        cwarn( infile->buffer, NULL, 0L, NULL);
                                    /* Always output the warning    */
        skip_nl();
        unget_ch();
        return;
    } else if (str_eq( identifier, "ident") || str_eq( identifier, "sccs")) {
        if ((compiling && (warn_level & 1))
                || (! compiling && (warn_level & 8))) {
            if (str_eq( identifier, "ident"))
                cwarn(
    compiling ? "Ignored #ident" : "#ident (in skipped block)"  /* _W1_ _W8_*/
                        , NULL, 0L, NULL);
            else
                cwarn(
    compiling ? "Ignored #sccs" : "#sccs (in skipped block)"    /* _W1_ _W8_*/
                        , NULL, 0L, NULL);
        }
        if (! compiling)
            return;
        skip_nl();
        unget_ch();
        return;
    }
#endif  /* COMPILER == GNUC */

#if COMPILER == MSC
    if (str_eq( identifier, "using") || str_eq( identifier, "import")) {
                                            /* #using or #import    */
        if (! compiling)
            return;
        mcpp_fputs( infile->buffer, OUT);   /* Putout the line as is*/
        skip_nl();
        unget_ch();
        return;
    }
#endif

#if SYSTEM == SYS_MAC
    if (str_eq( identifier, "import")) {
        if ((compiling && (warn_level & 2))
                || (! compiling && (warn_level & 8)))
            cwarn( ext, "#import", 0L
                    , compiling ? NULL : " (in skipped block)");
        if (! compiling)
            return;
        in_import = in_include = TRUE;
        do_include( FALSE);
        in_import = in_include = FALSE;
        return;
    }
#endif

    if (! standard && do_prestd_directive())
        return;

    if (compiling) {
        if (option_flags.lang_asm) {        /* "Assembler" source   */
            if (warn_level & 1)
                cwarn( unknown, identifier, 0L, NULL);
            mcpp_fputs( infile->buffer, OUT);   /* Putout the line  */
        } else {
            cerror( unknown, identifier, 0L, NULL);
        }
    } else if (warn_level & 8) {
        cwarn( unknown, identifier, 0L, " (in skipped block)");
    }
    skip_nl();
    unget_ch();
    return;
}

static int  do_prestd_directive( void)
/*
 * Process directives for pre-Standard mode.
 */
{
#if COMPILER != GNUC
    if (str_eq( identifier, "assert")) {    /* #assert              */
        if (! compiling)                    /* Only validity check  */
            return  TRUE;
        if (eval_if() == 0L) {              /* Assert expression    */
            cerror( "Preprocessing assertion failed"        /* _E_  */
                    , NULL, 0L, NULL);
            skip_nl();
            unget_ch();
        }
        return  TRUE;
    } else
#endif
    if (str_eq( identifier, "put_defines")) {
        if (! compiling)                    /* Only validity check  */
            return  TRUE;
        if (mcpp_mode != OLD_PREP && ! is_junk())
            dump_def( TRUE, FALSE);         /* #put_defines         */
        skip_nl();
        unget_ch();
        return  TRUE;
    } else if (str_eq( identifier, "preprocess")) {
        if (! compiling)                    /* Only validity check  */
            return  TRUE;
        if (mcpp_mode != OLD_PREP && ! is_junk())
        /* Just putout the directive for the succeding preprocessor */
            mcpp_fputs( "#preprocessed\n", OUT);
        skip_nl();
        unget_ch();
        return  TRUE;
    } else if (str_eq( identifier, "preprocessed")) {
        if (! compiling)                    /* Only validity check  */
            return  TRUE;
        if (mcpp_mode != OLD_PREP && ! is_junk()) {
            skip_nl();
            do_preprocessed();              /* #preprocessed        */
            return  TRUE;
        }
        skip_nl();
        unget_ch();
        return  TRUE;
    }

    if (str_eq( identifier, "debug")) {     /* #debug <args>        */
        if (! compiling)                    /* Only validity check  */
            return  TRUE;
        do_debug( TRUE);
        return  TRUE;
    } else if (str_eq( identifier, "end_debug")) {
        if (! compiling)
            return  TRUE;
        do_debug( FALSE);                   /* #end_debug <args>    */
        return  TRUE;
    }

    if (str_eq( identifier, "asm")) {       /* #asm                 */
        do_asm( TRUE);
        return  TRUE;
    }
    if (str_eq( identifier, "endasm")) {    /* #endasm              */
        do_asm( FALSE);
        skip_nl();                          /* Skip comments, etc.  */
        unget_ch();
        return  TRUE;
    }

    return  FALSE;                          /* Unknown directive    */
}

static void do_preprocessed( void)
/*
 * The source file has been already preprocessed.
 * Copy the lines to output.
 * Install macros according the #define directives.
 */
{
    const char *    corrupted =
            "This preprocessed file is corrupted";          /* _F_  */
    FILEINFO *      file;
    char *          lbuf;
    char *          cp;
    const char **   incptr;
    char *          comment = NULL;
    char *          colon = NULL;
    const char *    dir;
#if STD_LINE_PREFIX == FALSE
    char            conv[ NBUFF];
    char *          arg;

    /*
     * Compiler cannot accept C source style #line.
     * Convert it to the compiler-specific format.
     */
    strcpy( conv, LINE_PREFIX);
    arg = conv + strlen( conv);
#endif
    file = infile;
    lbuf = file->bptr = file->buffer;           /* Reset file->bptr */

    /* Copy the input to output until a comment line appears.       */
    while (fgets( lbuf, NBUFF, file->fp) != NULL
            && memcmp( lbuf, "/*", 2) != 0) {
#if STD_LINE_PREFIX == FALSE
        if (memcmp( lbuf, "#line ", 6) == 0) {
            strcpy( arg, lbuf + 6);
            mcpp_fputs( conv, OUT);
        } else
#endif
        {
            mcpp_fputs( lbuf, OUT);
        }
    }
    if (! str_eq( lbuf, "/* Currently defined macros. */\n"))
        cfatal( "This is not a preprocessed source"         /* _F_  */
                , NULL, 0L, NULL);

    /* Define macros according to the #define lines.    */
    while (fgets( lbuf, NWORK, file->fp) != NULL) {
        if (memcmp( lbuf, "/*", 2) == 0) {
                                    /* Standard predefined macro    */
            continue;
        }
        if (memcmp( lbuf, "#define ", 8) != 0) {
            if (memcmp( lbuf, "#line", 5) == 0)
                continue;
            else
                cfatal( corrupted, NULL, 0L, NULL);
        }
        /* Filename and line-number information in comment as:  */
        /* dir/fname:1234\t*/
        cp = lbuf + strlen( lbuf);
        if ((memcmp( cp - 4, "\t*/\n", 4) != 0)
                || (*(cp - 4) = EOS
                        , (comment = strrchr( lbuf, '*')) == NULL)
                || (memcmp( --comment, "/* ", 3) != 0)
                || ((colon = strrchr( comment, ':')) == NULL))
            cfatal( corrupted, NULL, 0L, NULL);
        src_line = atol( colon + 1);        /* Pseudo line number   */
        *colon = EOS;
        dir = comment + 3;
        inc_dirp = &null;
        /* Search the include directory list    */
        for (incptr = incdir ; incptr < incend; incptr++) {
            if (memcmp( *incptr, dir, strlen( *incptr)) == 0) {
                inc_dirp = incptr;
                break;
            }
        }
        /* Register the filename to fnamelist[] */
        /* inc_dirp may be NULL, and cur_fname may be "(predefined)"    */
        cur_fname = set_fname( dir + strlen( *inc_dirp));
        strcpy( comment - 2, "\n");         /* Remove the comment   */
        unget_string( lbuf + 8, NULL);
        do_define( FALSE, 0);
        get_ch();                               /* '\n' */
        get_ch();                               /* Clear the "file" */
        unget_ch();                             /* infile == file   */
    }
    file->bptr = file->buffer + strlen( file->buffer);
}

static int  do_debug(
    int     set                         /* TRUE to set debugging    */
)
/*
 * #pragma MCPP debug, #pragma MCPP end_debug, #debug, #end_debug
 * Return TRUE when diagnostic is issued else return FALSE.
 */
{
    struct Debug_arg {
        const char *    arg_name;               /* Name of option   */
        int     arg_num;                        /* Value of 'debug' */
    };
    static struct Debug_arg     debug_args[] = {
        { "path",   PATH    },
        { "token",  TOKEN   },
        { "expand", EXPAND  },
        { "macro_call", MACRO_CALL  },      /* Implemented only in STD mode */
        { "if",     IF      },
        { "expression", EXPRESSION  },
        { "getc",   GETC    },
        { "memory", MEMORY  },
        { NULL,     0       },
    };
    struct Debug_arg    *argp;
    int     num;
    int     c;

    c = skip_ws();
    if (c == '\n') {
        unget_ch();
        if (set) {
            if (warn_level & 1)
                cwarn( "No argument", NULL, 0L, NULL);      /* _W1_ */
            return TRUE;
        } else {
            mcpp_debug = 0;                 /* Clear all the flags  */
            return FALSE;
        }
    }
    while (scan_token( c, (workp = work_buf, &workp), work_end) == NAM) {
        argp = debug_args;
        while (argp->arg_name) {
            if (str_eq( argp->arg_name, work_buf))
                break;
            argp++;
        }
        if (argp->arg_name == NULL) {
            if (warn_level & 1)
                cwarn( unknown_arg, work_buf, 0L, NULL);
            goto  diagnosed;
        } else {
            num = argp->arg_num;
            if (set) {
                mcpp_debug |= num;
                if (num == PATH)
                    dump_path();
                else if (num == MEMORY)
                    print_heap();
                else if (num == MACRO_CALL)
                    option_flags.k = TRUE;  /* This pragma needs this mode  */
            } else {
                mcpp_debug &= ~num;
            }
        }
        c = skip_ws();
    }
    if ((mcpp_mode != STD && (mcpp_debug & MACRO_CALL)) || c != '\n') {
        if (warn_level & 1) {
            if (c != '\n') {
                cwarn( not_ident, work_buf, 0L, NULL);
            } else {
                cwarn( unknown_arg, work_buf, 0L, NULL);
                mcpp_debug &= ~num;                     /* Disable  */
            }
        }
        skip_nl();
        unget_ch();
        goto  diagnosed;
    }
    unget_ch();
    return FALSE;
diagnosed:
    return TRUE;
}

void    put_asm( void)
/*
 * Put out source line as it is.
 */
{
#if 0
    mcpp_fputs( "#2\n", OUT);
    mcpp_fputs( infile->buffer, OUT);
    skip_nl();
#endif
}

static void dump_path( void)
/*
 * Show the include directories.
 */
{
    const char **   incptr;
    const char *    inc_dir;
    const char *    dir = "./";
    int             i;

    mcpp_fputs( "Include paths are as follows --\n", DBG);
    for (incptr = incdir; incptr < incend; incptr++) {
        inc_dir = *incptr;
        if (*inc_dir == '\0')
            inc_dir = dir;
        mcpp_fprintf( DBG, "    %s\n", inc_dir);
    }
    mcpp_fputs( "End of include path list.\n", DBG);
#if SYSTEM == SYS_MAC
    mcpp_fputs( "Framework paths are as follows --\n", DBG);
    for (i = 0; i < num_framework; i++ )
        mcpp_fprintf( DBG, "    %s\n", framework[ i]);
    mcpp_fputs( "End of framework path list.\n", DBG);
#endif
}

/*
 * Note: The getopt() of glibc should not be used since the specification
 *  differs from the standard one.
 *  Use this mcpp_getopt() for mcpp.
 */

/* Based on the public-domain-software released by AT&T in 1985.    */

#define OPTERR( s, c)   if (mcpp_opterr) {  \
    mcpp_fputs( argv[0], ERR);  \
    mcpp_fputs( s, ERR);        \
    mcpp_fputc( c, ERR);        \
    mcpp_fputc( '\n', ERR);     \
    }

static int  mcpp_getopt(
    int         argc,
    char * const *  argv,
    const char *    opts
)
/*
 * Get the next option (and it's argument) from the command line.
 */
{
    const char * const   error1 = ": option requires an argument --";
    const char * const   error2 = ": illegal option --";
    static int      sp = 1;
    int             c;
    const char *    cp;

    if (sp == 1) {
        if (argc <= mcpp_optind ||
                argv[ mcpp_optind][ 0] != '-'
                    || argv[ mcpp_optind][ 1] == '\0') {
            return  EOF;
        } else if (strcmp( argv[ mcpp_optind], "--") == 0) {
            mcpp_optind++;
            return  EOF;
        }
    }
/*  mcpp_optopt = c = (unsigned char) argv[ mcpp_optind][ sp];  */
    mcpp_optopt = c = argv[ mcpp_optind][ sp] & UCHARMAX;
    if (c == ':' || (cp = strchr( opts, c)) == NULL) {
        OPTERR( error2, c)
        if (argv[ mcpp_optind][ ++sp] == '\0') {
            mcpp_optind++;
            sp = 1;
        }
        return  '?';
    }
    if (*++cp == ':') {
        if (argv[ mcpp_optind][ sp+1] != '\0') {
            mcpp_optarg = &argv[ mcpp_optind++][ sp+1];
        } else if (argc <= ++mcpp_optind) {
            OPTERR( error1, c)
            sp = 1;
            return  '?';
        } else {
            mcpp_optarg = argv[ mcpp_optind++];
        }
        sp = 1;
    } else {
        if (argv[ mcpp_optind][ ++sp] == '\0') {
            sp = 1;
            mcpp_optind++;
        }
        mcpp_optarg = NULL;
    }
    return  c;
}

#if ! HOST_HAVE_STPCPY
char *  stpcpy(
    char *          dest,
    const char *    src
)
/*
 * Copy the string and return the advanced pointer.
 */
{
    const char * s;
    char *  d;

    for (s = src, d = dest; (*d++ = *s++) != '\0'; )
        ;
    return  d - 1;
}
#endif

/*
 * list_heap() is a function to print out information of heap-memory.
 * See "kmmalloc-2.5.3.zip" by kmatsui.
 */
#if     KMMALLOC
    int     list_heap( int);
#elif   BSD_MALLOC
    int     list_heap( char *);
#elif   DB_MALLOC || DMALLOC
    int     list_heap( FILE *);
#endif

void    print_heap( void)
{
#if     KMMALLOC
    list_heap( 1);
#elif   BSD_MALLOC
    list_heap( ":cpp");
#elif   DB_MALLOC || DMALLOC || PHK_MALLOC || DLMALLOC
    list_heap( fp_debug);
#endif
}

void    at_end( void)
/*
 * Handle the commands to be executed at the end of processing.
 */
{
#if COMPILER == GNUC
    if (dMflag || dDflag)
        dump_def( FALSE, FALSE);
#endif
}

#if MCPP_LIB
void    clear_filelist( void)
/*
 * Free malloced memory for filename-list and directory-list.
 */
{
    const char **   incp;
    INC_LIST *  namep;

    for (incp = incdir; incp < incend; incp++)
        free( (void *) *incp);
    free( (void *) incdir);
    for (namep = fnamelist; namep < fname_end; namep++)
        free( (void *) namep->name);
    free( (void *) fnamelist);
    if (standard)
        free( (void *) once_list);
}
#endif
