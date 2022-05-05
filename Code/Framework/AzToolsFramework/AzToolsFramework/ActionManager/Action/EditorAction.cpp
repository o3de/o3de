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
        AZStd::string identifier,
        AZStd::string name,
        AZStd::string description,
        AZStd::string category,
        AZStd::function<void()> handler)
        : m_identifier(AZStd::move(identifier))
        , m_name(AZStd::move(name))
        , m_description(AZStd::move(description))
        , m_category(AZStd::move(category))
    {
        m_action = new QAction(m_name.c_str(), nullptr);

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
