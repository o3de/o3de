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
 *                          S U P P O R T . C
 *                  S u p p o r t   R o u t i n e s
 *
 * The common routines used by several source files are placed here.
 */

/*
 * The following are global functions.
 *
 * get_unexpandable()   Gets the next unexpandable token in the line, expanding
 *              macros.
 *              Called from #if, #line and #include processing routines.
 * skip_nl()    Skips over a line.
 * skip_ws()    Skips over white spaces but not skip over the end of the line.
 *              skip_ws() skips also COM_SEP and TOK_SEP.
 * scan_token() Reads the next token of any type into the specified output
 *              pointer, advances the pointer, returns the type of token.
 * scan_quote() Reads a string literal, character constant or header-name from
 *              the input stream, writes out to the specified buffer and
 *              returns the advanced output pointer.
 * get_ch()     Reads the next byte from the current input stream, handling
 *              end of (macro/file) input and embedded comments appropriately.
 * cnv_trigraph()   Maps trigraph sequence to C character.
 * cnv_digraph()    Maps digraph sequence to C character.
 * id_operator()    See whether the identifier is an operator in C++.
 * unget_ch()   Pushs last gotten character back on the input stream.
 * unget_string()   Pushs sequence on the input stream.
 * save_string() Saves a string in malloc() memory.
 * get_file()   Initializes a new FILEINFO structure, called when #include
 *              opens a new file, or from unget_string().
 * xmalloc()    Gets a specified number of bytes from heap memory.
 *              If malloc() returns NULL, exits with a message.
 * xrealloc()   realloc().  If it fails, exits with a message.
 * get_src_location()   Trace back line-column datum into pre-line-splicing
 *              phase.  A function for -K option.
 * cfatal(), cerror(), cwarn()
 *              These routines format print messages to the user.
 * mcpp_fputc(), mcpp_fputs(), mcpp_fprintf()
 *              Wrap library functions to support alternate output to memory
 *              buffer.
 */

#if PREPROCESSED
#include    "mcpp.H"
#else
#include    "system.H"
#include    "internal.H"
#endif

static void     scan_id( int c);
                /* Scan an identifier           */
static char *   scan_number( int c, char * out, char * out_end);
                /* Scan a preprocessing number  */
static char *   scan_number_prestd( int c, char * out, char * out_end);
                /* scan_number() for pre-Standard mode  */
#if OK_UCN
static char *   scan_ucn( int cnt, char * out);
                /* Scan an UCN sequence         */
#endif
static char *   scan_op( int c, char * out);
                /* Scan an operator or a punctuator     */
static char *   parse_line( void);
                /* Parse a logical line and convert comments    */
static char *   read_a_comment( char * sp, size_t * sizp);
                /* Read over a comment          */
static char *   get_line( int in_comment);
                /* Get a logical line from file, handle line-splicing   */
static char *   at_eof( int in_comment);
                /* Check erroneous end of file  */
static void     do_msg( const char * severity, const char * format
        , const char * arg1, long arg2, const char * arg3);
                /* Putout diagnostic message    */
static char *   cat_line( int del_bsl);
                /* Splice the line              */
static void     put_line( char * out, FILE * fp);
                /* Put out a logical line       */
static void     dump_token( int token_type, const char * cp);
                /* Dump a token and its type    */

#define EXP_MAC_IND_MAX     16
/* Information of current expanding macros for diagnostic   */
static struct {
    const char *    name;       /* Name of the macro just expanded  */
    int             to_be_freed;    /* Name should be freed later   */
} expanding_macro[ EXP_MAC_IND_MAX];
static int  exp_mac_ind = 0;        /* Index into expanding_macro[] */

static int  in_token = FALSE;       /* For token scanning functions */
static int  in_string = FALSE;      /* For get_ch() and parse_line()*/
static int  squeezews = FALSE;

#define MAX_CAT_LINE    256
/* Information on line catenated by <backslash><newline>    */
/* and by line-crossing comment.  This is for -K option.    */
typedef struct catenated_line {
    long    start_line;         /* Starting line of catenation      */
    long    last_line;          /* Ending line of catanation        */
    size_t  len[ MAX_CAT_LINE + 1];
                        /* Length of successively catenated lines   */
} CAT_LINE;
static CAT_LINE bsl_cat_line;
        /* Datum on the last catenated line by <backslash><newline> */
static CAT_LINE com_cat_line;
        /* Datum on the last catenated line by a line-crossing comment  */

#if MCPP_LIB
static int  use_mem_buffers = FALSE;

void    init_support( void)
{
    in_token = in_string = squeezews = FALSE;
    bsl_cat_line.len[ 0] = com_cat_line.len[ 0] = 0;
    clear_exp_mac();
}

typedef struct  mem_buf {
    char *  buffer;
    char *  entry_pt;
    size_t  size;
    size_t  bytes_avail;
} MEMBUF;

static MEMBUF   mem_buffers[ NUM_OUTDEST];

void    mcpp_use_mem_buffers(
    int    tf
)
{
    int i;

    use_mem_buffers = tf ? TRUE : FALSE;

    for (i = 0; i < NUM_OUTDEST; ++i) {
        if (mem_buffers[ i].buffer)
            /* Free previously allocated memory buffer  */
            free( mem_buffers[ i].buffer);
        if (use_mem_buffers) {
            /* Output to memory buffers instead of files    */
            mem_buffers[ i].buffer = NULL;
            mem_buffers[ i].entry_pt = NULL;
            mem_buffers[ i].size = 0;
            mem_buffers[ i].bytes_avail = 0;
        }
    }
}

int    using_mem_buffers( void)
{
    return use_mem_buffers;
}

#define BUF_INCR_SIZE   (NWORK * 2)
#define MAX( a, b)      (((a) > (b)) ? (a) : (b))

static char *   append_to_buffer(
    MEMBUF *    mem_buf_p,
    const char *    string,
    size_t      length
)
{
    if (mem_buf_p->bytes_avail < length) {  /* Need to allocate more memory */
        size_t size = MAX( BUF_INCR_SIZE, length);

        if (mem_buf_p->buffer == NULL) {            /* 1st append   */
            mem_buf_p->size = size;
            mem_buf_p->bytes_avail = size;
            mem_buf_p->buffer = xmalloc( mem_buf_p->size);
            mem_buf_p->entry_pt = mem_buf_p->buffer;
        } else {
            mem_buf_p->size += size;
            mem_buf_p->bytes_avail += size;
            mem_buf_p->buffer = xrealloc( mem_buf_p->buffer, mem_buf_p->size);
            mem_buf_p->entry_pt = mem_buf_p->buffer + mem_buf_p->size
                    - mem_buf_p->bytes_avail;
        }
    }

    /* Append the string to the tail of the buffer  */
    memcpy( mem_buf_p->entry_pt, string, length);
    mem_buf_p->entry_pt += length;
    mem_buf_p->entry_pt[ 0] = '\0';     /* Terminate the string buffer  */
    mem_buf_p->bytes_avail -= length;

    return mem_buf_p->buffer;
}

static int  mem_putc(
    int     c,
    OUTDEST od
)
{
    char string[ 1];

    string[ 0] = (char) c;

    if (append_to_buffer( &(mem_buffers[ od]), string, 1) != NULL)
        return 0;
    else
        return !0;
}

static int  mem_puts(
    const char *    s,
    OUTDEST od
)
{
    if (append_to_buffer( &(mem_buffers[od]), s, strlen(s)) != NULL)
        return 0;
    else
        return !0;
}

char *  mcpp_get_mem_buffer(
    OUTDEST od
)
{
    return mem_buffers[ od].buffer;
}

#endif  /* MCPP_LIB */

#define DEST2FP(od) \
    (od == OUT) ? fp_out : \
    ((od == ERR) ? fp_err : \
    ((od == DBG) ? fp_debug : \
    (NULL)))

/*
 * The following mcpp_*() wrapper functions are intended to centralize
 * the output generated by MCPP.  They support memory buffer alternates to
 * each of the primary output streams: out, err, debug.  The memory buffer
 * output option would be used in a setup where MCPP has been built as a
 * function call - i.e. mcpp_lib_main().
 */

int    mcpp_lib_fputc(
    int     c,
    OUTDEST od
)
{
#if MCPP_LIB
    if (use_mem_buffers) {
        return mem_putc( c, od);
    } else {
#endif
        FILE *  stream = DEST2FP( od);

        return (stream != NULL) ? fputc( c, stream) : EOF;
#if MCPP_LIB
    }
#endif
}

int (* mcpp_fputc)( int c, OUTDEST od) = mcpp_lib_fputc;

int    mcpp_lib_fputs(
    const char *    s,
    OUTDEST od
)
{
#if MCPP_LIB
    if (use_mem_buffers) {
        return mem_puts( s, od);
    } else {
#endif
        FILE *  stream = DEST2FP( od);

        return (stream != NULL) ? fputs( s, stream) : EOF;
#if MCPP_LIB
    }
#endif
}

int (* mcpp_fputs)( const char * s, OUTDEST od) = mcpp_lib_fputs;

#include <stdarg.h>

int    mcpp_lib_fprintf(
    OUTDEST od,
    const char *    format,
    ...
)
{
    va_list ap;
    FILE *  stream = DEST2FP( od);

    if (stream != NULL) {
        int rc;

        va_start( ap, format);
#if MCPP_LIB
        if (use_mem_buffers) {
            static char     mem_buffer[ NWORK];

            rc = vsprintf( mem_buffer, format, ap);

            if (rc != 0) {
                rc = mem_puts( mem_buffer, od);
            }
        } else {
#endif
            rc = vfprintf( stream, format, ap);
#if MCPP_LIB
        }
#endif
        va_end( ap);

        return rc;

    } else {
        return EOF;
    }
}

int (* mcpp_fprintf)( OUTDEST od, const char * format, ...) = mcpp_lib_fprintf;

#if MCPP_LIB
void    mcpp_reset_def_out_func( void)
{
    mcpp_fputc = mcpp_lib_fputc;
    mcpp_fputs = mcpp_lib_fputs;
    mcpp_fprintf = mcpp_lib_fprintf;
}

void    mcpp_set_out_func(
    int (* func_fputc)( int c, OUTDEST od),
    int (* func_fputs)( const char * s, OUTDEST od),
    int (* func_fprintf)( OUTDEST od, const char * format, ...)
)
{
    mcpp_fputc = func_fputc;
    mcpp_fputs = func_fputs;
    mcpp_fprintf = func_fprintf;
}
#endif

void (*g_report_include)(FILE *, const char *, const char *, const char *);

void mcpp_set_report_include_callback(
    void(*report_include)(
        FILE * fp,               /* Open file pointer    */
        const char *  src_dir,   /* Directory of source  */
        const char *  filename,  /* Name of the file     */
        const char *  fullname)  /* Full path            */
)
{
    g_report_include = report_include;
}

