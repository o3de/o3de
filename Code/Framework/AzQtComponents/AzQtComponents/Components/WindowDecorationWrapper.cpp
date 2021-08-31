/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <qglobal.h> // For Q_OS_WIN

#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <AzQtComponents/Components/DockBarButton.h>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/Titlebar.h>
#include <AzQtComponents/Components/TitleBarOverdrawHandler.h>
#include <AzQtComponents/Utilities/QtWindowUtilities.h>

#include <QTimer>
#include <QPainter>
#include <QDebug>
#include <QApplication>
#include <QSettings>
#include <QWindow>
#include <QScreen>
#include <QCloseEvent>
#include <QDesktopWidget>
#include <QDialog>
#include <QStyleOption>
#include <QScopedValueRollback>
#include <QStyle>
#include <QLayout>
#include <QtGui/private/qhighdpiscaling_p.h>

#ifdef Q_OS_WIN
#endif

namespace AzQtComponents
{
    enum
    {
        FrameWidth = 1, // 1px black line
    };

    namespace
    {
        // Restores the window state from QWidget::saveGeometry
        // Includes a workaround for restoring the maximization state correctly on Windows
        bool RestoreWindowState(QWidget* window, QByteArray geometry)
        {
            if (geometry.size() < 4 || !window->restoreGeometry(geometry))
            {
                return false;
            }

            // Read the geometry based on the Qt format, bailing out if it's invalid
            QDataStream stream(geometry);
            stream.setVersion(QDataStream::Qt_4_0);

            const quint32 magicNumber = 0x1D9D0CB;
            quint32 storedMagicNumber;
            stream >> storedMagicNumber;
            if (storedMagicNumber != magicNumber)
            {
                return false;
            }

            quint16 majorVersion = 0;
            quint16 minorVersion = 0;

            stream >> majorVersion >> minorVersion;

            QRect restoredFrameGeometry;
            QRect restoredNormalGeometry;
            qint32 restoredScreenNumber;
            quint8 maximized;
            quint8 fullScreen;
            qint32 restoredScreenWidth;

            stream >> restoredFrameGeometry
                >> restoredNormalGeometry
                >> restoredScreenNumber
                >> maximized
                >> fullScreen
                >> restoredScreenWidth;
            if (!window->isVisible())
            {
                if (maximized || fullScreen)
                {
                    window->showMaximized();
                }
                else
                {
                    window->show();
                }
            }
            if (maximized || fullScreen)
            {
                // Need to separately resize based on the available geometry for
                // the screen because since floating windows are frameless, on
                // Windows 10 they end up taking up the entire screen when maximized
                // instead of respecting the available space (e.g. taskbar)
                window->setGeometry(QApplication::desktop()->availableGeometry(window));
            }
            

            return true;
        }
    }

    WindowDecorationWrapper::WindowDecorationWrapper(Options options, QWidget* parent)
        : QFrame(parent, options & OptionDisabled ? Qt::Window : WindowDecorationWrapper::specialFlagsForOS() | Qt::Window)
        , m_options(options)
        , m_titleBar(options & OptionDisabled ? nullptr : new TitleBar(this))
    {
        if (m_titleBar)
        {
            m_titleBar->setDragEnabled(true);
            m_titleBar->setButtons({ DockBarButton::DividerButton, DockBarButton::MinimizeButton,
                                     DockBarButton::DividerButton, DockBarButton::MaximizeButton,
                                     DockBarButton::DividerButton, DockBarButton::CloseButton});
        }
        adjustTitleBarGeometry();
        m_initialized = true;

        // Create a QWindow -- windowHandle()
        setAttribute(Qt::WA_NativeWindow, true);
        TitleBarOverdrawHandler::getInstance()->addTitleBarOverdrawWidget(this);
    }

    WindowDecorationWrapper::~WindowDecorationWrapper()
    {
        if (saveRestoreGeometryEnabled())
        {
            saveGeometryToSettings();
        }
    }

    static bool shouldCenterInParent(const QWidget* w)
    {
        return (w->windowFlags() & Qt::Dialog) == Qt::Dialog;
    }

