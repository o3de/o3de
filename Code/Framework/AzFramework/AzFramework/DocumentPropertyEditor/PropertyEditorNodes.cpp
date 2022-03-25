/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>

namespace AZ::DocumentPropertyEditor::Nodes
{
    bool Adapter::CanAddToParentNode([[maybe_unused]] const Dom::Value& parentNode)
    {
        // Adapters are root nodes, they can't be parented to anything
        return false;
    }

    bool Adapter::CanBeParentToValue(const Dom::Value& value)
    {
        // Adapters can only be parents to rows
        return value.IsNode() && value.GetNodeName() == GetNodeName<Row>();
    }

    bool Row::CanAddToParentNode(const Dom::Value& parentNode)
    {
        // Rows may only be children of other rows or the Adapter element
        const AZ::Name nodeName = parentNode.GetNodeName();
        return nodeName == GetNodeName<Row>() || nodeName == GetNodeName<Adapter>();
    }

    bool Row::CanBeParentToValue(const Dom::Value& value)
    {
        // Rows may only contain nodes, not arbitrary values
        return value.IsNode();
    }
} // namespace AZ::DocumentPropertyEditor::Nodes
