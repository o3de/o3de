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

namespace AzToolsFramework::Prefab::DocumentPropertyEditor
{
    class PrefabAdapterInterface
    {
    public:
        AZ_RTTI(PrefabAdapterInterface, "{8272C235-1642-432F-8A51-63E9B4488D55}");

        virtual void AddPropertyHandlerIfOverridden(AZ::DocumentPropertyEditor::AdapterBuilder*, const AZ::Dom::Path&, AZ::EntityId entityId) = 0;
    };
} // namespace AzToolsFramework::Prefab::DocumentPropertyEditor
