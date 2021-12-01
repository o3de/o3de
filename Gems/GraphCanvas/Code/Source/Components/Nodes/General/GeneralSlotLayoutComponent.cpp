/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <functional>

#include <QGraphicsItem>
#include <QGraphicsLayoutItem>
#include <QGraphicsScene>
#include <QGraphicsWidget>
#include <QPainter>

#include <AzCore/RTTI/TypeInfo.h>

#include <Components/Nodes/General/GeneralSlotLayoutComponent.h>

#include <Components/Nodes/General/GeneralNodeFrameComponent.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/Nodes/NodeLayoutBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Editor/GraphCanvasProfiler.h>
#include <GraphCanvas/tools.h>

namespace GraphCanvas
{
    ///////////////////////////////
    // GeneralSlotLayoutComponent
    ///////////////////////////////
    void GeneralSlotLayoutComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<SlotGroupConfiguration>()
                ->Version(1)
                ->Field("LayoutOrder", &SlotGroupConfiguration::m_layoutOrder)
            ;

            serializeContext->Class<GeneralSlotLayoutComponent, AZ::Component>()
                ->Version(2)
                ->Field("EnableDividers", &GeneralSlotLayoutComponent::m_enableDividers)
                ->Field("ConfigurationMap", &GeneralSlotLayoutComponent::m_slotGroupConfigurations)
            ;
        }
    }

    GeneralSlotLayoutComponent::GeneralSlotLayoutComponent()
        : m_enableDividers(false)
    {
    }

    GeneralSlotLayoutComponent::~GeneralSlotLayoutComponent()
    {
    }

    void GeneralSlotLayoutComponent::Init()
    {
        m_nodeSlotsUi = aznew GeneralSlotLayoutGraphicsWidget(*this);

        // Default ordering of the slots determined by the SlotGroupConfiguration value.
        // Values are displayed lowest first to highest last.
        //
        // This needs to be done using emplace and in init to deal with serialization(serialization will not step on any values
        // already in this map, so they all need to be added after the deserialization step has occurred)
        m_slotGroupConfigurations.emplace(SlotGroups::ExecutionGroup, SlotGroupConfiguration(0));
        m_slotGroupConfigurations.emplace(SlotGroups::PropertyGroup, SlotGroupConfiguration(1));
        m_slotGroupConfigurations.emplace(SlotGroups::VariableReferenceGroup, SlotGroupConfiguration(2));
        m_slotGroupConfigurations.emplace(SlotGroups::DataGroup, SlotGroupConfiguration(3));
        m_slotGroupConfigurations.emplace(SlotGroups::VariableSourceGroup, SlotGroupConfiguration(4));
    }

    void GeneralSlotLayoutComponent::Activate()
    {
        if (m_nodeSlotsUi)
        {
            m_nodeSlotsUi->Activate();
        }
    }

    void GeneralSlotLayoutComponent::Deactivate()
    {
        if (m_nodeSlotsUi)
        {
            m_nodeSlotsUi->Deactivate();
        }
    }

    QGraphicsWidget* GeneralSlotLayoutComponent::GetGraphicsWidget()
    {
        return m_nodeSlotsUi;
    }

    ////////////////////////////////////
    // GeneralSlotLayoutGraphicsWidget
    ////////////////////////////////////

    ////////////////////////
    // LayoutDividerWidget
    ////////////////////////
    GeneralSlotLayoutGraphicsWidget::LayoutDividerWidget::LayoutDividerWidget(QGraphicsItem* parent)
        : QGraphicsWidget(parent)
    {
        setAutoFillBackground(true);
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

        setContentsMargins(0, 0, 0, 0);
    }

    void GeneralSlotLayoutGraphicsWidget::LayoutDividerWidget::UpdateStyle(const Styling::StyleHelper& styleHelper)
    {
        prepareGeometryChange();
        qreal border = AZStd::max(1., styleHelper.GetAttribute(Styling::Attribute::BorderWidth, 0.));
        
        QColor dividerColor = styleHelper.GetColor(Styling::Attribute::BorderColor);
        QPalette widgetPalette = palette();
        widgetPalette.setColor(QPalette::ColorGroup::Active, QPalette::ColorRole::Window, dividerColor);
        setPalette(widgetPalette);
        
        setMinimumHeight(border);
        setPreferredHeight(border);
        setMaximumHeight(border);

        updateGeometry();
        update();
    }

    //////////////////////////
    // LinearSlotGroupLayout
    //////////////////////////
    GeneralSlotLayoutGraphicsWidget::LinearSlotGroupWidget::LinearSlotGroupWidget(QGraphicsItem* parent)
        : QGraphicsWidget(parent)
        , m_inputs(nullptr)
        , m_outputs(nullptr)        
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        m_inputs = new QGraphicsLinearLayout(Qt::Vertical);
        m_inputs->setContentsMargins(0, 0, 0, 0);
        m_inputs->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        {
            QGraphicsWidget* spacer = new QGraphicsWidget();
            spacer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
            spacer->setContentsMargins(0, 0, 0, 0);
            spacer->setPreferredSize(0, 0);

            m_inputs->addItem(spacer);
        }

        m_outputs = new QGraphicsLinearLayout(Qt::Vertical);
        m_outputs->setContentsMargins(0, 0, 0, 0);
        m_outputs->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        {
            QGraphicsWidget* spacer = new QGraphicsWidget();
            spacer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
            spacer->setContentsMargins(0, 0, 0, 0);
            spacer->setPreferredSize(0, 0);

            m_outputs->addItem(spacer);
        }

        QGraphicsLinearLayout* layout = new QGraphicsLinearLayout(Qt::Horizontal);
        setLayout(layout);

        layout->setInstantInvalidatePropagation(true);

        // Creating the actual display
        // <input><spacer><output>
        layout->addItem(m_inputs);

        m_horizontalSpacer = new QGraphicsWidget();
        m_horizontalSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        m_horizontalSpacer->setContentsMargins(0, 0, 0, 0);
        m_horizontalSpacer->setPreferredSize(0, 0);

        layout->addItem(m_horizontalSpacer);

        layout->addItem(m_outputs);

        layout->setAlignment(m_outputs, Qt::AlignRight);
    }

    void GeneralSlotLayoutGraphicsWidget::LinearSlotGroupWidget::DisplaySlot(const AZ::EntityId& slotId)
    {
        ConnectionType connectionType = ConnectionType::CT_Invalid;
        SlotRequestBus::EventResult(connectionType, slotId, &SlotRequests::GetConnectionType);

        SlotLayoutInfo slotInfo(slotId);

        if (connectionType == CT_Input)
        {
            SlotUINotificationBus::MultiHandler::BusConnect(slotId);

            m_inputSlotSet.insert(slotId);
            LayoutSlot(m_inputs, m_inputSlots, slotInfo);
        }
        else if (connectionType == CT_Output)
        {
            SlotUINotificationBus::MultiHandler::BusConnect(slotId);

            m_outputSlotSet.insert(slotId);
            LayoutSlot(m_outputs, m_outputSlots, slotInfo);
        }
        else
        {
            AZ_Warning("GraphCanvas", false, "Invalid Connection Type for slot. Cannot add to Node Layout");
        }
    }

    void GeneralSlotLayoutGraphicsWidget::LinearSlotGroupWidget::RemoveSlot(const AZ::EntityId& slotId)
    {
        ConnectionType connectionType = ConnectionType::CT_Invalid;
        SlotRequestBus::EventResult(connectionType, slotId, &SlotRequests::GetConnectionType);

        QGraphicsLayoutItem* layoutItem = GetLayoutItem(slotId);

        if (layoutItem)
        {
            SlotUINotificationBus::MultiHandler::BusDisconnect(slotId);

            if (scene())
            {
                scene()->removeItem(layoutItem->graphicsItem());
            }

            if (connectionType == CT_Input)
            {
                m_inputSlotSet.erase(slotId);
                m_inputs->removeItem(layoutItem);

                for (unsigned int i = 0; i < m_inputSlots.size(); ++i)
                {
                    if (m_inputSlots[i].m_slotId == slotId)
                    {
                        m_inputSlots.erase(m_inputSlots.begin() + i);
                        break;
                    }
                }
            }
            else if (connectionType == CT_Output)
            {
                m_outputSlotSet.erase(slotId);
                m_outputs->removeItem(layoutItem);

                for (unsigned int i = 0; i < m_outputSlots.size(); ++i)
                {
                    if (m_outputSlots[i].m_slotId == slotId)
                    {
                        m_outputSlots.erase(m_outputSlots.begin() + i);
                        break;
                    }
                }
            }
        }
    }

    const AZStd::vector< SlotLayoutInfo >& GeneralSlotLayoutGraphicsWidget::LinearSlotGroupWidget::GetInputSlots() const
    {
        return m_inputSlots;
    }

    const AZStd::vector< SlotLayoutInfo >& GeneralSlotLayoutGraphicsWidget::LinearSlotGroupWidget::GetOutputSlots() const
    {
        return m_outputSlots;
    }

    bool GeneralSlotLayoutGraphicsWidget::LinearSlotGroupWidget::IsEmpty() const
    {
        // 1 because there is a spacer in each of the layouts that we need to account for.
        return m_inputs->count() == 1 && m_outputs->count() == 1;
    }

    void GeneralSlotLayoutGraphicsWidget::LinearSlotGroupWidget::UpdateStyle(const Styling::StyleHelper& styleHelper)
    {
        prepareGeometryChange();

        qreal spacing = styleHelper.GetAttribute(Styling::Attribute::Spacing, 0);
        qreal margin = styleHelper.GetAttribute(Styling::Attribute::Margin, 0);

        setContentsMargins(margin, margin, margin, margin);

        for (QGraphicsLinearLayout* internalLayout : { m_inputs, m_outputs })
        {
            internalLayout->setSpacing(spacing);
            internalLayout->invalidate();
            internalLayout->updateGeometry();
        }

        updateGeometry();
        update();
    }

    void GeneralSlotLayoutGraphicsWidget::LinearSlotGroupWidget::OnSlotLayoutPriorityChanged(int layoutPriority)
    {
        const SlotId* slotId = SlotUINotificationBus::GetCurrentBusId();

        if (slotId == nullptr)
        {
            return;
        }

        AZStd::vector< SlotLayoutInfo >* layoutVector = nullptr;
        QGraphicsLinearLayout* layoutElement = nullptr;

        if (m_inputSlotSet.count((*slotId)) > 0)
        {
            layoutVector = &m_inputSlots;
            layoutElement = m_inputs;
        }
        else if (m_outputSlotSet.count((*slotId)) > 0)
        {
            layoutVector = &m_outputSlots;
            layoutElement = m_outputs;
        }

        if (layoutVector == nullptr || layoutElement == nullptr)
        {
            return;
        }
        
        for (auto layoutIter = layoutVector->begin(); layoutIter != layoutVector->end(); ++layoutIter)
        {
            if (layoutIter->m_slotId == (*slotId))
            {
                SlotLayoutInfo info = (*layoutIter);
                info.m_priority = layoutPriority;

                layoutVector->erase(layoutIter);

                QGraphicsLayoutItem* layoutItem = GetLayoutItem(info.m_slotId);
                layoutElement->removeItem(layoutItem);

                LayoutSlot(layoutElement, (*layoutVector), info);
                break;
            }
        }
    }

    int GeneralSlotLayoutGraphicsWidget::LinearSlotGroupWidget::LayoutSlot(QGraphicsLinearLayout* layout, AZStd::vector<SlotLayoutInfo>& slotList, const SlotLayoutInfo& slotInfo)
    {
        bool inserted = false;
        int i = 0;
        auto listIter = slotList.begin();

        while (listIter != slotList.end())
        {
            if (listIter->m_priority < slotInfo.m_priority)
            {
                inserted = true;
                slotList.insert(listIter, slotInfo);
                break;
            }

            ++i;
            ++listIter;
        }

        if (!inserted)
        {
            slotList.insert(slotList.end(), slotInfo);
        }
        
        QGraphicsLayoutItem* layoutItem = GetLayoutItem(slotInfo.m_slotId);

        if (layoutItem)
        {
            layout->insertItem(i, layoutItem);
            layout->setAlignment(layoutItem, Qt::AlignTop);
            SlotRequestBus::Event(slotInfo.m_slotId, &SlotRequests::SetDisplayOrdering, i);
        }

        return i;
    }

    QGraphicsLayoutItem* GeneralSlotLayoutGraphicsWidget::LinearSlotGroupWidget::GetLayoutItem(const AZ::EntityId& slotId) const
    {
        QGraphicsLayoutItem* layoutItem = nullptr;
        VisualRequestBus::EventResult(layoutItem, slotId, &VisualRequests::AsGraphicsLayoutItem);

        AZ_Assert(layoutItem, "Slot must return a GraphicsLayoutItem.");

        return layoutItem;
    }

    ////////////////////////////////////
    // GeneralSlotLayoutGraphicsWidget
    ////////////////////////////////////

    GeneralSlotLayoutGraphicsWidget::GeneralSlotLayoutGraphicsWidget(GeneralSlotLayoutComponent& nodeSlots)
        : m_nodeSlots(nodeSlots)
        , m_entityId(nodeSlots.GetEntityId())
        , m_addedToScene(false)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        setGraphicsItem(this);
        setAcceptHoverEvents(false);
        setFlag(ItemIsMovable, false);
        setContentsMargins(0, 0, 0, 0);

        setData(GraphicsItemName, QStringLiteral("Slots/%1").arg(static_cast<AZ::u64>(GetEntityId()), 16, 16, QChar('0')));

        m_groupLayout = new QGraphicsLinearLayout(Qt::Vertical);
        m_groupLayout->setSpacing(0);
        m_groupLayout->setInstantInvalidatePropagation(true);

        setLayout(m_groupLayout);
    }

    GeneralSlotLayoutGraphicsWidget::~GeneralSlotLayoutGraphicsWidget()
    {
        // Because I'm allowing for re-use of widgets. There's no guarantee which widgets
        // will have a valid parent. So we want to clear our display, then delete everything
        // that we own.
        ClearLayout();

        for (LayoutDividerWidget* divider : m_dividers)
        {
            delete divider;
        }

        for (auto& mapPair : m_slotGroups)
        {
            delete mapPair.second;
        }
    }

    void GeneralSlotLayoutGraphicsWidget::Activate()
    {
        SlotLayoutRequestBus::Handler::BusConnect(GetEntityId());
        NodeNotificationBus::Handler::BusConnect(GetEntityId());
        NodeSlotsRequestBus::Handler::BusConnect(GetEntityId());
        StyleNotificationBus::Handler::BusConnect(GetEntityId());
        SceneMemberNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void GeneralSlotLayoutGraphicsWidget::OnNodeActivated()
    {
        ActivateSlots();
        UpdateLayout();
    }

    void GeneralSlotLayoutGraphicsWidget::Deactivate()
    {
        ClearLayout();

        SceneMemberNotificationBus::Handler::BusDisconnect();
        StyleNotificationBus::Handler::BusDisconnect();
        NodeSlotsRequestBus::Handler::BusDisconnect();
        NodeNotificationBus::Handler::BusDisconnect();
        SlotLayoutRequestBus::Handler::BusDisconnect();
    }

    void GeneralSlotLayoutGraphicsWidget::OnSlotAddedToNode(const AZ::EntityId& slotId)
    {
        bool needsUpate = DisplaySlot(slotId);

        if (needsUpate)
        {
            UpdateLayout();
        }
    }

    void GeneralSlotLayoutGraphicsWidget::OnSlotRemovedFromNode(const AZ::EntityId& slotId)
    {
        bool needsUpdate = RemoveSlot(slotId);

        if (needsUpdate)
        {
            UpdateLayout();
        }
    }

    QGraphicsLayoutItem* GeneralSlotLayoutGraphicsWidget::GetGraphicsLayoutItem()
    {
        return this;
    }

    void GeneralSlotLayoutGraphicsWidget::OnSceneSet(const AZ::EntityId& /*sceneId*/)
    {
        m_addedToScene = true;
        UpdateLayout();
    }

    void GeneralSlotLayoutGraphicsWidget::SetDividersEnabled(bool enabled)
    {
        m_nodeSlots.m_enableDividers = enabled;
        UpdateLayout();
    }

    void GeneralSlotLayoutGraphicsWidget::ConfigureSlotGroup(SlotGroup group, SlotGroupConfiguration configuration)
    {
        if (group != SlotGroups::Invalid)
        {
            m_nodeSlots.m_slotGroupConfigurations[group] = configuration;
            UpdateLayout();
        }
    }

    int GeneralSlotLayoutGraphicsWidget::GetSlotGroupDisplayOrder(SlotGroup group) const
    {        
        AZStd::set<SlotGroup, SlotGroupConfigurationComparator> slotOrdering(SlotGroupConfigurationComparator(&m_nodeSlots.m_slotGroupConfigurations));

        for (auto& mapPair : m_slotGroups)
        {
            if (!mapPair.second->IsEmpty())
            {
                const SlotGroupConfiguration& configuration = m_nodeSlots.m_slotGroupConfigurations[mapPair.first];

                if (configuration.m_visible)
                {
                    slotOrdering.insert(mapPair.first);
                }
            }
        }

        int counter = 1;
        for (const SlotGroup& slotGroup : slotOrdering)
        {
            if (slotGroup == group)
            {
                return counter;
            }

            ++counter;
        }

        return -1;
    }

    bool GeneralSlotLayoutGraphicsWidget::IsSlotGroupVisible(SlotGroup group) const
    {
        bool isVisible = false;
        if (group != SlotGroups::Invalid)
        {
            SlotGroupConfiguration& slotConfiguration = m_nodeSlots.m_slotGroupConfigurations[group];
            isVisible = slotConfiguration.m_visible;
        }

        return isVisible;
    }

    void GeneralSlotLayoutGraphicsWidget::SetSlotGroupVisible(SlotGroup group, bool visible)
    {
        if (group != SlotGroups::Invalid)
        {
            SlotGroupConfiguration& slotConfiguration = m_nodeSlots.m_slotGroupConfigurations[group];

            if (slotConfiguration.m_visible != visible)
            {
                slotConfiguration.m_visible = visible;
                UpdateLayout();
            }
        }
    }

    void GeneralSlotLayoutGraphicsWidget::ClearSlotGroup(SlotGroup group)
    {
        if (group != SlotGroups::Invalid)
        {
            LinearSlotGroupWidget* slotGroupWidget = FindCreateSlotGroupWidget(group);

            if (slotGroupWidget)
            {
                AZStd::vector< SlotLayoutInfo > inputSlots = slotGroupWidget->GetInputSlots();

                for (const SlotLayoutInfo& inputSlot : inputSlots)
                {
                    NodeRequestBus::Event(GetEntityId(), &NodeRequests::RemoveSlot, inputSlot.m_slotId);
                }

                AZStd::vector< SlotLayoutInfo > outputSlots = slotGroupWidget->GetOutputSlots();

                for (const SlotLayoutInfo& outputSlot : outputSlots)
                {
                    NodeRequestBus::Event(GetEntityId(), &NodeRequests::RemoveSlot, outputSlot.m_slotId);
                }
            }
        }
    }

    void GeneralSlotLayoutGraphicsWidget::OnStyleChanged()
    {
        UpdateStyles();
        update();
    }

    bool GeneralSlotLayoutGraphicsWidget::DisplaySlot(const AZ::EntityId& slotId)
    {
        bool needsUpdate = false;

        SlotGroup slotGroup = SlotGroups::Invalid;
        SlotRequestBus::EventResult(slotGroup, slotId, &SlotRequests::GetSlotGroup);

        LinearSlotGroupWidget* groupWidget = FindCreateSlotGroupWidget(slotGroup);

        if (groupWidget)
        {
            needsUpdate = groupWidget->IsEmpty();
            groupWidget->DisplaySlot(slotId);
        }

        return needsUpdate;
    }

    bool GeneralSlotLayoutGraphicsWidget::RemoveSlot(const AZ::EntityId& slotId)
    {
        bool needsUpdate = false;

        SlotGroup slotGroup = SlotGroups::Invalid;
        SlotRequestBus::EventResult(slotGroup, slotId, &SlotRequests::GetSlotGroup);

        LinearSlotGroupWidget* groupWidget = FindCreateSlotGroupWidget(slotGroup);

        if (groupWidget)
        {
            groupWidget->RemoveSlot(slotId);
            needsUpdate = groupWidget->IsEmpty();
        }

        return needsUpdate;
    }

    void GeneralSlotLayoutGraphicsWidget::ActivateSlots()
    {
        AZStd::vector<AZ::EntityId> slotIds;
        NodeRequestBus::EventResult(slotIds, m_nodeSlots.GetEntityId(), &NodeRequests::GetSlotIds);
        for (const AZ::EntityId& slotId : slotIds)
        {
            AZ::Entity* slot = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(slot, &AZ::ComponentApplicationRequests::FindEntity, slotId);
            AZ_Assert(slot, "A Slot (ID: %s) of Node (ID: %s) has no Entity!", slotId.ToString().data(), m_nodeSlots.GetEntityId().ToString().data());

            DisplaySlot(slotId);
        }
    }

    void GeneralSlotLayoutGraphicsWidget::ClearLayout()
    {
        // Clear out our previous configurations
        while (m_groupLayout->count() > 0)
        {
            m_groupLayout->removeAt(m_groupLayout->count() - 1);
        }
    }

    void GeneralSlotLayoutGraphicsWidget::UpdateLayout()
    {
        if (!m_addedToScene)
        {
            return;
        }

        ClearLayout();

        prepareGeometryChange();

        AZStd::set<SlotGroup, SlotGroupConfigurationComparator> slotOrdering(SlotGroupConfigurationComparator(&m_nodeSlots.m_slotGroupConfigurations));

        for (auto& mapPair : m_slotGroups)
        {
            if (!mapPair.second->IsEmpty())
            {
                const SlotGroupConfiguration& configuration = m_nodeSlots.m_slotGroupConfigurations[mapPair.first];

                if (configuration.m_visible)
                {
                    slotOrdering.insert(mapPair.first);
                }
                else
                {
                    // Fun with SceneFilter's.
                    // If an object with a scene filter is removed from the scene
                    // That scene filter gets destroyed or invalidated in some way.
                    //
                    // This means if we were to ever remove a data slot from the scene
                    // and add it back on, we need to re-hook up everything.
                    auto groupIter = m_slotGroups.find(mapPair.first);

                    if (groupIter != m_slotGroups.end())
                    {
                        if (groupIter->second->scene() != nullptr)
                        {
                            groupIter->second->scene()->removeItem(groupIter->second);
                        }
                    }
                }
            }
        }

        for (auto& divider : m_dividers)
        {
            if (divider->isVisible())
            {
                divider->setVisible(false);
            }
        }

        int dividerCount = 0;
        bool needsDivider = false;

        for (const SlotGroup& slotGroup : slotOrdering)
        {
            if (needsDivider)
            {
                needsDivider = false;
                LayoutDividerWidget* dividerWidget = FindCreateDividerWidget(dividerCount);
                ++dividerCount;

                m_groupLayout->addItem(dividerWidget);
                dividerWidget->setVisible(true);
            }

            auto groupIter = m_slotGroups.find(slotGroup);

            if (groupIter != m_slotGroups.end())
            {
                needsDivider = m_nodeSlots.m_enableDividers;
                m_groupLayout->addItem(groupIter->second);
            }
        }
        
        RefreshDisplay();

        NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::AdjustSize);
    }

    void GeneralSlotLayoutGraphicsWidget::UpdateStyles()
    {
        m_styleHelper.SetStyle(GetEntityId(), Styling::Elements::GeneralSlotLayout);

        prepareGeometryChange();

        qreal margin = m_styleHelper.GetAttribute(Styling::Attribute::Margin, 0.0);
        m_groupLayout->setContentsMargins(margin, margin, margin, margin);
        m_groupLayout->setSpacing(m_styleHelper.GetAttribute(Styling::Attribute::Spacing, 0.0));

        for (LayoutDividerWidget* divider : m_dividers)
        {
            divider->UpdateStyle(m_styleHelper);
        }

        for (auto& mapPair : m_slotGroups)
        {
            mapPair.second->UpdateStyle(m_styleHelper);
        }

        RefreshDisplay();
    }

    void GeneralSlotLayoutGraphicsWidget::RefreshDisplay()
    {
        updateGeometry();
        update();
    }

    GeneralSlotLayoutGraphicsWidget::LinearSlotGroupWidget* GeneralSlotLayoutGraphicsWidget::FindCreateSlotGroupWidget(SlotGroup slotType)
    {
        AZ_Warning("GraphCanvas", slotType != SlotGroups::Invalid, "Trying to Create a Slot Group for an Invalid slot group");

        LinearSlotGroupWidget* retVal = nullptr;

        if (slotType != SlotGroups::Invalid)
        {
            auto mapIter = m_slotGroups.find(slotType);

            if (mapIter != m_slotGroups.end())
            {
                retVal = mapIter->second;
            }
            else
            {
                auto configurationIter = m_nodeSlots.m_slotGroupConfigurations.find(slotType);

                if (configurationIter == m_nodeSlots.m_slotGroupConfigurations.end())
                {
                    SlotGroupConfiguration groupConfiguration;

                    groupConfiguration.m_layoutOrder = static_cast<int>(m_nodeSlots.m_slotGroupConfigurations.size());

                    m_nodeSlots.m_slotGroupConfigurations[slotType] = groupConfiguration;
                }

                retVal = aznew LinearSlotGroupWidget(this);
                retVal->UpdateStyle(m_styleHelper);

                m_slotGroups[slotType] = retVal;
            }
        }

        return retVal;
    }

    GeneralSlotLayoutGraphicsWidget::LayoutDividerWidget* GeneralSlotLayoutGraphicsWidget::FindCreateDividerWidget(int index)
    {
        AZ_Error("GraphCanvas", index <= m_dividers.size(), "Invalid Divider Creation flow. Jumped the line in divider indexing.");
        LayoutDividerWidget* retVal = nullptr;

        while (index >= m_dividers.size())
        {
            // Create and configure a new divider
            LayoutDividerWidget* divider = aznew LayoutDividerWidget(this);
            divider->UpdateStyle(m_styleHelper);

            m_dividers.push_back(divider);
        }

        retVal = m_dividers[index];

        return retVal;
    }
}
