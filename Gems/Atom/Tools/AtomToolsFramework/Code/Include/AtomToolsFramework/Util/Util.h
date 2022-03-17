/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/PlatformDef.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/any.h>
#include <AzCore/std/containers/vector.h>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QFileInfo>
#include <QRegExp>
#include <QString>
#include <QStringList>
AZ_POP_DISABLE_WARNING

class QImage;
class QWidget;

namespace AtomToolsFramework
{
    using LoadImageAsyncCallback = AZStd::function<void(const QImage&)>;
    void LoadImageAsync(const AZStd::string& path, LoadImageAsyncCallback callback);

    QWidget* GetToolMainWindow();
    AZStd::string GetDisplayNameFromPath(const AZStd::string& path);
    AZStd::string GetSaveFilePath(const AZStd::string& initialPath, const AZStd::string& title = "Document");
    AZStd::vector<AZStd::string> GetOpenFilePaths(const QRegExp& filter, const AZStd::string& title = "Document");
    AZStd::string GetUniqueFilePath(const AZStd::string& initialPath);
    AZStd::string GetUniqueDefaultSaveFilePath(const AZStd::string& extension);
    AZStd::string GetUniqueDuplicateFilePath(const AZStd::string& initialPath);
    bool ValidateDocumentPath(AZStd::string& path);
    bool LaunchTool(const QString& baseName, const QStringList& arguments);

    //! Generate a file path that is relative to either the source asset root or the export path
    //! @param exportPath absolute path of the file being saved
    //! @param referencePath absolute path of a file that will be treated as an external reference
    //! @param relativeToExportPath specifies if the path is relative to the source asset root or the export path
    AZStd::string GetExteralReferencePath(
        const AZStd::string& exportPath, const AZStd::string& referencePath, const bool relativeToExportPath = false);

    //! Traverse up the instance data hierarchy to find a node containing the corresponding type
    template<typename T>
    const T* FindAncestorInstanceDataNodeByType(const AzToolsFramework::InstanceDataNode* pNode)
    {
        // Traverse up the hierarchy from the input node to search for an instance matching the specified type
        for (const auto* currentNode = pNode; currentNode; currentNode = currentNode->GetParent())
        {
            const auto* context = currentNode->GetSerializeContext();
            const auto* classData = currentNode->GetClassMetadata();
            if (context && classData)
            {
                if (context->CanDowncast(classData->m_typeId, azrtti_typeid<T>(), classData->m_azRtti, nullptr))
                {
                    return static_cast<const T*>(currentNode->FirstInstance());
                }
            }
        }

        return nullptr;
    }

    template<typename T>
    T GetSettingsValue(AZStd::string_view path, const T& defaultValue = {})
    {
        T result;
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        return (settingsRegistry && settingsRegistry->Get(result, path)) ? result : defaultValue;
    }

    template<typename T>
    bool SetSettingsValue(AZStd::string_view path, const T& value)
    {
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        return settingsRegistry && settingsRegistry->Set(path, value);
    }

    template<typename T>
    T GetSettingsObject(AZStd::string_view path, const T& defaultValue = {})
    {
        T result;
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        return (settingsRegistry && settingsRegistry->GetObject<T>(result, path)) ? result : defaultValue;
    }

    template<typename T>
    bool SetSettingsObject(AZStd::string_view path, const T& value)
    {
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        return settingsRegistry && settingsRegistry->SetObject<T>(path, value);
    }

    bool SaveSettingsToFile(const AZ::IO::FixedMaxPath& savePath, const AZStd::vector<AZStd::string>& filters);
} // namespace AtomToolsFramework
