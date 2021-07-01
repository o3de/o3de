/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <QObject>
#include <QWidget>
#include <QFileSystemWatcher>
#include <QDateTime>
#include <QFileInfo>
#include "EMStudioConfig.h"
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/Array.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/CommandSystem/Source/ImporterCommands.h>
#endif


namespace EMStudio
{
    class EMSTUDIO_API FileManager
        : public QObject
        , public AzFramework::AssetCatalogEventBus::Handler
        , public AzToolsFramework::AssetSystemBus::Handler
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(FileManager, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK_CUSTOMWIDGETS)

    public:
        FileManager(QWidget* parent);
        ~FileManager();

        static AZStd::string GetAssetFilenameFromAssetId(const AZ::Data::AssetId& assetId);

        void SourceAssetChanged(AZStd::string filename);
        bool DidSourceAssetGetSaved(const AZStd::string& filename) const;
        void RemoveFromSavedSourceAssets(AZStd::string filename);

        static bool IsAssetLoaded(const char* filename);
        static bool IsSourceAssetLoaded(const char* filename);

        // Asset catalog EBus
        void OnCatalogAssetChanged(const AZ::Data::AssetId& assetId) override;
        void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;
        void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo) override;

        // Editor asset system API
        void SourceFileChanged(AZStd::string relativePath, AZStd::string scanFolder, AZ::TypeId sourceTypeId) override;

        // helpers
        void RelocateToAssetCacheFolder(AZStd::string& filename);
        bool RelocateToAssetSourceFolder(AZStd::string& filename);
        bool IsFileInAssetCache(const AZStd::string& filename) const;
        bool IsFileInAssetSource(const AZStd::string& filename) const;
        AZStd::vector<AZStd::string> SelectProductsOfType(AZ::Data::AssetType assetType, bool multiSelect) const;

        // actor file dialogs
        AZStd::string LoadActorFileDialog(QWidget* parent);
        AZStd::vector<AZStd::string> LoadActorsFileDialog(QWidget* parent);
        AZStd::string SaveActorFileDialog(QWidget* parent);
        void SaveActor(EMotionFX::Actor* actor);

        // workspace file dialogs
        AZStd::string LoadWorkspaceFileDialog(QWidget* parent);
        AZStd::string SaveWorkspaceFileDialog(QWidget* parent);

        // motion set file dialogs
        AZStd::string LoadMotionSetFileDialog(QWidget* parent);
        AZStd::string SaveMotionSetFileDialog(QWidget* parent);
        void SaveMotionSet(QWidget* parent, EMotionFX::MotionSet* motionSet, MCore::CommandGroup* commandGroup = nullptr);
        void SaveMotionSet(const char* filename, EMotionFX::MotionSet* motionSet, MCore::CommandGroup* commandGroup = nullptr);

        // motion file dialogs
        AZStd::string LoadMotionFileDialog(QWidget* parent);
        AZStd::vector<AZStd::string> LoadMotionsFileDialog(QWidget* parent);
        void SaveMotion(AZ::u32 motionId);

        // node mapping files
        AZStd::string LoadNodeMapFileDialog(QWidget* parent);
        AZStd::string SaveNodeMapFileDialog(QWidget* parent);

        // anim graph files
        AZStd::string LoadAnimGraphFileDialog(QWidget* parent);
        AZStd::string SaveAnimGraphFileDialog(QWidget* parent);

        // game controller preset files
        AZStd::string LoadControllerPresetFileDialog(QWidget* parent, const char* defaultFolder);
        AZStd::string SaveControllerPresetFileDialog(QWidget* parent, const char* defaultFolder);

        const AZStd::vector<AZStd::string>& GetSavedSourceAssets() const { return m_savedSourceAssets; }

    private:
        AZStd::vector<AZStd::string> m_savedSourceAssets;
        QString mLastActorFolder;
        QString mLastMotionSetFolder;
        QString mLastAnimGraphFolder;
        QString mLastWorkspaceFolder;
        QString mLastNodeMapFolder;

        bool mSkipFileChangedCheck;

        void UpdateLastUsedFolder(const char* filename, QString& outLastFolder) const;
        QString GetLastUsedFolder(const QString& lastUsedFolder) const;
    };
} // namespace EMStudio
