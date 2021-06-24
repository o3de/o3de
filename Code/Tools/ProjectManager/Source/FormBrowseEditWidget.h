/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <FormLineEditWidget.h> 
#endif

namespace O3DE::ProjectManager
{
    class FormBrowseEditWidget
        : public FormLineEditWidget 
    {
        Q_OBJECT

    public:
        explicit FormBrowseEditWidget(const QString& labelText, const QString& valueText = "", QWidget* parent = nullptr);
        ~FormBrowseEditWidget() = default;

    protected slots:
        virtual void HandleBrowseButton() = 0;
    };
} // namespace O3DE::ProjectManager
