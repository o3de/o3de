// # Modifications Copyright (c) Contributors to the Open 3D Engine Project, SPDX-License-Identifier: Apache-2.0 OR MIT
/* mcpp_lib.h: declarations of libmcpp exported (visible) functions */
#ifndef _MCPP_LIB_H
#define _MCPP_LIB_H

#ifndef _MCPP_OUT_H
#include    "mcpp_out.h"            /* declaration of OUTDEST   */
#endif

#if _WIN32 || _WIN64 || __CYGWIN__ || __CYGWIN64__ || __MINGW32__   \
            || __MINGW64__
#if     DLL_EXPORT || (__CYGWIN__ && PIC)
#define DLL_DECL    __declspec( dllexport)
#elif   MCPP_DLL_IMPORT
#define DLL_DECL    __declspec( dllimport)
#else
#define DLL_DECL
#endif
#else
#define DLL_DECL
#endif

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

    extern DLL_DECL int     mcpp_lib_main(int argc, const char ** argv);
    extern DLL_DECL void    mcpp_reset_def_out_func(void);
    extern DLL_DECL void    mcpp_set_out_func(
                        int (* func_fputc)  ( int c, MCPP_OUTDEST od),
                        int (* func_fputs)  ( const char * s, MCPP_OUTDEST od),
                        int (* func_fprintf)(MCPP_OUTDEST od, const char * format, ...)
                        );
    extern DLL_DECL void    mcpp_use_mem_buffers( int tf);
    extern DLL_DECL char *  mcpp_get_mem_buffer(MCPP_OUTDEST od);

    extern DLL_DECL void    mcpp_set_report_include_callback(
                        void(*report_include)(FILE * fp,               /* Open file pointer    */
                                              const char *  src_dir,   /* Directory of source  */
                                              const char *  filename,  /* Name of the file     */
                                              const char *  fullname)  /* Full path            */
    );

#ifdef __cplusplus
}
#endif

#endif  /* _MCPP_LIB_H  */
