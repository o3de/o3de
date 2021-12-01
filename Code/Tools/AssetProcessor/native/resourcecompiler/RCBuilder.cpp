/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RCBuilder.h"

#include <QElapsedTimer>
#include <QCoreApplication>

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/Utils/Utils.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Application/Application.h>

#include <AzFramework/Process/ProcessWatcher.h>
#include <AzToolsFramework/Application/ToolsApplication.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>

#include "native/resourcecompiler/rccontroller.h"

#include "native/utilities/assetUtils.h"
#include "native/utilities/AssetBuilderInfo.h"

#include <AssetProcessor_Traits_Platform.h>

namespace AssetProcessor
{
    // Temporary solution to get around the fact that we don't have job dependencies
    static AZStd::atomic_bool s_TempSolution_CopyJobsFinished(false);
    static AZStd::atomic<int> s_TempSolution_CopyJobActivityCounter(0);

    static void TempSolution_TouchCopyJobActivity()
    {
        s_TempSolution_CopyJobActivityCounter++;
        s_TempSolution_CopyJobsFinished = false;
    }

    //! Special ini configuration keyword to mark a asset pattern for skipping
    const QString ASSET_PROCESSOR_CONFIG_KEYWORD_SKIP = "skip";

    //! Special ini configuration keyword to mark a asset pattern for copying
    const QString ASSET_PROCESSOR_CONFIG_KEYWORD_COPY = "copy";

    namespace Internal
    {
        void PopulateCommonDescriptorParams(AssetBuilderSDK::JobDescriptor& descriptor, const QString& platformIdentifier, const AssetPlatformSpec& platformSpec, const InternalAssetRecognizer* const recognizer)
        {
            descriptor.m_jobKey = recognizer->m_name.toUtf8().constData();
            descriptor.SetPlatformIdentifier(platformIdentifier.toUtf8().constData());
            descriptor.m_priority = recognizer->m_priority;
            descriptor.m_checkExclusiveLock = recognizer->m_testLockSource;

            QString extraInformationForFingerprinting;
            extraInformationForFingerprinting.append(platformSpec.m_extraRCParams);
            extraInformationForFingerprinting.append(recognizer->m_version);

            // if we have specified the product asset type, changing it should cuase
            if (!recognizer->m_productAssetType.IsNull())
            {
                char typeAsString[64] = { 0 };
                recognizer->m_productAssetType.ToString(typeAsString, AZ_ARRAY_SIZE(typeAsString));
                extraInformationForFingerprinting.append(typeAsString);
            }

            descriptor.m_priority = recognizer->m_priority;

            descriptor.m_additionalFingerprintInfo = AZStd::string(extraInformationForFingerprinting.toUtf8().constData());

            bool isCopyJob = (platformSpec.m_extraRCParams.compare(ASSET_PROCESSOR_CONFIG_KEYWORD_COPY) == 0);

            // Temporary solution to get around the fact that we don't have job dependencies
            if (isCopyJob)
            {
                TempSolution_TouchCopyJobActivity();
            }

            // If this is a copy job or critical is set to true in the ini file, then its a critical job
            descriptor.m_critical = recognizer->m_isCritical || isCopyJob;
            descriptor.m_checkServer = recognizer->m_checkServer;

            // If the priority of copy job is default then we update it to 1
            // This will ensure that copy jobs will be processed before other critical jobs having default priority
            if (isCopyJob && recognizer->m_priority == 0)
            {
                descriptor.m_priority = 1;
            }
        }
    }

    // don't make this too high, its basically how slowly the app responds to a job finishing.
    // this basically puts a hard cap on how many RC jobs can execute per second, since at 10ms per job (minimum), with 8 cores, thats a max
    // of 800 jobs per second that can possibly run.  However, the actual time it takes to launch RC.EXE is far, far longer than 10ms, so this is not a bad number for now...
    const int NativeLegacyRCCompiler::s_maxSleepTime = 10;

    // You have up to 60 minutes to finish processing an asset.
    // This was increased from 10 to account for PVRTC compression
    // taking up to an hour for large normal map textures, and should
    // be reduced again once we move to the ASTC compression format, or
    // find another solution to reduce processing times to be reasonable.
    const unsigned int NativeLegacyRCCompiler::s_jobMaximumWaitTime = 1000 * 60 * 60;

    NativeLegacyRCCompiler::Result::Result(int exitCode, bool crashed, const QString& outputDir)
        : m_exitCode(exitCode)
        , m_crashed(crashed)
        , m_outputDir(outputDir)
    {
    }

    NativeLegacyRCCompiler::NativeLegacyRCCompiler()
        : m_resourceCompilerInitialized(false)
        , m_requestedQuit(false)
    {
    }
    
    bool NativeLegacyRCCompiler::Initialize()
    {
        this->m_resourceCompilerInitialized = true;
        return true;
    }

    bool NativeLegacyRCCompiler::Execute(
        [[maybe_unused]] const QString& inputFile,
        [[maybe_unused]] const QString& watchFolder,
        [[maybe_unused]] const QString& platformIdentifier,
        [[maybe_unused]] const QString& params,
        [[maybe_unused]] const QString& dest,
        [[maybe_unused]] const AssetBuilderSDK::JobCancelListener* jobCancelListener,
        [[maybe_unused]] Result& result) const
    {
        // running RC.EXE is deprecated.
        AZ_Error("RC Builder", false, "running RC.EXE is deprecated");

        return false;
    }

