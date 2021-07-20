/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TranslationContext.h"

#include <AzCore/Math/Uuid.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include "GraphToLuaUtility.h"

namespace ScriptCanvas
{
    namespace Translation
    {
        AZStd::string Context::GetCategoryLibraryName(const AZStd::string& categoryName)
        {
            AZStd::string safeName = Grammar::ToSafeName(categoryName);
            return AZStd::string::format("%s%s", safeName.c_str(), "_VM");
        }

        const AZStd::string& Context::FindAbbreviation(AZStd::string_view dependency) const
        {
            static AZStd::string empty("");
            auto libNameIter = m_categoryToLibraryName.find(dependency);
            if (libNameIter != m_categoryToLibraryName.end())
            {
                dependency = libNameIter->second;
            }

            auto iter = m_tableAbbreviations.find(dependency);
            return iter != m_tableAbbreviations.end() ? iter->second : empty;
        }
                
        const AZStd::string& Context::FindLibrary(AZStd::string_view dependency) const
        {
            static AZStd::string empty("");
            auto iter = m_categoryToLibraryName.find(dependency);
            return iter != m_categoryToLibraryName.end() ? iter->second : empty;
        }

        void Context::InitializeNames()
        {
            m_categoryToLibraryName =
                { {"Math/AABB", "AABB_VM"}
                , {"Math/CRC", "CRC_VM"}
                , {"Math/Color", "Color_VM"}
                , {"Math/Number", "Math_VM" }
                , {"Math/Matrix3x3", "Matrix3x3_VM"}
                , {"Math/Matrix4x4", "Matrix4x4_VM"}
                , {"Math/OBB", "OBB_VM"}
                , {"Math/Plane", "Plane_VM"}
                , {"Math/Quaternion", "Quaternion_VM"}
                , {"Math/Random", "Random_VM"}
                , {"Math/Transform", "Transform_VM"}
                , {"Math/Vector2", "Vector2_VM"}
                , {"Math/Vector3", "Vector3_VM"}
                , {"Math/Vector4", "Vector4_VM"}
                , {"AABB", "AABB_VM"}
                , {"CRC", "CRC_VM"}
                , {"Color", "Color_VM"}
                , {"Number", "Math_VM" }
                , {"Matrix3x3", "Matrix3x3_VM"}
                , {"Matrix4x4", "Matrix4x4_VM"}
                , {"OBB", "OBB_VM"}
                , {"Plane", "Plane_VM"}
                , {"Quaternion", "Quaternion_VM"}
                , {"Random", "Random_VM"}
                , {"Transform", "Transform_VM"}
                , {"Vector2", "Vector2_VM"}
                , {"Vector3", "Vector3_VM"}
                , {"Vector4", "Vector4_VM"} };

            m_tableAbbreviations =
                { { "AABB_VM", "aabb"}
                , { "CRC_VM", "crc"}
                , { "Color_VM", "color"}
                , { "Math_VM", "math" }
                , { "Matrix3x3_VM", "m3x3"}
                , { "Matrix4x4_VM", "m4x4"}
                , { "OBB_VM", "obb"}
                , { "Plane_VM", "plane"}
                , { "Quaternion_VM", "quat"}
                , { "Random_VM", "rand"}
                , { "Transform_VM", "tm"}
                , { "Vector2_VM", "v2"}
                , { "Vector3_VM", "v3"}
                , { "Vector4_VM", "v4"} };
        }

    } 

} 
