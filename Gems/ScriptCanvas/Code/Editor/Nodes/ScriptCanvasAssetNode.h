/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Bus/GraphBus.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <Editor/Assets/ScriptCanvasAssetInstance.h>

namespace ScriptCanvasEditor
{
    //! A group of entities in a scene
    class ScriptCanvasAssetNode
        : public ScriptCanvas::Node
        , protected AZ::Data::AssetBus::Handler
    {
    public:
        AZ_COMPONENT(ScriptCanvasAssetNode, "{65A34956-B6ED-4EB2-966C-5BC844F7B05E}", ScriptCanvas::Node);

        static void Reflect(AZ::ReflectContext* context);

        ScriptCanvasAssetNode() = default;
        ScriptCanvasAssetNode(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset, bool storeAssetDataInternally);
        ~ScriptCanvasAssetNode() override;

        // AZ::Component
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ScriptCanvas_AssetService", 0x17a357ae));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ScriptCanvas_AssetService", 0x17a357ae));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            (void)required;
        }
        ////

        void OnInit() override;

        // Retrieves the asset associated with this node
        AZ::Data::Asset<ScriptCanvasAsset>& GetAsset();
        const AZ::Data::Asset<ScriptCanvasAsset>& GetAsset() const;
        // Sets the asset associated with this node
        void SetAsset(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset);

        bool GetAssetDataStoredInternally() const;
        void SetAssetDataStoredInternally(bool storeInObjectStream);

        ScriptCanvas::ScriptCanvasData& GetScriptCanvasData();
        const ScriptCanvas::ScriptCanvasData& GetScriptCanvasData() const;

        AZ::Entity* GetScriptCanvasEntity() const;

        using VisitCB = AZStd::function<bool(ScriptCanvasAssetNode& scriptCanvasAssetNode)>;
        bool Visit(const VisitCB& preVisitCB, const VisitCB & postVisitCB);
        bool Visit(const VisitCB& preVisitCB, const VisitCB & postVisitCB, AZStd::unordered_set<AZ::Data::AssetId>& visitedGraphs);
        ////

    protected:
        void OnInputSignal(const ScriptCanvas::SlotId&) override;
        // AssetBus::Handler
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetUnloaded(const AZ::Data::AssetId assetId, const AZ::Data::AssetType) override;
        ////

        friend class ScriptCanvasAssetNodeEventHandler;
        void ComputeDataPatch();
        void ApplyDataPatch();

    private:
        ScriptCanvasAssetNode(const ScriptCanvasAssetNode&) = delete;
        //! References to the Script Canvas asset used by this component
        ScriptCanvasAssetInstance m_scriptCanvasAssetInstance;
    };
}
