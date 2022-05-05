/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Obb.h>
#include <ScriptCanvas/Core/NodeFunctionGeneric.h>
#include <ScriptCanvas/Data/NumericData.h>
#include <ScriptCanvas/Libraries/Math/MathNodeUtilities.h>

namespace ScriptCanvas
{
    //! OBBNodes is deprecated
    //! This file is deprecated and will be removed. Keep it to allow for seamless migration from node generic framework
    //! to new AutoGen function.
    namespace OBBNodes
    {
        using namespace Data;
        using namespace MathNodeUtilities;
        static constexpr const char* k_categoryName = "Math/OBB";

        AZ_INLINE OBBType FromAabb(const AABBType& source)
        {
            return OBBType::CreateFromAabb(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromAabb, k_categoryName, "{0D98DF33-2689-48BA-A4DA-139CD2F7E570}", "ScriptCanvas_OBBFunctions_FromAabb");
        
        AZ_INLINE OBBType FromPositionRotationAndHalfLengths(Vector3Type position, QuaternionType rotation, Vector3Type halfLengths)
        {
            return OBBType::CreateFromPositionRotationAndHalfLengths(position, rotation, halfLengths);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromPositionRotationAndHalfLengths, "Math/OBB", "{F09F5646-0BFF-4414-9389-2764E819917E}", "ScriptCanvas_OBBFunctions_FromPositionRotationAndHalfLengths");

        AZ_INLINE BooleanType IsFinite(const OBBType& source)
        {
            return source.IsFinite();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            IsFinite,
            k_categoryName, "{DDD8F512-8260-457E-8003-50AC8B86B2D9}", "ScriptCanvas_OBBFunctions_IsFinite");
        
        AZ_INLINE Vector3Type GetAxisX(const OBBType& source)
        {
            return source.GetAxisX();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            GetAxisX, k_categoryName, "{BAD460A2-D2D2-4191-A442-C5649F09FD54}", "ScriptCanvas_OBBFunctions_GetAxisX");

        AZ_INLINE Vector3Type GetAxisY(const OBBType& source)
        {
            return source.GetAxisY();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            GetAxisY, k_categoryName, "{9B593C32-5BAB-4CCA-BB6C-25FD09AD47F7}", "ScriptCanvas_OBBFunctions_GetAxisY");

        AZ_INLINE Vector3Type GetAxisZ(const OBBType& source)
        {
            return source.GetAxisZ();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            GetAxisZ, k_categoryName, "{8818F39D-69E5-4571-920A-4805BA3F6FD9}", "ScriptCanvas_OBBFunctions_GetAxisZ");

        AZ_INLINE Vector3Type GetPosition(const OBBType& source)
        {
            return source.GetPosition();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            GetPosition, k_categoryName, "{01907D93-C657-4CA6-A60B-5C0E6319A62D}", "ScriptCanvas_OBBFunctions_GetPosition");

        using Registrar = RegistrarGeneric
            < FromAabbNode
            , FromPositionRotationAndHalfLengthsNode
            , GetAxisXNode
            , GetAxisYNode
            , GetAxisZNode
            , GetPositionNode
            , IsFiniteNode
        >;

    }
} 

