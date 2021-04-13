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
 
 #include "EditorDefs.h"
 #include "ExeSelectPropertyHandler.h"
 
 #include <AzToolsFramework/UI/PropertyEditor/PropertyQTConstants.h>
 
 #include <QFileDialog>
 #include <QLayout>
 #include <QLineEdit>
 
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
     connect(m_browseEdit, &AzQtComponents::BrowseEdit::textChanged, this, [this](const QString& /*text*/)
         {
             EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, this);
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
     : AzToolsFramework::PropertyHandler <AZStd::string, ExeSelectPropertyCtrl>()
 {}
 
 AZ::u32 ExeSelectPropertyHandler::GetHandlerName(void) const
 {
     return ExeSelectBrowseEdit;
 }
 
 QWidget* ExeSelectPropertyHandler::CreateGUI(QWidget* pParent)
 {
     ExeSelectPropertyCtrl* ctrl = aznew ExeSelectPropertyCtrl(pParent);
     return ctrl;
 }
 
 void ExeSelectPropertyHandler::WriteGUIValuesIntoProperty(size_t /*index*/, ExeSelectPropertyCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* /*node*/)
 {
     instance = GUI->GetValue().toUtf8().data();
 }
 
 bool ExeSelectPropertyHandler::ReadValuesIntoGUI(size_t /*index*/, ExeSelectPropertyCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* /*node*/)
 {
     GUI->SetValue(instance.data());
     return true;
 }
 
 ExeSelectPropertyHandler* ExeSelectPropertyHandler::Register()
 {
     ExeSelectPropertyHandler* handler = aznew ExeSelectPropertyHandler();
     AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
         &AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Handler::RegisterPropertyType,
         handler);
     return handler;
 }
 
 #include <ExeSelectPropertyHandler.moc>