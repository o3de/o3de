/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : declarations of all physics interfaces and structures
#pragma once


#include <SerializeFwd.h>

#include "Cry_Geo.h"
#include "stridedptr.h"
#include "primitives.h"
#ifdef NEED_ENDIAN_SWAP
    #include "CryEndian.h"
#endif

#include <ISystem.h>
#include <AzCore/std/parallel/spin_mutex.h>
