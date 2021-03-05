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

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace MaterialEditor
{
    //! MaterialTypeEntry describes a materialType file that can be used to create new material
    struct MaterialTypeEntry final
    {
        AZ_TYPE_INFO(MaterialTypeEntry, "{ECD408E0-61B7-4553-B966-6131820B684E}");
        AZ_CLASS_ALLOCATOR(MaterialTypeEntry, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* context);

        AZStd::string m_displayName;
        AZStd::string m_filePath;
    };

    //! MaterialTypesLibrary is a collection of MaterialTypeEntries
    struct MaterialTypesLibrary final
    {
        AZ_TYPE_INFO(MaterialTypesLibrary, "{C062C6F6-2AFB-4695-9E31-2AD5067A9073}");
        AZ_CLASS_ALLOCATOR(MaterialTypesLibrary, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* context);

        AZStd::vector<MaterialTypeEntry> m_entries;
    };
}
