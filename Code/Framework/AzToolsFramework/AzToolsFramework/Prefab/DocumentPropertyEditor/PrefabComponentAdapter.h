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

    private:
        AZStd::string m_componentAlias;

        PrefabOverridePublicInterface* m_prefabOverridePublicInterface = nullptr;
        PrefabPublicInterface* m_prefabPublicInterface = nullptr;
    };

} // namespace AzToolsFramework::Prefab
