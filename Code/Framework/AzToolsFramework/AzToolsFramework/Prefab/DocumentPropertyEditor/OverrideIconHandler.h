/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/PropertyHandlerWidget.h>

#include <QIcon>
#include <QMenu>
#include <QObject>
#include <QToolButton>


namespace AzToolsFramework::Prefab
{
    class OverrideIconHandler
        : public PropertyHandlerWidget<QToolButton>
    {
    public:
        OverrideIconHandler();

        void SetValueFromDom(const AZ::Dom::Value&);

        static constexpr const AZStd::string_view GetHandlerName()
        {
            return AZ::DocumentPropertyEditor::Nodes::PrefabOverrideProperty::Name;
        }

    private:
        void ShowContextMenu(const QPoint&);
    };
} // namespace AzToolsFramework::Prefab
