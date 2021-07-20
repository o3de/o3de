/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QPushButton>
#endif

namespace O3DE::ProjectManager
{
    class TemplateButton
        : public QPushButton 
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit TemplateButton(const QString& imagePath, const QString& labelText, QWidget* parent = nullptr);
        ~TemplateButton() = default;

    protected slots:
        void onToggled();

    private:
        inline constexpr static int s_templateImageWidth = 92;
        inline constexpr static int s_templateImageHeight = 122;
    };
} // namespace O3DE::ProjectManager
