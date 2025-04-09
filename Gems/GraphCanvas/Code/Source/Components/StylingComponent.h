/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>

#include <GraphCanvas/Components/EntitySaveDataBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Styling/definitions.h>
#include <GraphCanvas/Styling/Selector.h>
#include <GraphCanvas/Types/EntitySaveData.h>

namespace GraphCanvas
{
    //! Implements a base \ref StyledEntityRequestBus::Handler for entities that have a "root visual"
    //! (QGraphicsItem/QGraphicsLayoutItem).
    class StylingComponent
        : public AZ::Component
        , public VisualNotificationBus::Handler
        , public StyledEntityRequestBus::Handler
        , public SceneMemberNotificationBus::Handler
        , public SceneNotificationBus::Handler
        , public EntitySaveDataRequestBus::Handler
    {
    public:
        class StylingComponentSaveData
            : public ComponentSaveData
        {
        public:
            AZ_RTTI(StylingComponentSaveData, "{B0B99C8A-03AF-4CF6-A926-F65C874C3D97}", ComponentSaveData);
            AZ_CLASS_ALLOCATOR(StylingComponentSaveData, AZ::SystemAllocator);

            StylingComponentSaveData() = default;
            StylingComponentSaveData(const AZStd::string& subStyle);
            ~StylingComponentSaveData() = default;

            AZStd::string m_subStyle;
        };

        AZ_COMPONENT(StylingComponent, "{94BF24F3-0EF1-41D9-B869-27AAB2B7F9AF}");
        static void Reflect(AZ::ReflectContext*);

        static AZ::EntityId CreateStyleEntity(const AZStd::string& element);

        StylingComponent() {}
        StylingComponent(const AZStd::string& element, const AZ::EntityId& parentStyledEntity = AZ::EntityId(), const AZStd::string& styleClass = AZStd::string());
        ~StylingComponent() override = default;

        // AZ::Component
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(StyledGraphicItemServiceCrc);
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(StyledGraphicItemServiceCrc);
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            (void)required;
        }

        void Activate() override;
        void Deactivate() override;
        ////
    
        // VisualNotificationBus
        void OnItemChange(const AZ::EntityId&, QGraphicsItem::GraphicsItemChange, const QVariant&) override;
        ////

        // StyledEntityRequestBus
        AZ::EntityId GetStyleParent() const override;

        Styling::SelectorVector GetStyleSelectors() const override;

        void AddSelectorState(const char* selector) override;
        void RemoveSelectorState(const char* selector) override;

        AZStd::string GetElement() const override;
        AZStd::string GetClass() const override;
        ////

        // SceneMemberNotificationBus
        void OnSceneSet(const AZ::EntityId& scene) override;
        void OnRemovedFromScene(const AZ::EntityId& scene) override;
        ////

        // SceneNotificationBus
        void OnStylesChanged() override;
        ////

        // EntitySaveDataRequestBus
        void WriteSaveData(EntitySaveDataContainer& saveDataContainer) const override;
        void ReadSaveData(const EntitySaveDataContainer& saveDataContainer) override;
        ////

    private:

        const AZ::EntityId m_parentStyledEntity;

        AZStd::string m_element;
        StylingComponentSaveData m_saveData;

        // The selectors for the element, class and ID
        Styling::SelectorVector m_coreSelectors;

        // These are refreshed on Activate and used to generate the set of current selectors
        Styling::Selector m_selectedSelector;
        Styling::Selector m_disabledSelector;
        Styling::Selector m_hoveredSelector;
        Styling::Selector m_collapsedSelector;
        Styling::Selector m_highlightedSelector;

        AZStd::unordered_map<AZ::Crc32, Styling::Selector> m_dynamicSelectors;

        bool m_hovered = false;
    };
}
