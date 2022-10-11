/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/Action/EditorWidgetAction.h>

namespace AzToolsFramework
{
    EditorWidgetAction::EditorWidgetAction(
            AZStd::string identifier,
            AZStd::string name,
            AZStd::string category,
            AZStd::function<QWidget*()> generator
        )
        : m_identifier(AZStd::move(identifier))
        , m_name(AZStd::move(name))
        , m_category(AZStd::move(category))
        , m_generator(AZStd::move(generator))
    {
    }

    const AZStd::string& EditorWidgetAction::GetName() const
    {
        return m_name;
    }

    void EditorWidgetAction::SetName(AZStd::string name)
    {
        m_name = AZStd::move(name);
    }

    const AZStd::string& EditorWidgetAction::GetCategory() const
    {
        return m_category;
    }

    void EditorWidgetAction::SetCategory(AZStd::string category)
    {
        m_category = AZStd::move(category);
    }

    QWidget* EditorWidgetAction::GenerateWidget() const
    {
        return m_generator();
    }

} // namespace AzToolsFramework
