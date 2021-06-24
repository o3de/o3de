/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QPushButton>
#endif

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API HelpButton
        : public QPushButton
    {
        Q_OBJECT

    public:
        explicit HelpButton(QWidget* parent = nullptr);
    };
} // namespace AzQtComponents


