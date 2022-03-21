/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/DocumentPropertyEditor/DocumentSchema.h>

namespace AZ::DocumentPropertyEditor::Nodes
{
    //! Adapter: The top-level tag for a DocumentAdapter that may contain any number of Rows.
    struct Adapter : NodeDefinition
    {
        static constexpr AZStd::string_view Name = "Adapter";
        static bool CanAddToParentNode(const Dom::Value& parentNode);
        static bool CanBeParentToValue(const Dom::Value& value);
    };

    //! Row: An adapter entry that may contain any number of child nodes or other Rows
    struct Row : NodeDefinition
    {
        static constexpr AZStd::string_view Name = "Row";
        static bool CanAddToParentNode(const Dom::Value& parentNode);
        static bool CanBeParentToValue(const Dom::Value& value);
    };

    //! Label: A textual label that shall render its contents as part of a Row.
    struct Label : NodeDefinition
    {
        static constexpr AZStd::string_view Name = "Label";
    };

    //! PropertyEditor: A property editor, of a type dictated by its "type" field,
    //! that can edit an associated value.
    struct PropertyEditor : NodeDefinition
    {
        static constexpr AZStd::string_view Name = "PropertyEditor";
        static constexpr auto Type = AttributeDefinition<AZStd::string_view>("type");
    };
} // namespace AZ::DocumentPropertyEditor::Nodes
