/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzFramework/Asset/GenericAssetHandler.h>
#include <AzFramework/Asset/SimpleAsset.h>

#include "ScriptEventDefinition.h"

namespace AZ
{
    class ReflectContext;
}

namespace ScriptEvents
{
    constexpr const char* k_builderJobKey = "Script Events";

    class ScriptEventsAsset
        : public AZ::Data::AssetData
    {
    public:

        AZ_RTTI(ScriptEventsAsset, "{CB4D603E-8CB0-4D80-8165-4244F28AF187}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(ScriptEventsAsset, AZ::SystemAllocator);

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
        static const char* GetFileFilter() { return "scriptevents"; }

        AZ::Crc32 GetBusId() const { return AZ::Crc32(GetId().ToString<AZStd::string>().c_str()); }

        static void Reflect(AZ::ReflectContext* context);

        ScriptEvents::ScriptEvent m_definition;
    };

    class ScriptEventsAssetPtr
        : public AZ::Data::Asset<ScriptEventsAsset>
    {
        using BaseType = AZ::Data::Asset<ScriptEventsAsset>;

    public:

        AZ_RTTI(ScriptEventsAssetPtr, "{CE2C30CB-709B-4BC0-BAEE-3D192D33367D}", BaseType);
        AZ_CLASS_ALLOCATOR(ScriptEventsAssetPtr, AZ::SystemAllocator);

        explicit ScriptEventsAssetPtr(AZ::Data::AssetLoadBehavior loadBehavior = AZ::Data::AssetLoadBehavior::PreLoad)
            : AZ::Data::Asset<ScriptEventsAsset>(loadBehavior)
        {}

        ScriptEventsAssetPtr(const ScriptEventsAssetPtr& scriptEventsAsset) = default;
        ScriptEventsAssetPtr(ScriptEventsAssetPtr&& scriptEventsAsset) = default;

        virtual ~ScriptEventsAssetPtr() = default;

        using BaseType::BaseType;

        ScriptEventsAssetPtr& operator=(const ScriptEventsAssetPtr&) = default;
        ScriptEventsAssetPtr& operator=(ScriptEventsAssetPtr&&) = default;

        static void Reflect(AZ::ReflectContext* context);
    };

    // This is the Script Event asset handler used by the builder (and at runtime)
    class ScriptEventAssetRuntimeHandler : public AzFramework::GenericAssetHandler<ScriptEventsAsset>
    {
    public:
        AZ_RTTI(ScriptEventAssetRuntimeHandler, "{002E913D-339A-4238-BCCD-ED077BBD72C5}", AzFramework::GenericAssetHandler<ScriptEventsAsset>);

        ScriptEventAssetRuntimeHandler(const char* displayName, const char* group, const char* extension, const AZ::Uuid& componentTypeId = AZ::Uuid::CreateNull(), AZ::SerializeContext* serializeContext = nullptr);

        void InitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset, bool loadStageSucceeded, bool isReload) override;
    };

}

