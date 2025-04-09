/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/tests/UnitTestUtilities.h>

namespace UnitTests
{
    MockComponentApplication::MockComponentApplication()
    {
        AZ::ComponentApplicationBus::Handler::BusConnect();
        AZ::Interface<AZ::ComponentApplicationRequests>::Register(this);
    }

    MockComponentApplication::~MockComponentApplication()
    {
        AZ::Interface<AZ::ComponentApplicationRequests>::Unregister(this);
        AZ::ComponentApplicationBus::Handler::BusDisconnect();
    }

    MockPathConversion::MockPathConversion(const char* scanfolder /*= "c:/somepath" */)
    {
        m_scanFolderInfo = AssetProcessor::ScanFolderInfo{ scanfolder, "scanfolder", "scanfolder", true, true, { AssetBuilderSDK::PlatformInfo{ "pc", {} } }, 0, 1 };
    }

    void MockPathConversion::SetScanFolder(const AssetProcessor::ScanFolderInfo& scanFolderInfo)
    {
        m_scanFolderInfo = scanFolderInfo;
    }

    bool MockPathConversion::ConvertToRelativePath(QString fullFileName, QString& databaseSourceName, QString& scanFolderName) const
    {
        EXPECT_TRUE(fullFileName.startsWith(m_scanFolderInfo.ScanPath(), Qt::CaseInsensitive));

        scanFolderName = m_scanFolderInfo.ScanPath();
        databaseSourceName = fullFileName.mid(scanFolderName.size() + 1);

        return true;
    }

    const AssetProcessor::ScanFolderInfo* MockPathConversion::GetScanFolderForFile(const QString& /*fullFileName*/) const
    {
        return &m_scanFolderInfo;
    }

    const AssetProcessor::ScanFolderInfo* MockPathConversion::GetScanFolderById(AZ::s64 /*id*/) const
    {
        return &m_scanFolderInfo;
    }

    bool MockMultiPathConversion::ConvertToRelativePath(QString fullFileName, QString& databaseSourceName, QString& scanFolderName) const
    {
        auto scanfolder = GetScanFolderForFile(fullFileName);
        EXPECT_TRUE(scanfolder);

        scanFolderName = scanfolder->ScanPath();
        databaseSourceName = fullFileName.mid(scanFolderName.size() + 1);

        return true;
    }

    const AssetProcessor::ScanFolderInfo* MockMultiPathConversion::GetScanFolderForFile(const QString& fullFileName) const
    {
        for (const auto& scanfolder : m_scanFolderInfo)
        {
            if (AZ::IO::PathView(fullFileName.toUtf8().constData())
                .IsRelativeTo(AZ::IO::PathView(scanfolder.ScanPath().toUtf8().constData())))
            {
                return &scanfolder;
            }
        }

        return nullptr;
    }

    const AssetProcessor::ScanFolderInfo* MockMultiPathConversion::GetScanFolderById(AZ::s64 id) const
    {
        return &m_scanFolderInfo[id - 1];
    }

    void MockMultiPathConversion::AddScanfolder(QString path, QString name)
    {
        m_scanFolderInfo.push_back(
            AssetProcessor::ScanFolderInfo(path, name, name, false, true, { AssetBuilderSDK::PlatformInfo{ "pc", {} } }, 0, m_scanFolderInfo.size() + 1));
    }
    
    constexpr AZ::u32 MockVirtualFileIO::ComputeHandle(AZ::IO::PathView path)
    {
        return AZ::u32(AZStd::hash<AZ::IO::PathView>{}(path));
    }

    MockMultiBuilderInfoHandler::~MockMultiBuilderInfoHandler()
    {
        BusDisconnect();
    }

    void MockMultiBuilderInfoHandler::GetMatchingBuildersInfo(
        const AZStd::string& assetPath, AssetProcessor::BuilderInfoList& builderInfoList)
    {
        AZStd::set<AZ::Uuid> uniqueBuilderDescIDs;

        for (AssetUtilities::BuilderFilePatternMatcher& matcherPair : m_matcherBuilderPatterns)
        {
            if (uniqueBuilderDescIDs.find(matcherPair.GetBuilderDescID()) != uniqueBuilderDescIDs.end())
            {
                continue;
            }
            if (matcherPair.MatchesPath(assetPath))
            {
                const AssetBuilderSDK::AssetBuilderDesc& builderDesc = m_builderDescMap[matcherPair.GetBuilderDescID()];
                uniqueBuilderDescIDs.insert(matcherPair.GetBuilderDescID());
                builderInfoList.push_back(builderDesc);
            }
        }
    }

