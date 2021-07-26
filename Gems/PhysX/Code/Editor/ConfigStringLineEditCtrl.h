/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef CONFIG_STRINGLINEEDIT_CTRL
#define CONFIG_STRINGLINEEDIT_CTRL

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyStringLineEditCtrl.hxx>
#include <QtWidgets/QWidget>
#include <QValidator>

#include "UniqueStringContainer.h"

#include "AzFramework/Physics/Material.h"
#endif

#pragma once

class QLineEdit;
class QPushButton;

namespace PhysX
{
    /// Validator for ConfigStringLineEditCtrl widgets
    /// Checks for unique string in a group, forbidden string values, and empty strings.
    class ConfigStringLineEditValidator
        : public QValidator
    {
        Q_OBJECT
    public:
        const static int s_qtLineEditMaxLen = 32767; ///< Max string length for Qt line edit widgets, specified in Qt documentation (https://doc.qt.io/qt-5/qlineedit.html#maxLength-prop).
        const static AZ::Crc32 s_groupStringNotUnique; ///< Identifies the default group in which strings are not kept unique.

        explicit ConfigStringLineEditValidator(QObject* parent = nullptr);

        void OnEditStart(AZ::Crc32 stringGroupId
            , const AZStd::string& stringToEdit
            , const UniqueStringContainer::StringSet& forbiddenStrings
            , int stringMaxLength
            , bool removeEditedString = true);

        void OnEditEnd(AZ::Crc32 stringGroupId, const AZStd::string& stringEditFinished);

        void RemoveUniqueString(AZ::Crc32 stringGroupId
            , const AZStd::string& stringIn);

        void fixup(QString& input) const override;
        QValidator::State validate(QString &input, int &pos) const override;

    private:
        AZ::Crc32 m_currStringGroup = s_groupStringNotUnique; ///< Group of string field undergoing edit.
        int m_currStringMaxLen = s_qtLineEditMaxLen; ///< Max length of string field undergoing edit.
        UniqueStringContainer::StringSet m_forbiddenStrings; ///< Value of string edit widget cannot be any of these strings.
        UniqueStringContainer m_uniqueStringContainer; ///< Collection of groups of unique strings. Serves for validation and fixing of string input.
    };

    /// Extends the functionality of PropertyStringLineEditCtrl for unique line edit widget values and forbidden string values.
    class ConfigStringLineEditCtrl
        : public AzToolsFramework::PropertyStringLineEditCtrl
    {
        friend class ConfigStringLineEditHandler;
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ConfigStringLineEditCtrl, AZ::SystemAllocator, 0);

        ConfigStringLineEditCtrl(QWidget* parent = nullptr
            , ConfigStringLineEditValidator* validator = nullptr);
        virtual ~ConfigStringLineEditCtrl();

        void SetForbiddenStrings(const UniqueStringContainer::StringSet& forbiddenStrings);
        void SetUniqueGroup(AZ::Crc32 uniqueGroup);
        AZStd::string Value() const;

        void setValue(AZStd::string& val) override;

    protected slots:
        void OnEditStart(bool removeEditedString = true);
        void OnEditEnd();

    protected:
        void ConnectWidgets() override;

        UniqueStringContainer::StringSet m_forbiddenStrings; ///< Value of this line edit ctrl cannot be any of these forbidden strings.
        ConfigStringLineEditValidator* m_pValidator = nullptr; ///< Validator for line edit widget.
        AZ::Crc32 m_uniqueGroup = ConfigStringLineEditValidator::s_groupStringNotUnique; ///< String group in which line edit value must remain unique.
    };

    /// Custom handler for ConfigStringLineEditCtrl
    /// Does not extend from PropertyStringLineEditHandler because base handler class is a template.
    class ConfigStringLineEditHandler
        : QObject
        , public AzToolsFramework::PropertyHandler<AZStd::string, ConfigStringLineEditCtrl>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ConfigStringLineEditHandler, AZ::SystemAllocator, 0);

        virtual AZ::u32 GetHandlerName(void) const override { return Physics::MaterialConfiguration::s_configLineEdit; }
        virtual bool IsDefaultHandler() const override { return true; }
        virtual QWidget* GetFirstInTabOrder(ConfigStringLineEditCtrl* widget) override { return widget->GetFirstInTabOrder(); }
        virtual QWidget* GetLastInTabOrder(ConfigStringLineEditCtrl* widget) override { return widget->GetLastInTabOrder(); }
        virtual void UpdateWidgetInternalTabbing(ConfigStringLineEditCtrl* widget) override { widget->UpdateTabOrder(); }

        virtual QWidget* CreateGUI(QWidget* pParent) override;
        virtual void ConsumeAttribute(ConfigStringLineEditCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
        virtual void WriteGUIValuesIntoProperty(size_t index, ConfigStringLineEditCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        virtual bool ReadValuesIntoGUI(size_t index, ConfigStringLineEditCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;

    private:
        ConfigStringLineEditValidator m_validator; ///< Validator for line edit widget.
    };

    void RegisterConfigStringLineEditHandler(); // Invoked by PhysX EditorSystemComponent to register custom line edit control.
};

#endif
