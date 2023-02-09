/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>
#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabAdapter.h>
#include <AzToolsFramework/Prefab/Overrides/PrefabOverridePublicInterface.h>

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

        AZ::Interface<PrefabAdapterInterface>::Register(this);
    }

    PrefabAdapter::~PrefabAdapter()
    {
        AZ::Interface<PrefabAdapterInterface>::Unregister(this);
    }

    void PrefabAdapter::AddPropertyHandlerIfOverridden(
        AZ::DocumentPropertyEditor::AdapterBuilder* adapterBuilder, const AZ::Dom::Path& componentPathFromEntity, AZ::EntityId entityId) 
    {
        if (m_prefabOverridePublicInterface->AreOverridesPresent(entityId, componentPathFromEntity))
        {
            adapterBuilder->BeginPropertyEditor<AZ::DocumentPropertyEditor::Nodes::PrefabOverrideProperty>();
            adapterBuilder->Attribute(AZ::DocumentPropertyEditor::Nodes::PropertyEditor::SharePriorColumn, true);
            adapterBuilder->Attribute(AZ::DocumentPropertyEditor::Nodes::PropertyEditor::UseMinimumWidth, true);
            adapterBuilder->Attribute(
                AZ::DocumentPropertyEditor::Nodes::PropertyEditor::Alignment,
                AZ::DocumentPropertyEditor::Nodes::PropertyEditor::Align::AlignLeft);
            adapterBuilder->EndPropertyEditor();
        }
    }
}
