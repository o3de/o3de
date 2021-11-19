
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/AZStdContainers.inl>
#include <AzCore/std/string/regex.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/bitset.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzCore/std/string/string_view.h>
#include "AssetBuilderBusses.h"

/** 
* this define exists to turn on and off the support for legacy m_platformFlags and the concept of platforms as an enum
* If you want to upgrade your system to use the new platform tag system, you can turn this define off in order to strip out
* any references to the old stuff and cause compile-time errors anywhere your code tries to use the legacy API.
* It is recommended that you leave this on so that code besides your own code (for example, in 3rd-party gems) continues to function
* until the responsible party upgrades that code also.
*/
#define ENABLE_LEGACY_PLATFORMFLAGS_SUPPORT

namespace AZ
{
    class ComponentDescriptor;
    class Entity;
}

// This needs to be up here because it needs to be defined before the hash definition, and the hash needs to be defined before the first use (which occurs further down in this file)
namespace AssetBuilderSDK
{
    enum class ProductPathDependencyType : AZ::u32
    {
        SourceFile,
        ProductFile
    };

    /**
     * Product dependency information that the builder will send to the assetprocessor
     * Indicates a product asset that depends on another product based on the path
     * Should only be used by legacy systems.  Prefer ProductDependencies whenever possible
     */
    struct ProductPathDependency
    {
        AZ_CLASS_ALLOCATOR(ProductPathDependency, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(ProductPathDependency, "{2632bfae-7490-476f-9214-a6d1f02e6085}");

        //! Relative path to the asset dependency
        AZStd::string m_dependencyPath;

        /**
         * Indicates if the dependency path points to a source file or a product file
         * A dependency on a source file will be converted into dependencies on all product files produced from the source
         * It is preferable to depend on product files whenever possible to avoid introducing unintended dependencies
         */
        ProductPathDependencyType m_dependencyType = ProductPathDependencyType::ProductFile;

        ProductPathDependency() = default;
        ProductPathDependency(AZStd::string_view dependencyPath, ProductPathDependencyType dependencyType);

        bool operator==(const ProductPathDependency& rhs) const;

        static void Reflect(AZ::ReflectContext* context);
    };
}

namespace AZStd
{
    template<>
    struct hash<AssetBuilderSDK::ProductPathDependency>
    {
        using argument_type = AssetBuilderSDK::ProductPathDependency;
        using result_type = size_t;

        result_type operator() (const argument_type& dependency) const
        {
            size_t h = 0;
            hash_combine(h, dependency.m_dependencyPath);
            hash_combine(h, dependency.m_dependencyType);
            return h;
        }
    };
} // namespace AZStd

namespace AssetBuilderSDK
{
    namespace ComponentTags
    {
        //! Components with the AssetBuilder tag in their reflect data's attributes as AZ::Edit::Attributes::SystemComponetTags will automatically be created on AssetBuilder startup
        const static AZ::Crc32 AssetBuilder = AZ_CRC("AssetBuilder", 0xc739c7d7);
    }

    extern const char* const ErrorWindow; //Use this window name to log error messages.
    extern const char* const WarningWindow; //Use this window name to log warning messages.
    extern const char* const InfoWindow; //Use this window name to log info messages.

    extern const char* const s_processJobRequestFileName; //!< File name for having job requests send from the Asset Processor.
    extern const char* const s_processJobResponseFileName; //!< File name for having job responses returned to the Asset Processor.

    // SubIDs uniquely identify a particular output product of a specific source asset
    // currently we use a scheme where various bits of the subId (which is a 32 bit unsigned) are used to designate different things.
    // we may expand this into a 64-bit "namespace" by adding additional 32 bits at the front at some point, if it becomes necessary.
    extern const AZ::u32 SUBID_MASK_ID;         //!< mask is 0xFFFF - so you can have up to 64k subids from a single asset before you start running into the upper bits which are used for other reasons.
    extern const AZ::u32 SUBID_MASK_LOD_LEVEL;  //!< the LOD level can be masked up to 15 LOD levels (it also represents the MIP level).  note that it starts at 1.
    extern const AZ::u32 SUBID_LOD_LEVEL_SHIFT; //!< the shift to move the LOD level in its expected bits.
    extern const AZ::u32 SUBID_FLAG_DIFF;       //!< this is a 'diff' map.  It may have the alpha, and lod set too if its an alpha of a diff
    extern const AZ::u32 SUBID_FLAG_ALPHA;      //!< this is an alpha mip or alpha channel.

    //! extract only the ID using the above masks
    AZ::u32 GetSubID_ID(AZ::u32 packedSubId);
    //! extract only the LOD using the above masks.  note that it starts at 1, not 0.  0 would be the base asset.
    AZ::u32 GetSubID_LOD(AZ::u32 packedSubId);

    //! create a subid using the above masks.  Note that if you want to add additional bits such as DIFF or ALPHA, you must add them afterwards.
    //! fromsubindex contains an existing subindex to replace the LODs and SUBs but no other bits with.
    AZ::u32 ConstructSubID(AZ::u32 subIndex, AZ::u32 lodLevel, AZ::u32 fromSubIndex = 0);

    //! Initializes the serialization context with all the reflection information for AssetBuilderSDK structures
    //! Should be called on startup by standalone builders.  Builders run by AssetBuilder will have this set up already
    void InitializeSerializationContext();
    void InitializeBehaviorContext();

