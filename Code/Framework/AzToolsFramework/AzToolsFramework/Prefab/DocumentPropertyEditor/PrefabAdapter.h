/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabAdapterInterface.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/DPEComponentAdapter.h>

namespace AzToolsFramework::Prefab
{
    class PrefabOverridePublicInterface;
    class PrefabPublicInterface;

    class PrefabAdapter
        : public AZ::DocumentPropertyEditor::ReflectionAdapter
        , private PrefabAdapterInterface
    {
    public:
        PrefabAdapter();
        ~PrefabAdapter();
    private:
        void AddPropertyLabelNode(
            AZ::DocumentPropertyEditor::AdapterBuilder* adapterBuilder,
            AZStd::string_view labelText,
            const AZ::Dom::Path& relativePathFromEntity,
            AZ::EntityId entityId) override;

        PrefabOverridePublicInterface* m_prefabOverridePublicInterface = nullptr;
        PrefabPublicInterface* m_prefabPublicInterface = nullptr;
    };

} // namespace AzToolsFramework::Prefab
