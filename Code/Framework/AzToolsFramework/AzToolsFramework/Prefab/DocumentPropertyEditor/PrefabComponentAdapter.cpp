/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabComponentAdapter.h>
#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabOverrideLabelHandler.h>
#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabPropertyEditorNodes.h>
#include <AzToolsFramework/Prefab/Overrides/PrefabOverridePublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>

namespace AzToolsFramework::Prefab
{
    PrefabComponentAdapter::PrefabComponentAdapter()
        : ComponentAdapter()
    {
        m_prefabOverridePublicInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabOverridePublicInterface>::Get();
        AZ_Assert(m_prefabOverridePublicInterface, "Could not get PrefabOverridePublicInterface on PrefabComponentAdapter construction.");

        m_prefabPublicInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabPublicInterface>::Get();
        AZ_Assert(m_prefabPublicInterface, "Could not get PrefabPublicInterface on PrefabComponentAdapter construction.");
    }

    PrefabComponentAdapter::~PrefabComponentAdapter()
    {
    }

    void PrefabComponentAdapter::SetComponent(AZ::Component* componentInstance)
    {
        // Set the component alias before calling SetValue() in base SetComponent().
        // Otherwise, an empty component alias will be used in DOM data.
        m_componentAlias = componentInstance->GetSerializedIdentifier();

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

        return ComponentAdapter::HandleMessage(message);
    }

} // namespace AzToolsFramework::Prefab
