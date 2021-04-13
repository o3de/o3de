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
 
 #pragma once
 
 #if !defined(Q_MOC_RUN)
 #include <QWidget>
 #include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
 #include <AzQtComponents/Components/Widgets/BrowseEdit.h>
 #endif
 
 static const AZ::Crc32 ExeSelectBrowseEdit = AZ_CRC("ExeSelect", 0xacf67241);
 
 class ExeSelectPropertyCtrl
     : public QWidget
 {
     Q_OBJECT
 
 public:
     AZ_CLASS_ALLOCATOR(ExeSelectPropertyCtrl, AZ::SystemAllocator, 0);
 
     ExeSelectPropertyCtrl(QWidget* pParent = nullptr);
 
     void SelectExe();
 
     virtual QString GetValue() const;
     // Sets value programmatically.
     virtual void SetValue(const QString& value);
     // Sets value as if user set it
     void SetValueUser(const QString& value);
 
     // Returns a pointer to the BrowseEdit object.
     AzQtComponents::BrowseEdit* browseEdit();
 
 signals:
     void ValueChangedByUser();
 
 protected:
 
     AzQtComponents::BrowseEdit* m_browseEdit = nullptr;
 };
 
 class ExeSelectPropertyHandler
     : QObject
     , public AzToolsFramework::PropertyHandler <AZStd::string, ExeSelectPropertyCtrl>
 {
     AZ_CLASS_ALLOCATOR(ExeSelectPropertyHandler, AZ::SystemAllocator, 0);
 
 public:
     ExeSelectPropertyHandler();
 
     AZ::u32 GetHandlerName(void) const override;
     // Need to unregister ourselves
     bool AutoDelete() const override { return false; }
 
     QWidget* CreateGUI(QWidget* pParent) override;
     void WriteGUIValuesIntoProperty(size_t index, ExeSelectPropertyCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
     bool ReadValuesIntoGUI(size_t index, ExeSelectPropertyCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;
     static ExeSelectPropertyHandler* Register();
 
 };