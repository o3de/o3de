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
 *                          E X P A N D . C
 *                  M a c r o   E x p a n s i o n
 *
 * The macro expansion routines are placed here.
 */

#if PREPROCESSED
#include    "mcpp.H"
#else
#include    "system.H"
#include    "internal.H"
#endif

#define ARG_ERROR   (-255)
#define CERROR      1
#define CWARN       2

typedef struct location {           /* Where macro or arg locate    */
    long            start_line;                 /* Beginning at 1   */
    size_t          start_col;                  /* Beginning at 0   */
    long            end_line;
    size_t          end_col;
} LOCATION;
typedef struct magic_seq {  /* Data of a sequence inserted between tokens   */
    char *          magic_start;        /* First MAC_INF sequence   */
    char *          magic_end;          /* End of last MAC_INF seq  */
    int             space;              /* Space succeeds or not    */
} MAGIC_SEQ;

static int      compat_mode;
/* Expand recursive macro more than Standard (for compatibility with GNUC)  */
#if COMPILER == GNUC
static int      ansi;                   /* __STRICT_ANSI__ flag     */
#endif

static char *   expand_std( DEFBUF * defp, char * out, char * out_end
        , LINE_COL line_col, int * pragma_op);
                /* Expand a macro completely (for Standard modes)   */
static char *   expand_prestd( DEFBUF * defp, char * out, char * out_end
        , LINE_COL line_col, int * pragma_op);
                /* Expand a macro completely (for pre-Standard modes)       */
static DEFBUF * is_macro_call( DEFBUF * defp, char ** cp, char ** endf
        , MAGIC_SEQ * mgc_seq);         /* Is this really a macro call ?    */
static int      collect_args( const DEFBUF * defp, char ** arglist, int m_num);
                /* Collect arguments of a macro call*/
static int      get_an_arg( int c, char ** argpp, char * arg_end
        , char ** seqp, int var_arg, int nargs, LOCATION ** locp, int m_num
        , MAGIC_SEQ * mgc_prefix);      /* Get an argument          */
static int      squeeze_ws( char ** out, char ** endf, MAGIC_SEQ * mgc_seq);
                /* Squeeze white spaces to a space  */
static void     skip_macro( void);
                /* Skip the rest of macro call      */
static void     diag_macro( int severity, const char * format
        , const char * arg1, long arg2, const char * arg3, const DEFBUF * defp1
        , const DEFBUF * defp2) ;
                /* Supplement diagnostic information*/
static void     dump_args( const char * why, int nargs, const char ** arglist);
                /* Dump arguments list              */

static int      rescan_level;           /* Times of macro rescan    */

static const char * const   macbuf_overflow
        = "Buffer overflow expanding macro \"%s\" at %.0ld\"%s\"";  /* _E_  */
static const char * const   empty_arg
        = "Empty argument in macro call \"%s\"";            /* _W2_ */
static const char * const   unterm_macro
        = "Unterminated macro call \"%s\"";                 /* _E_  */
static const char * const   narg_error
    = "%s than necessary %ld argument(s) in macro call \"%s\""; /* _E_ _W1_ */
static const char * const   only_name
        = "Macro \"%s\" needs arguments";                   /* _W8_ */

void     expand_init(
    int     compat,     /* "Compatible" to GNUC expansion of recursive macro*/
    int     strict_ansi         /* __STRICT_ANSI__ flag for GNUC    */
)
/* Set expand_macro() function  */
{
    expand_macro = standard ? expand_std : expand_prestd;
    compat_mode = compat;
#if COMPILER == GNUC
    ansi = strict_ansi;
#endif
}

DEFBUF *    is_macro(
    char **     cp
)
/*
 * The name is already in 'identifier', the next token is not yet read.
 * Return the definition info if the name is a macro call, else return NULL.
 */
{
    DEFBUF *    defp;

    if ((defp = look_id( identifier)) != NULL)  /* Is a macro name  */
        return  is_macro_call( defp, cp, NULL, NULL);
    else
        return  NULL;
}

static DEFBUF * is_macro_call(
    DEFBUF *    defp,
    char **     cp,                     /* Pointer to output buffer */
    char **     endf,   /* Pointer to indicate end of infile buffer */
    MAGIC_SEQ * mgc_seq /* Infs on MAC_INF sequences and space      */
)
/*
 * Return DEFBUF if the defp->name is a macro call, else return NULL.
 */
{
    int     c;

    if (defp->nargs >= 0                    /* Function-like macro  */
            || defp->nargs == DEF_PRAGMA) { /* _Pragma() pseudo-macro       */
        c = squeeze_ws( cp, endf, mgc_seq); /* See the next char.   */
        if (c == CHAR_EOF)                  /* End of file          */
            unget_string( "\n", NULL);      /* Restore skipped '\n' */
        else if (! standard || c != RT_END)
                        /* Still in the file and rescan boundary ?  */
            unget_ch();                     /* To see it again      */
        if (c != '(') {     /* Only the name of function-like macro */
            if (! standard && warn_level & 8)
                cwarn( only_name, defp->name, 0L, NULL);
            return  NULL;
        }
    }
    return  defp;                           /* Really a macro call  */
}

/*
 * expand_macro()   expands a macro call completely, and writes out the result
 *      to the specified output buffer and returns the advanced pointer.
 */


/*
 *          T h e   S T A N D A R D   C o n f o r m i n g   M o d e
 *                  o f   M a c r o   E x p a n s i o n
 *
 * 1998/08      First released.     kmatsui
 */

/* For debug of -K option: should be turned off on release version. */
#define DEBUG_MACRO_ANN     FALSE

/* Return value of is_able_repl()   */
#define NO          0               /* "Blue-painted"               */
#define YES         1               /* Not blue-painted             */
#define READ_OVER   2
            /* Still "blue-painted", yet has read over repl-list    */

/*
 * Macros related to macro notification mode.
 * The macro notification routines are hacks on the normal processing
 * routines, and very complicated and cumbersome.  Be sure to keep symmetry
 * of starting and closing magic sequences.  Enable the routines enclosed
 * by #if 0 - #endif for debugging.
 * Any byte in the sequences beginning with MAC_INF can coincide with any
 * other character.  Hence, the data stream should be read from the top,
 * not from the tail, in principle.
 */
/* Length of sequence of MAC_INF, MAC_CALL_START, mac_num-1, mac_num-2  */
#define MAC_S_LEN   4
/* Length of sequence of MAC_INF, MAC_ARG_START, mac_num1, mac_num2, arg-num*/
#define ARG_S_LEN   5
#define MAC_E_LEN   2   /* Length of MAC_INF, MAC_CALL_END sequence */
#define ARG_E_LEN   MAC_E_LEN   /* Lenght of MAC_INF, MAC_ARG_END sequence  */
#define MAC_E_LEN_V 4   /* Length of macro closing sequence in verbose mode */
                        /* MAC_INF, MAC_CALL_END, mac_num1, mac_num2        */
#define ARG_E_LEN_V 5   /* Length of argument closing sequence in verbose   */
                    /* MAC_INF, MAC_ARG_END, mac_num1, mac_num2, arg_num    */
#define IN_SRC_LEN  3   /* Length of sequence of IN_SRC, num-1, num-2   */
#define INIT_MAC_INF        0x100   /* Initial num of elements in mac_inf[] */
#define MAX_MAC_INF         0x1000  /* Maximum num of elements in mac_inf[] */
#define INIT_IN_SRC_NUM     0x100   /* Initial num of elements in in_src[]  */
#define MAX_IN_SRC_NUM      0x1000  /* Maximum num of elements in in_src[]  */

/* Variables for macro notification mode    */
typedef struct macro_inf {              /* Informations of a macro  */
    const DEFBUF *  defp;               /* Definition of the macro  */
    char *          args;               /* Arguments, if any        */
    int             num_args;           /* Number of real arguments */
    int             recur;              /* Recurrence of this macro */
    LOCATION        locs;               /* Location of macro call   */
    LOCATION *      loc_args;           /* Location of arguments    */
} MACRO_INF;
static MACRO_INF *  mac_inf;
static int      max_mac_num;        /* Current num of elements in mac_inf[] */
static int      mac_num;                /* Index into mac_inf[]     */
static LOCATION *   in_src; /* Location of identifiers in macro arguments   */
static int      max_in_src_num;     /* Current num of elements in in_src[]  */
static int      in_src_num;             /* Index into in_src[]      */
static int      trace_macro;        /* Enable to trace macro infs   */

static struct {
    const DEFBUF *  def;            /* Macro definition             */
    int             read_over;      /* Has read over repl-list      */
    /* 'read_over' is never used in POST_STD mode and in compat_mode*/
} replacing[ RESCAN_LIMIT];         /* Macros currently replacing   */
static int      has_pragma = FALSE;     /* Flag of _Pragma() operator       */

static int      print_macro_inf( int c, char ** cpp, char ** opp);
                /* Embed macro infs into comments   */
static char *   print_macro_arg( char *out, MACRO_INF * m_inf, int argn
        , int real_arg, int start);
                /* Embed macro arg inf into comments*/
static char *   chk_magic_balance( char * buf, char * buf_end, int move
        , int diag);    /* Check imbalance of magics        */
static char *   replace( DEFBUF * defp, char * out, char * out_end
        , const DEFBUF * outer, FILEINFO * rt_file, LINE_COL line_col
        , int in_src_n);
                /* Replace a macro recursively      */
static char *   close_macro_inf( char *  out_p, int m_num, int in_src_n);
                /* Put closing mark for a macro call*/
static DEFBUF * def_special( DEFBUF * defp);
                /* Re-define __LINE__, __FILE__     */
static int      prescan( const DEFBUF * defp, const char ** arglist
        , char * out, char * out_end);
                /* Process #, ## operator           */
static char *   catenate( const DEFBUF * defp, const char ** arglist
        , char * out, char * out_end, char ** token_p);
                /* Catenate tokens                  */
static const char * remove_magics( const char * argp, int from_last);
                /* Remove pair of magic characters  */
#if DEBUG_MACRO_ANN
static void     chk_symmetry( char *  start_id, char *  end_id, size_t  len);
                /* Check if a pair of magics are symmetrical    */
#endif
static char *   stringize( const DEFBUF * defp, const char * argp, char * out);
                /* Stringize an argument            */
static char *   substitute( const DEFBUF * defp, const char ** arglist
        , const char * in, char * out, char * out_end);
                /* Substitute parms with arguments  */
static char *   rescan( const DEFBUF * outer, const char * in, char * out
        , char * out_end);
                /* Rescan once replaced sequences   */
static int      disable_repl( const DEFBUF * defp);
                /* Disable the macro once replaced  */
static void     enable_repl( const DEFBUF * defp, int done);
                /* Enable the macro for later use   */
static int      is_able_repl( const DEFBUF * defp);
                /* Is the macro allowed to replace? */
static char *   insert_to_bptr( char * ins, size_t len);
                /* Insert a sequence into infile->bptr  */

static char *   expand_std(
    DEFBUF *    defp,                       /* Macro definition     */
    char *  out,                            /* Output buffer        */
    char *  out_end,                        /* End of output buffer */
    LINE_COL    line_col,                   /* Location of macro    */
    int *   pragma_op                       /* _Pragma() is found ? */
)
/*
 * Expand a macro call completely, write the results to the specified buffer
 * and return the advanced output pointer.
 */
{
    char    macrobuf[ NMACWORK + IDMAX];    /* Buffer for replace() */
    char *  out_p = out;
    size_t  len;
    int     c, c1;
    char *  cp;

    has_pragma = FALSE;                     /* Have to re-initialize*/
    macro_line = src_line;                  /* Line number for diag */
    macro_name = defp->name;
    rescan_level = 0;
    trace_macro = (mcpp_mode == STD) && (mcpp_debug & MACRO_CALL)
            && ! in_directive;
    if (trace_macro) {
        max_mac_num = INIT_MAC_INF;
        mac_inf = (MACRO_INF *) xmalloc( sizeof (MACRO_INF) * max_mac_num);
        memset( mac_inf, 0, sizeof (MACRO_INF) * max_mac_num);
        max_in_src_num = INIT_IN_SRC_NUM;
        in_src = (LOCATION *) xmalloc( sizeof (LOCATION) * max_in_src_num);
        memset( in_src, 0, sizeof (LOCATION) * max_in_src_num);
        mac_num = in_src_num = 0;           /* Initialize           */
    }
    if (replace( defp, macrobuf, macrobuf + NMACWORK, NULL, infile, line_col
            , 0) == NULL) {                 /* Illegal macro call   */
        skip_macro();
        macro_line = MACRO_ERROR;
        goto  exp_end;
    }
    len = (size_t) (out_end - out);
    if (strlen( macrobuf) > len) {
        cerror( macbuf_overflow, macro_name, 0, macrobuf);
        memcpy( out, macrobuf, len);
        out_p = out + len;
        macro_line = MACRO_ERROR;
        goto  exp_end;
    }

#if DEBUG_MACRO_ANN
    chk_magic_balance( macrobuf, macrobuf + strlen( macrobuf), FALSE, TRUE);
#endif
    cp = macrobuf;
    c1 = '\0';                          /* The char previous to 'c' */
    while ((c = *cp++) != EOS) {
        if (c == DEF_MAGIC)
            continue;                       /* Skip DEF_MAGIC       */
        if (mcpp_mode == STD) {
            if (c == IN_SRC) {              /* Skip IN_SRC          */
                if (trace_macro)
                    cp += 2;    /* Skip also the number (coded in 2 bytes)  */
                continue;
            } else if (c == TOK_SEP) {
                /* Remove redundant token separator */
                if ((char_type[ c1 & UCHARMAX] & HSP)
                        || (char_type[ *cp & UCHARMAX] & HSP)
                        || in_include || option_flags.lang_asm
                        || (*cp == MAC_INF && *(cp + 1) == MAC_CALL_END)
                        || (!option_flags.v && c1 == MAC_CALL_END)
                        || (option_flags.v
                            && *(cp - MAC_E_LEN_V - 1) == MAC_INF
                            && *(cp - MAC_E_LEN_V) == MAC_CALL_END))
                    continue;
                    /* Skip separator just after ' ', '\t'          */
                    /* and just after MAC_CALL_END.                 */
                    /* Also skip this in lang_asm mode, #include    */
                    /* Skip just before another TOK_SEP, ' ', '\t'  */
                    /* Skip just before MAC_INF,MAC_CALL_END seq too*/
                else
                    c = ' ';                /* Else convert to ' '  */
            } else if (trace_macro && (c == MAC_INF)) {
                /* Embed macro expansion informations into comments */
                c = *cp++;
                c1 = print_macro_inf( c, &cp, &out_p);
                if (out_end <= out_p) {
                    cerror( macbuf_overflow, macro_name, 0, out);
                    macro_line = MACRO_ERROR;
                    goto  exp_end;
                }
                continue;
            }
        }
        *out_p++ = c1 = c;
    }

    macro_line = 0;
exp_end:
    *out_p = EOS;
    if (mcpp_debug & EXPAND)
        dump_string( "expand_std exit", out);
    macro_name = NULL;
    clear_exp_mac();        /* Clear the information for diagnostic */
    if (trace_macro) {                  /* Clear macro informations */
        int     num;
        for (num = 1; num < mac_num; num++) {   /* 'num' start at 1 */
            if (mac_inf[ num].num_args >= 0) {  /* Macro with args  */
                free( mac_inf[ num].args);      /* Saved arguments  */
                free( mac_inf[ num].loc_args);  /* Location of args */
            }
        }
        free( mac_inf);
        free( in_src);
    }
    *pragma_op = has_pragma;

    return  out_p;
}