    void MockMultiBuilderInfoHandler::GetAllBuildersInfo([[maybe_unused]] AssetProcessor::BuilderInfoList& builderInfoList)
    {
        for (const auto& builder : m_builderDescMap)
        {
            builderInfoList.push_back(builder.second);
        }
    }

    void MockMultiBuilderInfoHandler::CreateJobs(
        AssetBuilderExtraInfo extraInfo, const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;

        for (const auto& platform : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor jobDescriptor;
            jobDescriptor.m_priority = 0;
            jobDescriptor.m_critical = true;
            jobDescriptor.m_jobKey = "Mock Job";
            jobDescriptor.SetPlatformIdentifier(platform.m_identifier.c_str());
            jobDescriptor.m_additionalFingerprintInfo = extraInfo.m_jobFingerprint.toUtf8().constData();

            if (!extraInfo.m_jobDependencyFilePath.isEmpty())
            {
                auto jobDependency = AssetBuilderSDK::JobDependency(
                    "Mock Job", "pc", AssetBuilderSDK::JobDependencyType::Order,
                    AssetBuilderSDK::SourceFileDependency(extraInfo.m_jobDependencyFilePath.toUtf8().constData(), AZ::Uuid::CreateNull()));

                if (!extraInfo.m_subIdDependencies.empty())
                {
                    jobDependency.m_productSubIds = extraInfo.m_subIdDependencies;
                }

                jobDescriptor.m_jobDependencyList.push_back(jobDependency);
            }

            if (!extraInfo.m_dependencyFilePath.isEmpty())
            {
                response.m_sourceFileDependencyList.push_back(
                    AssetBuilderSDK::SourceFileDependency(extraInfo.m_dependencyFilePath.toUtf8().constData(), AZ::Uuid::CreateNull()));
            }

            response.m_createJobOutputs.push_back(jobDescriptor);
            m_createJobsCount++;
        }
    }

    void MockMultiBuilderInfoHandler::ProcessJob(
        AssetBuilderExtraInfo extraInfo,
        [[maybe_unused]] const AssetBuilderSDK::ProcessJobRequest& request,
        AssetBuilderSDK::ProcessJobResponse& response)
    {
        response.m_resultCode = AssetBuilderSDK::ProcessJobResultCode::ProcessJobResult_Success;
    }

    void MockMultiBuilderInfoHandler::CreateBuilderDesc(
        const QString& builderName,
        const QString& builderId,
        const AZStd::vector<AssetBuilderSDK::AssetBuilderPattern>& builderPatterns,
        const AssetBuilderExtraInfo& extraInfo)
    {
        CreateBuilderDesc(
            builderName, builderId, builderPatterns,
            AZStd::bind(&MockMultiBuilderInfoHandler::CreateJobs, this, extraInfo, AZStd::placeholders::_1, AZStd::placeholders::_2),
            AZStd::bind(&MockMultiBuilderInfoHandler::ProcessJob, this, extraInfo, AZStd::placeholders::_1, AZStd::placeholders::_2), extraInfo.m_analysisFingerprint);
    }

    void MockMultiBuilderInfoHandler::CreateBuilderDescInfoRef(
        const QString& builderName,
        const QString& builderId,
        const AZStd::vector<AssetBuilderSDK::AssetBuilderPattern>& builderPatterns,
        const AssetBuilderExtraInfo& extraInfo)
    {
        CreateBuilderDesc(
            builderName, builderId, builderPatterns,
            AZStd::bind(&MockMultiBuilderInfoHandler::CreateJobs, this, AZStd::ref(extraInfo), AZStd::placeholders::_1, AZStd::placeholders::_2),
            AZStd::bind(&MockMultiBuilderInfoHandler::ProcessJob, this, AZStd::ref(extraInfo), AZStd::placeholders::_1, AZStd::placeholders::_2),
            extraInfo.m_analysisFingerprint);
    }

