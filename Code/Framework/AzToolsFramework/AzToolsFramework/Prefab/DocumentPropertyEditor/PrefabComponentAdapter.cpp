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
#include <AzToolsFramework/Prefab/Overrides/PrefabOverridePublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoComponentPropertyEdit.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoComponentPropertyOverride.h>

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
        m_componentAlias = componentInstance->GetSerializedIdentifier();
        AZ_Assert(!m_componentAlias.empty(), "PrefabComponentAdapter::SetComponent - Component alias should not be empty.");

        ComponentAdapter::SetComponent(componentInstance);
    }

    void PrefabComponentAdapter::CreateLabel(
        AZ::DocumentPropertyEditor::AdapterBuilder* adapterBuilder, AZStd::string_view labelText, AZStd::string_view serializedPath)
    {
        using PrefabPropertyEditorNodes::PrefabOverrideLabel;

        adapterBuilder->BeginPropertyEditor<PrefabOverrideLabel>();
        adapterBuilder->Attribute(PrefabOverrideLabel::Text, labelText);

        AZ::Dom::Path relativePathFromEntity;
        if (!serializedPath.empty())
        {
            AZ_Assert(!m_componentAlias.empty(), "PrefabComponentAdapter::CreateLabel - Component alias should not be empty.");

            relativePathFromEntity /= PrefabDomUtils::ComponentsName;
            relativePathFromEntity /= m_componentAlias;
            relativePathFromEntity /= AZ::Dom::Path(serializedPath);
        }

        adapterBuilder->Attribute(PrefabOverrideLabel::RelativePath, relativePathFromEntity.ToString());

        // Do not show override visualization on container entities or for empty serialized paths.
        if (m_prefabPublicInterface->IsInstanceContainerEntity(m_entityId) || relativePathFromEntity.IsEmpty())
        {
            adapterBuilder->Attribute(PrefabOverrideLabel::IsOverridden, false);
        }
        else
        {
            bool isOverridden = m_prefabOverridePublicInterface->AreOverridesPresent(m_entityId, relativePathFromEntity.ToString());
            adapterBuilder->Attribute(PrefabOverrideLabel::IsOverridden, isOverridden);

            if (isOverridden)
            {
                adapterBuilder->AddMessageHandler(this, PrefabOverrideLabel::RevertOverride);
            }
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
        if (propertyChangeInfo.changeType == AZ::DocumentPropertyEditor::Nodes::ValueChangeType::FinishedEdit)
        {
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

                auto prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();

                if (!prefabSystemComponentInterface)
                {
                    AZ_Assert(false, "PrefabSystemComponentInterface is not found.");
                    return false;
                }

                const PrefabDom& templateDom = prefabSystemComponentInterface->FindTemplateDom(owningInstance->get().GetTemplateId());
                PrefabDomPath prefabDomPathToComponentProperty(relativePathFromOwningPrefab.data());
                const PrefabDomValue* beforeValueOfComponentProperty = prefabDomPathToComponentProperty.Get(templateDom);

                PrefabDom afterValueOfComponentProperty = convertToRapidJsonOutcome.TakeValue();
                ScopedUndoBatch undoBatch("Update component in a prefab template");
                PrefabUndoComponentPropertyEdit* state = aznew PrefabUndoComponentPropertyEdit("Undo Updating Component");
                state->SetParent(undoBatch.GetUndoBatch());
                state->Capture(*beforeValueOfComponentProperty, afterValueOfComponentProperty, m_entityId, relativePathFromOwningPrefab);
                state->Redo();

                return true;
            }
        }
        else
        {
            AZ_Assert(
                false, "Opaque property encountered in PrefabComponentAdapter::GeneratePropertyEditPatch. It should have been a serialized value.");
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
                ScopedUndoBatch undoBatch("override a component in a nested prefab template");
                PrefabUndoComponentPropertyOverride* state = aznew PrefabUndoComponentPropertyOverride("Undo overriding Component");
                state->SetParent(undoBatch.GetUndoBatch());
                state->CaptureAndRedo(owningInstance->get(), relativePathFromOwningPrefab, afterValueOfComponentProperty);

                return true;
            }
        }
        else
        {
            AZ_Assert(
                false, "Opaque property encountered in PrefabComponentAdapter::GeneratePropertyEditPatch. It should have been a serialized value.");
            return false;
        }
    }
} // namespace AzToolsFramework::Prefab
