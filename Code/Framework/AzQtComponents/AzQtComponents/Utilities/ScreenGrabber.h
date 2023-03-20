/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QWidget>
#include <QImage>

namespace AzQtComponents
{

    class Eyedropper;

    class ScreenGrabber
        : public QObject
    {
        Q_OBJECT

    public:
        class Internal;

    public:
        explicit ScreenGrabber(const QSize size, Eyedropper* parent = nullptr);
        ~ScreenGrabber() override;

        QImage grab(const QPoint& point) const;

    private:
        QSize m_size;
        [[maybe_unused]] Eyedropper* m_owner;
        QScopedPointer<Internal> m_internal;
    };

} // namespace AzQtComponents