static int  print_macro_inf(
    int     c,
    char ** cpp,                    /* Magic character sequence     */
    char ** opp                     /* Output for macro information */
)
/*
 * Embed macro expansion information into comments.
 * Enabled by '#pragma MCPP debug macro_call' or -K option in STD mode.
 */
{
    MACRO_INF *     m_inf;
    int     num;
    int     num_args;   /* Number of actual args (maybe less than expected) */
    int     i;

    if (*((*opp) - 1) == '/' && *((*opp) - 2) != '*')
                    /* Immediately preceding token is '/' (not '*' and '/') */
        *((*opp)++) = ' ';
                /* Insert a space to separate with following '/' and '*'    */
    if (option_flags.v || c == MAC_CALL_START || c == MAC_ARG_START) {
        num = ((*(*cpp)++ & UCHARMAX) - 1) * UCHARMAX;
        num += (*(*cpp)++ & UCHARMAX) - 1;
        m_inf = & mac_inf[ num];            /* Saved information    */
    }
    switch (c) {
    case MAC_CALL_START :           /* Start of a macro expansion   */
        *opp += sprintf( *opp, "/*<%s", m_inf->defp->name); /* Macro name   */ 
        if (m_inf->locs.start_line) {
            /* Location of the macro call in source file        */
            *opp += sprintf( *opp, " %ld:%d-%ld:%d"
                    , m_inf->locs.start_line, (int) m_inf->locs.start_col
                    , m_inf->locs.end_line, (int) m_inf->locs.end_col);
        }
        *opp = stpcpy( *opp, "*/");
        if ((num_args = m_inf->num_args) >= 1) {
            /* The macro has arguments.  Show the locations.    */
            for (i = 0; i < num_args; i++)  /* Arg num begins at 0  */
                *opp = print_macro_arg( *opp, m_inf, i, TRUE, TRUE);
        }
        break;
    case MAC_ARG_START  :                   /* Start of an argument */
        i = (*(*cpp)++ & UCHARMAX) - 1;     /* Argument number      */
        *opp = print_macro_arg( *opp, m_inf, i, FALSE, TRUE);
        break;
    case MAC_CALL_END   :               /* End of a macro expansion */
        if (option_flags.v) {               /* Verbose mode         */
            *opp += sprintf( *opp, "/*%s>*/", m_inf->defp->name);
            break;
        }
        /* Else fall through    */
    case MAC_ARG_END    :               /* End of an argument       */
        if (option_flags.v) {
            i = (*(*cpp)++ & UCHARMAX) - 1;
           /* Output verbose infs symmetrical to start of the arg infs  */
            *opp = print_macro_arg( *opp, m_inf, i, FALSE, FALSE);
        } else {
            *opp = stpcpy( *opp, "/*>*/");
        }
        break;
    }

    return  **cpp & UCHARMAX;
}

static char *   print_macro_arg( 
    char *  out,                                /* Output buffer    */
    MACRO_INF *     m_inf,                      /* &mac_inf[ m_num] */
    int     argn,                               /* Argument number  */
    int     real_arg,       /* Real argument or expanded argument ? */
    int     start           /* Start of an argument or end ?        */
) 
/*
 * Embed an argument information into a comment.
 * This routine is only called from above print_macro_inf().
 */
{
    LOCATION *  loc = m_inf->loc_args + argn;

    out += sprintf( out, "/*%s%s:%d-%d", real_arg ? "!" : (start ? "<" : "")
            , m_inf->defp->name, m_inf->recur, argn);

    if (real_arg && m_inf->loc_args && loc->start_line) {
        /* Location of the argument in source file  */
        out += sprintf( out, " %ld:%d-%ld:%d", loc->start_line
                , (int) loc->start_col, loc->end_line, (int) loc->end_col);
    }
    if (! start)            /* End of an argument in verbose mode   */
        out = stpcpy( out, ">");
    out = stpcpy( out, "*/");

    return out;
}

static char *   chk_magic_balance(
    char *  buf,                            /* Sequence to check    */
    char *  buf_end,                        /* End of the sequence  */
    int     move,                   /* Move a straying magic ?      */
    int     diag                            /* Output a diagnostic? */
)
/*
 * Check imbalance of macro information magics and warn it.
 * get_an_arg() calls this routine setting 'move' argument on, hence a stray
 * magic is moved to an edge if found.
 * This routine does not do token parsing.  Yet it will do fine practically.
 */
{
#define MAX_NEST_MAGICS 255
    char    mac_id[ MAX_NEST_MAGICS][ MAC_E_LEN_V - 2];
    char    arg_id[ MAX_NEST_MAGICS][ ARG_E_LEN_V - 2];
    char *  mac_loc[ MAX_NEST_MAGICS];
    char *  arg_loc[ MAX_NEST_MAGICS];
    char *  mesg = "%s %ld %s-closing-comment(s) in tracing macro";
    int     mac, arg;
    int     mac_s_n, mac_e_n, arg_s_n, arg_e_n;
    char *  buf_p = buf;        /* Save 'buf' for debugging purpose */

    mac = arg = 0;

    while (buf_p < buf_end) {
        if (*buf_p++ != MAC_INF)
            continue;
        switch (*buf_p++) {
        case MAC_CALL_START :
            if (option_flags.v) {
                mac_loc[ mac] = buf_p - 2;
                memcpy( mac_id[ mac], buf_p, MAC_S_LEN - 2);
            }
            mac++;
            buf_p += MAC_S_LEN - 2;
            break;
        case MAC_ARG_START  :
            if (option_flags.v) {
                arg_loc[ arg] = buf_p - 2;
                memcpy( arg_id[ arg], buf_p, ARG_S_LEN - 2);
            }
            arg++;
            buf_p += ARG_S_LEN - 2;
            break;
        case MAC_ARG_END    :
            arg--;
            if (option_flags.v) {
                if (arg < 0) {          /* Perhaps moved magic  */
                    if (diag)
                        cwarn( mesg, "Redundant", (long) -arg, "argument");
                } else if (memcmp( arg_id[ arg], buf_p, ARG_E_LEN_V - 2) != 0)
                {
                    char *      to_be_edge = NULL;
                    char *      cur_edge;

                    if (arg >= 1 && memcmp( arg_id[ 0], buf_p, ARG_E_LEN_V - 2)
                            == 0) {
                        to_be_edge = arg_loc[ arg];
                                            /* To be moved to top   */
                        cur_edge = arg_loc[ 0];     /* Current top  */
                    } else if (arg == 0) {
                        char    arg_end_magic[ 2] = { MAC_INF, MAC_ARG_END};
                        cur_edge = buf_end - ARG_E_LEN_V;
                        /* Search the last magic    */
                        /* Sequence from get_an_arg() is always     */
                        /* surrounded by starting of an arg magic   */
                        /* and its corresponding closing magic.     */
                        while (buf_p + (ARG_E_LEN_V - 2) <= cur_edge 
                                && memcmp( cur_edge, arg_end_magic, 2) != 0)
                            cur_edge--;
                        if (buf_p + (ARG_E_LEN_V - 2) <= cur_edge
                                && memcmp( arg_id[ 0], cur_edge + 2
                                        , ARG_E_LEN_V - 2) == 0) {
                            to_be_edge = buf_p - 2; /* To be moved to end   */
                        }
                    }
                    if (to_be_edge) {   /* Appropriate place found  */
                        if (diag) {
                            mac_s_n = ((to_be_edge[ 2] & UCHARMAX) - 1)
                                    * UCHARMAX;
                            mac_s_n += (to_be_edge[ 3] & UCHARMAX) - 1;
                            arg_s_n = (to_be_edge[ 4] & UCHARMAX) - 1;
                            mcpp_fprintf( ERR,
                                "Stray arg inf of macro: %d:%d at line:%d\n"
                                            , mac_s_n, arg_s_n, src_line);
                        }
                        if (move) {
                            /* Move a stray magic to outside of sequences   */
                            char    magic[ ARG_E_LEN_V];
                            size_t  len = ARG_E_LEN_V;
                            memcpy( magic, cur_edge, len);
                                /* Save current edge    */
                            if (to_be_edge == arg_loc[ arg])
                                /* Shift followings to cur_edge */
                                memmove( cur_edge, cur_edge + len
                                        , to_be_edge - cur_edge);
                            else        /* Shift precedents to cur_edge */
                                memmove( to_be_edge + len, to_be_edge
                                        , cur_edge - to_be_edge);
                            memcpy( to_be_edge, magic, len);
                            /* Restore old 'cur_edge' into old 'to_be_edge' */
                        }
                    } else {        /* Serious imbalance, just warn */
                        char *      arg_p = arg_id[ arg];
                        arg_s_n = ((arg_p[ 0] & UCHARMAX) - 1) * UCHARMAX;
                        arg_s_n += (arg_p[ 1] & UCHARMAX) - 1;
                        arg_e_n = ((buf_p[ 0] & UCHARMAX) - 1) * UCHARMAX;
                        arg_e_n += (buf_p[ 1] & UCHARMAX) - 1;
                        mcpp_fprintf( ERR,
                "Asymmetry of arg inf found: start %d, end %d at line:%d\n"
                                , arg_s_n, arg_e_n, src_line);
                    }
                }
                buf_p += ARG_E_LEN_V - 2;
            }
            break;
        case MAC_CALL_END   :
            mac--;
            if (option_flags.v) {
                if (mac < 0) {
                    if (diag)
                        cwarn( mesg, "Redundant", (long) -mac, "macro");
                } else if (memcmp( mac_id[ mac], buf_p, MAC_E_LEN_V - 2) != 0)
                {
                    char *      mac_p = mac_id[ mac];
                    mac_s_n = ((mac_p[ 0] & UCHARMAX) - 1) * UCHARMAX;
                    mac_s_n += (mac_p[ 1] & UCHARMAX) - 1;
                    mac_e_n = ((buf_p[ 0] & UCHARMAX) - 1) * UCHARMAX;
                    mac_e_n += (buf_p[ 1] & UCHARMAX) - 1;
                    mcpp_fprintf( ERR,
                "Asymmetry of macro inf found: start %d, end %d at line:%d\n"
                            , mac_s_n, mac_e_n, src_line);
                }
                buf_p += MAC_E_LEN_V - 2;
            }
            break;
        default :                       /* Not a MAC_INF sequence   */
            break;                                  /* Continue     */
        }
    }

    if (diag && (warn_level & 1)) {
        if (mac > 0)
            cwarn( mesg, "Lacking", (long) mac, "macro");
        if (arg > 0)
            cwarn( mesg, "Lacking", (long) arg, "argument");
        if ((mac || arg) && (mcpp_debug & EXPAND))
            mcpp_fputs(
"Imbalance of magics occurred (perhaps a moved magic), see <expand_std exit> and diagnostics.\n"
                    , DBG);
    }

    return  buf;
}

