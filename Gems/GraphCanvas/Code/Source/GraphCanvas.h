/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Module/Environment.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Styling/PseudoElement.h>

#include <Translation/TranslationDatabase.h>
#include <Translation/TranslationBuilder.h>

namespace GraphCanvas
{
    class GraphCanvasSystemComponent
        : public AZ::Component
        , private GraphCanvasRequestBus::Handler
        , protected Styling::PseudoElementFactoryRequestBus::Handler
        , protected AzFramework::AssetCatalogEventBus::Handler
        , protected AZ::Data::AssetBus::MultiHandler

    {
    public:
        AZ_COMPONENT(GraphCanvasSystemComponent, "{F9F7BE55-4C28-4B8A-A722-D47C9EF24E60}")

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        GraphCanvasSystemComponent() = default;
        ~GraphCanvasSystemComponent() override = default;

    private:
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        // GraphCanvasRequestBus
        AZ::Entity* CreateBookmarkAnchor() const override;

        AZ::Entity* CreateScene() const override;

        AZ::Entity* CreateCoreNode() const override;
        AZ::Entity* CreateGeneralNode(const char* nodeType) const override;
        AZ::Entity* CreateCommentNode() const override;
        AZ::Entity* CreateWrapperNode(const char* nodeType) const override;

        AZ::Entity* CreateNodeGroup() const override;
        AZ::Entity* CreateCollapsedNodeGroup(const CollapsedNodeGroupConfiguration& groupedNodeConfiguration) const override;

        AZ::Entity* CreateSlot(const AZ::EntityId& nodeId, const SlotConfiguration& slotConfiguration) const override;

        NodePropertyDisplay* CreateBooleanNodePropertyDisplay(BooleanDataInterface* dataInterface) const override;
        NodePropertyDisplay* CreateNumericNodePropertyDisplay(NumericDataInterface* dataInterface) const override;
        NodePropertyDisplay* CreateComboBoxNodePropertyDisplay(ComboBoxDataInterface* dataInterface) const override;
        NodePropertyDisplay* CreateEntityIdNodePropertyDisplay(EntityIdDataInterface* dataInterface) const override;
        NodePropertyDisplay* CreateReadOnlyNodePropertyDisplay(ReadOnlyDataInterface* dataInterface) const override;
        NodePropertyDisplay* CreateStringNodePropertyDisplay(StringDataInterface* dataInterface) const override;
        NodePropertyDisplay* CreateVectorNodePropertyDisplay(VectorDataInterface* dataInterface) const override;
        NodePropertyDisplay* CreateAssetIdNodePropertyDisplay(AssetIdDataInterface* dataInterface) const override;

        AZ::Entity* CreatePropertySlot(const AZ::EntityId& nodeId, const AZ::Crc32& propertyId, const SlotConfiguration& slotConfiguration) const override;
        ////

        // PseudoElementRequestBus
        AZ::EntityId CreateStyleEntity(const AZStd::string& style) const override;
        AZ::EntityId CreateVirtualChild(const AZ::EntityId& real, const AZStd::string& virtualChildElement) const override;
        ////

        // AzFramework::AssetCatalogEventBus::Handler
        void OnCatalogLoaded(const char* /*catalogFile*/) override;
        void OnCatalogAssetChanged(const AZ::Data::AssetId&) override;
        ////

        AZStd::unique_ptr<TranslationAssetHandler> m_assetHandler;

        void RegisterTranslationBuilder();
        void UnregisterAssetHandler();
        TranslationAssetWorker m_translationAssetWorker;
        AZStd::vector<AZ::Data::AssetId> m_translationAssets;
        void PopulateTranslationDatabase();

        TranslationDatabase m_translationDatabase;
    };
}
