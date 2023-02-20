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
    struct PrefabOverrideProperty : AZ::DocumentPropertyEditor::NodeDefinition
    {
        static constexpr AZStd::string_view Name = "PrefabOverrideProperty";
    };
} // namespace AzToolsFramework::Prefab::PrefabPropertyEditorNodes
