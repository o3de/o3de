/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYCOMMON_MAESTRO_TYPES_ANIMVALUETYPE_H
#define CRYINCLUDE_CRYCOMMON_MAESTRO_TYPES_ANIMVALUETYPE_H
#pragma once

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
