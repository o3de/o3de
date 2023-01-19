/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QFrame>
#include <QPointer>
#include <QChildEvent>
#endif

class QSettings;
class QStyleOption;

namespace AzQtComponents
{
    class TitleBar;

    class AZ_QT_COMPONENTS_API WindowDecorationWrapper
        : public QFrame
    {
        Q_OBJECT

    public:

        enum Option
        {
            OptionNone = 0,
            OptionAutoAttach = 1, // Auto attaches to a widget. To use this feature pass the wrapper as the widget's parent.
                                  // The wrapper will wait for a ChildEvent and add the decorations at that point.
            OptionAutoTitleBarButtons = 2, // Removes minimize and maximize button depending on the guest widget's flags and size constraints
            OptionDisabled = 4 // don't activate the custom title bar at all
        };
        Q_DECLARE_FLAGS(Options, Option)

        explicit WindowDecorationWrapper(Options options = Option::OptionAutoTitleBarButtons, QWidget* parent = nullptr);

        /**
         * Destroys the wrapper and the guest if it's still alive.
         */
        ~WindowDecorationWrapper();

        /**
         * Sets the widget which should get custom window decorations.
         * You can only call this setter once.
         */
        void setGuest(QWidget*);
        QWidget* guest() const;

        /**
         * Returns true if there's a guest widget, which means either one was set with setGuest()
         * or we're using OptionAutoAttach and we got a ChildEvent.
         */
        bool isAttached() const;

        /**
         * Returns the title bar widget.
         */
        TitleBar* titleBar() const;

        /**
         * Enables restoring/saving geometry on creation/destruction.
         * If autoRestoreOnShow is set to true, the first time the window is shown, it will automatically restore the geometry.
         */
        void enableSaveRestoreGeometry(const QString& organization,
            const QString& app,
            const QString& key,
            bool autoRestoreOnShow = false);

        /**
         * Enables restoring/saving geometry on creation/destruction, using the default settings values for organization and application.
         * If autoRestoreOnShow is set to true, the first time the window is shown, it will automatically restore the geometry.
         */
        void enableSaveRestoreGeometry(const QString& key, bool autoRestoreOnShow = false);

        /**
         * Returns true if restoring/saving geometry on creation/destruction is enabled.
         * It's disabled by default.
         */
        bool saveRestoreGeometryEnabled() const;

        /**
         * Restores geometry and shows window.
         * Only shows window if there were saved settings, otherwise returns false.
         */
        bool restoreGeometryFromSettings();

        /**
         * Restores geometry if there are saved settings, and shows window (always).
         */
        void showFromSettings();

        QMargins margins() const;

    protected:
        bool event(QEvent* ev) override;
        bool eventFilter(QObject* watched, QEvent* event) override;
        void resizeEvent(QResizeEvent* ev) override;
        void childEvent(QChildEvent* ev) override;
        void closeEvent(QCloseEvent* ev) override;
        void showEvent(QShowEvent* ev) override;
        void hideEvent(QHideEvent* ev) override;
        bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
        void changeEvent(QEvent* ev) override;

    private:
        friend class StyledDockWidget;
        static bool handleNativeEvent(const QByteArray& eventType, void* message, long* result, const QWidget* widget);
        static QMargins win10TitlebarHeight(QWindow *w);
        static Qt::WindowFlags specialFlagsForOS();
        static void drawFrame(const QStyleOption *option, QPainter *painter, const QWidget *widget);
        QWidget* topLevelParent();
        void centerInParent();
        bool autoAttachEnabled() const;
        bool autoTitleBarButtonsEnabled() const;
        void updateTitleBarButtons();
        QSize nonContentsSize() const;
        void saveGeometryToSettings();
        void applyFlagsAndAttributes();
        bool canResize() const;
        bool canResizeHeight() const;
        bool canResizeWidth() const;
        void onWindowTitleChanged(const QString& title);
        void adjustTitleBarGeometry();
        void adjustWrapperGeometry();
        void adjustSizeGripGeometry();
        void adjustWidgetGeometry();
        void updateConstraints();
        void enableSaveRestoreGeometry(QSettings* settings, const QString& key, bool autoRestoreOnShow);

        bool m_initialized = false;
        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'AzQtComponents::WindowDecorationWrapper::m_options': class 'QFlags<AzQtComponents::WindowDecorationWrapper::Option>' needs to have dll-interface to be used by clients of class 'AzQtComponents::WindowDecorationWrapper'
        Options m_options = OptionNone;
        AZ_POP_DISABLE_WARNING
        TitleBar* const m_titleBar;
        QWidget* m_guestWidget = nullptr;
        QSettings* m_settings = nullptr;
        QString m_settingsKey;
        bool m_shouldCenterInParent = false;
        bool m_restoringGeometry = false;
        bool m_autoRestoreOnShow = false;
        bool m_blockForRestoreOnShow = false;
    };
} // namespace AzQtComponents

Q_DECLARE_OPERATORS_FOR_FLAGS(AzQtComponents::WindowDecorationWrapper::Options)
