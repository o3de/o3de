/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <QWidget>
#include <QAbstractButton>
#include <QLabel>
#include <QButtonGroup>

#if !defined(Q_MOC_RUN)
#include <AzCore/std/string/string.h>
#endif

namespace OpenParticleSystemEditor
{
    class InfoRadioButton : public QWidget
    {
        Q_OBJECT
    public:
        InfoRadioButton(const QString& text, int count, QWidget* parent = 0);
        ~InfoRadioButton() override;
        void SetCurrentChecked(int index);

    signals:
        void radioButtonChecked(int index);

    private slots:
        void onButtonClicked(QAbstractButton*);

    private:
        void SetupUI(const QString& text, int elementCount);

    private:
        QLabel* m_label = nullptr;
        QButtonGroup* m_buttonGroup = nullptr;
    };
} // namespace OpenParticleSystemEditor
