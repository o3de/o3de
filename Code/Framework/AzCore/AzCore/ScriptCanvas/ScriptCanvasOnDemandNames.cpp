/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/ScriptCanvas/ScriptCanvasOnDemandNames.h>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Plane.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace AZ::ScriptCanvasOnDemandReflection
{
    // the use of this might have to come at the end of on demand reflection...instead of instantly
    // basically, it required that dependent classes are reflected first, I'm not sure they are yet.
    AZStd::string GetPrettyNameForAZTypeId(AZ::BehaviorContext& context, AZ::Uuid typeId)
    {
        // return capitalized versions of what we need, otherwise just the regular name
        // then strip all the stuff
        if (typeId == azrtti_typeid<Aabb>())
        {
            return "AABB";
        }
        else if (typeId == azrtti_typeid<bool>())
        {
            return "Boolean";
        }
        else if (typeId == azrtti_typeid<Color>())
        {
            return "Color";
        }
        else if (typeId == azrtti_typeid<Crc32>())
        {
            return "CRC";
        }
        else if (typeId == azrtti_typeid<EntityId>())
        {
            return "EntityId";
        }
        else if (typeId == azrtti_typeid<Matrix3x3>())
        {
            return "Matrix3x3";
        }
        else if (typeId == azrtti_typeid<Matrix4x4>())
        {
            return "Matrix4x4";
        }
        else if (typeId == azrtti_typeid<AZ::s8>())
        {
            return "Number:s8";
        }
        else if (typeId == azrtti_typeid<AZ::s16>())
        {
            return "Number:s16";
        }
        else if (typeId == azrtti_typeid<AZ::s32>())
        {
            return "Number:s32";
        }
        else if (typeId == azrtti_typeid<AZ::s64>())
        {
            return "Number:s64";
        }
        else if (typeId == azrtti_typeid<AZ::u8>())
        {
            return "Number:u8";
        }
        else if (typeId == azrtti_typeid<AZ::u16>())
        {
            return "Number:u16";
        }
        else if (typeId == azrtti_typeid<AZ::u32>())
        {
            return "Number:u32";
        }
        else if (typeId == azrtti_typeid<AZ::u64>())
        {
            return "Number:u64";
        }
        else if (typeId == azrtti_typeid<float>())
        {
            return "Number:float";
        }
        else if (typeId == azrtti_typeid<double>())
        {
            return "Number";
        }
        else if (typeId == azrtti_typeid<Obb>())
        {
            return "OBB";
        }
        else if (typeId == azrtti_typeid<Plane>())
        {
            return "Plane";
        }
        else if (typeId == azrtti_typeid<Quaternion>())
        {
            return "Quaternion";
        }
        else if (typeId == azrtti_typeid<AZStd::string>() || typeId == azrtti_typeid<AZStd::string_view>())
        {
            return "String";
        }
        else if (typeId == azrtti_typeid<Transform>())
        {
            return "Transform";
        }
        else if (typeId == azrtti_typeid<Vector2>())
        {
            return "Vector2";
        }
        else if (typeId == azrtti_typeid<Vector3>())
        {
            return "Vector3";
        }
        else if (typeId == azrtti_typeid<Vector4>())
        {
            return "Vector4";
        }
        else
        {
            auto bcClassIter = context.m_typeToClassMap.find(typeId);
            if (bcClassIter != context.m_typeToClassMap.end())
            {
                const AZ::BehaviorClass& bcClass = *(bcClassIter->second);
                AZStd::string uglyName = bcClass.m_name;
                AZ::StringFunc::Replace(uglyName, "AZStd::", "", true);
                AZ::StringFunc::Replace(uglyName, "AZ::", "", true);
                AZ::StringFunc::Replace(uglyName, "::", ".", true);
                return uglyName;
            }
            else
            {
                return "Invalid";
            }
        }
    }
} // namespace AZ::ScriptCanvasOnDemandReflection
