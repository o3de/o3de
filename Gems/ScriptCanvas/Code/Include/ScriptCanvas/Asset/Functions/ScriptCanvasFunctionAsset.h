/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <ScriptCanvas/Asset/AssetDescription.h>
#include <ScriptCanvas/Asset/ScriptCanvasAssetBase.h>

#include <AzCore/std/containers/vector.h>

namespace AZ
{
    class Entity;
}

namespace ScriptCanvasEditor
{
    class ScriptCanvasFunctionAsset;
    class Graph;

    class ScriptCanvasFunctionDescription : public ScriptCanvas::AssetDescription
    {
    public:

        AZ_TYPE_INFO(ScriptCanvasFunctionDescription, "{B53569F6-8408-40FC-9A72-ED873BEF162E}");

        ScriptCanvasFunctionDescription()
            : AssetDescription(
                azrtti_typeid<ScriptCanvasEditor::ScriptCanvasFunctionAsset>(),
                "Script Canvas Function",
                "Script Canvas Function Graph Asset",
                "@devassets@/scriptcanvas/functions",
                ".scriptcanvas_fn",
                "Script Canvas Function",
                "Untitled-Function-%i",
                "Script Canvas Function Files (*.scriptcanvas_fn)",
                "Script Canvas Function",
                "Script Canvas Function",
                "Icons/ScriptCanvas/Viewport/ScriptCanvas_Function.png",
                AZ::Color(0.192f, 0.149f, 0.392f, 1.0f),
                true
                )
        {}
    };

    // TODO-LS: move these to their own file
    class ScriptCanvasDataRequests : public AZ::ComponentBus
    {
    public:

        virtual void SetPrettyName(const char* name) = 0;
        virtual AZStd::string GetPrettyName() = 0;
    };

    using ScriptCanvasDataRequestBus = AZ::EBus<ScriptCanvasDataRequests>;


    class ScriptCanvasFunctionDataComponent
        : public AZ::Component
        , ScriptCanvasDataRequestBus::MultiHandler
    {
    public:
        AZ_COMPONENT(ScriptCanvasFunctionDataComponent, "{440BB6DC-4E70-4304-A926-252925F77433}");

        void Activate() override
        {
            AZ::EntityId entityId = GetEntityId();
            ScriptCanvasDataRequestBus::MultiHandler::BusConnect(entityId);
        }

        void Deactivate() override
        {
            ScriptCanvasDataRequestBus::MultiHandler::BusDisconnect();

            AZ::EntityId entityId = GetEntityId();
        }

        void SetPrettyName(const char* name) override
        {
            m_assetPrettyName = name;
        }

        AZStd::string GetPrettyName() override
        {
            return m_assetPrettyName;
        }

        static void Reflect(AZ::ReflectContext* reflectContext)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
            {
                serializeContext->Class<ScriptCanvasFunctionDataComponent, AZ::Component>()
                    ->Version(2)
                    ->Field("m_assetPrettyName", &ScriptCanvasFunctionDataComponent::m_assetPrettyName)
                    ->Field("m_version", &ScriptCanvasFunctionDataComponent::m_functionDataComponentVersion)
                    ;
            }
        }

        size_t m_functionDataComponentVersion = 0;
        AZStd::string m_assetPrettyName;
    };

    class ScriptCanvasFunctionAsset
        : public ScriptCanvas::ScriptCanvasAssetBase
    {
    public:
        AZ_RTTI(ScriptCanvasFunctionAsset, "{ED078D3C-938D-41F8-A5F6-CC04311ECF4F}", ScriptCanvasAssetBase);
        AZ_CLASS_ALLOCATOR(ScriptCanvasFunctionAsset, AZ::SystemAllocator, 0);

        ScriptCanvasFunctionAsset(const AZ::Data::AssetId& assetId = AZ::Data::AssetId(AZ::Uuid::CreateRandom()), AZ::Data::AssetData::AssetStatus status = AZ::Data::AssetData::AssetStatus::NotLoaded)
            : ScriptCanvasAssetBase(assetId, status)
        {
            m_data = aznew ScriptCanvas::ScriptCanvasData();
        }

        ~ScriptCanvasFunctionAsset() override
        {}

        ScriptCanvas::AssetDescription GetAssetDescription() const override
        {
            return ScriptCanvasFunctionDescription();
        }

        using Description = ScriptCanvasFunctionDescription;

        ScriptCanvasFunctionDataComponent* m_cachedComponent = nullptr;

        ScriptCanvasFunctionDataComponent* GetFunctionData()
        {
            if (m_cachedComponent == nullptr)
            {
                m_cachedComponent = GetScriptCanvasEntity()->FindComponent<ScriptCanvasFunctionDataComponent>();
            }

            return m_cachedComponent;
        }
    };

}