int     get_unexpandable(
    int     c,                              /* First char of token  */
    int     diag                            /* Flag of diagnosis    */
)
/*
 * Get the next unexpandable token in the line, expanding macros.
 * Return the token type.  The token is written in work_buf[].
 * The once expanded macro is never expanded again.
 * Called only from the routines processing #if (#elif, #assert), #line and
 * #include directives in order to diagnose some subtle macro expansions.
 */
{
    DEFBUF *    defp = NULL;
    FILEINFO *  file;
    FILE *  fp = NULL;
    LINE_COL    line_col = { 0L, 0};
    int     token_type = NO_TOKEN;
    int     has_pragma;

    while (c != EOS && c != '\n'                /* In a line        */
            && (fp = infile->fp         /* Preserve current state   */
                , (token_type
                    = scan_token( c, (workp = work_buf, &workp), work_end))
                    == NAM)                     /* Identifier       */
            && fp != NULL                       /* In source !      */
            && (defp = is_macro( NULL)) != NULL) {      /* Macro    */
        expand_macro( defp, work_buf, work_end, line_col, & has_pragma);
                                                /* Expand macro     */
        if (has_pragma)
            cerror( "_Pragma operator found in directive line"      /* _E_  */
                    , NULL, 0L, NULL);
        file = unget_string( work_buf, defp->name);     /* Stack to re-read */
        c = skip_ws();                          /* Skip TOK_SEP     */
        if (file != infile && macro_line != MACRO_ERROR && (warn_level & 1)) {
            /* This diagnostic is issued even if "diag" is FALSE.   */
            cwarn( "Macro \"%s\" is expanded to 0 token"    /* _W1_ */
                    , defp->name, 0L, NULL);
            if (! option_flags.no_source_line)
                dump_a_def( "    macro", defp, FALSE, TRUE, fp_err);
        }
    }

    if (c == '\n' || c == EOS) {
        unget_ch();
        return  NO_TOKEN;
    }

    if (diag && fp == NULL && defp && (warn_level & 1)) {
        char    tmp[ NWORK + 16];
        char *  tmp_end = tmp + NWORK;
        char *  tmp_p;
        file = unget_string( infile->buffer, defp->name);   /* To diagnose  */
        c = get_ch();
        while (file == infile) {    /* Search the expanded macro    */
            if (scan_token( c, (tmp_p = tmp, &tmp_p), tmp_end) != NAM) {
                c = get_ch();
                continue;
            }
            if (standard && str_eq( identifier, "defined")) {
                cwarn( "Macro \"%s\" is expanded to \"defined\""    /* _W1_ */
                        , defp->name, 0L, NULL);
                break;
            }
            if (! standard && str_eq( identifier, "sizeof")) {
                cwarn( "Macro \"%s\" is expanded to \"sizeof\""     /* _W1_ */
                        , defp->name, 0L, NULL);
                break;
            }
            c = get_ch();
        }
        if (file == infile) {
            infile->bptr += strlen( infile->bptr);
            get_ch();
        }
        unget_ch();
        if (token_type == OPE) {
            unget_string( work_buf, NULL);  /* Set again 'openum'   */
            scan_token( get_ch(), (workp = work_buf, &workp), work_end);
        }
    }

    return  token_type;
}

void    skip_nl( void)
/*
 * Skip to the end of the current input line.
 */
{
    insert_sep = NO_SEP;
    while (infile && infile->fp == NULL) {  /* Stacked text         */
        infile->bptr += strlen( infile->bptr);
        get_ch();                           /* To the parent        */
    }
    if (infile)
        infile->bptr += strlen( infile->bptr);  /* Source line      */
}

int     skip_ws( void)
/*
 * Skip over horizontal whitespaces.
 */
{
    int     c;

    do {
        c = get_ch();
    } while (char_type[ c] & HSP);

    return  c;
}

#define MBMASK          0xFF    /* Mask to hide multibyte char      */

int     scan_token(
    int     c,                  /* The first character of the token */
    char ** out_pp,             /* Pointer to pointer to output buf */
    char *  out_end             /* End of output buffer             */
)
/*
 *   Scan the next token of any type.
 *   The token is written out to the specified buffer and the output pointer
 * is advanced.  Token is terminated by EOS.  Return the type of token.
 *   If the token is an identifier, the token is also in identifier[].
 *   If the token is a operator or punctuator, return OPE.
 *   If 'c' is token separator, then return SEP.
 *   If 'c' is not the first character of any known token and not a token
 * separator, return SPE.
 *   In POST_STD mode, inserts token separator (a space) between any tokens of
 * source.
 */
{
    char *  out = *out_pp;              /* Output pointer           */
    int     ch_type;                    /* Type of character        */
    int     token_type = 0;             /* Type of token            */
    int     ch;

    if (standard)
        in_token = TRUE;                /* While a token is scanned */
    c = c & UCHARMAX;
    ch_type = char_type[ c] & MBMASK;

    switch (ch_type) {
    case LET:                           /* Probably an identifier   */
        switch (c) {
        case 'L':
            if (! standard)
                goto  ident;
            ch = get_ch();
            if (char_type[ ch] & QUO) { /* char_type[ ch] == QUO    */
                if (ch == '"')
                    token_type = WSTR;  /* Wide-char string literal */
                else
                    token_type = WCHR;  /* Wide-char constant       */
                c = ch;
                *out++ = 'L';
                break;                  /* Fall down to "case QUO:" */
            } else {
                unget_ch();
            }                           /* Fall through             */
        default:                        /* An identifier            */
ident:
            scan_id( c);
            out = stpcpy( out, identifier);
            token_type = NAM;
            break;
        }
        if (token_type == NAM)
            break;
        /* Else fall through    -- i.e. WSTR, WCHR  */
    case QUO:                   /* String or character constant     */
        out = scan_quote( c, out, out_end, FALSE);
        if (token_type == 0) {                  /* Without prefix L */
            if (c == '"')
                token_type = STR;
            else
                token_type = CHR;
        }   /* Else WSTR or WCHR    */
        break;
    case DOT:
        ch = get_ch();
        unget_ch();
        if ((char_type[ ch] & DIG) == 0)        /* Operator '.' or '...'    */
            goto  operat;
        /* Else fall through    */
    case DIG:                           /* Preprocessing number     */
        out = (standard ? scan_number( c, out, out_end)
                : scan_number_prestd( c, out, out_end));
        token_type = NUM;
        break;
    case PUNC:
operat: out = scan_op( c, out);         /* Operator or punctuator   */
        token_type = OPE;       /* Number is set in global "openum" */
        break;
    default:                /* Special tokens or special characters */
#if OK_UCN
        if (mcpp_mode == STD && c == '\\' && stdc2) {
            ch = get_ch();
            unget_ch();
            if (ch == 'U' || ch == 'u')
                goto  ident;            /* Universal-Characte-Name  */
        }
#endif
#if OK_MBIDENT
        if (mcpp_mode == STD && (char_type[ c] & mbchk) && stdc3) {
            char *  bptr = infile->bptr;
            mb_read( c, &infile->bptr, &out);
            infile->bptr = bptr;
            out = *out_pp;
            goto  ident;        /* An identifier with multi-byte characters */
            /* Mbchar cheking has been done in scan_quote() and others. */
        }
#endif
        if ((standard && (c == CAT || c == ST_QUOTE)) || (char_type[ c] & SPA))
            token_type = SEP;       /* Token separator or magic char*/
        else
            token_type = SPE;
            /* Unkown token ($, @, multi-byte character or Latin    */
        *out++ = c;
        *out = EOS;
        break;
    }

    if (out_end < out)
        cfatal( "Buffer overflow scanning token \"%s\""     /* _F_  */
                , *out_pp, 0L, NULL);
    if (mcpp_debug & TOKEN)
        dump_token( token_type, *out_pp);
    if (mcpp_mode == POST_STD && token_type != SEP && infile->fp != NULL
            && (char_type[ *infile->bptr & UCHARMAX] & SPA) == 0)
        insert_sep = INSERT_SEP;    /* Insert token separator       */
    *out_pp = out;

    in_token = FALSE;               /* Token scanning has been done */
    return  token_type;
}

static void scan_id(
    int     c                               /* First char of id     */
)
/*
 * Reads the next identifier and put it into identifier[].
 * The caller has already read the first character of the identifier.
 */
{
    static char * const     limit = &identifier[ IDMAX];
    static int      dollar_diagnosed = FALSE;   /* Flag of diagnosing '$'   */
#if OK_UCN
    int     uc2 = 0, uc4 = 0;           /* Count of UCN16, UCN32    */
#endif
#if OK_MBIDENT
    int     mb = 0;                     /* Count of MBCHAR  */
#endif
    size_t  len;                        /* Length of identifier     */
    char *  bp = identifier;

    if (c == IN_SRC) {                  /* Magic character  */
        *bp++ = c;
        if ((mcpp_debug & MACRO_CALL) && ! in_directive) {
            *bp++ = get_ch();           /* Its 2-bytes      */
            *bp++ = get_ch();           /*      argument    */
        }
        c = get_ch();
    }

    do {
        if (bp < limit)
            *bp++ = c;
#if OK_UCN
        if (mcpp_mode == STD && c == '\\' && stdc2) {
            int     cnt;
            char *  tp = bp;

            if ((c = get_ch()) == 'u') {
                cnt = 4;
            } else if (c == 'U') {
                cnt = 8;
            } else {
                unget_ch();
                bp--;
                break;
            }
            *bp++ = c;
            if ((bp = scan_ucn( cnt, bp)) == NULL)      /* Error    */
                return;
            if (cnt == 4)
                uc2++;
            else if (cnt == 8)
                uc4++;
            if (limit <= tp)            /* Too long identifier      */
                bp = tp;                /* Back the pointer         */
            goto  next_c;
        }
#endif  /* OK_UCN   */
#if OK_MBIDENT
        if (mcpp_mode == STD && (char_type[ c] & mbchk) && stdc3) {
            len = mb_read( c, &infile->bptr, &bp);
            if (len & MB_ERROR) {
                if (infile->fp)
                    cerror(
                    "Illegal multi-byte character sequence."    /* _E_  */
                            , NULL, 0L, NULL);
            } else {
                mb += len;
            }
        }
#endif  /* OK_MBIDENT   */
#if OK_UCN
next_c:
#endif
        c = get_ch();
    } while ((char_type[ c] & (LET | DIG))      /* Letter or digit  */
#if OK_UCN
            || (mcpp_mode == STD && c == '\\' && stdc2)
#endif
#if OK_MBIDENT
            || (mcpp_mode == STD && (char_type[ c] & mbchk) && stdc3)
#endif
        );

    unget_ch();
    *bp = EOS;

    if (bp >= limit && (warn_level & 1))        /* Limit of token   */
        cwarn( "Too long identifier truncated to \"%s\""    /* _W1_ */
                , identifier, 0L, NULL);

    len = bp - identifier;
#if IDMAX > IDLEN90MIN
    /* UCN16, UCN32, MBCHAR are counted as one character for each.  */
#if OK_UCN
    if (mcpp_mode == STD)
        len -= (uc2 * 5) - (uc4 * 9);
#endif
#if OK_MBIDENT
    if (mcpp_mode == STD)
        len -= mb;
#endif
    if (standard && infile->fp && len > std_limits.id_len && (warn_level & 4))
        cwarn( "Identifier longer than %.0s%ld characters \"%s\""   /* _W4_ */
                , NULL, (long) std_limits.id_len, identifier);
#endif  /* IDMAX > IDLEN90MIN   */

    if (option_flags.dollar_in_name && dollar_diagnosed == FALSE
            && (warn_level & 2) && strchr( identifier, '$') != NULL) {
        cwarn( "'$' in identifier \"%s\"", identifier, 0L, NULL); /* _W2_ */
        dollar_diagnosed = TRUE;            /* Diagnose only once   */
    }
}

