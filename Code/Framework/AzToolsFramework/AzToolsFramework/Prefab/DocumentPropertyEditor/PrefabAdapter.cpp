/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabAdapter.h>
#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabPropertyEditorNodes.h>
#include <AzToolsFramework/Prefab/DocumentPropertyEditor/OverridePropertyHandler.h>
#include <AzToolsFramework/Prefab/Overrides/PrefabOverridePublicInterface.h>
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

        auto* propertyEditorToolsSystemInterface = AZ::Interface<PropertyEditorToolsSystemInterface>::Get();
        AZ_Assert(
            propertyEditorToolsSystemInterface != nullptr,
            "PrefabAdapter::PrefabAdapter() - PropertyEditorToolsSystemInterface is not available");
        propertyEditorToolsSystemInterface->RegisterHandler<OverridePropertyHandler>();

        AZ::Interface<PrefabAdapterInterface>::Register(this);
    }

    PrefabAdapter::~PrefabAdapter()
    {
        AZ::Interface<PrefabAdapterInterface>::Unregister(this);
    }

    void PrefabAdapter::AddPropertyHandlerIfOverridden(
        AZ::DocumentPropertyEditor::AdapterBuilder* adapterBuilder, const AZ::Dom::Path& relativePathFromEntity, AZ::EntityId entityId) 
    {
        if (m_prefabOverridePublicInterface->AreOverridesPresent(entityId, relativePathFromEntity.ToString()))
        {
            adapterBuilder->BeginPropertyEditor<PrefabPropertyEditorNodes::PrefabOverrideProperty>();
            adapterBuilder->Attribute(AZ::DocumentPropertyEditor::Nodes::PropertyEditor::SharePriorColumn, true);
            adapterBuilder->Attribute(AZ::DocumentPropertyEditor::Nodes::PropertyEditor::UseMinimumWidth, true);
            adapterBuilder->Attribute(
                AZ::DocumentPropertyEditor::Nodes::PropertyEditor::Alignment,
                AZ::DocumentPropertyEditor::Nodes::PropertyEditor::Align::AlignLeft);
            adapterBuilder->EndPropertyEditor();
        }
    }
} // namespace AzToolsFramework::Prefab
