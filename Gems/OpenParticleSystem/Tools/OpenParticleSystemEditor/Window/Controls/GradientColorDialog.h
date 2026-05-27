/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QDialog>
#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

namespace OpenParticleSystemEditor
{
    class GradientWidget;
    class GradientColorPickerWidget;

    class GradientColorDialog : public QDialog
    {
        Q_OBJECT
    public:
        explicit GradientColorDialog(QWidget* parent = nullptr);
        virtual ~GradientColorDialog() override{};

        QGradientStops GetGradient();
        void SetGradient(QGradientStops stops);
        void SetCallbackGradient(std::function<void(QGradientStops)> callback);
        void RemoveCallbacks();

        void OnGradientChanged(QGradientStops gradient);
        void OnChangeUpdateDisplayedGradient() const;
        void AddKeyEnabled(bool enabled) const;

    protected:
        void SetupUi();
        void SetupControls();
        void SetupButtons();
        void Initialize();
        void ShowKeys(int location = 100, QColor color = QColor(255, 255, 255, 255));

        QLineEdit* m_lineEditLocation;
        QLineEdit* m_lineEditColor;
        QLabel* m_labelLocaltion;
        QLabel* m_labelPercentage;

        QColor m_selectedColor;
        QGridLayout m_layout;
        QPushButton m_okButton;
        QPushButton m_cancelButton;
        std::function<void(QGradientStops)> m_gradientChangedCB;
        GradientColorPickerWidget* m_gradientColorPickerWidget;
        GradientWidget* m_gradientWidget;
        const static int m_headerHeight = 24;
    };
} // namespace OpenParticleSystemEditor
