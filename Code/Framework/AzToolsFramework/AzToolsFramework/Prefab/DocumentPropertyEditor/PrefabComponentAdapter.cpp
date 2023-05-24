/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/Backends/JSON/JsonSerializationUtils.h>
#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabComponentAdapter.h>
#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabOverrideLabelHandler.h>
#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabPropertyEditorNodes.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceUpdateExecutorInterface.h>
#include <AzToolsFramework/Prefab/Overrides/PrefabOverridePublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoComponentPropertyEdit.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoComponentPropertyOverride.h>
#include <AzToolsFramework/ToolsComponents/EditorDisabledCompositionBus.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionBus.h>

namespace AzToolsFramework::Prefab
{
    PrefabComponentAdapter::PrefabComponentAdapter()
        : ComponentAdapter()
    {
        m_prefabOverridePublicInterface = AZ::Interface<PrefabOverridePublicInterface>::Get();
        AZ_Assert(m_prefabOverridePublicInterface, "Could not get PrefabOverridePublicInterface on PrefabComponentAdapter construction.");

        m_prefabPublicInterface = AZ::Interface<PrefabPublicInterface>::Get();
        AZ_Assert(m_prefabPublicInterface, "Could not get PrefabPublicInterface on PrefabComponentAdapter construction.");
    }

    PrefabComponentAdapter::~PrefabComponentAdapter()
    {
    }

    void PrefabComponentAdapter::SetComponent(AZ::Component* componentInstance)
    {
        auto owningInstance = AZ::Interface<InstanceEntityMapperInterface>::Get()->FindOwningInstance(
            componentInstance->GetEntityId());
        AZ_Assert(owningInstance.has_value(), "Entity owning the component doesn't have an owning prefab instance.");
        auto entityAlias = owningInstance->get().GetEntityAlias(componentInstance->GetEntityId());
        AZ_Assert(entityAlias.has_value(), "Owning entity of component doesn't have a valid entity alias in the owning prefab.");
        m_entityAlias = entityAlias->get();

        // Set the component alias before calling SetValue() in base SetComponent().
        // Otherwise, an empty component alias will be used in DOM data.
        AZ_Assert(componentInstance, "PrefabComponentAdapter::SetComponent - component is null.")
        m_componentAlias = componentInstance->GetSerializedIdentifier();

        // Do not assert on empty alias for disabled or pending components.
        // See GHI: https://github.com/o3de/o3de/issues/15546
        if (!IsComponentDisabled(componentInstance) && !IsComponentPending(componentInstance))
        {
            AZ_Assert(!m_componentAlias.empty(), "PrefabComponentAdapter::SetComponent - Component alias should not be empty.");
        }

        ComponentAdapter::SetComponent(componentInstance);

        m_currentUndoBatch = nullptr;
    }

    void PrefabComponentAdapter::CreateLabel(
        AZ::DocumentPropertyEditor::AdapterBuilder* adapterBuilder, AZStd::string_view labelText, AZStd::string_view serializedPath)
    {
        using PrefabPropertyEditorNodes::PrefabOverrideLabel;

        adapterBuilder->BeginPropertyEditor<PrefabOverrideLabel>();
        adapterBuilder->Attribute(PrefabOverrideLabel::Text, labelText);

        AZ::Dom::Path relativePathFromEntity;
        if (!m_componentAlias.empty() && !serializedPath.empty())
        {
            relativePathFromEntity /= PrefabDomUtils::ComponentsName;
            relativePathFromEntity /= m_componentAlias;
            relativePathFromEntity /= AZ::Dom::Path(serializedPath);
        }

        adapterBuilder->Attribute(PrefabOverrideLabel::RelativePath, relativePathFromEntity.ToString());
        adapterBuilder->AddMessageHandler(this, PrefabOverrideLabel::RevertOverride);

        // Do not show override visualization on container entities or for empty serialized paths.
        if (m_prefabPublicInterface->IsInstanceContainerEntity(m_entityId) || relativePathFromEntity.IsEmpty())
        {
            adapterBuilder->Attribute(PrefabOverrideLabel::IsOverridden, false);
        }
        else
        {
            bool isOverridden = m_prefabOverridePublicInterface->AreOverridesPresent(m_entityId, relativePathFromEntity.ToString());
            adapterBuilder->Attribute(PrefabOverrideLabel::IsOverridden, isOverridden);
        }

        adapterBuilder->EndPropertyEditor();
    }

