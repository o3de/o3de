/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/UI/DocumentPropertyEditor/DPEComponentAdapter.h>

namespace AzToolsFramework::Prefab
{
    class PrefabOverridePublicInterface;
    class PrefabPublicInterface;

    class PrefabComponentAdapter
        : public AZ::DocumentPropertyEditor::ComponentAdapter
    {
    public:
        PrefabComponentAdapter();
        ~PrefabComponentAdapter();

        void SetComponent(AZ::Component* componentInstance) override;

        void CreateLabel(
            AZ::DocumentPropertyEditor::AdapterBuilder* adapterBuilder,
            AZStd::string_view labelText,
            AZStd::string_view serializedPath) override;

        AZ::Dom::Value HandleMessage(const AZ::DocumentPropertyEditor::AdapterMessage& message) override;

        //! Updates the DPE DOM using the property change information provided. If the property is owned by the focused prefab,
        //! the change is applied as direct template edit. If the property is owned by descendant of the focused prefab, it is
        //! applied as an override from the focused prefab.
        //! @param propertyChangeInfo The object containing information about the property change.
        void UpdateDomContents(const PropertyChangeInfo& propertyChangeInfo) override;

    private:

        //! Creates and applies a component edit prefab patch using the property change information provided.
        //! @param relativePathFromOwningPrefab The path to the property in the prefab from the owning prefab.
        //! @param propertyChangeInfo The object containing information about the property change.
        bool CreateAndApplyComponentEditPatch(
            AZStd::string_view relativePathFromOwningPrefab,
            const AZ::DocumentPropertyEditor::ReflectionAdapter::PropertyChangeInfo& propertyChangeInfo);

        //! Creates and applies a component override prefab patch using the property change information provided. 
        //! @param relativePathFromOwningPrefab The path to the property in the prefab from the owning prefab.
        //! @param propertyChangeInfo The object containing information about the property change.
        bool CreateAndApplyComponentOverridePatch(
            AZ::Dom::Path relativePathFromOwningPrefab,
            const AZ::DocumentPropertyEditor::ReflectionAdapter::PropertyChangeInfo& propertyChangeInfo);

        //! Checks if the component is disabled.
        static bool IsComponentDisabled(const AZ::Component* component);

        //! Checks if the component is pending.
        static bool IsComponentPending(const AZ::Component* component);

        AZStd::string m_entityAlias;
        AZStd::string m_componentAlias;

        PrefabOverridePublicInterface* m_prefabOverridePublicInterface = nullptr;
        PrefabPublicInterface* m_prefabPublicInterface = nullptr;
    };

} // namespace AzToolsFramework::Prefab
