/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "../utilities/ApplicationManagerAPI.h"
#include "native/AssetManager/assetProcessorManager.h"
#include "native/utilities/PlatformConfiguration.h"

namespace AssetProcessor
{
    struct RCCompiler
    {
        //! RC.exe execution result
        struct Result
        {
            Result(int exitCode, bool crashed, const QString& outputDir);
            Result() = default;
            int     m_exitCode = 1;
            bool    m_crashed = false;
            QString m_outputDir;
        };

        virtual ~RCCompiler() = default;
        virtual bool Initialize() = 0;
        virtual bool Execute(const QString& inputFile, const QString& watchFolder, const QString& platformIdentifier, const QString& params,
            const QString& dest, const AssetBuilderSDK::JobCancelListener* jobCancelListener, Result& result) const = 0;
        virtual void RequestQuit() = 0;
    };

    //! Worker class to handle shell execution of the legacy rc.exe compiler
    class NativeLegacyRCCompiler
        : public RCCompiler
    {
    public:
        NativeLegacyRCCompiler();

        bool Initialize() override;
        bool Execute(const QString& inputFile, const QString& watchFolder, const QString& platformIdentifier, const QString& params, const QString& dest,
            const AssetBuilderSDK::JobCancelListener* jobCancelListener, Result& result) const override;
        static QString BuildCommand(const QString& inputFile, const QString& watchFolder, const QString& platformIdentifier, const QString& params, const QString& dest);
        void RequestQuit()  override;
    private:
        static const int            s_maxSleepTime;
        static const unsigned int   s_jobMaximumWaitTime;
        bool                        m_resourceCompilerInitialized;
        volatile bool               m_requestedQuit;
    };

    //! Internal structure that consolidates a internal builder id, its name, and its custom fixed rc param match
    class BuilderIdAndName
    {
    public:
        enum class Type
        {
            REGISTERED_BUILDER,
            UNREGISTERED_BUILDER
        };

        BuilderIdAndName() = default;
        BuilderIdAndName(QString builderName, QString builderId, Type type, QString rcParam = QString(""));
        BuilderIdAndName(const BuilderIdAndName& src) = default;

        BuilderIdAndName& operator=(const AssetProcessor::BuilderIdAndName& src);


        const QString& GetName() const;
        bool GetUuid(AZ::Uuid& builderUuid) const;
        const QString& GetRcParam() const;
        const QString& GetId() const;
        const Type GetType() const;
    private:
        Type    m_type = Type::UNREGISTERED_BUILDER;
        QString m_builderName;
        QString m_builderId;
        QString m_rcParam;
    };

    extern const BuilderIdAndName  BUILDER_ID_COPY;
    extern const BuilderIdAndName  BUILDER_ID_RC;
    extern const BuilderIdAndName  BUILDER_ID_SKIP;
    extern const QHash<QString, BuilderIdAndName>  INTERNAL_BUILDER_BY_ID;

    //! Internal Builder version of the asset recognizer structure that is read in from the platform configuration class
    struct InternalAssetRecognizer
        : public AssetRecognizer
    {
        InternalAssetRecognizer(const AssetRecognizer& src, const QString& builderId, const QHash<QString, AssetPlatformSpec>& assetPlatformSpecByPlatform);
        InternalAssetRecognizer(const InternalAssetRecognizer& src) = default;

        AZ::u32 CalculateCRC() const;

        //! Map of platform specs based on the identifier of the platform
        QHash<QString, AssetPlatformSpec>           m_platformSpecsByPlatform;

        //! unique id that is generated for each unique internal asset recognizer
        //! which can be used as the key for the job parameter map (see AssetBuilderSDK::JobParameterMap)
        AZ::u32                                 m_paramID;

        //! Keep track which internal builder type this recognizer is for
        const QString                          m_builderId;
    };
    typedef QHash<AZ::u32, InternalAssetRecognizer*> InternalRecognizerContainer;
    typedef QList<const InternalAssetRecognizer*> InternalRecognizerPointerContainer;
    typedef AZStd::function<void(const AssetBuilderSDK::AssetBuilderDesc& builderDesc)> RegisterBuilderDescCallback;
    typedef AZStd::list<InternalAssetRecognizer*> InternalAssetRecognizerList;

    class InternalRecognizerBasedBuilder
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        //! Constructor to initialize the internal based builder to a present set of internal builders and fixed bus id
        InternalRecognizerBasedBuilder();

