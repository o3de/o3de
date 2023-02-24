/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "FunctorValidator.h"

#include <AzToolsFramework/UI/PropertyEditor/PropertyStringLineEditCtrl.hxx>
#endif

namespace ProjectSettingsTool
{
    // Forward Declare
    class ValidationHandler;

    class PropertyFuncValLineEditCtrl
        : public AzToolsFramework::PropertyStringLineEditCtrl
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(PropertyFuncValLineEditCtrl, AZ::SystemAllocator)
        PropertyFuncValLineEditCtrl(QWidget* pParent = nullptr);

        virtual QString GetValue() const;
        // Sets value programmtically and triggers validation
        virtual void SetValue(const QString& value);
        // Sets value as if user set it
        void SetValueUser(const QString& value);
        // Returns pointer to the validator used
        FunctorValidator* GetValidator();
        // Sets the validator for the lineedit
        void SetValidator(FunctorValidator* validator);
        // Sets the validator for the linedit
        void SetValidator(FunctorValidator::FunctorType validator);
        // Returns false if invalid and returns shows error as tooltip
        bool ValidateAndShowErrors();
        // Forces the values to up validated and style updated
        void ForceValidate();

        virtual void ConsumeAttribute(AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName);

    signals:
        void ValueChangedByUser();

    protected:
        // Keeps track of the validator so no const_casts must be done
        FunctorValidator* m_validator;
    };

    class PropertyFuncValLineEditHandler
        : public AzToolsFramework::PropertyHandler <AZStd::string, PropertyFuncValLineEditCtrl>
    {
        AZ_CLASS_ALLOCATOR(PropertyFuncValLineEditHandler, AZ::SystemAllocator);

    public:
        PropertyFuncValLineEditHandler(ValidationHandler* valHdlr);

        AZ::u32 GetHandlerName(void) const override;
        // Need to unregister ourselves
        bool AutoDelete() const override { return false; }

        QWidget* CreateGUI(QWidget* pParent) override;
        void ConsumeAttribute(PropertyFuncValLineEditCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, PropertyFuncValLineEditCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, PropertyFuncValLineEditCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;
        static PropertyFuncValLineEditHandler* Register(ValidationHandler* valHdlr);

    private:
        ValidationHandler* m_validationHandler;
    };
} // namespace ProjectSettingsTool
