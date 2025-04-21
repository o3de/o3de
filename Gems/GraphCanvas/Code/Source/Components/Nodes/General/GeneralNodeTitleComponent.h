/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QGraphicsLinearLayout>
#include <QGraphicsWidget>

#include <AzCore/Component/Component.h>

#include <GraphCanvas/Components/GraphCanvasPropertyBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Types/EntitySaveData.h>
#include <Widgets/GraphCanvasLabel.h>

namespace GraphCanvas
{
    class GeneralNodeTitleGraphicsWidget;

    //! The Title component gives a Node the ability to display a title.
    class GeneralNodeTitleComponent
        : public AZ::Component
        , public NodeTitleRequestBus::Handler
        , public SceneMemberNotificationBus::Handler
        , public VisualNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(GeneralNodeTitleComponent, "{67D54B26-A924-4028-8544-5684B16BF04A}");
        static void Reflect(AZ::ReflectContext*);

        GeneralNodeTitleComponent();
        ~GeneralNodeTitleComponent() = default;

        // AZ::Component
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(NodeTitleServiceCrc);
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(NodeTitleServiceCrc);
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("GraphCanvas_StyledGraphicItemService"));
            required.push_back(AZ_CRC_CE("GraphCanvas_SceneMemberService"));
        }
        
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        // NodeTitleRequestBus
        void SetDetails(const AZStd::string& title, const AZStd::string& subtitle) override;
        void SetTitle(const AZStd::string& title) override;
        AZStd::string GetTitle() const override;

        void SetSubTitle(const AZStd::string& subtitle) override;
        AZStd::string GetSubTitle() const override;

        QGraphicsWidget* GetGraphicsWidget() override;

        void SetDefaultPalette(const AZStd::string& palette) override;

        void SetPaletteOverride(const AZStd::string& paletteOverride) override;
        void SetDataPaletteOverride(const AZ::Uuid& uuid) override;
        void SetColorPaletteOverride(const QColor& color) override;

        void ConfigureIconConfiguration(PaletteIconConfiguration& paletteConfiguration) override;

        void ClearPaletteOverride() override;
        /////

        // SceneMemberNotificationBus
        void OnSceneSet(const AZ::EntityId& graphId) override;
        ////

    private:
        GeneralNodeTitleComponent(const GeneralNodeTitleComponent&) = delete;

        AZStd::string m_title;
        AZStd::string m_subTitle;

        AZStd::string          m_basePalette;

        GeneralNodeTitleComponentSaveData m_saveData;
        
        GeneralNodeTitleGraphicsWidget* m_generalNodeTitleWidget = nullptr;
    };    

    //! The Title QGraphicsWidget for displaying a title
    class GeneralNodeTitleGraphicsWidget
        : public QGraphicsWidget
        , public SceneNotificationBus::Handler
        , public SceneMemberNotificationBus::Handler
        , public NodeNotificationBus::Handler
        , public RootGraphicsItemNotificationBus::Handler
    {
    public:
        AZ_TYPE_INFO(GeneralNodeTitleGraphicsWidget, "{9DE7D3C0-D88C-47D8-85D4-5E0F619E60CB}");
        AZ_CLASS_ALLOCATOR(GeneralNodeTitleGraphicsWidget, AZ::SystemAllocator);        

        GeneralNodeTitleGraphicsWidget(const AZ::EntityId& entityId);
        ~GeneralNodeTitleGraphicsWidget() override;

        void Initialize();

        void Activate();
        void Deactivate();

        void SetDetails(const AZStd::string& title, const AZStd::string& subtitle);
        void SetTitle(const AZStd::string& title);
        void SetSubTitle(const AZStd::string& subtitle);

        void SetPaletteOverride(AZStd::string_view paletteOverride);
        void SetPaletteOverride(const AZ::Uuid& uuid);
        void SetPaletteOverride(const QColor& color);

        void ConfigureIconConfiguration(PaletteIconConfiguration& paletteConfiguration);

        void ClearPaletteOverride();

        void UpdateLayout();

        void UpdateStyles();
        void RefreshDisplay();
    
        // SceneNotificationBus
        void OnStylesChanged() override;
        ////

        // SceneMemberNotificationBus
        void OnAddedToScene(const AZ::EntityId& scene) override;
        void OnRemovedFromScene(const AZ::EntityId& scene) override;
        ////

        // NodeNotificationBus
        void OnTooltipChanged(const AZStd::string& tooltip) override;
        ////

        // RootGraphicsItemNotifications
        void OnEnabledChanged(RootGraphicsItemEnabledState enabledState) override;
        ////

    protected:

        // QGraphicsItem
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

        const AZ::EntityId& GetEntityId() const { return m_entityId; }

    private:
        GeneralNodeTitleGraphicsWidget(const GeneralNodeTitleGraphicsWidget&) = delete;

        QGraphicsLinearLayout* m_linearLayout;
        GraphCanvasLabel* m_titleWidget;
        GraphCanvasLabel* m_subTitleWidget;

        AZ::EntityId m_entityId;

        const Styling::StyleHelper* m_disabledPalette;
        const Styling::StyleHelper* m_paletteOverride;
        Styling::StyleHelper* m_colorOverride;        

        Styling::StyleHelper m_styleHelper;
    };
}
