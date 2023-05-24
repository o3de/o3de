/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>

#include <AzQtComponents/Components/Widgets/BrowseEdit.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <QWidget>
#endif

namespace AzToolsFramework
{
    class ExeSelectPropertyCtrl : public QWidget
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(ExeSelectPropertyCtrl, AZ::SystemAllocator);

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
        , public AzToolsFramework::PropertyHandler<AZStd::string, ExeSelectPropertyCtrl>
    {
    public:
        AZ_CLASS_ALLOCATOR(ExeSelectPropertyHandler, AZ::SystemAllocator);

        ExeSelectPropertyHandler();

        AZ::u32 GetHandlerName(void) const override;
        bool AutoDelete() const override;

        QWidget* CreateGUI(QWidget* pParent) override;
        void WriteGUIValuesIntoProperty(
            size_t index, ExeSelectPropertyCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(
            size_t index, ExeSelectPropertyCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };

    void RegisterExeSelectPropertyHandler();
} // namespace AzToolsFramework
