#pragma once

#include <AzToolsFramework/ViewportUi/Button.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzToolsFramework/ViewportUi/ViewportUiWidgetCallbacks.h>
#include <functional>
#include <QPointer>
#include <QToolBar>
#include <QToolButton>


class Cluster;

namespace AzToolsFramework::ViewportUi::Internal
{
    //! Helper class to make switchers (toolbars) for display in Viewport UI.
    class ViewportUiSwitcher : public QToolBar
    {
        Q_OBJECT

    public:
        ViewportUiSwitcher(AZStd::shared_ptr<Cluster> switcher, ButtonId currentMode);
        ~ViewportUiSwitcher();
        //! Adds a new button to the cluster.
        void AddButton(Button* button);
        //! Removes a button from the cluster.
        void RemoveButton(ButtonId buttonId);
        void Update();
        void SetActiveMode(ButtonId buttonId);

    private:
        QToolButton* m_activeButton;
        ButtonId m_currentMode;
        AZStd::shared_ptr<Cluster> m_switcher; //!< Data structure which the cluster will be displaying to the Viewport UI.
        AZStd::unordered_map<ButtonId, QPointer<QAction>> m_buttonActionMap; //!< Map for buttons to their corresponding actions.
        ViewportUiWidgetCallbacks m_widgetCallbacks; //!< Registers actions and manages updates.

    };

} // namespace AzToolsFramework::ViewportUi::Internal