char *  scan_quote(
    int         delim,              /* ', " or < (header-name)      */
    char *      out,                /* Output buffer                */
    char *      out_end,            /* End of output buffer         */
    int         diag                /* Diagnostic should be output  */
)
/*
 * Scan off a string literal or character constant to the output buffer.
 * Report diagnosis if the quotation is terminated by newline or character
 * constant is empty (provided 'diag' is TRUE).
 * Return the next output pointer or NULL (on error).
 */
{
    const char * const      skip_line = ", skipped the line";   /* _E_  */
    const char * const      unterm_string
                        = "Unterminated string literal%s";
    const char * const      unterm_char
                        = "Unterminated character constant %s%.0ld%s";
    const char * const      empty_const
                        = "Empty character constant %s%.0ld%s";
    const char *    skip;
    size_t      len;
    int         c;
    char *      out_p = out;

    /* Set again in case of called from routines other than scan_token().   */
    if (standard)
        in_token = TRUE;
    *out_p++ = delim;
    if (delim == '<')
        delim = '>';

scan:
    while ((c = get_ch()) != EOS) {

#if MBCHAR
        if (char_type[ c] & mbchk) {
            /* First of multi-byte character (or shift-sequence)    */
            char *  bptr = infile->bptr;
            len = mb_read( c, &infile->bptr, (*out_p++ = c, &out_p));
            if (len & MB_ERROR) {
                if (infile->fp != NULL && compiling && diag) {
                    if (warn_level & 1) {
                        char *  buf;
                        size_t  chlen;
                        buf = xmalloc( chlen = infile->bptr - bptr + 2);
                        memcpy( buf, bptr, chlen - 1);
                        buf[ chlen - 1] = EOS;
                        cwarn(
    "Illegal multi-byte character sequence \"%s\" in quotation",    /* _W1_ */
                        buf, 0L, NULL);
                        free( buf);
                    }
                }
                continue;
            } else {        /* Valid multi-byte character (or sequence) */
                goto  chk_limit;
            }
        }
#endif
        if (c == delim) {
            break;
        } else if (c == '\\' && delim != '>') { /* In string literal    */
#if OK_UCN
            if (mcpp_mode == STD && stdc2) {
                int         cnt;
                char *      tp;

                *out_p++ = c;
                if ((c = get_ch()) == 'u') {
                    cnt = 4;
                } else if (c == 'U') {
                    cnt = 8;
                } else {
                    goto  escape;
                }
                *out_p++ = c;
                if ((tp = scan_ucn( cnt, out_p)) != NULL)
                    out_p = tp;
                /* Else error   */
                continue;       /* Error or not, anyway continue    */
            }
#endif  /* OK_UCN   */
            *out_p++ = c;                   /* Escape sequence      */
            c = get_ch();
escape:
#if MBCHAR
            if (char_type[ c] & mbchk) {
                                /* '\\' followed by multi-byte char */
                unget_ch();
                continue;
            }
#endif
            if (! standard && c == '\n') {  /* <backslash><newline> */
                out_p--;                    /* Splice the lines     */
                if (cat_line( TRUE) == NULL)        /* End of file  */
                    break;
                c = get_ch();
            }
        } else if (mcpp_mode == POST_STD && c == ' ' && delim == '>'
                && infile->fp == NULL) {
            continue;   /* Skip space possibly inserted by macro expansion  */
        } else if (c == '\n') {
            break;
        }
        if (diag && iscntrl( c) && ((char_type[ c] & SPA) == 0)
                && (warn_level & 1))
            cwarn(
            "Illegal control character %.0s0lx%02x in quotation"    /* _W1_ */
                    , NULL, (long) c, NULL);
        *out_p++ = c;
chk_limit:
        if (out_end < out_p) {
            *out_end = EOS;
            cfatal( "Too long quotation", NULL, 0L, NULL);  /* _F_  */
        }
    }

    if (c == '\n' || c == EOS)
        unget_ch();
    if (c == delim)
        *out_p++ = delim;
    *out_p = EOS;
    if (diag) {                         /* At translation phase 3   */
        skip = (infile->fp == NULL) ? NULL : skip_line;
        if (c != delim) {
            if (mcpp_mode == OLD_PREP   /* Implicit closing of quote*/
                    && (delim == '"' || delim == '\''))
                goto  done;
            if (delim == '"') {
                if (mcpp_mode != POST_STD && option_flags.lang_asm) {
                    /* STD, KR      */
                    /* Concatenate the unterminated string to the next line */
                    if (warn_level & 1)
                        cwarn( unterm_string
                                , ", catenated to the next line"    /* _W1_ */
                                , 0L, NULL);
                    if (cat_line( FALSE) != NULL)
                        goto  scan;         /* Splice the lines     */
                    /* Else end of file     */
                } else {
                    cerror( unterm_string, skip, 0L, NULL); /* _E_  */
                }
            } else if (delim == '\'') {
                if (mcpp_mode != POST_STD && option_flags.lang_asm) {
                    /* STD, KR      */
                    if (warn_level & 1)
                        cwarn( unterm_char, out, 0L, NULL); /* _W1_ */
                    goto  done;
                } else {
                    cerror( unterm_char, out, 0L, skip);    /* _E_  */
                }
            } else {
                cerror( "Unterminated header name %s%.0ld%s"        /* _E_  */
                        , out, 0L, skip);
            }
            out_p = NULL;
        } else if (delim == '\'' && out_p - out <= 2) {
            if (mcpp_mode != POST_STD && option_flags.lang_asm) {
                /* STD, KR      */
                if (warn_level & 1)
                    cwarn( empty_const, out, 0L, skip);     /* _W1_ */
            } else {
                cerror( empty_const, out, 0L, skip);        /* _E_  */
                out_p = NULL;
                goto  done;
            }
        } else if (mcpp_mode == POST_STD && delim == '>' && (warn_level & 2)) {
            cwarn(
        "Header-name enclosed by <, > is an obsolescent feature %s" /* _W2_ */
                    , out, 0L, skip);
        }
#if NWORK-2 > SLEN90MIN
        if (standard && out_p - out > std_limits.str_len && (warn_level & 4))
            cwarn( "Quotation longer than %.0s%ld bytes"    /* _W4_ */
                    , NULL, std_limits.str_len, NULL);
#endif
    }

done:
    in_token = FALSE;
    return  out_p;
}

static char *   cat_line(
    int     del_bsl         /* Delete the <backslash><newline> ?    */
)
/*
 * If del_bsl == TRUE:
 *     Delete <backslash><newline> sequence in string literal.
 * FALSE: Overwrite the <newline> with <backslash>'n'.
 * Return NULL on end of file.  Called only from scan_quote().
 * This routine is never called in POST_STD mode.
 */
{
    size_t  len;
    char *  save1, * save2;

    if (del_bsl) {          /* Delete the <backslash><newline>      */
        infile->bptr -= 2;
        len = infile->bptr - infile->buffer;
    } else {        /* Overwrite the <newline> with <backslash>'n'  */
        strcpy( infile->bptr, "\\n");
        len = strlen( infile->buffer);
    }
    save1 = save_string( infile->buffer);
    save2 = get_line( FALSE);   /* infile->buffer is overwritten    */
    if (save2 == NULL) {
        free( save1);
        return  NULL;
    }
    save2 = save_string( infile->buffer);
    memcpy( infile->buffer, save1, len);
    strcpy( infile->buffer + len, save2);               /* Catenate */
    free( save1);
    free( save2);
    if (! del_bsl)
        len -= 2;
    infile->bptr = infile->buffer + len;
    return  infile->bptr;
}

static char *   scan_number(
    int     c,                              /* First char of number */
    char *  out,                            /* Output buffer        */
    char *  out_end                 /* Limit of output buffer       */
)
/*
 * Read a preprocessing number.
 * By scan_token() we know already that the first c is from 0 to 9 or dot,
 * and if c is dot then the second character is digit.
 * Returns the advanced output pointer.
 * Note: preprocessing number permits non-numeric forms such as 3E+xy,
 *   which are used in stringization or token-concatenation.
 */
{
    char *      out_p = out;        /* Current output pointer       */

    do {
        *out_p++ = c;
        if (c == 'E' || c == 'e'    /* Sign should follow 'E', 'e', */
                || (stdc3 && (c == 'P' || c == 'p'))
                                            /* 'P' or 'p'.          */
                ) {
            c = get_ch();
            if (c == '+' || c == '-') {
                *out_p++ = c;
                c = get_ch();
            }
#if OK_UCN
        } else if (mcpp_mode == STD && c == '\\' && stdc3) {
            int     cnt;
            char *  tp;

            if ((c = get_ch()) == 'u') {
                cnt = 4;
            } else if (c == 'U') {
                cnt = 8;
            } else {
                unget_ch();
                out_p--;
                break;
            }
            *out_p++ = c;
            if ((tp = scan_ucn( cnt, out_p)) == NULL)      /* Error    */
                break;
            else
                out_p = tp;
            c = get_ch();
#endif  /* OK_UCN   */
#if OK_MBIDENT
        } else if (mcpp_mode == STD && (char_type[ c] & mbchk) && stdc3) {
            len = mb_read( c, &infile->bptr, &out_p);
            if (len & MB_ERROR) {
                if (infile->fp)
                    cerror(
                    "Illegal multi-byte character sequence."    /* _E_  */
                            , NULL, 0L, NULL);
            }
#endif  /* OK_MBIDENT   */
        } else {
            c = get_ch();
        }
    } while ((char_type[ c] & (DIG | DOT | LET))    /* Digit, dot or letter */
#if OK_UCN
            || (mcpp_mode == STD && c == '\\' && stdc3)
#endif
#if OK_MBIDENT
            || (mcpp_mode == STD && (char_type[ c] & mbchk) && stdc3)
#endif
        );

    *out_p = EOS;
    if (out_end < out_p)
        cfatal( "Too long pp-number token \"%s\""           /* _F_  */
                , out, 0L, NULL);
    unget_ch();
    return  out_p;
}

