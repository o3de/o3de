/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

//AZTF-SHARED
#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/std/containers/unordered_set.h>

namespace AZ::Data
{
    class AssetData;
}

namespace AZStd
{
    template<>
    struct hash<AZ::Data::Asset<AZ::Data::AssetData>>
    {
        size_t operator()(const AZ::Data::Asset<AZ::Data::AssetData>& asset) const
        {
            return asset.GetId().m_guid.GetHash();
        }
    };
}

namespace AzToolsFramework
{
    class InstanceDataNode;

    namespace AssetEditor
    {
        // This class is used to track all of the asset editors that were open.  When the editor launches
        // these settings will be used to preemptively register all windows and then open them.
        struct AZTF_API AssetEditorWindowSettings : AZ::UserSettings
        {
            AZ_CLASS_ALLOCATOR(AssetEditorWindowSettings, AZ::SystemAllocator);
            AZ_RTTI(AssetEditorWindowSettings, "{981FE4FF-0B56-4115-9F75-79609E3D6337}", AZ::UserSettings);

            //! The list of opened assets is used to restore their window state
            AZStd::unordered_set<AZ::Data::Asset<AZ::Data::AssetData>> m_openAssets;

            static constexpr const char* s_name = "AssetEditorWindowSettings";

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace AssetEditor
} // namespace AzToolsFramework