    //! This method is used for logging builder related messages/error
    //! Do not use this inside ProcessJob, use AZ_TracePrintF instead.  This is only for general messages about your builder, not for job-specific messages
    extern void BuilderLog(AZ::Uuid builderId, const char* message, ...);

#if defined(ENABLE_LEGACY_PLATFORMFLAGS_SUPPORT)
    /**
    * DEPRECATED  -  LEGACY - this is retained for code compatbility with previous versions.  Please just use the m_enabledPlatforms
    * structure in all new code.
    **/
    enum Platform : AZ::u32
    {
        Platform_NONE       = 0x00,
        Platform_PC         = 0x01,
        Platform_LINUX      = 0x02,
        Platform_ANDROID    = 0x04,
        Platform_IOS        = 0x08,
        Platform_MAC        = 0x10,
        Platform_PROVO      = 0x20,
        Platform_SALEM      = 0x40,
        Platform_JASPER     = 0x80,

        //! if you add a new platform entry to this enum, you must add it to allplatforms as well otherwise that platform would not be considered valid. 
        AllPlatforms = Platform_PC | Platform_LINUX | Platform_ANDROID | Platform_IOS | Platform_MAC | Platform_PROVO | Platform_SALEM | Platform_JASPER
    };
#endif // defined(ENABLE_LEGACY_PLATFORMFLAGS_SUPPORT)
    //! Map data structure to holder parameters that are passed into a job for ProcessJob requests.
    //! These parameters can optionally be set during the create job function of the builder so that they are passed along
    //! to the ProcessJobFunction.  The values (key and value) are arbitrary and is up to the builder on how to use them
    typedef AZStd::unordered_map<AZ::u32, AZStd::string> JobParameterMap;

    //! Callback function type for creating jobs from job requests
    typedef AZStd::function<void(const CreateJobsRequest& request, CreateJobsResponse& response)> CreateJobFunction;

    //! Callback function type for processing jobs from process job requests
    typedef AZStd::function<void(const ProcessJobRequest& request, ProcessJobResponse& response)> ProcessJobFunction;

    //! Structure defining the type of pattern to use to apply
    struct AssetBuilderPattern
    {
        AZ_CLASS_ALLOCATOR(AssetBuilderPattern, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(AssetBuilderPattern, "{A8818121-D106-495E-9776-11F59E897BAD}");

        enum PatternType
        {
            //! The pattern is a file wildcard pattern (glob)
            Wildcard,
            //! The pattern is a regular expression pattern
            Regex
        };

        AZStd::string m_pattern;
        PatternType   m_type{};

        AssetBuilderPattern() = default;
        AssetBuilderPattern(const AssetBuilderPattern& src) = default;
        AssetBuilderPattern(const AZStd::string& pattern, PatternType type);

        AZStd::string ToString() const;

        static void Reflect(AZ::ReflectContext* context);
    };

    //! This class represents a matching pattern that is based on AssetBuilderSDK::AssetBuilderPattern::PatternType, which can either be a regex
    //! pattern or a wildcard (glob) pattern
    class FilePatternMatcher
    {
    public:
        FilePatternMatcher() = default;
        explicit FilePatternMatcher(const AssetBuilderSDK::AssetBuilderPattern& pattern);
        FilePatternMatcher(const AZStd::string& pattern, AssetBuilderSDK::AssetBuilderPattern::PatternType type);
        FilePatternMatcher(const FilePatternMatcher& copy);

        typedef AZStd::regex RegexType;

        FilePatternMatcher& operator=(const FilePatternMatcher& copy);

        bool MatchesPath(const AZStd::string& assetPath) const;
        bool IsValid() const;
        AZStd::string GetErrorString() const;
        const AssetBuilderSDK::AssetBuilderPattern& GetBuilderPattern() const;

    protected:
        static bool ValidatePatternRegex(const AZStd::string& pattern);
        
        AssetBuilderSDK::AssetBuilderPattern    m_pattern;
        RegexType           m_regex;
        AZStd::string       m_errorString;
        bool                m_isRegex{};
        bool                m_isValid{};
    };

    //!Information that builders will send to the assetprocessor
    struct AssetBuilderDesc
    {
        AZ_CLASS_ALLOCATOR(AssetBuilderDesc, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(AssetBuilderDesc, "{7778EB3D-7B3B-4231-80C0-94C4226309AF}");

        enum class AssetBuilderType
        {
            Internal,   //! Internal Recognizer builders for example.  Internal Builders are created and run inside the AP.
            External    //! External builders are those located within gems that run inside an AssetBuilder application.
        };

        // you don't have to set any flags but they are used for optimization.
        enum BuilderFlags : AZ::u8
        {
            BF_None = 0,
            BF_EmitsNoDependencies = 1<<0, // if you set this flag, dependency-related parts in the code will be skipped
            BF_DeleteLastKnownGoodProductOnFailure = 1<<1,  // if processing fails, delete previous successful product if it exists
        };
        
        //! The name of the Builder
        AZStd::string m_name;

        //! The collection of asset builder patterns that the builder will use to
        //! determine if a file will be processed by that builder
        AZStd::vector<AssetBuilderPattern>  m_patterns;

        //! The builder unique ID
        AZ::Uuid m_busId;

        //! Changing this version number will cause all your assets to be re-submitted to the builder for job creation and rebuilding.
        int m_version = 0;

        //! The required create job function callback that the asset processor will call during the job creation phase
        CreateJobFunction m_createJobFunction;
        //! The required process job function callback that the asset processor will call during the job processing phase
        ProcessJobFunction m_processJobFunction;

        //! The builder type.  We set this to External by default, as that is the typical set up for custom builders (builders in gems and legacy dll builders).
        AssetBuilderType m_builderType = AssetBuilderType::External;

        /** Analysis Fingerprint
         * you can optionally emit an analysis fingerprint, or leave this empty.  
         * The Analysis Fingerprint, used to quickly skip analysis if the source files modtime has not changed.
         * If your analysis fingerprint DOES change, then all source files will be sent to your CreateJobs function regardless of modtime changes.
         * This does not necessarily mean that the jobs will need doing, just that CreateJobs will be called.
         * For best results, make sure your analysis fingerprint only changes when its likely that you need to re-analyze source files for changes, which
         * may result in job fingerprints to be different (for example, if you have changed your logic inside your builder).
        **/
        AZStd::string m_analysisFingerprint;

        //! You don't have to set any flags, but if you do, it can improve speed.
        //! If you change your flags, bump the version number of your builder, too.
        AZ::u8 m_flags = 0;

        AZStd::unordered_map<AZStd::string, AZ::u8> m_flagsByJobKey;

        // If BF_DeleteLastKnownGoodProductOnFailure is raised, ALL specified product keys will be deleted on failure
        // use this set to keep specific products in the job, if necessary
        AZStd::unordered_map<AZStd::string, AZStd::unordered_set<AZ::u32>> m_productsToKeepOnFailure;

        void AddFlags(AZ::u8 flag, const AZStd::string& jobKey);

        bool HasFlag(AZ::u8 flag, const AZStd::string& jobKey) const;

        bool IsExternalBuilder() const;

        // Note that we don't serialize the function pointer fields as part of the registration since they should not be
        // sent over the wire.
        static void Reflect(AZ::ReflectContext* context);
    };