    AZ::Dom::Value PrefabComponentAdapter::HandleMessage(const AZ::DocumentPropertyEditor::AdapterMessage& message)
    {
        auto handleRevertOverride = [this](AZStd::string_view relativePathFromEntity)
        {
            m_prefabOverridePublicInterface->RevertOverrides(m_entityId, relativePathFromEntity);
        };

        message.Match(PrefabPropertyEditorNodes::PrefabOverrideLabel::RevertOverride, handleRevertOverride);

        return ReflectionAdapter::HandleMessage(message);
    }

    void PrefabComponentAdapter::UpdateDomContents(const PropertyChangeInfo& propertyChangeInfo)
    {
        if (propertyChangeInfo.changeType == AZ::DocumentPropertyEditor::Nodes::ValueChangeType::InProgressEdit)
        {
            if (auto instanceUpdateExecutorInterface = AZ::Interface<Prefab::InstanceUpdateExecutorInterface>::Get())
            {
                instanceUpdateExecutorInterface->SetShouldPauseInstancePropagation(true);
            }

            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                m_currentUndoBatch, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetCurrentUndoBatch);
            if (!m_currentUndoBatch)
            {
                AZ_Warning(
                    "Prefab",
                    false,
                    "New undo batch is being created in PrefabComponentAdapter. This is unusual. An undo batch should already have "
                    "existed by this point.");
                AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                    m_currentUndoBatch, &AzToolsFramework::ToolsApplicationRequests::BeginUndoBatch, "Modify Component Property");
            }

            AZ::Dom::Path serializedPath = propertyChangeInfo.path / AZ::Reflection::DescriptorAttributes::SerializedPath;

            AZ::Dom::Path relativePathFromOwningPrefab(PrefabDomUtils::EntitiesName);
            relativePathFromOwningPrefab /= m_entityAlias;
            relativePathFromOwningPrefab /= PrefabDomUtils::ComponentsName;
            relativePathFromOwningPrefab /= m_componentAlias;

            AZ::Dom::Value serializedPathValue = GetContents()[serializedPath];
            AZ_Assert(serializedPathValue.IsString(), "PrefabComponentAdapter::UpdateDomContents - SerialziedPath attribute value is not a string.");

            relativePathFromOwningPrefab /= AZ::Dom::Path(serializedPathValue.GetString());

