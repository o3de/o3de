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

    //! Special configuration keyword to mark a asset pattern for skipping
    const QString ASSET_PROCESSOR_CONFIG_KEYWORD_SKIP = "skip";

    //! Special configuration keyword to mark a asset pattern for copying
    const QString ASSET_PROCESSOR_CONFIG_KEYWORD_COPY = "copy";

    namespace Internal
    {
        void PopulateCommonDescriptorParams(AssetBuilderSDK::JobDescriptor& descriptor, const QString& platformIdentifier, const AssetInternalSpec& platformSpec, const InternalAssetRecognizer* const recognizer)
        {
            descriptor.m_jobKey = recognizer->m_name;
            descriptor.SetPlatformIdentifier(platformIdentifier.toUtf8().constData());
            descriptor.m_priority = recognizer->m_priority;
            descriptor.m_checkExclusiveLock = recognizer->m_testLockSource;

            AZStd::string extraInformationForFingerprinting;
            extraInformationForFingerprinting.append(platformSpec == AssetInternalSpec::Copy ? "copy" : "skip");
            extraInformationForFingerprinting.append(recognizer->m_version);

            // if we have specified the product asset type, changing it should cause
            if (!recognizer->m_productAssetType.IsNull())
            {
                char typeAsString[64] = { 0 };
                recognizer->m_productAssetType.ToString(typeAsString, AZ_ARRAY_SIZE(typeAsString));
                extraInformationForFingerprinting.append(typeAsString);
            }

            descriptor.m_priority = recognizer->m_priority;

            descriptor.m_additionalFingerprintInfo = extraInformationForFingerprinting;

            const bool isCopyJob = (platformSpec == AssetInternalSpec::Copy);

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

        void RegisterInternalAssetRecognizerToMap(
            const AssetRecognizer& assetRecognizer,
            const AZStd::string& builderId,
            AZStd::unordered_map<AZStd::string, AssetInternalSpec>& sourceAssetInternalSpecs,
            AZStd::unordered_map<AZStd::string, InternalAssetRecognizerList>& internalRecognizerListByType)
        {
            // this is called to say that the internal builder with builderID is to handle assets recognized by the given recognizer.
            InternalAssetRecognizer* newAssetRecognizer = new InternalAssetRecognizer(assetRecognizer, builderId, sourceAssetInternalSpecs);
            // the list is keyed off the builderID.
            internalRecognizerListByType[builderId].push_back(newAssetRecognizer);
        }

        // Split all of the asset recognizers from a container into buckets based on their specific builder action type
        void BuildInternalAssetRecognizersByType(const RecognizerContainer& assetRecognizers, AZStd::unordered_map<AZStd::string, InternalAssetRecognizerList>& internalRecognizerListByType)
        {
            // Go through each asset recognizer's platform specs to determine which type bucket to create and put the converted internal
            // assert recognizer into
            for (const auto& assetRecognizer : assetRecognizers)
            {
                // these hashes are keyed on the same key as the incoming asset recognizers list, which is
                // [ name in ini file ] --> [regognizer details]
                // so like "rc png" --> [details].  Specifically, the QString key is the name of the entry in the INI file and NOT a platform name.
                AZStd::unordered_map<AZStd::string, AssetInternalSpec> copyAssetInternalSpecs;
                AZStd::unordered_map<AZStd::string, AssetInternalSpec> skipAssetInternalSpecs;

                // Go through the global asset recognizers and split them by operation keywords if they exist or by the main rc param
                for (auto iterSrcPlatformSpec = assetRecognizer.second.m_platformSpecs.begin();
                    iterSrcPlatformSpec != assetRecognizer.second.m_platformSpecs.end();
                    iterSrcPlatformSpec++)
                {
                    auto platformSpec = (*iterSrcPlatformSpec).second;
                    auto& platformId = (*iterSrcPlatformSpec).first;

                    if (platformSpec == AssetInternalSpec::Copy)
                    {
                        copyAssetInternalSpecs[platformId] = platformSpec;
                    }
                    else if (platformSpec == AssetInternalSpec::Skip)
                    {
                        skipAssetInternalSpecs[platformId] = platformSpec;
                    }
                }

                // Create separate internal asset recognizers based on whether or not they were detected
                if (copyAssetInternalSpecs.size() > 0)
                {
                    RegisterInternalAssetRecognizerToMap(
                        assetRecognizer.second,
                        BUILDER_ID_COPY.GetId().toUtf8().data(),
                        copyAssetInternalSpecs,
                        internalRecognizerListByType);
                }
                if (skipAssetInternalSpecs.size() > 0)
                {
                    RegisterInternalAssetRecognizerToMap(
                        assetRecognizer.second,
                        BUILDER_ID_SKIP.GetId().toUtf8().data(),
                        skipAssetInternalSpecs,
                        internalRecognizerListByType);
                }
            }
        }
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
    const BuilderIdAndName  BUILDER_ID_SKIP("Internal Skip Builder", "A033AF24-5041-4E24-ACEC-161A2E522BB6", BuilderIdAndName::Type::UNREGISTERED_BUILDER, ASSET_PROCESSOR_CONFIG_KEYWORD_SKIP);

    const QHash<QString, BuilderIdAndName> ALL_INTERNAL_BUILDER_BY_ID =
    {
        { BUILDER_ID_COPY.GetId(), BUILDER_ID_COPY },
        { BUILDER_ID_SKIP.GetId(), BUILDER_ID_SKIP }
    };

    InternalAssetRecognizer::InternalAssetRecognizer(const AssetRecognizer& src, const AZStd::string& builderId, const AZStd::unordered_map<AZStd::string, AssetInternalSpec>& AssetInternalSpecByPlatform)
        : AssetRecognizer(src.m_name, src.m_testLockSource, src.m_priority, src.m_isCritical, src.m_supportsCreateJobs, src.m_patternMatcher, src.m_version, src.m_productAssetType, src.m_outputProductDependencies, src.m_checkServer)
        , m_builderId(builderId)
    {
        // AssetInternalSpecByPlatform is a hash table like
        // "pc" --> (settings to compile on pc)
        // "ios" --> settings to compile on ios)
        // and so is m_platformSpecsByPlatform
        m_platformSpecsByPlatform = AssetInternalSpecByPlatform;
        m_paramID = CalculateCRC();
    }

    AZ::u32 InternalAssetRecognizer::CalculateCRC() const
    {
        AZ::Crc32 crc;

        crc.Add(m_name);
        crc.Add(m_builderId);
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
    }

    bool InternalRecognizerBasedBuilder::Initialize(const RecognizerConfiguration& recognizerConfig)
    {
        InitializeAssetRecognizers(recognizerConfig.GetAssetRecognizerContainer());
        return true;
    }


    void InternalRecognizerBasedBuilder::InitializeAssetRecognizers(const RecognizerContainer& assetRecognizers)
    {
        // Split the asset recognizers that were scanned in into 'buckets' for each of the 3 builder ids based on
        // either the custom fixed rc params or the standard rc param ('copy','skip', or others)

        AZStd::unordered_map<AZStd::string, InternalAssetRecognizerList> internalRecognizerListByType;
        Internal::BuildInternalAssetRecognizersByType(assetRecognizers, internalRecognizerListByType);

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

        for (const auto& internalRecognizerList : internalRecognizerListByType)
        {
            QString builderId = internalRecognizerList.first.c_str();
            const BuilderIdAndName& builderInfo = m_builderById[builderId];
            QString builderName = builderInfo.GetName();
            AZStd::vector<AssetBuilderSDK::AssetBuilderPattern> builderPatterns;

            bool supportsCreateJobs = false;
            // intentionally using a set here, as we want it to be the same order each time for hashing.
            AZStd::set<AZStd::string> fingerprintRelevantParameters;

            for (auto* internalAssetRecognizer : internalRecognizerList.second)
            {
                // so referring to the structure explanation above, internalAssetRecognizer is
                // one of those objects that has the RegEx in it, (along with list of commands to apply per platform)
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

                for (const auto& iteratorValue : internalAssetRecognizer->m_platformSpecsByPlatform)
                {
                    fingerprintRelevantParameters.insert(
                        AZStd::string::format(
                            "%s-%s",
                            iteratorValue.first.c_str(),
                            iteratorValue.second == AssetInternalSpec::Copy ? "copy" : "skip"));
                }

                // note that the version number must be included here, despite the builder dirty-check function taking version into account
                // because the RC Builder is just a single builder (with version#0) that defers to these "internal" builders when called upon.
                if (!internalAssetRecognizer->m_version.empty())
                {
                    fingerprintRelevantParameters.insert(internalAssetRecognizer->m_version);
                }
                fingerprintRelevantParameters.insert(internalAssetRecognizer->m_productAssetType.ToString<AZStd::string>());

                // Register the recognizer
                builderPatterns.push_back(internalAssetRecognizer->m_patternMatcher.GetBuilderPattern());
                m_assetRecognizerDictionary[internalAssetRecognizer->m_paramID] = internalAssetRecognizer;
                AZ_TracePrintf(AssetProcessor::DebugChannel, "Registering %s as a %s\n", internalAssetRecognizer->m_name.c_str(),
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
                AssetBuilderRegistrationBus::Broadcast(&AssetBuilderRegistrationBus::Events::UnRegisterBuilderDescriptor, builderUuid);
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

        // First pass, check for simple jobs like 'copy'
        for (const InternalAssetRecognizer* recognizer : recognizers)
        {
            bool skippedByPlatform = false;

            // Iterate through the platform specific specs and apply the ones that match the platform flag
            for (const auto& platformSpec : recognizer->m_platformSpecsByPlatform)
            {
                if (request.HasPlatform(platformSpec.first.c_str()))
                {
                    // Check if this is the 'skip' parameter
                    if (platformSpec.second == AssetInternalSpec::Skip)
                    {
                        skippedByPlatform = true;
                    }
                    // The recognizer's builder id must match the job requests' builder id
                    else if (requestedBuilderID.compare(recognizer->m_builderId.c_str()) == 0)
                    {
                        AssetBuilderSDK::JobDescriptor descriptor;
                        Internal::PopulateCommonDescriptorParams(descriptor, platformSpec.first.c_str(), platformSpec.second, recognizer);
                        // Job Parameter Value can be any arbitrary string since we are relying on the key to lookup
                        // the parameter in the process job
                        descriptor.m_jobParameters[recognizer->m_paramID] = descriptor.m_jobKey;

                        response.m_createJobOutputs.push_back(descriptor);
                        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
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
                request.m_builderGuid.ToFixedString().c_str());

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
                    request.m_builderGuid.ToFixedString().c_str(),
                    jobParam->first);
                continue;
            }
            InternalAssetRecognizer* assetRecognizer = m_assetRecognizerDictionary[jobParam->first];
            if (!assetRecognizer->m_platformSpecsByPlatform.contains(request.m_jobDescription.GetPlatformIdentifier().c_str()))
            {
                // Skip due to platform restrictions
                continue;
            }

            auto internalJobType = assetRecognizer->m_platformSpecsByPlatform[request.m_jobDescription.GetPlatformIdentifier().c_str()];
            if (internalJobType == AssetInternalSpec::Copy)
            {
                ProcessCopyJob(request, assetRecognizer->m_productAssetType, assetRecognizer->m_outputProductDependencies, jobCancelListener, response);
            }
            else if (internalJobType == AssetInternalSpec::Skip)
            {
                // This should not occur because 'skipped' jobs should not be processed
                AZ_TracePrintf(AssetProcessor::DebugChannel, "Job ID %lld Failed, encountered an invalid 'skip' parameter during job processing\n", AssetProcessor::GetThreadLocalJobId());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
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
}

