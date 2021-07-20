/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#if !defined(Q_MOC_RUN)
#include <QWidget>
#include <QScopedPointer>
#endif

namespace Ui
{
    class GoToButton;
}

namespace AssetProcessor
{
    class GoToButton
        : public QWidget
    {
        Q_OBJECT
    public:
        explicit GoToButton(QWidget* parent = nullptr);
        ~GoToButton() override;

        bool eventFilter(QObject* watched, QEvent* event) Q_DECL_OVERRIDE;

        QScopedPointer<Ui::GoToButton> m_ui;
    };
} // namespace AssetProcessor
