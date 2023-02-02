/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/Backends/JSON/JsonSerializationUtils.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/DPEComponentAdapter.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>
#include <QtCore/QTimer>
namespace AZ::DocumentPropertyEditor
{
    ComponentAdapter::ComponentAdapter()
    {
        m_propertyChangeHandler = ReflectionAdapter::PropertyChangeEvent::Handler(
            [this](const ReflectionAdapter::PropertyChangeInfo& changeInfo)
            {
                this->GeneratePropertyEditPatch(changeInfo);
            });
        ConnectPropertyChangeHandler(m_propertyChangeHandler);
        m_prefabOverridePublicInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabOverridePublicInterface>::Get();
        if (m_prefabOverridePublicInterface == nullptr)
        {
            AZ_Assert(false, "Could not get PrefabOverridePublicInterface on ComponentAdapter construction.");
            return;
        }
        
    }

    ComponentAdapter::ComponentAdapter(AZ::Component* componentInstace)
    {
        m_propertyChangeHandler = ReflectionAdapter::PropertyChangeEvent::Handler(
            [this](const ReflectionAdapter::PropertyChangeInfo& changeInfo)
            {
                this->GeneratePropertyEditPatch(changeInfo);
            });
        ConnectPropertyChangeHandler(m_propertyChangeHandler);
        SetComponent(componentInstace);
    }

    ComponentAdapter::~ComponentAdapter()
    {
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::BusDisconnect();
        AzToolsFramework::ToolsApplicationEvents::Bus::Handler::BusDisconnect();
        AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusDisconnect(m_componentInstance->GetEntityId());
    }

    void ComponentAdapter::OnEntityComponentPropertyChanged(AZ::ComponentId componentId)
    {
        if (m_componentInstance->GetId() == componentId)
        {
            m_queuedRefreshLevel = AzToolsFramework::PropertyModificationRefreshLevel::Refresh_Values;
            QTimer::singleShot(
                0,
                [this]()
                {
                    DoRefresh();
                });
        }
    }

    void ComponentAdapter::InvalidatePropertyDisplay(AzToolsFramework::PropertyModificationRefreshLevel level)
    {
        if (level > m_queuedRefreshLevel)
        {
            m_queuedRefreshLevel = level;
            QTimer::singleShot(
                0,
                [this]()
                {
                    DoRefresh();
                });
        }
    }

    void ComponentAdapter::RequestRefresh(AzToolsFramework::PropertyModificationRefreshLevel level)
    {
        if (level > m_queuedRefreshLevel)
        {
            m_queuedRefreshLevel = level;
            QTimer::singleShot(
                0,
                [this]()
                {
                    DoRefresh();
                });
        }
    }

    void ComponentAdapter::SetComponent(AZ::Component* componentInstance)
    {
        m_componentInstance = componentInstance;
        AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusConnect(m_componentInstance->GetEntityId());
        AzToolsFramework::ToolsApplicationEvents::Bus::Handler::BusConnect();
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::BusConnect();
        AZ::Uuid instanceTypeId = azrtti_typeid(m_componentInstance);
        SetValue(m_componentInstance, instanceTypeId);
        m_componentAlias = componentInstance->GetAlias();
        auto owningInstance =
            AZ::Interface<AzToolsFramework::Prefab::InstanceEntityMapperInterface>::Get()->FindOwningInstance(componentInstance->GetEntityId());
        AZ_Assert(owningInstance.has_value(), "Entity owning the component doesn't have an owning prefab instance.");
        auto entityAlias = owningInstance->get().GetEntityAlias(componentInstance->GetEntityId());
        AZ_Assert(entityAlias.has_value(), "Owning entity of component doesn't have a valid entity alias in the owning prefab.");
        m_entityAlias = entityAlias->get();
        m_entityId = m_componentInstance->GetEntityId();
    }

    void ComponentAdapter::DoRefresh()
    {
        if (m_queuedRefreshLevel == AzToolsFramework::PropertyModificationRefreshLevel::Refresh_None)
        {
            return;
        }
        m_queuedRefreshLevel = AzToolsFramework::PropertyModificationRefreshLevel::Refresh_None;
        NotifyResetDocument();
    }

