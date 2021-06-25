/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QObject>

namespace AzQtComponents
{

    class MouseHider
        : public QObject
    {
    public:
        explicit MouseHider(QObject* parent = nullptr);
        ~MouseHider() override;
    };

} // namespace AzQtComponents
