/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QPushButton>
#include <QLabel>
#endif

namespace O3DE::ProjectManager
{
    class TemplateImageOverlay
        : public QLabel
    {
        Q_OBJECT // AUTOMOC
    public:
        explicit TemplateImageOverlay(const QString& imagePath, QWidget* parent = nullptr);
        ~TemplateImageOverlay() = default;

        void SetTemplateImageOverlayImage(const QString& overlayImagePath);

    private:
        QLabel* m_overlayLabel;
    };

    class TemplateButton
        : public QPushButton 
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit TemplateButton(const QString& imagePath, const QString& labelText, QWidget* parent = nullptr);
        ~TemplateButton() = default;

    protected slots:
        void onToggled();

    protected:
        TemplateImageOverlay* m_imageOverlay = nullptr;
    };
} // namespace O3DE::ProjectManager
