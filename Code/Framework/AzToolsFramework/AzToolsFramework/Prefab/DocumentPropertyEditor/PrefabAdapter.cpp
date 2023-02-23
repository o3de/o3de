/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabAdapter.h>
#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabOverrideLabelHandler.h>
#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabPropertyEditorNodes.h>
#include <AzToolsFramework/Prefab/Overrides/PrefabOverridePublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/PropertyEditorToolsSystemInterface.h>

namespace AzToolsFramework::Prefab
{
    PrefabAdapter::PrefabAdapter()
    {
        m_prefabOverridePublicInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabOverridePublicInterface>::Get();
        if (m_prefabOverridePublicInterface == nullptr)
        {
            AZ_Assert(false, "Could not get PrefabOverridePublicInterface on PrefabAdapter construction.");
            return;
        }

        m_prefabPublicInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabPublicInterface>::Get();
        if (m_prefabPublicInterface == nullptr)
        {
            AZ_Assert(false, "Could not get PrefabPublicInterface on PrefabAdapter construction.");
            return;
        }

        auto* propertyEditorToolsSystemInterface = AZ::Interface<PropertyEditorToolsSystemInterface>::Get();
        AZ_Assert(
            propertyEditorToolsSystemInterface != nullptr,
            "PrefabAdapter::PrefabAdapter() - PropertyEditorToolsSystemInterface is not available");
        propertyEditorToolsSystemInterface->RegisterHandler<PrefabOverrideLabelHandler>();

        AZ::Interface<PrefabAdapterInterface>::Register(this);
    }

    PrefabAdapter::~PrefabAdapter()
    {
        AZ::Interface<PrefabAdapterInterface>::Unregister(this);
    }

    void PrefabAdapter::AddPropertyLabelNode(
        AZ::DocumentPropertyEditor::AdapterBuilder* adapterBuilder,
        AZStd::string_view labelText,
        const AZ::Dom::Path& relativePathFromEntity,
        AZ::EntityId entityId)
    {
        using PrefabPropertyEditorNodes::PrefabOverrideLabel;

        adapterBuilder->BeginPropertyEditor<PrefabOverrideLabel>();
        adapterBuilder->Attribute(PrefabOverrideLabel::Text, labelText);

        // Do not show override visualization on container entities or for empty serialized paths.
        if (m_prefabPublicInterface->IsInstanceContainerEntity(entityId) || relativePathFromEntity.IsEmpty())
        {
            adapterBuilder->Attribute(PrefabOverrideLabel::IsOverridden, false);
        }
        else
        {
            bool isOverridden = m_prefabOverridePublicInterface->AreOverridesPresent(entityId, relativePathFromEntity.ToString());
            adapterBuilder->Attribute(PrefabOverrideLabel::IsOverridden, isOverridden);
        }
        
        adapterBuilder->EndPropertyEditor();
    }
} // namespace AzToolsFramework::Prefab
