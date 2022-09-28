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
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <QWidget>
#endif

namespace AzQtComponents
{
    class BrowseEdit;
}

namespace AtomToolsFramework
{
    // Custom property widget for editing strings using a browse edit control. This allows the strings to be cleared using the browse
    // controls clear button. It also allows for custom dialogs and editors to be opened using the browse edit button.
    class PropertyStringBrowseEditCtrl : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyStringBrowseEditCtrl, AZ::SystemAllocator, 0);

        PropertyStringBrowseEditCtrl(QWidget* parent = nullptr);
        virtual void ConsumeAttribute(AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue);
        virtual void Edit();
        virtual void Clear();
        virtual void SetValue(const AZStd::string& value);
        virtual AZStd::string GetValue() const;

    Q_SIGNALS:
        void ValueChanged();

    protected:
        void SetEditButtonEnabled(bool value);
        void SetEditButtonVisible(bool value);
        AzQtComponents::BrowseEdit* m_browseEdit = nullptr;
    };

    // Generic handler for manipulating strings using the PropertyStringBrowseEditCtrl
    class PropertyStringBrowseEditHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::string, PropertyStringBrowseEditCtrl>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyStringBrowseEditHandler, AZ::SystemAllocator, 0);

        AZ::u32 GetHandlerName() const override { return AZ_CRC_CE("StringBrowseEdit"); }

        bool IsDefaultHandler() const override { return false; }

        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(
            PropertyStringBrowseEditCtrl* GUI,
            AZ::u32 attrib,
            AzToolsFramework::PropertyAttributeReader* attrValue,
            const char* debugName) override;

        void WriteGUIValuesIntoProperty(
            size_t index,
            PropertyStringBrowseEditCtrl* GUI,
            property_t& instance,
            AzToolsFramework::InstanceDataNode* node) override;

        bool ReadValuesIntoGUI(
            size_t index,
            PropertyStringBrowseEditCtrl* GUI,
            const property_t& instance,
            AzToolsFramework::InstanceDataNode* node) override;
    };

    // Property widget that interprets string data as file paths and opens a custom source file browser.
    class PropertyFilePathStringCtrl : public PropertyStringBrowseEditCtrl
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyFilePathStringCtrl, AZ::SystemAllocator, 0);

        PropertyFilePathStringCtrl(QWidget* parent = nullptr);
        void ConsumeAttribute(AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue) override;
        void Edit() override;

    private:
        AZStd::string m_title = "File";
        AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>> m_extensions;
    };

    // Property handler for PropertyFilePathStringCtrl
    class PropertyFilePathStringHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::string, PropertyFilePathStringCtrl>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyFilePathStringHandler, AZ::SystemAllocator, 0);

        AZ::u32 GetHandlerName() const override { return AZ_CRC_CE("FilePathString"); }

        bool IsDefaultHandler() const override { return false; }

        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(
            PropertyFilePathStringCtrl* GUI,
            AZ::u32 attrib,
            AzToolsFramework::PropertyAttributeReader* attrValue,
            const char* debugName) override;

        void WriteGUIValuesIntoProperty(
            size_t index,
            PropertyFilePathStringCtrl* GUI,
            property_t& instance,
            AzToolsFramework::InstanceDataNode* node) override;

        bool ReadValuesIntoGUI(
            size_t index,
            PropertyFilePathStringCtrl* GUI,
            const property_t& instance,
            AzToolsFramework::InstanceDataNode* node) override;
    };

    // Property widget that opens a separate dialog for extended editing of large strings.
    class PropertyMultiLineStringCtrl : public PropertyStringBrowseEditCtrl
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyMultiLineStringCtrl, AZ::SystemAllocator, 0);

        PropertyMultiLineStringCtrl(QWidget* parent = nullptr);
        void ConsumeAttribute(AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue) override;
        void Edit() override;
    };

    // Property handler for PropertyMultiLineStringCtrl
    class PropertyMultiLineStringHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::string, PropertyMultiLineStringCtrl>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyMultiLineStringHandler, AZ::SystemAllocator, 0);

        AZ::u32 GetHandlerName() const override { return AZ_CRC_CE("MultiLineString"); }

        bool IsDefaultHandler() const override { return false; }

        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(
            PropertyMultiLineStringCtrl* GUI,
            AZ::u32 attrib,
            AzToolsFramework::PropertyAttributeReader* attrValue,
            const char* debugName) override;

        void WriteGUIValuesIntoProperty(
            size_t index, PropertyMultiLineStringCtrl* GUI,
            property_t& instance,
            AzToolsFramework::InstanceDataNode* node) override;

        bool ReadValuesIntoGUI(
            size_t index,
            PropertyMultiLineStringCtrl* GUI,
            const property_t& instance,
            AzToolsFramework::InstanceDataNode* node) override;
    };

    // Property widget that splits an incoming stream into multiple values and allows selecting those values from another list of
    // available strings.
    class PropertyMultiSelectSplitStringCtrl : public PropertyStringBrowseEditCtrl
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyMultiSelectSplitStringCtrl, AZ::SystemAllocator, 0);

        PropertyMultiSelectSplitStringCtrl(QWidget* parent = nullptr);
        void ConsumeAttribute(AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue) override;
        void Edit() override;

        void SetValues(const AZStd::string& values);
        AZStd::string GetValues() const;

        void SetValuesVec(const AZStd::vector<AZStd::string>& values);
        AZStd::vector<AZStd::string> GetValuesVec() const;

        void SetOptions(const AZStd::string& options);
        AZStd::string GetOptions() const;

        void SetOptionsVec(const AZStd::vector<AZStd::string>& options);
        AZStd::vector<AZStd::string> GetOptionsVec() const;

    private:
        AZStd::string m_options;
    };

    // PropertyMultiSelectSplitStringCtrl handler that tokenizes strings into a list of selected and available options
    class PropertyMultiSelectSplitStringHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::string, PropertyMultiSelectSplitStringCtrl>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyMultiSelectSplitStringHandler, AZ::SystemAllocator, 0);

        AZ::u32 GetHandlerName() const override { return AZ_CRC_CE("MultiSelectSplitString"); }

        bool IsDefaultHandler() const override { return false; }

        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(
            PropertyMultiSelectSplitStringCtrl* GUI,
            AZ::u32 attrib,
            AzToolsFramework::PropertyAttributeReader* attrValue,
            const char* debugName) override;

        void WriteGUIValuesIntoProperty(
            size_t index,
            PropertyMultiSelectSplitStringCtrl* GUI,
            property_t& instance,
            AzToolsFramework::InstanceDataNode* node) override;

        bool ReadValuesIntoGUI(
            size_t index,
            PropertyMultiSelectSplitStringCtrl* GUI,
            const property_t& instance,
            AzToolsFramework::InstanceDataNode* node) override;
    };

    // PropertyMultiSelectSplitStringCtrl handler that works directly with vectors to get the list of selected and available strains
    class PropertyMultiSelectStringVectorHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::string>, PropertyMultiSelectSplitStringCtrl>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyMultiSelectStringVectorHandler, AZ::SystemAllocator, 0);

        AZ::u32 GetHandlerName() const override { return AZ_CRC_CE("MultiSelectStringVector"); }

        bool IsDefaultHandler() const override { return false; }

        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(
            PropertyMultiSelectSplitStringCtrl* GUI,
            AZ::u32 attrib,
            AzToolsFramework::PropertyAttributeReader* attrValue,
            const char* debugName) override;

        void WriteGUIValuesIntoProperty(
            size_t index,
            PropertyMultiSelectSplitStringCtrl* GUI,
            property_t& instance,
            AzToolsFramework::InstanceDataNode* node) override;

        bool ReadValuesIntoGUI(
            size_t index,
            PropertyMultiSelectSplitStringCtrl* GUI,
            const property_t& instance,
            AzToolsFramework::InstanceDataNode* node) override;
    };

    void RegisterStringBrowseEditHandler();
} // namespace AtomToolsFramework