    void WindowDecorationWrapper::setGuest(QWidget* guest)
    {
        if (m_guestWidget || m_guestWidget == guest || !guest)
        {
            qDebug() << "WindowDecorationWrapper::setGuest bailing out" << m_guestWidget << guest << this;
            return;
        }

        m_guestWidget = guest;

        if (guest->isWindow())
        {
            if (auto layout = guest->layout())
            {
                // On windows, by default the layout will set the widget's minimum size to be the layout's
                // minimum size. Since guest won't be a window any more, change the resize mode to SetMinimumSize
                // so that minimum size of the contents is honored.
                layout->setSizeConstraint(QLayout::SetMinimumSize);
            }
        }

        m_shouldCenterInParent = shouldCenterInParent(guest); // Don't move this variable after applyFlagsAndAttributes()
        guest->setParent(this);
        connect(guest, &QWidget::windowTitleChanged,
            this, &WindowDecorationWrapper::onWindowTitleChanged);

        // The wrapper is deleted when widget is destroyed
        // Connect even if OptionDisable is used otherwise the WindowDecorationWrapper is still
        // visible after the guest widget is closed.
        connect(guest, &QWidget::destroyed, this, [this]
        {
            m_guestWidget = nullptr;

            // the Open 3D Engine Editor has code that checks for Modal widgets, and blocks on doing other things
            // if there are still active Modal dialogs.
            // So we need to ensure that this WindowDecorationWrapper doesn't report itself as being modal
            // after the guest widget has been deleted.
            if (isModal())
            {
                setWindowModality(Qt::NonModal);
            }

            deleteLater();
        });

        if (m_options & OptionDisabled)
        {
            return;
        }

        applyFlagsAndAttributes();

        guest->installEventFilter(this);

        m_titleBar->setWindowTitleOverride(guest->windowTitle());

        updateConstraints();
    }

    QWidget* WindowDecorationWrapper::topLevelParent()
    {
        // Returns the parent window
        QWidget* pw = parentWidget();
        if (!pw)
        {
            return nullptr;
        }

        if (pw->window() == window())
        {
            // Just a sanity check that probably doesn't happen.
            // We're only interested in the case where there's an actual parent window
            return nullptr;
        }

        return pw->window();
    }

    static QPoint screenCenter(QWidget* w)
    {
        return w->screen()->availableGeometry().center();
    }

    void WindowDecorationWrapper::centerInParent()
    {
        if (QWidget* topLevelWidget = topLevelParent())
        {
            QPoint parentWindowCenter = topLevelWidget->mapToGlobal(topLevelWidget->rect().center());
            if (topLevelWidget->isMaximized())
            {
                // During startup if the parent window is maximized it can happen that it still
                // doesn't have the correct geometry, because it's being restored. This only affects
                // maximized windows and is easy to fix:
                parentWindowCenter = screenCenter(topLevelWidget);
            }

            // Center within the parent
            QRect geo = geometry();

            // If the base size of the guest widget is larger than the screen,
            // then we need to resize it so that it will fit by either using
            // the minimum size (if one is set), or fallback to the screen size.
            if (m_guestWidget)
            {
                if (auto screen = m_guestWidget->screen())
                {
                    const QRect screenGeometry = screen->availableGeometry();
                    if (geo.width() > screenGeometry.width())
                    {
                        auto guestMinimumWidth = m_guestWidget->minimumWidth();
                        if (guestMinimumWidth && guestMinimumWidth <= screenGeometry.width())
                        {
                            geo.setWidth(guestMinimumWidth);
                        }
                        else
                        {
                            geo.setWidth(screenGeometry.width());
                        }
                    }
                    if (geo.height() > screenGeometry.height())
                    {
                        auto guestMinimumHeight = m_guestWidget->minimumHeight();
                        if (guestMinimumHeight && guestMinimumHeight <= screenGeometry.height())
                        {
                            geo.setHeight(guestMinimumHeight);
                        }
                        else
                        {
                            geo.setHeight(screenGeometry.height());
                        }
                    }
                }
            }

            geo.moveCenter(parentWindowCenter);

            QWindow* w = topLevelWidget->windowHandle();
            if (!w)
            {
                return;
            }

            QScreen* screen = w->screen();
            if (!screen)
            {
                // defensive, shouldn't happen
                return;
            }

            const QRect screenGeometry = screen->availableGeometry();
            const bool crossesScreenBoundaries = geo.intersected(screenGeometry) != geo;
            if (crossesScreenBoundaries)
            {
                // We don't want the window half-hidden, or even worse like having it's title-bar hidden.
                // Just center in the middle of the screen
                geo.moveCenter(screenCenter(topLevelWidget));
            }

            setGeometry(geo);
        }
    }

    QWidget* WindowDecorationWrapper::guest() const
    {
        return m_guestWidget;
    }

    bool WindowDecorationWrapper::isAttached() const
    {
        return m_guestWidget;
    }

