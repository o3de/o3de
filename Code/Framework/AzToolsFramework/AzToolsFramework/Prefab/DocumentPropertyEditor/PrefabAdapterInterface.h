/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    namespace DocumentPropertyEditor
    {
        class AdapterBuilder;
    }

    namespace Dom
    {
        class Path;
    }
}

namespace AzToolsFramework::Prefab
{
    class PrefabAdapterInterface
    {
    public:
        AZ_RTTI(PrefabAdapterInterface, "{8272C235-1642-432F-8A51-63E9B4488D55}");

        //! Adds a property label node to the DPE DOM and configures its style if an override is present
        //! on the entity at the provided path.
        //! @param adapterBuilder The builder to use to add the property label node.
        //! @param labelText The text string to be displayed in label.
        //! @param relativePathFromEntity The path as seen by the entity a component or its properties.
        //! @param entityId The entity id to use to query the prefab system about the presence of overrides.
        virtual void AddPropertyLabelNode(
            AZ::DocumentPropertyEditor::AdapterBuilder* adapterBuilder,
            AZStd::string_view labelText,
            const AZ::Dom::Path& relativePathFromEntity,
            AZ::EntityId entityId) = 0;
    };
} // namespace AzToolsFramework::Prefab
