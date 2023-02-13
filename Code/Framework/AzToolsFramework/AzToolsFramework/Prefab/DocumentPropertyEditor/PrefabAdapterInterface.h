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
    class DocumentPropertyEditor::AdapterBuilder;
    class Dom::Path;
}

namespace AzToolsFramework::Prefab
{
    class PrefabAdapterInterface
    {
    public:
        AZ_RTTI(PrefabAdapterInterface, "{8272C235-1642-432F-8A51-63E9B4488D55}");

        //! Adds a property handler to the DPE DOM if an override is present on the entity at the provided path.
        //! @param adapterBuilder The builder to use to add the property handler. It could have already added other things to DOM.
        //! @param path The path as seen by the entity to any one of its component properties
        //! @param entityId The entity id to use to query the prefab system about the presence of overrides.
        virtual void AddPropertyHandlerIfOverridden(
            AZ::DocumentPropertyEditor::AdapterBuilder* adapterBuilder, const AZ::Dom::Path& path, AZ::EntityId entityId) = 0;
    };
} // namespace AzToolsFramework::Prefab
