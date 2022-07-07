#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/Widgets/OverlayWidget.h> // needed for OverlayWidget::OverlayWidgetButton
#include <QWidget>
#include <QFrame>
#include <QVector>
#include <QScopedPointer>
#endif

class QPushButton;

namespace AzQtComponents
{
    class OverlayWidget;

    namespace Internal
    {
        // QT space
        namespace Ui
        {
            class OverlayWidgetLayer;
        }

        // Layer for the OverlayWidget. Do not use directly but use the functions in OverlayWidget.
        class OverlayWidgetLayer : public QFrame
        {
            struct Button
            {
                QPushButton* m_button;
                OverlayWidgetButton::Callback m_callback;
                OverlayWidgetButton::EnabledCheck m_enabledCheck;
                bool m_triggersPop;
            };

            Q_OBJECT
        public:
            OverlayWidgetLayer(OverlayWidget* parent, QWidget* centerWidget, QWidget* breakoutWidget, const char* title, const OverlayWidgetButtonList& buttons);
            ~OverlayWidgetLayer() override;

            virtual void Refresh();
            virtual bool CanClose() const;
            virtual void PopLayer();

        protected:
            virtual void AddButtons(Ui::OverlayWidgetLayer& widget, const OverlayWidgetButtonList& buttons, bool hasBreakoutWindow);
            virtual void ButtonClicked(size_t index);

            void RefreshCloseButton();

            bool eventFilter(QObject* object, QEvent* event) override;

            QVector<Button> m_buttons;
            QScopedPointer<Ui::OverlayWidgetLayer> m_ui;
            OverlayWidget* m_parent;
            QPointer<QDialog> m_breakoutDialog;
            int m_breakoutCloseButtonIndex;
        };
    }
} // namespace AzQtComponents