    //! Source file dependency information that the builder will send to the assetprocessor
    //! It is important to note that the builder do not need to provide both the sourceFileDependencyUUID or sourceFileDependencyPath info to the asset processor,
    //! any one of them should be sufficient
    struct SourceFileDependency
    {
        AZ_CLASS_ALLOCATOR(SourceFileDependency, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(SourceFileDependency, "{d3c055d8-b5e8-44ab-a6ce-1ecb0da091ec}");

        // Corresponds to SourceFileDependencyEntry TypeOfDependency Values
        enum class SourceFileDependencyType : AZ::u32
        {
            Absolute, // Corresponds to DEP_SourceToSource
            Wildcards // DEP_SourceLikeMatch
        };
        /** Filepath on which the source file depends, it can be either be a relative path from the assets folder, or an absolute path. 
        * if it's relative, the asset processor will check every watched folder in the order specified in the assetprocessor config file until it finds that file. 
        * For example if the builder sends a SourceFileDependency with m_sourceFileDependencyPath = "texture/blah.tif" to the asset processor,
        * it will check all watch folders for a file whose relative path with regard to it is "texture/blah.tif".
        * and supposing it finds it in "C:/dev/gamename/texture/blah.tif", it will use that as the dependency.
        * You can also send absolute path, which will obey the usual overriding rules.
        *     @note You must EITHER provide the m_sourceFileDependencyPath OR the m_sourceFileDependencyUUID. 
        **/
        AZStd::string m_sourceFileDependencyPath;

        /** UUID of the file on which the source file depends.  
        *     @note You must EITHER provide the m_sourceFileDependencyPath OR the m_sourceFileDependencyUUID if you have that instead.
        */
        AZ::Uuid m_sourceFileDependencyUUID = AZ::Uuid::CreateNull();

        SourceFileDependencyType m_sourceDependencyType{ SourceFileDependencyType::Absolute };

        SourceFileDependency() = default;
        
        SourceFileDependency(const AZStd::string& sourceFileDependencyPath, AZ::Uuid sourceFileDependencyUUID, SourceFileDependencyType sourceDependencyType = SourceFileDependencyType::Absolute)
            : m_sourceFileDependencyPath(sourceFileDependencyPath)
            , m_sourceFileDependencyUUID(sourceFileDependencyUUID)
            , m_sourceDependencyType(sourceDependencyType)
        {
        }

        SourceFileDependency(AZStd::string&& sourceFileDependencyPath, AZ::Uuid sourceFileDependencyUUID, SourceFileDependencyType sourceDependencyType = SourceFileDependencyType::Absolute)
            : m_sourceFileDependencyPath(AZStd::move(sourceFileDependencyPath))
            , m_sourceFileDependencyUUID(sourceFileDependencyUUID)
            , m_sourceDependencyType(sourceDependencyType)
        {
        }

        AZStd::string ToString() const;

        static void Reflect(AZ::ReflectContext* context);
    };
    enum class JobDependencyType : AZ::u32
    {
        //! This implies that the dependent job should get processed by the assetprocessor, if the fingerprint of job it depends on changes.
        Fingerprint,

        //! This implies that the dependent job should only run after the job it depends on is processed by the assetprocessor.
        Order,

        //! This is similiar to Order where the dependent job should only run after all the jobs it depends on are processed by the assetprocessor.
        //! The difference is that here only those dependent jobs matter that have never been processed by the asset processor.
        //! Also important to note is the fingerprint of the dependent jobs will not alter the the fingerprint of the job.  
        OrderOnce,
    };

    //! Job dependency information that the builder will send to the assetprocessor.
    struct JobDependency
    {
        AZ_CLASS_ALLOCATOR(JobDependency, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(JobDependency, "{93A9D915-8C9E-4588-8D86-578C01EEA388}");
        //! Source file dependency information that the builder will send to the assetprocessor
        //! It is important to note that the builder do not need to provide both the sourceFileDependencyUUID or sourceFileDependencyPath info to the asset processor,
        //! any one of them should be sufficient
        SourceFileDependency m_sourceFile;
        
        //! JobKey of the dependent job
        AZStd::string m_jobKey;
        
        //! Platform Identifier of the dependent job
        AZStd::string m_platformIdentifier;

        //! Type of Job Dependency (order or fingerprint)
        JobDependencyType m_type;

        JobDependency() = default;
        JobDependency(const AZStd::string& jobKey, const AZStd::string& platformIdentifier, const JobDependencyType& type, const SourceFileDependency& sourceFile);

        static void Reflect(AZ::ReflectContext* context);
    };

