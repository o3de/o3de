/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

#include <QString>
#include <QStringList>

namespace AzToolsFramework
{
    class LocalFileSCComponent
        : public AZ::Component
        , private SourceControlCommandBus::Handler
        , private SourceControlConnectionRequestBus::Handler
    {
        friend class PerforceComponent;

    public:
        AZ_COMPONENT(LocalFileSCComponent, "{5AE6565F-046D-42F4-8E95-77C163A98420}");

        // Resolves an absolute wildcard path to a list of all matching files
        // Note that wildcards will not match across a directory level.  EX:  C:\some*\file.txt will NOT match C:\something\folder\file.txt.  It WILL match C:\something\file.txt
        static QStringList GetFiles(QString absolutePathQuery);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component overrides
        void Init() override {}
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////
    private:

        static void Reflect(AZ::ReflectContext* context);

        // Takes a path and splits it into 3 parts: Root folder, wildcard file/folder entry, and the remaining path
        // EX: C:/some/new*folder/file.txt -> C:/some/, new*folder, file.txt
        static bool SplitWildcardPath(QString path, QString& root, QString& wildcardEntry, QString& remaining);

        // Takes a wildcard path and adds all entries that match the first wildcard entry to result
        // EX: C:/some/new*folder/file*.txt -> { C:/some/new1folder/file*.txt, C:/some/new2folder/file*.txt, etc }
        // Note that multiple wildcards at one directory level will be resolved at the same time (EX: C:/some*folder*name/file.txt will resolve in 1 pass)
        // Returns true if there are more levels (wildcards) to resolve
        static bool ResolveOneWildcardLevel(QString search, QStringList& result);

        // Takes an absolute path to a file, the absolute wildcard path used to look up that file, and an absolute wildcard destination path to move that file to
        // Returns the destination path with the wildcards resolved
        static AZStd::string ResolveWildcardDestination(AZStd::string_view absFile, AZStd::string_view absSearch, AZStd::string destination);

        //////////////////////////////////////////////////////////////////////////
        // SourceControlCommandBus::Handler overrides
        void GetFileInfo(const char* fullFilePath, const SourceControlResponseCallback& respCallback) override;
        void GetBulkFileInfo(const AZStd::unordered_set<AZStd::string>& fullFilePaths, const SourceControlResponseCallbackBulk& respCallback) override;
        void RequestEdit(const char* fullFilePath, bool allowMultiCheckout, const SourceControlResponseCallback& respCallback) override;
        void RequestEditBulk(const AZStd::unordered_set<AZStd::string>& fullFilePaths, bool allowMultiCheckout, const SourceControlResponseCallbackBulk& respCallback) override;
        void RequestDelete(const char* fullFilePath, const SourceControlResponseCallback& respCallback) override;
        void RequestDeleteExtended(const char* fullFilePath, bool skipReadOnly, const SourceControlResponseCallback& respCallback) override;
        void RequestDeleteBulk(const char* fullFilePath, const SourceControlResponseCallbackBulk& respCallback) override;
        void RequestDeleteBulkExtended(const char* fullFilePath, bool skipReadOnly, const SourceControlResponseCallbackBulk& respCallback) override;
        void RequestRevert(const char* fullFilePath, const SourceControlResponseCallback& respCallback) override;
        void RequestLatest(const char* fullFilePath, const SourceControlResponseCallback& respCallback) override;
        void RequestRename(const char* sourcePathFull, const char* destPathFull, const SourceControlResponseCallback& respCallback) override;
        void RequestRenameExtended(const char* sourcePathFull, const char* destPathFull, bool skipReadOnly, const SourceControlResponseCallback& respCallback) override;
        void RequestRenameBulk(const char* sourcePathFull, const char* destPathFull, const SourceControlResponseCallbackBulk& respCallback) override;
        void RequestRenameBulkExtended(const char* sourcePathFull, const char* destPathFull, bool skipReadOnly, const SourceControlResponseCallbackBulk& respCallback) override;
        //////////////////////////////////////////////////////////////////////////

        
        //////////////////////////////////////////////////////////////////////////
        // SourceControlConnectionRequestBus::Handler overrides
        void EnableSourceControl(bool) override {}
        bool IsActive() const override { return false; }
        void EnableTrust(bool, AZStd::string) override {}
        void SetConnectionSetting(const char*, const char*, const SourceControlSettingCallback&) override {}
        void GetConnectionSetting(const char*, const SourceControlSettingCallback&) override {}
        //////////////////////////////////////////////////////////////////////////
    };
} // namespace AzToolsFramework