    void MockMultiBuilderInfoHandler::CreateBuilderDesc(
        const QString& builderName,
        const QString& builderId,
        const AZStd::vector<AssetBuilderSDK::AssetBuilderPattern>& builderPatterns,
        const AssetBuilderSDK::CreateJobFunction& createJobsFunction,
        const AssetBuilderSDK::ProcessJobFunction& processJobFunction,
        AZStd::optional<QString> analysisFingerprint)
    {
        AssetBuilderSDK::AssetBuilderDesc builderDesc;

        builderDesc.m_name = builderName.toUtf8().data();
        builderDesc.m_patterns = builderPatterns;
        builderDesc.m_busId = AZ::Uuid::CreateString(builderId.toUtf8().data());
        builderDesc.m_builderType = AssetBuilderSDK::AssetBuilderDesc::AssetBuilderType::Internal;
        builderDesc.m_createJobFunction = createJobsFunction;
        builderDesc.m_processJobFunction = processJobFunction;

        if (analysisFingerprint && !analysisFingerprint->isEmpty())
        {
            builderDesc.m_analysisFingerprint = analysisFingerprint.value().toUtf8().constData();
        }

        m_builderDescMap[builderDesc.m_busId] = builderDesc;

        for (const AssetBuilderSDK::AssetBuilderPattern& pattern : builderDesc.m_patterns)
        {
            AssetUtilities::BuilderFilePatternMatcher patternMatcher(pattern, builderDesc.m_busId);
            m_matcherBuilderPatterns.push_back(patternMatcher);
        }
    }

    TraceBusErrorChecker::TraceBusErrorChecker()
    {
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
    }

    TraceBusErrorChecker::~TraceBusErrorChecker()
    {
        EXPECT_FALSE(m_expectingFailure);
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
    }

    void TraceBusErrorChecker::Begin()
    {
        m_expectingFailure = true;
        m_suppressedMessages.clear();
    }

    void TraceBusErrorChecker::End(int expectedCount)
    {
        m_expectingFailure = false;

        if (expectedCount != m_suppressedMessages.size())
        {
            for (const auto& message : m_suppressedMessages)
            {
                AZ::Debug::Trace::Instance().Output("", message.c_str());
            }

            EXPECT_EQ(expectedCount, m_suppressedMessages.size());
        }
    }

    bool TraceBusErrorChecker::OnPreAssert(
        const char* fileName,
        int line,
        const char* func,
        const char* message)
    {
        RecordError(fileName, line, func, message);

        return m_expectingFailure;
    }

    bool TraceBusErrorChecker::OnPreError(
        [[maybe_unused]] const char* window,
        const char* fileName,
        int line,
        const char* func,
        const char* message)
    {
        RecordError(fileName, line, func, message);

        return m_expectingFailure;
    }

    bool TraceBusErrorChecker::OnPreWarning(
        [[maybe_unused]] const char* window,
        const char* fileName,
        int line,
        const char* func,
        const char* message)
    {
        RecordError(fileName, line, func, message);

        return m_expectingFailure;
    }

    void TraceBusErrorChecker::RecordError(const char* fileName, int line, const char* func, const char* message)
    {
        AZStd::string errorMessage = AZStd::string::format("%s(%d): %s - %s\n", fileName, line, func, message);

        if (!m_expectingFailure)
        {
            AZ::Debug::Trace::Instance().Output("", errorMessage.c_str());
            GTEST_NONFATAL_FAILURE_("Unexpected failure occurred");
        }

        m_suppressedMessages.push_back(errorMessage);
    }

