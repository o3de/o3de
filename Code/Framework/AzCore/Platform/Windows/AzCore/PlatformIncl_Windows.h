/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#define UNICODE
//#define NOGDICAPMASKS     //- CC_*, LC_*, PC_*, CP_*, TC_*, RC_
//#define NOVIRTUALKEYCODES //- VK_*
//#define NOWINMESSAGES     //- WM_*, EM_*, LB_*, CB_*
//#define NOWINSTYLES       //- WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
//#define NOSYSMETRICS      //- SM_*
//#define NOMENUS           //- MF_*
//#define NOICONS           //- IDI_*
//#define NOKEYSTATES       //- MK_*
//#define NOSYSCOMMANDS     //- SC_*
//#define NORASTEROPS       //- Binary and Tertiary raster ops
//#define NOSHOWWINDOW      //- SW_*
//#define OEMRESOURCE       //- OEM Resource values
//#define NOATOM            //- Atom Manager routines
//#define NOCLIPBOARD       //- Clipboard routines
//#define NOCOLOR           //- Screen colors
//#define NOCTLMGR          //- Control and Dialog routines
//#define NODRAWTEXT        //- DrawText() and DT_*
//#define NOGDI             //- All GDI defines and routines
//#define NOKERNEL          //- All KERNEL defines and routines
////#define NOUSER            //- All USER defines and routines
////#define NONLS             //- All NLS defines and routines
////#define NOMB              //- MB_* and MessageBox()
//#define NOMEMMGR          //- GMEM_*, LMEM_*, GHND, LHND, associated routines
//#define NOMETAFILE        //- typedef METAFILEPICT
//#define NOMSG             //- typedef MSG and associated routines
//#define NOOPENFILE        //- OpenFile(), OemToAnsi, AnsiToOem, and OF_*
//#define NOSCROLL          //- SB_* and scrolling routines
//#define NOSERVICE         //- All Service Controller routines, SERVICE_ equates, etc.
//#define NOSOUND           //- Sound driver routines
//#define NOTEXTMETRIC      //- typedef TEXTMETRIC and associated routines
//#define NOWH              //- SetWindowsHook and WH_*
//#define NOWINOFFSETS      //- GWL_*, GCL_*, associated routines
//#define NOCOMM            //- COMM driver routines
//#define NOKANJI           //- Kanji support stuff.
//#define NOHELP            //- Help engine interface.
//#define NOPROFILER        //- Profiler interface.
//#define NODEFERWINDOWPOS  //- DeferWindowPos routines
//#define NOMCX             //- Modem Configuration Extensions

#if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0602)
    #undef _WIN32_WINNT
    #define _WIN32_WINNT 0x0602   // Windows Server 2012 and later
#endif
#if !defined(NOMINMAX)
    #define NOMINMAX    // - Dont define Macros min(a,b) and max(a,b)
#endif

#include <WinSock2.h>
#include <windows.h>

// Undef common function names that Windows.h defines
#if defined(SetJob)
    #undef SetJob
#endif
#if defined(GetObject)
    #undef GetObject
#endif
#if defined(GetCommandLine)
    #undef GetCommandLine
#endif
#if defined(LoadImage)
#undef LoadImage
#endif
#if defined(DeleteFile)
#undef DeleteFile
#endif
#if defined(GetCurrentTime)
#undef GetCurrentTime
#endif

