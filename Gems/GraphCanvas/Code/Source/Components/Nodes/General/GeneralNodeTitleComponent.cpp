/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QGraphicsItem>
#include <QFont>
#include <QPainter>

#include <AzCore/Serialization/EditContext.h>

#include <Components/Nodes/General/GeneralNodeTitleComponent.h>

#include <Components/Nodes/General/GeneralNodeFrameComponent.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Editor/GraphCanvasProfiler.h>
#include <GraphCanvas/tools.h>
#include <GraphCanvas/Styling/StyleHelper.h>
#include <GraphCanvas/Utils/QtDrawingUtils.h>
#include <GraphCanvas/Components/VisualBus.h>


namespace GraphCanvas
{
    // Implementation of RTTI functions for the GeneralNodeTitleComponent SaveData class in a cpp file
    AZ_RTTI_NO_TYPE_INFO_IMPL(GeneralNodeTitleComponentSaveData, SceneMemberComponentSaveData<GeneralNodeTitleComponentSaveData>);

    //////////////////////////////
    // GeneralNodeTitleComponent
    //////////////////////////////

    void GeneralNodeTitleComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<GeneralNodeTitleComponentSaveData, ComponentSaveData>()
                ->Version(1)
                ->Field("PaletteOverride", &GeneralNodeTitleComponentSaveData::m_paletteOverride)
            ;