/* Original version of DECUS CPP with slight modifications, */
/* too exact for Standard preprocessing.                    */
static char *   scan_number_prestd(
    int         c,                          /* First char of number */
    char *      out,                        /* Output buffer        */
    char *      out_end             /* Limit of output buffer       */
)
/*
 * Process a number.  We know that c is from 0 to 9 or dot.
 * Algorithm from Dave Conroy's Decus C.
 * Returns the advanced output pointer.
 */
{
    char * const    out_s = out;            /* For diagnostics      */
    int             radix;                  /* 8, 10, or 16         */
    int             expseen;                /* 'e' seen in floater  */
    int             octal89;                /* For bad octal test   */
    int             dotflag;                /* TRUE if '.' was seen */

    expseen = FALSE;                        /* No exponent seen yet */
    octal89 = FALSE;                        /* No bad octal yet     */
    radix = 10;                             /* Assume decimal       */
    if ((dotflag = (c == '.')) != FALSE) {  /* . something?         */
        *out++ = '.';                       /* Always out the dot   */
        if ((char_type[(c = get_ch())] & DIG) == 0) {
                                            /* If not a float numb, */
            goto  nomore;                   /* All done for now     */
        }
    }                                       /* End of float test    */
    else if (c == '0') {                    /* Octal or hex?        */
        *out++ = c;                         /* Stuff initial zero   */
        radix = 8;                          /* Assume it's octal    */
        c = get_ch();                       /* Look for an 'x'      */
        if (c == 'x' || c == 'X') {         /* Did we get one?      */
            radix = 16;                     /* Remember new radix   */
            *out++ = c;                     /* Stuff the 'x'        */
            c = get_ch();                   /* Get next character   */
        }
    }
    while (1) {                             /* Process curr. char.  */
        /*
         * Note that this algorithm accepts "012e4" and "03.4"
         * as legitimate floating-point numbers.
         */
        if (radix != 16 && (c == 'e' || c == 'E')) {
            if (expseen)                    /* Already saw 'E'?     */
                break;                      /* Exit loop, bad nbr.  */
            expseen = TRUE;                 /* Set exponent seen    */
            radix = 10;                     /* Decimal exponent     */
            *out++ = c;                     /* Output the 'e'       */
            if ((c = get_ch()) != '+' && c != '-')
                continue;
        }
        else if (radix != 16 && c == '.') {
            if (dotflag)                    /* Saw dot already?     */
                break;                      /* Exit loop, two dots  */
            dotflag = TRUE;                 /* Remember the dot     */
            radix = 10;                     /* Decimal fraction     */
        }
        else {                              /* Check the digit      */
            switch (c) {
            case '8': case '9':             /* Sometimes wrong      */
                octal89 = TRUE;             /* Do check later       */
            case '0': case '1': case '2': case '3':
            case '4': case '5': case '6': case '7':
                break;                      /* Always ok            */

            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                if (radix == 16)            /* Alpha's are ok only  */
                    break;                  /* if reading hex.      */
            default:                        /* At number end        */
                goto done;                  /* Break from for loop  */
            }                               /* End of switch        */
        }                                   /* End general case     */
        *out++ = c;                         /* Accept the character */
        c = get_ch();                       /* Read another char    */
    }                                       /* End of scan loop     */

    if (out_end < out)                      /* Buffer overflow      */
        goto  nomore;
    /*
     * When we break out of the scan loop, c contains the first
     * character (maybe) not in the number.  If the number is an
     * integer, allow a trailing 'L' for long.  If not those, push
     * the trailing character back on the input stream.
     * Floating point numbers accept a trailing 'L' for "long double".
     */
done:
    if (! (dotflag || expseen)) {           /* Not floating point   */
        /*
         * We know that dotflag and expseen are both zero, now:
         *   dotflag signals "saw 'L'".
         */
        for (;;) {
            switch (c) {
            case 'l':
            case 'L':
                if (dotflag)
                    goto nomore;
                dotflag = TRUE;
                break;
            default:
                goto nomore;
            }
            *out++ = c;                     /* Got 'L' .            */
            c = get_ch();                   /* Look at next, too.   */
        }
    }

nomore: *out = EOS;
    if (out_end < out)
        goto  overflow;
    unget_ch();                             /* Not part of a number */
    if (octal89 && radix == 8 && (warn_level & 1))
        cwarn( "Illegal digit in octal number \"%s\""       /* _W1_ */
                , out_s, 0L, NULL);
    return  out;

overflow:
    cfatal( "Too long number token \"%s\"", out_s, 0L, NULL);       /* _F_  */
    return  out;
}

#if OK_UCN
static char *   scan_ucn(
    int     cnt,                            /* Bytes of sequence    */
    char *  out                             /* Output buffer        */
)
/*
 * Scan an UCN sequence and put the sequence to 'out'.
 * Return the advanced pointer or NULL on failure.
 * This routine is never called in POST_STD mode.
 */
{
    uexpr_t value;                              /* Value of UCN     */
    int     i, c;

    value = 0L;
    for (i = 0; i < cnt; i++) {
        c = get_ch();
        if (! isxdigit( c)) {
            if (infile->fp)
                cerror( "Illegal UCN sequence"              /* _E_  */
                        , NULL, 0L, NULL);
                *out = EOS;
                unget_ch();
                return  NULL;
        }
        c = tolower( c);
        *out++ = c;
        c = (isdigit( c) ? (c - '0') : (c - 'a' + 10));
        value = (value << 4) | c;
    }
    if (infile->fp                              /* In source        */
            && ((value >= 0L && value <= 0x9FL
                && value != 0x24L && value != 0x40L && value != 0x60L)
                                    /* Basic source character       */
            || (stdc3 && (value >= 0xD800L && value <= 0xDFFFL))))
                                    /* Reserved for special chars   */
        cerror( "UCN cannot specify the value %.0s\"%08lx\""    /* _E_    */
                    , NULL, (long) value, NULL);
    return  out;
}
#endif  /* OK_UCN   */

static char *   scan_op(
    int     c,                          /* First char of the token  */
    char *  out                         /* Output buffer            */
)
/*
 * Scan C operator or punctuator into the specified buffer.
 * Return the advanced output pointer.
 * The code-number of the operator is stored to global variable 'openum'.
 * Note: '#' is not an operator nor a punctuator in other than directive line,
 *   nevertheless is handled as a punctuator in this cpp for convenience.
 */
{
    int     c2, c3, c4;

    *out++ = c;

    switch (c) {
    case '~':   openum = OP_COM;    break;
    case '(':   openum = OP_LPA;    break;
    case ')':   openum = OP_RPA;    break;
    case '?':   openum = OP_QUE;    break;
    case ';':    case '[':    case ']':    case '{':
    case '}':    case ',':
        openum = OP_1;
        break;
    default:
        openum = OP_2;                  /* Tentative guess          */
    }

    if (openum != OP_2) {               /* Single byte operators    */
        *out = EOS;
        return  out;
    }

    c2 = get_ch();                      /* Possibly two bytes ops   */
    *out++ = c2;

    switch (c) {
    case '=':
        openum = ((c2 == '=') ? OP_EQ : OP_1);          /* ==, =    */
        break;
    case '!':
        openum = ((c2 == '=') ? OP_NE : OP_NOT);        /* !=, !    */
        break;
    case '&':
        switch (c2) {
        case '&':   openum = OP_ANA;        break;      /* &&       */
        case '=':   /* openum = OP_2; */    break;      /* &=       */
        default :   openum = OP_AND;        break;      /* &        */
        }
        break;
    case '|':
        switch (c2) {
        case '|':   openum = OP_ORO;        break;      /* ||       */
        case '=':   /* openum = OP_2; */    break;      /* |=       */
        default :   openum = OP_OR;         break;      /* |        */
        }
        break;
    case '<':
        switch (c2) {
        case '<':   c3 = get_ch();
            if (c3 == '=') {
                openum = OP_3;                          /* <<=      */
                *out++ = c3;
            } else {
                openum = OP_SL;                         /* <<       */
                unget_ch();
            }
            break;
        case '=':   openum = OP_LE;         break;      /* <=       */
        case ':':                                   /* <: i.e. [    */
            if (mcpp_mode == STD && option_flags.dig)
                openum = OP_LBRCK_D;
            else
                openum = OP_LT;
            break;
        case '%':                                   /* <% i.e. {    */
            if (mcpp_mode == STD && option_flags.dig)
                openum = OP_LBRACE_D;
            else
                openum = OP_LT;
            break;
        default :   openum = OP_LT;         break;      /* <        */
        }
        break;
    case '>':
        switch (c2) {
        case '>':   c3 = get_ch();
            if (c3 == '=') {
                openum = OP_3;                          /* >>=      */
                *out++ = c3;
            } else {
                openum = OP_SR;                         /* >>       */
                unget_ch();
            }
            break;
        case '=':   openum = OP_GE;     break;          /* >=       */
        default :   openum = OP_GT;     break;          /* >        */
        }
        break;
    case '#':
        if (standard && (in_define || macro_line))  /* in #define or macro  */
            openum = ((c2 == '#') ? OP_CAT : OP_STR);   /* ##, #    */
        else
            openum = OP_1;                              /* #        */
        break;
    case '+':
        switch (c2) {
        case '+':                                       /* ++       */
        case '=':   /* openum = OP_2; */    break;      /* +=       */
        default :   openum = OP_ADD;        break;      /* +        */
        }
        break;
    case '-':
        switch (c2) {
        case '-':                                       /* --       */
        case '=':                                       /* -=       */
            /* openum = OP_2;   */
            break;
        case '>':
            if (cplus_val) {
                if ((c3 = get_ch()) == '*') {           /* ->*      */
                    openum = OP_3;
                    *out++ = c3;
                } else {
                    /* openum = OP_2;   */
                    unget_ch();
                }
            }   /* else openum = OP_2;  */              /* ->       */
            /* else openum = OP_2;      */
            break;
        default :   openum = OP_SUB;        break;      /* -        */
        }
        break;
    case '%':
        switch (c2) {
        case '=':                           break;      /* %=       */
        case '>':                                   /* %> i.e. }    */
            if (mcpp_mode == STD && option_flags.dig)
                openum = OP_RBRACE_D;
            else
                openum = OP_MOD;
            break;
        case ':':
            if (mcpp_mode == STD && option_flags.dig) {
                if ((c3 = get_ch()) == '%') {
                    if ((c4 = get_ch()) == ':') {   /* %:%: i.e. ## */
                        openum = OP_DSHARP_D;
                        *out++ = c3;
                        *out++ = c4;
                    } else {
                        unget_ch();
                        unget_ch();
                        openum = OP_SHARP_D;        /* %: i.e. #    */
                    }
                } else {
                    unget_ch();
                    openum = OP_SHARP_D;            /* %: i.e. #    */
                }
                if (in_define) {                    /* in #define   */
                    if (openum == OP_DSHARP_D)
                        openum = OP_CAT;
                    else
                        openum = OP_STR;
                }
            } else {
                openum = OP_MOD;
            }
            break;
        default :   openum = OP_MOD;        break;      /* %        */
        }
        break;
    case '*':
        if (c2 != '=')                                  /* *        */
            openum = OP_MUL;
        /* else openum = OP_2;  */                      /* *=       */
        break;
    case '/':
        if (c2 != '=')                                  /* /        */
            openum = OP_DIV;
        /* else openum = OP_2;  */                      /* /=       */
        break;
    case '^':
        if (c2 != '=')                                  /* ^        */
            openum = OP_XOR;
        /* else openum = OP_2;  */                      /* ^=       */
        break;
    case '.':
        if (standard) {
            if (c2 == '.') {
                c3 = get_ch();
                if (c3 == '.') {
                    openum = OP_ELL;                    /* ...      */
                    *out++ = c3;
                    break;
                } else {
                    unget_ch();
                    openum = OP_1;
                }
            } else if (cplus_val && c2 == '*') {        /* .*       */
                /* openum = OP_2    */  ;
            } else {                                    /* .        */
                openum = OP_1;
            }
        } else {
            openum = OP_1;
        }
        break;
    case ':':
        if (cplus_val && c2 == ':')                     /* ::       */
            /* openum = OP_2    */  ;
        else if (mcpp_mode == STD && c2 == '>' && option_flags.dig)
            openum = OP_RBRCK_D;                    /* :> i.e. ]    */
        else                                            /* :        */
            openum = OP_COL;
        break;
    default:                                    /* Never reach here */
        cfatal( "Bug: Punctuator is mis-implemented %.0s0lx%x"      /* _F_  */
                , NULL, (long) c, NULL);
        openum = OP_1;
        break;
    }

    switch (openum) {
    case OP_STR:
        if (mcpp_mode == STD && c == '%')    break;              /* %:   */
    case OP_1:
    case OP_NOT:    case OP_AND:    case OP_OR:     case OP_LT:
    case OP_GT:     case OP_ADD:    case OP_SUB:    case OP_MOD:
    case OP_MUL:    case OP_DIV:    case OP_XOR:    case OP_COM:
    case OP_COL:    /* Any single byte operator or punctuator       */
        unget_ch();
        out--;
        break;
    default:        /* Two or more bytes operators or punctuators   */
        break;
    }

    *out = EOS;
    return  out;
}

