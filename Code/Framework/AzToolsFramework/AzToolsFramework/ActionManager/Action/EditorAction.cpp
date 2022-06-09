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
        AZStd::string iconPath,
        AZStd::function<void()> handler,
        AZStd::function<bool()> checkStateCallback)
        : m_identifier(AZStd::move(identifier))
        , m_name(AZStd::move(name))
        , m_description(AZStd::move(description))
        , m_category(AZStd::move(category))
        , m_iconPath(AZStd::move(iconPath))
    {
        UpdateIconFromPath();
        m_action = new QAction(m_icon, m_name.c_str(), nullptr);

        QObject::connect(
            m_action, &QAction::triggered, parentWidget,
            [h = AZStd::move(handler)]()
            {
                h();
            }
        );

        if (checkStateCallback)
        {
            m_action->setCheckable(true);
            m_checkStateCallback = AZStd::move(checkStateCallback);

            // Trigger it to set the starting value correctly.
            m_action->setChecked(m_checkStateCallback());

            AZStd::function<bool()> callbackCopy = m_checkStateCallback;

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

    const AZStd::string& EditorAction::GetName() const
    {
        return m_name;
    }

    void EditorAction::SetName(AZStd::string name)
    {
        m_name = AZStd::move(name);
    }

    const AZStd::string& EditorAction::GetDescription() const
    {
        return m_description;
    }

    void EditorAction::SetDescription(AZStd::string description)
    {
        m_description = AZStd::move(description);
    }

    const AZStd::string& EditorAction::GetCategory() const
    {
        return m_category;
    }

    void EditorAction::SetCategory(AZStd::string category)
    {
        m_category = AZStd::move(category);
    }

    const AZStd::string& EditorAction::GetIconPath() const
    {
        return m_iconPath;
    }

    void EditorAction::SetIconPath(AZStd::string iconPath)
    {
        m_iconPath = AZStd::move(iconPath);
        UpdateIconFromPath();

        if (!m_icon.isNull())
        {
            m_action->setIcon(m_icon);
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
        if (m_checkStateCallback)
        {
            // Refresh checkable action value.
            m_action->setChecked(m_checkStateCallback());
        }
    }

    bool EditorAction::IsCheckable()
    {
        return m_action->isCheckable();
    }

    void EditorAction::UpdateIconFromPath()
    {
        m_icon = QIcon(m_iconPath.c_str());

        // If no icon was found at path, clear the path variable.
        if (m_icon.isNull())
        {
            m_iconPath.clear();
        }
    }

} // namespace AzToolsFramework