    QString NativeLegacyRCCompiler::BuildCommand(const QString& inputFile, const QString& watchFolder, const QString& platformIdentifier, const QString& params, const QString& dest)
    {
        QString cmdLine;
        if (!dest.isEmpty())
        {
            QString projectPath = AssetUtilities::ComputeProjectPath();

            int portNumber = 0;
            ApplicationServerBus::BroadcastResult(portNumber, &ApplicationServerBus::Events::GetServerListeningPort);

            AZStd::string appBranchToken;
            AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::CalculateBranchTokenForEngineRoot, appBranchToken);
            cmdLine = QString("\"%1\" --platform=%2 %3 --unattended=true --project-path=\"%4\" --watchfolder=\"%6\" --targetroot=\"%5\" --logprefix=\"%5/\" --port=%7 --branchtoken=\"%8\"");
            cmdLine = cmdLine.arg(inputFile, platformIdentifier, params, projectPath, dest, watchFolder).arg(portNumber).arg(appBranchToken.c_str());
        }
        else
        {
            cmdLine = QString("\"%1\" --platform=%2 %3").arg(inputFile, platformIdentifier, params);
        }
        return cmdLine;
    }

    void NativeLegacyRCCompiler::RequestQuit()
    {
        this->m_requestedQuit = true;
    }

    BuilderIdAndName::BuilderIdAndName(QString builderName, QString builderId, Type type, QString rcParam /*=QString("")*/)
        : m_builderName(builderName)
        , m_builderId(builderId)
        , m_type(type)
        , m_rcParam(rcParam)
    {
    }
    BuilderIdAndName& BuilderIdAndName::operator=(const AssetProcessor::BuilderIdAndName& src)
    {
        this->m_builderId = src.m_builderId;
        this->m_builderName = src.m_builderName;
        this->m_type = src.m_type;
        this->m_rcParam = src.m_rcParam;
        return *this;
    }

    const QString& BuilderIdAndName::GetName() const
    {
        return this->m_builderName;
    }

    bool BuilderIdAndName::GetUuid(AZ::Uuid& builderUuid) const
    {
        if (this->m_type == Type::REGISTERED_BUILDER)
        {
            builderUuid = AZ::Uuid::CreateString(this->m_builderId.toUtf8().data());
            return true;
        }
        else
        {
            return false;
        }
    }

    const QString& BuilderIdAndName::GetRcParam() const
    {
        return this->m_rcParam;
    }

    const QString& BuilderIdAndName::GetId() const
    {
        return this->m_builderId;
    }

    const BuilderIdAndName::Type BuilderIdAndName::GetType() const
    {
        return this->m_type;
    }

    const char* INTERNAL_BUILDER_UUID_STR = "589BE398-2EBB-4E3C-BE66-C894E34C944D";
    const BuilderIdAndName  BUILDER_ID_COPY("Internal Copy Builder", "31B74BFD-7046-47AC-A7DA-7D5167E9B2F8", BuilderIdAndName::Type::REGISTERED_BUILDER, ASSET_PROCESSOR_CONFIG_KEYWORD_COPY);
    const BuilderIdAndName  BUILDER_ID_RC  ("Internal RC Builder",   "0BBFC8C1-9137-4404-BD94-64C0364EFBFB", BuilderIdAndName::Type::REGISTERED_BUILDER);
    const BuilderIdAndName  BUILDER_ID_SKIP("Internal Skip Builder", "A033AF24-5041-4E24-ACEC-161A2E522BB6", BuilderIdAndName::Type::UNREGISTERED_BUILDER, ASSET_PROCESSOR_CONFIG_KEYWORD_SKIP);

    const QHash<QString, BuilderIdAndName> ALL_INTERNAL_BUILDER_BY_ID =
    {
        { BUILDER_ID_COPY.GetId(), BUILDER_ID_COPY },
        { BUILDER_ID_RC.GetId(), BUILDER_ID_RC },
        { BUILDER_ID_SKIP.GetId(), BUILDER_ID_SKIP }
    };

    InternalAssetRecognizer::InternalAssetRecognizer(const AssetRecognizer& src, const QString& builderId, const QHash<QString, AssetPlatformSpec>& assetPlatformSpecByPlatform)
        : AssetRecognizer(src.m_name, src.m_testLockSource, src.m_priority, src.m_isCritical, src.m_supportsCreateJobs, src.m_patternMatcher, src.m_version, src.m_productAssetType, src.m_outputProductDependencies, src.m_checkServer)
        , m_builderId(builderId)
    {
        // assetPlatformSpecByPlatform is a hash table like
        // "pc" --> (settings to compile on pc)
        // "ios" --> settings to compile on ios)
        // and so is m_platformSpecsByPlatform
        m_platformSpecsByPlatform = assetPlatformSpecByPlatform;
        m_paramID = CalculateCRC();
    }

    AZ::u32 InternalAssetRecognizer::CalculateCRC() const
    {
        AZ::Crc32 crc;

        crc.Add(m_name.toUtf8().data());
        crc.Add(m_builderId.toUtf8().data());
        crc.Add(const_cast<void*>(static_cast<const void*>(&m_testLockSource)), sizeof(m_testLockSource));
        crc.Add(const_cast<void*>(static_cast<const void*>(&m_priority)), sizeof(m_priority));
        crc.Add(m_patternMatcher.GetBuilderPattern().m_pattern.c_str());
        crc.Add(const_cast<void*>(static_cast<const void*>(&m_patternMatcher.GetBuilderPattern().m_type)), sizeof(m_patternMatcher.GetBuilderPattern().m_type));
        return static_cast<AZ::u32>(crc);
    }

    //! Constructor to initialize the internal builders and a general internal builder uuid that is used for bus
    //! registration.  This constructor is helpful for deriving other classes from this builder for purposes like
    //! unit testing.
    InternalRecognizerBasedBuilder::InternalRecognizerBasedBuilder(QHash<QString, BuilderIdAndName> inputBuilderByIdMap, AZ::Uuid internalBuilderUuid)
        : m_isShuttingDown(false)
        , m_rcCompiler(new NativeLegacyRCCompiler())
        , m_internalRecognizerBuilderUuid(internalBuilderUuid)
    {
        for (BuilderIdAndName builder : inputBuilderByIdMap)
        {
            m_builderById[builder.GetId()] = inputBuilderByIdMap[builder.GetId()];
        }
        AssetBuilderSDK::AssetBuilderCommandBus::Handler::BusConnect(m_internalRecognizerBuilderUuid);
    }

    //! Constructor to initialize the internal based builder to a present set of internal builders and fixed bus id
    InternalRecognizerBasedBuilder::InternalRecognizerBasedBuilder()
        : InternalRecognizerBasedBuilder(ALL_INTERNAL_BUILDER_BY_ID, AZ::Uuid::CreateString(INTERNAL_BUILDER_UUID_STR))
    {
    }

    InternalRecognizerBasedBuilder::~InternalRecognizerBasedBuilder()
    {
        AssetBuilderSDK::AssetBuilderCommandBus::Handler::BusDisconnect(m_internalRecognizerBuilderUuid);
        for (auto assetRecognizer : m_assetRecognizerDictionary)
        {
            delete assetRecognizer;
        }
    }

    AssetBuilderSDK::AssetBuilderDesc InternalRecognizerBasedBuilder::CreateBuilderDesc(const QString& builderId, const AZStd::vector<AssetBuilderSDK::AssetBuilderPattern>& builderPatterns)
    {
        const BuilderIdAndName& builder = m_builderById[builderId];

        AssetBuilderSDK::AssetBuilderDesc   builderDesc;
        builderDesc.m_name = builder.GetName().toUtf8().data();
        builderDesc.m_version = 2;
        builderDesc.m_patterns = builderPatterns;
        builderDesc.m_builderType = AssetBuilderSDK::AssetBuilderDesc::AssetBuilderType::Internal;

        // Only set a bus id on the descriptor if the builder is a registered builder
        AZ::Uuid busId;
        if (builder.GetUuid(busId))
        {
            builderDesc.m_busId = AZ::Uuid::CreateString(builderId.toUtf8().data());
        }

        builderDesc.m_createJobFunction = AZStd::bind(&InternalRecognizerBasedBuilder::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDesc.m_processJobFunction = AZStd::bind(&InternalRecognizerBasedBuilder::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
        return builderDesc;
    }

    void InternalRecognizerBasedBuilder::ShutDown()
    {
        m_isShuttingDown = true;
        this->m_rcCompiler->RequestQuit();
    }

    bool InternalRecognizerBasedBuilder::FindRC(QString& rcAbsolutePathOut)
    {
        AZ::IO::FixedMaxPath executableDirectory = AZ::Utils::GetExecutableDirectory();
        executableDirectory /= ASSETPROCESSOR_TRAIT_LEGACY_RC_RELATIVE_PATH;
        if (AZ::IO::SystemFile::Exists(executableDirectory.c_str()))
        {
            rcAbsolutePathOut = QString::fromUtf8(executableDirectory.c_str(), static_cast<int>(executableDirectory.Native().size()));
            return true;
        }

        return false;
    }

    bool InternalRecognizerBasedBuilder::Initialize(const RecognizerConfiguration& recognizerConfig)
    {
        InitializeAssetRecognizers(recognizerConfig.GetAssetRecognizerContainer());
        return m_rcCompiler->Initialize();
    }


    void InternalRecognizerBasedBuilder::InitializeAssetRecognizers(const RecognizerContainer& assetRecognizers)
    {
        // Split the asset recognizers that were scanned in into 'buckets' for each of the 3 builder ids based on
        // either the custom fixed rc params or the standard rc param ('copy','skip', or others)

        QHash<QString, InternalAssetRecognizerList> internalRecognizerListByType;
        InternalRecognizerBasedBuilder::BuildInternalAssetRecognizersByType(assetRecognizers, internalRecognizerListByType);

        // note that the QString key to this map is actually the builder ID (as in the QString "Internal Copy Builder" for example)
        // and the key of the map is actually a AZStd::list for InternalAssetRecognizer* which belong to that builder
        // inside of each such recognizer is a map of [platform] --> options for that platform.
        // so visualizing this whole struct in summary might look something like

        // "Internal RC Builder" :
        // {
        //      {                  <----- list of recognizers for that RC builder starts here
        //        regex: "*.tif",
        //        builderUUID : "12345-12354-123145",
        //        platformSpecsByPlatform :
        //        {
        //          "pc"  : "streaming = 1",
        //          "ios" : "streaming = 0"
        //        }
        //      },
        //      {
        //        regex: "*.png",
        //        builderUUID : "12345-12354-123145",
        //        platformSpecsByPlatform :
        //        {
        //          "pc"  : "split=1"
        //        }
        //      },
        // },
        // "Internal Copy Builder",
        //      {
        //        regex: "*.cfg",
        //        builderUUID : "12345-12354-123145",
        //        platformSpecsByPlatform :
        //        {
        //          "pc"  : "copy",
        //          "ios" : "copy"
        //        }
        //      },

        for (auto internalRecognizerList = internalRecognizerListByType.begin();
             internalRecognizerList != internalRecognizerListByType.end();
             internalRecognizerList++)
        {
            QString builderId = internalRecognizerList.key();
            const BuilderIdAndName& builderInfo = m_builderById[builderId];
            QString builderName = builderInfo.GetName();
            AZStd::vector<AssetBuilderSDK::AssetBuilderPattern> builderPatterns;

            bool supportsCreateJobs = false;
            // intentionaly using a set here, as we want it to be the same order each time for hashing.
            AZStd::set<AZStd::string> fingerprintRelevantParameters;

            for (auto internalAssetRecognizer : *internalRecognizerList)
            {
                // so referring to the structure explanation above, internalAssetRecognizer is
                // one of those objects that has the regex in it, (along with list of commands to apply per platform)
                if (internalAssetRecognizer->m_platformSpecsByPlatform.size() == 0)
                {
                    delete internalAssetRecognizer;
                    AZ_Warning(AssetProcessor::DebugChannel, false, "Skipping recognizer %s, no platforms supported\n", builderName.toUtf8().data());
                    continue;
                }

                // Ignore duplicate recognizers
                // note that m_paramID is the CRC of a bunch of values inside the recognizer, so different recognizers should have a different paramID.
                if (m_assetRecognizerDictionary.contains(internalAssetRecognizer->m_paramID))
                {
                    delete internalAssetRecognizer;
                    AZ_Warning(AssetProcessor::DebugChannel, false, "Ignoring duplicate asset recognizer in configuration: %s\n", builderName.toUtf8().data());
                    continue;
                }

                for (auto iteratorValue = internalAssetRecognizer->m_platformSpecsByPlatform.begin(); iteratorValue != internalAssetRecognizer->m_platformSpecsByPlatform.end(); ++iteratorValue)
                {
                    fingerprintRelevantParameters.insert(AZStd::string::format("%s-%s", iteratorValue.key().toUtf8().constData(), iteratorValue.value().m_extraRCParams.toUtf8().constData()));
                }

                // note that the version number must be included here, despite the builder dirty-check function taking version into account
                // because the RC Builder is just a single builder (with version#0) that defers to these "internal" builders when called upon.
                if (!internalAssetRecognizer->m_version.isEmpty())
                {
                    fingerprintRelevantParameters.insert(internalAssetRecognizer->m_version.toUtf8().constData());
                }
                fingerprintRelevantParameters.insert(internalAssetRecognizer->m_productAssetType.ToString<AZStd::string>());

                // Register the recognizer
                builderPatterns.push_back(internalAssetRecognizer->m_patternMatcher.GetBuilderPattern());
                m_assetRecognizerDictionary[internalAssetRecognizer->m_paramID] = internalAssetRecognizer;
                AZ_TracePrintf(AssetProcessor::DebugChannel, "Registering %s as a %s\n", internalAssetRecognizer->m_name.toUtf8().data(),
                    builderName.toUtf8().data());

                supportsCreateJobs = supportsCreateJobs || (internalAssetRecognizer->m_supportsCreateJobs);
            }
            // Register the builder desc if its registrable
            if (builderInfo.GetType() == BuilderIdAndName::Type::REGISTERED_BUILDER)
            {
                AssetBuilderSDK::AssetBuilderDesc builderDesc = CreateBuilderDesc(builderId, builderPatterns);

                // RC Builder also needs to include its platforms and its RC command lines so that if you change this, the jobs
                // are re-evaluated.
                size_t currentHash = 0;
                for (const AZStd::string& element : fingerprintRelevantParameters)
                {
                    AZStd::hash_combine<AZStd::string>(currentHash, element);
                }

                builderDesc.m_analysisFingerprint = AZStd::string::format("0x%zX", currentHash);

                // the "rc" builder can only emit dependencies if it has createjobs in a recognizer.
                if (!supportsCreateJobs)
                {
                    // optimization: copy builder emits no dependencies since its just a copy builder.
                    builderDesc.m_flags |= AssetBuilderSDK::AssetBuilderDesc::BF_EmitsNoDependencies;
                }
                AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, builderDesc);
            }
        }
    }

    void InternalRecognizerBasedBuilder::UnInitialize()
    {
        for (BuilderIdAndName builder: m_builderById.values())
        {
            AZ::Uuid builderUuid;
            // Register the builder desc if its registrable
            if ((builder.GetType() == BuilderIdAndName::Type::REGISTERED_BUILDER) && (builder.GetUuid(builderUuid)))
            {
                EBUS_EVENT(AssetBuilderRegistrationBus, UnRegisterBuilderDescriptor, builderUuid);
            }
        }
    }

    bool InternalRecognizerBasedBuilder::GetMatchingRecognizers(const AZStd::vector<AssetBuilderSDK::PlatformInfo>& platformInfos, const QString& fileName, InternalRecognizerPointerContainer& output) const
    {
        QByteArray fileNameUtf8 = fileName.toUtf8();
        AZ_Assert(fileName.contains('\\') == false, "fileName must not contain backslashes: %s", fileNameUtf8.constData());

        bool foundAny = false;
        // assetRecognizerDictionary is a key value pair dictionary where
        // [key] is m_paramID of a recognizer - that is, a unique id of an internal asset recognizer
        // and [value] is the actual recognizer.
        // inside recognizers are the pattern that they match, as well as the various platforms that they compile for.
        for (const InternalAssetRecognizer* recognizer : m_assetRecognizerDictionary)
        {
            // so this platform is supported.  Check if the file matches the regex in MatchesPath.
            if (recognizer->m_patternMatcher.MatchesPath(fileNameUtf8.constData()))
            {
                // this recognizer does match that particular file name.
                // do we know how to compile it for any of the platforms?
                for (const AssetBuilderSDK::PlatformInfo& platformInfo : platformInfos)
                {
                    // recognizer->m_platformSpecsByplatform is a dictionary like
                    // ["pc"] -> what to do with the asset on PC.
                    // ["ios"] -> what to do wiht the asset on ios
                    if (recognizer->m_platformSpecsByPlatform.find(platformInfo.m_identifier.c_str()) != recognizer->m_platformSpecsByPlatform.end())
                    {
                        // yes, we have at least one platform that overlaps with the enabled platform list.
                        output.push_back(recognizer);
                        foundAny = true;
                        break;
                    }
                }


            }
        }
        return foundAny;
    }

    void InternalRecognizerBasedBuilder::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        // Convert the incoming builder id (AZ::Uuid) to the equivalent GUID from the asset recognizers
        AZStd::string azBuilderId;
        request.m_builderid.ToString(azBuilderId,false);
        QString requestedBuilderID = QString(azBuilderId.c_str());

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Failed;

        QDir watchFolder(request.m_watchFolder.c_str());
        QString normalizedPath = watchFolder.absoluteFilePath(request.m_sourceFile.c_str());
        normalizedPath = AssetUtilities::NormalizeFilePath(normalizedPath);

        // Locate recognizers that match the file
        InternalRecognizerPointerContainer  recognizers;
        if (!GetMatchingRecognizers(request.m_enabledPlatforms, normalizedPath, recognizers))
        {
            AssetBuilderSDK::BuilderLog(m_internalRecognizerBuilderUuid, "Cannot find recognizer for %s.", request.m_sourceFile.c_str());
            if (request.m_enabledPlatforms.empty())
            {
                response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
            }
            return;
        }

        // First pass
        for (const InternalAssetRecognizer* recognizer : recognizers)
        {
            if (recognizer->m_supportsCreateJobs)
            {
                // The recognizer's builder id must match the job requests' builder id
                if (recognizer->m_builderId.compare(requestedBuilderID) != 0)
                {
                    continue;
                }

                AssetBuilderSDK::CreateJobsResponse rcResponse;

                CreateLegacyRCJob(request, "", rcResponse);

                if (rcResponse.m_result != AssetBuilderSDK::CreateJobsResultCode::Success)
                {
                    // Error is already printed out by CreateLegacyRCJob
                    continue;
                }

                for (auto& descriptor : rcResponse.m_createJobOutputs)
                {
                    descriptor.m_jobParameters[recognizer->m_paramID] = descriptor.m_jobKey;
                }

                // Move-append the response outputs
                response.m_createJobOutputs.reserve(response.m_createJobOutputs.size() + rcResponse.m_createJobOutputs.size());
                response.m_createJobOutputs.reserve(response.m_sourceFileDependencyList.size() + rcResponse.m_sourceFileDependencyList.size());

                AZStd::move(rcResponse.m_createJobOutputs.begin(), rcResponse.m_createJobOutputs.end(), AZStd::back_inserter(response.m_createJobOutputs));
                AZStd::move(rcResponse.m_sourceFileDependencyList.begin(), rcResponse.m_sourceFileDependencyList.end(), AZStd::back_inserter(response.m_sourceFileDependencyList));

                response.m_result = rcResponse.m_result;
            }
            else
            {
                bool skippedByPlatform = false;

                // Iterate through the platform specific specs and apply the ones that match the platform flag
                for (auto iterPlatformSpec = recognizer->m_platformSpecsByPlatform.cbegin();
                    iterPlatformSpec != recognizer->m_platformSpecsByPlatform.cend();
                    iterPlatformSpec++)
                {
                    if (request.HasPlatform(iterPlatformSpec.key().toUtf8().constData()))
                    {
                        QString rcParam = iterPlatformSpec.value().m_extraRCParams;

                        // Check if this is the 'skip' parameter
                        if (rcParam.compare(ASSET_PROCESSOR_CONFIG_KEYWORD_SKIP) == 0)
                        {
                            skippedByPlatform = true;
                        }
                        // The recognizer's builder id must match the job requests' builder id
                        else if (recognizer->m_builderId.compare(requestedBuilderID) == 0)
                        {
                            AssetBuilderSDK::JobDescriptor descriptor;
                            Internal::PopulateCommonDescriptorParams(descriptor, iterPlatformSpec.key(), iterPlatformSpec.value(), recognizer);
                            // Job Parameter Value can be any arbitrary string since we are relying on the key to lookup
                            // the parameter in the process job
                            descriptor.m_jobParameters[recognizer->m_paramID] = descriptor.m_jobKey;

                            response.m_createJobOutputs.push_back(descriptor);
                            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
                        }
                    }
                }

                // Adjust response if we did not get any jobs, but one or more platforms were marked as skipped
                if ((response.m_result == AssetBuilderSDK::CreateJobsResultCode::Failed) && (skippedByPlatform))
                {
                    response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
                }
            }
        }
    }


    void InternalRecognizerBasedBuilder::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);
        if (m_isShuttingDown)
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;

        if (request.m_jobDescription.m_jobParameters.empty())
        {
            AZ_TracePrintf(AssetProcessor::ConsoleChannel,
                "Job request for %s in builder %s missing job parameters.",
                request.m_sourceFile.c_str(),
                BUILDER_ID_RC.GetId().toUtf8().data());

            return;
        }

        for (auto jobParam = request.m_jobDescription.m_jobParameters.begin();
             jobParam != request.m_jobDescription.m_jobParameters.end();
             jobParam++)
        {
            if (jobCancelListener.IsCancelled())
            {
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                return;
            }

            if (m_assetRecognizerDictionary.find(jobParam->first) == m_assetRecognizerDictionary.end())
            {
                AZ_TracePrintf(AssetProcessor::ConsoleChannel,
                    "Job request for %s in builder %s has invalid job parameter (%ld).",
                    request.m_sourceFile.c_str(),
                    BUILDER_ID_RC.GetId().toUtf8().data(),
                    jobParam->first);
                continue;
            }
            InternalAssetRecognizer* assetRecognizer = m_assetRecognizerDictionary[jobParam->first];
            if (!assetRecognizer->m_platformSpecsByPlatform.contains(request.m_jobDescription.GetPlatformIdentifier().c_str()))
            {
                // Skip due to platform restrictions
                continue;
            }
            QString rcParam = assetRecognizer->m_platformSpecsByPlatform[request.m_jobDescription.GetPlatformIdentifier().c_str()].m_extraRCParams;

            if (rcParam.compare(ASSET_PROCESSOR_CONFIG_KEYWORD_COPY) == 0)
            {
                ProcessCopyJob(request, assetRecognizer->m_productAssetType, assetRecognizer->m_outputProductDependencies, jobCancelListener, response);
            }
            else if (rcParam.compare(ASSET_PROCESSOR_CONFIG_KEYWORD_SKIP) == 0)
            {
                // This should not occur because 'skipped' jobs should not be processed
                AZ_TracePrintf(AssetProcessor::DebugChannel, "Job ID %lld Failed, encountered an invalid 'skip' parameter during job processing\n", AssetProcessor::GetThreadLocalJobId());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            }
            else
            {
                // If the job fails due to a networking issue, we will attempt to retry RetriesForJobNetworkError times
                int retryCount = 0;

                do
                {
                    ++retryCount;
                    ProcessLegacyRCJob(request, rcParam, assetRecognizer->m_productAssetType, jobCancelListener, response);

                    AZ_Warning("RC Builder", response.m_resultCode != AssetBuilderSDK::ProcessJobResult_NetworkIssue, "RC.exe reported a network connection issue.  %s",
                        retryCount <= AssetProcessor::RetriesForJobNetworkError ? "Attempting to retry job." : "Maximum retry attempts exceeded, giving up.");
                } while (response.m_resultCode == AssetBuilderSDK::ProcessJobResult_NetworkIssue && retryCount <= AssetProcessor::RetriesForJobNetworkError);

            }

            if (jobCancelListener.IsCancelled())
            {
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            }

            if (response.m_resultCode != AssetBuilderSDK::ProcessJobResult_Success)
            {
                // If anything other than a success occurred, break out of the loop and report the failed job
                return;
            }

        }
    }

    void InternalRecognizerBasedBuilder::CreateLegacyRCJob(const AssetBuilderSDK::CreateJobsRequest& request, QString rcParam, AssetBuilderSDK::CreateJobsResponse& response)
    {
        const char* requestFileName = "createjobsRequest.xml";
        const char* responseFileName = "createjobsResponse.xml";
        const char* createJobsParam = "/createjobs";

        QDir watchDir(request.m_watchFolder.c_str());
        auto normalizedPath = watchDir.absoluteFilePath(request.m_sourceFile.c_str());

        QString workFolder;

        if (!AssetUtilities::CreateTempWorkspace(workFolder))
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Failed to create temporary workspace");
            return;
        }

        QString watchFolder = request.m_watchFolder.c_str();
        NativeLegacyRCCompiler::Result  rcResult;

        QDir workDir(workFolder);
        QString requestPath = workDir.absoluteFilePath(requestFileName);
        QString responsePath = workDir.absoluteFilePath(responseFileName);

        if (!AZ::Utils::SaveObjectToFile(requestPath.toStdString().c_str(), AZ::DataStream::ST_XML, &request))
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Failed to write CreateJobsRequest to file %s", requestPath.toStdString().c_str());
            return;
        }

        QString params = QString("%1=\"%2\"").arg(createJobsParam).arg(requestPath);

        //Platform and platform id are hard coded to PC because it doesn't matter, the actual platform info is in the CreateJobsRequest
        if ((!this->m_rcCompiler->Execute(normalizedPath, watchFolder, "pc", rcParam.append(params), workFolder, nullptr, rcResult)) || (rcResult.m_exitCode != 0))
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Job ID %lld Failed with exit code %d\n", AssetProcessor::GetThreadLocalJobId(), rcResult.m_exitCode);
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Failed;
            return;
        }

        if (AZ::Utils::LoadObjectFromFileInPlace(responsePath.toStdString().c_str(), response))
        {
            workDir.removeRecursively();
        }
    }

    bool InternalRecognizerBasedBuilder::SaveProcessJobRequestFile(const char* requestFileDir, const char* requestFileName, const AssetBuilderSDK::ProcessJobRequest& request)
    {
        AZStd::string finalFullPath;
        AzFramework::StringFunc::Path::Join(requestFileDir, requestFileName, finalFullPath);
        if (!AZ::Utils::SaveObjectToFile(finalFullPath.c_str(), AZ::DataStream::ST_XML, &request))
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Failed to write ProcessJobRequest to file %s", finalFullPath.c_str());
            return false;
        }

        return true;
    }

    bool InternalRecognizerBasedBuilder::LoadProcessJobResponseFile(const char* responseFileDir, const char* responseFileName, AssetBuilderSDK::ProcessJobResponse& response, bool& responseLoaded)
    {
        responseLoaded = false;
        AZStd::string finalFullPath;
        AzFramework::StringFunc::Path::Join(responseFileDir, responseFileName, finalFullPath);
        if (AZ::IO::FileIOBase::GetInstance()->Exists(finalFullPath.c_str()))
        {
            // make a new one in case of pollution.
            response = AssetBuilderSDK::ProcessJobResponse();
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Loading the response from '%s' emitted by RC.EXE.\n", finalFullPath.c_str());
            if (!AZ::Utils::LoadObjectFromFileInPlace(finalFullPath.c_str(), response))
            {
                // rc TRIED to make a response file and failed somehow!
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                AZ_Error(AssetProcessor::DebugChannel, false, "Failed to deserialize '%s' despite RC.EXE having written it.", finalFullPath.c_str());
                return false;
            }
            else
            {
                // we loaded it.
                responseLoaded = true;
            }
        }
        // either way, its not a failure if the file doesn't exist or loaded successfully
        return true;
    }

    void InternalRecognizerBasedBuilder::ProcessLegacyRCJob(const AssetBuilderSDK::ProcessJobRequest& request, QString rcParam,
        AZ::Uuid productAssetType, const AssetBuilderSDK::JobCancelListener& jobCancelListener, AssetBuilderSDK::ProcessJobResponse& response)
    {
        // Process this job
        QString inputFile = QString(request.m_fullPath.c_str());
        QString platformIdentifier = request.m_jobDescription.GetPlatformIdentifier().c_str();
        QString dest = request.m_tempDirPath.c_str();
        QString watchFolder = request.m_watchFolder.c_str();
        NativeLegacyRCCompiler::Result  rcResult;

        QDir workDir(dest);

        if (!SaveProcessJobRequestFile(dest.toUtf8().constData(), AssetBuilderSDK::s_processJobRequestFileName, request))
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        if ((!this->m_rcCompiler->Execute(inputFile, watchFolder, platformIdentifier, rcParam, dest, &jobCancelListener, rcResult)) || (rcResult.m_exitCode != 0))
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Job ID %lld Failed with exit code %d\n", AssetProcessor::GetThreadLocalJobId(), rcResult.m_exitCode);
            if (jobCancelListener.IsCancelled())
            {
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                return;
            }
            else if (rcResult.m_crashed)
            {
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Crashed;
                return;
            }
        }

        // did the rc Compiler output a response file?
        bool responseFromRCCompiler = false;
        if (!LoadProcessJobResponseFile(dest.toUtf8().constData(), AssetBuilderSDK::s_processJobResponseFileName, response, responseFromRCCompiler))
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        if (!responseFromRCCompiler)
        {
            if(rcResult.m_exitCode != 0)
            {
                // RC didn't crash and didn't write a response, but returned a failure code
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }
            else
            {
                // if the response was NOT loaded from a response file, we assume success (since RC did not crash or anything)
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            }
        }

        if (jobCancelListener.IsCancelled())
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }

        if (response.m_requiresSubIdGeneration)
        {
            ProcessRCResultFolder(dest, productAssetType, responseFromRCCompiler, response);
        }
    }

    void InternalRecognizerBasedBuilder::ProcessRCResultFolder(const QString &dest, const AZ::Uuid& productAssetType, bool responseFromRCCompiler, AssetBuilderSDK::ProcessJobResponse &response)
    {
        // Get all of the files from the dest folder
        QFileInfoList   originalFiles = GetFilesInDirectory(dest);
        QFileInfoList   filteredFiles;

        // Filter out the log files and add to the result products
        AZStd::unordered_set<AZ::u32> m_alreadyAssignedSubIDs;

        bool hasSubIdCollision = false;

        // for legacy compatibility, we generate the list of output Products ourselves so that we can
        // assign legacy SubIDs to them:
        AZStd::vector<AssetBuilderSDK::JobProduct> generatedOutputProducts;

        for (const auto& file : originalFiles)
        {
            if (MatchTempFileToSkip(file.fileName()))
            {
                AZ_TracePrintf(AssetProcessor::DebugChannel, "RC created temporary file: (%s), ignoring.\n", file.absoluteFilePath().toUtf8().data());
                continue;
            }

            AZStd::string productName = file.fileName().toUtf8().constData();

            // convert to abs path:
            if (AzFramework::StringFunc::Path::IsRelative(productName.c_str()))
            {
                // convert to absolute path.
                AZStd::string joinedPath;
                AzFramework::StringFunc::Path::Join(dest.toUtf8().constData(), productName.c_str(), joinedPath);
                productName.swap(joinedPath);
            }
            else
            {
                AzFramework::StringFunc::Path::Normalize(productName);
            }

            // this kind of job can output multiple products.
            // we are going to generate SUBIds for them if they collide, here!
            // ideally, the builder SDK builder written for this asset type would deal with it.

            AZ_TracePrintf(AssetProcessor::DebugChannel, "RC created product file: (%s).\n", productName.c_str());
            generatedOutputProducts.push_back(AssetBuilderSDK::JobProduct(productName, productAssetType));
            hasSubIdCollision |= (m_alreadyAssignedSubIDs.insert(generatedOutputProducts.back().m_productSubID).second == false); // insert returns pair<iter, bool> where the bool is false if it was already there.
        }

        // now fix any subid collisions, but only if we have an actual collision.
        // previously these would be the real subids, but now they are legacy subids if the response was given.
        if (hasSubIdCollision)
        {
            m_alreadyAssignedSubIDs.clear();
            for (AssetBuilderSDK::JobProduct& product : generatedOutputProducts)
            {
                AZ_TracePrintf("RC Builder", "SubId collision detected for product file: (%s).\n", product.m_productFileName.c_str());
                AZ::u32 seedValue = 0;
                while (m_alreadyAssignedSubIDs.find(product.m_productSubID) != m_alreadyAssignedSubIDs.end())
                {
                    // its already in!  pick another one.  For now, lets pick something based on the name so that ordering doesn't mess it up
                    QFileInfo productFileInfo(product.m_productFileName.c_str());
                    QString filePart = productFileInfo.fileName(); // the file part only (blah.dds) - not the path.
                    AZ::u32 fullCRC = AZ::Crc32(filePart.toUtf8().data());
                    AZ::u32 maskedCRC = (fullCRC + seedValue) & AssetBuilderSDK::SUBID_MASK_ID;

                    // preserve the LOD and the other flags, but replace the CRC:
                    product.m_productSubID = AssetBuilderSDK::ConstructSubID(maskedCRC, AssetBuilderSDK::GetSubID_LOD(product.m_productSubID), product.m_productSubID);
                    ++seedValue;
                }

                m_alreadyAssignedSubIDs.insert(product.m_productSubID);
            }
        }

        // now that we have generated both the legacy product subIDs and have potentially gotten a response from RC.EXE, we reconcile them
        if (!responseFromRCCompiler)
        {
            // If we get here, it means that RC.EXE did no generate a response file for us, so its 100% up to our heuristic to decide on subIds.
            response.m_outputProducts = generatedOutputProducts;

            // its fine for RC to decide there are no outputs.  The only factor is what its exit code is.
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        }
        else
        {
            // otherwise, if we get here it means that RC.EXE output a valid response file for us, including a list of canonical products
            // all we need to do thus is match up the generated products with the ones it made to fill out the legacy SubID field.

            // this is also our opportunity to clean and check for problems such as duplicate product files and duplicate subIDs.
            AZStd::unordered_set<AZStd::string> productsToCheckForDuplicates;
            AZStd::unordered_map<AZ::u32, AZStd::string> subIdsToCheckForDuplicates;

            for (AssetBuilderSDK::JobProduct& product : response.m_outputProducts)
            {
                AZStd::string productName = product.m_productFileName;

                if (productName.empty())
                {
                    AZ_Error(AssetProcessor::DebugChannel, false, "The RC Builder responded with a processJobResult.xml but that xml contained an empty product name.");
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return; // its a failure to do this!
                }

                // now clean it, normalize it, and make it absolute so that its unambiguous.
                // do not convert its case - this has to work on Operating Systems which have case sensitivy.
                // we'll insert a lowercase copy into the set to check.
                if (AzFramework::StringFunc::Path::IsRelative(productName.c_str()))
                {
                    // convert to absolute path.
                    AZStd::string joinedPath;
                    AzFramework::StringFunc::Path::Join(dest.toUtf8().constData(), productName.c_str(), joinedPath);
                    productName.swap(joinedPath);
                }

                // update it in the structure to be absolute normalized path.
                product.m_productFileName = productName;

                AZStd::string productNameLowerCase = productName;
                AZStd::to_lower(productNameLowerCase.begin(), productNameLowerCase.end());

                if (subIdsToCheckForDuplicates.find(product.m_productSubID) != subIdsToCheckForDuplicates.end())
                {
                    AZStd::string collidingFileName = subIdsToCheckForDuplicates[product.m_productSubID];
                    AZ_Error(AssetProcessor::DebugChannel, false, "Duplicate subID emitted by builder: '%s' with subID 0x%08x was in the outputProducts array more than once.", collidingFileName.c_str(), product.m_productSubID);
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return; // its a failure to do this!
                }
                subIdsToCheckForDuplicates.insert(product.m_productSubID);

                if (productsToCheckForDuplicates.find(productNameLowerCase) != productsToCheckForDuplicates.end())
                {
                    AZ_Error(AssetProcessor::DebugChannel, false, "Duplicate product emitted by builder: '%s' was in the outputProducts array more than once.", productName.c_str());
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return; // its a failure to do this!
                }
                productsToCheckForDuplicates.insert(productNameLowerCase);

                // we are through the gauntlet.  Now reconcile this with the previously generated legacy subIDs.
                for (AssetBuilderSDK::JobProduct& generatedProduct : generatedOutputProducts)
                {
                    AZStd::string generatedPath = generatedProduct.m_productFileName;
                    // (this is already absolute path and already normalized)

                    // this is icase compare and is being done in utf8 strings.
                    if (AzFramework::StringFunc::Equal(generatedPath.c_str(), productName.c_str()))
                    {
                        // found it.
                        if (AZStd::find(product.m_legacySubIDs.begin(), product.m_legacySubIDs.end(), generatedProduct.m_productSubID) == product.m_legacySubIDs.end())
                        {
                            product.m_legacySubIDs.push_back(generatedProduct.m_productSubID);
                        }
                    }
                }
            }
        }
    }

    void InternalRecognizerBasedBuilder::ProcessCopyJob(
        const AssetBuilderSDK::ProcessJobRequest& request,
        AZ::Uuid productAssetType,
        bool outputProductDependencies,
        const AssetBuilderSDK::JobCancelListener& jobCancelListener,
        AssetBuilderSDK::ProcessJobResponse& response)
    {
        AssetBuilderSDK::JobProduct jobProduct(request.m_fullPath, productAssetType);

        if(!outputProductDependencies)
        {
            jobProduct.m_dependenciesHandled = true; // Copy jobs are meant to be used for assets that have no dependencies and just need to be copied.
        }

        response.m_outputProducts.push_back(jobProduct);
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;

        if (jobCancelListener.IsCancelled())
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }
        // Temporary solution to get around the fact that we don't have job dependencies
        TempSolution_TouchCopyJobActivity();

        if (outputProductDependencies)
        {
            // Launch the external process when it requires the old cry code to parse a specific type of asset
            QString rcParam = QString("/copyonly /outputproductdependencies /targetroot");
            rcParam = QString("%1=\"%2\"").arg(rcParam).arg(request.m_tempDirPath.c_str());
            ProcessLegacyRCJob(request, rcParam, productAssetType, jobCancelListener, response);
        }
    }

    QFileInfoList InternalRecognizerBasedBuilder::GetFilesInDirectory(const QString& directoryPath)
    {
        QDir            workingDir(directoryPath);
        QFileInfoList   filesInDir(workingDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files));
        return filesInDir;
    }


    bool InternalRecognizerBasedBuilder::MatchTempFileToSkip(const QString& outputFilename)
    {
        // List of specific files to skip
        static const char* s_fileNamesToSkip[] = {
            AssetBuilderSDK::s_processJobRequestFileName,
            AssetBuilderSDK::s_processJobResponseFileName,
            "rc_createdfiles.txt",
            "rc_log.log",
            "rc_log_warnings.log",
            "rc_log_errors.log"
        };

        for (const char* filenameToSkip : s_fileNamesToSkip)
        {
            if (QString::compare(outputFilename, filenameToSkip, Qt::CaseInsensitive) == 0)
            {
                return true;
            }
        }

        // List of specific file name patters to skip
        static const QString s_filePatternsToSkip[] = {
            QString(".*\\.\\$.*"),
            QString("log.*\\.txt")
        };

        for (const QString& patternsToSkip : s_filePatternsToSkip)
        {
            QRegExp skipRegex(patternsToSkip, Qt::CaseInsensitive, QRegExp::RegExp);
            if (skipRegex.exactMatch(outputFilename))
            {
                return true;
            }
        }

        return false;
    }


    void InternalRecognizerBasedBuilder::RegisterInternalAssetRecognizerToMap(const AssetRecognizer& assetRecognizer, const QString& builderId, QHash<QString, AssetPlatformSpec>& sourceAssetPlatformSpecs, QHash<QString, InternalAssetRecognizerList>& internalRecognizerListByType)
    {
        // this is called to say that the internal builder with builderID is to handle assets recognized by the givne recognizer.
        InternalAssetRecognizer* newAssetRecognizer = new InternalAssetRecognizer(assetRecognizer, builderId, sourceAssetPlatformSpecs);
        // the list is keyed off the builderID.
        internalRecognizerListByType[builderId].push_back(newAssetRecognizer);
    }

    void InternalRecognizerBasedBuilder::BuildInternalAssetRecognizersByType(const RecognizerContainer& assetRecognizers, QHash<QString, InternalAssetRecognizerList>& internalRecognizerListByType)
    {
        // Go through each asset recognizer's platform specs to determine which type bucket to create and put the converted internal
        // assert recognizer into
        for (const AssetRecognizer& assetRecognizer : assetRecognizers)
        {
            // these hashes are keyed on the same key as the incoming asset recognizers list, which is
            // [ name in ini file ] --> [regognizer details]
            // so like "rc png" --> [details].  Specifically, the QString key is the name of the entry in the INI file and NOT a platform name.
            QHash<QString, AssetPlatformSpec>   copyAssetPlatformSpecs;
            QHash<QString, AssetPlatformSpec>   skipAssetPlatformSpecs;
            QHash<QString, AssetPlatformSpec>   rcAssetPlatformSpecs;

            // Go through the global asset recognizers and split them by operation keywords if they exist or by the main rc param
            for (auto iterSrcPlatformSpec = assetRecognizer.m_platformSpecs.begin();
                 iterSrcPlatformSpec != assetRecognizer.m_platformSpecs.end();
                 iterSrcPlatformSpec++)
            {
                if (iterSrcPlatformSpec->m_extraRCParams.compare(BUILDER_ID_COPY.GetRcParam()) == 0)
                {
                    copyAssetPlatformSpecs[iterSrcPlatformSpec.key()] = iterSrcPlatformSpec.value();
                }
                else if (iterSrcPlatformSpec->m_extraRCParams.compare(BUILDER_ID_SKIP.GetRcParam()) == 0)
                {
                    skipAssetPlatformSpecs[iterSrcPlatformSpec.key()] = iterSrcPlatformSpec.value();
                }
                else
                {
                    rcAssetPlatformSpecs[iterSrcPlatformSpec.key()] = iterSrcPlatformSpec.value();
                }
            }

            // Create separate internal asset recognizers based on whether or not they were detected
            if (copyAssetPlatformSpecs.size() > 0)
            {
                RegisterInternalAssetRecognizerToMap(assetRecognizer, BUILDER_ID_COPY.GetId(), copyAssetPlatformSpecs, internalRecognizerListByType);
            }
            if (skipAssetPlatformSpecs.size() > 0)
            {
                RegisterInternalAssetRecognizerToMap(assetRecognizer, BUILDER_ID_SKIP.GetId(), skipAssetPlatformSpecs, internalRecognizerListByType);
            }
            if (rcAssetPlatformSpecs.size() > 0)
            {
                RegisterInternalAssetRecognizerToMap(assetRecognizer, BUILDER_ID_RC.GetId(), rcAssetPlatformSpecs, internalRecognizerListByType);
            }
        }
    }
}

