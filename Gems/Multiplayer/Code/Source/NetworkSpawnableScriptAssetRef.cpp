/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Spawnable/SpawnableAssetHandler.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <Multiplayer/NetworkSpawnableScriptAssetRef.h>

namespace Multiplayer
{
    void NetworkSpawnableScriptAssetRef::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<NetworkSpawnableScriptAssetRef, AzFramework::Scripts::SpawnableScriptAssetRef>()
                ->Version(0)
            ;

            serializeContext->RegisterGenericType<AZStd::vector<NetworkSpawnableScriptAssetRef>>();
            serializeContext->RegisterGenericType<AZStd::unordered_map<AZStd::string, NetworkSpawnableScriptAssetRef>>();
            // required to support Map<Number, NetworkSpawnableScriptAssetRef> in Script Canvas
            serializeContext->RegisterGenericType<AZStd::unordered_map<double, NetworkSpawnableScriptAssetRef>>(); 

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext
                    ->Class<NetworkSpawnableScriptAssetRef>(
                        "NetworkSpawnableScriptAssetRef",
                        "A wrapper around a .network.spawnable asset to be used as a variable in Script Canvas.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<NetworkSpawnableScriptAssetRef>("NetworkSpawnableScriptAssetRef")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::EnableAsScriptEventParamType, true)
                ->Attribute(AZ::Script::Attributes::Category, "Prefab/Spawning")
                ->Attribute(AZ::Script::Attributes::Module, "prefabs")
                ->Constructor()
                ->Method("GetAsset", &NetworkSpawnableScriptAssetRef::GetAsset)
                ->Method("SetAsset", &NetworkSpawnableScriptAssetRef::SetAsset);
        }
    }

    bool NetworkSpawnableScriptAssetRef::ShowProductAssetFileName() const
    {
        // Show the product asset name on the component so that it's clear that we chose a networked spawnable.
        return true;
    }

    bool NetworkSpawnableScriptAssetRef::HideProductAssetFiles() const
    {
        // Show product asset files in the asset picker so that we can pick a .network.spawnable file.
        return false;
    }

    const char* NetworkSpawnableScriptAssetRef::GetAssetPickerTitle() const
    {
        // Call the dialog "Pick Network Spawnable Asset"
        return "Network Spawnable Asset";
    }

    AZ::Outcome<void, AZStd::string> NetworkSpawnableScriptAssetRef::ValidatePotentialSpawnableAsset(
        void* newValue, const AZ::Uuid& valueType) const
    {
        // Only allow .network.spawnable files to get selected.
        return NetworkSpawnable::ValidatePotentialSpawnableAsset(newValue, valueType);
    }

    NetworkSpawnable NetworkSpawnableScriptAssetRef::GetAsset() const
    {
        return NetworkSpawnable(SpawnableScriptAssetRef::GetAsset());
    }

    void NetworkSpawnableScriptAssetRef::SetAsset(NetworkSpawnable& asset)
    {
        SpawnableScriptAssetRef::SetAsset(asset.m_spawnableAsset);
    }

} // namespace Multiplayer