int     id_operator(
    const char *    name
)
/*
 * Check whether the name is identifier-like operator in C++.
 * Return the operator number if matched, return 0 if not matched.
 * Note: these identifiers are defined as macros in <iso646.h> in C95.
 * This routine is never called in POST_STD mode.
 */
{
    typedef struct  id_op {
        const char *    name;
        int             op_num;
    } ID_OP;

    ID_OP   id_ops[] = {
        { "and",    OP_ANA},
        { "and_eq", OP_2},
        { "bitand", OP_AND},
        { "bitor",  OP_OR},
        { "compl",  OP_COM},
        { "not",    OP_NOT},
        { "not_eq", OP_NE},
        { "or",     OP_ORO},
        { "or_eq",  OP_2},
        { "xor",    OP_XOR},
        { "xor_eq", OP_2},
        { NULL,     0},
    };

    ID_OP *     id_p = id_ops;

    while (id_p->name != NULL) {
        if (str_eq( name, id_p->name))
            return  id_p->op_num;
        id_p++;
    }
    return  0;
}

void    expanding(
    const char *    name,       /* The name of (nested) macro just expanded. */
    int             to_be_freed /* The name should be freed later.  */
)
/*
 * Remember used macro name for diagnostic.
 */
{
    if (exp_mac_ind < EXP_MAC_IND_MAX - 1) {
        exp_mac_ind++;
    } else {
        clear_exp_mac();
        exp_mac_ind++;
    }
    expanding_macro[ exp_mac_ind].name = name;
    expanding_macro[ exp_mac_ind].to_be_freed = to_be_freed;
}

void    clear_exp_mac( void)
/*
 * Initialize expanding_macro[] freeing names registered in
 * name_to_be_freed[].
 */
{
    int     i;

    for (i = 1; i < EXP_MAC_IND_MAX; i++) {
        if (expanding_macro[ i].to_be_freed) {
            free( (void *) expanding_macro[ i].name);
            expanding_macro[ i].to_be_freed = FALSE;
        }
    }
    exp_mac_ind = 0;
}

int     get_ch( void)
/*
 * Return the next character from a macro or the current file.
 * Always return the value representable by unsigned char.
 */
{
    int             len;
    int             c;
    FILEINFO *      file;

    /*
     * 'in_token' is set to TRUE while scan_token() is executed (and
     * scan_id(), scan_quote(), scan_number(), scan_ucn() and scan_op()
     * via scan_token()) in Standard mode to simplify tokenization.
     * Any token cannot cross "file"s.
     */
    if (in_token)
        return (*infile->bptr++ & UCHARMAX);

    if ((file = infile) == NULL)
        return  CHAR_EOF;                   /* End of all input     */

    if (mcpp_mode == POST_STD && file->fp) {        /* In a source file     */
        switch (insert_sep) {
        case NO_SEP:
            break;
        case INSERT_SEP:                /* Insert a token separator */
            insert_sep = INSERTED_SEP;      /* Remember this fact   */
            return  ' ';                    /*   for unget_ch().    */
        case INSERTED_SEP:                  /* Has just inserted    */
            insert_sep = NO_SEP;            /* Clear the flag       */
            break;
        }
    }
    if (! standard && squeezews) {
        if (*file->bptr == ' ')
            file->bptr++;                   /* Squeeze white spaces */
        squeezews = FALSE;
    }

    if (mcpp_debug & GETC) {
        mcpp_fprintf( DBG, "get_ch(%s) '%c' line %ld, bptr = %d, buffer"
            , file->fp ? cur_fullname : file->real_fname ? file->real_fname
            : file->filename ? file->filename : "NULL"
            , *file->bptr & UCHARMAX
            , src_line, (int) (file->bptr - file->buffer));
        dump_string( NULL, file->buffer);
        dump_unget( "get entrance");
    }

    /*
     * Read a character from the current input logical line or macro.
     * At EOS, either finish the current macro (freeing temporary storage)
     * or get another logical line by parse_line().
     * At EOF, exit the current file (#included) or, at EOF from the MCPP input
     * file, return CHAR_EOF to finish processing.
     * The character is converted to int with no sign-extension.
     */
    if ((c = (*file->bptr++ & UCHARMAX)) != EOS) {
        if (standard)
            return  c;                      /* Just a character     */
        if (! in_string && c == '\\' && *file->bptr == '\n'
                && in_define        /* '\\''\n' is deleted in #define line, */
                    /*   provided the '\\' is not the 2nd byte of mbchar.   */
                && ! last_is_mbchar( file->buffer, strlen( file->buffer) - 2
                && ! keep_spaces)
            ) {
            if (*(file->bptr - 2) == ' ')
                squeezews = TRUE;
        } else {
            return  c;
        }
    }

    /*
     * Nothing in current line or macro.  Get next line (if input from a
     * file), or do end of file/macro processing, and reenter get_ch() to
     * restart from the top.
     */
    if (file->fp &&                         /* In source file       */
            parse_line() != NULL)           /* Get line from file   */
        return  get_ch();
    /*
     * Free up space used by the (finished) file or macro and restart
     * input from the parent file/macro, if any.
     */
    infile = file->parent;                  /* Unwind file chain    */
    free( file->buffer);                    /* Free buffer          */
    if (infile == NULL) {                   /* If at end of input   */
        free( file->filename);
        free( file->src_dir);
        free( file);    /* full_fname is the same with filename for main file*/
        return  CHAR_EOF;                   /* Return end of file   */
    }
    if (file->fp) {                         /* Source file included */
        free( file->filename);              /* Free filename        */
        free( file->src_dir);               /* Free src_dir         */
        fclose( file->fp);                  /* Close finished file  */
        /* Do not free file->real_fname and file->full_fname        */
        cur_fullname = infile->full_fname;
        cur_fname = infile->real_fname;     /* Restore current fname*/
        if (infile->pos != 0L) {            /* Includer was closed  */
            infile->fp = fopen( cur_fullname, "r");
            fseek( infile->fp, infile->pos, SEEK_SET);
        }   /* Re-open the includer and restore the file-position   */
        len = (int) (infile->bptr - infile->buffer);
        infile->buffer = xrealloc( infile->buffer, NBUFF);
            /* Restore full size buffer to get the next line        */
        infile->bptr = infile->buffer + len;
        src_line = infile->line;            /* Reset line number    */
        inc_dirp = infile->dirp;            /* Includer's directory */
#if MCPP_LIB
        mcpp_set_out_func( infile->last_fputc, infile->last_fputs,
                           infile->last_fprintf);
#endif
        include_nest--;
        src_line++;                         /* Next line to #include*/
        sharp( NULL, infile->include_opt ? 1 : (file->include_opt ? 0 : 2));
            /* Need a #line now.  Marker depends on include_opt.    */
            /* The file of include_opt should be marked as 1.       */
            /* Else if returned from include_opt file, it is the    */
            /* main input file, and should not be marked.           */
            /* Else, it is normal includer file, and marked as 2.   */
        src_line--;
        newlines = 0;                       /* Clear the blank lines*/
        if (mcpp_debug & MACRO_CALL)    /* Should be re-initialized */
            com_cat_line.last_line = bsl_cat_line.last_line = 0L;
    } else if (file->filename) {            /* Expanding macro      */
        if (macro_name)     /* file->filename should be freed later */
            expanding( file->filename, TRUE);
        else
            free( file->filename);
    }
    free( file);                            /* Free file space      */
    return  get_ch();                       /* Get from the parent  */
}

static char *   parse_line( void)
/*
 * ANSI (ISO) C: translation phase 3.
 * Parse a logical line.
 * Check illegal control characters.
 * Check unterminated string literal, character constant or comment.
 * Convert each comment to one space (or spaces of the comment length on
 * 'keep_spaces' mode)..
 * Squeeze succeding white spaces other than <newline> (including comments) to
 * one space (unless keep_spaces == TRUE).
 * The lines might be spliced by comments which cross the lines.
 */
{
    char *      temp;                       /* Temporary buffer     */
    char *      limit;                      /* Buffer end           */
    char *      tp;     /* Current pointer into temporary buffer    */
    char *      sp;                 /* Pointer into input buffer    */
    size_t      com_size;
    int         c;

    // By O3DE:
    // ---- The Goal:
    // Match the number of lines between the source AZSL files, before preprocessing,
    // and the AZSL files after preprocessing. Line number matching is important because it helps
    // providing accurate line numbers when reporting errors either by AZSLc or DXC.
    //
    // ---- The Problem:
    // MCPP adds a new line after each comment line that starts with '//'
    //
    // ---- Solution:
    // Do not add the extra line ending character '\n'.
    //
    // ---- How:
    // We count here the amount of leading spaces before the first
    // non space character is detected.
    // This is used to decide whether comments are going to be printed out or
    // skipped altogether.
    // If the current line only contains comments, preceded ONLY by whitespaces then
    // will print the comment in the output WITHOUT line ending character. Otherwise
    // the comment won't make it into the output.
    int         NumLeadingSpaces = 0;

    if ((sp = get_line( FALSE)) == NULL)    /* Next logical line    */
        return  NULL;                       /* End of a file        */
    if (in_asm) {                           /* In #asm block        */
        while (char_type[ *sp++ & UCHARMAX] & SPA)
            ;
        if (*--sp == '#')                   /* Directive line       */
            infile->bptr = sp;
        return  infile->bptr;               /* Don't tokenize       */
    }
    tp = temp = xmalloc( (size_t) NBUFF);
    limit = temp + NBUFF - 2;

    while (char_type[ c = *sp++ & UCHARMAX] & HSP) {
        if (mcpp_mode != POST_STD)
        { // O3DE
            /* Preserve line top horizontal white spaces    */
            /*      as they are for human-readability       */
            *tp++ = c;
            ++NumLeadingSpaces; //O3DE
        } // O3DE
        /* Else skip the line top spaces    */
    }
    sp--;

    while ((c = *sp++ & UCHARMAX) != '\n') {

        switch (c) {
        case '/':
            switch (*sp++) {
            case '*':                       /* Start of a comment   */
com_start:
                if ((sp = read_a_comment( sp, &com_size)) == NULL) {
                    free( temp);            /* End of file with un- */
                    return  NULL;           /*   terminated comment */
                }
                if (keep_spaces && mcpp_mode != OLD_PREP) {
                    if (tp + com_size >= limit - 1)     /* Too long comment */
                        com_size = limit - tp - 1;      /* Truncate */
                    while (com_size--)
                        *tp++ = ' ';        /* Spaces of the comment length */
                    break;
                }
                switch (mcpp_mode) {
                case POST_STD:
                    if (temp < tp && *(tp - 1) != ' ')
                        *tp++ = ' ';        /* Squeeze white spaces */
                    break;
                case OLD_PREP:
                    if (temp == tp
                            || ! (char_type[ *(tp - 1) & UCHARMAX] & HSP))
                        *tp++ = COM_SEP;    /* Convert to magic character   */
                    break;
                default:
                    if (temp == tp ||
                            ! (char_type[ *(tp - 1) & UCHARMAX] & HSP))
                        *tp++ = ' ';        /* Squeeze white spaces */
                    break;
                }
                break;
            case '/':                                       /* //   */
                if (! standard)
                    goto  not_comment;
                /* Comment when C++ or __STDC_VERSION__ >= 199901L      */
                /* Need not to convert to a space because '\n' follows  */
                if (! stdc2 && (warn_level & 2))
                    cwarn( "Parsed \"//\" as comment"       /* _W2_ */
                            , NULL, 0L, NULL);
                if (keep_comments) {
                    const int numCharactersBeforeComment = (tp - temp);
                    if (numCharactersBeforeComment == NumLeadingSpaces)
                    {
                        // O3DE: It is safe to print the comments
                        sp -= 2;
                        while (*sp != '\n')     /* Until end of line    */
                            mcpp_fputc( *sp++, OUT);
                        // mcpp_fputc('\n', OUT); // Removed by O3DE. This prevents that each comment line from presenting additional empty lines.
                    }
                }
                goto  end_line;
            default:                        /* Not a comment        */
not_comment:
                *tp++ = '/';
                sp--;                       /* To re-read           */
                break;
            }
            break;
        case '\r':                          /* Vertical white spaces*/
                /* Note that [CR+LF] is already converted to [LF].  */
        case '\f':
        case '\v':
            if (warn_level & 4)
                cwarn( "Converted %.0s0x%02lx to a space"   /* _W4_ */
                    , NULL, (long) c, NULL);
        case '\t':                          /* Horizontal space     */
        case ' ':
            if (keep_spaces) {
                if (c == '\t')
                    *tp++ = '\t';
                else
                    *tp++ = ' ';            /* Convert to ' '       */
            } else if (! (char_type[ *(tp - 1) & UCHARMAX] & HSP)) {
                *tp++ = ' ';                /* Squeeze white spaces */
            } else if (mcpp_mode == OLD_PREP && *(tp - 1) == COM_SEP) {
                *(tp - 1) = ' ';    /* Replace COM_SEP with ' '     */
            }
            break;
        case '"':                           /* String literal       */
        case '\'':                          /* Character constant   */
            infile->bptr = sp;
            if (standard) {
                tp = scan_quote( c, tp, limit, TRUE);
            } else {
                in_string = TRUE;   /* Enable line splicing by scan_quote() */
                tp = scan_quote( c, tp, limit, TRUE);   /* (not by get_ch())*/
                in_string = FALSE;
            }
            if (tp == NULL) {
                free( temp);                /* Unbalanced quotation */
                return  parse_line();       /* Skip the line        */
            }
            sp = infile->bptr;
            break;
        default:
            if (iscntrl( c)) {
                cerror(             /* Skip the control character   */
    "Illegal control character %.0s0x%lx, skipped the character"    /* _E_  */
                        , NULL, (long) c, NULL);
            } else {                        /* Any valid character  */
                *tp++ = c;
            }
            break;
        }

        if (limit < tp) {
            *tp = EOS;
            cfatal( "Too long line spliced by comments"     /* _F_  */
                    , NULL, 0L, NULL);
        }
    }

end_line:
    if (temp < tp && (char_type[ *(tp - 1) & UCHARMAX] & HSP))
        tp--;                       /* Remove trailing white space  */
    *tp++ = '\n';
    *tp = EOS;
    infile->bptr = strcpy( infile->buffer, temp);   /* Write back to buffer */
    free( temp);
    if (macro_line != 0 && macro_line != MACRO_ERROR) { /* Expanding macro  */
        temp = infile->buffer;
        while (char_type[ *temp & UCHARMAX] & HSP)
            temp++;
        if (*temp == '#'        /* This line starts with # token    */
                || (mcpp_mode == STD && *temp == '%' && *(temp + 1) == ':'))
            if (warn_level & 1)
                cwarn(
    "Macro started at line %.0s%ld swallowed directive-like line"   /* _W1_ */
                    , NULL, macro_line, NULL);
    }
    return  infile->buffer;
}