    Dom::Value ComponentAdapter::HandleMessage(const AdapterMessage& message)
    {
        auto handlePropertyEditorChanged = [&]([[maybe_unused]] const Dom::Value& valueFromEditor, Nodes::ValueChangeType changeType)
        {
            switch (changeType)
            {
            case Nodes::ValueChangeType::InProgressEdit:
                if (m_componentInstance)
                {
                    const AZ::EntityId& entityId = m_componentInstance->GetEntityId();
                    if (entityId.IsValid())
                    {
                        if (m_currentUndoNode)
                        {
                            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                                m_currentUndoNode,
                                &AzToolsFramework::ToolsApplicationRequests::ResumeUndoBatch,
                                m_currentUndoNode,
                                "Modify Entity Property");
                        }
                        else
                        {
                            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                                m_currentUndoNode, &AzToolsFramework::ToolsApplicationRequests::BeginUndoBatch, "Modify Entity Property");
                        }

                        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                            &AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, entityId);
                    }
                }
                break;
            case Nodes::ValueChangeType::FinishedEdit:
                if (m_currentUndoNode)
                {
                    AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::EndUndoBatch);
                    m_currentUndoNode = nullptr;
                }
                break;
            }
        };

        Dom::Value returnValue = message.Match(Nodes::PropertyEditor::OnChanged, handlePropertyEditorChanged);

        ReflectionAdapter::HandleMessage(message);

        return returnValue;
    }

    void ComponentAdapter::AddIconIfPropertyOverride(AdapterBuilder* adapterBuilder, const AZStd::string_view& serializedPath)
    {
        AZ::Dom::Path prefabPatchPath(AzToolsFramework::Prefab::PrefabDomUtils::ComponentsName);
        prefabPatchPath /= m_componentAlias;
        prefabPatchPath /= AZ::Dom::Path(serializedPath);
        if (!serializedPath.empty() && m_prefabOverridePublicInterface->AreOverridesPresent(m_entityId, prefabPatchPath))
        {
            adapterBuilder->BeginPropertyEditor<Nodes::OverrideIcon>();
            adapterBuilder->Attribute(Nodes::PropertyEditor::SharePriorColumn, true);
            adapterBuilder->Attribute(Nodes::PropertyEditor::UseMinimumWidth, true);
            adapterBuilder->Attribute(Nodes::PropertyEditor::Alignment, Nodes::PropertyEditor::Align::AlignLeft);
            adapterBuilder->EndPropertyEditor();
        }
    }

    void ComponentAdapter::GeneratePropertyEditPatch(const ReflectionAdapter::PropertyChangeInfo& propertyChangeInfo)
    {
        if (propertyChangeInfo.changeType == Nodes::ValueChangeType::FinishedEdit)
        {
            AZ::Dom::Value domValue = GetContents();
            AZ::Dom::Path serializedPath = propertyChangeInfo.path / Reflection::DescriptorAttributes::SerializedPath;

            AZ::Dom::Path prefabPatchPath(AzToolsFramework::Prefab::PrefabDomUtils::EntitiesName);
            prefabPatchPath /= m_entityAlias;
            prefabPatchPath /= AzToolsFramework::Prefab::PrefabDomUtils::ComponentsName;
            prefabPatchPath /= m_componentAlias;
            prefabPatchPath /= serializedPath;

             
            AzToolsFramework::Prefab::PrefabDom prefabPatch;
            prefabPatch.SetObject();

            AZStd::string patchPath = domValue[serializedPath].GetString();
            rapidjson::Value path = rapidjson::Value(patchPath.c_str(),
                aznumeric_caster(patchPath.size()),
                prefabPatch.GetAllocator());
            prefabPatch.AddMember(rapidjson::StringRef("op"), rapidjson::StringRef("replace"), prefabPatch.GetAllocator())
                .AddMember(
                    rapidjson::StringRef("path"),
                    rapidjson::Value(patchPath.c_str(), aznumeric_caster(patchPath.size())),
                    prefabPatch.GetAllocator());
            
            
            if (propertyChangeInfo.newValue.IsOpaqueValue())
            {
                AZStd::any opaqueValue = propertyChangeInfo.newValue.GetOpaqueValue();
                void* marshalledPointer = AZ::Dom::Utils::TryMarshalValueToPointer(propertyChangeInfo.newValue, opaqueValue.type());
                rapidjson::Document patchValue;
                auto result =
                    JsonSerialization::Store(patchValue, prefabPatch.GetAllocator(), marshalledPointer, nullptr, opaqueValue.type());
                prefabPatch.AddMember(rapidjson::StringRef("value"), AZStd::move(patchValue), prefabPatch.GetAllocator());
            }
            else
            {
                auto convertToRapidJsonOutcome = AZ::Dom::Json::WriteToRapidJsonDocument(
                    [propertyChangeInfo](AZ::Dom::Visitor& visitor)
                    {
                        const bool copyStrings = false;
                        return propertyChangeInfo.newValue.Accept(visitor, copyStrings);
                    });

                if (!convertToRapidJsonOutcome.IsSuccess())
                {
                    AZ_Assert(false, "PrefabDom value converted from AZ::Dom::Value.");
                }
                else
                {
                    AzToolsFramework::Prefab::PrefabDom prefabPatchValue = convertToRapidJsonOutcome.TakeValue();
                    prefabPatch.AddMember(rapidjson::StringRef("value"), AZStd::move(prefabPatchValue), prefabPatch.GetAllocator());
                    AZ_Warning("Prefab", !prefabPatch.IsNull(), "Prefab patch generated from DPE is null");
                }
            }

            auto* prefabFocusPublicInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabFocusPublicInterface>::Get();
            AZ_Assert(prefabFocusPublicInterface, "PrefabFocusPublicInterface cannot be fetched.");
        }
    }

} // namespace AZ::DocumentPropertyEditor