    //! JobDescriptor is used by the builder to store job related information
    struct JobDescriptor
    {
        AZ_CLASS_ALLOCATOR(JobDescriptor, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(JobDescriptor, "{bd0472a4-7634-41f3-97ef-00f3b239bae2}");

        //! Any builder specific parameters to pass to the Process Job Request
        JobParameterMap m_jobParameters;

        //! Any additional info that should be taken into account during fingerprinting for this job
        AZStd::string m_additionalFingerprintInfo;
        
        //! Job specific key, e.g. TIFF Job, etc
        AZStd::string m_jobKey;

#if defined(ENABLE_LEGACY_PLATFORMFLAGS_SUPPORT)
        /**
        * DEPRECATED - this remains only for backward compatiblity with older modules
        * consider using m_platformIdentifier (via getter/setter) instead.  This will still work but as new platforms are added
        * using the data-driven approach, your enum will no longer be sufficient.
        */
        int m_platform = Platform_NONE;
#endif // defined(ENABLE_LEGACY_PLATFORMFLAGS_SUPPORT)

        //! Priority value for the jobs within the job queue.  If less than zero, than the priority of this job is not considered or or is lowest priority.
        //! If zero or greater, the value is prioritized by this number (the higher the number, the higher priority).  Note: priorities are set within critical
        //! and non-critical job separately.
        int m_priority = -1;

        //! Flag to determine if this is a critical job or not.  Critical jobs are given the higher priority in the processing queue than non-critical jobs
        bool m_critical = false;

        //! Flag to determine whether we need to check the input file for exclusive lock before we process the job
        bool m_checkExclusiveLock = false;

        //! Flag to determine whether we need to check the server for the outputs of this job 
        //! before we start processing the job locally.
        //! If the asset processor is running in server mode then this will be used to determine whether we need 
        //! to store the outputs of this jobs in the server.
        bool m_checkServer = false;

        //! This is required for jobs that want to declare job dependency on other jobs. 
        AZStd::vector<JobDependency> m_jobDependencyList;

        //! If set to true, reported errors, asserts and exceptions will automatically cause the job to fail even is ProcessJobResult_Success is the result code.
        bool m_failOnError = false;

        /** 
        * construct using a platformIdentifier from your CreateJobsRequest.  it is the m_identifier member of the PlatformInfo.
        */
        JobDescriptor(const AZStd::string& additionalFingerprintInfo, AZStd::string jobKey, const char* platformIdentifier); 
        
#if defined(ENABLE_LEGACY_PLATFORMFLAGS_SUPPORT)
        /**
        * DEPRECATED - please use the above constructor
        * This is retained for backward compatiblity only
        * Construct a JobDescriptor using the platform index from the Platform enum.
        */
        JobDescriptor(AZStd::string additionalFingerprintInfo, int platform, const AZStd::string& jobKey);
#endif // defined(ENABLE_LEGACY_PLATFORMFLAGS_SUPPORT)
        
        JobDescriptor() = default;

        static void Reflect(AZ::ReflectContext* context);

        /** Use this to set the platform identifier.  it knows when it needs to retroactively compute
        * the old m_platform flag when that code is enabled.
        */
        void SetPlatformIdentifier(const char* platformIdentifier);
        const AZStd::string& GetPlatformIdentifier() const;

    protected:
        /**
        * This describes which platform its for.  It should match one of the enabled platforms passed into CreateJobs.
        * It is the identifier of the platform from that PlatformInfo struct.
        */
        AZStd::string m_platformIdentifier;
    };

    //! RegisterBuilderRequest contains input data that will be sent by the AssetProcessor to the builder during the startup registration phase
    struct RegisterBuilderRequest
    {
        AZ_CLASS_ALLOCATOR(RegisterBuilderRequest, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(RegisterBuilderRequest, "{7C6C5198-4766-42B8-9A1E-48479CE2F5EA}");

        AZStd::string m_filePath;

        RegisterBuilderRequest() {}

        explicit RegisterBuilderRequest(const AZStd::string& filePath)
            : m_filePath(filePath)
        {
        }

        static void Reflect(AZ::ReflectContext* context);
    };

    //! INTERNAL USE ONLY - RegisterBuilderResponse contains registration data that will be sent by the builder to the AssetProcessor in response to RegisterBuilderRequest
    struct RegisterBuilderResponse
    {
        AZ_CLASS_ALLOCATOR(RegisterBuilderResponse, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(RegisterBuilderResponse, "{0AE5583F-C763-410E-BA7F-78BD90546C01}");

        AZStd::vector<AssetBuilderDesc> m_assetBuilderDescList;

        static void Reflect(AZ::ReflectContext* context);
    };

    /**
    *  This tells you about a platform in your CreateJobsRequest or your ProcessJobRequest
    */
    struct PlatformInfo
    {
        AZ_CLASS_ALLOCATOR(PlatformInfo, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(PlatformInfo, "{F7DA39A5-C319-4552-954B-3479E2454D3F}");

        AZStd::string m_identifier; ///< like "pc" or "android" or "ios"...
        AZStd::unordered_set<AZStd::string> m_tags; ///< The tags like "console" or "tools" on that platform

        PlatformInfo() = default;
        PlatformInfo(AZStd::string identifier, const AZStd::unordered_set<AZStd::string>& tags);
        bool operator==(const PlatformInfo& other);

        ///! utility function.  It just searches the set for you:
        bool HasTag(const char* tag) const;

        static void Reflect(AZ::ReflectContext* context);
        static AZStd::string PlatformVectorAsString(const AZStd::vector<PlatformInfo>& platforms);
    };

