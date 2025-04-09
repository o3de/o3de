/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/PropertyWidgets/BlendTreeRotationLimitHandler.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <EMotionFX/Source/ConstraintTransformRotationAngles.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeRotationLimitHandler, EditorAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeRotationLimitContainerHandler, EditorAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(RotationLimitWdget, EditorAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(RotationLimitContainerWdget, EditorAllocator);

    const int RotationLimitWdget::s_decimalPlaces = 1;

    RotationLimitWdget::RotationLimitWdget([[maybe_unused]] QWidget* parent)
    {
        m_tooltipText = QString("Min %1 \xB0\n").arg((int)BlendTreeRotationLimitNode::RotationLimit::s_rotationLimitRangeMin);
        m_tooltipText += QString("Max %1 \xB0").arg((int)BlendTreeRotationLimitNode::RotationLimit::s_rotationLimitRangeMax);

        QHBoxLayout* hLayout = new QHBoxLayout(this);

        hLayout->setMargin(2);
        setLayout(hLayout);
        m_spinBoxMin = new AzQtComponents::DoubleSpinBox(this);
        layout()->addWidget(m_spinBoxMin);
        m_spinBoxMax = new AzQtComponents::DoubleSpinBox(this);
        layout()->addWidget(m_spinBoxMax);
        m_spinBoxMin->setRange(BlendTreeRotationLimitNode::RotationLimit::s_rotationLimitRangeMin, BlendTreeRotationLimitNode::RotationLimit::s_rotationLimitRangeMax);
        m_spinBoxMax->setRange(BlendTreeRotationLimitNode::RotationLimit::s_rotationLimitRangeMin, BlendTreeRotationLimitNode::RotationLimit::s_rotationLimitRangeMax);
        m_spinBoxMin->setDecimals(s_decimalPlaces);
        m_spinBoxMax->setDecimals(s_decimalPlaces);

        m_spinBoxMin->setToolTip(m_tooltipText);
        m_spinBoxMax->setToolTip(m_tooltipText);
        connect(m_spinBoxMin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &RotationLimitWdget::HandleMinValueChanged);
        connect(m_spinBoxMax, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &RotationLimitWdget::HandleMaxValueChanged);
    }

    void RotationLimitWdget::SetRotationLimit(const BlendTreeRotationLimitNode::RotationLimit& rotationLimit)
    {
        m_rotationLimit = &rotationLimit;
    }

    void RotationLimitWdget::UpdateGui()
    {
        m_spinBoxMin->setValue(m_rotationLimit->GetLimitMin());
        m_spinBoxMax->setValue(m_rotationLimit->GetLimitMax());
    }

    void RotationLimitWdget::HandleMinValueChanged(double value)
    {
        m_spinBoxMin->setValue(value);
        if (m_spinBoxMin->value() <= m_spinBoxMax->value())
        {
            AzQtComponents::SpinBox::setHasError(m_spinBoxMax, false);
            AzQtComponents::SpinBox::setHasError(m_spinBoxMin, false);
            m_spinBoxMin->setToolTip(m_tooltipText);
            m_spinBoxMax->setToolTip(m_tooltipText);
            emit DataChanged();
        }
        else
        {
            AzQtComponents::SpinBox::setHasError(m_spinBoxMin, true);
            QString errorTooltip = QString("The value has to be less than or equal to %1 \xB0").arg(m_spinBoxMax->value());
            m_spinBoxMin->setToolTip(errorTooltip);
        }
    }

    void RotationLimitWdget::HandleMaxValueChanged(double value)
    {
        m_spinBoxMax->setValue(value);
        if (m_spinBoxMin->value() <= m_spinBoxMax->value())
        {
            AzQtComponents::SpinBox::setHasError(m_spinBoxMax, false);
            AzQtComponents::SpinBox::setHasError(m_spinBoxMin, false);
            m_spinBoxMin->setToolTip(m_tooltipText);
            m_spinBoxMax->setToolTip(m_tooltipText);

            emit DataChanged();
        }
        else
        {
            AzQtComponents::SpinBox::setHasError(m_spinBoxMax, true);
            QString errorTooltip = QString("The value has to be greater than or equal to %1 \xB0").arg(m_spinBoxMin->value());
            m_spinBoxMax->setToolTip(errorTooltip);
        }
    }


    float RotationLimitWdget::GetMin() const
    {
        return static_cast<float>(m_spinBoxMin->value());
    }

    float RotationLimitWdget::GetMax() const
    {
        return static_cast<float>(m_spinBoxMax->value());
    }

    QWidget* BlendTreeRotationLimitHandler::CreateGUI(QWidget* parent)
    {
        RotationLimitWdget* rotationLimitWdget = aznew RotationLimitWdget(parent);

        connect(rotationLimitWdget, &RotationLimitWdget::DataChanged, this, [rotationLimitWdget]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestWrite, rotationLimitWdget);
        });

        return rotationLimitWdget;
    }

    AZ::u32 BlendTreeRotationLimitHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("BlendTreeRotationLimitHandler");
    }

    void BlendTreeRotationLimitHandler::ConsumeAttribute(RotationLimitWdget* /*widget*/, AZ::u32 /*attrib*/, AzToolsFramework::PropertyAttributeReader* /*attrValue*/, const char* /*debugName*/)
    {

    }

    void BlendTreeRotationLimitHandler::WriteGUIValuesIntoProperty(size_t /*index*/, RotationLimitWdget* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* /*node*/)
    {
        instance.SetMin(GUI->GetMin());
        instance.SetMax(GUI->GetMax());
    }

    bool BlendTreeRotationLimitHandler::ReadValuesIntoGUI(size_t /*index*/, RotationLimitWdget* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* /*node*/)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetRotationLimit(instance);
        GUI->UpdateGui();
        return true;
    }



    RotationLimitContainerWdget::RotationLimitContainerWdget([[maybe_unused]] QWidget* parent)
    {
        QHBoxLayout* hLayout = new QHBoxLayout(this);

        hLayout->setMargin(2);
        setLayout(hLayout);
        QLabel* headerColumn1 = new QLabel("Min angle \xB0", this);
        layout()->addWidget(headerColumn1);
        QLabel* headerColumn2 = new QLabel("Max angle \xB0", this);
        layout()->addWidget(headerColumn2);
    }


    QWidget* BlendTreeRotationLimitContainerHandler::CreateGUI(QWidget* parent)
    {
        RotationLimitContainerWdget* rotationLimitWdget = aznew RotationLimitContainerWdget(parent);

        return rotationLimitWdget;
    }

    AZ::u32 BlendTreeRotationLimitContainerHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("BlendTreeRotationLimitContainerHandler");
    }

    void BlendTreeRotationLimitContainerHandler::ConsumeAttribute(RotationLimitContainerWdget* /*widget*/, AZ::u32 /*attrib*/, AzToolsFramework::PropertyAttributeReader* /*attrValue*/, const char* /*debugName*/)
    { }

    void BlendTreeRotationLimitContainerHandler::WriteGUIValuesIntoProperty(size_t /*index*/, [[maybe_unused]] RotationLimitContainerWdget* GUI, [[maybe_unused]] property_t& instance, AzToolsFramework::InstanceDataNode* /*node*/)
    { }

    bool BlendTreeRotationLimitContainerHandler::ReadValuesIntoGUI(size_t /*index*/, [[maybe_unused]] RotationLimitContainerWdget* GUI, [[maybe_unused]] const property_t& instance, AzToolsFramework::InstanceDataNode* /*node*/)
    {
        return true;
    }

}
