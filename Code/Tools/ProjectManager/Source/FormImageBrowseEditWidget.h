/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <FormBrowseEditWidget.h>
#endif

namespace O3DE::ProjectManager
{
    class FormImageBrowseEditWidget
        : public FormBrowseEditWidget
    {
        Q_OBJECT

    public:
        explicit FormImageBrowseEditWidget(const QString& labelText, const QString& valueText = "", QWidget* parent = nullptr);
        ~FormImageBrowseEditWidget() = default;

    protected:
        void HandleBrowseButton() override;
    };
} // namespace O3DE::ProjectManager
