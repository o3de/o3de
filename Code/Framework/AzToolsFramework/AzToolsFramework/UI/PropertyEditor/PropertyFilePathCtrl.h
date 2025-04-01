/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <QString>
#include <QWidget>

#include "PropertyEditorAPI.h"
#endif

namespace AzQtComponents
{
    class BrowseEdit;
}

namespace AzToolsFramework
{
    //! Helper method to retrieve an absolute path given a relative source path
    AZ::IO::Path GetAbsolutePathFromRelativePath(const AZ::IO::Path& relativePath);

    //! A control for representing an AZ::IO::Path field with
    //! a BrowseEdit whose popup button triggers a native
    //! file dialog.
    class PropertyFilePathCtrl : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyFilePathCtrl, AZ::SystemAllocator);

        PropertyFilePathCtrl(QWidget* parent = nullptr);

        const AZ::IO::Path& GetFilePath() const;
        void SetFilter(const QString& filter);
        void SetDefaultFileName(AZ::IO::Path fileName);

    Q_SIGNALS:
        void FilePathChanged();

    public Q_SLOTS:
        void SetFilePath(const AZ::IO::Path& filePath);

    private:
        AzQtComponents::BrowseEdit* m_browseEdit = nullptr;
        AZ::IO::Path m_currentFilePath;
        QString m_filter;
        AZ::IO::Path m_defaultFileName;

        void OnOpenButtonClicked();
        QString GetPreselectedFilePath() const;
        void HandleNewFilePathSelected(const QString& newFilePath);
        void OnClearButtonClicked();
    };

    class PropertyFilePathHandler
        : public QObject
        , public PropertyHandler<AZ::IO::Path, PropertyFilePathCtrl>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyFilePathHandler, AZ::SystemAllocator);

        AZ::u32 GetHandlerName() const override;

        // Override to be true so that the user to be able to either specify no handler or "Default"
        // and this handler will be found based on type match (AZ::IO::Path)
        bool IsDefaultHandler() const override;

        QWidget* CreateGUI(QWidget* parent) override;
        void ConsumeAttribute(
            PropertyFilePathCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, PropertyFilePathCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, PropertyFilePathCtrl* GUI, const property_t& instance, InstanceDataNode* node) override;
    };

    void RegisterFilePathHandler();
} // namespace AzToolsFramework