    MockVirtualFileIO::MockVirtualFileIO()
    {
        // Cache the existing file io instance and build our mock file io
        m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
        m_fileIOMock = AZStd::make_unique<testing::NiceMock<AZ::IO::MockFileIOBase>>();

        // Swap out current file io instance for our mock
        AZ::IO::FileIOBase::SetInstance(nullptr);
        AZ::IO::FileIOBase::SetInstance(m_fileIOMock.get());

        // Setup the default returns for our mock file io calls
        AZ::IO::MockFileIOBase::InstallDefaultReturns(*m_fileIOMock.get());

        using namespace ::testing;
        using namespace AZ;

        ON_CALL(*m_fileIOMock, Open(_, _, _))
            .WillByDefault(Invoke(
                [this](auto filePath, IO::OpenMode mode, IO::HandleType& handle)
                {
                    AZStd::string normalizedPath(filePath);
                    AzFramework::StringFunc::Path::Normalize(normalizedPath);
                    handle = ComputeHandle(AZ::IO::PathView(normalizedPath));

                    int systemMode = AZ::IO::TranslateOpenModeToSystemFileMode(normalizedPath.c_str(), mode);

                    // Any mode besides OPEN_READ_ONLY creates a file
                    if ((systemMode & ~int(IO::SystemFile::SF_OPEN_READ_ONLY)) > 0)
                    {
                        m_mockFiles[handle] = { normalizedPath, "" };
                    }

                    return AZ::IO::Result(AZ::IO::ResultCode::Success);
                }));

        ON_CALL(*m_fileIOMock, Tell(_, _))
            .WillByDefault(Invoke(
                [](auto, auto& offset)
                {
                    offset = 0;
                    return AZ::IO::ResultCode::Success;
                }));

        ON_CALL(*m_fileIOMock, Size(An<AZ::IO::HandleType>(), _))
            .WillByDefault(Invoke(
                [this](auto handle, AZ::u64& size)
                {
                    auto itr = m_mockFiles.find(handle);

                    size = itr != m_mockFiles.end() ? itr->second.second.size() : 0;

                    return AZ::IO::ResultCode::Success;
                }));

        ON_CALL(*m_fileIOMock, Size(An<const char*>(), _))
            .WillByDefault(Invoke(
                [this](const char* filePath, AZ::u64& size)
                {
                    AZStd::string normalizedPath(filePath);
                    AzFramework::StringFunc::Path::Normalize(normalizedPath);
                    auto handle = ComputeHandle(AZ::IO::PathView(normalizedPath));
                    auto itr = m_mockFiles.find(handle);

                    size = itr != m_mockFiles.end() ? itr->second.second.size() : 0;

                    return AZ::IO::ResultCode::Success;
                }));

        ON_CALL(*m_fileIOMock, Exists(_))
            .WillByDefault(Invoke(
                [this](const char* filePath)
                {
                    AZStd::string normalizedPath(filePath);
                    AzFramework::StringFunc::Path::Normalize(normalizedPath);
                    auto handle = ComputeHandle(AZ::IO::PathView(normalizedPath));
                    auto itr = m_mockFiles.find(handle);
                    return itr != m_mockFiles.end();
                }));

        ON_CALL(*m_fileIOMock, Rename(_, _))
            .WillByDefault(Invoke(
                [this](const char* originalPath, const char* newPath)
                {
                    AZStd::string normalizedOriginalPath(originalPath);
                    AzFramework::StringFunc::Path::Normalize(normalizedOriginalPath);
                    auto originalHandle = ComputeHandle(AZ::IO::PathView(normalizedOriginalPath));

                    AZStd::string normalizedNewPath(newPath);
                    AzFramework::StringFunc::Path::Normalize(normalizedNewPath);
                    auto newHandle = ComputeHandle(AZ::IO::PathView(normalizedNewPath));

                    auto itr = m_mockFiles.find(originalHandle);

                    if (itr != m_mockFiles.end())
                    {
                        auto& [path, contents] = itr->second;
                        path = normalizedNewPath;

                        if (originalHandle != newHandle)
                        {
                            m_mockFiles[newHandle] = itr->second;
                            m_mockFiles.erase(itr);
                        }

                        return AZ::IO::ResultCode::Success;
                    }

                    return AZ::IO::ResultCode::Error;
                }));

        ON_CALL(*m_fileIOMock, Remove(_))
            .WillByDefault(Invoke(
                [this](const char* filePath)
                {
                    AZStd::string normalizedPath(filePath);
                    AzFramework::StringFunc::Path::Normalize(normalizedPath);
                    auto handle = ComputeHandle(AZ::IO::PathView(normalizedPath));

                    m_mockFiles.erase(handle);

                    return AZ::IO::ResultCode::Success;
                }));

        ON_CALL(*m_fileIOMock, Read(_, _, _, _, _))
            .WillByDefault(Invoke(
                [this](auto handle, void* buffer, auto, auto, AZ::u64* bytesRead)
                {
                    auto itr = m_mockFiles.find(handle);

                    if (itr == m_mockFiles.end())
                    {
                        return AZ::IO::ResultCode::Error;
                    }

                    memcpy(buffer, itr->second.second.c_str(), itr->second.second.size());
                    *bytesRead = itr->second.second.size();
                    return AZ::IO::ResultCode::Success;
                }));

        ON_CALL(*m_fileIOMock, Write(_, _, _, _))
            .WillByDefault(Invoke(
                [this](IO::HandleType fileHandle, const void* buffer, AZ::u64 size, AZ::u64* bytesWritten)
                {
                    auto& pair = m_mockFiles[fileHandle];

                    pair.second.resize(size);
                    memcpy((void*)pair.second.c_str(), buffer, size);

                    if (bytesWritten)
                    {
                        *bytesWritten = size;
                    }

                    return AZ::IO::ResultCode::Success;
                }));

        ON_CALL(*m_fileIOMock, FindFiles(_, _, _))
            .WillByDefault(Invoke(
                [this](const char* filePath, const char* filter, auto callback)
                {
                    if ((!filePath)||(filePath[0] == 0))
                    {
                        return AZ::IO::ResultCode::Error;
                    }

                    AZStd::string normalizedPath(filePath);
                    AzFramework::StringFunc::Path::Normalize(normalizedPath);

                    size_t filePathLen = normalizedPath.length();

                    // there is unfortunately an extra special case here
                    // This function could be called with filePath being something like "c:/" for the root of the file system
                    // so the wildcard search term has to be "c:/{filter}" 
                    // but could also be called without a trailing slash for all other folders
                    // like "c:/somepath", and thus the formatting string to combine them must have a trailing slash.
                    // We are specifically AVOIDING using AZ::IO::Path here because these are mock paths that might
                    // be invalid paths on posix (for example, a unit test could ask for "c:/whatever" - the file system
                    // is also a mock file system.)
                    const char endingChar = normalizedPath[filePathLen - 1];
                    bool filePathHasTrailingSlash = endingChar == AZ_CORRECT_FILESYSTEM_SEPARATOR;
                    AZStd::string formatter = filePathHasTrailingSlash ? "%s%s" : "%s" AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING "%s";

                    // mockFiles contains only files, but this function is expected to output directories as well
                    // we will emit any directory that is a substring of a stored file path.
                    // this will cause it to emit the same one multiple times, but this is enough for emulation.
                    for (const auto& [hash, pair] : m_mockFiles)
                    {
                        const auto& [path, contents] = pair;
                        
                        if (AZStd::wildcard_match(AZStd::string::format(formatter.c_str(), normalizedPath.c_str(), filter), path))
                        {
                            // path is a full path to a file, but we need to emulate directory traversal
                            // e.g. normalizedPath is a path like "c:/" and the path in the cache might be something like
                            // "c:/somepath/somefile.txt".  For this to function correctly, we must behave as if
                            // we return "c:/somepath" here, indicating that the contents of "c:/" is "somepath"
                            //  and not "c:/somepath/somefile.txt" as this is NOT a recursive call.

                            AZStd::string pathWithoutRoot = path.substr(filePathHasTrailingSlash ? filePathLen : filePathLen + 1);
                            size_t slashPos = pathWithoutRoot.find_first_of(AZ_CORRECT_FILESYSTEM_SEPARATOR);
                            // eg, input: "c:/", we found "c:/somepath/somefile.txt" in our cache,
                            // so the pathWithoutRoot is "somepath/somefile.txt".
                            if (slashPos != AZStd::string::npos)
                            {
                                // if we get here, it means that the path we found in our hash is deeper
                                // in the virtual file tree than the local we are virtually traversing,
                                // so we return just the folder name after adding the root back in the front:
                                AZStd::string reassembled = AZStd::string::format(formatter.c_str(), filePath, pathWithoutRoot.substr(0, slashPos).c_str());
                                if (!callback(reassembled.c_str()))
                                {
                                    return AZ::IO::ResultCode::Success;
                                }
                            }
                            else
                            {
                                if(!callback(path.c_str()))
                                {
                                    return AZ::IO::ResultCode::Success;
                                }
                            }
                        }
                    }
                    return AZ::IO::ResultCode::Success;
                }));

        ON_CALL(*m_fileIOMock, IsDirectory(_))
            .WillByDefault(Invoke(
                [this](const char* filePath)
                {
                    AZStd::string normalizedPath(filePath);
                    AzFramework::StringFunc::Path::Normalize(normalizedPath);
                    for (const auto& [hash, pair] : m_mockFiles)
                    {
                        const auto& [path, contents] = pair;
                        // if the given path is a prefix of the file path, it could be a directory
                        if (path.find(normalizedPath.c_str()) == 0)
                        {
                            // its not a directory if the path is the exact same as the file path
                            if (path.compare(normalizedPath.c_str()) == 0)
                            {
                                return false; // we found an exact match, so its not a directory, its a file.
                            }

                            // if we get here, path starts with filePath must logically be longer than filePath
                            // since it did not match exactly but started with it.
                            // It is a directory if there is a slash immediately after filePath inside path
                            if (path[normalizedPath.length()] == AZ_CORRECT_FILESYSTEM_SEPARATOR)
                            {
                                // if we get here, we have positively identified a file path in the cache that
                                // has the given filePath as a substring of it and the character
                                // after the substring in the cache is a slash... it must be a directory.
                                return true;
                            }
                        }
                    }
                    return false;
                }));
        }

