/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        explicit FormBrowseEditWidget(const QString& labelText = "", QWidget* parent = nullptr);
        ~FormBrowseEditWidget() = default;

    signals:
        void OnBrowse();

    protected:
        void keyPressEvent(QKeyEvent* event) override;

    protected slots:
        virtual void HandleBrowseButton() {};
    };
} // namespace O3DE::ProjectManager
