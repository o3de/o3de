/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QPushButton>
#endif

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QProgressBar)

namespace O3DE::ProjectManager
{
    class TemplateButton
        : public QPushButton 
    {
        Q_OBJECT

    public:
        explicit TemplateButton(const QString& imagePath, const QString& labelText, QWidget* parent = nullptr);
        ~TemplateButton() = default;

        void SetIsRemote(bool isRemote);
        void ShowDownloadProgress(bool showProgress);
        void SetProgressPercentage(float percent);

    protected slots:
        void onToggled();

    private:
        QLabel* m_cloudIcon = nullptr;
        QLabel* m_darkenOverlay = nullptr;
        QLabel* m_progressMessageLabel = nullptr;
        QProgressBar* m_progessBar = nullptr;
    };
} // namespace O3DE::ProjectManager
