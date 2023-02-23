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

    void PrefabAdapter::AddPropertyIconAndLabel(
        AZ::DocumentPropertyEditor::AdapterBuilder* adapterBuilder,
        const AZStd::string_view labelText,
        const AZ::Dom::Path& relativePathFromEntity,
        AZ::EntityId entityId)
    {
        using PrefabPropertyEditorNodes::PrefabOverrideLabel;

        // Do not show override visualization on container entities.
        if (m_prefabPublicInterface->IsInstanceContainerEntity(entityId))
        {
            adapterBuilder->BeginPropertyEditor<PrefabOverrideLabel>();
            adapterBuilder->Attribute(PrefabOverrideLabel::Text, labelText);
            adapterBuilder->Attribute(PrefabOverrideLabel::IsOverridden, false);
            adapterBuilder->EndPropertyEditor();
        }
        else
        {
            bool isPropertyOverridden = false;

            // Mark as non-overridden if the relative path is empty.
            if (!relativePathFromEntity.IsEmpty() &&
                m_prefabOverridePublicInterface->AreOverridesPresent(entityId, relativePathFromEntity.ToString()))
            {
                isPropertyOverridden = true;
            }

            adapterBuilder->BeginPropertyEditor<PrefabOverrideLabel>();
            adapterBuilder->Attribute(PrefabOverrideLabel::Text, labelText);
            adapterBuilder->Attribute(PrefabOverrideLabel::IsOverridden, isPropertyOverridden);
            adapterBuilder->EndPropertyEditor();
        }
    }
} // namespace AzToolsFramework::Prefab
