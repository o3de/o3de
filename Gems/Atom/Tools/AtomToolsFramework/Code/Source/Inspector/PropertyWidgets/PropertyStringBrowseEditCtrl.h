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
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/set.h>
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
        AZ_CLASS_ALLOCATOR(PropertyStringBrowseEditCtrl, AZ::SystemAllocator);

        PropertyStringBrowseEditCtrl(QWidget* parent = nullptr);
        virtual void ConsumeAttribute(AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue);
        virtual void EditValue();
        virtual void ClearValue();
        virtual void SetValue(const AZStd::string& value);
        virtual AZStd::string GetValue() const;

    protected:
        virtual void OnValueChanged();
        virtual void OnTextEditingFinished();
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
        AZ_CLASS_ALLOCATOR(PropertyStringBrowseEditHandler, AZ::SystemAllocator);

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
    class PropertyStringFilePathCtrl : public PropertyStringBrowseEditCtrl
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyStringFilePathCtrl, AZ::SystemAllocator);

        PropertyStringFilePathCtrl(QWidget* parent = nullptr);
        void ConsumeAttribute(AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue) override;
        void EditValue() override;

    private:
        AZStd::string m_title = "File";
        AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>> m_extensions;
    };

    // Property handler for PropertyStringFilePathCtrl
    class PropertyStringFilePathHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::string, PropertyStringFilePathCtrl>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyStringFilePathHandler, AZ::SystemAllocator);

        AZ::u32 GetHandlerName() const override { return AZ_CRC_CE("StringFilePath"); }

        bool IsDefaultHandler() const override { return false; }

        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(
            PropertyStringFilePathCtrl* GUI,
            AZ::u32 attrib,
            AzToolsFramework::PropertyAttributeReader* attrValue,
            const char* debugName) override;

        void WriteGUIValuesIntoProperty(
            size_t index,
            PropertyStringFilePathCtrl* GUI,
            property_t& instance,
            AzToolsFramework::InstanceDataNode* node) override;

        bool ReadValuesIntoGUI(
            size_t index,
            PropertyStringFilePathCtrl* GUI,
            const property_t& instance,
            AzToolsFramework::InstanceDataNode* node) override;
    };

    // Property widget that opens a separate dialog for extended editing of large strings.
    class PropertyMultilineStringDialogCtrl : public PropertyStringBrowseEditCtrl
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyMultilineStringDialogCtrl, AZ::SystemAllocator);

        PropertyMultilineStringDialogCtrl(QWidget* parent = nullptr);
        void ConsumeAttribute(AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue) override;
        void EditValue() override;
    };

    // Property handler for PropertyMultilineStringDialogCtrl
    class PropertyMultilineStringDialogHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::string, PropertyMultilineStringDialogCtrl>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyMultilineStringDialogHandler, AZ::SystemAllocator);

        AZ::u32 GetHandlerName() const override { return AZ_CRC_CE("MultilineStringDialog"); }

        bool IsDefaultHandler() const override { return false; }

        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(
            PropertyMultilineStringDialogCtrl* GUI,
            AZ::u32 attrib,
            AzToolsFramework::PropertyAttributeReader* attrValue,
            const char* debugName) override;

        void WriteGUIValuesIntoProperty(
            size_t index, PropertyMultilineStringDialogCtrl* GUI,
            property_t& instance,
            AzToolsFramework::InstanceDataNode* node) override;

        bool ReadValuesIntoGUI(
            size_t index,
            PropertyMultilineStringDialogCtrl* GUI,
            const property_t& instance,
            AzToolsFramework::InstanceDataNode* node) override;
    };

    // Property widget that splits an incoming stream into multiple values and allows selecting those values from another list of
    // available strings.
    class PropertyMultiStringSelectCtrl : public PropertyStringBrowseEditCtrl
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyMultiStringSelectCtrl, AZ::SystemAllocator);

        PropertyMultiStringSelectCtrl(QWidget* parent = nullptr);
        void ConsumeAttribute(AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue) override;
        void EditValue() override;

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
        bool m_multiSelect = true;
        AZStd::string m_delimitersForSplit = ";:, \t\r\n\\/|";
        AZStd::string m_delimitersForJoin = ", ";
    };

    // PropertyMultiStringSelectCtrl handler that tokenizes strings into a list of selected and available options
    class PropertyMultiStringSelectDelimitedHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::string, PropertyMultiStringSelectCtrl>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyMultiStringSelectDelimitedHandler, AZ::SystemAllocator);

        AZ::u32 GetHandlerName() const override { return AZ_CRC_CE("MultiStringSelectDelimited"); }

        bool IsDefaultHandler() const override { return false; }

        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(
            PropertyMultiStringSelectCtrl* GUI,
            AZ::u32 attrib,
            AzToolsFramework::PropertyAttributeReader* attrValue,
            const char* debugName) override;

        void WriteGUIValuesIntoProperty(
            size_t index,
            PropertyMultiStringSelectCtrl* GUI,
            property_t& instance,
            AzToolsFramework::InstanceDataNode* node) override;

        bool ReadValuesIntoGUI(
            size_t index,
            PropertyMultiStringSelectCtrl* GUI,
            const property_t& instance,
            AzToolsFramework::InstanceDataNode* node) override;
    };

    // PropertyMultiStringSelectCtrl handler that works directly with vectors to get the list of selected and available strains
    class PropertyMultiStringSelectVectorHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::string>, PropertyMultiStringSelectCtrl>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyMultiStringSelectVectorHandler, AZ::SystemAllocator);

        AZ::u32 GetHandlerName() const override { return AZ_CRC_CE("MultiStringSelectVector"); }

        bool IsDefaultHandler() const override { return false; }

        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(
            PropertyMultiStringSelectCtrl* GUI,
            AZ::u32 attrib,
            AzToolsFramework::PropertyAttributeReader* attrValue,
            const char* debugName) override;

        void WriteGUIValuesIntoProperty(
            size_t index,
            PropertyMultiStringSelectCtrl* GUI,
            property_t& instance,
            AzToolsFramework::InstanceDataNode* node) override;

        bool ReadValuesIntoGUI(
            size_t index,
            PropertyMultiStringSelectCtrl* GUI,
            const property_t& instance,
            AzToolsFramework::InstanceDataNode* node) override;
    };

    // PropertyMultiStringSelectCtrl handler that works directly with sets to get the list of selected and available strains
    class PropertyMultiStringSelectSetHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::set<AZStd::string>, PropertyMultiStringSelectCtrl>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyMultiStringSelectSetHandler, AZ::SystemAllocator);

        AZ::u32 GetHandlerName() const override { return AZ_CRC_CE("MultiStringSelectSet"); }

        bool IsDefaultHandler() const override { return false; }

        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(
            PropertyMultiStringSelectCtrl* GUI,
            AZ::u32 attrib,
            AzToolsFramework::PropertyAttributeReader* attrValue,
            const char* debugName) override;

        void WriteGUIValuesIntoProperty(
            size_t index,
            PropertyMultiStringSelectCtrl* GUI,
            property_t& instance,
            AzToolsFramework::InstanceDataNode* node) override;

        bool ReadValuesIntoGUI(
            size_t index,
            PropertyMultiStringSelectCtrl* GUI,
            const property_t& instance,
            AzToolsFramework::InstanceDataNode* node) override;
    };

    void RegisterStringBrowseEditHandler();
} // namespace AtomToolsFramework