static char *   read_a_comment(
    char *      sp,                         /* Source               */
    size_t *    sizp                        /* Size of the comment  */
)
/*
 * Read over a comment (which may cross the lines).
 */
{
    int         c;
    char *      saved_sp;
    int         cat_line = 0;       /* Number of catenated lines    */

    if (keep_spaces) {
        saved_sp = sp - 2;          /* '-2' for beginning / and *   */
        *sizp = 0;
    }
    if (keep_comments)                      /* If writing comments  */
        mcpp_fputs( "/*", OUT);             /* Write the initializer*/
    c = *sp++;

    while (1) {                             /* Eat a comment        */
        if (keep_comments)
            mcpp_fputc( c, OUT);

        switch (c) {
        case '/':
            if ((c = *sp++) != '*')         /* Don't let comments   */
                continue;                   /*   nest.              */
            if (warn_level & 1)
                cwarn( "\"/*\" within comment", NULL, 0L, NULL);    /* _W1_ */
            if (keep_comments)
                mcpp_fputc( c, OUT);
                                            /* Fall into * stuff    */
        case '*':
            if ((c = *sp++) != '/')         /* If comment doesn't   */
                continue;                   /*   end, look at next. */
            if (keep_comments) {            /* Put out comment      */
                mcpp_fputc( c, OUT);        /*   terminator, too.   */
                mcpp_fputc( '\n', OUT);     /* Append '\n' to avoid */
                    /*  trouble on some other tools such as rpcgen. */
                wrong_line = TRUE;
            }
            if (keep_spaces)                /* Save the length      */
                *sizp = *sizp + (sp - saved_sp);
            if ((mcpp_debug & MACRO_CALL) && compiling) {
                if (cat_line) {
                    cat_line++;
                    com_cat_line.len[ cat_line]         /* Catenated length */
                            = com_cat_line.len[ cat_line - 1]
                                + strlen( infile->buffer) - 1;
                                            /* '-1' for '\n'        */
                    com_cat_line.last_line = src_line;
                }
            }
            return  sp;                     /* End of comment       */
        case '\n':                          /* Line-crossing comment*/
            if (keep_spaces)                /* Save the length      */
                *sizp = *sizp + (sp - saved_sp) - 1;    /* '-1' for '\n'    */
            if ((mcpp_debug & MACRO_CALL) && compiling) {
                                    /* Save location informations   */
                if (cat_line == 0)  /* First line of catenation     */
                    com_cat_line.start_line = src_line;
                if (cat_line >= MAX_CAT_LINE - 1) {
                    *sizp = 0;      /* Discard the too long comment */
                    cat_line = 0;
                    if (warn_level & 4)
                        cwarn(
                        "Too long comment, discarded up to here"    /* _W4_ */
                                , NULL, 0L, NULL);
                }
                cat_line++;
                com_cat_line.len[ cat_line]
                        = com_cat_line.len[ cat_line - 1]
                            + strlen( infile->buffer) - 1;
            }
            if ((saved_sp = sp = get_line( TRUE)) == NULL)
                return  NULL;       /* End of file within comment   */
                /* Never happen, because at_eof() supplement closing*/
            wrong_line = TRUE;      /* We'll need a #line later     */
            break;
        default:                            /* Anything else is     */
            break;                          /*   just a character   */
        }                                   /* End switch           */

        c = *sp++;
    }                                       /* End comment loop     */

    return  sp;                             /* Never reach here     */
}

static char *   mcpp_fgets(
    char *  s,
    int     size,
    FILE *  stream
)
{
    return fgets( s, size, stream);
}

static char *   get_line(
    int     in_comment
)
/*
 * ANSI (ISO) C: translation phase 1, 2.
 * Get the next logical line from source file.
 * Convert [CR+LF] to [LF].
 */
{
#if COMPILER == INDEPENDENT
#define cr_warn_level 1
#else
#define cr_warn_level 2
#endif
    static int  cr_converted;
    int     converted = FALSE;
    int     len;                            /* Line length - alpha  */
    char *  ptr;
    int     cat_line = 0;           /* Number of catenated lines    */

    if (infile == NULL)                     /* End of a source file */
        return  NULL;
    ptr = infile->bptr = infile->buffer;
    if ((mcpp_debug & MACRO_CALL) && src_line == 0) /* Initialize   */
        com_cat_line.last_line = bsl_cat_line.last_line = 0L;

    while (mcpp_fgets( ptr, (int) (infile->buffer + NBUFF - ptr), infile->fp)
            != NULL) {
        /* Translation phase 1  */
        src_line++;                 /* Gotten next physical line    */
        if (standard && src_line == std_limits.line_num + 1
                && (warn_level & 1))
            cwarn( "Line number %.0s\"%ld\" got beyond range"       /* _W1_ */
                    , NULL, src_line, NULL);
        if (mcpp_debug & (TOKEN | GETC)) {  /* Dump it to DBG       */
            mcpp_fprintf( DBG, "\n#line %ld (%s)", src_line, cur_fullname);
            dump_string( NULL, ptr);
        }
        len = strlen( ptr);
        if (NBUFF - 1 <= ptr - infile->buffer + len
                && *(ptr + len - 1) != '\n') {
                /* The line does not yet end, though the buffer is full.    */
            if (NBUFF - 1 <= len)
                cfatal( "Too long source line"              /* _F_  */
                        , NULL, 0L, NULL);
            else
                cfatal( "Too long logical line"             /* _F_  */
                        , NULL, 0L, NULL);
        }
        if (*(ptr + len - 1) != '\n')   /* Unterminated source line */
            break;
        if (len >= 2 && *(ptr + len - 2) == '\r') {         /* [CR+LF]      */
            *(ptr + len - 2) = '\n';
            *(ptr + --len) = EOS;
            if (! cr_converted && (warn_level & cr_warn_level)) {
                cwarn( "Converted [CR+LF] to [LF]"  /* _W1_ _W2_    */
                        , NULL, 0L, NULL);
                cr_converted = TRUE;
            }
        }
        if (standard) {
            if (option_flags.trig)
                converted = cnv_trigraph( ptr);
            if (mcpp_mode == POST_STD && option_flags.dig)
                converted += cnv_digraph( ptr);
            if (converted)
                len = strlen( ptr);
            /* Translation phase 2  */
            len -= 2;
            if (len >= 0) {
                if ((*(ptr + len) == '\\') && ! last_is_mbchar( ptr, len)) {
                            /* <backslash><newline> (not MBCHAR)    */
                    ptr = infile->bptr += len;  /* Splice the lines */
                    wrong_line = TRUE;
                    if ((mcpp_debug & MACRO_CALL) && compiling) {
                                    /* Save location informations   */
                        if (cat_line == 0)      /* First line of catenation */
                            bsl_cat_line.start_line = src_line;
                        if (cat_line < MAX_CAT_LINE)
                                    /* Record the catenated length  */
                            bsl_cat_line.len[ ++cat_line]
                                    = strlen( infile->buffer) - 2;
                        /* Else ignore  */
                    }
                    continue;
                }
            }
#if NBUFF-2 > SLEN90MIN
            if (ptr - infile->buffer + len + 2 > std_limits.str_len + 1
                    && (warn_level & 4))    /* +1 for '\n'          */
            cwarn( "Logical source line longer than %.0s%ld bytes"  /* _W4_ */
                        , NULL, std_limits.str_len, NULL);
#endif
        }
        if ((mcpp_debug & MACRO_CALL) && compiling) {
            if (cat_line && cat_line < MAX_CAT_LINE) {
                bsl_cat_line.len[ ++cat_line] = strlen( infile->buffer) - 1;
                                /* Catenated length: '-1' for '\n'  */
                bsl_cat_line.last_line = src_line;
            }
        }
        return  infile->bptr = infile->buffer;      /* Logical line */
    }

    /* End of a (possibly included) source file */
    if (ferror( infile->fp))
        cfatal( "File read error", NULL, 0L, NULL);         /* _F_  */
    if ((ptr = at_eof( in_comment)) != NULL)        /* Check at end of file */
        return  ptr;                        /* Partial line supplemented    */
    if (option_flags.z) {
        no_output--;                        /* End of included file */
        keep_comments = option_flags.c && compiling && !no_output;
    }
    return  NULL;
}

#define TRIOFFSET       10

