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
    extern const BuilderIdAndName  BUILDER_ID_SKIP;
    extern const QHash<QString, BuilderIdAndName>  INTERNAL_BUILDER_BY_ID;

    //! Internal Builder version of the asset recognizer structure that is read in from the platform configuration class
    struct InternalAssetRecognizer
        : public AssetRecognizer
    {
        AZ_CLASS_ALLOCATOR(InternalAssetRecognizer, AZ::SystemAllocator)
        InternalAssetRecognizer(const AssetRecognizer& src, const AZStd::string& builderId, const AZStd::unordered_map<AZStd::string, AssetInternalSpec>& assetPlatformSpecByPlatform);
        InternalAssetRecognizer(const InternalAssetRecognizer& src) = default;

        AZ::u32 CalculateCRC() const;

        //! Map of platform specs based on the identifier of the platform
        AZStd::unordered_map<AZStd::string, AssetInternalSpec> m_platformSpecsByPlatform;

        //! unique id that is generated for each unique internal asset recognizer
        //! which can be used as the key for the job parameter map (see AssetBuilderSDK::JobParameterMap)
        AZ::u32 m_paramID;

        //! Keep track which internal builder type this recognizer is for
        const AZStd::string m_builderId;
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

    protected:
        //! Constructor to initialize the internal builders and a general internal builder uuid that is used for bus
        //! registration.  This constructor is helpful for deriving other classes from this builder for purposes like
        //! unit testing.
        InternalRecognizerBasedBuilder(QHash<QString, BuilderIdAndName> inputBuilderByIdMap, AZ::Uuid internalBuilderUuid);

        void ProcessCopyJob(
            const AssetBuilderSDK::ProcessJobRequest& request,
            AZ::Uuid productAssetType,
            bool outputProductDependencies,
            const AssetBuilderSDK::JobCancelListener& jobCancelListener,
            AssetBuilderSDK::ProcessJobResponse& response);

        // overridable so we can unit-test override it.
        virtual QFileInfoList GetFilesInDirectory(const QString& directoryPath);

        volatile bool                           m_isShuttingDown;
        InternalRecognizerContainer             m_assetRecognizerDictionary;

        QHash<QString, BuilderIdAndName>        m_builderById;

        //! UUid for the internal recognizer for logging purposes since the this
        //! class manages multiple internal build uuids
        AZ::Uuid                                m_internalRecognizerBuilderUuid;

    };
} // namespace AssetProcessor
