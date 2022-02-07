/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/std/containers/set.h>
#include <Editor/Nodes/NodeUtils.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <ScriptCanvas/Data/DataRegistry.h>
#include <ScriptCanvas/Bus/ScriptCanvasBus.h>
#include <ScriptCanvasDeveloperEditor/Mock.h>

namespace ScriptCanvasDeveloper
{
    namespace Nodes
    {
        struct DataTypeNameSortFunctor
        {
            bool operator()(const AZStd::pair<Data::Type, AZStd::string_view>& lhs, const AZStd::pair<Data::Type, AZStd::string>& rhs) const
            {
                return lhs.second < rhs.second;
            }
        };

        void SlotConfig::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<SlotConfig>()
                    ->Version(0)
                    ->Field("m_name", &SlotConfig::m_name)
                    ->Field("m_toolTip", &SlotConfig::m_toolTip)
                    ->Field("m_type", &SlotConfig::m_type)
                    ->Field("m_slotId", &SlotConfig::m_slotId)
                    ->Field("m_state", &SlotConfig::m_state)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<SlotConfig>("SlotConfig", "Configuration for slot")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SlotConfig::m_name, "Slot Name", "Slot Name")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SlotConfig::OnSlotNameChanged)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SlotConfig::m_toolTip, "Slot Tooltip", "Slot Tooltip")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SlotConfig::OnSlotToolTipChanged)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SlotConfig::m_type, "Slot Type", "The Data Type represented by this slot")
                        ->Attribute(AZ::Edit::Attributes::GenericValueList, &SlotConfig::GetCreatableTypes)
                        ->Attribute(AZ::Edit::Attributes::PostChangeNotify, &SlotConfig::OnSlotDataTypeChanged)
                        ;
                }
            }
        }

        SlotConfig::SlotConfig()
            : m_name("New Slot")
        {
        }

        SlotConfig::~SlotConfig()
        {
            if (m_owner)
            {
                m_owner->m_pendingConfigRemovals.push_back(m_slotId);
            }
        }

        AZStd::vector<AZStd::pair<Data::Type, AZStd::string>> SlotConfig::GetCreatableTypes()
        {
            if (!m_owner)
            {
                return AZStd::vector<AZStd::pair<Data::Type, AZStd::string>>{};
            }

            Slot* slot = m_owner->GetSlot(m_slotId);
            SlotDescriptor slotDescriptor = slot ? slot->GetDescriptor() : SlotDescriptor();

            if (slotDescriptor.IsExecution())
            {
                return AZStd::vector<AZStd::pair<Data::Type, AZStd::string>>{};
            }

            AZStd::set<AZStd::pair<Data::Type, AZStd::string_view>, DataTypeNameSortFunctor> sortedTypes;
            sortedTypes.emplace(Data::Type::Invalid(), "");

            AZStd::unordered_set<Data::Type> creatableTypes;
            ScriptCanvasEditor::SystemRequestBus::Broadcast(&ScriptCanvasEditor::SystemRequests::GetEditorCreatableTypes, creatableTypes);
            for (const Data::Type& creatableType : creatableTypes)
            {
                if (const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(creatableType.GetAZType()))
                {
                    if (AZ::FindAttribute(AZ::ScriptCanvasAttributes::VariableCreationForbidden, behaviorClass->m_attributes))
                    {
                        continue;
                    }
                    
                    bool isDeprecated{};
                    if (auto isDeprecatedAttributePtr = AZ::FindAttribute(AZ::Script::Attributes::Deprecated, behaviorClass->m_attributes))
                    {
                        AZ::AttributeReader(nullptr, isDeprecatedAttributePtr).Read<bool>(isDeprecated);
                    }

                    if (isDeprecated)
                    {
                        continue;
                    }
                }

                sortedTypes.emplace(creatableType, Data::GetName(creatableType));
            }

            return AZStd::vector<AZStd::pair<Data::Type, AZStd::string>>(sortedTypes.begin(), sortedTypes.end());
        }

        AZ::Crc32 SlotConfig::OnSlotNameChanged()
        {
            if (m_owner)
            {
                auto gcSlotIt = AZStd::find_if(m_owner->m_graphCanvasSlotIds.begin(), m_owner->m_graphCanvasSlotIds.end(), [this](AZ::EntityId testSlotId)
                {
                    AZStd::any* slotUserData{};
                    GraphCanvas::SlotRequestBus::EventResult(slotUserData, testSlotId, &GraphCanvas::SlotRequests::GetUserData);
                    auto scSlotId = AZStd::any_cast<SlotId>(slotUserData);
                    return scSlotId && *scSlotId == m_slotId;
                });
                if (gcSlotIt != m_owner->m_graphCanvasSlotIds.end())
                {
                    AZ::EntityId gcSlotId = *gcSlotIt;
                    GraphCanvas::SlotRequestBus::Event(gcSlotId, &GraphCanvas::SlotRequests::SetName, m_name);
                }
            }
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }

        AZ::Crc32 SlotConfig::OnSlotToolTipChanged()
        {
            if (m_owner)
            {
                auto gcSlotIt = AZStd::find_if(m_owner->m_graphCanvasSlotIds.begin(), m_owner->m_graphCanvasSlotIds.end(), [this](AZ::EntityId testSlotId)
                {
                    AZStd::any* slotUserData{};
                    GraphCanvas::SlotRequestBus::EventResult(slotUserData, testSlotId, &GraphCanvas::SlotRequests::GetUserData);
                    auto scSlotId = AZStd::any_cast<SlotId>(slotUserData);
                    return scSlotId && *scSlotId == m_slotId;
                });
                if (gcSlotIt != m_owner->m_graphCanvasSlotIds.end())
                {
                    AZ::EntityId gcSlotId = *gcSlotIt;
                    GraphCanvas::SlotRequestBus::Event(gcSlotId, &GraphCanvas::SlotRequests::SetTooltip, m_toolTip);
                }
            }
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }

        void SlotConfig::OnSlotDataTypeChanged([[maybe_unused]] const Data::Type& oldDataType)
        {
            if (!m_owner)
            {
                return;
            }
            Slot* slot = m_owner->GetSlot(m_slotId);
            SlotDescriptor slotDescriptor = slot ? slot->GetDescriptor() : SlotDescriptor();

            if (slotDescriptor.IsExecution())
            {
                return;
            }

            m_owner->RemoveSlot(m_slotId);

            m_slotId = {};
            if (slotDescriptor.IsInput())
            {
                if (m_type.IsValid())
                {
                    DataSlotConfiguration slotConfiguration;

                    slotConfiguration.m_name = m_name;
                    slotConfiguration.m_toolTip = m_toolTip;
                    slotConfiguration.SetType(m_type);
                    slotConfiguration.SetConnectionType(ConnectionType::Input);

                    m_slotId = m_owner->AddSlot(slotConfiguration);
                }
                else
                {
                    DynamicDataSlotConfiguration slotConfiguration;

                    slotConfiguration.m_name = m_name;
                    slotConfiguration.m_toolTip = m_toolTip;
                    slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
                    slotConfiguration.SetConnectionType(ConnectionType::Input);

                    m_slotId = m_owner->AddSlot(slotConfiguration);
                }
            }
            else if (slotDescriptor.IsOutput())
            {
                DataSlotConfiguration slotConfiguration;

                slotConfiguration.m_name = m_name;
                slotConfiguration.m_toolTip = m_toolTip;
                slotConfiguration.SetType(m_type);
                slotConfiguration.SetConnectionType(ConnectionType::Output);

                m_slotId = m_owner->AddSlot(slotConfiguration);
            }
        }

        void Mock::OnGraphCanvasNodeDisplayed(AZ::EntityId graphCanvasId)
        {
            if (!MockDescriptorRequestBus::Handler::BusIsConnected())
            {
                MockDescriptorRequestBus::Handler::BusConnect(GetEntityId());

                m_graphCanvasNodeId = graphCanvasId;

                SetUiEntityId(graphCanvasId);
                OnNodeDisplayed(graphCanvasId);
            }
        }

        void Mock::Reflect(AZ::ReflectContext* context)
        {
            SlotConfig::Reflect(context);
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<Mock, ScriptCanvas::Node>()
                    ->Version(0)
                    ->Field("m_uiEntityId", &Mock::m_graphCanvasNodeId)
                    ->Field("m_uiSlotIds", &Mock::m_graphCanvasSlotIds)
                    ->Field("m_nodeTitle", &Mock::m_nodeTitle)
                    ->Field("m_nodeSubTitle", &Mock::m_nodeSubTitle)
                    ->Field("m_dataInConfigArray", &Mock::m_dataInConfigArray)
                    ->Field("m_dataOutConfigArray", &Mock::m_dataOutConfigArray)
                    ->Field("m_executionInConfigArray", &Mock::m_executionInConfigArray)
                    ->Field("m_executionOutConfigArray", &Mock::m_executionOutConfigArray)
                    ->Field("m_nodeColorPaletteOverride", &Mock::m_nodeColorPaletteOverride);
                ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<Mock>("Mock", "Node for mocking node visuals")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Mock::m_nodeTitle, "Node Title", "Node Title for this mock node")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Mock::OnNodeTitleChanged)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Mock::m_nodeSubTitle, "Node Sub Title", "Node Sub Title for this mock node")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Mock::OnNodeSubTitleChanged)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Mock::m_dataInConfigArray, "Data Input Slot Configuration", "Configuration array of adding/removing Mock data input slots")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Mock::OnDataInSlotArrayChanged)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Mock::m_dataOutConfigArray, "Data Output Slot Configuration", "Configuration array of adding/removing Mock data output slots")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Mock::OnDataOutSlotArrayChanged)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Mock::m_executionInConfigArray, "Execution Input Slot Configuration", "Configuration array of adding/removing Mock execution input slots")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Mock::OnExecutionInSlotArrayChanged)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Mock::m_executionOutConfigArray, "Execution Output Slot Configuration", "Configuration array of adding/removing Mock execution output slots")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Mock::OnExecutionOutSlotArrayChanged)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &Mock::m_nodeColorPaletteOverride, "Node Color Palette Override", "Updates the node color from one of the possible palettes")
                        ->Attribute(AZ::Edit::Attributes::StringList, &Mock::GetColorPaletteList)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Mock::OnNodeColorPaletteChanged)
                        ;
                }
            }
        }

        Mock::Mock()
            : m_nodeTitle("Mock")
            , m_nodeSubTitle("Node")
        {
        }

        Mock::~Mock()
        {
            ScriptCanvasEditor::EditorNodeNotificationBus::Handler::BusDisconnect();
            GraphCanvas::NodeNotificationBus::Handler::BusDisconnect();
        }

        void Mock::OnInit()
        {
            ScriptCanvasEditor::EditorNodeNotificationBus::Handler::BusConnect(GetEntityId());

            if (m_graphCanvasNodeId.IsValid())
            {
                GraphCanvas::NodeNotificationBus::Handler::BusConnect(m_graphCanvasNodeId);
                GraphCanvas::SceneMemberNotificationBus::Handler::BusConnect(m_graphCanvasNodeId);
            }

            OnNodeTitleChanged();
            OnNodeSubTitleChanged();
            OnNodeColorPaletteChanged();

            for (auto& slotConfig : m_executionInConfigArray)
            {
                slotConfig.m_owner = this;
                slotConfig.OnSlotNameChanged();
                slotConfig.OnSlotToolTipChanged();
            }
            for (auto& slotConfig : m_executionOutConfigArray)
            {
                slotConfig.m_owner = this;
                slotConfig.OnSlotNameChanged();
                slotConfig.OnSlotToolTipChanged();
            }
            for (auto& slotConfig : m_dataInConfigArray)
            {
                slotConfig.m_owner = this;
                slotConfig.OnSlotNameChanged();
                slotConfig.OnSlotToolTipChanged();
            }
            for (auto& slotConfig : m_dataOutConfigArray)
            {
                slotConfig.m_owner = this;
                slotConfig.OnSlotNameChanged();
                slotConfig.OnSlotToolTipChanged();
            }
        }

        void Mock::SetUiEntityId(AZ::EntityId uiEntityId)
        {
            GraphCanvas::NodeNotificationBus::Handler::BusDisconnect();
            m_graphCanvasNodeId = uiEntityId;
            if (m_graphCanvasNodeId.IsValid())
            {
                GraphCanvas::NodeNotificationBus::Handler::BusConnect(m_graphCanvasNodeId);
            }
        }

        void Mock::Clear()
        {
            m_graphCanvasNodeId.SetInvalid();
            m_graphCanvasSlotIds.clear();
            m_nodeTitle = "Mock";
            OnNodeTitleChanged();

            m_nodeSubTitle.clear();
            OnNodeSubTitleChanged();

            m_dataInConfigArray.clear();
            OnDataInSlotArrayChanged();

            m_dataOutConfigArray.clear();
            OnDataOutSlotArrayChanged();

            m_executionInConfigArray.clear();
            OnExecutionInSlotArrayChanged();

            m_executionInConfigArray.clear();
            OnExecutionOutSlotArrayChanged();

            OnClear();
        }

        void Mock::OnClear()
        {
        }

        void Mock::OnNodeDisplayed([[maybe_unused]] const GraphCanvas::NodeId& nodeId)
        {
        }

        AZ::Crc32 Mock::OnNodeTitleChanged()
        {
            GraphCanvas::NodeTitleRequestBus::Event(m_graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetTitle, m_nodeTitle);
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }

        AZ::Crc32 Mock::OnNodeSubTitleChanged()
        {
            GraphCanvas::NodeTitleRequestBus::Event(m_graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetSubTitle, m_nodeSubTitle);
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }

        AZ::Crc32 Mock::OnNodeColorPaletteChanged()
        {
            if (!m_nodeColorPaletteOverride.empty())
            {
                GraphCanvas::NodeTitleRequestBus::Event(m_graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetDefaultPalette, m_nodeColorPaletteOverride);
                return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
            }

            return AZ::Edit::PropertyRefreshLevels::None;
        }

        AZStd::vector<AZStd::string> Mock::GetColorPaletteList() const
        {
            AZStd::vector<AZStd::string> colorPaletteOptions;

            AZ::EntityId uiSceneId;
            GraphCanvas::SceneMemberRequestBus::EventResult(uiSceneId, m_graphCanvasNodeId, &GraphCanvas::SceneMemberRequests::GetScene);

            GraphCanvas::EditorId editorId;
            GraphCanvas::SceneRequestBus::EventResult(editorId, uiSceneId, &GraphCanvas::SceneRequests::GetEditorId);
            GraphCanvas::StyleManagerRequestBus::EventResult(colorPaletteOptions, editorId, &GraphCanvas::StyleManagerRequests::GetColorPaletteStyles);
            return colorPaletteOptions;
        }

        AZ::Crc32 Mock::OnDataInSlotArrayChanged()
        {
            const auto pendingConfigRemovals = AZStd::move(m_pendingConfigRemovals);
            for (const auto& oldSlotId : pendingConfigRemovals)
            {
                RemoveSlot(oldSlotId);
            }
            for (auto& dataInConfig : m_dataInConfigArray)
            {
                dataInConfig.m_owner = this;
                if (dataInConfig.m_state == SlotConfigState::New)
                {
                    if (dataInConfig.m_type.IsValid())
                    {
                        DataSlotConfiguration slotConfiguration;

                        slotConfiguration.m_name = dataInConfig.m_name;
                        slotConfiguration.m_toolTip = dataInConfig.m_toolTip;                        
                        slotConfiguration.SetType(dataInConfig.m_type);
                        slotConfiguration.SetConnectionType(ConnectionType::Input);

                        dataInConfig.m_slotId = AddSlot(slotConfiguration);
                    }
                    else
                    {
                        DynamicDataSlotConfiguration slotConfiguration;

                        slotConfiguration.m_name = dataInConfig.m_name;
                        slotConfiguration.m_toolTip = dataInConfig.m_toolTip;
                        slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
                        slotConfiguration.SetConnectionType(ConnectionType::Input);

                        dataInConfig.m_slotId = AddSlot(slotConfiguration);
                    }

                    dataInConfig.m_state = SlotConfigState::Unmodified;
                }
            }

            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }

        AZ::Crc32 Mock::OnDataOutSlotArrayChanged()
        {
            const auto pendingConfigRemovals = AZStd::move(m_pendingConfigRemovals);
            for (const auto& oldSlotId : pendingConfigRemovals)
            {
                RemoveSlot(oldSlotId);
            }
            for (auto& dataOutConfig : m_dataOutConfigArray)
            {
                dataOutConfig.m_owner = this;
                if (dataOutConfig.m_state == SlotConfigState::New)
                {
                    DynamicDataSlotConfiguration slotConfiguration;

                    slotConfiguration.m_name = dataOutConfig.m_name;
                    slotConfiguration.m_toolTip = dataOutConfig.m_toolTip;
                    slotConfiguration.SetConnectionType(ConnectionType::Output);

                    dataOutConfig.m_slotId = AddSlot(slotConfiguration);

                    dataOutConfig.m_state = SlotConfigState::Unmodified;
                }
            }
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }

        AZ::Crc32 Mock::OnExecutionInSlotArrayChanged()
        {
            const auto pendingConfigRemovals = AZStd::move(m_pendingConfigRemovals);
            for (const auto& oldSlotId : pendingConfigRemovals)
            {
                RemoveSlot(oldSlotId);
            }
            for (auto& executionInConfig : m_executionInConfigArray)
            {
                executionInConfig.m_owner = this;
                if (executionInConfig.m_state == SlotConfigState::New)
                {
                    ExecutionSlotConfiguration slotConfiguration;

                    slotConfiguration.m_name = executionInConfig.m_name;
                    slotConfiguration.m_toolTip = executionInConfig.m_toolTip;
                    slotConfiguration.SetConnectionType(ConnectionType::Input);

                    executionInConfig.m_slotId = AddSlot(slotConfiguration);

                    executionInConfig.m_state = SlotConfigState::Unmodified;
                }
            }
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }

        AZ::Crc32 Mock::OnExecutionOutSlotArrayChanged()
        {
            const auto pendingConfigRemovals = AZStd::move(m_pendingConfigRemovals);
            for (const auto& oldSlotId : pendingConfigRemovals)
            {
                RemoveSlot(oldSlotId);
            }
            for (auto& executionOutConfig : m_executionOutConfigArray)
            {
                executionOutConfig.m_owner = this;
                if (executionOutConfig.m_state == SlotConfigState::New)
                {
                    ExecutionSlotConfiguration slotConfiguration;

                    slotConfiguration.m_name = executionOutConfig.m_name;
                    slotConfiguration.m_toolTip = executionOutConfig.m_toolTip;
                    slotConfiguration.SetConnectionType(ConnectionType::Output);

                    executionOutConfig.m_slotId = AddSlot(slotConfiguration);

                    executionOutConfig.m_state = SlotConfigState::Unmodified;
                }
            }
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }

        void Mock::OnSlotAddedToNode(const AZ::EntityId& slotId)
        {
            if (AZStd::find(m_graphCanvasSlotIds.begin(), m_graphCanvasSlotIds.end(), slotId) == m_graphCanvasSlotIds.end())
            {
                m_graphCanvasSlotIds.push_back(slotId);
            }
        }

        void Mock::OnSlotRemovedFromNode(const AZ::EntityId& slotId)
        {
            auto slotIt = AZStd::find(m_graphCanvasSlotIds.begin(), m_graphCanvasSlotIds.end(), slotId);
            if (slotIt == m_graphCanvasSlotIds.end())
            {
                m_graphCanvasSlotIds.erase(slotIt);
            }
        }

        void Mock::OnAddedToScene([[maybe_unused]] const AZ::EntityId& sceneId)
        {
            OnNodeTitleChanged();
            OnNodeSubTitleChanged();
            OnNodeColorPaletteChanged();

            for (auto& slotConfig : m_executionInConfigArray)
            {
                slotConfig.OnSlotNameChanged();
                slotConfig.OnSlotToolTipChanged();
            }
            for (auto& slotConfig : m_executionOutConfigArray)
            {
                slotConfig.OnSlotNameChanged();
                slotConfig.OnSlotToolTipChanged();
            }
            for (auto& slotConfig : m_dataInConfigArray)
            {
                slotConfig.OnSlotNameChanged();
                slotConfig.OnSlotToolTipChanged();
            }
            for (auto& slotConfig : m_dataOutConfigArray)
            {
                slotConfig.m_owner = this;
                slotConfig.OnSlotNameChanged();
                slotConfig.OnSlotToolTipChanged();
            }

            MockDescriptorNotificationBus::Event(GetEntityId(), &MockDescriptorNotifications::OnGraphCanvasNodeSetup, m_graphCanvasNodeId);            
        }

        void Mock::OnSceneMemberDeserialized([[maybe_unused]] const GraphCanvas::GraphId& graphId, [[maybe_unused]] const GraphCanvas::GraphSerialization& serialization)
        {
            OnGraphCanvasNodeDisplayed(m_graphCanvasNodeId);
        }
    }
}
