#include <AzToolsFramework/ViewportUi/Cluster.h>
#include <AzToolsFramework/ViewportUi/ViewportUiSwitcher.h>

namespace AzToolsFramework::ViewportUi::Internal
{
    ViewportUiSwitcher::ViewportUiSwitcher(AZStd::shared_ptr<Cluster> switcher, ButtonId currentMode)
        : m_switcher(switcher)
        , m_currentMode(currentMode)
    {
        setOrientation(Qt::Orientation::Horizontal);
        setStyleSheet(QString("QToolBar {background-color: none; border: none; spacing: 3px;}"
                              "QToolButton {background-color: #464646; border: outset; border-color: white; border-radius: 7px; "
                              "border-width: 2px; padding: 7px; color: white;}"));

        const AZStd::vector<Button*> buttons = switcher->GetButtons();

        for (auto button : buttons)
        {
            // add all the other buttons after
            if (button->m_buttonId)
            {
                AddButton(button);
            }
        }
    }

    ViewportUiSwitcher::~ViewportUiSwitcher()
    {
    }

    void ViewportUiSwitcher::AddButton(Button* button)
    {
        QAction* action = new QAction();
        action->setCheckable(true);
        action->setIcon(QIcon(QString(button->m_icon.c_str())));

        if (!action)
        {
            return;
        }

        // set hover to true by default
        action->setProperty("IconHasHoverEffect", true);

        // if its the first button added to the switcher add it as a button instead of action
        if (button->m_buttonId == m_currentMode)
        {
            QString buttonName = (button->m_name).c_str();
            QIcon buttonIcon = QString((button->m_icon).c_str());

            m_activeButton = new QToolButton();

            // No hover effect for the main button as it's not clickable
            m_activeButton->setProperty("IconHasHoverEffect", false);
            m_activeButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            m_activeButton->setIcon(buttonIcon);
            m_activeButton->setText(buttonName);
            addWidget(m_activeButton);
        }
        else
        {
            // add the action
            addAction(action);
        }

        // resize to fit new action with minimum extra space
        resize(minimumSizeHint());

        const AZStd::function<void()>&callback = [this, button]() { m_switcher->PressButton(button->m_buttonId); };
        const AZStd::function<void(QAction*)>& updateCallback = [button](QAction* action) {
            action->setChecked(button->m_state == Button::State::Selected);
        };

        // connect the callback if provided
        if (callback)
        {
            QObject::connect(action, &QAction::triggered, action, callback);
        }

        // register the action
        m_widgetCallbacks.AddWidget(action, [updateCallback](QPointer<QObject> object)
        {
            updateCallback(static_cast<QAction*>(object.data()));
        });

        m_buttonActionMap.insert({button->m_buttonId, action});
    }
   
    void ViewportUiSwitcher::RemoveButton(ButtonId buttonId)
    {
        if (auto actionEntry = m_buttonActionMap.find(buttonId); actionEntry != m_buttonActionMap.end())
        {
            QAction* action = actionEntry->second;

            // remove the action from the toolbar
            removeAction(action);

            // deregister from the widget manager
            m_widgetCallbacks.RemoveWidget(action);

            // resize to fit new area with minimum extra space
            resize(minimumSizeHint());

            m_buttonActionMap.erase(buttonId);

            // reset current active mode if its the button being removed
            if (buttonId == m_currentMode)
            {
                if (auto nextEntry = m_buttonActionMap.find(ButtonId(buttonId + 1)); nextEntry != m_buttonActionMap.end())
                {
                    SetActiveMode(nextEntry->first);
                }
            }
        }
    }
    
    void ViewportUiSwitcher::Update()
    {
        m_widgetCallbacks.Update();
    }

    void ViewportUiSwitcher::SetActiveMode(ButtonId buttonId)
    {
        // Change the toolbutton's name and icon to that button
        const AZStd::vector<Button*> buttons = m_switcher->GetButtons();
        for (auto button : buttons)
        {
            if (button->m_buttonId == buttonId)
            {
                QString buttonName = (button->m_name).c_str();
                QIcon buttonIcon = QString((button->m_icon).c_str());
                m_activeButton->setIcon(buttonIcon);
                m_activeButton->setText(buttonName);
            }
        }

        // look up button ID in map then remove it from its current position
        auto itr = m_buttonActionMap.find(buttonId);
        QAction* action = itr->second;
        removeAction(action); // call remove button?

        // add the last action removed
        if (m_currentMode != buttonId)
        {
            itr = m_buttonActionMap.find(m_currentMode);
            action = itr->second;

            addAction(action);
        }

        m_currentMode = buttonId;
    }
} // namespace AzToolsFramework::ViewportUi::Internal