    //! CreateJobsRequest contains input job data that will be send by the AssetProcessor to the builder for creating jobs
    struct CreateJobsRequest
    {
        AZ_CLASS_ALLOCATOR(CreateJobsRequest, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(CreateJobsRequest, "{02d470fb-4cb6-4cd7-876f-f0652910ff75}");

        //! The builder id to identify which builder will process this job request
        AZ::Uuid m_builderid; // builder id

        //! m_watchFolder contains the subfolder that the sourceFile came from, out of all the folders being watched by the Asset Processor.
        //! If you combine the Watch Folder with the Source File (m_sourceFile), you will result in the full absolute path to the file.
        AZStd::string m_watchFolder;

        //! The source file path that is relative to the watch folder (m_watchFolder)
        AZStd::string m_sourceFile;

        AZ::Uuid m_sourceFileUUID; ///< each source file has a unique UUID.

        
        //! Information about each platform you are expected to build is stored here.
        //! You can emit any number of jobs to produce some or all of the assets for each of these platforms.
        AZStd::vector<PlatformInfo> m_enabledPlatforms;

        CreateJobsRequest();

        CreateJobsRequest(AZ::Uuid builderid, AZStd::string sourceFile, AZStd::string watchFolder, const AZStd::vector<PlatformInfo>& enabledPlatforms, const AZ::Uuid& sourceFileUuid);

        /**
        * New Data-driven platform API - will return true if the m_enabledPlatforms contains
        * a platform with that identifier
        */
        bool HasPlatform(const char* platformIdentifier) const;
        /**
        * New Data-driven platform API - will return true if the m_enabledPlatforms contains
        * a platform which itself contains that tag.  Note that multiple platforms may match this tag.
        */
        bool HasPlatformWithTag(const char* platformTag) const;

#if defined(ENABLE_LEGACY_PLATFORMFLAGS_SUPPORT)
        /** 
        * Legacy - DEPRECATED - use m_enabledPlatforms instead.
        * returns the number of platforms that are enabled for the source file
        */
        size_t GetEnabledPlatformsCount() const;

        /***
        * Legacy - DEPRECATED - use m_enabledPlatforms instead.
        * returns the enabled platform by index, if no platform is found then we will return  Platform_NONE. 
        */
        AssetBuilderSDK::Platform GetEnabledPlatformAt(size_t index) const;

        /***
        * Legacy - DEPRECATED - use m_enabledPlatforms instead.
        * determine whether the platform is enabled or not, returns true if enabled  otherwise false  
        */
        bool IsPlatformEnabled(AZ::u32 platform) const;

        /***
        * Legacy - DEPRECATED - use m_enabledPlatforms instead.
        * determine whether the inputted platform is valid or not, returns true if valid otherwise false
        */
        bool IsPlatformValid(AZ::u32 platform) const;
        /**
        * Legacy - deprecated!  Only here for backward compatibility.  Will not support new platforms - please use the m_enabledPlatform APIs going forward
        * Platform flags informs the builder which platforms the AssetProcessor is interested in.  Its the platforms enum as bitmasks
        */
        int m_platformFlags = 0;
#endif // defined(ENABLE_LEGACY_PLATFORMFLAGS_SUPPORT)

        
        static void Reflect(AZ::ReflectContext* context);

        
    };

    //! Possible result codes from CreateJobs requests
    enum class CreateJobsResultCode
    {
        //! Jobs were created successfully
        Success,
        //! Jobs failed to be created
        Failed,
        //! The builder is in the process of shutting down
        ShuttingDown
    };

    //! CreateJobsResponse contains job data that will be send by the builder to the assetProcessor in response to CreateJobsRequest
    struct CreateJobsResponse
    {
        AZ_CLASS_ALLOCATOR(CreateJobsResponse, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(CreateJobsResponse, "{32a27d68-25bc-4425-a12b-bab961d6afcd}");

        CreateJobsResultCode         m_result = CreateJobsResultCode::Failed;   // The result code from the create jobs request

        AZStd::vector<SourceFileDependency> m_sourceFileDependencyList; // This is required for source files that want to declare dependencies on other source files.
        AZStd::vector<JobDescriptor> m_createJobOutputs;

        bool Succeeded() const;

        static void Reflect(AZ::ReflectContext* context);
    };

    //! Product dependency information that the builder will send to the assetprocessor
    //! Indicates a product asset that depends on another product asset
    struct ProductDependency
    {
        AZ_CLASS_ALLOCATOR(ProductDependency, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(ProductDependency, "{54338921-b437-4f39-a0da-b1d0d1ee7b57}");

        //! ID of the asset dependency
        AZ::Data::AssetId m_dependencyId;


        AZ::Data::ProductDependencyInfo::ProductDependencyFlags m_flags;

        // By default, initialize the dependency flags to "NoLoad" so that dependent assets aren't triggered to load.
        // Only set dependent assets to load if the creation of a product dependency explicitly requests it.  This makes it
        // more likely to prevent accidental loads when creating dependencies based solely on IDs or other implicit asset
        // references.
        ProductDependency()
            : m_flags(AZ::Data::ProductDependencyInfo::CreateFlags(AZ::Data::AssetLoadBehavior::NoLoad))
        {
        }

        ProductDependency(AZ::Data::AssetId dependencyId, const AZStd::bitset<64>& flags);

        static void Reflect(AZ::ReflectContext* context);
    };

    using ProductPathDependencySet = AZStd::unordered_set<AssetBuilderSDK::ProductPathDependency>;

