/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <QFrame>
#include <QPointer>
#include <QTimer>

class QLabel;

namespace WhiteBox
{
    //! Non-interactive, themed tool guidance anchored near the viewport's bottom-left edge.
    //! A tool window keeps it above the native render surface without taking viewport input.
    class WhiteBoxToolStatus : public QFrame
    {
    public:
        explicit WhiteBoxToolStatus(QWidget* viewport);
        void SetStatus(const QString& instructions, const QString& error);
        void HideStatus();

    private:
        void UpdatePlacement();

        QPointer<QWidget> m_viewport;
        QLabel* m_instructions = nullptr;
        QLabel* m_error = nullptr;
        QTimer m_placementTimer;
        bool m_requestedVisible = false;
    };
} // namespace WhiteBox