static char *   replace(
    DEFBUF *    defp,                       /* Macro to be replaced */
    char *      out,                        /* Output Buffer        */
    char *      out_end,                    /* End of output buffer */
    const DEFBUF *    outer,                /* Outer macro replacing*/
    FILEINFO *  rt_file,                    /* Repl-text "file"     */
    LINE_COL    line_col,                   /* Location of macro    */
    int         in_src_n                    /* Index into in_src[]  */
)
/*
 * Replace a possibly nested macro recursively.
 * replace() and rescan() call each other recursively.
 * Return the advanced output pointer or NULL on error.
 */
{
    char ** arglist = NULL;                 /* Pointers to arguments*/
    int     nargs;                  /* Number of arguments expected */
    char *  catbuf;                         /* Buffer for prescan() */
    char *  expbuf;                 /* Buffer for  substitute()     */
    char *  out_p;                          /* Output pointer       */
    char *  cur_out = out;          /* One more output pointer      */
    int     num_args;
        /* Number of actual arguments (maybe less than expected)    */
    int     enable_trace_macro;     /* To exclude _Pragma() pseudo macro    */
    int     m_num = 0;              /* 'mac_num' of current macro   */
    MACRO_INF *     m_inf;          /* Pointer into mac_inf[]       */

    if (mcpp_debug & EXPAND) {
        dump_a_def( "replace entry", defp, FALSE, TRUE, fp_debug);
        dump_unget( "replace entry");
    }
    if ((mcpp_debug & MACRO_CALL) && in_if)
        mcpp_fprintf( OUT, "/*%s*/", defp->name);

    enable_trace_macro = trace_macro && defp->nargs != DEF_PRAGMA;
    if (enable_trace_macro) {
        int     num;
        int     recurs;

        if (mac_num >= MAX_MAC_INF - 1) {
            cerror( "Too many nested macros in tracing %s"  /* _E_  */
                    , defp->name, 0L, NULL);
            return  NULL;
        } else if (mac_num >= max_mac_num - 1) {
            size_t  len = sizeof (MACRO_INF) * max_mac_num;
            /* Enlarge the array    */
            mac_inf = (MACRO_INF *) xrealloc( (char *) mac_inf, len * 2);
            memset( mac_inf + max_mac_num, 0, len);
                                        /* Clear the latter half    */
            max_mac_num *= 2;
        }
        m_num = ++mac_num;                  /* Remember this number */
                                    /* Note 'mac_num' starts at 1   */
        *cur_out++ = MAC_INF;               /* Embed a magic char   */
        *cur_out++ = MAC_CALL_START;        /* A macro call         */
        /* Its index number, can be greater than UCHARMAX           */
        /* We represent the number by 2 bytes where each byte is not '\0'   */
        *cur_out++ = (m_num / UCHARMAX) + 1;
        *cur_out++ = (m_num % UCHARMAX) + 1;
        *cur_out = EOS;
        m_inf = & mac_inf[ m_num];
        m_inf->defp = defp;                 /* The macro definition */
        m_inf->num_args = 0;                /* Default num of args  */
        if (line_col.line) {
            get_src_location( & line_col);
            m_inf->locs.start_line = line_col.line;
            m_inf->locs.start_col = line_col.col;
        } else {
            m_inf->locs.start_col = m_inf->locs.start_line = 0L;
        }
        m_inf->args = m_inf->loc_args = NULL;       /* Default args */
        for (num = 1, recurs = 0; num < m_num; num++)
            if (mac_inf[ num].defp == defp)
                recurs++;           /* Recursively nested macro     */
        m_inf->recur = recurs;
    }

    nargs = (defp->nargs == DEF_PRAGMA) ? 1 : (defp->nargs & ~AVA_ARGS);

    if (nargs < DEF_NOARGS_DYNAMIC) {       /* __FILE__, __LINE__   */
        defp = def_special( defp);      /* These are redefined dynamically  */
        if (mcpp_mode == STD) {
        /* Wrap repl-text with token separators to prevent token merging    */
            *cur_out++ = TOK_SEP;     
            cur_out = stpcpy( cur_out, defp->repl);
            *cur_out++ = TOK_SEP;     
            *cur_out = EOS;
        } else {
            cur_out = stpcpy( cur_out, defp->repl);
        }
        if (enable_trace_macro) {
            m_inf->defp = defp;             /* Redefined dynamically*/
            cur_out = close_macro_inf( cur_out, m_num, in_src_n);
        }
        return  cur_out;
    } else if (nargs == DEF_NOARGS_PREDEF_OLD && standard
            && (warn_level & 1)) {          /* Some macros on GCC   */
        cwarn( "Old style predefined macro \"%s\" is used", /* _W2_ */
                defp->name, 0L, NULL);
    } else if (nargs >= 0) {                /* Function-like macro  */
        squeeze_ws( NULL, NULL, NULL);      /* Skip to '('          */
            /* Magic sequences are already read over by is_macro_call() */
        arglist = (char **) xmalloc( (nargs + 1) * sizeof (char *));
        arglist[ 0] = xmalloc( (size_t) (NMACWORK + IDMAX * 2));
                            /* Note: arglist[ n] may be reallocated */
                            /*   and re-written by collect_args()   */
        if ((num_args = collect_args( defp, arglist, m_num)) == ARG_ERROR) {
            free( arglist[ 0]);             /* Syntax error         */
            free( arglist);
            return  NULL;
        }
        if (enable_trace_macro) {
            /* Save the arglist for later informations  */
            m_inf->args = arglist[ 0];
            m_inf->num_args = num_args;     /* Number of actual args*/
        }
        if (mcpp_mode == STD && outer && rt_file != infile) {
                                 /* Has read over replacement-text  */
            if (compat_mode) {
                enable_repl( outer, FALSE); /* Enable re-expansion  */
                if (mcpp_debug & EXPAND)
                    dump_string( "enabled re-expansion"
                            , outer ? outer->name : "<arg>");
            } else {
                replacing[ rescan_level-1].read_over = READ_OVER;
            }
        }
    }

    catbuf = xmalloc( (size_t) (NMACWORK + IDMAX));
    if (mcpp_debug & EXPAND) {
        mcpp_fprintf( DBG, "(%s)", defp->name);
        dump_string( "prescan entry", defp->repl);
    }
    if (prescan( defp, (const char **) arglist, catbuf, catbuf + NMACWORK)
            == FALSE) {             /* Process #, ## operators      */
        diag_macro( CERROR, macbuf_overflow, defp->name, 0L, catbuf, defp
                , NULL);
        if (nargs >= 0) {
            if (! enable_trace_macro)
                /* arglist[0] is needed for macro infs  */
                free( arglist[ 0]);
            free( arglist);
        }
        free( catbuf);
        return  NULL;
    }
    catbuf = xrealloc( catbuf, strlen( catbuf) + 1);
                                            /* Use memory sparingly */
    if (mcpp_debug & EXPAND) {
        mcpp_fprintf( DBG, "(%s)", defp->name);
        dump_string( "prescan exit", catbuf);
    }

    if (nargs > 0) {    /* Function-like macro with any argument    */
        expbuf = xmalloc( (size_t) (NMACWORK + IDMAX));
        if (mcpp_debug & EXPAND) {
            mcpp_fprintf( DBG, "(%s)", defp->name);
            dump_string( "substitute entry", catbuf);
        }
        out_p = substitute( defp, (const char **) arglist, catbuf, expbuf
                , expbuf + NMACWORK);   /* Expand each arguments    */
        if (! enable_trace_macro)
            free( arglist[ 0]);
        free( arglist);
        free( catbuf);
        expbuf = xrealloc( expbuf, strlen( expbuf) + 1);
                                            /* Use memory sparingly */
        if (mcpp_debug & EXPAND) {
            mcpp_fprintf( DBG, "(%s)", defp->name);
            dump_string( "substitute exit", expbuf);
        }
    } else {                                /* Object-like macro or */
        if (nargs == 0 && ! enable_trace_macro)
                            /* Function-like macro with no argument */
            free( arglist[ 0]);
        free( arglist);
        out_p = expbuf = catbuf;
    }

    if (out_p)
        out_p = rescan( defp, expbuf, cur_out, out_end);
    if (out_p && defp->nargs == DEF_PRAGMA)
        has_pragma = TRUE;
                    /* Inform mcpp_main() that _Pragma() was found  */
    free( expbuf);
    if (enable_trace_macro && out_p)
        out_p = close_macro_inf( out_p, m_num, in_src_n);
    if (mcpp_debug & EXPAND)
        dump_string( "replace exit", out);

    if (trace_macro && defp->nargs == DEF_PRAGMA) {
        /* Remove intervening magics if the macro is _Pragma pseudo-macro   */
        /* These magics have been inserted by macros in _Pragma()'s args    */
        int     c;
        cur_out = out_p = out;
        while ((c = *cur_out++) != EOS) {
            if (c == MAC_INF) {
                if (! option_flags.v) {
                    switch (*cur_out) {
                    case MAC_ARG_START  :
                        cur_out++;
                        /* Fall through */
                    case MAC_CALL_START :
                        cur_out++;
                        cur_out++;
                        /* Fall through */
                    default:
                        cur_out++;
                        break;
                    }
                } else {
                    switch (*cur_out) {
                    case MAC_ARG_START  :
                    case MAC_ARG_END    :
                        cur_out++;
                        /* Fall through */
                    default:
                        cur_out += 3;
                        break;
                    }
                }
            } else {
                *out_p++ = c;
            }
        }
        *out_p = EOS;
    }

    return  out_p;
}

static char *   close_macro_inf(
    char *  out_p,                      /* Current output pointer   */
    int     m_num,                      /* 'mac_num' of this macro  */
    int     in_src_n                    /* Location of macro in arg */
)
/*
 * Mark up closing of a macro expansion.
 * Note that 'm_num' argument is necessary rather than 'm_inf' from replace(),
 * because mac_inf[] may have been reallocated while rescanning.
 */
{
    MACRO_INF * m_inf;
    LINE_COL    e_line_col;

    m_inf = & mac_inf[ m_num];
    *out_p++ = MAC_INF;         /* Magic for end of macro expansion */
    *out_p++ = MAC_CALL_END;
    if (option_flags.v) {
        *out_p++ = (m_num / UCHARMAX) + 1;
        *out_p++ = (m_num % UCHARMAX) + 1;
    }
    *out_p = EOS;
    get_ch();                               /* Clear the garbage    */
    unget_ch();
    if (infile->fp || in_src_n) {
        if (infile->fp) {           /* Macro call on source file    */
            e_line_col.line = src_line;
            e_line_col.col = infile->bptr - infile->buffer;
        } else {    /* Macro in argument of parent macro and from source    */
            e_line_col.line = in_src[ in_src_n].end_line;
            e_line_col.col = in_src[ in_src_n].end_col;
        }
        /* Get the location before line splicing by <backslash><newline>    */
        /*      or by a line-crossing comment                       */
        get_src_location( & e_line_col);
        m_inf->locs.end_line = e_line_col.line;
        m_inf->locs.end_col = e_line_col.col;
    } else {
        m_inf->locs.end_col = m_inf->locs.end_line = 0L;
    }

    return  out_p;
}

static DEFBUF * def_special(
    DEFBUF *    defp                        /* Macro definition     */
)
/*
 * Re-define __LINE__, __FILE__.
 * Return the new definition.
 */
{
    const FILEINFO *    file;
    DEFBUF **   prevp;
    int         cmp;

    switch (defp->nargs) {
    case DEF_NOARGS_DYNAMIC - 1:            /* __LINE__             */
        if ((src_line > std_limits.line_num || src_line <= 0)
                && (warn_level & 1))
            diag_macro( CWARN
                    , "Line number %.0s\"%ld\" is out of range"     /* _W1_ */
                    , NULL, src_line, NULL, defp, NULL);
        sprintf( defp->repl, "%ld", src_line);      /* Re-define    */
        break;
    case DEF_NOARGS_DYNAMIC - 2:            /* __FILE__             */
        for (file = infile; file != NULL; file = file->parent) {
            if (file->fp != NULL) {
                sprintf( work_buf, "\"%s\"", file->filename);
                if (str_eq( work_buf, defp->repl))
                    break;                          /* No change    */
                defp->nargs = DEF_NOARGS;   /* Enable to redefine   */
                prevp = look_prev( defp->name, &cmp);
                defp = install_macro( "__FILE__", DEF_NOARGS_DYNAMIC - 2, ""
                        , work_buf, prevp, cmp, 0); /* Re-define    */
                break;     
            }
        }
        break;
    }
    return  defp;
}

static int  prescan(
    const DEFBUF *  defp,           /* Definition of the macro      */
    const char **   arglist,        /* Pointers to actual arguments */
    char *      out,                /* Output buffer                */
    char *      out_end             /* End of output buffer         */
)
/*
 * Concatenate the tokens surounding ## by catenate(), and stringize the
 * argument following # by stringize().
 */
{
    FILEINFO *  file;
    char *      prev_token = NULL;  /* Preceding token              */
    char *      horiz_space = NULL; /* Horizontal white space       */
    int         c;                  /* Value of a character         */
    /*
     * The replacement lists are --
     *          stuff1<SEP>stuff2
     *      or  stuff1<SEP>stuff2<SEP>stuff3...
     * where <SEP> is CAT, maybe with preceding space and following space,
     * stuff might be
     *          ordinary-token
     *          MAC_PARM<n>
     *      or  <QUO>MAC_PARM<n>
     * where <QUO> is ST_QUO, possibly with following space.
     *
     * DEF_MAGIC may has been inserted sometimes.
     * In other than POST_STD modes, TOK_SEP and IN_SRC may have been
     * inserted, and TOK_SEPs are inserted also in this routine.
     * In trace_macro mode, many magic character sequences may have been
     * inserted here and there.
     */

    if (mcpp_mode == POST_STD) {
        file = unget_string( defp->repl, defp->name);
    } else {
        *out++ = TOK_SEP;                   /* Wrap replacement     */
        workp = work_buf;                   /*  text with token     */
        workp = stpcpy( workp, defp->repl); /*   separators to      */
        *workp++ = TOK_SEP;                 /*    prevent unintended*/
        *workp = EOS;                       /*     token merging.   */
        file = unget_string( work_buf, defp->name);
    }

    while (c = get_ch(), file == infile) {  /* To the end of repl   */

        switch (c) {
        case ST_QUOTE:
            skip_ws();      /* Skip spaces and the returned MAC_PARM*/
            c = get_ch() - 1;               /* Parameter number     */
            prev_token = out;               /* Remember the token   */
            out = stringize( defp, arglist[ c], out);
                                    /* Stringize without expansion  */
            horiz_space = NULL;
            break;
        case CAT:
            if (*prev_token == DEF_MAGIC || *prev_token == IN_SRC) {
                /* Rare case yet possible after catenate()  */
                size_t  len = 1;
                /* Remove trailing white space prior to removing DEF_MAGIC  */
                if (horiz_space == out - 1) {
                    *--out = EOS;
                    horiz_space = NULL;
                }
                if (*prev_token == IN_SRC && trace_macro)
                    len = IN_SRC_LEN;
                memmove( prev_token, prev_token + len
                        , strlen( prev_token + len));
                out -= len;
                *out = EOS;             /* Remove DEF_MAGIC, IN_SRC */
            }
#if COMPILER == GNUC
            if (*prev_token == ',')
                break;      /* ', ##' sequence (peculiar to GCC)    */
#endif
            if (horiz_space == out - 1) {
                *--out = EOS;       /* Remove trailing white space  */
                horiz_space = NULL;
            }
            out = catenate( defp, arglist, out, out_end, &prev_token);
            if (char_type[ *(out - 1) & UCHARMAX] & HSP)
                horiz_space = out - 1;      /* TOK_SEP has been appended    */
            break;
        case MAC_PARM:
            prev_token = out;
            *out++ = MAC_PARM;
            *out++ = get_ch();              /* Parameter number     */
            break;
        case TOK_SEP:
        case ' ':
        case '\t'   :
            if (out - 1 == horiz_space)
                continue;                   /* Squeeze white spaces */
            horiz_space = out;
            *out++ = c;
            break;
        default:
            prev_token = out;
            scan_token( c, &out, out_end);  /* Ordinary token       */
            break;
        }

        *out = EOS;                         /* Ensure termination   */
        if (out_end <= out)                 /* Buffer overflow      */
            return  FALSE;
    }

    *out = EOS;         /* Ensure terminatation in case of no token */
    unget_ch();
    return  TRUE;
}

