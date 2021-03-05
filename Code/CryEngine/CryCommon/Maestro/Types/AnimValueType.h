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

#ifndef CRYINCLUDE_CRYCOMMON_MAESTRO_TYPES_ANIMVALUETYPE_H
#define CRYINCLUDE_CRYCOMMON_MAESTRO_TYPES_ANIMVALUETYPE_H
#pragma once

// **NOTES**: Do not include into the precompiled header hierarchy!
//            There are a couple legacy defines in IMovieSystem.h that
//            correspond to these values. Unknown and Float.

//! Values that animation track can hold.
// Do not change values! they are serialized
//
// Attention: This should only be expanded if you add a completely new value type that tracks can control!
// If you just want to control a new parameter of an entity etc. extend EParamType
//
// Note: If the param type of a track is known and valid these can be derived from the node.
//       These are serialized in case the parameter got invalid (for example for material nodes)
//
enum class AnimValueType
{
    Float = 0,
    Vector = 1,
    Quat = 2,
    Bool = 3,
    Select = 5,
    Vector4 = 15,
    DiscreteFloat = 16,
    RGB = 20,
    CharacterAnim = 21,
    AssetBlend = 22,

    Unknown = static_cast<int>(0xFFFFFFFF)
};


#endif // CRYINCLUDE_CRYCOMMON_MAESTRO_TYPES_ANIMVALUETYPE_H