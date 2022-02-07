/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/osstring.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzFramework/Asset/AssetRegistry.h>
#include <AzFramework/Asset/AssetSystemTypes.h>

namespace AZ
{
    class SerializeContext;
}

namespace AzFramework
{
    namespace AssetSystem
    {
        // Pack any serializable type into any vector<>-like container
        template <class Message, class Buffer>
        bool PackMessage(const Message& message, Buffer& buffer)
        {
            AZ::IO::ByteContainerStream<Buffer> byteStream(&buffer);
            return AZ::Utils::SaveObjectToStream(byteStream, AZ::DataStream::ST_BINARY, &message, message.RTTI_GetType());
        }

        // Unpack any serializable type from a vector<>-like buffer
        template <class Message, class Buffer>
        bool UnpackMessage(const Buffer& buffer, Message& message)
        {
            AZ::IO::ByteContainerStream<const Buffer> byteStream(&buffer);
            
            // load object from stream but note here that we do not allow any errors to occur since this is a message that is supposed
            // to be sent between matching server/client versions.
            
            return AZ::Utils::LoadObjectFromStreamInPlace(byteStream, nullptr, message.RTTI_GetType(), &message, AZ::ObjectStream::FilterDescriptor(&AZ::Data::AssetFilterNoAssetLoading, AZ::ObjectStream::FILTERFLAG_STRICT));
        }

        class BaseAssetProcessorMessage
        {
        public:
            AZ_RTTI(BaseAssetProcessorMessage, "{366A7093-C57B-4514-A1BD-A6437AEF2098}");
            explicit BaseAssetProcessorMessage(bool requireFencing = false);
            virtual ~BaseAssetProcessorMessage() = default;
            //! The id of the message type, an unsigned int which can be used to identify which message it is.
            virtual unsigned int GetMessageType() const = 0;
            static void Reflect(AZ::ReflectContext* context);
            //! Some asset messages might require that the requests be evaluated by the asset processor only after the OS has send it a file notification regarding that asset,
            //! Otherwise there could be a race condition and the asset request could be processed before the asset processor gets the file notification.To prevent this we create a fence file 
            //! and only evaluate the request after the asset processor picks up that fence file.We call this fencing.
            bool RequireFencing() const;
        private:
            bool m_requireFencing = false;
        };

        //////////////////////////////////////////////////////////////////////////
        //negotiation
        class NegotiationMessage
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(NegotiationMessage, AZ::OSAllocator, 0);
            AZ_RTTI(NegotiationMessage, "{BA6336E4-4DF5-49EF-A184-FE8F5BC73731}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::Negotiation", 0x141ebc28);

            NegotiationMessage() = default;
            unsigned int GetMessageType() const override;
            int m_apiVersion = 7; // Changing the value will cause negotiation to fail between incompatible versions
            AZ::OSString m_identifier;
            typedef AZStd::unordered_map<unsigned int, AZ::OSString> NegotiationInfoMap;
            NegotiationInfoMap m_negotiationInfoMap;
        };

        //////////////////////////////////////////////////////////////////////////
        // ping request/response
        class RequestPing
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(RequestPing, AZ::OSAllocator, 0);
            AZ_RTTI(RequestPing, "{E06F6663-A168-439A-83E1-6F2215BAC0B6}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::RequestPing", 0xa6124cfb);

