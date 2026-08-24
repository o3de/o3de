/* mcpp_out.h: declarations of OUTDEST data types for MCPP  */
// # Modifications Copyright (c) Contributors to the Open 3D Engine Project, SPDX-License-Identifier: Apache-2.0 OR MIT
#ifndef _MCPP_OUT_H
#define _MCPP_OUT_H

/* Choices for output destination */
typedef enum {
    MCPP_OUT,                        /* ~= fp_out    */
    MCPP_ERR,                        /* ~= fp_err    */
    MCPP_DBG,                        /* ~= fp_debug  */
    MCPP_NUM_OUTDEST
} MCPP_OUTDEST;

// O3DE: keep build compatibility with MCPP
#ifndef MCPP_DONT_USE_SHORT_NAMES
 #define OUT MCPP_OUT
 #define ERR MCPP_ERR
 #define DBG MCPP_DBG
 #define NUM_OUTDEST MCPP_NUM_OUTDEST
 #define OUTDEST MCPP_OUTDEST
#endif

#endif  /* _MCPP_OUT_H  */