int     cnv_trigraph(
    char *      in
)
/*
 * Perform in-place trigraph replacement on a physical line.  This was added
 * to the C90.  In an input text line, the sequence ??[something] is
 * transformed to a character (which might not appear on the input keyboard).
 */
{
    const char * const  tritext = "=(/)'<!>-\0#[\\]^{|}~";
    /*                             ^          ^
     *                             +----------+
     *                             this becomes this
     */
    int     count = 0;
    const char *    tp;

    while ((in = strchr( in, '?')) != NULL) {
        if (*++in != '?')
            continue;
        while (*++in == '?')
            ;
        if ((tp = strchr( tritext, *in)) == NULL)
            continue;
        *(in - 2) = *(tp + TRIOFFSET);
        in--;
        memmove( in, in + 2, strlen( in + 1));
        count++;
    }

    if (count && (warn_level & 16))
        cwarn( "%.0s%ld trigraph(s) converted"          /* _W16_    */
                , NULL, (long) count, NULL);
    return  count;
}

int     cnv_digraph(
    char *      in
)
/*
 * Perform in-place digraph replacement on a physical line.
 * Called only in POST_STD mode.
 */
{
    int     count = 0;
    int     i;
    int     c1, c2;

    while ((i = strcspn( in, "%:<")), (c1 = *(in + i)) != '\0') {
        in += i + 1;
        c2 = *in;
        switch (c1) {
        case '%'    :
            switch (c2) {
            case ':'    :   *(in - 1) = '#';    break;
            case '>'    :   *(in - 1) = '}';    break;
            default     :   continue;
            }
            break;
        case ':'    :
            switch (c2) {
            case '>'    :   *(in - 1) = ']';    break;
            default     :   continue;
            }
            break;
        case '<'    :
            switch (c2) {
            case '%'    :   *(in - 1) = '{';    break;
            case ':'    :   *(in - 1) = '[';    break;
            default     :   continue;
            }
            break;
        }
        memmove( in, in + 1, strlen( in));
        count++;
    }

    if (count && (warn_level & 16))
        cwarn( "%.0s%ld digraph(s) converted"           /* _W16_    */
                , NULL, (long) count, NULL);
    return  count;
}

static char *   at_eof(
    int     in_comment
)
/*
 * Check the partial line, unterminated comment, unbalanced #if block,
 * uncompleted macro call at end of a file or at end of input.
 * Supplement the line terminator, if possible.
 * Return the supplemented line or NULL on unrecoverable error.
 */
{
    const char * const  format
            = "End of %s with %.0ld%s";                 /* _E_ _W1_ */
    const char * const  unterm_if_format
= "End of %s within #if (#ifdef) section started at line %ld";  /* _E_ _W1_ */
    const char * const  unterm_macro_format
            = "End of %s within macro call started at line %ld";/* _E_ _W1_ */
    const char * const  input
            = infile->parent ? "file" : "input";        /* _E_ _W1_ */
    const char * const  no_newline
            = "no newline, supplemented newline";       /* _W1_     */
    const char * const  unterm_com
            = "unterminated comment, terminated the comment";   /* _W1_     */
    const char * const  backsl = "\\, deleted the \\";  /* _W1_     */
    const char * const  unterm_asm_format
= "End of %s with unterminated #asm block started at line %ld"; /* _E_ _W1_ */
    size_t  len;
    char *  cp;

    cp = infile->buffer;
    len = strlen( cp);
    if (len && *(cp += (len - 1)) != '\n') {
        *++cp = '\n';                       /* Supplement <newline> */
        *++cp = EOS;
        if (mcpp_mode != OLD_PREP && (warn_level & 1))
            cwarn( format, input, 0L, no_newline);
        return  infile->bptr = infile->buffer;
    }
    if (standard && infile->buffer < infile->bptr) {
                            /* No line after <backslash><newline>   */
        cp = infile->bptr;
        *cp++ = '\n';                       /* Delete the \\        */
        *cp = EOS;
        if (warn_level & 1)
            cwarn( format, input, 0L, backsl);
        return  infile->bptr = infile->buffer;
    }
    if (in_comment) {               /* End of file within a comment */
        if (mcpp_mode != OLD_PREP && (warn_level & 1))
            cwarn( format, input, 0L, unterm_com);
        /* The partial comment line has been already read by        */
        /* read_a_comment(), so supplement the  next line.          */
        strcpy( infile->buffer, "*/\n");
        return  infile->bptr = infile->buffer;
    }

    if (infile->initif < ifptr) {
        IFINFO *    ifp = infile->initif + 1;
        if (standard) {
            cerror( unterm_if_format, input, ifp->ifline, NULL);
            ifptr = infile->initif;         /* Clear information of */
            compiling = ifptr->stat;        /*   erroneous grouping */
        } else if (mcpp_mode == KR && (warn_level & 1)) {
            cwarn( unterm_if_format, input, ifp->ifline, NULL);
        }
    }

    if (macro_line != 0 && macro_line != MACRO_ERROR
            && ((mcpp_mode == STD && in_getarg) || ! standard)) {
        if (standard) {
            cerror( unterm_macro_format, input, macro_line, NULL);
            macro_line = MACRO_ERROR;
        } else if (warn_level & 1) {
            cwarn( unterm_macro_format, input, macro_line, NULL);
        }
    }

    if (in_asm && mcpp_mode == KR && (warn_level & 1))
        cwarn( unterm_asm_format, input, in_asm, NULL);

    return  NULL;
}

void    unget_ch( void)
/*
 * Back the pointer to reread the last character.  Fatal error (code bug)
 * if we back too far.  unget_ch() may be called, without problems, at end of
 * file.  Only one character may be ungotten.  If you need to unget more,
 * call unget_string().
 */
{
    if (in_token) {
        infile->bptr--;
        return;
    }

    if (infile != NULL) {
        if (mcpp_mode == POST_STD && infile->fp) {
            switch (insert_sep) {
            case INSERTED_SEP:  /* Have just read an inserted separator */
                insert_sep = INSERT_SEP;
                return;
            case INSERT_SEP:
                cfatal( "Bug: unget_ch() just after scan_token()"   /* _F_  */
                        , NULL, 0L, NULL);
                break;
            default:
                break;
            }
        }
        --infile->bptr;
        if (infile->bptr < infile->buffer)      /* Shouldn't happen */
            cfatal( "Bug: Too much pushback", NULL, 0L, NULL);      /* _F_  */
    }

    if (mcpp_debug & GETC)
        dump_unget( "after unget");
}

FILEINFO *  unget_string(
    const char *    text,               /* Text to unget            */
    const char *    name                /* Name of the macro, if any*/
)
/*
 * Push a string back on the input stream.  This is done by treating
 * the text as if it were a macro or a file.
 */
{
    FILEINFO *      file;
    size_t          size;

    if (text)
        size = strlen( text) + 1;
    else
        size = 1;
    file = get_file( name, NULL, NULL, size, FALSE);
    if (text)
        memcpy( file->buffer, text, size);
    else
        *file->buffer = EOS;
    return  file;
}

char *  save_string(
    const char *      text
)
/*
 * Store a string into free memory.
 */
{
    char *      result;
    size_t      size;

    size = strlen( text) + 1;
    result = xmalloc( size);
    memcpy( result, text, size);
    return  result;
}

FILEINFO *  get_file(
    const char *    name,                   /* File or macro name   */
    const char *    src_dir,                /* Source file directory*/
    const char *    fullname,               /* Full path list       */
    size_t      bufsize,                    /* Line buffer size     */
    int         include_opt         /* Specified by -include opt (for GCC)  */
)
/*
 * Common FILEINFO buffer initialization for a new file or macro.
 */
{
    FILEINFO *  file;

    file = (FILEINFO *) xmalloc( sizeof (FILEINFO));
    file->buffer = xmalloc( bufsize);
    file->bptr = file->buffer;              /* Initialize line ptr  */
    file->buffer[ 0] = EOS;                 /* Force first read     */
    file->line = 0L;                        /* (Not used just yet)  */
    file->fp = NULL;                        /* No file yet          */
    file->pos = 0L;                         /* No pos to remember   */
    file->parent = infile;                  /* Chain files together */
    file->initif = ifptr;                   /* Initial ifstack      */
    file->include_opt = include_opt;        /* Specified by -include*/
    file->dirp = NULL;                      /* No include dir yet   */
    file->real_fname = name;                /* Save file/macro name */
    file->full_fname = fullname;            /* Full path list       */
    if (name) {
        file->filename = xmalloc( strlen( name) + 1);
        strcpy( file->filename, name);      /* Copy for #line       */
    } else {
        file->filename = NULL;
    }
    if (src_dir) {
        file->src_dir = xmalloc( strlen( src_dir) + 1);
        strcpy( file->src_dir, src_dir);
    } else {
        file->src_dir = NULL;
    }
#if MCPP_LIB
    file->last_fputc = mcpp_lib_fputc;
    file->last_fputs = mcpp_lib_fputs;
    file->last_fprintf = mcpp_lib_fprintf;
#endif
    if (infile != NULL) {                   /* If #include file     */
        infile->line = src_line;            /* Save current line    */
#if MCPP_LIB
        infile->last_fputc = mcpp_fputc;
        infile->last_fputs = mcpp_fputs;
        infile->last_fprintf = mcpp_fprintf;
#endif
    }
    infile = file;                          /* New current file     */

    return  file;                           /* All done.            */
}

static const char * const   out_of_memory
    = "Out of memory (required size is %.0s0x%lx bytes)";   /* _F_  */

char *
(xmalloc)(
    size_t      size
)
/*
 * Get a block of free memory.
 */
{
    char *      result;

    if ((result = (char *) malloc( size)) == NULL) {
        if (mcpp_debug & MEMORY)
            print_heap();
       cfatal( out_of_memory, NULL, (long) size, NULL);
    }
    return  result;
}

char *  (xrealloc)(
    char *      ptr,
    size_t      size
)
/*
 * Reallocate malloc()ed memory.
 */
{
    char *      result;

    if ((result = (char *) realloc( ptr, size)) == NULL && size != 0) {
        /* 'size != 0' is necessary to cope with some               */
        /*   implementation of realloc( ptr, 0) which returns NULL. */
        if (mcpp_debug & MEMORY)
            print_heap();
        cfatal( out_of_memory, NULL, (long) size, NULL);
    }
    return  result;
}

LINE_COL *  get_src_location(
    LINE_COL *  p_line_col          /* Line and column on phase 4   */
)
/*
 * Convert line-column datum of just after translation phase 3 into that of
 * phase 2, tracing back line splicing by a comment and <backslash><newline>.
 * Note: This conversion does not give correct datum on a line catenated by
 * both of <backslash><newline> and line-crossing-comment at the same time.
 *
 * com_cat_line and bsl_cat_line have data only on last catenated line.
 * com_cat_line.len[] and bsl_cat_line.len[] have the length of catenated
 * line, and len[ 0] is always 0, followed by len[ 1], len[ 2], ..., as
 * accumulated length of successively catenated lines.
 */
{
    long        line;
    size_t      col;
    size_t *    cols;
    CAT_LINE *  l_col_p;
    int         i;

    line = p_line_col->line;
    col = p_line_col->col;

    for (i = 0; i <= 1; i++) {
        l_col_p = i ? & bsl_cat_line : & com_cat_line;
        if (l_col_p->last_line != line)
            continue;
        /* Else just catenated line */
        cols = l_col_p->len + 1;
        while (*cols < col)
            cols++;
        if (col <= *cols) {
            cols--;
            col -= *cols;
        }
        line = l_col_p->start_line + (cols - l_col_p->len);
    }

    p_line_col->line = line;
    p_line_col->col = col + 1;
                    /* col internally start at 0, output start at 1 */

    return  p_line_col;
}

