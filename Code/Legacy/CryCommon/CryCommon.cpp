/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */



//      This contains compiled code that is used by other projects in the solution.
//      Because we don't want static DLL dependencies, the CryCommon project is not compiled into a library.
//      Instead, this .cpp file is included in every project which needs it.
//      But we also include it in the CryCommon project (disabled in the build),
//      so that CryCommon can have the same editable settings as other projects.

// Set this to 1 to get an output of some pre-defined compiler symbols.

#if 0

#ifdef _WIN32
#pragma message("_WIN32")
#endif
#ifdef _WIN64
#pragma message("_WIN64")
#endif

#ifdef _M_IX86
#pragma message("_M_IX86")
#endif
#ifdef _M_PPC
#pragma message("_M_PPC")
#endif

#ifdef _DEBUG
#pragma message("_DEBUG")
#endif

#ifdef _DLL
#pragma message("_DLL")
#endif
#ifdef _USRDLL
#pragma message("_USRDLL")
#endif
#ifdef _MT
#pragma message("_MT")
#endif

#endif

#include <AzCore/PlatformIncl.h>

#include "TypeInfo_impl.h"