    //! JobProduct is used by the builder to store job product information
    struct JobProduct
    {
        AZ_CLASS_ALLOCATOR(JobProduct, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(JobProduct, "{d1d35d2c-3e4a-45c6-a13a-e20056344516}");

        AZStd::string m_productFileName; // relative or absolute product file path

        AZ::Data::AssetType m_productAssetType = AZ::Data::AssetType::CreateNull(); // the type of asset this is
        AZ::u32 m_productSubID; ///< a stable product identifier - see note below.

        /// LegacySUBIds are other names for the same product for legacy compatibility.
        /// if you ever referred to this product by a different sub-id previously but have decided to change your numbering scheme
        /// You should emit the prior sub ids into this array.  If we ever go looking for an asset and we fail to find it under a
        /// canonical product SubID, the system will attempt to look it up in the list of "previously known as..." legacy subIds in case
        /// the source data it is reading is old.  This allows you to change your subID scheme at any time as long as you include
        /// the old scheme in the legacySubIDs list.
        AZStd::vector<AZ::u32> m_legacySubIDs; 

        // SUB ID context: A Stable sub id means a few things. Products (game ready assets) are identified in the engine by AZ::Data::AssetId, which is a combination of source guid which is random and this product sub id. AssetType is currently NOT USED to differentiate assets by the system. So if two or more products of the same source are for the same platform they can not generate the same sub id!!! If they did this would be a COLLISION!!! which would not allow the rngine to access one or more of the products!!! Not using asset type in the differentiation may change in the future, but it is the way it is done for now.
        // SUB ID RULES:
        // 1. The builder alone is responsible for determining asset type and sub id.
        // 2. The sub id has to be build run stable, meaning if the builder were to run again for the same source the same sub id would be generated by the builder to identify this product.
        // 3. The sub id has to be location stable, meaning they can not be based on the location of the source or product, so if the source was moved to a different location it should still produce the same sub id for the same product.
        // 4. The sub id has to be platform stable, meaning if the builder were to make the equivalent product for a different platform the sub id for the equivalent product on the other platform should be the same.
        // 5. The sub id has to be multi output stable and mutually exclusive, meaning if your builder outputs multiple products from a source, the product sub id for each product must be different from one another and reproducible. So if you use an incrementing number scheme to differentiate products, that must also be stable, even when the source changes. So if a change occurs to the source, it gets rebuilt and the sub ids must still be the same. Put another way, if your builder outputs multiple product files, and produces the number and order and type of product, no matter what change to the source is made, then you're good. However, if changing the source may result in less or more products than last time, you may have a problem. The same products this time must have the same sub id as last time and can not have shifted up or down. Its ok if the extra product has the next new number, or if one less product is produced doesn't effect the others, in short they can never shift ids which would be the case for incrementing ids if one should no longer be produced. Note that the builder has no other information from run to run than the source data, it can not access any other data, source, product, database or otherwise receive data from any previous run. If the builder used an enumerated value for different outputs, that would work, say if he diffuse output always uses the enumerated value sub id 2 and the alpha always used 6, that should be fine, even if the source is modified such that it no longer outputs an alpha, the diffuse would still always map to 2.
        // SUGGESTIONS:
        // 1. If your builder only ever has one product for a source then we recommend that sub id be set to 0, this should satisfy all the above rules.
        // 2. Do not base sub id on file paths, if the location of source or destination changes the sub id will not be stable.
        // 3. Do not base sub id on source or product file name, extensions usually differ per platform and across platform they should be the stable.
        // 4. It might be ok to base sub id on extension-less product file name. It seems likely it would be stable as the product name would most likely be the same no matter its location as the path to the file and files extension could be different per platform and thus using only the extension-less file name would mostly likely be the same across platform. Be careful though, because if you output many same named files just with different extensions FOR THE SAME PLATFORM you will have collision problems.
        // 5. Basing the sub id on a simple incrementing number may be reasonable ONLY if order can never change, or the order if changed it would not matter. This may make sense for mip levels of textures if produced as separate products such that the sub id is equal to mip level, or lods for a mesh such that the sub id is the lod level.
        // 6. Think about using some other encoding scheme like using enumerations or using flag bits. If we do then we might be able to guess the sub id at runtime, that could be useful. Name spacing using the upper bits might be useful for final determination of product. This could be part of a localization scheme, or user settings options like choosing green blood via upper bits, or switching between products built by different builders which have stable lower bits and different name space upper bits. I am not currently convinced that encoding information into the sub id like this is a really great idea, however if it does not violate the rules, it is allowed, and it may solve a problem or two for specific systems.
        // 7. A Tagging system for products (even sources?) that allows the builder to add any tag it want to a product that would be available at tool time (and at runtime?) might be a better way than trying to encode that kind of data in product sub id's.

        //! Product assets this asset depends on
        AZStd::vector<ProductDependency> m_dependencies;

        /// Dependencies specified by relative path in the resource
        /// Paths should only be used in legacy systems, put ProductDependency objects in m_dependencies wherever possible.
        ProductPathDependencySet m_pathDependencies;

        /// Indicate to Asset Processor that the builder has output any possible dependencies (including if there are none).
        /// This should only be set if the builder really does take care of outputting its dependencies OR the output product never has dependencies.
        /// When false, AP will emit a warning that dependencies have not been handled.
        bool m_dependenciesHandled{ false };

        JobProduct() = default;
        JobProduct(const AZStd::string& productName, AZ::Data::AssetType productAssetType = AZ::Data::AssetType::CreateNull(), AZ::u32 productSubID = 0);
        JobProduct(AZStd::string&& productName, AZ::Data::AssetType productAssetType = AZ::Data::AssetType::CreateNull(), AZ::u32 productSubID = 0);
        //////////////////////////////////////////////////////////////////////////
        // Legacy compatibility
        // when builders output asset type, but don't specify what type they actually are, we guess by file extension and other
        // markers.  This is not ideal.  If you're writing a new builder, endeavor to actually select a product asset type and a subId
        // that matches your needs.
        static AZ::Data::AssetType InferAssetTypeByProductFileName(const char* productFile);
        static AZ::u32 InferSubIDFromProductFileName(const AZ::Data::AssetType& assetType, const char* productFile);
        //////////////////////////////////////////////////////////////////////////

        static void Reflect(AZ::ReflectContext* context);
    };

