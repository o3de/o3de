/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QWidget>
#include <QLabel>
#if !defined(Q_MOC_RUN)
#include <AzCore/std/string/string.h>
#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <AzQtComponents/Components/Widgets/VectorInput.h>
#include <Serializer/DataConvertor.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyColorCtrl.hxx>
#endif

namespace OpenParticleSystemEditor
{
    class InfoLineEdit : public QWidget
    {
        Q_OBJECT
    public:
        InfoLineEdit(const QString& text, AZ::TypeId id, QWidget* parent = 0);
        ~InfoLineEdit() override;

        // OpenParticle::ValueObjVecX
        template<typename ValueType>
        inline void SetValue(const ValueType& value)
        {
            for (int i = 0; i < m_vectorInput->getSize(); ++i)
            {
                m_vectorInput->setValuebyIndex(value.dataValue.GetElement(i), i);
            }
        }

        template<>
        void SetValue<float>(const float& value)
        {
            m_spinBox->setValue(value);
        }

        template<>
        void SetValue<OpenParticle::ValueObjFloat>(const OpenParticle::ValueObjFloat& value)
        {
            m_spinBox->setValue(value.dataValue);
        }

        template<>
        void SetValue(const AZ::Vector3& value)
        {
            for (int i = 0; i < m_vectorInput->getSize(); ++i)
            {
                m_vectorInput->setValuebyIndex(value.GetElement(i), i);
            }
        }

        template<>
        void SetValue<OpenParticle::ValueObjColor>(const OpenParticle::ValueObjColor& value)
        {
            QColor asQColor;
            asQColor.setRedF(value.dataValue.GetR());
            asQColor.setGreenF(value.dataValue.GetG());
            asQColor.setBlueF(value.dataValue.GetB());
            asQColor.setAlphaF(value.dataValue.GetA());
            m_colorEdit->setValue(asQColor);
        }

        template<>
        void SetValue<OpenParticle::ValueObjLinear>(const OpenParticle::ValueObjLinear& value)
        {
            for (int i = 0; i < m_vectorInput->getSize(); ++i)
            {
                m_vectorInput->setValuebyIndex(value.dataValue.minValue.GetElement(i), i);
            }             
        }

        // ValueObjVecX
        template<typename ValueType>
        inline ValueType GetValue()
        {
            ValueType value;
            AzQtComponents::VectorElement** elements = m_vectorInput->getElements();
            
            for (int i = 0; i < m_vectorInput->getSize(); ++i)
            {
                AzQtComponents::VectorElement* currentElement = elements[i];
                value.dataValue.SetElement(i, static_cast<float>(currentElement->getValue()));
            }
            return value;
        }

        template<>
        float GetValue<float>()
        {
            return (float)m_spinBox->value();
        }

        template<>
        AZ::Vector3 GetValue()
        {
            AZ::Vector3 value = AZ::Vector3::CreateZero();
            AzQtComponents::VectorElement** elements = m_vectorInput->getElements();
            
            for (int i = 0; i < m_vectorInput->getSize(); ++i)
            {
                AzQtComponents::VectorElement* currentElement = elements[i];
                value.SetElement(i, static_cast<float>(currentElement->getValue()));
            }
            return value;
        }

        template<>
        OpenParticle::ValueObjFloat GetValue<OpenParticle::ValueObjFloat>()
        {
            OpenParticle::ValueObjFloat value;
            value.dataValue = (float)m_spinBox->value();
            return value;
        }

        template<>
        OpenParticle::ValueObjColor GetValue<OpenParticle::ValueObjColor>()
        {
            QColor val = m_colorEdit->value();
            OpenParticle::ValueObjColor color;
            color.dataValue.Set((float)val.redF(), (float)val.greenF(), (float)val.blueF(), (float)val.alphaF());
            return color;
        }

        template<>
        OpenParticle::ValueObjLinear GetValue<OpenParticle::ValueObjLinear>()
        {
            OpenParticle::ValueObjLinear value;

            AzQtComponents::VectorElement** firstElements = m_vectorInput->getElements();
            for (int i = 0; i < m_vectorInput->getSize(); ++i)
            {
                AzQtComponents::VectorElement* currentElement = firstElements[i];
                value.dataValue.minValue.SetElement(i, static_cast<float>(currentElement->getValue()));
            }
            return value;
        }

        void SetMinimum(double min);
        void SetMaximum(double max);
        void SetSuffix(const QString& suffix);
        void SetLabelText(const QString& text);
        void SetUniformVisibility(bool uniform);

    signals:
        void valueChanged();

    private:
        void SetupUI(const QString& text);
        bool IsVectorType(AZ::TypeId id);
        int GetVectorElementCount(AZ::TypeId id);

        // Value Type Id
        AZ::TypeId m_id;
        QLabel* m_label = nullptr;
        AzQtComponents::DoubleSpinBox* m_spinBox = nullptr;
        AzToolsFramework::PropertyColorCtrl* m_colorEdit = nullptr;
        AzQtComponents::VectorInput* m_vectorInput = nullptr;
    };
} // namespace OpenParticleSystemEditor
