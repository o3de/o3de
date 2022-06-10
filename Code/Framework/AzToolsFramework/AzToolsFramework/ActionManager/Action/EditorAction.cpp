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
        AZStd::function<void()> handler,
        AZStd::function<bool()> updateCallback
        )
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

        if (updateCallback)
        {
            m_action->setCheckable(true);
            m_updateCallback = AZStd::move(updateCallback);

            // Trigger it to set the starting value correctly.
            m_action->setChecked(m_updateCallback());

            AZStd::function<bool()> callbackCopy = m_updateCallback;

            // Trigger the update after the handler is called.
            QObject::connect(
                m_action, &QAction::triggered, parentWidget,
                [u = AZStd::move(callbackCopy), a = m_action]()
                {
                    a->setChecked(u());
                }
            );
        }
    }

    QAction* EditorAction::GetAction()
    {
        // Update the action to ensure it is visualized correctly.
        Update();

        return m_action;
    }
    
    void EditorAction::Update()
    {
        if (m_updateCallback)
        {
            // Refresh checkable action value.
            m_action->setChecked(m_updateCallback());
        }
    }
    
    bool EditorAction::IsCheckable()
    {
        return m_action->isCheckable();
    }

} // namespace AzToolsFramework