            auto prefabFocusPublicInterface = AZ::Interface<PrefabFocusPublicInterface>::Get();
            if (prefabFocusPublicInterface->IsOwningPrefabBeingFocused(m_entityId))
            {
                if (CreateAndApplyComponentEditPatch(relativePathFromOwningPrefab.ToString(), propertyChangeInfo))
                {
                    NotifyContentsChanged(
                        { AZ::Dom::PatchOperation::ReplaceOperation(propertyChangeInfo.path / "Value", propertyChangeInfo.newValue) });
                }
            }
            else if (prefabFocusPublicInterface->IsOwningPrefabInFocusHierarchy(m_entityId))
            {
                if (CreateAndApplyComponentOverridePatch(relativePathFromOwningPrefab, propertyChangeInfo))
                {
                    AZ::Dom::Patch patches(
                        { AZ::Dom::PatchOperation::ReplaceOperation(propertyChangeInfo.path / "Value", propertyChangeInfo.newValue) });

                    AZ::Dom::Path pathToProperty = propertyChangeInfo.path;

                    // Get the path to parent row and its value.
                    pathToProperty.Pop();
                    AZ::Dom::Value propertyRowValue = GetContents()[pathToProperty];

                    AZ_Assert(
                        propertyRowValue.IsNode() &&
                            propertyRowValue.GetNodeName().GetStringView() == AZ::DocumentPropertyEditor::Nodes::Row::Name,
                        "PrefabComponentAdapter::UpdateDomContents - Parent path to property doesn't map to a 'Row' node. ");

                    AZ::Dom::Value firstRowElement = propertyRowValue[0];
                    AZ_Assert(
                        firstRowElement.IsNode() &&
                            firstRowElement.GetNodeName().GetStringView() == AZ::DocumentPropertyEditor::Nodes::PropertyEditor::Name &&
                            firstRowElement["Type"].GetString() == PrefabPropertyEditorNodes::PrefabOverrideLabel::Name,
                        "PrefabComponentAdapter::UpdateDomContents - First element in the property row is not a 'PrefabOverrideLabel'.");

                    // Patch the first child in the row, which is going to the PrefabOverrideLabel.
                    patches.PushBack(AZ::Dom::PatchOperation::ReplaceOperation(
                        pathToProperty / 0 / PrefabPropertyEditorNodes::PrefabOverrideLabel::IsOverridden.GetName(), AZ::Dom::Value(true)));
                    NotifyContentsChanged(patches);
                }
            }
        }
        else if (propertyChangeInfo.changeType == AZ::DocumentPropertyEditor::Nodes::ValueChangeType::FinishedEdit)
        {
            if (auto instanceUpdateExecutorInterface = AZ::Interface<Prefab::InstanceUpdateExecutorInterface>::Get())
            {
                instanceUpdateExecutorInterface->SetShouldPauseInstancePropagation(false);
            }

            m_currentUndoBatch = nullptr;
        }
    }

    bool PrefabComponentAdapter::CreateAndApplyComponentEditPatch(
        AZStd::string_view relativePathFromOwningPrefab, const ReflectionAdapter::PropertyChangeInfo& propertyChangeInfo)
    {
        if (!propertyChangeInfo.newValue.IsOpaqueValue())
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
                return false;
            }
            else
            {
                auto owningInstance = AZ::Interface<InstanceEntityMapperInterface>::Get()->FindOwningInstance(m_entityId);
                if (!owningInstance.has_value())
                {
                    AZ_Assert(false, "Entity owning the component doesn't have an owning prefab instance.");
                    return false;
                }

                PrefabDom afterValueOfComponentProperty = convertToRapidJsonOutcome.TakeValue();

                ScopedUndoBatch undoBatch("Update component in a prefab template");

                PrefabUndoComponentPropertyEdit* state = aznew PrefabUndoComponentPropertyEdit("Undo Updating Component");
                state->SetParent(m_currentUndoBatch);
                state->Capture(
                    owningInstance->get(), AZ::Dom::Path(relativePathFromOwningPrefab).ToString(), afterValueOfComponentProperty);
                state->Redo();

                return state->Changed();
            }
        }
        else
        {
            AZ_Assert(
                false, "Opaque property encountered in PrefabComponentAdapter::GeneratePropertyEditPatch. "
                "It should have been a serialized value.");
            return false;
        }
    }

    bool PrefabComponentAdapter::CreateAndApplyComponentOverridePatch(
        AZ::Dom::Path relativePathFromOwningPrefab, const ReflectionAdapter::PropertyChangeInfo& propertyChangeInfo)
    {
        if (!propertyChangeInfo.newValue.IsOpaqueValue())
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
                return false;
            }
            else
            {
                auto owningInstance = AZ::Interface<InstanceEntityMapperInterface>::Get()->FindOwningInstance(m_entityId);
                if (!owningInstance.has_value())
                {
                    AZ_Assert(false, "Entity owning the component doesn't have an owning prefab instance.");
                    return false;
                }

                PrefabDom afterValueOfComponentProperty = convertToRapidJsonOutcome.TakeValue();
                PrefabUndoComponentPropertyOverride* state = aznew PrefabUndoComponentPropertyOverride("Undo overriding Component");
                state->SetParent(m_currentUndoBatch);
                state->CaptureAndRedo(owningInstance->get(), relativePathFromOwningPrefab, afterValueOfComponentProperty);

                return state->Changed();
            }
        }
        else
        {
            AZ_Assert(
                false, "Opaque property encountered in PrefabComponentAdapter::CreateAndApplyComponentOverridePatch. "
                "It should have been a serialized value.");
            return false;
        }
    }

    bool PrefabComponentAdapter::IsComponentDisabled(const AZ::Component* component)
    {
        AZ_Assert(component, "Unable to check a component that is nullptr");

        bool result = false;
        EditorDisabledCompositionRequestBus::EventResult(
            result, component->GetEntityId(), &EditorDisabledCompositionRequests::IsComponentDisabled, component);
        return result;
    }

    bool PrefabComponentAdapter::IsComponentPending(const AZ::Component* component)
    {
        AZ_Assert(component, "Unable to check a component that is nullptr");

        bool result = false;
        EditorPendingCompositionRequestBus::EventResult(
            result, component->GetEntityId(), &EditorPendingCompositionRequests::IsComponentPending, component);
        return result;
    }

} // namespace AzToolsFramework::Prefab
