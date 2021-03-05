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

#pragma once

#include <AzCore/Math/Obb.h>
#include <ScriptCanvas/Core/NodeFunctionGeneric.h>
#include <ScriptCanvas/Data/NumericData.h>
#include <ScriptCanvas/Libraries/Math/MathNodeUtilities.h>

namespace ScriptCanvas
{
    namespace OBBNodes
    {
        using namespace Data;
        using namespace MathNodeUtilities;

        AZ_INLINE OBBType FromAabb(const AABBType& source)
        {
            return OBBType::CreateFromAabb(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromAabb, "Math/OBB", "{0D98DF33-2689-48BA-A4DA-139CD2F7E570}", "converts the Source to an OBB", "Source");
        
        AZ_INLINE OBBType FromPositionRotationAndHalfLengths(Vector3Type position, QuaternionType rotation, Vector3Type halfLengths)
        {
            return OBBType::CreateFromPositionRotationAndHalfLengths(position, rotation, halfLengths);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromPositionRotationAndHalfLengths, "Math/OBB", "{F09F5646-0BFF-4414-9389-2764E819917E}", "returns an OBB from the position, rotation and half lengths", "Position", "Rotation", "HalfLengths");

        AZ_INLINE BooleanType IsFinite(const OBBType& source)
        {
            return source.IsFinite();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(IsFinite, "Math/OBB", "{DDD8F512-8260-457E-8003-50AC8B86B2D9}", "returns true if every element in Source is finite, is false", "Source");
        
        AZ_INLINE Vector3Type GetAxisX(const OBBType& source)
        {
            return source.GetAxisX();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(GetAxisX, "Math/OBB", "{BAD460A2-D2D2-4191-A442-C5649F09FD54}", "returns the X-Axis of Source", "Source");

        AZ_INLINE Vector3Type GetAxisY(const OBBType& source)
        {
            return source.GetAxisY();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(GetAxisY, "Math/OBB", "{9B593C32-5BAB-4CCA-BB6C-25FD09AD47F7}", "returns the Y-Axis of Source", "Source");

        AZ_INLINE Vector3Type GetAxisZ(const OBBType& source)
        {
            return source.GetAxisZ();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(GetAxisZ, "Math/OBB", "{8818F39D-69E5-4571-920A-4805BA3F6FD9}", "returns the Z-Axis of Source", "Source");

        AZ_INLINE NumberType GetHalfLengthX(const OBBType& source)
        {
            return source.GetHalfLengthX();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(GetHalfLengthX, "Math/OBB", "{3DD48947-E9F3-4A6C-ADB5-5EED51D02871}", "returns the half length of the X-Axis of Source", "Source");
        
        AZ_INLINE NumberType GetHalfLengthY(const OBBType& source)
        {
            return source.GetHalfLengthY();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(GetHalfLengthY, "Math/OBB", "{69E83347-8F86-41AB-84C3-8D32001E597E}", "returns the half length of the Y-Axis of Source", "Source");

        AZ_INLINE NumberType GetHalfLengthZ(const OBBType& source)
        {
            return source.GetHalfLengthZ();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(GetHalfLengthZ, "Math/OBB", "{CEF6E46D-3E81-4BFB-BC05-497245B2C55F}", "returns the half length of the Z-Axis of Source", "Source");

        AZ_INLINE Vector3Type GetPosition(const OBBType& source)
        {
            return source.GetPosition();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(GetPosition, "Math/OBB", "{01907D93-C657-4CA6-A60B-5C0E6319A62D}", "returns the position of Source", "Source");

        using Registrar = RegistrarGeneric
            < FromAabbNode
            , FromPositionRotationAndHalfLengthsNode
            , GetAxisXNode
            , GetAxisYNode
            , GetAxisZNode

#if ENABLE_EXTENDED_MATH_SUPPORT
            , GetHalfLengthXNode
            , GetHalfLengthYNode
            , GetHalfLengthZNode
#endif

            , GetPositionNode
            , IsFiniteNode
            >;

    } // namespace OBBNodes
} // namespace ScriptCanvas

