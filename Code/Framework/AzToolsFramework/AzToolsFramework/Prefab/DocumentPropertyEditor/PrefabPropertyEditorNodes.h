/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/DocumentPropertyEditor/DocumentSchema.h>

namespace AzToolsFramework::Prefab::PrefabPropertyEditorNodes
{
    struct PrefabOverrideLabel : AZ::DocumentPropertyEditor::NodeDefinition
    {
        static constexpr AZStd::string_view Name = "PrefabOverrideLabel";
        static constexpr auto Value = AZ::DocumentPropertyEditor::AttributeDefinition<AZStd::string_view>("Value");
        static constexpr auto IsOverridden = AZ::DocumentPropertyEditor::AttributeDefinition<bool>("IsOverridden");
        static constexpr auto RelativePath = AZ::DocumentPropertyEditor::AttributeDefinition<AZStd::string_view>("RelativePath");
        static constexpr auto RevertOverride =
            AZ::DocumentPropertyEditor::CallbackAttributeDefinition<void(AZStd::string_view)>("RevertOverride");
    };
} // namespace AzToolsFramework::Prefab::PrefabPropertyEditorNodes
