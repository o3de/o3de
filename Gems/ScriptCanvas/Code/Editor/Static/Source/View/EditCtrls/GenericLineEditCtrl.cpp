/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <ScriptCanvas/View/EditCtrls/GenericLineEditCtrl.h>

#include <AzToolsFramework/UI/PropertyEditor/PropertyQTConstants.h>
#include <QtWidgets/QHBoxLayout>

#include <QFocusEvent>

namespace ScriptCanvasEditor
{
    GenericLineEditCtrlBase::GenericLineEditCtrlBase(QWidget* pParent)
        : QWidget(pParent)
    {
        // create the gui, it consists of a layout, and in that layout, a text field for the value
        // and then a slider for the value.
        QHBoxLayout* pLayout = new QHBoxLayout(this);
        m_pLineEdit = new QLineEdit(this);

        pLayout->setSpacing(4);
        pLayout->setContentsMargins(1, 0, 1, 0);

        pLayout->addWidget(m_pLineEdit);

        m_pLineEdit->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        m_pLineEdit->setMinimumWidth(AzToolsFramework::PropertyQTConstant_MinimumWidth);
        m_pLineEdit->setFixedHeight(AzToolsFramework::PropertyQTConstant_DefaultHeight);

        m_pLineEdit->setFocusPolicy(Qt::StrongFocus);

        setLayout(pLayout);
        setFocusProxy(m_pLineEdit);
        setFocusPolicy(m_pLineEdit->focusPolicy());

        connect(m_pLineEdit, &QLineEdit::textChanged, this, &GenericLineEditCtrlBase::onChildLineEditValueChange);
        connect(m_pLineEdit, &QLineEdit::editingFinished, this, [this]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::OnEditingFinished, this);
        });
    }

    void GenericLineEditCtrlBase::setValue(AZStd::string_view value)
    {
        QSignalBlocker signalBlocker(m_pLineEdit);
        m_pLineEdit->setText(value.data());
    }

    void GenericLineEditCtrlBase::focusInEvent(QFocusEvent* e)
    {
        m_pLineEdit->event(e);
        m_pLineEdit->selectAll();
    }

    AZStd::string GenericLineEditCtrlBase::value() const
    {
        return AZStd::string(m_pLineEdit->text().toUtf8().data());
    }

    void GenericLineEditCtrlBase::setMaxLen(int maxLen)
    {
        QSignalBlocker signalBlocker(m_pLineEdit);
        m_pLineEdit->setMaxLength(maxLen);
    }

    void GenericLineEditCtrlBase::onChildLineEditValueChange(const QString& newValue)
    {
        AZStd::string changedVal(newValue.toUtf8().data());
        emit valueChanged(changedVal);
    }

    QWidget* GenericLineEditCtrlBase::GetFirstInTabOrder()
    {
        return m_pLineEdit;
    }
    QWidget* GenericLineEditCtrlBase::GetLastInTabOrder()
    {
        return m_pLineEdit;
    }

    void GenericLineEditCtrlBase::UpdateTabOrder()
    {
        // There's only one QT widget on this property.
    }
}

#include <Editor/Static/Include/ScriptCanvas/View/EditCtrls/moc_GenericLineEditCtrl.cpp>
