/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if defined(LMBR_CENTRAL_EDITOR)

#include <AzCore/PlatformDef.h>

/////////////////////////////////////////////////////////////////////////////
// Engine
/////////////////////////////////////////////////////////////////////////////
#include <Cry_Math.h>
#include <ISystem.h>
#include <ISerialize.h>
#include <CryName.h>

/////////////////////////////////////////////////////////////////////////////
// STL
/////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <list>
#include <map>
#include <set>
#include <algorithm>

#else // defined(LMBR_CENTRAL_EDITOR)

#include <platform.h> // Many CryCommon files require that this is included first.

#endif // defined(LMBR_CENTRAL_EDITOR)

#ifdef max
#undef max
#undef min
#endif