        virtual ~InternalRecognizerBasedBuilder();

        AssetBuilderSDK::AssetBuilderDesc CreateBuilderDesc(const QString& builderId, const AZStd::vector<AssetBuilderSDK::AssetBuilderPattern>& builderPatterns);

        void ShutDown() override;

        virtual bool Initialize(const RecognizerConfiguration& recognizerConfig);
        virtual void InitializeAssetRecognizers(const RecognizerContainer& assetRecognizers);
        virtual void UnInitialize();

        //! Returns false if there were no matches, otherwise returns true
        virtual bool GetMatchingRecognizers(const AZStd::vector<AssetBuilderSDK::PlatformInfo>& platformInfos, const QString& fileName, InternalRecognizerPointerContainer& output) const;

        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

        static bool MatchTempFileToSkip(const QString& outputFilename);

        static void RegisterInternalAssetRecognizerToMap(
            const AssetRecognizer& assetRecognizer,
            const QString& builderId,
            QHash<QString, AssetPlatformSpec>& sourceAssetPlatformSpecs,
            QHash<QString, InternalAssetRecognizerList>& internalRecognizerListByType);

        //! Split all of the asset recognizers from a container into buckets based on their specific builder action type
        static void BuildInternalAssetRecognizersByType(
            const RecognizerContainer& assetRecognizers,
            QHash<QString, InternalAssetRecognizerList>& internalRecognizerListByType);

    protected:
        //! Constructor to initialize the internal builders and a general internal builder uuid that is used for bus
        //! registration.  This constructor is helpful for deriving other classes from this builder for purposes like
        //! unit testing.
        InternalRecognizerBasedBuilder(QHash<QString, BuilderIdAndName> inputBuilderByIdMap, AZ::Uuid internalBuilderUuid);

        // overridden in unit tests.  Searches for RC.EXE
        virtual bool FindRC(QString& rcAbsolutePathOut);

        void CreateLegacyRCJob(
            const AssetBuilderSDK::CreateJobsRequest& request,
            QString rcParam,
            AssetBuilderSDK::CreateJobsResponse& response);

        void ProcessLegacyRCJob(
            const AssetBuilderSDK::ProcessJobRequest& request,
            QString rcParam,
            AZ::Uuid productAssetType,
            const AssetBuilderSDK::JobCancelListener& jobCancelListener,
            AssetBuilderSDK::ProcessJobResponse& response);

        //! Given a folder (dest) containing the aftermath of a RC process, generate a response structure.
        //! if responseFromRCCompiler is true it means that the response is an already-populated response struct
        //! that was loaded from a response file and we should just append legacy SubIDs to it (responses are used INSTEAD of
        //! heuristics).
        //! otherwise it means that we know nothing about the files that were produced and should perform heuristics to determine.
        //! productAssetType is what uuid type (or nulls) to apply to the generated products for when responseFromRCCompiler is false.
        void ProcessRCResultFolder(const QString &dest, const AZ::Uuid& productAssetType, bool responseFromRCCompiler, AssetBuilderSDK::ProcessJobResponse &response);

        void ProcessCopyJob(
            const AssetBuilderSDK::ProcessJobRequest& request,
            AZ::Uuid productAssetType,
            bool outputProductDependencies,
            const AssetBuilderSDK::JobCancelListener& jobCancelListener,
            AssetBuilderSDK::ProcessJobResponse& response);

        // overridable so we can unit-test override it.
        virtual QFileInfoList GetFilesInDirectory(const QString& directoryPath);

        // overridable so we can unit-test override it.
        virtual bool SaveProcessJobRequestFile(const char* requestFileDir, const char* requestFileName, const AssetBuilderSDK::ProcessJobRequest& request);

        // returns false only if there is a critical failure.
        virtual bool LoadProcessJobResponseFile(const char* responseFileDir, const char* responseFileName, AssetBuilderSDK::ProcessJobResponse& response, bool& responseLoaded);

        AZStd::unique_ptr<RCCompiler>           m_rcCompiler;
        volatile bool                           m_isShuttingDown;
        InternalRecognizerContainer             m_assetRecognizerDictionary;

        QHash<QString, BuilderIdAndName>        m_builderById;

        //! UUid for the internal recognizer for logging purposes since the this
        //! class manages multiple internal build uuids
        AZ::Uuid                                m_internalRecognizerBuilderUuid;

    };
} // namespace AssetProcessor