static void put_line(
    char *  out,
    FILE *  fp
)
/*
 * Put out a logical source line.
 * This routine is called only in OLD_PREP mode.
 */
{
    int     c;

    while ((c = *out++) != EOS) {
        if (c != COM_SEP)           /* Skip 0-length comment        */
            mcpp_fputc( c, FP2DEST( fp));
    }
}

static void do_msg(
    const char *    severity,       /* "fatal", "error", "warning"  */
    const char *    format,         /* Format for the error message */
    const char *    arg1,           /* String arg. for the message  */
    long            arg2,           /* Integer argument             */
    const char *    arg3            /* Second string argument       */
)
/*
 * Print filenames, macro names, line numbers and error messages.
 * Also print macro definitions on macro expansion problems.
 */
{
    FILEINFO *  file;
    DEFBUF *    defp;
    int         i;
    size_t      slen;
    const char *    arg_s[ 2];
    char *      arg_t[ 2];
    char *      tp;
    const char *    sp;
    int         c;
    int         ind;

    fflush( fp_out);                /* Synchronize output and diagnostics   */
    arg_s[ 0] = arg1;  arg_s[ 1] = arg3;

    for (i = 0; i < 2; i++) {   /* Convert special characters to visible    */
        sp = arg_s[ i];
        if (sp != NULL)
            slen = strlen( sp) + 1;
        else
            slen = 1;
        tp = arg_t[ i] = (char *) malloc( slen);
            /* Don't use xmalloc() so as not to cause infinite recursion    */
        if (sp == NULL || *sp == EOS) {
            *tp = EOS;
            continue;
        }

        while ((c = *sp++) != EOS) {
            switch (c) {
            case TOK_SEP:
                if (mcpp_mode == OLD_PREP)      /* COM_SEP          */
                    break;              /* Skip magic characters    */
                /* Else fall through    */
            case RT_END:
            case CAT:
            case ST_QUOTE:
            case DEF_MAGIC:
                if (! standard)
                    *tp++ = ' ';
                break;                  /* Skip the magic characters*/
            case IN_SRC:
                if (! standard)
                    *tp++ = ' ';
                if ((mcpp_debug & MACRO_CALL) && ! in_directive)
                    sp += 2;            /* Skip two more bytes      */
                break;
            case MAC_INF:
                if (mcpp_mode != STD) {
                    *tp++ = ' ';
                    /* Illegal control character, convert to a space*/
                } else {
                    switch (*sp++) {    /* Skip the magic characters*/
                    case MAC_ARG_START  :
                        sp++;
                        /* Fall through */
                    case MAC_CALL_START :
                        sp += 2;
                        break;
                    case MAC_ARG_END    :
                        if (! option_flags.v)
                            break;
                        else
                            sp++;
                            /* Fall through */
                    case MAC_CALL_END   :
                        if (option_flags.v)
                            sp += 2;
                        break;
                    }
                }
                break;
            case '\n':
                *tp++ = ' ';            /* Convert '\n' to a space  */
                break;
            default:
                *tp++ = c;
                break;
            }
        }

        if (*(sp - 2) == '\n')
            tp--;
        *tp = EOS;
    }

    /* Print source location and diagnostic */
    file = infile;
    while (file != NULL && (file->fp == NULL || file->fp == (FILE *)-1))
        file = file->parent;                        /* Skip macro   */
    if (file != NULL) {
        file->line = src_line;
        mcpp_fprintf( ERR, "%s:%ld: %s: ", cur_fullname, src_line, severity);
    }
    mcpp_fprintf( ERR, format, arg_t[ 0], arg2, arg_t[ 1]);
    mcpp_fputc( '\n', ERR);
    if (option_flags.no_source_line)
        goto  free_arg;

    /* Print source line, includers and expanding macros    */
    file = infile;
    if (file != NULL && file->fp != NULL) {
        if (mcpp_mode == OLD_PREP) {
            mcpp_fputs( "    ", ERR);
            put_line( file->buffer, fp_err);
        } else {
            mcpp_fprintf( ERR, "    %s", file->buffer);
                                            /* Current source line  */
        }
        file = file->parent;
    }
    while (file != NULL) {                  /* Print #includes, too */
        if (file->fp == NULL) {             /* Macro                */
            if (file->filename) {
                defp = look_id( file->filename);
                if ((defp->nargs > DEF_NOARGS_STANDARD)
                    && ! (file->parent && file->parent->filename
                        && str_eq( file->filename, file->parent->filename)))
                        /* If the name is not duplicate of parent   */
                    dump_a_def( "    macro", defp, FALSE, TRUE, fp_err);
            }
        } else {                            /* Source file          */
            if (file->buffer[ 0] == '\0')
                strcpy( file->buffer, "\n");
            if (mcpp_mode != OLD_PREP) {
                mcpp_fprintf( ERR, "    from %s: %ld:    %s",
                    file->line ? file->full_fname       /* Full-path-list   */
                        : "<stdin>",        /* Included by -include */
                    file->line,             /* Current line number  */
                    file->buffer);          /* The source line      */
            } else {
                mcpp_fprintf( ERR, "    from %s: %ld:    ", file->full_fname
                        , file->line);
                put_line( file->buffer, fp_err);
            }
        }
        file = file->parent;
    }

    if (! macro_name)
        goto  free_arg;
    /* Additional information of macro definitions  */
    expanding_macro[ 0].name = macro_name;
    for (ind = 0; ind <= exp_mac_ind; ind++) {
        int         ind_done;

        for (ind_done = 0; ind_done < ind; ind_done++)
            if (str_eq( expanding_macro[ ind].name
                    , expanding_macro[ ind_done].name))
                break;                      /* Already reported     */
        if (ind_done < ind)
            continue;
        for (file = infile; file; file = file->parent)
            if (file->fp == NULL && file->filename
                    && str_eq( expanding_macro[ ind].name, file->filename))
                break;                      /* Already reported     */
        if (file)
            continue;
        if ((defp = look_id( expanding_macro[ ind].name)) != NULL) {
            if (defp->nargs <= DEF_NOARGS_STANDARD)
                continue;                   /* Standard predefined  */
            dump_a_def( "    macro", defp, FALSE, TRUE, fp_err);
            /* Macro already read over  */
        }
    }

free_arg:
    for (i = 0; i < 2; i++)
        free( arg_t[ i]);
}

void    cfatal(
    const char *    format,
    const char *    arg1,
    long    arg2,
    const char *    arg3
)
/*
 * A real disaster.
 */
{
    do_msg( "fatal error", format, arg1, arg2, arg3);
    longjmp( error_exit, -1);
}

void    cerror(
    const char *    format,
    const char *    arg1,
    long    arg2,
    const char *    arg3
)
/*
 * Print a error message.
 */
{
    do_msg( "error", format, arg1, arg2, arg3);
    errors++;
}

void    cwarn(
    const char *    format,
    const char *    arg1,
    long    arg2,
    const char *    arg3
)
/*
 * Maybe an error.
 */
{
    do_msg( "warning", format, arg1, arg2, arg3);
}

void    dump_string(
    const char *    why,
    const char *    text
)
/*
 * Dump text readably.
 * Bug: macro argument number may be putout as a control character or any
 * other character, just after MAC_PARM has been read away.
 */
{
    const char *    cp;
    const char *    chr;
    int     c, c1, c2;

    if (why != NULL)
        mcpp_fprintf( DBG, " (%s)", why);
    mcpp_fputs( " => ", DBG);

    if (text == NULL) {
        mcpp_fputs( "NULL", DBG);
        return;
    }

    for (cp = text; (c = *cp++ & UCHARMAX) != EOS; ) {
        chr = NULL;

        switch (c) {
        case MAC_PARM:
            c = *cp++ & UCHARMAX;       /* Macro parameter number   */
            mcpp_fprintf( DBG, "<%d>", c);
            break;
        case MAC_INF:
            if (! (mcpp_mode == STD && (mcpp_debug & MACRO_CALL)))
                goto  no_magic;
            /* Macro informations inserted by -K option */
            c2 = *cp++ & UCHARMAX;
            if (option_flags.v || c2 == MAC_CALL_START
                    || c2 == MAC_ARG_START) {
                c = ((*cp++ & UCHARMAX) - 1) * UCHARMAX;
                c += (*cp++ & UCHARMAX) - 1;
            }
            switch (c2) {
            case MAC_CALL_START:
                mcpp_fprintf( DBG, "<MAC%d>", c);
                break;
            case MAC_CALL_END:
                if (option_flags.v)
                    mcpp_fprintf( DBG, "<MAC_END%d>", c);
                else
                    chr = "<MAC_END>";
                break;
            case MAC_ARG_START:
                c1 = *cp++ & UCHARMAX;
                mcpp_fprintf( DBG, "<MAC%d:ARG%d>", c, c1 - 1);
                break;
            case MAC_ARG_END:
                if (option_flags.v) {
                    c1 = *cp++ & UCHARMAX;
                    mcpp_fprintf( DBG, "<ARG_END%d-%d>", c, c1 - 1);
                } else {
                    chr = "<ARG_END>";
                }
                break;
            }
            break;
        case DEF_MAGIC:
            if (standard) {
                chr = "<MAGIC>";
                break;
            }       /* Else fall through    */
        case CAT:
            if (standard) {
                chr = "##";
                break;
            }       /* Else fall through    */
        case ST_QUOTE:
            if (standard) {
                chr = "#";
                break;
            }       /* Else fall through    */
        case RT_END:
            if (standard) {
                chr = "<RT_END>";
                break;
            }       /* Else fall through    */
        case IN_SRC:
            if (standard) {
                if ((mcpp_debug & MACRO_CALL) && ! in_directive) {
                    int     num;
                    num = ((*cp++ & UCHARMAX) - 1) * UCHARMAX;
                    num += (*cp++ & UCHARMAX) - 1;
                    mcpp_fprintf( DBG, "<SRC%d>", num);
                } else {
                    chr = "<SRC>";
                }
            } else {                        /* Control character    */
                mcpp_fprintf( DBG, "<^%c>", c + '@');
            }
            break;
        case TOK_SEP:
            if (mcpp_mode == STD) {
                chr = "<TSEP>";
                break;
            } else if (mcpp_mode == OLD_PREP) {     /* COM_SEP      */
                chr = "<CSEP>";
                break;
            }       /* Else fall through    */
        default:
no_magic:
            if (c < ' ')
                mcpp_fprintf( DBG, "<^%c>", c + '@');
            else
                mcpp_fputc( c, DBG);
            break;
        }

        if (chr)
            mcpp_fputs( chr, DBG);
    }

    mcpp_fputc( '\n', DBG);
}

void    dump_unget(
    const char *    why
)
/*
 * Dump all ungotten junk (pending macros and current input lines).
 */
{
    const FILEINFO *    file;

    mcpp_fputs( "dump of pending input text", DBG);
    if (why != NULL) {
        mcpp_fputs( "-- ", DBG);
        mcpp_fputs( why, DBG);
    }
    mcpp_fputc( '\n', DBG);

    for (file = infile; file != NULL; file = file->parent)
        dump_string( file->real_fname ? file->real_fname
                : file->filename ? file->filename : "NULL", file->bptr);
}

static void dump_token(
    int     token_type,
    const char *    cp                              /* Token        */
)
/*
 * Dump a token.
 */
{
    static const char * const   t_type[]
            = { "NAM", "NUM", "STR", "WSTR", "CHR", "WCHR", "OPE", "SPE"
            , "SEP", };

    mcpp_fputs( "token", DBG);
    dump_string( t_type[ token_type - NAM], cp);
}
