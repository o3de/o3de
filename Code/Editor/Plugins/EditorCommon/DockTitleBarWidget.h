/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#if !defined(Q_MOC_RUN)
#include "EditorCommonAPI.h"
#include <QAbstractButton>
#include <QWidget>
#include <QPainter>
#include <QDockWidget>
#include <QBoxLayout>

#include <vector>
#endif

class EDITOR_COMMON_API CDockTitleBarWidget
    : public QWidget
{
    Q_OBJECT
public:
    CDockTitleBarWidget(QDockWidget* dockWidget);

    QSize sizeHint() const override
    {
        QFontMetrics fm(font());
        return QSize(40, fm.height() + 8);
    }

    void AddCustomButton(const QIcon& icon, const char* tooltip, int id);
signals:
    void SignalCustomButtonPressed(int id);
private slots:
    void OnCloseButtonPressed();
    void OnFloatButtonPressed();
    void OnCustomButtonPressed();
private:
    QDockWidget* m_dockWidget;
    QBoxLayout* m_layout;
    QBoxLayout* m_buttonLayout;

    QAbstractButton* m_floatButton;
    QAbstractButton* m_closeButton;

    struct SCustomButton
    {
        int id;
        QAbstractButton* button;
    };
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    std::vector<SCustomButton> m_customButtons;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

