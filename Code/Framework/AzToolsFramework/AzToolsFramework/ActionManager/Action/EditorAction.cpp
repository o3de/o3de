/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/Action/EditorAction.h>

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerNotificationBus.h>

#include <QAction>
#include <QIcon>

namespace AzToolsFramework
{
    EditorAction::EditorAction(
        QWidget* parentWidget,
        AZStd::string contextIdentifier,
        AZStd::string identifier,
        AZStd::string name,
        AZStd::string description,
        AZStd::string category,
        AZStd::string iconPath,
        bool hideFromMenusWhenDisabled,
        bool hideFromToolBarsWhenDisabled,
        AZStd::function<void()> handler,
        AZStd::function<bool()> checkStateCallback)
        : m_contextIdentifier(AZStd::move(contextIdentifier))
        , m_identifier(AZStd::move(identifier))
        , m_name(AZStd::move(name))
        , m_description(AZStd::move(description))
        , m_category(AZStd::move(category))
        , m_iconPath(AZStd::move(iconPath))
        , m_hideFromMenusWhenDisabled(hideFromMenusWhenDisabled)
        , m_hideFromToolBarsWhenDisabled(hideFromToolBarsWhenDisabled)
    {
        UpdateIconFromPath();
        m_action = new QAction(m_icon, m_name.c_str(), parentWidget);

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

    void EditorAction::Initialize()
    {
        s_actionManagerInterface = AZ::Interface<ActionManagerInterface>::Get();
        AZ_Assert(s_actionManagerInterface, "EditorAction - Could not retrieve instance of ActionManagerInterface");
    }

    const AZStd::string& EditorAction::GetActionContextIdentifier() const
    {
        return m_contextIdentifier;
    }

    const AZStd::string& EditorAction::GetName() const
    {
        return m_name;
    }

    void EditorAction::SetName(AZStd::string name)
    {
        m_name = AZStd::move(name);
        m_action->setText(m_name.c_str());
    }

    const AZStd::string& EditorAction::GetDescription() const
    {
        return m_description;
    }

    void EditorAction::SetDescription(AZStd::string description)
    {
        m_description = AZStd::move(description);
        UpdateTooltipText();
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

    AZStd::string EditorAction::GetHotKey() const
    {
        return m_action->shortcut().toString().toStdString().c_str();
    }

    void EditorAction::SetHotKey(const AZStd::string& hotKey)
    {
        m_action->setShortcut(QKeySequence(hotKey.c_str()));
        UpdateTooltipText();
    }

    bool EditorAction::GetHideFromMenusWhenDisabled() const
    {
        return m_hideFromMenusWhenDisabled;
    }

    bool EditorAction::GetHideFromToolBarsWhenDisabled() const
    {
        return m_hideFromToolBarsWhenDisabled;
    }

    QAction* EditorAction::GetAction()
    {
        return m_action;
    }

    const QAction* EditorAction::GetAction() const
    {
        return m_action;
    }
    
    void EditorAction::SetEnabledStateCallback(AZStd::function<bool()> enabledStateCallback)
    {
        if (enabledStateCallback)
        {
            m_enabledStateCallback = AZStd::move(enabledStateCallback);
            m_action->setEnabled(m_enabledStateCallback());
        }
    }

    void EditorAction::AssignToMode(AZStd::string modeIdentifier)
    {
        m_modes.emplace(AZStd::move(modeIdentifier));
        Update();
    }

    bool EditorAction::HasEnabledStateCallback() const
    {
        return m_enabledStateCallback != nullptr;
    }

    bool EditorAction::IsEnabled() const
    {
        return m_action->isEnabled();
    }

    void EditorAction::Update()
    {
        bool previousCheckedState = m_action->isChecked();
        bool previousEnabledState = m_action->isEnabled();

        if (m_checkStateCallback)
        {
            // Refresh checkable state.
            m_action->setChecked(m_checkStateCallback());
        }

        // Refresh enabled state.
        if (m_enabledStateCallback)
        {
            m_action->setEnabled(m_enabledStateCallback() && IsEnabledInCurrentMode());
        }
        else
        {
            m_action->setEnabled(IsEnabledInCurrentMode());
        }

        if (previousCheckedState != m_action->isChecked() || previousEnabledState != m_action->isEnabled())
        {
            ActionManagerNotificationBus::Broadcast(&ActionManagerNotificationBus::Handler::OnActionStateChanged, m_identifier);
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

    void EditorAction::UpdateTooltipText()
    {
        AZStd::string toolTipText = m_description;

        if (!m_action->shortcut().isEmpty())
        {
            toolTipText += AZStd::string::format(" (%s)", GetHotKey().c_str());
        }

        m_action->setToolTip(toolTipText.c_str());
    }

    bool EditorAction::IsEnabledInCurrentMode() const
    {
        auto outcome = s_actionManagerInterface->GetActiveActionContextMode(m_contextIdentifier);

        AZStd::string currentMode = [outcome]() -> AZStd::string
        {
            // If no mode can be retrieved, consider it to be the default.
            if (!outcome.IsSuccess())
            {
                return "default";
            }

            return AZStd::move(outcome.GetValue());
        }();

        // If no modes are defined, the actions is always active regardless of the mode.
        return (
            m_modes.empty() ||
            (AZStd::find(m_modes.begin(), m_modes.end(), currentMode) != m_modes.end())
        );
    }

} // namespace AzToolsFramework