    TitleBar* WindowDecorationWrapper::titleBar() const
    {
        return m_titleBar;
    }

    void WindowDecorationWrapper::enableSaveRestoreGeometry(const QString& organization, const QString& app, const QString& key, bool autoRestoreOnShow)
    {
        enableSaveRestoreGeometry(new QSettings(organization, app, this), key, autoRestoreOnShow);
    }

    void WindowDecorationWrapper::enableSaveRestoreGeometry(const QString& key, bool autoRestoreOnShow)
    {
        enableSaveRestoreGeometry(new QSettings(this), key, autoRestoreOnShow);
    }

    void WindowDecorationWrapper::enableSaveRestoreGeometry(QSettings* settings, const QString& key, bool autoRestoreOnShow)
    {
        if (m_settings)
        {
            qWarning() << Q_FUNC_INFO << "Save/restore already enabled";
            return;
        }

        if (key.isEmpty())
        {
            qWarning() << Q_FUNC_INFO << "Invalid parameters";
            return;
        }

        m_settings = settings;
        m_settingsKey = key;
        m_autoRestoreOnShow = autoRestoreOnShow;
        m_blockForRestoreOnShow = autoRestoreOnShow;

        connect(qApp, &QCoreApplication::aboutToQuit, this, &WindowDecorationWrapper::saveGeometryToSettings);
        connect(qApp, &QGuiApplication::applicationStateChanged, this, &WindowDecorationWrapper::saveGeometryToSettings);
    }

    bool WindowDecorationWrapper::saveRestoreGeometryEnabled() const
    {
        return m_settings != nullptr;
    }

    bool WindowDecorationWrapper::eventFilter(QObject* watched, QEvent* ev)
    {
        if (watched != m_guestWidget)
        {
            return QFrame::eventFilter(watched, ev);
        }

        if (ev->type() == QEvent::HideToParent)
        {
            hide();
        }
        else if (ev->type() == QEvent::Resize && !m_restoringGeometry)
        {
            // if WA_Resized is set it means we already explicitly resized the wrapper, so that should
            // take precedence

            updateConstraints(); // Better way to detect size constraints changed ?
            adjustWrapperGeometry();
            adjustTitleBarGeometry();
        }
        else if (ev->type() == QEvent::ShowToParent || ev->type() == QEvent::Show)
        {
            applyFlagsAndAttributes();
            if (m_shouldCenterInParent)
            {
                centerInParent();
            }

            if (!m_restoringGeometry)
            {
                const QSize guestMinSize = m_guestWidget->minimumSize();

                show();

                if (!guestMinSize.isNull())
                {
                    // If the widget is not a window then it's layout will remove the size constraints
                    // So save and restore constraints after showing
                    m_guestWidget->setMinimumSize(guestMinSize);
                }
            }

            // Titlebar may have a native window that could have received hide() (due to e.g. Alt+F4).
            // Because of that it won't be shown automatically when its parent, wrapper, is shown.
            // Make sure, that it's indeed shown.
            if (m_titleBar)
            {
                m_titleBar->show();
            }
        }
        else if (ev->type() == QEvent::LayoutRequest)
        {
            updateConstraints();
        }

        return QFrame::eventFilter(watched, ev);
    }

    void WindowDecorationWrapper::resizeEvent(QResizeEvent* ev)
    {
        Q_UNUSED(ev);
        // qDebug() << "WindowDecorationWrapper::resizeEvent old=" << ev->oldSize() << ";new=" << ev->size() << this;
        adjustTitleBarGeometry();

        if (m_guestWidget && !m_guestWidget->testAttribute(Qt::WA_PendingResizeEvent))
        {
            adjustWidgetGeometry();
        }
    }

    void WindowDecorationWrapper::childEvent(QChildEvent* ev)
    {
        if (!m_initialized || ev->type() != QEvent::ChildAdded || !autoAttachEnabled() || isAttached())
        {
            return;
        }

        if (!ev->child()->isWidgetType())
        {
            return;
        }

        QWidget* w = static_cast<QWidget*>(ev->child());
        if (w == m_titleBar)
        {
            return;
        }

#if AZ_TRAIT_OS_PLATFORM_APPLE
        // On macOS, tool windows correspond to the Floating class of windows. This means that the
        // window lives on a level above normal windows making it impossible to put a normal window
        // on top of it. Therefore we need to add Qt::Tool to QDialogs to ensure they are not hidden
        // under a Floating window.
        // qobject_cast in QObject::childEvent is not ideal because the child object may not have
        // been constructed yet. To be on the safe side, check the windowFlags too.
        if ((qobject_cast<QDialog*>(w) != nullptr) || (w->windowFlags() & Qt::Dialog))
        {
            setWindowFlags(windowFlags() | Qt::Tool);
        }
#endif
        setGuest(w);
    }

