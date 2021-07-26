/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzToolsFramework/SourceControl/LocalFileSCComponent.h>

#include <AzCore/Component/TickBus.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/regex.h>
#include <AzCore/StringFunc/StringFunc.h>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QFileInfo::d_ptr': class 'QSharedDataPointer<QFileInfoPrivate>' needs to have dll-interface to be used by clients of class 'QFileInfo'
#include <QDir>
#include <QDirIterator>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    void RefreshInfoFromFileSystem(SourceControlFileInfo& fileInfo)
    {
        fileInfo.m_flags = 0;
        if (!AZ::IO::SystemFile::Exists(fileInfo.m_filePath.c_str()))
        {
            fileInfo.m_flags |= SCF_Writeable;  // Non-existent files are not read only
        }
        else
        {
            fileInfo.m_flags |= SCF_Tracked;
            if (AZ::IO::SystemFile::IsWritable(fileInfo.m_filePath.c_str()))
            {
                fileInfo.m_flags |= SCF_Writeable | SCF_OpenByUser;
            }
        }

        fileInfo.m_status = SCS_OpSuccess;
    }

    void RemoveReadOnly(const SourceControlFileInfo& fileInfo)
    {
        if (!fileInfo.HasFlag(SCF_Writeable) && fileInfo.HasFlag(SCF_Tracked))
        {
            AZ::IO::SystemFile::SetWritable(fileInfo.m_filePath.c_str(), true);
        }
    }

    void LocalFileSCComponent::Activate()
    {
        SourceControlConnectionRequestBus::Handler::BusConnect();
        SourceControlCommandBus::Handler::BusConnect();
    }

    void LocalFileSCComponent::Deactivate()
    {
        SourceControlCommandBus::Handler::BusDisconnect();
        SourceControlConnectionRequestBus::Handler::BusDisconnect();
    }

    void LocalFileSCComponent::GetFileInfo(const char* fullFilePath, const SourceControlResponseCallback& respCallback)
    {
        SourceControlFileInfo fileInfo(fullFilePath);
        auto job = AZ::CreateJobFunction([fileInfo, respCallback]() mutable
                {
                    RefreshInfoFromFileSystem(fileInfo);
                    AZ::TickBus::QueueFunction(respCallback, fileInfo.CompareStatus(SCS_OpSuccess), fileInfo);
                }, true);
        job->Start();
    }

    void LocalFileSCComponent::GetBulkFileInfo(const AZStd::unordered_set<AZStd::string>& fullFilePaths, const SourceControlResponseCallbackBulk& respCallback)
    {
        auto job = AZ::CreateJobFunction([fullFilePaths, respCallback]() mutable
            {
                AZStd::vector<SourceControlFileInfo> fileInfo;

                for (const AZStd::string& fullFilePath : fullFilePaths)
                {
                    for (QString file : GetFiles(fullFilePath.c_str()))
                    {
                        fileInfo.push_back();
                        fileInfo.back().m_filePath = file.toUtf8().constData();

                        RefreshInfoFromFileSystem(fileInfo.back());
                    }
                }

                AZ::TickBus::QueueFunction(respCallback, true, fileInfo);
            }, true);
        job->Start();
    }

    void LocalFileSCComponent::RequestEdit(const char* fullFilePath, bool /*allowMultiCheckout*/, const SourceControlResponseCallback& respCallback)
    {
        SourceControlFileInfo fileInfo(fullFilePath);
        auto job = AZ::CreateJobFunction([fileInfo, respCallback]() mutable
                {
                    RefreshInfoFromFileSystem(fileInfo);
                    RemoveReadOnly(fileInfo);
                    RefreshInfoFromFileSystem(fileInfo);

                    // As a quality of life improvement for our users, we want request edit
                    // to report success in the case where a file doesn't exist.  We do this so
                    // developers can always call RequestEdit before a save operation; instead of:
                    //   File Exists            --> RequestEdit, then SaveOperation
                    //   File Does not Exist    --> SaveOperation, then RequestEdit
                    AZ::TickBus::QueueFunction(respCallback, fileInfo.HasFlag(SCF_Writeable), fileInfo);
                }, true);
        job->Start();
    }

    void LocalFileSCComponent::RequestEditBulk(const AZStd::unordered_set<AZStd::string>& fullFilePaths, bool /*allowMultiCheckout*/, const SourceControlResponseCallbackBulk& respCallback)
    {
        auto job = AZ::CreateJobFunction([fullFilePaths, respCallback]() mutable
            {
                AZStd::vector<SourceControlFileInfo> info;

                for (const auto& filePath : fullFilePaths)
                {
                    info.push_back();
                    auto& fileInfo = info.back();

                    fileInfo.m_filePath = filePath;
                    RefreshInfoFromFileSystem(fileInfo);
                    RemoveReadOnly(fileInfo);
                    RefreshInfoFromFileSystem(fileInfo);
                }

                // As a quality of life improvement for our users, we want request edit
                // to report success in the case where a file doesn't exist.  We do this so
                // developers can always call RequestEdit before a save operation; instead of:
                //   File Exists            --> RequestEdit, then SaveOperation
                //   File Does not Exist    --> SaveOperation, then RequestEdit
                AZ::TickBus::QueueFunction(respCallback, true, info);
            }, true);
        job->Start();
    }

    void LocalFileSCComponent::RequestDelete(const char* fullFilePath, const SourceControlResponseCallback& respCallback)
    {
        RequestDeleteExtended(fullFilePath, false, respCallback);
    }

    void LocalFileSCComponent::RequestDeleteExtended(const char* fullFilePath, bool skipReadOnly, const SourceControlResponseCallback& respCallback)
    {
        SourceControlFileInfo fileInfo(fullFilePath);
        auto job = AZ::CreateJobFunction([fileInfo, skipReadOnly, respCallback]() mutable
            {
                RefreshInfoFromFileSystem(fileInfo);

                if (!skipReadOnly)
                {
                    RemoveReadOnly(fileInfo);
                }

                auto succeeded = AZ::IO::SystemFile::Delete(fileInfo.m_filePath.c_str());
                RefreshInfoFromFileSystem(fileInfo);
                AZ::TickBus::QueueFunction(respCallback, succeeded, fileInfo);
            }, true);
        job->Start();
    }

    bool LocalFileSCComponent::SplitWildcardPath(QString path, QString& root, QString& wildcardEntry, QString& remaining)
    {
        static constexpr char WildcardCharacter = '*';
        static constexpr char RecursiveWildcard[] = "...";

        int firstWildcardIndex = path.indexOf(WildcardCharacter);

        if(firstWildcardIndex < 0)
        {
            firstWildcardIndex = path.indexOf(RecursiveWildcard);
        }

        if(firstWildcardIndex >= 0)
        {
            int lastSlashBeforeWildcard = path.lastIndexOf(AZ_CORRECT_FILESYSTEM_SEPARATOR, firstWildcardIndex);

            root = path.left(lastSlashBeforeWildcard + 1); // Include the separator
            wildcardEntry = path.mid(lastSlashBeforeWildcard + 1); // Skip the separator
            remaining = "";

            int nextSlashAfterWildcard = wildcardEntry.indexOf(AZ_CORRECT_FILESYSTEM_SEPARATOR);

            if(nextSlashAfterWildcard >= 0)
            {
                remaining = wildcardEntry.mid(nextSlashAfterWildcard + 1); // Skip the separator
                wildcardEntry = wildcardEntry.left(nextSlashAfterWildcard); // Skip the separator
            }

            return true;
        }

        return false;
    }

    QStringList RecurseAllFiles(QString path)
    {
        QDirIterator itr(path, QDir::Files | QDir::NoSymLinks | QDir::Hidden, QDirIterator::Subdirectories);
        QStringList result;

        while(itr.hasNext())
        {
            result += itr.next();
        }

        return result;
    }

    bool LocalFileSCComponent::ResolveOneWildcardLevel(QString search, QStringList& result)
    {
        QString root;
        QString searchEntry;
        QString remaining;

        if(SplitWildcardPath(search, root, searchEntry, remaining))
        {
            bool recursive = searchEntry.endsWith("...");
            searchEntry = searchEntry.replace("...", "*");

            QDir searchRoot(root);

            QStringList nameFilters;
            nameFilters << searchEntry;
            QDir::Filters dirFilters = remaining.isEmpty() ? QDir::Files : QDir::Dirs;

            bool searchEverything = recursive && remaining.isEmpty();

            QStringList files = searchRoot.entryList(nameFilters,  dirFilters | QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Hidden);

            for (QString& file : files)
            {
                file = root + file;

                if (!remaining.isEmpty())
                {
                    file += AZ_CORRECT_FILESYSTEM_SEPARATOR + remaining;
                }
            }

            result += files;

            if(searchEverything)
            {
                for (QString& file : searchRoot.entryList(nameFilters, QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Hidden))
                {
                    result += RecurseAllFiles(root + file);
                }
            }

            return !remaining.isEmpty();
        }

        if(AZ::IO::SystemFile::Exists(search.toUtf8().constData()))
        {
            result = QStringList{ search };
        }
        else
        {
            result = QStringList{};
        }
        
        return !remaining.isEmpty();
    }

    QStringList LocalFileSCComponent::GetFiles(QString absolutePathQuery)
    {
        AZStd::string azPath = absolutePathQuery.toUtf8().constData();

        AZ::StringFunc::Replace(azPath, AZ_WRONG_FILESYSTEM_SEPARATOR, AZ_CORRECT_FILESYSTEM_SEPARATOR);

        QString remaining;
        QStringList entries;

        if(!ResolveOneWildcardLevel(azPath.c_str(), entries))
        {
            return entries;
        }

        QStringList results;

        for (QString entry : entries)
        {
             results += GetFiles(entry);
        }

        return results;
    }

    AZStd::string LocalFileSCComponent::ResolveWildcardDestination(AZStd::string_view absFile, AZStd::string_view absSearch, AZStd::string destination)
    {
        AZStd::string searchAsRegex = absSearch;
        AZ::StringFunc::Replace(searchAsRegex, AZ_WRONG_FILESYSTEM_SEPARATOR, AZ_CORRECT_FILESYSTEM_SEPARATOR);
        AZ::StringFunc::Replace(destination, AZ_WRONG_FILESYSTEM_SEPARATOR, AZ_CORRECT_FILESYSTEM_SEPARATOR);
        AZ::StringFunc::Replace(searchAsRegex, "...", "*");
        AZ::StringFunc::Replace(destination, "...", "*");

        AZStd::regex specialCharacters(R"([\\.?^$+(){}[\]-])");

        // Escape the regex special characters
        searchAsRegex = AZStd::regex_replace(searchAsRegex, specialCharacters, R"(\$0)");
        // Replace * with .*
        searchAsRegex = AZStd::regex_replace(searchAsRegex, AZStd::regex(R"(\*)"), R"((.*))");

        AZStd::smatch result;

        // Match absSearch against absFile to find what each * expands to
        if (AZStd::regex_search(absFile.begin(), absFile.end(), result, AZStd::regex(searchAsRegex, AZStd::regex::icase)))
        {
            // For each * expansion, replace the * in the destination with the expanded result
            for (size_t i = 1; i < result.size(); ++i)
            {
                auto matchedString = result[i].str();

                // Only the last match can match across directory levels
                if (matchedString.find(AZ_CORRECT_FILESYSTEM_SEPARATOR) != matchedString.npos && i < result.size() - 1)
                {
                    AZ_Error("LocalFileSCComponent", false, "Wildcard cannot match across directory levels.  Please simplify your search or put a wildcard at the end of the search to match across directories.");
                    return {};
                }

                destination.replace(destination.find('*'), 1, result[i].str().c_str());
            }
        }

        return destination;
    }

    void LocalFileSCComponent::RequestDeleteBulk(const char* fullFilePath, const SourceControlResponseCallbackBulk& respCallback)
    {
        RequestDeleteBulkExtended(fullFilePath, false, respCallback);
    }

    void LocalFileSCComponent::RequestDeleteBulkExtended(const char* fullFilePath, bool skipReadOnly, const SourceControlResponseCallbackBulk& respCallback)
    {
        AZStd::string fullFilePathString = fullFilePath; // We need a string we can copy to the job thread since fullFilePath could go out of scope before the job runs

        auto job = AZ::CreateJobFunction([fullFilePathString, skipReadOnly, respCallback]() mutable
            {
                AZStd::vector<SourceControlFileInfo> info;
                QStringList files = GetFiles(fullFilePathString.c_str());

                for (QString file : files)
                {
                    info.push_back();
                    auto& fileInfo = info.back();

                    fileInfo.m_filePath = file.toUtf8().constData();
                    RefreshInfoFromFileSystem(fileInfo);

                    if (!skipReadOnly)
                    {
                        RemoveReadOnly(fileInfo);
                    }

                    auto succeeded = AZ::IO::SystemFile::Delete(fileInfo.m_filePath.c_str());
                    RefreshInfoFromFileSystem(fileInfo);

                    fileInfo.m_status = succeeded ? SCS_OpSuccess : SCS_ProviderError;
                }

                AZ::TickBus::QueueFunction(respCallback, true, info);
            }, true);
        job->Start();
    }

    void LocalFileSCComponent::RequestRevert(const char* fullFilePath, const SourceControlResponseCallback& respCallback)
    {
        // Get the info, and fail if the file doesn't exist.
        GetFileInfo(fullFilePath, respCallback);
    }

    void LocalFileSCComponent::RequestLatest(const char* fullFilePath, const SourceControlResponseCallback& respCallback)
    {
        SourceControlFileInfo fileInfo(fullFilePath);
        auto job = AZ::CreateJobFunction([fileInfo, respCallback]() mutable
        {
            RefreshInfoFromFileSystem(fileInfo);
            AZ::TickBus::QueueFunction(respCallback, true, fileInfo);
        }, true);
        job->Start();
    }

    void LocalFileSCComponent::RequestRename(const char* sourcePathFull, const char* destPathFull, const SourceControlResponseCallback& respCallback)
    {
        RequestRenameExtended(sourcePathFull, destPathFull, false, respCallback);
    }

    void LocalFileSCComponent::RequestRenameExtended(const char* sourcePathFull, const char* destPathFull, bool skipReadOnly, const SourceControlResponseCallback& respCallback)
    {
        SourceControlFileInfo fileInfoSrc(sourcePathFull);
        SourceControlFileInfo fileInfoDst(destPathFull);
        auto job = AZ::CreateJobFunction([fileInfoSrc, fileInfoDst, skipReadOnly, respCallback]() mutable
            {
                bool succeeded = true;

                RefreshInfoFromFileSystem(fileInfoSrc);

                if (!skipReadOnly || fileInfoSrc.HasFlag(SourceControlFlags::SCF_Writeable))
                {
                    succeeded = AZ::IO::SystemFile::Rename(fileInfoSrc.m_filePath.c_str(), fileInfoDst.m_filePath.c_str());
                    RefreshInfoFromFileSystem(fileInfoDst);
                    fileInfoDst.m_status = succeeded ? SCS_OpSuccess : SCS_ProviderError;
                }
                else
                {
                    fileInfoDst.m_status = SCS_ProviderError;
                }
                
                AZ::TickBus::QueueFunction(respCallback, succeeded, fileInfoDst);
            }, true);
        job->Start();
    }

    void LocalFileSCComponent::RequestRenameBulk(const char* sourcePathFull, const char* destPathFull, const SourceControlResponseCallbackBulk& respCallback)
    {
        RequestRenameBulkExtended(sourcePathFull, destPathFull, false, respCallback);
    }

    void LocalFileSCComponent::RequestRenameBulkExtended(const char* sourcePathFull, const char* destPathFull, bool skipReadOnly, const SourceControlResponseCallbackBulk& respCallback)
    {
        AZStd::string sourcePathFullString = sourcePathFull;
        AZStd::string destPathFullString = destPathFull;

        auto job = AZ::CreateJobFunction([sourcePathFullString, destPathFullString, skipReadOnly, respCallback]() mutable
            {
                bool success = true;
                AZStd::vector<SourceControlFileInfo> info;

                AZ::s64 sourceWildcardCount = std::count(sourcePathFullString.begin(), sourcePathFullString.end(), '*');
                AZ::s64 destinationWildcardCount = std::count(destPathFullString.begin(), destPathFullString.end(), '*');

                if (sourceWildcardCount != destinationWildcardCount)
                {
                    success = false;
                    AZ_Error("LocalFileSCComponent", false, "Source and destination paths must have the same number of wildcards.");
                }
                else
                {
                    QStringList files = GetFiles(sourcePathFullString.c_str());

                    for (QString file : files)
                    {
                        AZStd::string absFilePath(file.toUtf8().constData());
                        AZ::StringFunc::Replace(absFilePath, AZ_WRONG_FILESYSTEM_SEPARATOR, AZ_CORRECT_FILESYSTEM_SEPARATOR);
                        auto destination = ResolveWildcardDestination(absFilePath, sourcePathFullString, destPathFullString);

                        SourceControlFileInfo fileInfoSrc(file.toUtf8().constData());
                        SourceControlFileInfo fileInfoDst(destination.c_str());

                        RefreshInfoFromFileSystem(fileInfoSrc);

                        bool succeeded = false;

                        if (!skipReadOnly || fileInfoSrc.HasFlag(SourceControlFlags::SCF_Writeable))
                        {
                            AZStd::string destinationFolder;
                            AZ::StringFunc::Path::GetFullPath(fileInfoDst.m_filePath.c_str(), destinationFolder);
                            AZ::IO::SystemFile::CreateDir(destinationFolder.c_str());

                            succeeded = AZ::IO::SystemFile::Rename(file.toUtf8().constData(), fileInfoDst.m_filePath.c_str());
                            RefreshInfoFromFileSystem(fileInfoDst);
                        }

                        fileInfoDst.m_status = succeeded ? SCS_OpSuccess : SCS_ProviderError;
                        info.push_back(fileInfoDst);
                    }
                }

                AZ::TickBus::QueueFunction(respCallback, success, info);
            }, true);
        job->Start();
    }

    void LocalFileSCComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<LocalFileSCComponent, AZ::Component>()

            ;
        }
    }
}   // namespace AzToolsFramework