    //! ProcessJobRequest contains input job data that will be send by the AssetProcessor to the builder for processing jobs
    struct ProcessJobRequest
    {
        AZ_CLASS_ALLOCATOR(ProcessJobRequest, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(ProcessJobRequest, "{20461454-d2f9-4079-ab95-703905e06002}");

        AZStd::string m_sourceFile; ///! relative source file name
        AZStd::string m_watchFolder; ///! watch folder for this source file
        AZStd::string m_fullPath; ///! full source file name
        AZ::Uuid m_builderGuid; ///! builder id
        JobDescriptor m_jobDescription; ///! job descriptor for this job.  Note that this still contains the job parameters from when you emitted it during CreateJobs
        PlatformInfo m_platformInfo; ///! the information about the platform that this job was emitted for.
        AZStd::string m_tempDirPath; // temp directory that the builder should use to create job outputs for this job request
        AZ::u64 m_jobId; ///! job id for this job, this is also the address for the JobCancelListener
        AZ::Uuid m_sourceFileUUID; ///! the UUID of the source file.  Will be used as the uuid of the AssetID of the product when combined with the subID.
        AZStd::vector<SourceFileDependency> m_sourceFileDependencyList;

        static void Reflect(AZ::ReflectContext* context);
    };


    enum ProcessJobResultCode
    {
        ProcessJobResult_Success = 0,
        ProcessJobResult_Failed = 1,
        ProcessJobResult_Crashed = 2,
        ProcessJobResult_Cancelled = 3,
        ProcessJobResult_NetworkIssue = 4
    };

    //! ProcessJobResponse contains job data that will be send by the builder to the assetProcessor in response to ProcessJobRequest
    struct ProcessJobResponse
    {
        AZ_CLASS_ALLOCATOR(ProcessJobResponse, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(ProcessJobResponse, "{6b48ada5-0d52-43be-ad57-0bf8aeaef04b}");
        
        ProcessJobResultCode m_resultCode = ProcessJobResult_Failed;
        AZStd::vector<JobProduct> m_outputProducts;
        bool m_requiresSubIdGeneration = true; //!< Used to determine if legacy RC products need sub ids generated for them.
        //! Populate m_sourcesToReprocess with sources by absolute path which you want to trigger a rebuild for
        //! To reprocess these sources, make sure to update fingerprints in CreateJobs of those builders which process them, like changing source dependencies.
        AZStd::vector<AZStd::string> m_sourcesToReprocess; 
        
        bool Succeeded() const;

        static void Reflect(AZ::ReflectContext* context);
    };

    //! BuilderHelloRequest is sent by an AssetBuilder that is attempting to connect to the AssetProcessor to register itself as a worker
    class BuilderHelloRequest : public AzFramework::AssetSystem::BaseAssetProcessorMessage
    {
    public:

        AZ_CLASS_ALLOCATOR(BuilderHelloRequest, AZ::OSAllocator, 0);
        AZ_RTTI(BuilderHelloRequest, "{5fab5962-a1d8-42a5-bf7a-fb1a8c5a9588}", BaseAssetProcessorMessage);

        static void Reflect(AZ::ReflectContext* context);
        static unsigned int MessageType();

        unsigned int GetMessageType() const override;

        //! Unique ID assigned to this builder to identify it
        AZ::Uuid m_uuid = AZ::Uuid::CreateNull();
    };

    //! BuilderHelloResponse contains the AssetProcessor's response to a builder connection attempt, indicating if it is accepted and the ID that it was assigned
    class BuilderHelloResponse : public AzFramework::AssetSystem::BaseAssetProcessorMessage
    {
    public:

        AZ_CLASS_ALLOCATOR(BuilderHelloResponse, AZ::OSAllocator, 0);
        AZ_RTTI(BuilderHelloResponse, "{5f3d7c11-6639-4c6f-980a-32be546903c2}", BaseAssetProcessorMessage);

        static void Reflect(AZ::ReflectContext* context);

        unsigned int GetMessageType() const override;

        //! Indicates if the builder was accepted by the AP
        bool m_accepted = false;

        //! Unique ID assigned to the builder.  If the builder isn't a local process, this is the ID assigned by the AP
        AZ::Uuid m_uuid = AZ::Uuid::CreateNull();
    };

    class CreateJobsNetRequest : public AzFramework::AssetSystem::BaseAssetProcessorMessage
    {
    public:

        AZ_CLASS_ALLOCATOR(CreateJobsNetRequest, AZ::OSAllocator, 0);
        AZ_RTTI(CreateJobsNetRequest, "{97fa717d-3a09-4d21-95c6-b2eafd773f1c}", BaseAssetProcessorMessage);

        static void Reflect(AZ::ReflectContext* context);
        static unsigned int MessageType();

        unsigned int GetMessageType() const override;

        CreateJobsRequest m_request;
    };

    class CreateJobsNetResponse : public AzFramework::AssetSystem::BaseAssetProcessorMessage
    {
    public:

        AZ_CLASS_ALLOCATOR(CreateJobsNetResponse, AZ::OSAllocator, 0);
        AZ_RTTI(CreateJobsNetResponse, "{b2c7c2d3-b60e-4b27-b699-43e0ba991c33}", BaseAssetProcessorMessage);

        static void Reflect(AZ::ReflectContext* context);

        unsigned int GetMessageType() const override;

        CreateJobsResponse m_response;
    };

    class ProcessJobNetRequest : public AzFramework::AssetSystem::BaseAssetProcessorMessage
    {
    public:

