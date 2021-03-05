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

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/Asset/SimpleAsset.h>

#include "ScriptEventDefinition.h"
#include <AzFramework/Asset/GenericAssetHandler.h>
#include "ScriptEventsBus.h"
#include "ScriptEvent.h"

namespace ScriptEvents
{
    class ScriptEventsAsset
        : public AZ::Data::AssetData
    {
    public:

        AZ_RTTI(ScriptEventsAsset, "{CB4D603E-8CB0-4D80-8165-4244F28AF187}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(ScriptEventsAsset, AZ::SystemAllocator, 0);
        
        ScriptEventsAsset(const AZ::Data::AssetId& assetId = AZ::Data::AssetId(), AZ::Data::AssetData::AssetStatus status = AZ::Data::AssetData::AssetStatus::NotLoaded)
            : AZ::Data::AssetData(assetId, status)
        {
            
        }

        ScriptEventsAsset(const ScriptEventsAsset& rhs)
        {
            m_definition = rhs.m_definition;
        }
        ~ScriptEventsAsset() = default;

        static const char* GetDisplayName() { return "Script Events"; }
        static const char* GetGroup() { return "ScriptEvents"; }
        static const char* GetFileFilter() { return "*.scriptevents"; }
        static const char* GetFileExtension() { return ".scriptevents"; }

        AZ::Crc32 GetBusId() const { return AZ::Crc32(GetId().ToString<AZStd::string>().c_str()); }

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ScriptEventsAsset>()
                    ->Version(1)
                    ->Attribute(AZ::Edit::Attributes::EnableForAssetEditor, true)
                    ->Field("m_definition", &ScriptEventsAsset::m_definition)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<ScriptEventsAsset>("Script Events Asset", "")
                        ->DataElement(0, &ScriptEventsAsset::m_definition, "Definition", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;
                }
            }
        }

        ScriptEvents::ScriptEvent m_definition;
    };

    class ScriptEventsAssetPtr
        : public AZ::Data::Asset<ScriptEventsAsset>
    {
        using BaseType = AZ::Data::Asset<ScriptEventsAsset>;

    public:

        AZ_RTTI(ScriptEventsAssetPtr, "{CE2C30CB-709B-4BC0-BAEE-3D192D33367D}", BaseType);
        AZ_CLASS_ALLOCATOR(ScriptEventsAssetPtr, AZ::SystemAllocator, 0);

        ScriptEventsAssetPtr(AZ::Data::AssetLoadBehavior loadBehavior = AZ::Data::AssetLoadBehavior::PreLoad)
            : AZ::Data::Asset<ScriptEventsAsset>(loadBehavior)
        {}

        ScriptEventsAssetPtr(const BaseType& scriptEventsAsset)
            : BaseType(scriptEventsAsset)
        {}

        virtual ~ScriptEventsAssetPtr() = default;

        using BaseType::BaseType;
        using BaseType::operator=;

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext * serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ScriptEventsAssetPtr>()
                    ;
            }
        }
    };

    // This is the Script Event asset handler used by the builder (and at runtime)
    class ScriptEventAssetRuntimeHandler : public AzFramework::GenericAssetHandler<ScriptEventsAsset>
    {
    public:
        AZ_RTTI(ScriptEventAssetRuntimeHandler, "{002E913D-339A-4238-BCCD-ED077BBD72C5}", AzFramework::GenericAssetHandler<ScriptEventsAsset>);

        ScriptEventAssetRuntimeHandler(const char* displayName, const char* group, const char* extension, const AZ::Uuid& componentTypeId = AZ::Uuid::CreateNull(), AZ::SerializeContext* serializeContext = nullptr)
            : AzFramework::GenericAssetHandler<ScriptEventsAsset>(displayName, group, extension, componentTypeId, serializeContext)
        {
        }

        using AzFramework::GenericAssetHandler<ScriptEventsAsset>::LoadAssetData;

        AZ::Data::AssetHandler::LoadResult LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB) override
        {
            if (AzFramework::GenericAssetHandler<ScriptEventsAsset>::LoadAssetData(asset, stream, assetLoadFilterCB) ==
                AZ::Data::AssetHandler::LoadResult::LoadComplete)
            {
                // Queue the registration against the AssetBus to be run on the main thread
                // This avoids the following deadlock situation by moving RegisterScriptEvent to the main thread only:
                // JobThread: LoadAssetData -> lock ScriptEventBus -> lock AssetBus
                // MainThread: lock AssetBus OnAssetReady -> LoadAssetData -> ...
                AZ::Data::AssetBus::QueueFunction([asset]()
                {
                    const ScriptEvents::ScriptEvent& definition = asset.GetAs<ScriptEventsAsset>()->m_definition;

                    AZStd::intrusive_ptr<Internal::ScriptEvent> scriptEvent;
                    ScriptEvents::ScriptEventBus::BroadcastResult(scriptEvent, &ScriptEvents::ScriptEventRequests::RegisterScriptEvent, asset.GetId(), definition.GetVersion());
                });

                return AZ::Data::AssetHandler::LoadResult::LoadComplete;
            }
            return AZ::Data::AssetHandler::LoadResult::Error;
        }

    };

}