    void WindowDecorationWrapper::closeEvent(QCloseEvent* ev)
    {
        saveGeometryToSettings();
        if (m_guestWidget)
        {
            QApplication::sendEvent(m_guestWidget, ev);
        }
    }

    void WindowDecorationWrapper::hideEvent(QHideEvent* ev)
    {
        saveGeometryToSettings();
        QFrame::hideEvent(ev);
    }

    static void centerOnScreen(WindowDecorationWrapper* window)
    {
        const QDesktopWidget* desktop = QApplication::desktop();
        QRect availableGeometry = desktop->availableGeometry(window);
        QRect alignedRect = QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, window->size(), availableGeometry);

        window->setGeometry(alignedRect);
    }

    void WindowDecorationWrapper::showEvent(QShowEvent* ev)
    {
        if (m_autoRestoreOnShow && (m_settings != nullptr))
        {
            // use a timer to trigger this as soon as possible, but not now.
            // We're in the middle of the show right now and restoreGeometryFromSettings
            // can call show. No recursion shenanigans.
            QTimer::singleShot(0, this, [this] {
                if (!restoreGeometryFromSettings())
                {
                    // default to centering it on screen if the restore failed
                    centerOnScreen(this);
                }

                m_blockForRestoreOnShow = false;
            });

            // reset this so that we don't do this again
            m_autoRestoreOnShow = false;
        }

        QFrame::showEvent(ev);
    }

    bool WindowDecorationWrapper::nativeEvent(const QByteArray& eventType, void* message, long* result)
    {
        return handleNativeEvent(eventType, message, result, this);
    }

    void WindowDecorationWrapper::changeEvent(QEvent* ev)
    {
        if (ev->type() == QEvent::WindowStateChange)
        {
            // only way to know when the window has minimized/maximized or full screen has changed
            saveGeometryToSettings();
        }

        QFrame::changeEvent(ev);
    }

    /* static */
    bool WindowDecorationWrapper::handleNativeEvent(const QByteArray& eventType, void* message, long* result, const QWidget* widget)
    {
#ifdef Q_OS_WIN
        MSG* msg = static_cast<MSG*>(message);
        if (strcmp(eventType.constData(), "windows_generic_MSG") != 0)
        {
            return false;
        }

        if (isWin10())
        {
            if (widget->window() && msg->message == WM_NCHITTEST && GetAsyncKeyState(VK_RBUTTON) >= 0) // We're not interested in right click
            {

                /**
                 * This code block enables Windows native dragging, which enables the "Aero Snap" feature,
                 * where we can snap our windows to the sides of the screen.
                 */
                HWND handle = (HWND)widget->window()->winId();
                const LRESULT defWinProcResult = DefWindowProc(handle, msg->message, msg->wParam, msg->lParam);
                if (defWinProcResult == 1)
                {
                    if (auto wrapper = qobject_cast<const WindowDecorationWrapper *>(widget))
                    {
                        /**
                         * We only care about the title bars belonging to WindowDecorationWrapper.
                         * The ones from StyledDockWidget::titleBar() must use our custom dragging, so the docking system works,
                         * we can't use the native dragging and we can't have "Aero Snap" for dock widgets.
                         */
                        TitleBar* titleBar = wrapper->titleBar();
                        const short global_x = static_cast<short>(LOWORD(msg->lParam));
                        const short global_y = static_cast<short>(HIWORD(msg->lParam));

                        const QPoint globalPos = QHighDpi::fromNativePixels(QPoint(global_x, global_y), widget->window()->windowHandle());
                        const QPoint local = titleBar->mapFromGlobal(globalPos);
                        if (titleBar->draggableRect().contains(local))
                        {
                            if (titleBar->isTopResizeArea(globalPos))
                            {
                                *result = HTTOP;
                            }
                            else
                            {
                                *result = HTCAPTION;
                            }

                            return true;
                        }
                    }
                }
            }
        }
        else
        {
            // No other event to process for win10()
            // for Win10 we have the native title-bar, so maximized geometry is calculated correctly out of the box
            return false;
        }

        if (msg->message == WM_GETMINMAXINFO && widget->isMaximized())
        {
            // When Windows maximizes a window without native titlebar it will cover the taskbar.
            // Qt is patched to catch this case, but only for Qt::FramelessWindowHint cases, which
            // we're not using, so calculate the size ourselves.

            QWindow* w = widget->windowHandle();
            if (!w || !w->screen() || w->screen() != QApplication::primaryScreen())
            {
                // WM_GETMINMAXINFO only works for the primary screen
                return false;
            }

            // Get the sizes Windows would have chosen
            DefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam);

            const QScreen *screen = w->screen();
            const QRect availableGeometry = QHighDpi::toNativePixels(screen->availableGeometry(), screen);
            auto mmi = reinterpret_cast<MINMAXINFO*>(msg->lParam);
            mmi->ptMaxSize.y = availableGeometry.height();
            mmi->ptMaxSize.x = availableGeometry.width();
            mmi->ptMaxPosition.y = availableGeometry.y();
            mmi->ptMaxPosition.x = availableGeometry.x();

            *result = 0;
            return true;
        }
