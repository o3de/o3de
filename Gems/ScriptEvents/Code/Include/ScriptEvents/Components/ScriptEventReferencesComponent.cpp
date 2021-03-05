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

#include "precompiled.h"
#include "ScriptEventReferencesComponent.h"

namespace ScriptEvents
{
    namespace Components
    {
        void ScriptEventReferencesComponent::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                // The Script Event References component is no longer necessary, as all Script Event assets
                // will be properly loaded as needed.
                serializeContext->ClassDeprecate("ScriptEventReferencesComponent", "{D0F440AC-32D4-49EC-8B93-860B188266A6}");
            }
        }

        void ScriptEventReferencesComponent::Activate()
        {
            for (auto& scriptEventReferences : m_scriptEventAssets)
            {
                const auto& asset = scriptEventReferences.GetAsset();
                if (asset)
                {
                    if (!AZ::Data::AssetBus::MultiHandler::BusIsConnectedId(asset.GetId()))
                    {
                        AZ::Data::AssetBus::MultiHandler::BusConnect(asset.GetId());
                    }
                
                    // Load the asset if it's not ready
                    if (!asset.IsReady())
                    {
                        AZ::Data::AssetInfo assetInfo;
                        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, asset.GetId());
                        if (assetInfo.m_assetId.IsValid())
                        {
                            AZ::Data::AssetManager::Instance().GetAsset(asset.GetId(), azrtti_typeid<ScriptEventsAsset>(), AZ::Data::AssetLoadBehavior::Default)
                                .BlockUntilLoadComplete();
                        }
                    }
                }
                else
                {
                    AZ_Warning("Script Events", false, "ScriptEventReferencesComponent could not find Script Event asset: %s", scriptEventReferences.GetDefinition() ? scriptEventReferences.GetDefinition()->GetName().c_str() : scriptEventReferences.GetAsset().GetId().ToString<AZStd::string>().c_str());
                }
            }
        }

        void ScriptEventReferencesComponent::Deactivate()
        {
            for (auto& scriptEventReferences : m_scriptEventAssets)
            {
                const auto& asset = scriptEventReferences.GetAsset();
                if (asset)
                {
                    AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());
                }
            }
        }

        void ScriptEventReferencesComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ScriptEventReference", 0x3df92d40));
        }

        void ScriptEventReferencesComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ScriptEventReference", 0x3df92d40));
        }

        void ScriptEventReferencesComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC("LuaScriptService", 0x21d76c4b));
        }

        void ScriptEventReferencesComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            if (ScriptEventsAsset* scriptEventAsset = asset.GetAs<ScriptEventsAsset>())
            {
                scriptEventAsset->m_definition.RegisterInternal();
            }
        }

    }
}
