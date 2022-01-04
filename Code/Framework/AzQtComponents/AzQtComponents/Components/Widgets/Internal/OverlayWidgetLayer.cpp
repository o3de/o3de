/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzQtComponents/Components/Widgets/Internal/OverlayWidgetLayer.h>
#include <AzQtComponents/Components/StyledDialog.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <AzQtComponents/Components/Titlebar.h>
#include <QEvent>
#include <QPushButton>
#include <QMessageBox>
#include <AzCore/Casting/numeric_cast.h>
#include <AzQtComponents/Components/Widgets/Internal/ui_OverlayWidgetLayer.h>
#include <AzCore/Debug/Trace.h>

namespace AzQtComponents
{
    namespace
    {
        QWidget *topmostAncestor(QWidget *w)
        {
            while (w->parentWidget())
                w = w->parentWidget();
            return w;
        }
    }

    namespace Internal
    {
        static const QString LayerStyle = QStringLiteral("background-color:rgba(0, 0, 0, 179)");

        OverlayWidgetLayer::OverlayWidgetLayer(OverlayWidget* parent, QWidget* centerWidget, QWidget* breakoutWidget, 
            const char* title, const OverlayWidgetButtonList& buttons)
            : QFrame(parent)
            , m_ui(new Ui::OverlayWidgetLayer())
            , m_parent(parent)
            , m_breakoutDialog(nullptr)
            , m_breakoutCloseButtonIndex(-1)
        {
            // If there's no parent to dock the center widget in and there's no widget
            // assigned to breakout, put the center widget in the breakout window.
            if (!parent && !breakoutWidget)
            {
                breakoutWidget = centerWidget;
                centerWidget = nullptr;
            }

            // HACK: make this dialog a child of the topmost ancestor widget of `parent', not
            // `parent' itself; otherwise, if parent's window is destroyed (which may happen
            // if e.g. parent is in a floating dock widget that gets docked) the dialog loses
            // its transient parent, and can then be hidden by the main window
            m_breakoutDialog = new AzQtComponents::StyledDialog(topmostAncestor(parent));
            auto closeBreakoutDialog = [this]()
            {
                if (m_breakoutDialog)
                {
                    m_breakoutDialog->deleteLater();
                    m_breakoutDialog = nullptr;
                }
            };
                
            if (breakoutWidget)
            {
                setStyleSheet(LayerStyle);
                setLayout(new QHBoxLayout());
                
                // close the overlay if either dependent widget is destroyed
                QObject::connect(breakoutWidget, &QObject::destroyed, this, closeBreakoutDialog);

                if (centerWidget)
                {
                    layout()->addWidget(centerWidget);
                    centerWidget->installEventFilter(this);
                }

                connect(m_breakoutDialog, &QDialog::finished, this, &OverlayWidgetLayer::PopLayer);
                m_ui->setupUi(m_breakoutDialog);
                m_ui->m_centerLayout->addWidget(breakoutWidget);
                m_breakoutDialog->installEventFilter(this);
                breakoutWidget->installEventFilter(this);

                AddButtons(*m_ui.data(), buttons, true);

                m_breakoutDialog->setWindowTitle(title);
                m_breakoutDialog->setMinimumSize(640, 480);
                m_breakoutDialog->show();
            }
            else
            {
                m_ui->setupUi(this);
                if (centerWidget)
                {
                    m_ui->m_centerLayout->addWidget(centerWidget);
                    centerWidget->installEventFilter(this);
                }
                else
                {
                    setStyleSheet(LayerStyle);
                }
                AddButtons(*m_ui.data(), buttons, parent == nullptr);
            }
        }

        OverlayWidgetLayer::~OverlayWidgetLayer() = default;

        void OverlayWidgetLayer::Refresh()
        {
            for (Button& button : m_buttons)
            {
                if (button.m_button && button.m_enabledCheck)
                {
                    button.m_button->setEnabled(button.m_enabledCheck());
                }
            }

            RefreshCloseButton();
        }