#else
        Q_UNUSED(eventType)
        Q_UNUSED(message)
        Q_UNUSED(result)
        Q_UNUSED(widget)
#endif

        return false;
    }

    QMargins WindowDecorationWrapper::margins() const
    {
        if (m_titleBar)
            return QMargins(FrameWidth, m_titleBar->height(), FrameWidth, FrameWidth);
        else
            return QMargins(0, 0, 0, 0);
    }

    bool WindowDecorationWrapper::autoAttachEnabled() const
    {
        return m_options & OptionAutoAttach;
    }

    bool WindowDecorationWrapper::autoTitleBarButtonsEnabled() const
    {
        return m_options & OptionAutoTitleBarButtons;
    }

    QSize WindowDecorationWrapper::nonContentsSize() const
    {
        // The size of the wrapper is always m_guestWidget->size() + nonContentsSize()
        // This returns the non-widget size, left and right frame, bottom frame, and titlebar height
        const QMargins m = margins();
        return QSize(m.left() + m.right(), m.top() + m.bottom());
    }

    void WindowDecorationWrapper::saveGeometryToSettings()
    {
        if (!m_settings)
        {
            return;
        }
        if (m_blockForRestoreOnShow)
        {
            // if m_blockForRestoreOnShow is set, it means that we haven't loaded
            // settings from show yet. If we save the geometry settings now, it will
            // overwrite the settings that were previously saved and haven't been restored yet.
            return;
        }

        QByteArray geo = saveGeometry();

        m_settings->setValue(m_settingsKey, geo);
    }

    bool WindowDecorationWrapper::restoreGeometryFromSettings()
    {
        if (!m_settings || m_restoringGeometry)
        {
            return false;
        }

        const QByteArray savedGeometry = m_settings->value(m_settingsKey).toByteArray();
        QScopedValueRollback<bool> rollback(m_restoringGeometry);
        m_restoringGeometry = true;

        if (!RestoreWindowState(this, savedGeometry))
        {
            return false;
        }

        // We don't currently support fullscreen mode, so remove that window state when we are restoring
        // in case it was set inadvertently
        setWindowState(windowState() & ~Qt::WindowFullScreen);

        adjustWidgetGeometry();
        m_restoringGeometry = false;

        return true;
    }

    void WindowDecorationWrapper::showFromSettings()
    {
        if (!restoreGeometryFromSettings())
        {
            show();

            // If we failed to restore from settings (the first time this window is loaded),
            // then center it on the screen by default
            centerOnScreen(this);
        }
    }

    void WindowDecorationWrapper::applyFlagsAndAttributes()
    {
        if (m_titleBar == nullptr)
            return;

        // Remove Qt::Window, if it's present.
        m_guestWidget->setWindowFlags(m_guestWidget->windowFlags() & ~Qt::Window);

        // Copy other relevant flags
        const QList<Qt::WindowFlags> flags = { Qt::WindowStaysOnTopHint };
        for (auto flag : flags)
        {
            if (m_guestWidget->windowFlags() & flag)
            {
                setWindowFlags(windowFlags() | flag);
            }
        }

        // Copy relevant attributes from widget
        const QList<Qt::WidgetAttribute> attrs = {
            Qt::WA_DeleteOnClose,
            Qt::WA_QuitOnClose,
            Qt::WA_ShowModal,
            Qt::WA_Hover
        };
        for (auto attr : attrs)
        {
            setAttribute(attr, m_guestWidget->testAttribute(attr));
        }

        updateTitleBarButtons();
    }

    bool WindowDecorationWrapper::canResize() const
    {
        return (canResizeHeight() || canResizeWidth()) && !isMaximized();
    }

    bool WindowDecorationWrapper::canResizeHeight() const
    {
        return m_guestWidget && m_guestWidget->minimumHeight() < m_guestWidget->maximumHeight();
    }

    bool WindowDecorationWrapper::canResizeWidth() const
    {
        return m_guestWidget && m_guestWidget->minimumWidth() < m_guestWidget->maximumWidth();
    }

    void WindowDecorationWrapper::onWindowTitleChanged(const QString& title)
    {
        if (m_titleBar)
            m_titleBar->setWindowTitleOverride(title);
        else
            setWindowTitle(title);
    }

    void WindowDecorationWrapper::adjustTitleBarGeometry()
    {
        if (m_titleBar)
            m_titleBar->resize(width(), m_titleBar->height());
    }

    void WindowDecorationWrapper::adjustWrapperGeometry()
    {
        if (m_guestWidget && m_titleBar)
        {
            const QMargins m = margins();
            const QSize nonContentsSz = nonContentsSize();
            const QSize contentsSz = m_guestWidget->size();
            resize(nonContentsSz + contentsSz);
            m_guestWidget->move(m.left(), m.top());
        }
    }

    void WindowDecorationWrapper::adjustWidgetGeometry()
    { // Called when resizing the wrapper manually
        if (m_guestWidget)
        {
            const QMargins m = margins();
            const QSize newSize = size() - nonContentsSize();
            const QPoint topLeft = QPoint(m.left(), m.top());
            m_guestWidget->setGeometry(QRect(topLeft, newSize));
        }
    }

    void WindowDecorationWrapper::updateConstraints()
    {
        // Copy size constraints from the widget
        if (!m_guestWidget)
        {
            return;
        }

        const QSize nonContentsSize = this->nonContentsSize();
        const int nonContentsWidth = nonContentsSize.width();
        const int nonContentsHeight = nonContentsSize.height();

        QSize guestMinSize = m_guestWidget->minimumSize();
        guestMinSize.setWidth(qMax(guestMinSize.width(), 5));
        guestMinSize.setHeight(qMax(guestMinSize.height(), 5));

        setMinimumSize(guestMinSize + nonContentsSize);

        setMaximumHeight(qMin(m_guestWidget->maximumHeight() + nonContentsHeight, QWIDGETSIZE_MAX));
        setMaximumWidth(qMin(m_guestWidget->maximumWidth() + nonContentsWidth, QWIDGETSIZE_MAX));

        updateTitleBarButtons();
    }

    void WindowDecorationWrapper::updateTitleBarButtons()
    {
        if (!autoTitleBarButtonsEnabled() || !isAttached() || m_titleBar == nullptr)
        {
            return;
        }

        TitleBar::WindowDecorationButtons buttons;
        const auto flags = m_guestWidget->windowFlags();

        if (flags & Qt::WindowMinimizeButtonHint)
        {
            buttons.append(DockBarButton::MinimizeButton);
        }

        if ((canResize() || isMaximized()) && (flags & Qt::WindowMaximizeButtonHint))
        {
            buttons.append(DockBarButton::MaximizeButton);
        }

        // We could also honor WindowCloseButtonHint, but there's no good reason, for now.

        buttons.append(DockBarButton::CloseButton);

        m_titleBar->setButtons(buttons);
    }

    /** static */
    QMargins WindowDecorationWrapper::win10TitlebarHeight(QWindow* w)
    {
        qDebug() << w->geometry() << w->frameGeometry() << w->frameMargins();
        return QMargins(0, -w->frameMargins().top(), 0, 0);
    }

    Qt::WindowFlags WindowDecorationWrapper::specialFlagsForOS()
    {
        // Qt::CustomizeWindowHint means native frame but no native titlebar
        // For Win 10 we have the native titlebar but we draw on top of it, otherwise QTBUG-47543

        return isWin10() ? Qt::WindowFlags() : Qt::CustomizeWindowHint;
    }

    void WindowDecorationWrapper::drawFrame(const QStyleOption *option, QPainter *painter, const QWidget *widget)
    {
        Q_UNUSED(widget);
        painter->save();
        painter->setPen(QColor(33, 34, 35));
        painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
        painter->restore();
    }

    bool WindowDecorationWrapper::event(QEvent* ev)
    {
        // Overridden for debugging purposes
        return QFrame::event(ev);
    }

} // namespace AzQtComponents

#include "Components/moc_WindowDecorationWrapper.cpp"
