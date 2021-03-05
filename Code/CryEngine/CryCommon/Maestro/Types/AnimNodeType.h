/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYCOMMON_MAESTRO_TYPES_ANIMNODETYPE_H
#define CRYINCLUDE_CRYCOMMON_MAESTRO_TYPES_ANIMNODETYPE_H
#pragma once

// **NOTE**: Do not include into the precompiled header hierarchy!

//////////////////////////////////////////////////////////////////////////
//! Node-Types
//
// You need to register new types in Movie.cpp/RegisterNodeTypes for serialization.
// Note: Enums are serialized by string now, there is no need for specific IDs
// anymore for new parameters. Values are for backward compatibility.
enum class AnimNodeType
{
    Invalid                   = 0x00,
    Entity                    = 0x01,       // Legacy Cry Entities (CE is AzEntity)
    Director                  = 0x02,
    Camera                    = 0x03,       // Legacy Cry Camera Object
    CVar                      = 0x04,
    ScriptVar                 = 0x05,
    Material                  = 0x06,
    Event                     = 0x07,
    Group                     = 0x08,
    Layer                     = 0x09,
    Comment                   = 0x10,
    RadialBlur                = 0x11,
    ColorCorrection           = 0x12,
    DepthOfField              = 0x13,
    ScreenFader               = 0x14,
    Light                     = 0x15,       // Legacy Cry Light
    // HDRSetup               = 0x16,       // deprecated Jan 2016
    ShadowSetup               = 0x17,
    Alembic                   = 0x18,       // Used in cinebox, added so nobody uses that number
    GeomCache                 = 0x19,
    Environment,
    ScreenDropsSetup,                       // deprecated Jan 2016
    AzEntity,
    Component,
    Num
};

#endif // CRYINCLUDE_CRYCOMMON_MAESTRO_TYPES_ANIMNODETYPE_H