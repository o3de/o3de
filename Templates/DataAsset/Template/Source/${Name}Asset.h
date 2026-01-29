// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace ${GemName}
{
    /*
    * TODO: Register this component in your Gem's AZ::Module interface by inserting the following into the list of m_descriptors:
    *       ${SanitizedCppName}Asset::CreateDescriptor(),
    */

    class ${SanitizedCppName}Asset
        : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(${SanitizedCppName}Asset, "{${Random_Uuid}}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(${SanitizedCppName}Asset, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        // Replace this variable with ANY other variable you intend to expose through Reflection().
        // This is in order to make this Asset visible to the Editor. No variables prevents it from appearing.
        bool singleVariable = true;
    };
} // namespace ${GemName}
