/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ExeSelectPropertyHandler.h"

#include <AzToolsFramework/UI/PropertyEditor/PropertyQTConstants.h>

#include <QFileDialog>
#include <QLayout>
#include <QLineEdit>

namespace AzToolsFramework
{
    ExeSelectPropertyCtrl::ExeSelectPropertyCtrl(QWidget* pParent)
        : QWidget(pParent)
    {
        QHBoxLayout* layout = new QHBoxLayout(this);
        m_browseEdit = new AzQtComponents::BrowseEdit(this);

        layout->setSpacing(4);
        layout->setContentsMargins(1, 0, 1, 0);
        layout->addWidget(m_browseEdit);

        browseEdit()->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        browseEdit()->setMinimumWidth(AzToolsFramework::PropertyQTConstant_MinimumWidth);
        browseEdit()->setFixedHeight(AzToolsFramework::PropertyQTConstant_DefaultHeight);

        browseEdit()->setFocusPolicy(Qt::StrongFocus);

        setLayout(layout);
        setFocusProxy(browseEdit());
        setFocusPolicy(browseEdit()->focusPolicy());

        m_browseEdit->setClearButtonEnabled(true);
        connect(m_browseEdit, &AzQtComponents::BrowseEdit::textChanged, this, &ExeSelectPropertyCtrl::ValueChangedByUser);
        connect(
            m_browseEdit, &AzQtComponents::BrowseEdit::textChanged, this,
            [this](const QString& /*text*/)
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::RequestWrite, this);
            });

        connect(m_browseEdit, &AzQtComponents::BrowseEdit::attachedButtonTriggered, this, &ExeSelectPropertyCtrl::SelectExe);
    }

    void ExeSelectPropertyCtrl::SelectExe()
    {
        QFileDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted)
        {
            QString file = dialog.selectedFiles()[0];
            dialog.fileSelected(file);
            SetValue(file);

            emit ValueChangedByUser();
        }
    }

    QString ExeSelectPropertyCtrl::GetValue() const
    {
        return m_browseEdit->text();
    }

    void ExeSelectPropertyCtrl::SetValue(const QString& value)
    {
        m_browseEdit->setText(value);
    }

    void ExeSelectPropertyCtrl::SetValueUser(const QString& value)
    {
        SetValue(value);

        emit ValueChangedByUser();
    }

    AzQtComponents::BrowseEdit* ExeSelectPropertyCtrl::browseEdit()
    {
        return m_browseEdit;
    }

    //  Handler  ///////////////////////////////////////////////////////////////////

    ExeSelectPropertyHandler::ExeSelectPropertyHandler()
        : AzToolsFramework::PropertyHandler<AZStd::string, ExeSelectPropertyCtrl>()
    {
    }

    AZ::u32 ExeSelectPropertyHandler::GetHandlerName(void) const
    {
        return AZ::Edit::UIHandlers::ExeSelectBrowseEdit;
    }

    bool ExeSelectPropertyHandler::AutoDelete() const
    {
        return true;
    }

    QWidget* ExeSelectPropertyHandler::CreateGUI(QWidget* pParent)
    {
        ExeSelectPropertyCtrl* ctrl = aznew ExeSelectPropertyCtrl(pParent);
        return ctrl;
    }

    void ExeSelectPropertyHandler::WriteGUIValuesIntoProperty(
        size_t /*index*/, ExeSelectPropertyCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* /*node*/)
    {
        instance = GUI->GetValue().toUtf8().data();
    }

    bool ExeSelectPropertyHandler::ReadValuesIntoGUI(
        size_t /*index*/, ExeSelectPropertyCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* /*node*/)
    {
        GUI->SetValue(instance.data());
        return true;
    }

    void RegisterExeSelectPropertyHandler()
    {
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
            &AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Handler::RegisterPropertyType, aznew ExeSelectPropertyHandler());
    }
} // namespace AzToolsFramework

