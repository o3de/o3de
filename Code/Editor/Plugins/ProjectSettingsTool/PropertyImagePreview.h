/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "PropertyFileSelect.h"

#include <AzCore/base.h>
#include <AzCore/std/containers/unordered_map.h>
#include <QLabel>
#endif

// Forward Declaration
class QValidator;

namespace ProjectSettingsTool
{
    // Forward Declaration
    class ValidationHandler;

    // Used to select a png image from a file dialog then display it
    class PropertyImagePreviewCtrl
        : public PropertyFuncValBrowseEditCtrl
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(PropertyImagePreviewCtrl, AZ::SystemAllocator)
        PropertyImagePreviewCtrl(QWidget* parent = nullptr);

        // Sets path for the image preview
        void SetValue(const QString& path) override;
        // Returns path for the default image preview
        const QString& DefaultImagePath() const;
        // Sets path for the default image preview
        void SetDefaultImagePath(const QString& newPath);
        // Sets the default image preview control to use for image previews
        void SetDefaultImagePreview(PropertyImagePreviewCtrl* imageSelect);
        // Returns current default image select control pointer
        PropertyImagePreviewCtrl* DefaultImagePreview() const;
        // Add a specific image override to the default image selects validator
        void AddOverrideToValidator(PropertyImagePreviewCtrl* preview);
        // Loads a preview of the image at the current path or default if path isEmpty
        void LoadPreview();
        // Upgrades the Validator to a DefaultImageValidator
        void UpgradeToDefaultValidator();

        virtual void ConsumeAttribute(AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

    protected:
        AZ_DISABLE_COPY_MOVE(PropertyImagePreviewCtrl);

        void emitSelectClicked();

        // Default image select to use
        PropertyImagePreviewCtrl* m_defaultImagePreview;
        // Full Path to default image preview
        QString m_defaultPath;
        // Displays the image preview
        QLabel* m_preview;
    };

    class PropertyImagePreviewHandler
        : public AzToolsFramework::PropertyHandler<AZStd::string, PropertyImagePreviewCtrl>
    {
        AZ_CLASS_ALLOCATOR(PropertyImagePreviewHandler, AZ::SystemAllocator);

    public:
        PropertyImagePreviewHandler(ValidationHandler* valHdlr);

        AZ::u32 GetHandlerName(void) const override;
        // Need to unregister ourselves
        bool AutoDelete() const override { return false; }

        QWidget* CreateGUI(QWidget* pParent) override;
        void ConsumeAttribute(PropertyImagePreviewCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, PropertyImagePreviewCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, PropertyImagePreviewCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;
        static PropertyImagePreviewHandler* Register(ValidationHandler* valHdlr);

    private:
        AZStd::unordered_map<AZStd::string, PropertyImagePreviewCtrl*> m_identToCtrl;
        ValidationHandler* m_validationHandler;
    };
} // namespace ProjectSettingsTool

