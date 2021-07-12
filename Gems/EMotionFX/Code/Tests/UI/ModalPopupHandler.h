/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <Tests/SystemComponentFixture.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <QDialogButtonBox>
#include <QtTest>
#include <QPushButton>
#include <QMessageBox>
#endif

QT_FORWARD_DECLARE_CLASS(QWidget)
QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QTreeView)
QT_FORWARD_DECLARE_CLASS(ReselectingTreeView)

namespace EMotionFX
{
    using MenuActiveCallback = AZStd::function<void(QMenu* menu)>;
    using WidgetActiveCallback = AZStd::function<void(QWidget* widget)>;
    using ActionCompletionCallback = AZStd::function<void(const QString& action)>;

    /// This class can be used to manipulate modal popups which cannot otherwise be interracted with, due to the thread being on hold until they are complete.
    /// To use it: set up an instance before the popup is triggered, with a callback that handles any interraction you require.
    /// Can also be used with modeless popups, by calling WaitForCompletion after the popup is triggered.
    class ModalPopupHandler
        : public QObject
    {
        Q_OBJECT
    public:
        explicit ModalPopupHandler(QObject* pParent = nullptr);

        /// Brings up a context menu on a widget, then triggers the named action.
        void ShowContextMenuAndTriggerAction(QWidget* widget, const QString& actionName, int timeout, ActionCompletionCallback callback);

        /// Wait for a modal widget (returned by activeModalWidget) which is of WidgetType to appear, then call callback.
        /// @param callback routine to call when a n active modal widget is detected.
        /// @param timeout maximum time to wait before giving up.
        template <class WidgetType>
        void WaitForPopup(AZStd::function<void(WidgetType menu)> callback, const int timeout = DefaultTimeout)
        {
            m_complete = false;
            ResetSeenTargetWidget();

            WidgetActiveCallback widgetCallback = [this, callback](QWidget* widget)
            {
                if (!widget)
                {
                    QTimer::singleShot(WaitTickTime, this, &ModalPopupHandler::CheckForPopupWidget);
                    return;
                }

                WidgetType popupWidget = nullptr;
                // If the style manager is active, the hierarchy will be different.
                if (AzQtComponents::StyleManager::isInstanced())
                {
                    AzQtComponents::WindowDecorationWrapper* wrapper = qobject_cast<AzQtComponents::WindowDecorationWrapper*>(widget);
                    if (wrapper)
                    {
                        popupWidget = wrapper->findChild<WidgetType>();
                    }
                }
                else
                {
                    popupWidget = qobject_cast<WidgetType>(widget);
                }

                if (!popupWidget)
                {
                    QTimer::singleShot(WaitTickTime, this, &ModalPopupHandler::CheckForPopupWidget);
                    return;
                }

                callback(popupWidget);
                m_complete = true;
            };

            m_totalTime = 0;
            m_widgetActiveCallback = widgetCallback;
            m_timeout = timeout;

            // Kick a timer off to check whether the menu is open.
            QTimer::singleShot(WaitTickTime, this, &ModalPopupHandler::CheckForPopupWidget);
        }

        /// Wait for a modal widget (returned by activeModalWidget) which is of WidgetType to appear, then press a button in a child QDialogButtonBox.
        /// @param buttonRole button to press.
        /// @param timeout maximum time to wait before giving up.
        template <class WidgetType>
        void WaitForPopupPressDialogButton(QDialogButtonBox::StandardButton buttonRole, const int timeout = DefaultTimeout)
        {
            WidgetActiveCallback pressButtonCallback = [buttonRole](QWidget* widget)
            {
                ASSERT_TRUE(widget);

                QDialogButtonBox* buttonBox = widget->findChild< QDialogButtonBox*>();
                ASSERT_TRUE(buttonBox) << "Unable to find button box in SaveDirtySettingsWindow";

                QPushButton* button = buttonBox->button(buttonRole);
                ASSERT_TRUE(button) << "Unable to find button in SaveDirtySettingsWindow";

                QTest::mouseClick(button, Qt::LeftButton);
            };

            WaitForPopup<WidgetType>(pressButtonCallback, timeout);
        }

        /// Wait for a modal widget (returned by activeModalWidget) which is of WidgetType to appear, then press a specific button in a child QDialogButtonBox.
        /// @param buttonRole button to press.
        /// @param timeout maximum time to wait before giving up.
        template <class WidgetType>
        void WaitForPopupPressSpecificButton(const AZStd::string buttonObjectName, const int timeout = DefaultTimeout)
        {
            WidgetActiveCallback pressButtonCallback = [buttonObjectName](QWidget* widget)
            {
                ASSERT_TRUE(widget);

                QDialogButtonBox* buttonBox = widget->findChild< QDialogButtonBox*>();
                ASSERT_TRUE(buttonBox) << "Unable to find button box in SaveDirtySettingsWindow";

                QAbstractButton* selectedButton = nullptr;
                for (QAbstractButton* button : buttonBox->buttons())
                {
                    if (button->objectName().toStdString() == buttonObjectName.c_str())
                    {
                        selectedButton = button;
                        break;
                    }
                }

                ASSERT_TRUE(selectedButton) << "Unable to find button in SaveDirtySettingsWindow";

                QTest::mouseClick(selectedButton, Qt::LeftButton);
            };

            WaitForPopup<WidgetType>(pressButtonCallback, timeout);
        }

        /// @return true if the expected dialog was seen, false otherwise.
        bool GetSeenTargetWidget();

        /// Reset the seen target flag, to be used if you want to use the same handler twice.
        void ResetSeenTargetWidget();

        /// Returns whether or not the dialog has been completed (is closed). Only of use with modeless widgets.
        bool GetIsComplete();

        /// Wait until the dialog is complete, then return. Has no effect if the dialog is modal.
        void WaitForCompletion(const int timeout = DefaultTimeout);

    private Q_SLOTS:
        void CheckForContextMenu();
        void CheckForPopupWidget();

    private:
        MenuActiveCallback m_MenuActiveCallback = nullptr;
        WidgetActiveCallback m_widgetActiveCallback = nullptr;
        ActionCompletionCallback m_actionCompletionCallback = nullptr;

        int m_totalTime = 0;
        int m_timeout = 0;

        bool m_seenTargetWidget = false;
        bool m_complete = false;

        static constexpr int WaitTickTime = 10;
        static constexpr int DefaultTimeout = 3000;
    };
} // end namespace EMotionFX