            serializeContext->Class<GeneralNodeTitleComponent, AZ::Component >()
                ->Version(4)
                ->Field("Title", &GeneralNodeTitleComponent::m_title)
                ->Field("SubTitle", &GeneralNodeTitleComponent::m_subTitle)
                ->Field("SaveData", &GeneralNodeTitleComponent::m_saveData)
                ->Field("DefaultPalette", &GeneralNodeTitleComponent::m_basePalette)
                ;
        }
    }

    GeneralNodeTitleComponent::GeneralNodeTitleComponent()
    {
    }

    void GeneralNodeTitleComponent::Init()
    {
        m_generalNodeTitleWidget = aznew GeneralNodeTitleGraphicsWidget(GetEntityId());
    }

    void GeneralNodeTitleComponent::Activate()
    {
        AZ::EntityId entityId = GetEntityId();
        m_saveData.Activate(entityId);
        SceneMemberNotificationBus::Handler::BusConnect(entityId);
        NodeTitleRequestBus::Handler::BusConnect(entityId);
        VisualNotificationBus::Handler::BusConnect(entityId);

        if (m_generalNodeTitleWidget)
        {
            m_generalNodeTitleWidget->SetTitle(m_title);
            m_generalNodeTitleWidget->SetSubTitle(m_subTitle);

            m_generalNodeTitleWidget->UpdateLayout();

            m_generalNodeTitleWidget->Activate();
        }
    }

    void GeneralNodeTitleComponent::Deactivate()
    {
        if (m_generalNodeTitleWidget)
        {
            m_generalNodeTitleWidget->Deactivate();
        }

        SceneMemberNotificationBus::Handler::BusDisconnect();
        NodeTitleRequestBus::Handler::BusDisconnect();
        VisualNotificationBus::Handler::BusDisconnect();

    }

    void GeneralNodeTitleComponent::SetDetails(const AZStd::string& title, const AZStd::string& subtitle)
    {
        m_title = title;
        m_subTitle = subtitle;

        if (m_generalNodeTitleWidget)
        {
            m_generalNodeTitleWidget->SetDetails(title, subtitle);
        }

    }

    void GeneralNodeTitleComponent::SetTitle(const AZStd::string& title)
    {
        m_title = title;

        if (m_generalNodeTitleWidget)
        {
            m_generalNodeTitleWidget->SetTitle(title);
        }
    }

    AZStd::string GeneralNodeTitleComponent::GetTitle() const
    {
        return m_title;
    }

    void GeneralNodeTitleComponent::SetSubTitle(const AZStd::string& subtitle)
    {
        m_subTitle = subtitle;

        if (m_generalNodeTitleWidget)
        {
            m_generalNodeTitleWidget->SetSubTitle(subtitle);
        }
    }

    AZStd::string GeneralNodeTitleComponent::GetSubTitle() const
    {
        return m_subTitle;
    }

    QGraphicsWidget* GeneralNodeTitleComponent::GetGraphicsWidget()
    {
        return m_generalNodeTitleWidget;
    }

    void GeneralNodeTitleComponent::SetDefaultPalette(const AZStd::string& basePalette)
    {
        if (m_generalNodeTitleWidget)
        {
            m_basePalette = basePalette;
            m_generalNodeTitleWidget->SetPaletteOverride(basePalette);
        }
    }

    void GeneralNodeTitleComponent::SetPaletteOverride(const AZStd::string& paletteOverride)
    {
        m_saveData.m_paletteOverride = paletteOverride;
        m_saveData.SignalDirty();

        if (m_generalNodeTitleWidget)
        {
            m_generalNodeTitleWidget->SetPaletteOverride(paletteOverride);
        }
    }

    void GeneralNodeTitleComponent::SetDataPaletteOverride(const AZ::Uuid& uuid)
    {
        if (m_generalNodeTitleWidget)
        {
            m_generalNodeTitleWidget->SetPaletteOverride(uuid);
        }
    }

    void GeneralNodeTitleComponent::SetColorPaletteOverride(const QColor& color)
    {
        if (m_generalNodeTitleWidget)
        {
            m_generalNodeTitleWidget->SetPaletteOverride(color);
        }
    }

    void GeneralNodeTitleComponent::ConfigureIconConfiguration(PaletteIconConfiguration& paletteConfiguration)
    {
        if (m_generalNodeTitleWidget)
        {
            m_generalNodeTitleWidget->ConfigureIconConfiguration(paletteConfiguration);
        }
    }

    void GeneralNodeTitleComponent::ClearPaletteOverride()
    {
        m_saveData.m_paletteOverride = "";
        m_saveData.SignalDirty();

        if (m_generalNodeTitleWidget)
        {
            m_generalNodeTitleWidget->ClearPaletteOverride();
        }
    }

    void GeneralNodeTitleComponent::OnSceneSet(const AZ::EntityId& /*graphId*/)
    {
        if (!m_saveData.m_paletteOverride.empty())
        {
            if (m_generalNodeTitleWidget)
            {
                m_generalNodeTitleWidget->SetPaletteOverride(m_saveData.m_paletteOverride);
            }
        }
        else if (!m_basePalette.empty())
        {
            if (m_generalNodeTitleWidget)
            {
                m_generalNodeTitleWidget->SetPaletteOverride(m_basePalette);
            }
        }
    }

    ///////////////////////////////////
    // GeneralNodeTitleGraphicsWidget
    ///////////////////////////////////
    GeneralNodeTitleGraphicsWidget::GeneralNodeTitleGraphicsWidget(const AZ::EntityId& entityId)
        : m_entityId(entityId)
        , m_paletteOverride(nullptr)
        , m_colorOverride(nullptr)
    {
        Initialize();
    }

    GeneralNodeTitleGraphicsWidget::~GeneralNodeTitleGraphicsWidget()
    {
        delete m_colorOverride;
    }

    void GeneralNodeTitleGraphicsWidget::Initialize()
    {
        setCacheMode(QGraphicsItem::CacheMode::DeviceCoordinateCache);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setGraphicsItem(this);
        setAcceptHoverEvents(false);
        setFlag(ItemIsMovable, false);

        m_titleWidget = aznew GraphCanvasLabel(this);
        m_subTitleWidget = aznew GraphCanvasLabel(this);
        m_linearLayout = new QGraphicsLinearLayout(Qt::Vertical);

        setLayout(m_linearLayout);
        setData(GraphicsItemName, QStringLiteral("Title/%1").arg(static_cast<AZ::u64>(m_entityId), 16, 16, QChar('0')));
    }

    void GeneralNodeTitleGraphicsWidget::Activate()
    {
        AZ::EntityId entityId = GetEntityId();
        SceneMemberNotificationBus::Handler::BusConnect(entityId);
        NodeNotificationBus::Handler::BusConnect(entityId);
        RootGraphicsItemNotificationBus::Handler::BusConnect(entityId);

        AZ::EntityId scene;
        SceneMemberRequestBus::EventResult(scene, entityId, &SceneMemberRequests::GetScene);
        if (scene.IsValid())
        {
            SceneNotificationBus::Handler::BusConnect(scene);
            UpdateStyles();
        }
    }

    void GeneralNodeTitleGraphicsWidget::Deactivate()
    {        
        SceneMemberNotificationBus::Handler::BusDisconnect();
        RootGraphicsItemNotificationBus::Handler::BusDisconnect();
        NodeNotificationBus::Handler::BusDisconnect();
        SceneNotificationBus::Handler::BusDisconnect();
    }

    void GeneralNodeTitleGraphicsWidget::SetDetails(const AZStd::string& title, const AZStd::string& subtitle)
    {
        bool updateLayout = false;
        if (m_titleWidget)
        {
            m_titleWidget->SetLabel(title);
            updateLayout = true;
        }

        if (m_subTitleWidget)
        {
            m_subTitleWidget->SetLabel(subtitle);
            updateLayout = true;
        }

        if (updateLayout)
        {
            UpdateLayout();
        }
    }

    void GeneralNodeTitleGraphicsWidget::SetTitle(const AZStd::string& title)
    {
        if (m_titleWidget)
        {
            m_titleWidget->SetLabel(title);
            UpdateLayout();
        }
    }

    void GeneralNodeTitleGraphicsWidget::SetSubTitle(const AZStd::string& subtitle)
    {
        if (m_subTitleWidget)
        {
            m_subTitleWidget->SetLabel(subtitle);
            UpdateLayout();
        }
    }

    void GeneralNodeTitleGraphicsWidget::SetPaletteOverride(AZStd::string_view paletteOverride)
    {
        AZ_Error("GraphCanvas", m_colorOverride == nullptr, "Unsupported use of Color and Palete Overrides");
        if (m_colorOverride)
        {
            delete m_colorOverride;
            m_colorOverride = nullptr;
        }

        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);
        
        m_paletteOverride = nullptr;
        StyleManagerRequestBus::BroadcastResult(m_paletteOverride, &StyleManagerRequests::FindColorPalette, paletteOverride);
        update();
    }

    void GeneralNodeTitleGraphicsWidget::ConfigureIconConfiguration(PaletteIconConfiguration& paletteConfiguration)
    {
        bool isEnabled = false;
        RootGraphicsItemRequestBus::EventResult(isEnabled, GetEntityId(), &RootGraphicsItemRequests::IsEnabled);

        if (!isEnabled)
        {
            if (!m_disabledPalette)
            {
                StyleManagerRequestBus::BroadcastResult(m_disabledPalette, &StyleManagerRequests::FindColorPalette, "DisabledColorPalette");
            }

            if (m_disabledPalette)
            {
                m_disabledPalette->PopulatePaletteConfiguration(paletteConfiguration);
            }
        }
        else if (m_colorOverride)
        {
            m_colorOverride->PopulatePaletteConfiguration(paletteConfiguration);
        }
        else if (m_paletteOverride)
        {
            m_paletteOverride->PopulatePaletteConfiguration(paletteConfiguration);
        }
        else
        {
            m_styleHelper.PopulatePaletteConfiguration(paletteConfiguration);
        }
    }

    void GeneralNodeTitleGraphicsWidget::SetPaletteOverride(const AZ::Uuid& uuid)
    {
        AZ_Error("GraphCanvas", m_colorOverride == nullptr, "Unsupported use of Color and DataType Overrides");
        if (m_colorOverride)
        {
            delete m_colorOverride;
            m_colorOverride = nullptr;
        }

        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

        m_paletteOverride = nullptr;
        StyleManagerRequestBus::BroadcastResult(m_paletteOverride, &StyleManagerRequests::FindDataColorPalette, uuid);
        update();
    }

    void GeneralNodeTitleGraphicsWidget::SetPaletteOverride(const QColor& color)
    {
        if (m_colorOverride == nullptr)
        {
            if (m_paletteOverride != nullptr)
            {
                m_paletteOverride = nullptr;
            }

            AZ::EntityId sceneId;
            SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

            m_colorOverride = new Styling::StyleHelper();
            m_colorOverride->SetScene(sceneId);
            m_colorOverride->SetStyle("ColorOverrideNodeTitlePalette");
        }

        if (m_colorOverride)
        {
            m_colorOverride->AddAttributeOverride(Styling::Attribute::BackgroundColor, color);
            m_colorOverride->AddAttributeOverride(Styling::Attribute::LineColor, color);
            update();
        }
    }

    void GeneralNodeTitleGraphicsWidget::ClearPaletteOverride()
    {
        m_paletteOverride = nullptr;
        update();
    }

    void GeneralNodeTitleGraphicsWidget::UpdateLayout()
    {
        while (m_linearLayout->count() != 0)
        {
            m_linearLayout->removeAt(0);
        }

        if (!m_titleWidget->GetLabel().empty())
        {
            m_linearLayout->addItem(m_titleWidget);
        }

        if (!m_subTitleWidget->GetLabel().empty())
        {
            m_linearLayout->addItem(m_subTitleWidget);
        }

        RefreshDisplay();
        NodeTitleNotificationsBus::Event(GetEntityId(), &NodeTitleNotifications::OnTitleChanged);
        NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::AdjustSize);
    }

    void GeneralNodeTitleGraphicsWidget::UpdateStyles()
    {
        m_styleHelper.SetStyle(GetEntityId(), Styling::Elements::Title);

        qreal spacing = m_styleHelper.GetAttribute(Styling::Attribute::Spacing, 0);
        qreal margin = m_styleHelper.GetAttribute(Styling::Attribute::Margin, 0);

        m_linearLayout->setSpacing(spacing);
        m_linearLayout->setContentsMargins(margin, margin, margin, margin);
        
        m_titleWidget->SetStyle(GetEntityId(), Styling::Elements::MainTitle);
        m_subTitleWidget->SetStyle(GetEntityId(), Styling::Elements::SubTitle);

        // Just clear our the disabled palette and we'll get it when we need it.
        m_disabledPalette = nullptr;

    }

    void GeneralNodeTitleGraphicsWidget::RefreshDisplay()
    {
        updateGeometry();
        update();
    }

    void GeneralNodeTitleGraphicsWidget::OnStylesChanged()
    {
        UpdateStyles();
        RefreshDisplay();
    }

    void GeneralNodeTitleGraphicsWidget::OnAddedToScene(const AZ::EntityId& scene)
    {
        SceneNotificationBus::Handler::BusConnect(scene);

        UpdateStyles();
        RefreshDisplay();
    }

    void GeneralNodeTitleGraphicsWidget::OnRemovedFromScene(const AZ::EntityId& /*scene*/)
    {
        SceneNotificationBus::Handler::BusDisconnect();
    }

    void GeneralNodeTitleGraphicsWidget::OnTooltipChanged(const AZStd::string& tooltip)
    {
        setToolTip(Tools::qStringFromUtf8(tooltip));
    }

    void GeneralNodeTitleGraphicsWidget::OnEnabledChanged(RootGraphicsItemEnabledState /*enabledState*/)
    {
        UpdateStyles();
        RefreshDisplay();        
    }

    void GeneralNodeTitleGraphicsWidget::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        if (m_styleHelper.GetColor(Styling::Attribute::BackgroundColor) == Qt::transparent)    // If the background color is set to transparent, we dont need to worry about any of this
        {
            return;
        }

        // Background
        QRectF bounds = boundingRect();

        qreal cornerRadius = 0.0;

        NodeUIRequestBus::EventResult(cornerRadius, GetEntityId(), &NodeUIRequests::GetCornerRadius);

        // Ensure the bounds are large enough to draw the full radius
        // Even in our smaller section
        if (bounds.height() < 2.0 * cornerRadius)
        {
            bounds.setHeight(2.0 * cornerRadius);
        }

        QPainterPath path;
        path.setFillRule(Qt::WindingFill);

        // -1.0 because the rounding is a little bit short(for some reason), so I subtract one and let it overshoot a smidge.
        path.addRoundedRect(bounds, cornerRadius - 1.0, cornerRadius - 1.0);

        // We only want corners on the top half. So we need to draw a rectangle over the bottom bits to square it out.
        QPointF bottomTopLeft(bounds.bottomLeft());
        bottomTopLeft.setY(bottomTopLeft.y() - cornerRadius - 1.0);
        path.addRect(QRectF(bottomTopLeft, bounds.bottomRight()));

        painter->save();
        painter->setClipPath(path);

        bool isEnabled = false;
        RootGraphicsItemRequestBus::EventResult(isEnabled, GetEntityId(), &RootGraphicsItemRequests::IsEnabled);

        if (!isEnabled)
        {
            if (!m_disabledPalette)
            {
                StyleManagerRequestBus::BroadcastResult(m_disabledPalette, &StyleManagerRequests::FindColorPalette, "DisabledColorPalette");
            }            

            if (m_disabledPalette)
            {
                QtDrawingUtils::FillArea((*painter), path.boundingRect(), (*m_disabledPalette));
            }
        }
        else if (m_colorOverride)
        {
            QtDrawingUtils::FillArea((*painter), path.boundingRect(), (*m_colorOverride));
        }
        else if (m_paletteOverride)
        {
            QtDrawingUtils::FillArea((*painter), path.boundingRect(), (*m_paletteOverride));
        }
        else
        {
            QtDrawingUtils::FillArea((*painter), path.boundingRect(), m_styleHelper);
        }

        QLinearGradient gradient(bounds.bottomLeft(), bounds.topLeft());
        gradient.setColorAt(0, QColor(0, 0, 0, 102));
        gradient.setColorAt(1, QColor(0, 0, 0, 77));
        painter->fillPath(path, gradient);

        painter->restore();

        QGraphicsWidget::paint(painter, option, widget);
    }
}
