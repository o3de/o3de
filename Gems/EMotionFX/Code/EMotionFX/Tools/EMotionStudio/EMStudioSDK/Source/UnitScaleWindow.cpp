/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "UnitScaleWindow.h"
#include <QLabel>
#include <QSizePolicy>
#include <QPixmap>
#include <QCheckBox>
#include <QSettings>
#include <QVBoxLayout>
#include <QPushButton>


namespace EMStudio
{
    // constructor
    UnitScaleWindow::UnitScaleWindow(QWidget* parent)
        : QDialog(parent)
    {
        m_scaleFactor = 1.0f;
        setModal(true);

        setWindowTitle("Scale Factor Setup");
        setObjectName("StyledWidgetDark");
        setFixedSize(220, 107);

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setMargin(0);

        QVBoxLayout* topLayout = new QVBoxLayout();

        QLabel* topLabel = new QLabel("<b>Please setup a scale factor:</b>");
        topLabel->setStyleSheet("background-color: rgb(40, 40, 40); padding: 6px;");
        topLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        topLayout->addWidget(topLabel);

        layout->addLayout(topLayout);

        QHBoxLayout* scaleLayout = new QHBoxLayout();
        scaleLayout->setMargin(9);

        scaleLayout->addWidget(new QLabel("Scale Factor:"));

        m_scaleSpinBox = new AzQtComponents::DoubleSpinBox();
        m_scaleSpinBox->setRange(0.00001, 100000.0f);
        m_scaleSpinBox->setSingleStep(0.01);
        m_scaleSpinBox->setDecimals(7);
        m_scaleSpinBox->setValue(1.0f);
        scaleLayout->addWidget(m_scaleSpinBox);

        layout->addLayout(scaleLayout);

        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setContentsMargins(9, 0, 9, 9);

        m_ok     = new QPushButton("OK");
        m_cancel = new QPushButton("Cancel");
        hLayout->addWidget(m_ok);
        hLayout->addWidget(m_cancel);

        layout->addLayout(hLayout);

        connect(m_ok, &QPushButton::clicked, this, &UnitScaleWindow::OnOKButton);
        connect(m_cancel, &QPushButton::clicked, this, &UnitScaleWindow::OnCancelButton);
    }


    // destructor
    UnitScaleWindow::~UnitScaleWindow()
    {
    }


    // accept
    void UnitScaleWindow::OnOKButton()
    {
        m_scaleFactor = static_cast<float>(m_scaleSpinBox->value());
        emit accept();
    }


    // reject
    void UnitScaleWindow::OnCancelButton()
    {
        emit reject();
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/moc_UnitScaleWindow.cpp>
