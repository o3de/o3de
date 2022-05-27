/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QCheckbox>
#endif

namespace O3DE::ProjectManager
{
    class ToggleCheckbox
        : public QCheckBox
    {
    public:
        explicit ToggleCheckbox(QWidget* parent = nullptr);

    protected:
        void paintEvent(QPaintEvent* e) override;
        bool hitButton(const QPoint& pos) const override;
    };
} // namespace O3DE::ProjectManager
