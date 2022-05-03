/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/Action/EditorAction.h>

namespace AzToolsFramework
{
    EditorAction::EditorAction(
        QWidget* parentWidget,
        AZStd::string_view identifier,
        AZStd::string_view name,
        AZStd::string_view description,
        AZStd::string_view category,
        AZStd::function<void()> handler)
        : m_identifier(identifier)
        , m_name(name)
        , m_description(description)
        , m_category(category)
    {
        m_action = new QAction(name.data(), nullptr);

        QObject::connect(
            m_action, &QAction::triggered, parentWidget,
            [h = AZStd::move(handler)]()
            {
                h();
            }
        );
    }

    QAction* EditorAction::GetAction()
    {
        return m_action;
    }

} // namespace AzToolsFramework
