/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QFrame>
#include <QString>

class QVBoxLayout;
class QIcon;

namespace EMotionFX
{
    class NotificationWidget
        : public QFrame
    {
        Q_OBJECT
    public:
        NotificationWidget(QWidget* parent, const QString& title);

        void addFeature(QWidget* feature);
    private:
        QVBoxLayout* m_featureLayout = nullptr;
    };
} // namespace AzQtComponents
