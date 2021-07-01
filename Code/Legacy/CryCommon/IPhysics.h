/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYCOMMON_IPHYSICS_H
#define CRYINCLUDE_CRYCOMMON_IPHYSICS_H
#pragma once


//
#ifdef PHYSICS_EXPORTS
    #define CRYPHYSICS_API DLL_EXPORT
#else
    #define CRYPHYSICS_API DLL_IMPORT
#endif

#define vector_class Vec3_tpl


#include <CrySizer.h>

//////////////////////////////////////////////////////////////////////////
// IDs that can be used for foreign id.
//////////////////////////////////////////////////////////////////////////
enum EPhysicsForeignIds
{
    PHYS_FOREIGN_ID_TERRAIN = 0,
    PHYS_FOREIGN_ID_STATIC = 1,
    PHYS_FOREIGN_ID_ENTITY = 2,
    PHYS_FOREIGN_ID_FOLIAGE = 3,
    PHYS_FOREIGN_ID_ROPE = 4,
    PHYS_FOREIGN_ID_SOUND_OBSTRUCTION = 5,
    PHYS_FOREIGN_ID_SOUND_PROXY_OBSTRUCTION = 6,
    PHYS_FOREIGN_ID_SOUND_REVERB_OBSTRUCTION = 7,
    PHYS_FOREIGN_ID_WATERVOLUME = 8,
    PHYS_FOREIGN_ID_BREAKABLE_GLASS = 9,
    PHYS_FOREIGN_ID_BREAKABLE_GLASS_FRAGMENT = 10,
    PHYS_FOREIGN_ID_RIGID_PARTICLE = 11,
    PHYS_FOREIGN_ID_RESERVED1 = 12,
    PHYS_FOREIGN_ID_RAGDOLL = 13,
    PHYS_FOREIGN_ID_COMPONENT_ENTITY = 14,

    PHYS_FOREIGN_ID_USER = 100, // All user defined foreign ids should start from this enum.
};


//#include "utils.h"
#include "Cry_Math.h"
#include "primitives.h"
#include <physinterface.h> // <> required for Interfuscator


#endif // CRYINCLUDE_CRYCOMMON_IPHYSICS_H