static char *   catenate(
    const DEFBUF *  defp,           /* The macro definition         */
    const char **   arglist,        /* Pointers to actual arguments */
    char *  out,                    /* Output buffer                */
    char *  out_end,                /* End of output buffer         */
    char ** token_p         /* Address of preceding token pointer   */
)
/*
 * Concatenate the previous and the following tokens.
 *   Note: The parameter codes may coincide with white spaces or any
 * other characters.
 */
{
    FILEINFO *      file;
    char *  prev_prev_token = NULL;
    const char *    invalid_token
    = "Not a valid preprocessing token \"%s\"";     /* _E_ _W2_     */
    const char *    argp;           /* Pointer to an actual argument*/
    char *  prev_token = *token_p;  /* Preceding token              */
    int     in_arg = FALSE;
    int     c;                      /* Value of a character         */

    /* Get the previous token   */
    if (*prev_token == MAC_PARM) {          /* Formal parameter     */
        c = (*(prev_token + 1) & UCHARMAX) - 1;     /* Parm number  */
        argp = arglist[ c];                 /* Actual argument      */
        out = prev_token;                   /* To overwrite         */
        if (trace_macro)
            argp = remove_magics( argp, TRUE);  /* Remove pair of magics    */
        if ((mcpp_mode == POST_STD && *argp == EOS)
                || (mcpp_mode == STD && *argp == RT_END)) {
            *out = EOS;                     /* An empty argument    */
        } else {
            if (mcpp_mode == POST_STD) {
                file = unget_string( argp, NULL);
                while (c = get_ch(), file == infile) {
                    prev_token = out;   /* Remember the last token  */
                    scan_token( c, &out, out_end);
                }       /* Copy actual argument without expansion   */
                unget_ch();
            } else {
                unget_string( argp, NULL);
                if (trace_macro)
                    free( (char *) argp);
                    /* malloc()ed in remove_magics()    */
                while ((c = get_ch()) != RT_END) {
                    prev_prev_token = prev_token;
                    prev_token = out;   /* Remember the last token  */
                    scan_token( c, &out, out_end);
                }       /* Copy actual argument without expansion   */
                if (*prev_token == TOK_SEP) {
                    out = prev_token;
                    prev_token = prev_prev_token;       /* Skip separator   */
                }
            }
            if (*prev_token == DEF_MAGIC 
                    || (mcpp_mode == STD && *prev_token == IN_SRC)) {
                size_t  len = 1;
                if (trace_macro && *prev_token == IN_SRC)
                    len = IN_SRC_LEN;
                memmove( prev_token, prev_token + len
                        , (size_t) ((out -= len) - prev_token));
                /* Remove DEF_MAGIC enabling the name to replace later      */
            }
        }
    }   /* Else the previous token is an ordinary token, not an argument    */

    c = skip_ws();

    /* Catenate */
    switch (c) {
    case ST_QUOTE:          /* First stringize and then catenate    */
        skip_ws();                  /* Skip MAC_PARM, ST_QUOTE      */
        c = get_ch() - 1;
        out = stringize( defp, arglist[ c], out);
        break;
    case MAC_PARM:
        c = get_ch() - 1;                   /* Parameter number     */
        argp = arglist[ c];                 /* Actual argument      */
        if (trace_macro)
            argp = remove_magics( argp, FALSE); /* Remove pair of magics    */
        if ((mcpp_mode == POST_STD && *argp == EOS)
                || (mcpp_mode == STD && *argp == RT_END)) {
            *out = EOS;                     /* An empty argument    */
        } else {
            unget_string( argp, NULL);
            if (trace_macro)
                free( (char *) argp);
            if ((c = get_ch()) == DEF_MAGIC) {  /* Remove DEF_MAGIC */
                c = get_ch();               /*  enabling to replace */
            } else if (c == IN_SRC) {       /* Remove IN_SRC        */
                if (trace_macro) {
                    get_ch();               /* Also its number      */
                    get_ch();
                }
                c = get_ch();
            }
            scan_token( c, &out, out_end);  /* The first token      */
            if (*infile->bptr)              /* There are more tokens*/
                in_arg = TRUE;
        }
        break;
    case IN_SRC:
        if (trace_macro) {
            get_ch();
            get_ch();
        }
        /* Fall through */
    case DEF_MAGIC:
        c = get_ch();                   /* Skip DEF_MAGIC, IN_SRC   */
        /* Fall through */
    default:
        scan_token( c, &out, out_end);      /* Copy the token       */
        break;
    }

    /* The generated sequence is a valid preprocessing-token ?      */
    if (*prev_token) {                      /* There is any token   */
        unget_string( prev_token, NULL);    /* Scan once more       */
        c = get_ch();  /* This line should be before the next line. */
        infile->fp = (FILE *)-1;            /* To check token length*/
        if (mcpp_debug & EXPAND)
            dump_string( "checking generated token", infile->buffer);
        scan_token( c, (workp = work_buf, &workp), work_end);
        infile->fp = NULL;
        if (*infile->bptr != EOS) {         /* More than a token    */
            if (option_flags.lang_asm) {    /* Assembler source     */
                if (warn_level & 2)
                    diag_macro( CWARN, invalid_token, prev_token, 0L, NULL
                            , defp, NULL);
            } else {
                diag_macro( CERROR, invalid_token, prev_token, 0L, NULL, defp
                       , NULL);
            }
            infile->bptr += strlen( infile->bptr);
        }
        get_ch();                           /* To the parent "file" */
        unget_ch();
    }

    if (mcpp_mode == STD && ! option_flags.lang_asm) {
        *out++ = TOK_SEP;                   /* Prevent token merging*/
        *out = EOS;
    }
    if (in_arg) {       /* There are more tokens after the generated one    */
        if (mcpp_mode == POST_STD) {
            file = infile;
            while (c = get_ch(), file == infile) {
                prev_token = out;       /* Remember the last token  */
                scan_token( c, &out, out_end);
            }           /* Copy rest of argument without expansion  */
            unget_ch();
        } else {
            while ((c = get_ch()) != RT_END) {
                if (c == TOK_SEP)
                    continue;           /* Skip separator           */
                prev_token = out;       /* Remember the last token  */
                scan_token( c, &out, out_end);
            }           /* Copy rest of argument without expansion  */
        }
    }
    *token_p = prev_token;          /* Report back the last token   */

    return  out;
}

static const char *     remove_magics(
    const char *    argp,       /* The argument list    */
    int     from_last           /* token is the last or first?  */
)
/*
 * Remove pair of magic character sequences in an argument in order to catenate
 * the last or first token to another.
 * Or remove pair of magic character sequences surrounding an argument in order
 * to keep symmetry of magics.
 */
{
#define INIT_MAGICS     128

    char    (* mac_id)[ MAC_S_LEN];
    char    (* arg_id)[ ARG_S_LEN];
    char ** mac_loc;
    char ** arg_loc;
    char *  mgc_index;
    size_t  max_magics;
    int     mac_n, arg_n, ind, n;
    char *  first = NULL;
    char *  last = NULL;
    char *  token;
    char *  arg_p;
    char *  ap;
    char *  ep;
    char *  tp;
    char *  space = NULL;
    int     with_rtend;
    int     c;
    FILEINFO *  file;

    mac_id = (char (*)[ MAC_S_LEN]) xmalloc( MAC_S_LEN * INIT_MAGICS);
    arg_id = (char (*)[ ARG_S_LEN]) xmalloc( ARG_S_LEN * INIT_MAGICS * 2);
    mac_loc = (char **) xmalloc( sizeof (char *) * INIT_MAGICS);
    arg_loc = (char **) xmalloc( sizeof (char *) * INIT_MAGICS * 2);
    mgc_index = xmalloc( INIT_MAGICS * 3);
    max_magics = INIT_MAGICS;

    mac_n = arg_n = ind = 0;
    ap = arg_p = xmalloc( strlen( argp) + 1);
    strcpy( arg_p, argp);
    ep = arg_p + strlen( arg_p);
    if (*(ep - 1) == RT_END) {
        with_rtend = TRUE;
        ep--;                               /* Point to RT_END      */
    } else {
        with_rtend = FALSE;
    }
    file = unget_string( arg_p, NULL);  /* Stack to "file" for token parsing*/

    /* Search all the magics in argument, as well as first and last token   */
    /* Read stacked arg_p and write it to arg_p as a dummy buffer   */
    while ((*ap++ = c = get_ch()) != RT_END && file == infile) {
        if (c == MAC_INF) {
            if (mac_n >= max_magics || arg_n >= max_magics * 2) {
                max_magics *= 2;
                mac_id = (char (*)[ MAC_S_LEN]) xrealloc( (void *) mac_id
                        , MAC_S_LEN * max_magics);
                arg_id = (char (*)[ ARG_S_LEN]) xrealloc( (void *) arg_id
                        , ARG_S_LEN * max_magics * 2);
                mac_loc = (char **) xrealloc( (void *) mac_loc
                        , sizeof (char *) * max_magics);
                arg_loc = (char **) xrealloc( (void *) arg_loc
                        , sizeof (char *) * max_magics * 2);
                mgc_index = xrealloc( mgc_index, max_magics * 3);
            }
            *ap++ = c = get_ch();
            switch (c) {
            case MAC_CALL_START :
                *ap++ = get_ch();
                *ap++ = get_ch();
                mac_loc[ mac_n] = ap - MAC_S_LEN;   /* Location of the seq  */
                memcpy( mac_id[ mac_n], ap - (MAC_S_LEN - 1), MAC_S_LEN - 1);
                        /* Copy the sequence from its second byte   */
                mac_id[ mac_n++][ MAC_S_LEN - 1] = FALSE;
                                    /* Mark of to-be-removed or not */
                break;
            case MAC_ARG_START  :
                *ap++ = get_ch();
                *ap++ = get_ch();
                *ap++ = get_ch();
                arg_loc[ arg_n] = ap - ARG_S_LEN;
                memcpy( arg_id[ arg_n], ap - (ARG_S_LEN - 1), ARG_S_LEN - 1);
                arg_id[ arg_n++][ ARG_S_LEN - 1] = FALSE;
                break;
            case MAC_CALL_END   :
                mac_loc[ mac_n] = ap - MAC_E_LEN;
                mac_id[ mac_n][ 0] = c;
                mac_id[ mac_n++][ MAC_E_LEN_V - 1] = FALSE;
                break;
            case MAC_ARG_END    :
                arg_loc[ arg_n] = ap - ARG_E_LEN;
                arg_id[ arg_n][ 0] = c;
                arg_id[ arg_n++][ ARG_E_LEN_V - 1] = FALSE;
                break;
            }
            if (option_flags.v) {
                switch (c) {
                case MAC_CALL_END   :
                    mac_id[ mac_n - 1][ 1] = *ap++ = get_ch();
                    mac_id[ mac_n - 1][ 2] = *ap++ = get_ch();
                    break;
                case MAC_ARG_END    :
                    arg_id[ arg_n - 1][ 1] = *ap++ = get_ch();
                    arg_id[ arg_n - 1][ 2] = *ap++ = get_ch();
                    arg_id[ arg_n - 1][ 3] = *ap++ = get_ch();
                    break;
                }
            }
            mgc_index[ ind++] = c;      /* Index to mac_id[] and arg_id[]   */
            continue;
        } else if (char_type[ c & UCHARMAX] & HSP) {
            if (! first) {
                ap--;   /* Skip white space on top of the argument  */
                ep--;
            }
            continue;
        }
        last = --ap;
        if (! first)
            first = ap;
        if (char_type[ c & UCHARMAX] & HSP)
            space = ap;         /* Remember the last white space    */
        scan_token( c, &ap, ep);
    }
    if (file == infile)
        get_ch();                               /* Clear the "file" */
    unget_ch();
    if (space == ep - 1)
        ep--;                       /* Remove trailing white space  */
    if (with_rtend)
        *ep++ = RT_END;
    *ep = EOS;
    if ((from_last && !last) || (!from_last && !first))
        return  arg_p;
    if (mac_n == 0 && arg_n == 0)           /* No magic sequence    */
        return  arg_p;
    token = from_last ? last : first;

    /* Remove pair of magics surrounding the last (or first) token  */
    if (mac_n) {
        /* Remove pair of macro magics surrounding the token    */
        int     magic, mac_s, mac_e;
        int     nest_s, nest_e;

        nest_s = 0;
        for (mac_s = 0; mac_loc[ mac_s] < token; mac_s++) {
            magic = mac_id[ mac_s][ 0];
            if (magic == MAC_CALL_START) {      /* Starting magic   */
                nest_e = ++nest_s;
                /* Search the corresponding closing magic   */
                for (mac_e = mac_s + 1; mac_e < mac_n; mac_e++) {
                    magic = mac_id[ mac_e][ 0];
                    if (magic == MAC_CALL_START) {
                        nest_e++;
                    } else {       /* MAC_CALL_END: Closing magic   */
                        nest_e--;
                        /* Search after the token   */
                        if (token < mac_loc[ mac_e] && nest_e == nest_s - 1) {
#if DEBUG_MACRO_ANN
                            if (option_flags.v)
                                chk_symmetry( mac_id[ mac_s], mac_id[ mac_e]
                                        , MAC_E_LEN - 2);
#endif
                            mac_id[ mac_e][ MAC_S_LEN - 1] = TRUE;
                                                /* To be removed    */
                            break;          /* Done for this mac_s  */
                        }
                    }
                }
                if (mac_e < mac_n)  /* Found corresponding magic    */
                    mac_id[ mac_s][ MAC_S_LEN - 1] = TRUE;  /* To be removed*/
                else                                /* Not found    */
                    break;
            } else {
                nest_s--;           /* MAC_CALL_END: Closing magic  */
            }
        }
    }
    if (arg_n) {
        /* Remove pair of arg magics surrounding the token  */
        int     magic, arg_s, arg_e;
        int     nest_s, nest_e;

        nest_s = 0;
        for (arg_s = 0; arg_loc[ arg_s] < token; arg_s++) {
            magic = arg_id[ arg_s][ 0];
            if (magic == MAC_ARG_START) {
                nest_e = ++nest_s;
                for (arg_e = arg_s + 1; arg_e < arg_n; arg_e++) {
                    magic = arg_id[ arg_e][ 0];
                    if (magic == MAC_ARG_START) {
                        nest_e++;
                    } else {
                        nest_e--;
                        if (token < arg_loc[ arg_e] && nest_e == nest_s - 1) {
#if DEBUG_MACRO_ANN
                            if (option_flags.v)
                                chk_symmetry( arg_id[ arg_s], arg_id[ arg_e]
                                        , ARG_E_LEN_V - 2);
#endif
                            arg_id[ arg_e][ ARG_S_LEN - 1] = TRUE;
                            break;
                        }
                    }
                }
                if (arg_e < arg_n)
                    arg_id[ arg_s][ ARG_S_LEN - 1] = TRUE;
                else
                    break;
            } else {
                nest_s--;
            }
        }
    }

    /* Copy the sequences skipping the to-be-removed magic seqs */
    file = unget_string( arg_p, NULL);  /* Stack to "file" for token parsing*/
    tp = arg_p;
    ep = arg_p + strlen( arg_p);
    mac_n = arg_n = n = 0;

    while ((*tp++ = c = get_ch()) != RT_END && file == infile) {
        char ** loc_tab;
        int     num, mark, rm, magic;
        size_t  len;

        if (c != MAC_INF) {
            scan_token( c, (--tp, &tp), ep);
            continue;
        }
        unget_ch();                             /* Pushback MAC_INF */
        tp--;

        switch (magic = mgc_index[ n++]) {
        case MAC_CALL_START :
            len = MAC_S_LEN;
            mark = MAC_S_LEN - 1;
            break;
        case MAC_CALL_END   :
            len = option_flags.v ? MAC_E_LEN_V : MAC_E_LEN;
            mark = MAC_E_LEN_V - 1;
            break;
        case MAC_ARG_START  :
            len = ARG_S_LEN;
            mark = ARG_S_LEN - 1;
            break;
        case MAC_ARG_END    :
            len = option_flags.v ? ARG_E_LEN_V : ARG_E_LEN;
            mark = ARG_E_LEN_V - 1;
            break;
        }
        switch (magic) {
        case MAC_CALL_START :
        case MAC_CALL_END   :
            loc_tab = mac_loc;
            num = mac_n;
            rm = mac_id[ mac_n++][ mark];
            break;
        case MAC_ARG_START  :
        case MAC_ARG_END    :
            loc_tab = arg_loc;
            num = arg_n;
            rm = arg_id[ arg_n++][ mark];
            break;
        }
        if (rm == FALSE) {                  /* Not to be removed    */
            memmove( tp, loc_tab[ num], len);
                    /* Copy it (from arg_p buffer for convenience)  */
            tp += len;
        }
        infile->bptr += len;
    }
    if (! with_rtend)
        tp--;
    *tp = EOS;
    if (file == infile)
        get_ch();                               /* Clear the "file" */
    unget_ch();

    return  arg_p;
}

