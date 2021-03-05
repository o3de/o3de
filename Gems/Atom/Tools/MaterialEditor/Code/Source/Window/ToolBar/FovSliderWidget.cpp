/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzQtComponents/Components/Widgets/Slider.h>
#include <Viewport/InputController/MaterialEditorViewportInputController.h>
#include <Source/Window/ToolBar/FovSliderWidget.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QHBoxLayout>
#include <QLabel>
AZ_POP_DISABLE_WARNING

namespace MaterialEditor
{
    FovSliderWidget::FovSliderWidget(QWidget* parent)
        : QWidget(parent)
    {
        QHBoxLayout* layout = new QHBoxLayout();
        m_label = new QLabel("Field of View");
        layout->addWidget(m_label);
        AzQtComponents::SliderInt* slider = new AzQtComponents::SliderInt(Qt::Orientation::Horizontal);
        slider->setRange(60, 120);
        connect(slider, &AzQtComponents::SliderInt::valueChanged, this, &FovSliderWidget::SliderValueChanged);
        slider->setValue(90);
        layout->addWidget(slider);
        setLayout(layout);
    }

    void FovSliderWidget::SliderValueChanged(int value) const
    {
        m_label->setText(QString("Field of View (%1)").arg(value));
        MaterialEditorViewportInputControllerRequestBus::Broadcast(
            &MaterialEditorViewportInputControllerRequestBus::Handler::SetFieldOfView,
            aznumeric_cast<float>(value));
    }
} // namespace MaterialEditor