            RequestPing() = default;
            unsigned int GetMessageType() const override;
        };

        class ResponsePing
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(ResponsePing, AZ::OSAllocator, 0);
            AZ_RTTI(ResponsePing, "{54E0B5ED-F0DB-4DA1-81C7-A4F96AA7F6BA}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            ResponsePing() = default;
            unsigned int GetMessageType() const override;
        };
       
        //////////////////////////////////////////////////////////////////////////
        //!  Request the status of an asset or force one to compile
        class RequestAssetStatus
            : public BaseAssetProcessorMessage
        {
        public:
            enum SearchType : int
            {
                Default = 0, //< SearchTerm searches will perform Heuristic Searches
                Exact, //< SearchTerm searches will not perform the various herusitic fallbacks such as stripping extensions
            };

            AZ_CLASS_ALLOCATOR(RequestAssetStatus, AZ::OSAllocator, 0);
            AZ_RTTI(RequestAssetStatus, "{0CBE6A7C-9D19-4D41-B29C-A52476BB337A}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::RequestAssetStatus", 0x63146187);
            explicit RequestAssetStatus(bool requireFencing = true);
            RequestAssetStatus(const char* sourceData, bool isStatusRequest, bool requireFencing = true, int searchType = SearchType::Default);
            RequestAssetStatus(const AZ::Data::AssetId& assetId, bool isStatusRequest, bool requireFencing = true);
            unsigned int GetMessageType() const override;

            AZ::OSString m_searchTerm; // some generic search term - can be parts of a name, or full name, of source file or product
            AZ::Data::AssetId m_assetId; 
            bool m_isStatusRequest = false; // if this is true, it will only query status.  if false it will actually compile it.
            int m_searchType{ SearchType::Default };
        };
        
        //! this will be sent in response to the RequestAssetStatus request
        class ResponseAssetStatus
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(ResponseAssetStatus, AZ::OSAllocator, 0);
            AZ_RTTI(ResponseAssetStatus, "{151CB7D2-8A11-4072-A173-5EDF2A11C9E2}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            unsigned int GetMessageType() const override;

            ResponseAssetStatus() = default;
            int m_assetStatus = AssetStatus_Unknown;
        };

        /**
        * Used to ask the AP to move any jobs related to an asset to the front of the queue.
        * There is no response to this request, nor does it fence, so it is "fire-and-forget"
        * and thus doesn't need to introduce a lot of latency
        */
        class RequestEscalateAsset
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(RequestEscalateAsset, AZ::OSAllocator, 0);
            AZ_RTTI(RequestEscalateAsset, "{E95C5422-5F00-478B-A984-C041DE70484F}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::RequestEscalateAsset", 0x1894d94e);

            RequestEscalateAsset() = default;
            ~RequestEscalateAsset() override = default;
            explicit RequestEscalateAsset(const char* searchTerm);
            explicit RequestEscalateAsset(const AZ::Uuid& assetUuid);
            unsigned int GetMessageType() const override;

            // either of the following must be filled in.  If uuid is filled in, search term is ignored.
            AZ::Uuid m_assetUuid; // the uuid of the asset (which also uniquely identifies a source file).
            AZ::OSString m_searchTerm; // the name of a source file, or a heuristic
        };


        //////////////////////////////////////////////////////////////////////////
        //! Request the status of the asset processor
        class RequestAssetProcessorStatus
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(RequestAssetProcessorStatus, AZ::OSAllocator, 0);
            AZ_RTTI(RequestAssetProcessorStatus, "{DEC6CF93-0A16-4D83-AA6D-97FB86340525}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::RequestAssetProcessorStatus", 0x5172b959);

            RequestAssetProcessorStatus() = default;
            unsigned int GetMessageType() const override;
            AZ::OSString m_platform; // platform for which we want to know whether all copy jobs are completed or not
        };

        //! This will be send in response to the RequestAssetProcessorStatus request,
        //! Will contain information whether the AP is ready and how many jobs are remaining
        class ResponseAssetProcessorStatus
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(ResponseAssetProcessorStatus, AZ::OSAllocator, 0);
            AZ_RTTI(ResponseAssetProcessorStatus, "{978350FB-3BD2-4F5C-ABEF-5AB78981C23A}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            ResponseAssetProcessorStatus() = default;
            unsigned int GetMessageType() const override;
            int m_numberOfPendingJobs; // number of copy jobs that are still pending
            bool m_isAssetProcessorReady = false;
        };
        
        //////////////////////////////////////////////////////////////////////////
        struct GetUnresolvedDependencyCountsRequest
            : BaseAssetProcessorMessage
        {
            AZ_CLASS_ALLOCATOR(GetUnresolvedDependencyCountsRequest, AZ::OSAllocator, 0);
            AZ_RTTI(GetUnresolvedDependencyCountsRequest, "{DE432E6F-72D8-48A7-857C-51D1D41EB880}", BaseAssetProcessorMessage);

            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::GetUnresolvedDependencyCountsRequest", 0xb430e444);

            GetUnresolvedDependencyCountsRequest() = default;
            explicit GetUnresolvedDependencyCountsRequest(AZ::Data::AssetId assetId)
                : m_assetId(AZStd::move(assetId))
            {
            }

            unsigned int GetMessageType() const override;

            AZ::Data::AssetId m_assetId;
        };

        struct GetUnresolvedDependencyCountsResponse
            : BaseAssetProcessorMessage
        {
            AZ_CLASS_ALLOCATOR(GetUnresolvedDependencyCountsResponse, AZ::OSAllocator, 0);
            AZ_RTTI(GetUnresolvedDependencyCountsResponse, "{6FC67F5D-4941-41B9-92C5-7778CB853F94}", BaseAssetProcessorMessage);

            static void Reflect(AZ::ReflectContext* context);

            GetUnresolvedDependencyCountsResponse() = default;
            GetUnresolvedDependencyCountsResponse(AZ::u32 unresolvedAssetIdReferences, AZ::u32 unresolvedPathReferences)
                : m_unresolvedAssetIdReferences(unresolvedAssetIdReferences), m_unresolvedPathReferences(unresolvedPathReferences)
            {
            }

            unsigned int GetMessageType() const override;

            AZ::u32 m_unresolvedAssetIdReferences = 0;
            AZ::u32 m_unresolvedPathReferences = 0;
        };

        //////////////////////////////////////////////////////////////////////////
        class GetRelativeProductPathFromFullSourceOrProductPathRequest
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(GetRelativeProductPathFromFullSourceOrProductPathRequest, AZ::OSAllocator, 0);
            AZ_RTTI(GetRelativeProductPathFromFullSourceOrProductPathRequest, "{2ED7888B-959C-451C-90B6-8EF7B0B4E385}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::GetRelativeProductPathFromFullSourceOrProductPath", 0x53293a7a);

            GetRelativeProductPathFromFullSourceOrProductPathRequest() = default;
            GetRelativeProductPathFromFullSourceOrProductPathRequest(const AZ::OSString& sourceOrProductPath);
            unsigned int GetMessageType() const override;

            AZ::OSString m_sourceOrProductPath;
        };

        class GetRelativeProductPathFromFullSourceOrProductPathResponse
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(GetRelativeProductPathFromFullSourceOrProductPathResponse, AZ::OSAllocator, 0);
            AZ_RTTI(GetRelativeProductPathFromFullSourceOrProductPathResponse, "{4BAD0A94-EE97-42A7-ACEC-0698012114A8}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            GetRelativeProductPathFromFullSourceOrProductPathResponse() = default;
            GetRelativeProductPathFromFullSourceOrProductPathResponse(bool resolved, const AZ::OSString& relativeProductPath);
            unsigned int GetMessageType() const override;

            AZ::OSString m_relativeProductPath;
            bool m_resolved;
        };

        //////////////////////////////////////////////////////////////////////////
        class GenerateRelativeSourcePathRequest : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(GenerateRelativeSourcePathRequest, AZ::OSAllocator, 0);
            AZ_RTTI(GenerateRelativeSourcePathRequest, "{B3865033-F5A3-4749-8147-7B1AB04D5F6D}",
                BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            // For people that are debugging the network messages and just see MessageType as a value,
            // the CRC value below is 739777771 (0x2C181CEB)
            static constexpr unsigned int MessageType =
                AZ_CRC_CE("AssetSystem::GenerateRelativeSourcePathRequest");

            GenerateRelativeSourcePathRequest() = default;
            GenerateRelativeSourcePathRequest(const AZ::OSString& sourcePath);
            unsigned int GetMessageType() const override;

            AZ::OSString m_sourcePath;
        };

        class GenerateRelativeSourcePathResponse : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(GenerateRelativeSourcePathResponse, AZ::OSAllocator, 0);
            AZ_RTTI(GenerateRelativeSourcePathResponse, "{938D33DB-C8F6-4FA4-BC81-2F139A9BE1D7}",
                BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            GenerateRelativeSourcePathResponse() = default;
            GenerateRelativeSourcePathResponse(
                bool resolved, const AZ::OSString& relativeSourcePath, const AZ::OSString& rootFolder);
            unsigned int GetMessageType() const override;

            AZ::OSString m_relativeSourcePath;
            AZ::OSString m_rootFolder; ///< This is the folder it was found in (the watched/scanned folder, such as gems /assets/ folder)
            bool m_resolved;
        };

        //////////////////////////////////////////////////////////////////////////
        class GetFullSourcePathFromRelativeProductPathRequest
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(GetFullSourcePathFromRelativeProductPathRequest, AZ::OSAllocator, 0);
            AZ_RTTI(GetFullSourcePathFromRelativeProductPathRequest, "{F48E2159-4711-4D0E-838F-91B472AE10FF}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::GetFullSourcePathFromRelativeProductPath", 0x08057afe);
            
            GetFullSourcePathFromRelativeProductPathRequest() = default;
            GetFullSourcePathFromRelativeProductPathRequest(const AZ::OSString& relativeProductPath);
            unsigned int GetMessageType() const override;

            // for now this is a partial path, in the future it will be a UUID
            AZ::OSString m_relativeProductPath;
        };

        class GetFullSourcePathFromRelativeProductPathResponse
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(GetFullSourcePathFromRelativeProductPathResponse, AZ::OSAllocator, 0);
            AZ_RTTI(GetFullSourcePathFromRelativeProductPathResponse, "{AA80F608-A8A7-49D2-A125-BCB9378526F0}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            GetFullSourcePathFromRelativeProductPathResponse() = default;
            GetFullSourcePathFromRelativeProductPathResponse(bool resolved, const AZ::OSString& fullProductPath);
            unsigned int GetMessageType() const override;

            AZ::OSString m_fullSourcePath;
            bool m_resolved;
        };

        //////////////////////////////////////////////////////////////////////////

        class SourceAssetInfoRequest
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(SourceAssetInfoRequest, AZ::OSAllocator, 0);
            AZ_RTTI(SourceAssetInfoRequest, "{e92cd74f-11e0-4ad8-a786-61d3b9715e35}", AzFramework::AssetSystem::BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetProcessor::SourceAssetInfoRequest", 0x350a86f3);

            SourceAssetInfoRequest() = default;

            /**
            * Gets information about an asset, given the assetId.
            * @param assetType This parameter is optional but could help detect problems with incorrect asset types being assigned to products.
            */
            explicit SourceAssetInfoRequest(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType = AZ::Data::s_invalidAssetType);
            
            //! You can also make a request with the relative or absolute path to the asset instead.  This always returns the source path.
            explicit SourceAssetInfoRequest(const char* assetPath);

            unsigned int GetMessageType() const override;

            AZ::OSString m_assetPath; ///< At least one of AssetPath or AssetId must be non-empty.
            AZ::Data::AssetId m_assetId;
            AZ::Data::AssetType m_assetType = AZ::Data::s_invalidAssetType;
        };

        class SourceAssetInfoResponse
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(SourceAssetInfoResponse, AZ::OSAllocator, 0);
            AZ_RTTI(SourceAssetInfoResponse, "{2e748a05-9acc-4459-9e98-76b71e8a7bb7}", AzFramework::AssetSystem::BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            SourceAssetInfoResponse() = default;
            SourceAssetInfoResponse(const AZ::Data::AssetInfo& assetInfo, const char* rootFolder);

            unsigned int GetMessageType() const override;

            bool m_found = false;
            AZ::Data::AssetInfo m_assetInfo; ///< This contains defaults such as relative path from watched folder, size, Uuid.
            AZ::OSString m_rootFolder; ///< This is the folder it was found in (the watched/scanned folder, such as gems /assets/ folder)
        };

        //////////////////////////////////////////////////////////////////////////

        class AssetInfoRequest
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetInfoRequest, AZ::OSAllocator, 0);
            AZ_RTTI(AssetInfoRequest, "{AB1468DB-99B5-4666-A619-4D3F746805A5}", AzFramework::AssetSystem::BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetProcessor::AssetInfoRequest", 0xfe3e020a);

            AssetInfoRequest() = default;

            /**
            * Gets information about an asset, given the assetId.
            * Note that this will return the source asset instead of the product for any types registered as "source asset types" during asset compilation.
            * The rest of the time, the asset's path, id, and returned type will behave identical to an Asset Catalog lookup.).
            */
            explicit AssetInfoRequest(const AZ::Data::AssetId& assetId);

            //! You can also make a request with the relative or absolute path to the asset instead.
            explicit AssetInfoRequest(const char* assetPath);

            unsigned int GetMessageType() const override;

            AZ::OSString m_assetPath; ///< At least one of AssetPath or AssetId must be non-empty.
            AZ::OSString m_platformName; ///< Name of the platform the asset info should be for
            AZ::Data::AssetId m_assetId;
            AZ::Data::AssetType m_assetType = AZ::Data::s_invalidAssetType;
        };

        class AssetInfoResponse
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetInfoResponse, AZ::OSAllocator, 0);
            AZ_RTTI(AssetInfoResponse, "{B217A11F-430A-40EA-AF4A-4644F5879695}", AzFramework::AssetSystem::BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            AssetInfoResponse() = default;
            AssetInfoResponse(const AZ::Data::AssetInfo& assetInfo);

            unsigned int GetMessageType() const override;

            bool m_found = false;
            AZ::Data::AssetInfo m_assetInfo; ///< This contains defaults such as relative path from watched folder, size, Uuid.
            AZ::OSString m_rootFolder; ///< This is the folder it was found in (the watched/scanned folder, such as gems /assets/ folder)
        };


        //////////////////////////////////////////////////////////////////////////

        class AssetDependencyInfoRequest
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetDependencyInfoRequest, AZ::OSAllocator, 0);
            AZ_RTTI(AssetDependencyInfoRequest, "{2365EEA3-D8A6-41E4-9C9F-2113D5617230}",
                    AzFramework::AssetSystem::BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetProcessor::AssetDependencyInfoRequest");

            AssetDependencyInfoRequest() = default;

            /**
            * Specifies the type of asset dependency lookup to perform.
            */
            enum class DependencyType : AZ::u8
            {
                DirectDependencies,
                AllDependencies,
                LoadBehaviorDependencies,
            };

            /**
            * Gets dependency information about an asset, given the assetId.
            */
            AssetDependencyInfoRequest(const AZ::Data::AssetId& assetId, DependencyType dependencyType);

            unsigned int GetMessageType() const override;

            AZ::Data::AssetId m_assetId{};
            DependencyType m_dependencyType{ DependencyType::AllDependencies };
        };

        class AssetDependencyInfoResponse
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetDependencyInfoResponse, AZ::OSAllocator, 0);
            AZ_RTTI(AssetDependencyInfoResponse, "{B6F9FD89-3F62-43FF-A457-2767CE904C81}",
                    AzFramework::AssetSystem::BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            AssetDependencyInfoResponse() = default;

            unsigned int GetMessageType() const override;

            //! Returns whether or not dependency info for the requested asset ID was found
            bool m_found{ false };

            //! Provides the error string on failures
            AZStd::string m_errorString{};

            //! The set of requested product dependencies
            AZStd::vector<AZ::Data::ProductDependency> m_dependencies;

            //! For LoadBehaviorDependencies requests, the list of dependencies that have the 'NoLoad' Load Behavior
            AZStd::unordered_set<AZ::Data::AssetId> m_noloadSet;

            //! For LoadBehaviorDependencies requests, the list of dependencies that have the 'PreLoad' Load Behavior
            AZ::Data::PreloadAssetListType m_preloadAssetList;
        };

        //////////////////////////////////////////////////////////////////////////

        class RegisterSourceAssetRequest
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(RegisterSourceAssetRequest, AZ::OSAllocator, 0);
            AZ_RTTI(RegisterSourceAssetRequest, "{189c6045-e1d4-4d78-b0e7-2bb7bd05fde1}", AzFramework::AssetSystem::BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetProcessor::RegisterSourceAssetRequest", 0x5f414e59);

            RegisterSourceAssetRequest() = default;
            RegisterSourceAssetRequest(const AZ::Data::AssetType& assetType, const char* assetFileFilter);

            unsigned int GetMessageType() const override;

            AZ::Data::AssetType m_assetType;
            AZ::OSString m_assetFileFilter;
        };

        //////////////////////////////////////////////////////////////////////////

        class UnregisterSourceAssetRequest
            : public AzFramework::AssetSystem::BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(UnregisterSourceAssetRequest, AZ::OSAllocator, 0);
            AZ_RTTI(UnregisterSourceAssetRequest, "{ce3cf055-cf91-4851-9e2c-cb24b2b172d3}", AzFramework::AssetSystem::BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetProcessor::UnregisterSourceAssetRequest", 0xfbe53ee1);

            UnregisterSourceAssetRequest() = default;
            UnregisterSourceAssetRequest(const AZ::Data::AssetType& assetType);

            unsigned int GetMessageType() const override;

            AZ::Data::AssetType m_assetType;
        };

        //////////////////////////////////////////////////////////////////////////
        //ShowAssetProcessorRequest
        class ShowAssetProcessorRequest
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(ShowAssetProcessorRequest, AZ::OSAllocator, 0);
            AZ_RTTI(ShowAssetProcessorRequest, "{509CA545-1213-4064-9B58-6FFE3DDD27D3}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::ShowWindow", 0x33a6cd1f);

            ShowAssetProcessorRequest() = default;
            unsigned int GetMessageType() const override;
        };

        //////////////////////////////////////////////////////////////////////////
        //ShowAssetInAssetProcessorRequest
        class ShowAssetInAssetProcessorRequest
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(ShowAssetInAssetProcessorRequest, AZ::OSAllocator, 0);
            AZ_RTTI(ShowAssetInAssetProcessorRequest, "{04A068A0-58D7-4404-ABAD-AED72287FFE8}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::ShowAssetInAssetProcessor", 0x3c9d1be0);

            ShowAssetInAssetProcessorRequest() = default;
            unsigned int GetMessageType() const override;

            AZ::OSString m_assetPath;
        };

        //////////////////////////////////////////////////////////////////////////
        // AssetNotificationMessage
        class AssetNotificationMessage
            : public BaseAssetProcessorMessage
        {
        public:
            enum NotificationType : unsigned int
            {
                AssetChanged, //< Indicates an asset (product) has been changed.
                AssetRemoved, //< Indicates an asset (product) is no longer available.
                JobFileClaimed, //< Indicates the Asset Processor is claiming full ownership of a file as it's about
                                //< to update it, which can mean creating, overwriting or deleting it.
                JobFileReleased, //< Indicates the Asset Processor releases the claim it previous requested with JobFileClaim
                JobStarted, //< Indicates processing of a source file has started.
                JobCompleted, //< Indicates processing of a source file has completed.
                JobFailed, //< Indicates processing of a source file has failed.
                JobCount, //< Returns the number of jobs that are pending. The count will be returned as a string.
            };

            AZ_CLASS_ALLOCATOR(AssetNotificationMessage, AZ::OSAllocator, 0);
            AZ_RTTI(AssetNotificationMessage, "{09EDFFA4-6851-4AB2-B018-51F0F671D9D5}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetProcessorManager::AssetNotification", 0xd6191df5);

            AssetNotificationMessage() = default;
            AssetNotificationMessage(const AZ::OSString& data, NotificationType type, const AZ::Data::AssetType& assetType, const AZ::OSString& platform);
            unsigned int GetMessageType() const override;

            AZ::OSString m_data;
            AZ::OSString m_platform;
            NotificationType m_type;
            AZ::u64 m_sizeBytes = 0;
            AZ::Data::AssetId m_assetId = AZ::Data::AssetId();
            AZStd::vector<AZ::Data::AssetId> m_legacyAssetIds; // if this asset was referred to by other legacy assetIds in the past, then they will be included here.
            AZ::Data::AssetType m_assetType = AZ::Data::s_invalidAssetType;
            AZStd::vector<AZ::Data::ProductDependency> m_dependencies;
        };

        // SaveAssetCatalogRequest
        class SaveAssetCatalogRequest
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(SaveAssetCatalogRequest, AZ::OSAllocator, 0);
            AZ_RTTI(SaveAssetCatalogRequest, "{12B0C076-97A8-4FAE-9F56-22A890766272}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetProcessorManager::SaveCatalogRequest", 0x79e0f02f);
            SaveAssetCatalogRequest() = default;
            unsigned int GetMessageType() const override;
        };

        //////////////////////////////////////////////////////////////////////////
        // SaveAssetCatalogResponse
        class SaveAssetCatalogResponse
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(SaveAssetCatalogResponse, AZ::OSAllocator, 0);
            AZ_RTTI(SaveAssetCatalogResponse, "{F1B4F440-1251-4516-9FAE-2BB067D58191}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            SaveAssetCatalogResponse() = default;
            unsigned int GetMessageType() const override;

            bool m_saved = false;
        };

        //////////////////////////////////////////////////////////////////////////
        //file op messages
        class FileOpenRequest
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileOpenRequest, AZ::OSAllocator, 0);
            AZ_RTTI(FileOpenRequest, "{C230ADF3-970D-4A4D-A128-112C9E2DC164}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::FileOpen", 0x344f46ca);

            FileOpenRequest() = default;
            FileOpenRequest(const char* filePath, AZ::u32 mode);
            unsigned int GetMessageType() const override;

            AZ::OSString m_filePath;
            AZ::u32 m_mode;
        };

        class FileOpenResponse
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileOpenResponse, AZ::OSAllocator, 0);
            AZ_RTTI(FileOpenResponse, "{50EC3F69-C6F6-4835-964B-155112B37EDC}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            FileOpenResponse() = default;
            FileOpenResponse(AZ::u32 fileHandle, AZ::u32 returnCode);
            unsigned int GetMessageType() const override;

            AZ::u32 m_fileHandle;
            AZ::u32 m_returnCode;
        };

        class FileCloseRequest
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileCloseRequest, AZ::OSAllocator, 0);
            AZ_RTTI(FileCloseRequest, "{D294976E-7664-436F-ACA4-7BCABAA2F5EC}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::FileClose", 0xb99bf25e);

            FileCloseRequest() = default;
            FileCloseRequest(AZ::u32 fileHandle);
            unsigned int GetMessageType() const override;

            AZ::u32 m_fileHandle;
        };

        class FileReadRequest
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileReadRequest, AZ::OSAllocator, 0);
            AZ_RTTI(FileReadRequest, "{9FC866C7-A9C0-41CB-BC41-A333240D3C7E}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::FileRead", 0x08688409);

            FileReadRequest() = default;
            FileReadRequest(AZ::u32 fileHandle, AZ::u64 bytesToRead, bool failOnFewerRead = false);
            unsigned int GetMessageType() const override;

            AZ::u32 m_fileHandle;
            AZ::u64 m_bytesToRead;
            bool m_failOnFewerRead;
        };

        class FileReadResponse
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileReadResponse, AZ::OSAllocator, 0);
            AZ_RTTI(FileReadResponse, "{FFD02544-A10A-42B2-9899-54D7F6C426ED}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            FileReadResponse() = default;
            FileReadResponse(AZ::u32 resultCode, void* data, AZ::u64 dataLength);
            unsigned int GetMessageType() const override;

            AZ::u32 m_resultCode;
            AZStd::vector<AZ::u8, AZ::OSStdAllocator> m_data;
        };

        class FileWriteRequest
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileWriteRequest, AZ::OSAllocator, 0);
            AZ_RTTI(FileWriteRequest, "{4CB9EBC7-ACB9-45DC-9D58-2E8BDF975E12}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::FileWrite", 0xd7f109c5);

            FileWriteRequest() = default;
            FileWriteRequest(AZ::u32 fileHandle, const void* data, AZ::u64 dataLength);
            unsigned int GetMessageType() const override;

            AZ::u32 m_fileHandle;
            AZStd::vector<AZ::u8, AZ::OSStdAllocator> m_data;
        };

        class FileWriteResponse
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileWriteResponse, AZ::OSAllocator, 0);
            AZ_RTTI(FileWriteResponse, "{6EBE6BF5-3B7E-4C49-9B17-14025F5B80CA}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            FileWriteResponse() = default;
            FileWriteResponse(AZ::u32 resultCode, AZ::u64 bytesWritten);
            unsigned int GetMessageType() const override;

            AZ::u32 m_resultCode;
            AZ::u64 m_bytesWritten;
        };

        class FileTellRequest
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileTellRequest, AZ::OSAllocator, 0);
            AZ_RTTI(FileTellRequest, "{EF11067E-5C35-4A3F-8BB2-FEC57C037E3F}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::FileTell", 0x96762daa);

            FileTellRequest() = default;
            FileTellRequest(AZ::u32 fileHandle);
            unsigned int GetMessageType() const override;

            AZ::u32 m_fileHandle;
        };

        class FileTellResponse
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileTellResponse, AZ::OSAllocator, 0);
            AZ_RTTI(FileTellResponse, "{18870776-D1FF-40DA-B78D-3A3BB40F20F8}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            FileTellResponse() = default;
            FileTellResponse(AZ::u32 resultCode, AZ::u64 offset);
            unsigned int GetMessageType() const override;

            AZ::u32 m_resultCode;
            AZ::u64 m_offset;
        };

        class FileSeekRequest
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileSeekRequest, AZ::OSAllocator, 0);
            AZ_RTTI(FileSeekRequest, "{B6E9C144-8033-416D-8E90-0260BE32E164}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::FileSeek", 0x44073bf9);

            FileSeekRequest() = default;
            FileSeekRequest(AZ::u32 fileHandle, AZ::u32 mode, AZ::s64 offset);
            unsigned int GetMessageType() const override;

            AZ::u32 m_fileHandle;
            AZ::u32 m_seekMode;
            AZ::s64 m_offset;
        };

        class FileSeekResponse
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileSeekResponse, AZ::OSAllocator, 0);
            AZ_RTTI(FileSeekResponse, "{D6D1AD08-1051-4E0D-8F08-41C2460747F3}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            FileSeekResponse() = default;
            FileSeekResponse(AZ::u32 resultCode);
            unsigned int GetMessageType() const override;

            AZ::u32 m_resultCode;
        };

        class FileIsReadOnlyRequest
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileIsReadOnlyRequest, AZ::OSAllocator, 0);
            AZ_RTTI(FileIsReadOnlyRequest, "{408CBD3D-582B-4C78-968D-00BDA9B7CBF3}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::IsReadOnly", 0x6ee110cf);

            FileIsReadOnlyRequest() = default;
            FileIsReadOnlyRequest(const AZ::OSString& filePath);
            unsigned int GetMessageType() const override;

            AZ::OSString m_filePath;
        };

        class FileIsReadOnlyResponse
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileIsReadOnlyResponse, AZ::OSAllocator, 0);
            AZ_RTTI(FileIsReadOnlyResponse, "{E0FC20EC-2563-47DE-9D6B-ADCEB14ED70E}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            FileIsReadOnlyResponse() = default;
            FileIsReadOnlyResponse(bool isReadOnly);
            unsigned int GetMessageType() const override;

            bool m_isReadOnly;
        };

        class PathIsDirectoryRequest
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(PathIsDirectoryRequest, AZ::OSAllocator, 0);
            AZ_RTTI(PathIsDirectoryRequest, "{0F35F08C-4F93-4EA5-8C98-7FB923160A39}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::IsDirectory", 0xfe975e0e);

            PathIsDirectoryRequest() = default;
            PathIsDirectoryRequest(const AZ::OSString& path);
            unsigned int GetMessageType() const override;

            AZ::OSString m_path;
        };

        class PathIsDirectoryResponse
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(PathIsDirectoryResponse, AZ::OSAllocator, 0);
            AZ_RTTI(PathIsDirectoryResponse, "{24BCC53E-1364-4C1E-BB19-7346CB3A2E7D}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            PathIsDirectoryResponse() = default;
            PathIsDirectoryResponse(bool isDir);
            unsigned int GetMessageType() const override;

            bool m_isDir;
        };

        class FileSizeRequest
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileSizeRequest, AZ::OSAllocator, 0);
            AZ_RTTI(FileSizeRequest, "{08F67CF7-A91A-498E-A010-7E3FCDE959FC}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::FileSize", 0x67ffe104);

            FileSizeRequest() = default;
            FileSizeRequest(const AZ::OSString& filePath);
            unsigned int GetMessageType() const override;

            AZ::OSString m_filePath;
        };

        class FileSizeResponse
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileSizeResponse, AZ::OSAllocator, 0);
            AZ_RTTI(FileSizeResponse, "{8FA2402C-5ED4-4B5B-BF64-71D3888A4F0D}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            FileSizeResponse() = default;
            FileSizeResponse(AZ::u32 resultCode, AZ::u64 size);
            unsigned int GetMessageType() const override;

            AZ::u32 m_resultCode;
            AZ::u64 m_size;
        };

        class FileModTimeRequest
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileModTimeRequest, AZ::OSAllocator, 0);
            AZ_RTTI(FileModTimeRequest, "{AFE37457-7EBE-4432-A6CA-78EEDF82F760}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::FileModTime", 0xee6f318c);

            FileModTimeRequest() = default;
            FileModTimeRequest(const AZ::OSString& filePath);
            unsigned int GetMessageType() const override;

            AZ::OSString m_filePath;
        };

        class FileModTimeResponse
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileModTimeResponse, AZ::OSAllocator, 0);
            AZ_RTTI(FileModTimeResponse, "{4F347EBF-74C5-4963-807B-11CB7268AD08}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            FileModTimeResponse() = default;
            FileModTimeResponse(AZ::u64 modTime);
            unsigned int GetMessageType() const override;

            AZ::u64 m_modTime;
        };

        class FileExistsRequest
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileExistsRequest, AZ::OSAllocator, 0);
            AZ_RTTI(FileExistsRequest, "{82751F22-4441-42E7-8187-4D84B97BD2AD}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::FileExists", 0xa2ef8699);

            FileExistsRequest() = default;
            FileExistsRequest(const AZ::OSString& filePath);
            unsigned int GetMessageType() const override;

            AZ::OSString m_filePath;
        };

        class FileExistsResponse
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileExistsResponse, AZ::OSAllocator, 0);
            AZ_RTTI(FileExistsResponse, "{D5B51BB4-4683-476E-BC2F-6906D17EE028}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            FileExistsResponse() = default;
            FileExistsResponse(bool exists);
            unsigned int GetMessageType() const override;

            bool m_exists;
        };

        class FileFlushRequest
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileFlushRequest, AZ::OSAllocator, 0);
            AZ_RTTI(FileFlushRequest, "{0313BA96-2844-4007-9EB8-B98831CA68C7}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::FileFlush", 0x1e7b2735);

            FileFlushRequest() = default;
            FileFlushRequest(AZ::u32 fileHandle);
            unsigned int GetMessageType() const override;

            AZ::u32 m_fileHandle;
        };

        class FileFlushResponse
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileFlushResponse, AZ::OSAllocator, 0);
            AZ_RTTI(FileFlushResponse, "{8085022A-58DB-4CB4-A81C-B32B4B6DBB0A}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            FileFlushResponse() = default;
            FileFlushResponse(AZ::u32 resultCode);
            unsigned int GetMessageType() const override;

            AZ::u32 m_resultCode;
        };

        class PathCreateRequest
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(PathCreateRequest, AZ::OSAllocator, 0);
            AZ_RTTI(PathCreateRequest, "{C7DF8777-2497-4473-8F33-AFD5A4015497}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::PathCreate", 0xc047183b);

            PathCreateRequest() = default;
            PathCreateRequest(const AZ::OSString& path);
            unsigned int GetMessageType() const override;

            AZ::OSString m_path;
        };

        class PathCreateResponse
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(PathCreateResponse, AZ::OSAllocator, 0);
            AZ_RTTI(PathCreateResponse, "{8FA23D48-93E4-453A-BBC3-58C831265B42}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            PathCreateResponse() = default;
            PathCreateResponse(AZ::u32 resultCode);
            unsigned int GetMessageType() const override;

            AZ::u32 m_resultCode;
        };

        class PathDestroyRequest
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(PathDestroyRequest, AZ::OSAllocator, 0);
            AZ_RTTI(PathDestroyRequest, "{628A4C23-1F32-4A35-91F0-C7DFB76FAA9C}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::PathDestroy", 0xe761d533);

            PathDestroyRequest() = default;
            PathDestroyRequest(const AZ::OSString& path);
            unsigned int GetMessageType() const override;

            AZ::OSString m_path;
        };

        class PathDestroyResponse
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(PathDestroyResponse, AZ::OSAllocator, 0);
            AZ_RTTI(PathDestroyResponse, "{850AEAB7-E3AD-4BD4-A08E-78A4E3A62D73}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            PathDestroyResponse() = default;
            PathDestroyResponse(AZ::u32 resultCode);
            unsigned int GetMessageType() const override;

            AZ::u32 m_resultCode;
        };

        class FileRemoveRequest
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileRemoveRequest, AZ::OSAllocator, 0);
            AZ_RTTI(FileRemoveRequest, "{3EB05CEF-D98A-47EC-A688-A485EFB11DC6}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::FileRemove", 0x78f0fd19);

            FileRemoveRequest() = default;
            FileRemoveRequest(const AZ::OSString& filePath);
            unsigned int GetMessageType() const override;

            AZ::OSString m_filePath;
        };

        class FileRemoveResponse
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileRemoveResponse, AZ::OSAllocator, 0);
            AZ_RTTI(FileRemoveResponse, "{1B81110E-7004-462A-98EB-12C3D73477BB}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            FileRemoveResponse() = default;
            FileRemoveResponse(AZ::u32 resultCode);
            unsigned int GetMessageType() const override;

            AZ::u32 m_resultCode;
        };

        class FileCopyRequest
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileCopyRequest, AZ::OSAllocator, 0);
            AZ_RTTI(FileCopyRequest, "{2107C8FD-8150-44A1-B984-AA70D9FD36E2}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::FileCopy", 0xdd857eec);

            FileCopyRequest() = default;
            FileCopyRequest(const AZ::OSString& srcPath, const AZ::OSString& destPath);
            unsigned int GetMessageType() const override;

            AZ::OSString m_srcPath;
            AZ::OSString m_destPath;
        };

        class FileCopyResponse
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileCopyResponse, AZ::OSAllocator, 0);
            AZ_RTTI(FileCopyResponse, "{968DBCE3-2916-47F4-8AB2-A2E12179FB49}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            FileCopyResponse() = default;
            FileCopyResponse(AZ::u32 resultCode);
            unsigned int GetMessageType() const override;

            AZ::u32 m_resultCode;
        };

        class FileRenameRequest
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileRenameRequest, AZ::OSAllocator, 0);
            AZ_RTTI(FileRenameRequest, "{188FD344-DDE2-4C25-BBE0-360F2022B276}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::FileRename", 0xc9edb467);

            FileRenameRequest() = default;
            FileRenameRequest(const AZ::OSString& srcPath, const AZ::OSString& destPath);
            unsigned int GetMessageType() const override;

            AZ::OSString m_srcPath;
            AZ::OSString m_destPath;
        };

        class FileRenameResponse
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileRenameResponse, AZ::OSAllocator, 0);
            AZ_RTTI(FileRenameResponse, "{F553AC26-7C05-4C1B-861D-6C8D934E151D}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            FileRenameResponse() = default;
            FileRenameResponse(AZ::u32 resultCode);
            unsigned int GetMessageType() const override;

            AZ::u32 m_resultCode;
        };

        class FindFilesRequest
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FindFilesRequest, AZ::OSAllocator, 0);
            AZ_RTTI(FindFilesRequest, "{66355EF6-B91F-4E2E-B50A-F59F6E46712D}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::FindFiles", 0xf06cf14f);

            FindFilesRequest() = default;
            FindFilesRequest(const AZ::OSString& path, const AZ::OSString& filter);
            unsigned int GetMessageType() const override;

            AZ::OSString m_path;
            AZ::OSString m_filter;
        };

        class FindFilesResponse
            : public BaseAssetProcessorMessage
        {
        public:
            typedef AZStd::vector<AZ::OSString, AZ::OSStdAllocator> FileList;

            AZ_CLASS_ALLOCATOR(FindFilesResponse, AZ::OSAllocator, 0);
            AZ_RTTI(FindFilesResponse, "{422C7AD1-CEA7-4E1C-B098-687B2A68116F}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            FindFilesResponse() = default;
            FindFilesResponse(AZ::u32 resultCode, const FileList& files = FileList());
            unsigned int GetMessageType() const override;

            AZ::u32 m_resultCode;
            FileList m_files;
        };

        class FileTreeRequest
            : public BaseAssetProcessorMessage
        {
        public:
            AZ_CLASS_ALLOCATOR(FileTreeRequest, AZ::OSAllocator, 0);
            AZ_RTTI(FileTreeRequest, "{6838CC3C-2CF1-443C-BFBF-A530003B6A71}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);
            static constexpr unsigned int MessageType = AZ_CRC("AssetSystem::FileTree", 0x27019bb2);

            FileTreeRequest() = default;
            unsigned int GetMessageType() const override;
        };

        class FileTreeResponse
            : public BaseAssetProcessorMessage
        {
        public:
            typedef AZStd::vector<AZ::OSString, AZ::OSStdAllocator> FileList;
            typedef AZStd::vector<AZ::OSString, AZ::OSStdAllocator> FolderList;

            AZ_CLASS_ALLOCATOR(FileTreeResponse, AZ::OSAllocator, 0);
            AZ_RTTI(FileTreeResponse, "{0F7854DA-63FA-4D59-B298-53D84150DFF9}", BaseAssetProcessorMessage);
            static void Reflect(AZ::ReflectContext* context);

            FileTreeResponse() = default;
            FileTreeResponse(AZ::u32 resultCode
                , const FileList& files = FileList()
                , const FolderList& folders = FolderList());
            unsigned int GetMessageType() const override;

            AZ::u32 m_resultCode;
            FileList m_fileList;
            FolderList m_folderList;
        };


    } // namespace AssetSystem
} // namespace AzFramework