#if DEBUG_MACRO_ANN
static void     chk_symmetry(
    char *  start_id,   /* Sequence of macro (or arg) starting inf  */
    char *  end_id,     /* Sequence of macro (or arg) closing inf   */
    size_t  len                         /* Length of the sequence   */
)
/*
 * Check whether starting sequence and corresponding closing sequence is the
 * same.
 */
{
    int     s_id, e_id, arg_s_n, arg_e_n;

    if (memcmp( start_id + 1, end_id + 1, len) == 0)
        return;                     /* The sequences are the same   */
    s_id = ((start_id[ 1] & UCHARMAX) - 1) * UCHARMAX;
    s_id += (start_id[ 2] & UCHARMAX) - 1;
    e_id = ((end_id[ 1] & UCHARMAX) - 1) * UCHARMAX;
    e_id += (end_id[ 2] & UCHARMAX) - 1;
    if (len >= 3) {
        arg_s_n = (start_id[ 3] & UCHARMAX) - 1;
        arg_e_n = (end_id[ 3] & UCHARMAX) - 1;
        mcpp_fprintf( ERR,
"Asymmetry of arg inf found removing magics: start %d:%d, end: %d:%d at line:%d\n"
                , s_id, arg_s_n, e_id, arg_e_n, src_line);
    } else {
        mcpp_fprintf( ERR,
"Asymmetry of macro inf found removing magics: start %d, end: %d at line:%d\n"
                , s_id, e_id, src_line);
    }
}
#endif

static char *   stringize(
    const DEFBUF *  defp,                   /* The macro definition */
    const char *    argp,                   /* Pointer to argument  */
    char *      out                         /* Output buffer        */
)
/*
 * Make a string literal from an argument.
 */
{
    char        arg_end_inf[ 8][ ARG_E_LEN_V - 1];
                        /* Verbose information of macro arguments   */
    FILEINFO *  file;
    int         stray_bsl = FALSE;          /* '\\' not in literal  */
    char *      out_p = out;
    int         token_type;
    int         num_arg_magic = 0;
    size_t      len;
    size_t      arg_e_len = option_flags.v ? ARG_E_LEN_V : ARG_E_LEN;
    int         c;

    if (trace_macro) {
        while ((*argp == MAC_INF && *(argp + 1) == MAC_ARG_START)
            /* Argument is prefixed with macro tracing magics   */
                || (char_type[ *argp & UCHARMAX] & HSP)) {
            if (*argp == MAC_INF) {     /* Move magics to outside of string */
                memcpy( out_p, argp, ARG_S_LEN);
                out_p += ARG_S_LEN;
                argp += ARG_S_LEN;
                num_arg_magic++;
            } else {                        /* Skip white spaces    */
                argp++;
            }
        }
    }

    file = unget_string( argp, NULL);
    len = strlen( infile->buffer);  /* Sequence ends with RT_END    */
    if (trace_macro) {          /* Remove suffixed argument closing magics  */
        /* There are 0 or more argument closing magic sequences and */
        /* 0 or more TOK_SEPs and no space at the end of argp.      */
        /* This is assured by get_an_arg().                         */
        int         nmagic = 0;
        while (len > arg_e_len
            && (((*(infile->buffer + len - arg_e_len - 1) == MAC_INF
                    && *(infile->buffer + len - arg_e_len) == MAC_ARG_END)
                || *(infile->buffer + len - 2) == TOK_SEP))) {
            if (*(infile->buffer + len - arg_e_len - 1) == MAC_INF
                    && *(infile->buffer + len - arg_e_len) == MAC_ARG_END) {
                if (option_flags.v) {
                    memcpy( arg_end_inf[ nmagic]
                            , infile->buffer + len - arg_e_len + 1
                            , arg_e_len - 2);
                    arg_end_inf[ nmagic][ arg_e_len - 2] = EOS;
                }
                nmagic++;
                len -= arg_e_len;
                *(infile->buffer + len - 1) = RT_END;
                *(infile->buffer + len) = EOS;
            } else if (*(infile->buffer + len - 2) == TOK_SEP) {
                len--;
                *(infile->buffer + len - 1) = RT_END;
                *(infile->buffer + len) = EOS;
            }
        }
        if (nmagic != num_arg_magic) {  /* There are some imbalances    */
            /* Some surrounding magics correspond to intervening ones.  */
            /* So, unmatched surrounding magics should be removed.      */
            if (num_arg_magic > nmagic) {
                num_arg_magic = nmagic;     /* Ignore the surplus   */
                out_p = out + ARG_S_LEN * num_arg_magic;
            }   /* Else simply ignore the surplus nmagic    */
        }
    }
    *out_p++ = '"';                         /* Starting quote       */

    while ((c = get_ch()), ((mcpp_mode == POST_STD && file == infile)
            || (mcpp_mode == STD && c != RT_END))) {
        if (c == ' ' || c == '\t') {
            *out_p++ = c;
            continue;
        } else if (c == TOK_SEP) {
            continue;                   /* Skip inserted separator  */
        } else if (c == IN_SRC) {           /* Skip magics          */
            if (trace_macro) {
                get_ch();
                get_ch();
            }
            continue;
        } else if (c == '\\') {
            stray_bsl = TRUE;               /* May cause a trouble  */
        } else if (c == MAC_INF) {  /* Remove intervening magics    */
            switch (c = get_ch()) {
            case MAC_ARG_START  :
                get_ch();
                /* Fall through */
            case MAC_CALL_START :
                get_ch();
                get_ch();
                break;
            }
            if (option_flags.v) {
                switch (c) {
                case MAC_ARG_END    :
                    get_ch();
                    /* Fall through */
                case MAC_CALL_END   :
                    get_ch();
                    get_ch();
                    break;
                }
            }
            continue;
        }
        token_type = scan_token( c, (workp = work_buf, &workp), work_end);

        switch (token_type) {
        case WSTR:
        case WCHR:
        case STR:
        case CHR:
            workp = work_buf;
            while ((c = *workp++ & UCHARMAX) != EOS) {
                if (char_type[ c] & mbchk) {        /* Multi-byte character */
                    mb_read( c, &workp, (*out_p++ = c, &out_p));
                                            /* Copy as it is        */
                    continue;
                } else if (c == '"') {
                    *out_p++ = '\\';        /* Insert '\\'          */
                } else if (c == '\\') {
#if OK_UCN
                    if (mcpp_mode == POST_STD || ! stdc3
                            || (*workp != 'u' && *workp != 'U'))
                                            /* Not UCN              */
#endif
                        *out_p++ = '\\';
                }
                *out_p++ = c;
            }
            *out_p = EOS;
            break;
        default:
            out_p = stpcpy( out_p, work_buf);
            break;
        }
    }

    if (mcpp_mode == POST_STD)
        unget_ch();
    *out_p++ = '"';                         /* Closing quote        */
    if (trace_macro) {
        while (num_arg_magic--) {
            *out_p++ = MAC_INF;             /* Restore removed magic*/
            *out_p++ = MAC_ARG_END;
            if (option_flags.v)
                out_p = stpcpy( out_p, arg_end_inf[ num_arg_magic]);
        }
    }
    *out_p = EOS;

    if (stray_bsl) {    /* '\\' outside of quotation has been found */
        int     invalid = FALSE;
        unget_string( out, defp->name);
        if (mcpp_debug & EXPAND)
            dump_string( "checking generated token", infile->buffer);
        scan_quote( get_ch(), work_buf, work_end, TRUE);
            /* Unterminated or too long string will be diagnosed    */
        if (*infile->bptr != EOS)           /* More than a token    */
            invalid = TRUE; /* Diagnose after clearing the "file"   */
        infile->bptr += strlen( infile->bptr);
        get_ch();                           /* Clear the "file"     */
        unget_ch();
        if (invalid)
            diag_macro( CERROR
                    , "Not a valid string literal %s"       /* _E_  */
                    , out, 0L, NULL, defp, NULL);
    }
#if NWORK-2 > SLEN90MIN
    else if ((warn_level & 4) && out_p - out > std_limits.str_len)
        diag_macro( CWARN
                , "String literal longer than %.0s%ld bytes %s"     /* _W4_ */
                , NULL , (long) std_limits.str_len, out, defp, NULL);
#endif
    return  out_p;
}

