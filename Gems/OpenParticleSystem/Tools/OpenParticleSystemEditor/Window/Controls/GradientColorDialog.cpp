/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientColorDialog.h"

#include "GradientWidget.h"
#include "GradientColorPickerWidget.h"

#include <AzQtComponents/Components/Widgets/ColorPicker.h>
#include <AzQtComponents/Utilities/Conversions.h>

#include <QSettings>
#include <QApplication>
#include <QScreen>
#include <QLineEdit>

namespace OpenParticleSystemEditor
{
    GradientColorDialog::GradientColorDialog(QWidget* parent)
        : QDialog(parent)
        , m_layout(this)
        , m_okButton(this)
        , m_cancelButton(this)
    {
        setWindowTitle(tr("Gradient Editor"));
        setFixedSize(400, m_headerHeight * 5);

        SetupControls();
        SetupButtons();
        SetupUi();

        Initialize();
        ShowKeys();
    }

    void GradientColorDialog::SetupUi()
    {
        QLabel* ColorLabel = new QLabel(this);
        ColorLabel->setText(tr("Color"));
        ColorLabel->setMinimumSize(30, m_headerHeight);

        QVBoxLayout* verticleLayout = new QVBoxLayout();
        QHBoxLayout* infoLayout = new QHBoxLayout();

        infoLayout->addWidget(m_labelLocaltion);
        infoLayout->addWidget(m_lineEditLocation);
        infoLayout->addWidget(m_labelPercentage);
        infoLayout->addWidget(ColorLabel);
        infoLayout->addWidget(m_lineEditColor);

        verticleLayout->addLayout(infoLayout);
        verticleLayout->addSpacing(m_headerHeight);
        verticleLayout->addWidget(m_gradientWidget);
        verticleLayout->addWidget(m_gradientColorPickerWidget);
        verticleLayout->addSpacing(m_headerHeight);

        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->setAlignment(Qt::AlignRight);
        buttonLayout->addWidget(&m_okButton);
        buttonLayout->addWidget(&m_cancelButton);
        verticleLayout->addLayout(buttonLayout);

        m_layout.addLayout(verticleLayout, 0, 0);
        m_layout.setAlignment(Qt::AlignTop);
    }

    void GradientColorDialog::SetupControls()
    {
        m_gradientColorPickerWidget = new GradientColorPickerWidget(this);
        m_gradientColorPickerWidget->setFixedHeight(16);
        m_gradientColorPickerWidget->setMinimumWidth(400);

        m_gradientWidget = new GradientWidget();
        m_gradientWidget->setFixedHeight(m_headerHeight);

        m_labelLocaltion = new QLabel(this);
        m_labelLocaltion->setText(tr("Location"));
        m_labelLocaltion->setMinimumWidth(30);

        m_lineEditLocation = new QLineEdit(this);
        m_lineEditLocation->setText("100");
        m_lineEditLocation->setMinimumWidth(45);
        m_lineEditLocation->setReadOnly(true);
        m_lineEditLocation->installEventFilter(this);

        m_labelPercentage = new QLabel(this);
        m_labelPercentage->setText("%");
        m_labelPercentage->setMinimumSize(10, m_headerHeight);

        m_lineEditColor = new QLineEdit(this);
        m_lineEditColor->setText("100");
        m_lineEditColor->setMinimumWidth(80);
        m_lineEditColor->setReadOnly(true);
        m_lineEditColor->installEventFilter(this);
    }

    void GradientColorDialog::SetupButtons()
    {
        m_okButton.setText(tr("OK"));
        m_okButton.setFixedWidth(80);
        m_cancelButton.setText(tr("Cancel"));
        m_cancelButton.setFixedWidth(80);
        connect(&m_okButton, &QPushButton::clicked, this, [this]()
            {
                accept();
            });
        connect(&m_cancelButton, &QPushButton::clicked, this, [this]()
            {
                reject();
            });
    }

    void GradientColorDialog::Initialize()
    {
        m_gradientWidget->setAutoFillBackground(true);
        m_gradientWidget->AddGradient(new QLinearGradient(0, 0, 1, 0), QPainter::CompositionMode::CompositionMode_Source);

        m_gradientColorPickerWidget->SetGradientChangedCB([this](QGradientStops _stops)
            {
                this->OnGradientChanged(_stops);
            });
        connect(m_lineEditLocation, &QLineEdit::editingFinished, this, [this]()
            {
                // only update position if text is not empty
                if (m_lineEditLocation->text().isEmpty())
                {
                    while (m_lineEditLocation->isUndoAvailable())
                    {
                        m_lineEditLocation->undo();
                    }
                }
                // clamp the value and update location text
                int val = qMax(0, qMin(100, m_lineEditLocation->text().toInt()));
                m_lineEditLocation->setText(QString("%1").arg(val));
                m_gradientColorPickerWidget->SetSelectedKeyPosition(val);
                m_lineEditLocation->clearFocus();
            });
        m_gradientColorPickerWidget->SetLocationChangedCB([this](short location, [[maybe_unused]] QColor color)
            {
                ShowKeys(location, color);
                OnChangeUpdateDisplayedGradient();
            });
        m_gradientColorPickerWidget->SetDefaultLocationChangedCB([this]()
            {
                ShowKeys();
                OnChangeUpdateDisplayedGradient();
            });
    }

    void GradientColorDialog::AddKeyEnabled(bool enabled) const
    {
        if (m_gradientColorPickerWidget != nullptr)
        {
            m_gradientColorPickerWidget->AddKeyEnabled(enabled);
        }
    }

    void GradientColorDialog::OnGradientChanged(QGradientStops gradient)
    {
        m_gradientWidget->SetKeys(0, gradient);

        if ((bool)m_gradientChangedCB)
        {
            m_gradientChangedCB(gradient);
        }
        OnChangeUpdateDisplayedGradient();
    }

    QGradientStops GradientColorDialog::GetGradient()
    {
        return m_gradientColorPickerWidget->GetKeys();
    }

    void GradientColorDialog::SetGradient(QGradientStops stops)
    {
        m_gradientColorPickerWidget->SetKeys(stops);
    }

    void GradientColorDialog::OnChangeUpdateDisplayedGradient() const
    {
        m_gradientWidget->update();
        m_gradientColorPickerWidget->update();
    }

    void GradientColorDialog::RemoveCallbacks()
    {
        m_gradientChangedCB = nullptr;
    }

    void GradientColorDialog::SetCallbackGradient(std::function<void(QGradientStops)> callback)
    {
        m_gradientChangedCB = callback;
    }

    void GradientColorDialog::ShowKeys(int location, QColor color)
    {
        m_lineEditLocation->blockSignals(true);
        m_lineEditLocation->setText(QString("%1").arg(location));
        m_lineEditLocation->blockSignals(false);

        m_lineEditColor->blockSignals(true);
        m_lineEditColor->setText(QString().asprintf("%d,%d,%d,%d", color.red(), color.green(), color.blue(), color.alpha()));
        m_lineEditColor->blockSignals(false);

        m_labelPercentage->show();
        m_labelLocaltion->show();
        m_lineEditLocation->show();
        m_lineEditColor->show();
    }

#include <moc_GradientColorDialog.cpp>
} // namespace OpenParticleSystemEditor