        AZ_CLASS_ALLOCATOR(ProcessJobNetRequest, AZ::OSAllocator, 0);
        AZ_RTTI(ProcessJobNetRequest, "{05288de1-020b-48db-b9de-715f17284efa}", BaseAssetProcessorMessage);

        static void Reflect(AZ::ReflectContext* context);
        static unsigned int MessageType();

        unsigned int GetMessageType() const override;

        ProcessJobRequest m_request;
    };

    class ProcessJobNetResponse : public AzFramework::AssetSystem::BaseAssetProcessorMessage
    {
    public:

        AZ_CLASS_ALLOCATOR(ProcessJobNetResponse, AZ::OSAllocator, 0);
        AZ_RTTI(ProcessJobNetResponse, "{26ddf882-246c-4cfb-912f-9b8e389df4f6}", BaseAssetProcessorMessage);

        static void Reflect(AZ::ReflectContext* context);

        unsigned int GetMessageType() const override;

        ProcessJobResponse m_response;
    };

    //! JobCancelListener can be used by builders in their processJob method to listen for job cancellation request.
    //! The address of this listener is the jobid which can be found in the process job request.
    class JobCancelListener : public JobCommandBus::Handler
    {
    public:
        explicit JobCancelListener(AZ::u64 jobId);
        ~JobCancelListener() override;
        JobCancelListener(const JobCancelListener&) = delete;
        //////////////////////////////////////////////////////////////////////////
        //!JobCommandBus::Handler overrides
        //!Note: This will be called on a thread other than your processing job thread. 
        //!You can derive from JobCancelListener and reimplement Cancel if you need to do something special in order to cancel your job.
        void Cancel() override;
        ///////////////////////////////////////////////////////////////////////

        bool IsCancelled() const;

    private:
        AZStd::atomic_bool m_cancelled;
    };

    // the Assert Absorber here is used to absorb asserts during regex creation.
    // it only absorbs asserts spawned by this thread;
    class AssertAbsorber
        : public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        AssertAbsorber();
        ~AssertAbsorber();

        bool OnAssert(const char* message) override;
                
        AZStd::string m_assertMessage;
        // only absorb messages for your thread!
        static AZ_THREAD_LOCAL bool s_onAbsorbThread;
    };

    //! Trace hook for asserts/errors.
    //! This allows us to detect any errors that occur during a job so we can fail it.
    class AssertAndErrorAbsorber
        : public AZ::Debug::TraceMessageBus::Handler
    {
    public:

        explicit AssertAndErrorAbsorber(bool errorsWillFailJob);
        ~AssertAndErrorAbsorber() override;

        bool OnAssert(const char* message) override;
        bool OnError(const char* window, const char* message) override;

        size_t GetErrorCount() const;

    private:

        bool m_errorsWillFailJob;
        size_t m_errorsOccurred = 0;

        //! The id of the thread that created this object.  
        //! There can be multiple builders running at once, so we need to filter out ones coming from other builders
        AZStd::thread_id m_jobThreadId;
    };
} // namespace AssetBuilderSDK

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(AssetBuilderSDK::AssetBuilderPattern::PatternType, "{8519E97D-1159-4CA4-A6DD-16043349B15A}");
    AZ_TYPE_INFO_SPECIALIZE(AssetBuilderSDK::CreateJobsResultCode, "{D3F90549-CE6C-4155-BE19-33E4C05373DB}");
    AZ_TYPE_INFO_SPECIALIZE(AssetBuilderSDK::JobDependencyType, "{854ADE4E-0C2F-43BC-B5F6-8D99C26A17DF}");
    AZ_TYPE_INFO_SPECIALIZE(AssetBuilderSDK::ProcessJobResultCode, "{15797D63-4980-436A-9DE1-E0CCA9B5DB19}");
    AZ_TYPE_INFO_SPECIALIZE(AssetBuilderSDK::ProductPathDependencyType, "{EF77742B-9627-4072-B431-396AA7183C80}");
    AZ_TYPE_INFO_SPECIALIZE(AssetBuilderSDK::SourceFileDependency::SourceFileDependencyType, "{BE9C8805-DB17-4500-944A-EB33FD0BE347}");
}

//! This macro should be used by every AssetBuilder to register itself,
//! AssetProcessor uses these exported function to identify whether a dll is an Asset Builder or not
//! If you want something highly custom you can do these entry points yourself instead of using the macro.
#define REGISTER_ASSETBUILDER                                                      \
    extern void BuilderOnInit();                                                   \
    extern void BuilderDestroy();                                                  \
    extern void BuilderRegisterDescriptors();                                      \
    extern void BuilderAddComponents(AZ::Entity * entity);                         \
    extern "C"                                                                     \
    {                                                                              \
    AZ_DLL_EXPORT int IsAssetBuilder()                                             \
    {                                                                              \
        return 0;                                                                  \
    }                                                                              \
                                                                                   \
    AZ_DLL_EXPORT void InitializeModule(AZ::EnvironmentInstance sharedEnvironment) \
    {                                                                              \
        AZ::Environment::Attach(sharedEnvironment);                                \
        BuilderOnInit();                                                           \
    }                                                                              \
                                                                                   \
    AZ_DLL_EXPORT void UninitializeModule()                                        \
    {                                                                              \
        BuilderDestroy();                                                          \
        AZ::Environment::Detach();                                                 \
    }                                                                              \
                                                                                   \
    AZ_DLL_EXPORT void ModuleRegisterDescriptors()                                 \
    {                                                                              \
        BuilderRegisterDescriptors();                                              \
    }                                                                              \
                                                                                   \
    AZ_DLL_EXPORT void ModuleAddComponents(AZ::Entity * entity)                    \
    {                                                                              \
        BuilderAddComponents(entity);                                              \
    }                                                                              \
    }
// confusion-reducing note: above end-brace is part of the macro, not a namespace


