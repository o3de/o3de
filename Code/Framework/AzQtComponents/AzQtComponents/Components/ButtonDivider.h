/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QFrame>
#endif

class QPainter;

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API ButtonDivider
        : public QFrame
    {
        Q_OBJECT

    public:
        explicit ButtonDivider(QWidget* parent = nullptr);
    };
} // namespace AzQtComponents