        void OverlayWidgetLayer::PopLayer()
        {
            // Can't use scope signal blocker here as signals do need to be sent to child widgets.
            if (m_breakoutDialog)
            {
                m_breakoutDialog->removeEventFilter(this);
                m_breakoutDialog->close();
            }

            if (m_parent)
            {
                m_parent->PopLayer(this);
            }
        }

        bool OverlayWidgetLayer::CanClose() const
        {
            if (m_breakoutCloseButtonIndex >= 0)
            {
                const Button& button = m_buttons[m_breakoutCloseButtonIndex];
                if (button.m_enabledCheck)
                {
                    return button.m_enabledCheck();
                }
            }
            return true;
        }

        bool OverlayWidgetLayer::eventFilter(QObject* object, QEvent* event)
        {
            if (event->type() == QEvent::Close)
            {
                // There's a breakout window with close button
                if (CanClose())
                {
                    if (m_breakoutCloseButtonIndex >= 0)
                    {
                        Button& button = m_buttons[m_breakoutCloseButtonIndex];
                        if (button.m_callback)
                        {
                            button.m_callback();
                        }
                    }

                    PopLayer();
                    return true;
                }
                else
                {
                    event->ignore();
                    return true;
                }
            }
            return QWidget::eventFilter(object, event);
        }

        void OverlayWidgetLayer::AddButtons(Ui::OverlayWidgetLayer& widget, const OverlayWidgetButtonList& buttons, bool hasBreakoutWindow)
        {
            bool hasButtons = false;
            for (const OverlayWidgetButton* info : buttons)
            {
                if (info->m_isCloseButton && hasBreakoutWindow && m_breakoutCloseButtonIndex == -1)
                {
                    m_breakoutCloseButtonIndex = aznumeric_caster(m_buttons.size());
                    m_buttons.push_back({ nullptr, info->m_callback, info->m_enabledCheck, true });
                }
                    
                QPushButton* button = new QPushButton(info->m_text);
                size_t index = m_buttons.size();
                m_buttons.push_back({ button, info->m_callback, info->m_enabledCheck, info->m_triggersPop });

                // pass "this" as a context object so that the connection is properly torn down if the OverlayWidgetLayer is destroyed
                connect(button, &QPushButton::clicked, this, [this, index]()
                {
                    ButtonClicked(index);
                });
                widget.m_controlsLayout->addWidget(button);
                if (info->m_enabledCheck)
                {
                    button->setEnabled(info->m_enabledCheck());
                }
                hasButtons = true;
            }
            m_ui->m_controls->setVisible(hasButtons);

            RefreshCloseButton();
        }

        void OverlayWidgetLayer::ButtonClicked(size_t index)
        {
            AZ_Assert(index < m_buttons.size(), "Invalid index for button in overlay layer.");

            Button& button = m_buttons[static_cast<int>(index)];
            if (button.m_enabledCheck)
            {
                if (!button.m_enabledCheck())
                {
                    return;
                }
            }

            if (button.m_callback)
            {
                button.m_callback();
            }

            if (button.m_triggersPop)
            {
                PopLayer();
            }
        }

        void OverlayWidgetLayer::RefreshCloseButton()
        {
            if (m_breakoutDialog)
            {
                using namespace AzQtComponents;
                WindowDecorationWrapper* wrapper = qobject_cast<WindowDecorationWrapper*>(m_breakoutDialog->parentWidget());
                if (wrapper)
                {
                    TitleBar* titleBar = wrapper->titleBar();
                    if (titleBar)
                    {
                        if (CanClose())
                        {
                            titleBar->enableButton(DockBarButton::CloseButton);
                        }
                        else
                        {
                            titleBar->disableButton(DockBarButton::CloseButton);
                        }
                    }
                }
            }
        }

    } // namespace Internal
} // namespace AzQtComponents

#include "Components/Widgets/Internal/moc_OverlayWidgetLayer.cpp"
