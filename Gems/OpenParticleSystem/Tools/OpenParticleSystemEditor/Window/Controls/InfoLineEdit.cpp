/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Window/Controls/InfoLineEdit.h>
#include <QHBoxLayout>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Casting/numeric_cast.h>

namespace OpenParticleSystemEditor
{
    InfoLineEdit::InfoLineEdit(const QString& text, AZ::TypeId id, QWidget* parent)
        : QWidget(parent), m_id(id)
    {
        SetupUI(text);
    }

    InfoLineEdit::~InfoLineEdit()
    {
    }

    bool InfoLineEdit::IsVectorType(AZ::TypeId id)
    {
        return id == azrtti_typeid<AZ::Vector2>() || id == azrtti_typeid<OpenParticle::ValueObjVec2>() ||
            id == azrtti_typeid<AZ::Vector3>() || id == azrtti_typeid<OpenParticle::ValueObjVec3>() ||
            id == azrtti_typeid<AZ::Vector4>() || id == azrtti_typeid<OpenParticle::ValueObjVec4>();
    }

    int InfoLineEdit::GetVectorElementCount(AZ::TypeId id)
    {
        const int ELEMENTCOUNT_VECTOR2 = 2;
        const int ELEMENTCOUNT_VECTOR3 = 3;
        const int ELEMENTCOUNT_VECTOR4 = 4;
        if (id == azrtti_typeid<AZ::Vector2>() || id == azrtti_typeid<OpenParticle::ValueObjVec2>())
        {
            return ELEMENTCOUNT_VECTOR2;
        }
        if (id == azrtti_typeid<AZ::Vector3>() || id == azrtti_typeid<OpenParticle::ValueObjVec3>())
        {
            return ELEMENTCOUNT_VECTOR3;
        }
        if (id == azrtti_typeid<AZ::Vector4>() || id == azrtti_typeid<OpenParticle::ValueObjVec4>())
        {
            return ELEMENTCOUNT_VECTOR4;
        }
        return 0;
    }

    void InfoLineEdit::SetupUI(const QString& text)
    {
        QHBoxLayout* layout = new QHBoxLayout();

        layout->setAlignment(Qt::AlignLeft);
        m_label = new QLabel();
        m_label->setContentsMargins(0, 0, 0, 0);
        m_label->setMinimumWidth(32);
        m_label->setText(text);
        layout->addWidget(m_label);
        if (text.length() == 0)
        {
            m_label->setVisible(false);
        }

        auto emitSignalFun = [this]()
        {
            emit valueChanged();
        };

        if (IsVectorType(m_id))
        {
            m_vectorInput = new AzQtComponents::VectorInput(this, GetVectorElementCount(m_id));
            layout->addWidget(m_vectorInput);

            connect(m_vectorInput, &AzQtComponents::VectorInput::editingFinished, this, emitSignalFun);
        }
        if (m_id == azrtti_typeid<float>() || m_id == azrtti_typeid<OpenParticle::ValueObjFloat>())
        {
            m_spinBox = new AzQtComponents::DoubleSpinBox(this);
            m_spinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            layout->addWidget(m_spinBox);

            connect(m_spinBox, &QDoubleSpinBox::editingFinished, this, emitSignalFun);
        }
        if (m_id == azrtti_typeid<OpenParticle::ValueObjColor>())
        {
            m_colorEdit = new AzToolsFramework::PropertyColorCtrl(this);
            m_colorEdit->setAlphaChannelEnabled(true);
            layout->addWidget(m_colorEdit);

            connect(m_colorEdit, &AzToolsFramework::PropertyColorCtrl::editingFinished, this, emitSignalFun);
        }
        if (m_id == azrtti_typeid<OpenParticle::ValueObjLinear>())
        {
            m_vectorInput = new AzQtComponents::VectorInput(this, 3);
            layout->addWidget(m_vectorInput);

            connect(m_vectorInput, &AzQtComponents::VectorInput::editingFinished, this, emitSignalFun);
        }

        layout->setContentsMargins(0, 0, 0, 0);
        setLayout(layout);
    }

    void InfoLineEdit::SetMinimum(double min)
    {
        if (m_id == azrtti_typeid<float>() || m_id == azrtti_typeid<OpenParticle::ValueObjFloat>())
        {
            m_spinBox->setMinimum(min);
        }
        if (IsVectorType(m_id))
        {
            m_vectorInput->setMinimum(min);
        }
    }

    void InfoLineEdit::SetMaximum(double max)
    {
        if (m_id == azrtti_typeid<float>() || m_id == azrtti_typeid<OpenParticle::ValueObjFloat>())
        {
            m_spinBox->setMaximum(max);
        }
        if (IsVectorType(m_id))
        {
            m_vectorInput->setMaximum(max);
        }
    }

    void InfoLineEdit::SetSuffix(const QString& suffix)
    {
        if (m_id == azrtti_typeid<float>() || m_id == azrtti_typeid<OpenParticle::ValueObjFloat>())
        {
            m_spinBox->setSuffix(suffix);
        }
        if (IsVectorType(m_id))
        {
            m_vectorInput->setSuffix(suffix);
        }
    }

    void InfoLineEdit::SetLabelText(const QString& text)
    {
        m_label->setText(text);
        m_label->setVisible(true);
    }

    void InfoLineEdit::SetUniformVisibility(bool uniform)
    {
        if (m_vectorInput == nullptr)
        {
            return;
        }
        AzQtComponents::VectorElement** vectorElements = m_vectorInput->getElements();
        for (int i = 1; i < m_vectorInput->getSize(); ++i)
        {
            vectorElements[i]->setVisible(!uniform);
        }
    }
} // namespace OpenParticleSystemEditor
