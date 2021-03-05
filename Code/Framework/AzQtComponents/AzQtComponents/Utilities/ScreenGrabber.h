/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        Eyedropper* m_owner;
        QScopedPointer<Internal> m_internal;
    };

} // namespace AzQtComponents