static char *   substitute(
    const DEFBUF *  defp,           /* The macro getting arguments  */
    const char **   arglist,    /* Pointers to actual arguments     */
    const char *    in,                     /* Replacement text     */
    char *      out,                        /* Output buffer        */
    char *      out_end                     /* End of output buffer */
)
/*
 * Replace completely each actual arguments of the macro, and substitute for
 * the formal parameters in the replacement list.
 */
{
    char *  out_start = out;
    const char *    arg;
    int     c;
    int     gvar_arg;   /* gvar_arg'th argument is GCC variable argument    */

    gvar_arg = (defp->nargs & GVA_ARGS) ? (defp->nargs & ~AVA_ARGS) : 0;
    *out = EOS;                             /* Ensure to termanate  */

    while ((c = *in++) != EOS) {
        if (c == MAC_PARM) {                /* Formal parameter     */
            c = *in++ & UCHARMAX;           /* Parameter number     */
            if (mcpp_debug & EXPAND) {
                mcpp_fprintf( DBG, " (expanding arg[%d])", c);
                dump_string( NULL, arglist[ c - 1]);
            }
#if COMPILER == GNUC || COMPILER == MSC
            arg = arglist[ c - 1];
            if (trace_macro) {
                if (*arg == MAC_INF) {
                    if (*++arg == MAC_ARG_START)
                        arg += ARG_S_LEN - 1;       /* Next to magic chars  */
                }
            }
#if COMPILER == GNUC
            if (c == gvar_arg && *arg == RT_END && ! ansi) {
                /*
                 * GCC variadic macro and its variable argument is absent.
                 * Note that in its "strict-ansi" mode GCC does not remove 
                 * ',', nevertheless it ignores '##' (inconsistent
                 * behavior).  Though GCC2 changes behavior depending the
                 * ',' is preceded by space or not, we only count on the
                 * "strict-ansi" flag.
                 */
#else
            if ((defp->nargs & VA_ARGS) && c == (defp->nargs & ~VA_ARGS)
                    && *arg == RT_END && mcpp_mode == STD) {
                /* Visual C 2005 also removes ',' immediately preceding     */
                /* absent variable arguments.  It does not use '##' though. */
#endif
                char *  tmp;
                tmp = out - 1;
                while (char_type[ *tmp & UCHARMAX] & HSP)
                    tmp--;
                if (*tmp == ',') {
                    out = tmp;      /* Remove the immediately preceding ',' */
                    if (warn_level & 1) {
                        *out = EOS;
                        diag_macro( CWARN,
        "Removed ',' preceding the absent variable argument: %s"    /* _W1_ */
                                , out_start, 0L, NULL, defp, NULL);
                    }
                }
            } else
#endif
            if ((out = rescan( NULL, arglist[ c - 1], out, out_end))
                    == NULL) {              /* Replace completely   */
                return  NULL;               /* Error                */
            }
        } else {
            *out++ = c;                     /* Copy the character   */
        }
    }
    *out = EOS;
    return  out;
}

static char *   rescan(
    const DEFBUF *  outer,          /* Outer macro just replacing   */
    const char *    in,             /* Sequences to be rescanned    */
    char *  out,                            /* Output buffer        */
    char *  out_end                         /* End of output buffer */
)
/*
 * Re-scan the once replaced sequences to replace the remaining macros
 * completely.
 * rescan() and replace() call each other recursively.
 *
 * Note: POST_STD mode does not use IN_SRC nor TOK_SEP and seldom uses RT_END.
 * Checking of those are unnecessary overhead for POST_STD mode.  To integrate
 * the code for POST_STD with STD mode, however, we use these checkings
 * commonly.
 * Also compat_mode does not use IN_SRC unless in trace_macro mode.
 * STD mode has macro notification mode (trace_macro mode), too.  Its routines
 * are complicated and not easy to understand.
 */
{
    char *  cur_cp = NULL;
    char *  tp = NULL;              /* Temporary pointer into buffer*/
    char *  out_p = out;            /* Current output pointer       */
    FILEINFO *  file;       /* Input sequences stacked on a "file"  */
    DEFBUF *    inner;              /* Inner macro to replace       */
    int     c;                      /* First character of token     */
    int     token_type;
    char *  mac_arg_start = NULL;
#if COMPILER == GNUC
    int     within_defined = FALSE;
    int     within_defined_arg_depth = 0;
#endif

    if (mcpp_debug & EXPAND) {
        mcpp_fprintf( DBG, "rescan_level--%d (%s) "
                , rescan_level + 1, outer ? outer->name : "<arg>");
        dump_string( "rescan entry", in);
    }
    if (! disable_repl( outer)) /* Don't re-replace replacing macro */
        return  NULL;               /* Too deeply nested macro call */
    if (mcpp_mode == STD) {
        get_ch();                   /* Clear empty "file"s          */
        unget_ch();                 /*      for diagnostic          */
        cur_cp = infile->bptr;      /* Remember current location    */
    }
    file = unget_string( in, outer ? outer->name : NULL);
                                    /* Stack input on a "file"      */

    while ((c = get_ch()), file == infile
        /* Rescanning is limited to the "file"  */
            && c != RT_END) {
            /*
             * This is the trick of STD mode.  collect_args() via replace()
             * may read over to file->parent (provided the "file" is macro)
             * unless stopped by RT_END.
             */
        size_t  len = 0;

        if (char_type[ c] & HSP) {
            *out_p++ = c;
            continue;
        } else if (c == MAC_INF) {              /* Only in STD mode */
            *out_p++ = c;
            *out_p++ = c = get_ch();
            switch (c) {
            case MAC_ARG_START  :
                mac_arg_start = out_p - 2;      /* Remember the position    */
                *out_p++ = get_ch();
                /* Fall through */
            case MAC_CALL_START :
                *out_p++ = get_ch();
                *out_p++ = get_ch();
                break;
            case MAC_ARG_END    :
                if (! option_flags.v)
                    break;
                else
                    *out_p++ = get_ch();
                    /* Fall through */
            case MAC_CALL_END   :
                if (option_flags.v) {
                    *out_p++ = get_ch();
                    *out_p++ = get_ch();
                }
                break;
            }               /* Pass these characters as they are    */
            continue;
        }
        token_type = scan_token( c, (tp = out_p, &out_p), out_end);
#if COMPILER == GNUC
        if (mcpp_mode == STD) {
            /* Pass stuff within defined() as they are, if in_directive */
            if ((within_defined || within_defined_arg_depth)) {
                if (c == '(') {
                    within_defined_arg_depth++;
                    within_defined = FALSE;
                } else if (within_defined_arg_depth && c == ')') {
                    within_defined_arg_depth--;
                }       /* Else should be a name (possibly macro)   */
                continue;
            } else if (token_type == NAM && in_directive
                        && str_eq(identifier, "defined")) {
                within_defined = TRUE;
                            /* 'defined' token in directive line    */
                continue;
            }
        } 
#endif
        if (mcpp_mode == STD && c == IN_SRC)
            len = trace_macro ? IN_SRC_LEN : 1;
        if (token_type == NAM && c != DEF_MAGIC 
                && (inner = look_id( tp + len)) != NULL) {  /* A macro name */
            int     is_able;        /* Macro is not "blue-painted"  */
            char *  endf = NULL;    /* Output stream at end of infile       */
            MAGIC_SEQ   mgc_seq;    /* Magics between macro name and '('    */

            if (trace_macro)
                memset( &mgc_seq, 0, sizeof (MAGIC_SEQ));
            if (is_macro_call( inner, &out_p, &endf
                        , trace_macro ? &mgc_seq : NULL)
                    && ((mcpp_mode == POST_STD && is_able_repl( inner))
                        || (mcpp_mode == STD
                            && (((is_able = is_able_repl( inner)) == YES)
                                || (is_able == READ_OVER 
                                    && (c == IN_SRC || compat_mode)))))) {
                                            /* Really a macro call  */
                LINE_COL    in_src_line_col = { 0L, 0};
                int     in_src_n = 0;

                if (trace_macro) {
                    if (c == IN_SRC) {  /* Macro in argument from source    */
                        /* Get the location in source   */
                        in_src_n = ((*(tp + 1) & UCHARMAX) - 1) * UCHARMAX;
                        in_src_n += (*(tp + 2) & UCHARMAX) - 1;
                        in_src_line_col.line = in_src[ in_src_n].start_line;
                        in_src_line_col.col = in_src[ in_src_n].start_col;
                    }
                    if (inner->nargs >= 0 && mgc_seq.magic_start) {
                        /* Magic sequence is found between macro */
                        /* name and '('.  This is a nuisance.    */
                        char *      mgc_cleared;
                        size_t      seq_len;
                        size_t      arg_elen = option_flags.v ? ARG_E_LEN_V
                                            : ARG_E_LEN;
                        if ((tp - ARG_S_LEN) == mac_arg_start
                                && *mgc_seq.magic_start == MAC_INF
                                && *(mgc_seq.magic_start + 1) == MAC_ARG_END) {
                            /* Name of function-like macro is surrounded by */
                            /* magics, which were inserted by outer macro.  */
                            /* Remove the starting magic. (The closing magic*/
                            /* has already been removed by is_macro_call(). */
                            tp -= ARG_S_LEN;
                            mgc_seq.magic_start += arg_elen;    /* Next seq */
                        }
                        /* Restore once skipped magic sequences,    */
                        /* then remove "pair"s of sequences.        */
                        seq_len = mgc_seq.magic_end - mgc_seq.magic_start;
                        if (seq_len) {
                            insert_to_bptr( mgc_seq.magic_start, seq_len);
                            mgc_cleared = remove_magics(
                                    (const char *) infile->bptr, FALSE);
                                        /* Remove pair of magics    */
                            strcpy( infile->bptr, mgc_cleared);
                            free( mgc_cleared);
                        }
                    }
                }
                if ((out_p = replace( inner, tp, out_end, outer, file
                        , in_src_line_col, in_src_n)) == NULL)
                    break;                  /* Error of macro call  */
            } else {
                if (endf && strlen( endf)) {
                    /* Has read over to parent file: another nuisance.      */
                    /* Restore the read-over sequence into current buffer.  */
                    /* Don't use unget_string() here.                       */
                    insert_to_bptr( endf, out_p - endf);
                    out_p = endf;
                    *out_p = EOS;
                }
                if ((is_able = is_able_repl( inner)) == NO
                        || (mcpp_mode == STD && is_able == READ_OVER
                                && c != IN_SRC && ! compat_mode)) {
                    if (mcpp_mode == POST_STD || c != IN_SRC)
                        memmove( tp + 1, tp, (size_t) (out_p++ - tp));
                    *tp = DEF_MAGIC;        /* Mark not to replace  */
                }                           /* Else not a macro call*/
            }
        }
        if (out_end <= out_p) {
            *out_p = EOS;
            diag_macro( CERROR, macbuf_overflow, outer ? outer->name : in, 0L
                    , out, outer, inner);
            out_p = NULL;
            break;
        }
    }

    if (out_p) {
        *out_p = EOS;
        if (mcpp_mode == STD) {
            if  (c != RT_END) {
                unget_ch();
                if (outer != NULL) {    /* outer isn't a macro in argument  */
                    if (infile && infile->bptr != cur_cp
                                    /* Have overrun replacement list*/
                            && !(tp && *tp == DEF_MAGIC)
                                                /* Macro is enabled */
                            && ((!compat_mode && (warn_level & 1))
                                || (compat_mode && (warn_level & 8)))) {
                        diag_macro( CWARN,
"Replacement text \"%s\" of macro %.0ld\"%s\" involved subsequent text" /* _W1_ */
                            , in, 0L, outer->name, outer, inner);
                    }
                }
            }                       /* Else remove RT_END           */
        } else {
            unget_ch();
        }
    }
    enable_repl( outer, TRUE);      /* Enable macro for later text  */
    if (mcpp_debug & EXPAND) {
        mcpp_fprintf( DBG, "rescan_level--%d (%s) "
                , rescan_level + 1, outer ? outer->name : "<arg>");
        dump_string( "rescan exit", out);
    }
    return  out_p;
}

static int  disable_repl(
    const DEFBUF *  defp
)
/*
 * Register the macro name currently replacing.
 */
{
    if (defp == NULL)
        return  TRUE;
    if (rescan_level >= RESCAN_LIMIT) {
        diag_macro( CERROR,
            "Rescanning macro \"%s\" more than %ld times at \"%s\"" /* _E_  */
                , macro_name, (long) RESCAN_LIMIT, defp->name, defp, NULL);
        return  FALSE;
    }
    replacing[ rescan_level].def = defp;
    replacing[ rescan_level++].read_over = NO;
    return  TRUE;
}

static void enable_repl(
    const DEFBUF *  defp,
    int         done
)
/*
 * Un-register the macro name just replaced for later text.
 */
{
    if (defp == NULL)
        return;
    replacing[ rescan_level - 1].def = NULL;
    if (done && rescan_level)
        rescan_level--;
}

static int  is_able_repl(
    const DEFBUF *  defp
)
/*
 * The macro is permitted to replace ?
 */
{
    int     i;

    if (defp == NULL)
        return  YES;
    for (i = rescan_level-1; i >= 0; i--) {
        if (defp == replacing[ i].def)
            return  replacing[ i].read_over;
    }
    return  YES;
}

static char *   insert_to_bptr(
    char *  ins,            /* Sequence to be inserted  */
    size_t  len             /* Byte to be inserted      */
)
/*
 * Insert a sequence into infile->bptr.
 * infile->buffer is reallocated to ensure buffer size.
 * This routine changes absolute address of infile->bptr, hence rescan() emits
 * a "Replacement text ... involved subsequent text" warning.  Anyway,
 * a macro which needs this routine deserves that warning.
 */
{
    size_t  bptr_offset = infile->bptr - infile->buffer;

    if (infile->fp == NULL) {               /* Not source file      */
        infile->buffer = xrealloc( infile->buffer
                , strlen( infile->buffer) + len + 1);
        infile->bptr = infile->buffer + bptr_offset;
    }
    memmove( infile->bptr + len, infile->bptr, strlen( infile->bptr) + 1);
    memcpy( infile->bptr, ins, len);

    return  infile->buffer;
}

/*
 *  M a c r o   E x p a n s i o n   i n   P R E - S T A N D A R D   M o d e
 */

#include    "setjmp.h"

static jmp_buf  jump;

static char *   arglist_pre[ NMACPARS];     /* Pointers to args     */

static int      rescan_pre( int c, char * mp, char * mac_end);
                /* Replace a macro repeatedly   */
static int      replace_pre( DEFBUF * defp);
                /* Replace a macro once         */
static void     substitute_pre( DEFBUF * defp);
                /* Substitute parms with args   */

static char *   expand_prestd(
    DEFBUF *    defp,                       /* Macro definition     */
    char *  out,                            /* Output buffer        */
    char *  out_end,                        /* End of output buffer */
    LINE_COL    line_col,       /* Location of macro (not used in prestd)   */
    int *   pragma_op           /* Flag of _Pragma (not used in prestd)     */
)
/*
 * Expand a macro call completely, write the results to the specified buffer
 * and return the advanced pointer.
 */
{
    char    macrobuf[ NMACWORK + IDMAX];    /* Buffer for rescan_pre()      */
    char *  mac_end = &macrobuf[ NMACWORK]; /* End of macrobuf[]    */
    char *  out_p;                          /* Pointer into out[]   */
    char *  mp = macrobuf;                  /* Pointer into macrobuf*/
    int     len;                            /* Length of a token    */
    int     token_type;                     /* Type of token        */
    int     c;

    macro_line = src_line;                  /* Line number for diag.*/
    unget_string( identifier, identifier);  /* To re-read           */
    macro_name = defp->name;
    rescan_level = 0;
    if (setjmp( jump) == 1) {
        skip_macro();
        mp = macrobuf;
        *mp = EOS;
        macro_line = MACRO_ERROR;
        goto  err_end;
    }

    while ((c = get_ch()) != CHAR_EOF && infile->fp == NULL) {
                            /* While the input stream is a macro    */
        while (c == ' ' || c == '\t') {     /* Output the spaces    */
            *mp++ = c;
            c = get_ch();
            if (infile == NULL || infile->fp != NULL)
                goto  exp_end;
        }
        token_type = rescan_pre( c, mp, mac_end);   /* Scan token   */
            /*  and expand.  Result of expansion is written at mp.  */

        switch (token_type) {
        case STR:                           /* String literal       */
        case CHR:                           /* Character constant   */
        case NUM:                           /* Number token         */
        case OPE:                           /* Operator or punct.   */
        case NAM:                           /* Identifier           */
            len = strlen( mp);
            mp += len;
            break;
        case SEP:                           /* Special character    */
            switch( *mp) {
            case COM_SEP:
                if (mcpp_mode == OLD_PREP)
                    break;  /* Zero-length comment is removed now   */
                /* Else fall through    */
            default:                        /* Who knows ?          */
                mp++;                       /* Copy the character   */
                break;
            }
            break;
        case NO_TOKEN:  break;              /* End of file          */
        default:                            /* Unkown token char.   */
            mp++;                           /* Copy the character   */
            break;
        }

        if (mac_end <= mp) {
            *mp = EOS;
            cerror( macbuf_overflow, macro_name, 0L, macrobuf);
            longjmp( jump, 1);
        }
        if (mcpp_debug & GETC) {
            *mp = EOS;
            dump_string( "macrobuf", macrobuf);
        }
    }

exp_end:
    unget_ch();
    while (macrobuf < mp && (*(mp - 1) == ' ' || *(mp - 1) == '\t'))
        mp--;                           /* Remove trailing blank    */
    macro_line = 0;
    *mp = EOS;
    if (mp - macrobuf > out_end - out) {
        cerror( macbuf_overflow, macro_name, 0L, macrobuf);
        macro_line = MACRO_ERROR;
    }
err_end:
    out_p = stpcpy( out, macrobuf);
    if (mcpp_debug & EXPAND) {
        dump_string( "expand_prestd exit", out);
    }
    macro_name = NULL;
    clear_exp_mac();
    *pragma_op = FALSE;
    return  out_p;
}

static int  rescan_pre(
    int     c,                      /* First character of token     */
    char *  mp,                     /* Output buffer                */
    char *  mac_end                 /* End of output buffer         */
)
/*
 * If the token is a macro name, replace the macro repeatedly until the first
 * token becomes a non-macro and return the type of token after expansion.
 */
{
    int     token_type;             /* Type of token                */
    char *  cp = mp;        /* Value of mp should not be changed    */
    DEFBUF *    defp;
    FILEINFO *  file;

    while ((token_type = scan_token( c, &cp, mac_end)) == NAM
            && (defp = look_id( identifier)) != NULL) { /* Macro    */
        if (replace_pre( defp) == FALSE)
            break;                  /* Macro name with no argument  */
        file = infile;
        c = get_ch();
        if (file != infile) {       /* Replaced to 0 token          */
            unget_ch();
            token_type = NO_TOKEN;
            break;
        }
        cp = mp;                    /* Overwrite on the macro call  */
    }                               /* The macro call is replaced   */
    return  token_type;
}

static int  replace_pre(
    DEFBUF *    defp                /* Definition of the macro      */
)
/*
 * Replace a macro one level.  Called from expand_prestd() (via rescan_pre())
 * when an identifier is found in the macro table.  It calls collect_args()
 * to parse actual arguments, checking for the correct number.  It then
 * creates a "file" containing single line containing the replacement text
 * with the actual arguments inserted appropriately.  This is "pushed back"
 * onto the input stream.  (When get_ch() routine runs off the end of the macro
 * line, it will dismiss the macro itself.)
 */
{
    int         arg_len;
    int         c;

    if (mcpp_debug & EXPAND) {
        dump_a_def( "replace_pre entry", defp, FALSE, TRUE, fp_debug);
        dump_unget( "replace_pre entry");
    }
    if (++rescan_level >= PRESTD_RESCAN_LIMIT) {
        diag_macro( CERROR
                , "Recursive macro definition of \"%s\""    /* _E_  */
                , defp->name, 0L, NULL, defp, NULL);
        longjmp( jump, 1);
    }

    /*
     * Here's a macro to replace.
     */
    switch (defp->nargs) {
    case DEF_NOARGS:                /* No argument just stuffs      */
    case DEF_NOARGS_PREDEF_OLD:     /* Compiler-specific predef without '_' */
    case DEF_NOARGS_PREDEF:         /* Compiler-specific predef     */
        break;
    default:                                /* defp->nargs >= 0     */
        c = squeeze_ws( NULL, NULL, NULL);  /* Look for and skip '('*/
        if (c != '(') {         /* Macro name without following '(' */
            unget_ch();
            if (warn_level & 8)
                diag_macro( CWARN, only_name, defp->name, 0L, NULL, defp, NULL);
            return  FALSE;
        } else {
            arglist_pre[ 0] = xmalloc( (size_t) (NMACWORK + IDMAX * 2));
            arg_len = collect_args( defp, arglist_pre, 0);
                                            /* Collect arguments    */
            if (arg_len == ARG_ERROR) {     /* End of input         */
                free( arglist_pre[ 0]);
                longjmp( jump, 1);
            }
        }
        break;
    }

    if (defp->nargs > 0)
        substitute_pre( defp);              /* Do actual arguments  */
    else
        unget_string( defp->repl, defp->name);

    if (mcpp_debug & EXPAND)
        dump_unget( "replace_pre exit");
    if (defp->nargs >= 0)
        free( arglist_pre[ 0]);
    return  TRUE;
}

static void substitute_pre(
    DEFBUF *        defp            /* Current macro being replaced */
)
/*
 * Stuff the macro body, substituting formal parameters with actual arguments.
 */
{
    int         c;                          /* Current character    */
    FILEINFO *  file;                       /* Funny #include       */
    char *      out_end;                    /* -> output buffer end */
    char *      in_p;                       /* -> replacement text  */
    char *      out_p;                      /* -> macro output buff */

    file = get_file( defp->name, NULL, NULL, (size_t) (NMACWORK + 1), FALSE);
                                            /* file == infile       */
    in_p = defp->repl;                      /* -> macro replacement */
    out_p = file->buffer;                   /* -> output buffer     */
    out_end = file->buffer + NMACWORK;      /* -> buffer end        */

    while ((c = *in_p++) != EOS) {
        if (c == MAC_PARM) {
            c = (*in_p++ & UCHARMAX) - 1;   /* Parm number          */
            /*
             * Substitute formal parameter with actual argument.
             */
            if (out_end <= (out_p + strlen( arglist_pre[ c])))
                goto nospace;
            out_p = stpcpy( out_p, arglist_pre[ c]);
        } else {
            *out_p++ = c;
        }
        if (out_end <= out_p)
            goto  nospace;
    }

    *out_p = EOS;
    file->buffer = xrealloc( file->buffer, strlen( file->buffer) + 1);
    file->bptr = file->buffer;              /* Truncate buffer      */
    if (mcpp_debug & EXPAND)
        dump_string( "substitute_pre macroline", file->buffer);
    return;

nospace:
    *out_p = EOS;
    diag_macro( CERROR, macbuf_overflow, defp->name, 0L, file->buffer, defp
            , NULL);
    longjmp( jump, 1);
}


/*
 *                  C O M M O N   R O U T I N E S
 *  f o r   S T A N D A R D   a n d   p r e - S T A N D A R D   M o d e s
 */

static int  collect_args(
    const DEFBUF *  defp,       /* Definition of the macro          */
    char **     arglist,        /* Pointers to actual arguments     */
    int         m_num           /* Index into mac_inf[]             */
)
/*
 *   Collect the actual arguments for the macro, checking for correct number
 * of arguments.
 *   Variable arguments (on Standard modes) are read as a merged argument.
 *   Return number of real arguments, or ARG_ERROR on error of unterminated
 * macro.
 *   collect_args() may read over to the next line unless 'in_directive' is
 * set to TRUE.
 *   collect_args() may read over into file->parent to complete a macro call
 * unless stopped by RT_END (provided the "file" is macro).  This is a key
 * trick of STD mode macro expansion.  Meanwhile, POST_STD mode limits the
 * arguments in the "file" (macro or not).
 *   Note: arglist[ n] may be reallocated by collect_args().
 */
{
    const char *    name = defp->name;
    char *  argp = arglist[ 0];         /* Pointer to an argument   */
    char *  arg_end;                    /* End of arguments buffer  */
    char *  valid_argp = NULL;          /* End of valid arguments   */
    char *  sequence;           /* Token sequence for diagnostics   */
    char *  seq;                /* Current pointer into 'sequence'  */
    char *  seq_end;                            /* Limit of buffer  */
    int     args;               /* Number of arguments expected     */
    int     nargs = 0;                  /* Number of collected args */
    int     var_arg = defp->nargs & VA_ARGS;    /* Variable args    */
    int     more_to_come = FALSE;       /* Next argument is expected*/
    LOCATION *  locs;           /* Location of args in source file  */
    LOCATION *  loc;                            /* Current locs     */
    MAGIC_SEQ   mgc_prefix;     /* MAC_INF seqs and spaces preceding an arg */
    int     c;

    if (mcpp_debug & EXPAND)
        dump_unget( "collect_args entry");
    args = (defp->nargs == DEF_PRAGMA) ? 1 : (defp->nargs & ~AVA_ARGS);
    if (args == 0)                      /* Need no argument         */
        valid_argp = argp;
    *argp = EOS;                        /* Make sure termination    */
    arg_end = argp + NMACWORK/2;
    seq = sequence = arg_end + IDMAX;   /* Use latter half of argp  */
    seq_end = seq + NMACWORK/2;
    seq = stpcpy( seq, name);
    *seq++ = '(';
    if (mcpp_mode == STD) {
        /*
         * in_getarg is set TRUE while getting macro arguments, for the sake
         * of diagnostic's convenience.  in_getarg is used only in STD mode.
         */
        in_getarg = TRUE;
        if (trace_macro && m_num) {
            /* #pragma MCPP debug macro_call, and the macro is on source    */
            mac_inf[ m_num].loc_args = loc = locs
                    = (LOCATION *) xmalloc( (sizeof (LOCATION)) * UCHARMAX);
            memset( loc, 0, (sizeof (LOCATION)) * UCHARMAX);
                    /* 0-clear for default values, including empty argument */
        }
    }

    while (1) {
        memset( &mgc_prefix, 0, sizeof (MAGIC_SEQ));
        c = squeeze_ws( &seq, NULL
                , (trace_macro && m_num) ? &mgc_prefix : NULL);
            /* Skip MAC_INF seqs and white spaces, still remember   */
            /* the sequence in buffer, if necessary.                */
        if (c == ')' || c == ',')
            scan_token( c, &seq, seq_end);  /* Ensure token parsing */
        else
            *seq = EOS;

        switch (c) {                    /* First character of token */
        case ')':
            if (! more_to_come) {       /* Zero argument            */
                if (trace_macro && m_num)
                    loc++;
                break;
            }                           /* Else fall through        */
        case ',':                       /* Empty argument           */
            if (trace_macro && m_num)
                loc++;                  /* Advance pointer to infs  */
            if (warn_level & 2)
                diag_macro( CWARN, empty_arg, sequence, 0L, NULL, defp, NULL);
            if (standard && var_arg && nargs == args - 1) {
                /* Variable arguments begin with an empty argument  */
                c = get_an_arg( c, &argp, arg_end, &seq, 1, nargs, &loc
                        , m_num, (trace_macro && m_num) ? &mgc_prefix : NULL);
            } else {
                if (mcpp_mode == STD)
                    *argp++ = RT_END;
                *argp++ = EOS;
            }
            if (++nargs == args)
                valid_argp = argp;
            if (c == ',') {
                more_to_come = TRUE;
                continue;
            } else {                    /* ')'                      */
                break;
            }
        case '\n':      /* Unterminated macro call in directive line*/
            unget_ch();                 /* Fall through             */
        case RT_END:                    /* Error of missing ')'     */
            diag_macro( CERROR, unterm_macro, sequence, 0L, NULL, defp, NULL);
                                        /* Fall through             */
        case CHAR_EOF:                  /* End of file in macro call*/
            nargs = ARG_ERROR;
            goto  arg_ret;              /* Diagnosed by at_eof()    */
        default:                        /* Nomal argument           */
            break;
        }

        if (c == ')')                   /* At end of all args       */
            break;

        c = get_an_arg( c, &argp, arg_end, &seq
                , (var_arg && nargs == args - 1) ? 1 : 0, nargs, &loc
                , m_num, (trace_macro && m_num) ? &mgc_prefix : NULL);

        if (++nargs == args)
            valid_argp = argp;          /* End of valid arguments   */
        if (c == ')')
            break;
        if (c == 0) {                   /* End of file              */
            nargs = ARG_ERROR;
            goto  arg_ret;              /* Diagnosed by at_eof()    */
        }
        if (c == -1) {                  /* Untermanated macro call  */
            diag_macro( CERROR, unterm_macro, sequence, 0L, NULL, defp, NULL);
            nargs = ARG_ERROR;
            goto  arg_ret;
        }
        more_to_come = (c == ',');
    }                                   /* Collected all arguments  */

    if (nargs == 0 && args == 1) {      /* Only and empty argument  */
        if (warn_level & 2)
            diag_macro( CWARN, empty_arg, sequence, 0L, NULL, defp, NULL);
    } else if (nargs != args) {         /* Wrong number of arguments*/
        if (mcpp_mode != OLD_PREP || (warn_level & 1)) {
            if ((standard && var_arg && (nargs == args - 1))
                                /* Absence of variable arguments    */
                        || (mcpp_mode == OLD_PREP)) {
                if (warn_level & 1)
                    diag_macro( CWARN, narg_error, nargs < args ? "Less"
                            : "More", (long) args, sequence, defp, NULL);
            } else {
                diag_macro( CERROR, narg_error, nargs < args ? "Less" : "More"
                        , (long) args, sequence, defp, NULL);
            }
        }
    }
    if (args < nargs) {
        argp = valid_argp;              /* Truncate excess arguments*/
    } else {
        for (c = nargs; c < args; c++) {
            if (mcpp_mode == STD)
                *argp++ = RT_END;       /* For rescan()             */
            *argp++ = EOS;              /* Missing arguments        */
        }
        if (c == 0)
            argp++;                     /* Ensure positive length   */
    }
    arglist[ 0] = argp
            = xrealloc( arglist[ 0], (size_t) (argp - arglist[ 0]));
                                        /* Use memory sparingly     */
    for (c = 1; c < args; c++)
        arglist[ c] = argp += strlen( argp) + 1;
    if (trace_macro && m_num)
        mac_inf[ m_num].loc_args        /* Truncate excess memory   */
                = (LOCATION *) xrealloc( (char *) locs
                        , (loc - locs) * sizeof (LOCATION));

    if (mcpp_debug & EXPAND) {
        if (nargs > 0) {
            if (nargs > args)
                nargs = args;
            dump_args( "collect_args exit", nargs, (const char **) arglist);
        }
        dump_unget( "collect_args exit");
    }
arg_ret:
    if (mcpp_mode == STD)
        in_getarg = FALSE;
    /* Return number of found arguments for function-like macro at most */
    /* defp->nargs, or return defp->nargs for object-like macro.        */
    return  defp->nargs <= DEF_NOARGS ? defp->nargs : nargs;
}
 
static int  get_an_arg(
    int     c,
    char ** argpp,      /* Address of pointer into argument list    */
    char *  arg_end,                /* End of argument list buffer  */
    char ** seqp,                   /* Buffer for diagnostics       */
    int     var_arg,                /* 1 on __VA_ARGS__, 0 on others*/
    int     nargs,                  /* Argument number              */
    LOCATION **     locp,           /* Where to save location infs  */
    int     m_num,                  /* Macro number to trace        */
    MAGIC_SEQ * mgc_prefix  /* White space and magics leading to argument   */
)
/*
 * Get an argument of macro into '*argpp', return the next punctuator.
 * Variable arguments are read as a merged argument.
 * Note: nargs, locp and m_num are used only in macro trace mode of
 * '#pragma MCPP debug macro_call' or -K option.
 */
{
    struct {
        int     n_par;
        int     n_in_src;
    } n_paren[ 16];
    int     num_paren = 0;
    int     end_an_arg = FALSE;             /* End-of-an-arg flag   */
    int     paren = var_arg;                /* For embedded ()'s    */
    int     token_type;
    char *  prevp;
    char *  argp = *argpp;
    int     trace_arg = 0;                  /* Enable tracing arg   */
    LINE_COL    s_line_col, e_line_col; /* Location of macro in an argument */
    MAGIC_SEQ   mgc_seq;        /* Magic seqs and spaces succeeding an arg  */
    size_t  len;

    if (trace_macro) {
        trace_arg = m_num && infile->fp;
        if (m_num) {
            if (trace_arg) {        /* The macro call is in source  */
                s_line_col.line = src_line;
                s_line_col.col = infile->bptr - infile->buffer - 1;
                    /* '-1': bptr is one byte passed beginning of the token */
                get_src_location( & s_line_col);
                (*locp)->start_line = s_line_col.line;
                (*locp)->start_col = s_line_col.col;
                e_line_col = s_line_col;
                    /* Save the location,   */
                    /*      also for end of arg in case of empty arg*/
                memset( n_paren, 0, sizeof (n_paren));
            }
            *argp++ = MAC_INF;
            *argp++ = MAC_ARG_START;
            *argp++ = (m_num / UCHARMAX) + 1;
            *argp++ = (m_num % UCHARMAX) + 1;
            *argp++ = nargs + 1;
                    /* Argument number internally starts at 1       */
            if (mgc_prefix->magic_start) {
                /* Copy the preceding magics, if any    */
                len = mgc_prefix->magic_end - mgc_prefix->magic_start;
                memcpy( argp, mgc_prefix->magic_start, len);
                argp += len;
            }
        }
        memset( &mgc_seq, 0, sizeof (MAGIC_SEQ));
    }

    while (1) {
        if (c == '\n'                       /* In control line      */
                || c == RT_END) {       /* Boundary of rescan (in STD mode) */
            if (c == '\n')
                unget_ch();
            break;
        }
        if (trace_arg) {                    /* Save the location    */
            s_line_col.line = src_line;     /*      of the token    */
            s_line_col.col = infile->bptr - infile->buffer - 1;
        }
        token_type = scan_token( c, (prevp = argp, &argp), arg_end);
                                            /* Scan the next token  */
        switch (c) {
        case '(':                           /* Worry about balance  */
            paren++;                        /* To know about commas */
            break;
        case ')':                           /* Other side too       */
            if (paren-- == var_arg)         /* At the end?          */
                end_an_arg = TRUE;          /* Else more to come    */
            if (trace_arg) {
                if (num_paren && paren == n_paren[ num_paren].n_par) {
                    /* Maybe corresponding parentheses for the macro in arg */
                    int     src_n;
                    src_n = n_paren[ num_paren].n_in_src;
                    in_src[ src_n].end_line = s_line_col.line;
                    in_src[ src_n].end_col = s_line_col.col + 1;
                    num_paren--;
                }
            }
            break;
        case ',':
            if (paren == 0)                 /* Comma delimits arg   */
                end_an_arg = TRUE;
            break;
        case MAC_INF    :               /* Copy magics as they are  */
            switch (*argp++ = get_ch()) {
            case MAC_ARG_START  :
                *argp++ = get_ch();
                /* Fall through */
            case MAC_CALL_START :
                *argp++ = get_ch();
                *argp++ = get_ch();
                break;
            case MAC_ARG_END    :
                if (! option_flags.v)
                    break;
                else
                    *argp++ = get_ch();
                    /* Fall through */
            case MAC_CALL_END   :
                if (option_flags.v) {
                    *argp++ = get_ch();
                    *argp++ = get_ch();
                }
                break;
            }
            break;
        case CHAR_EOF   :                   /* Unexpected EOF       */
            return  0;
        default :                           /* Any token            */
            if (mcpp_mode == STD && token_type == NAM
                    && c != IN_SRC && c != DEF_MAGIC && infile->fp) {
                len = trace_arg ? IN_SRC_LEN : 1;
                memmove( prevp + len, prevp, (size_t) (argp - prevp));
                argp += len;
                *prevp = IN_SRC;
                    /* Mark that the name is read from source file  */
                if (trace_arg) {
                    DEFBUF *    defp;

                    defp = look_id( prevp + IN_SRC_LEN);
                    if (in_src_num >= MAX_IN_SRC_NUM - 1) {
                        cerror(
                        "Too many names in arguments tracing %s"    /* _E_  */
                                , defp ? defp->name : null, 0L, NULL);
                        return  0;
                    } else if (++in_src_num > max_in_src_num) {
                        size_t  old_len;
                        old_len = sizeof (LOCATION) * max_in_src_num;
                        /* Enlarge the array    */
                        in_src = (LOCATION *) xrealloc( (char *) in_src
                                , old_len * 2);
                        /* Have to initialize the enlarged area     */
                        memset( in_src + max_in_src_num, 0, old_len);
                        max_in_src_num *= 2;
                    }
                    /* Insert the identifier number in 2-bytes-encoding     */
                    *(prevp + 1) = (in_src_num / UCHARMAX) + 1;
                    *(prevp + 2) = (in_src_num % UCHARMAX) + 1;
                    if (defp) {             /* Macro name in arg    */
                        in_src[ in_src_num].start_line = s_line_col.line;
                        in_src[ in_src_num].start_col = s_line_col.col;
                        /* For object-like macro, also for function-like    */
                        /* macro in case of parens are not found.           */
                        in_src[ in_src_num].end_line = s_line_col.line;
                        in_src[ in_src_num].end_col
                                = infile->bptr - infile->buffer;
                        if (defp->nargs >= 0) {
                            /* Function-like macro: search parentheses  */
                            n_paren[ ++num_paren].n_par = paren;
                            n_paren[ num_paren].n_in_src = in_src_num;
                        }
                    }   /* Else in_src[ in_src_num].* are 0L        */
                }
            }
            break;
        }                                   /* End of switch        */

        if (end_an_arg)                     /* End of an argument   */
            break;
        if (trace_arg) {                    /* Save the location    */
            e_line_col.line = src_line;     /*      before spaces   */
            e_line_col.col = infile->bptr - infile->buffer;
        }
        memset( &mgc_seq, 0, sizeof (MAGIC_SEQ));
        c = squeeze_ws( &argp, NULL, &mgc_seq);
                                            /* To the next token    */
    }                                       /* Collected an argument*/

    *argp = EOS;
    *seqp = stpcpy( *seqp, *argpp);         /* Save the sequence    */
    if (c == '\n' || c == RT_END)
        return  -1;                         /* Unterminated macro   */
    argp--;                                 /* Remove the punctuator*/
    if (mgc_seq.space)
        --argp;                     /* Remove trailing space        */
    if (mcpp_mode == STD) {
        if (trace_macro && m_num) {
            if (trace_arg) {        /* Location of end of an arg    */
                get_src_location( & e_line_col);
                (*locp)->end_line = e_line_col.line;
                (*locp)->end_col = e_line_col.col;
            }
            (*locp)++;      /* Advance pointer even if !trace_arg   */
            *argp++ = MAC_INF;
            *argp++ = MAC_ARG_END;
            if (option_flags.v) {
                *argp++ = (m_num / UCHARMAX) + 1;
                *argp++ = (m_num % UCHARMAX) + 1;
                *argp++ = nargs + 1;
                *argp = EOS;
                *argpp = chk_magic_balance( *argpp, argp, TRUE, FALSE);
                    /* Check a stray magic caused by abnormal macro */
                    /* and move it to an edge if found.             */
            }
        }
        *argp++ = RT_END;                   /* For rescan()         */
    }
    *argp++ = EOS;                          /* Terminate an argument*/
    *argpp = argp;
    return  c;
}

static int  squeeze_ws(
    char **     out,                /* Pointer to output pointer    */
    char **     endf,               /* Pointer to end of infile data*/
    MAGIC_SEQ * mgc_seq         /* Sequence of MAC_INFs and space   */
            /* mgc_seq should be initialized in the calling routine */
)
/*
 * Squeeze white spaces to one space.
 * White spaces are ' ' (and possibly '\t', when keep_spaces == TRUE.  Note
 * that '\r', '\v', '\f' have been already converted to ' ' by get_ch()),
 * and '\n' unless in_directive is set.
 * COM_SEP is skipped.  TOK_SEPs are squeezed to one TOK_SEP.
 * Copy MAC_INF and its sequences as they are.
 * If white spaces are found and 'out' is not NULL, write a space to *out and
 * increment *out.
 * Record start and end of MAC_INF sequences and whether space is found or
 * not for a convenience of get_an_arg().
 * Return the next character.
 */
{
    int     c;
    int     space = 0;
    int     tsep = 0;
    FILEINFO *      file = infile;
    FILE *  fp = infile->fp;
    int     end_of_file = (out && endf) ? FALSE : TRUE;

    while (((char_type[ c = get_ch()] & SPA) && (! standard 
                || (mcpp_mode == POST_STD && file == infile)
                || (mcpp_mode == STD
                    && ((macro_line != 0 && macro_line != MACRO_ERROR)
                        || file == infile))))
            || c == MAC_INF) {
        if (! end_of_file && file != infile) {  /* Infile has been read over*/
            *endf = *out;               /* Remember the location    */
            end_of_file = TRUE;
        }
        if (c == '\n' && in_directive)  /* If scanning control line */
            break;                      /*   do not skip newline.   */
        switch (c) {
        case '\n':
            space++;
            wrong_line = TRUE;
            break;
        case TOK_SEP:
            if (mcpp_mode == STD)
                tsep++;
            continue;           /* Skip COM_SEP in OLD_PREP mode    */
        case MAC_INF    :       /* Copy magics as they are, or skip */
            if (mgc_seq && ! mgc_seq->magic_start)
                mgc_seq->magic_start = *out;
                                    /* First occurence of magic seq */
            if (out)
                *(*out)++ = c;
            c = get_ch();
            if (out)
                *(*out)++ = c;
            switch (c) {
            case MAC_ARG_START  :
                c = get_ch();
                if (out)
                    *(*out)++ = c;
                /* Fall through */
            case MAC_CALL_START :
                c = get_ch();
                if (out)
                    *(*out)++ = c;
                c = get_ch();
                if (out)
                    *(*out)++ = c;
                break;
            case MAC_ARG_END    :
                if (! option_flags.v) {
                    break;
                } else {
                    c = get_ch();
                    if (out)
                        *(*out)++ = c;
                    /* Fall through */
                }
            case MAC_CALL_END   :
                if (option_flags.v) {
                    c = get_ch();
                    if (out)
                        *(*out)++ = c;
                    c = get_ch();
                    if (out)
                        *(*out)++ = c;
                }
                break;
            }
            if (mgc_seq)        /* Remember end of last magic seq   */
                mgc_seq->magic_end = *out;
            break;
        default:
            space++;
            break;
        }
    }

    if (out) {
        if (space) {            /* Write a space to output pointer  */
            *(*out)++ = ' ';    /*   and increment the pointer.     */
            if (mgc_seq)
                mgc_seq->space = TRUE;
        }
        if (tsep && !space)     /* Needs to preserve token separator*/
            *(*out)++ = TOK_SEP;
        **out = EOS;
    }
    if (mcpp_mode == POST_STD && file != infile) {
        unget_ch();             /* Arguments cannot cross "file"s   */
        c = fp ? CHAR_EOF : RT_END; /* EOF is diagnosed by at_eof() */
    } else if (mcpp_mode == STD && macro_line == MACRO_ERROR
            && file != infile) {            /* EOF                  */
        unget_ch();             /*   diagnosed by at_eof() or only  */
        c = CHAR_EOF;           /*   name of a function-like macro. */
    }                       /* at_eof() resets macro_line on error  */
    return  c;                  /* Return the next character        */
}

static void skip_macro( void)
/*
 * Clear the stacked (i.e. half-expanded) macro, called on macro error.
 */
{
    if (infile == NULL)                     /* End of input         */
        return;
    if (infile->fp)                         /* Source file          */
        return;
    while (infile->fp == NULL) {            /* Stacked stuff        */
        infile->bptr += strlen( infile->bptr);
        get_ch();                           /* To the parent "file" */
    }
    unget_ch();
}

static void diag_macro(
    int     severity,                       /* Error or warning     */ 
    const char *    format,
    const char *    arg1,
    long            arg2, 
    const char *    arg3,
    const DEFBUF *  defp1,          /* Macro causing the problem 1  */
    const DEFBUF *  defp2                                   /*   2  */
)
/*
 * Supplement macro information for diagnostic.
 */
{

    if (defp1 && defp1->name != macro_name)
        expanding( defp1->name, FALSE);
                            /* Inform of the problematic macro call */
    if (defp2 && defp2->name != macro_name)
        expanding( defp2->name, FALSE);
    if (severity == CERROR)
        cerror( format, arg1, arg2, arg3);
    else
        cwarn( format, arg1, arg2, arg3);
}

static void dump_args(
    const char *    why,
    int             nargs,
    const char **   arglist
)
/*
 * Dump arguments list.
 */
{
    int     i;

    mcpp_fprintf( DBG, "dump of %d actual arguments %s\n", nargs, why);
    for (i = 0; i < nargs; i++) {
        mcpp_fprintf( DBG, "arg[%d]", i + 1);
        dump_string( NULL, arglist[ i]);
    }
}