        MockVirtualFileIO::~MockVirtualFileIO()
        {
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_priorFileIO);
        }

    void MockFileStateCache::RegisterForDeleteEvent(AZ::Event<AssetProcessor::FileStateInfo>::Handler& handler)
    {
        handler.Connect(m_deleteEvent);
    }

    bool MockFileStateCache::GetHash(const QString& absolutePath, FileHash* foundHash)
    {
        if (!Exists(absolutePath))
        {
            return false;
        }

        *foundHash = AssetUtilities::GetFileHash(absolutePath.toUtf8().constData(), true);

        return true;
    }

    // this MockFileStateCache has to be permissive for the case where on posix systems the unit tests
    // still ask for non-posix paths like "c:/whatever" and expect the rootpath to be "c:/".  Calling Path::RootPath
    // actually fails on those operating systems (correctly!) because "c:/something" on those systems
    // is a relative path, representing '${PWD}/c:/something' with c: being just another directory name.
    // To get around this we have to manually split the path here.
    static void MockAbsoluteSplit(const QString& absolutePath, AZStd::string& rootPath, AZStd::string& relPathFromRoot)
    {
        int firstSlash = absolutePath.indexOf("/"); // we assume normalized forward slashes.
        if (firstSlash == -1)
        {
            rootPath = AZStd::string();
            relPathFromRoot = absolutePath.toUtf8().constData();
            return;
        }

        rootPath = absolutePath.left(firstSlash + 1).toUtf8().constData();
        relPathFromRoot = absolutePath.mid(firstSlash + 1).toUtf8().constData();
    }

    bool MockFileStateCache::GetFileInfo(const QString& absolutePath, AssetProcessor::FileStateInfo* foundFileInfo) const
    {
        if (Exists(absolutePath))
        {
            auto* io = AZ::IO::FileIOBase::GetInstance();
            AZ::u64 size;
            io->Size(absolutePath.toUtf8().constData(), size);

            AZStd::string rootPath;
            AZStd::string relPathFromRoot;
            MockAbsoluteSplit(absolutePath, rootPath, relPathFromRoot);

            // convert the path to the correct case (to emulate GetFileInfo actual).  
            // Note that calling AssetUtilities::UpdateToCorrectCase would
            // cause a stack overflow since it would call this function again.
            // Instead, call the underlying AzToolsFramework function.
            AzToolsFramework::AssetUtils::UpdateFilePathToCorrectCase(rootPath.c_str(), relPathFromRoot);

            AZ::IO::FixedMaxPath correctedPath{rootPath};
            correctedPath /= relPathFromRoot;

            *foundFileInfo = AssetProcessor::FileStateInfo(
                correctedPath.c_str(),
                QDateTime::fromMSecsSinceEpoch(io->ModificationTime(absolutePath.toUtf8().constData())),
                size,
                io->IsDirectory(absolutePath.toUtf8().constData()));

            return true;
        }

        return false;
    }

    bool MockFileStateCache::Exists(const QString& absolutePath) const
    {
        // this API needs to be case insensitive to be satisfied, so on case sensitive file systems, we should
        // double check.  Note that UpdateFilePathToCorrectCase is very expensive, so we only use it as a fallback
        // and prefer if the initial if statement passes and returns true
        if (AZ::IO::FileIOBase::GetInstance()->Exists(absolutePath.toUtf8().constData()))
        {
            return true;
        }

        // note that during Mock unit test operations, the above FileIOBase might be a mock file io base, 
        // which uses a cache of files and is itself case sensitive.  So even on case-insensitive file systems
        // this mock still has to do the below de-sensitizing.
        AZStd::string rootPath;
        AZStd::string relPathFromRoot;
        MockAbsoluteSplit(absolutePath, rootPath, relPathFromRoot);

        if (AzToolsFramework::AssetUtils::UpdateFilePathToCorrectCase(rootPath, relPathFromRoot))
        {
            return true;
        }
        return false;
    }
} // namespace UnitTests
