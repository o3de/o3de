/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace AssetBuilder
{
    void Reflect(AZ::ReflectContext* context);

    void InitializeSerializationContext();

    //! BuilderHelloRequest is sent by an AssetBuilder that is attempting to connect to the AssetProcessor to register itself as a worker
    class BuilderHelloRequest : public AzFramework::AssetSystem::BaseAssetProcessorMessage
    {
    public:
        AZ_CLASS_ALLOCATOR(BuilderHelloRequest, AZ::OSAllocator);
        AZ_RTTI(BuilderHelloRequest, "{5fab5962-a1d8-42a5-bf7a-fb1a8c5a9588}", BaseAssetProcessorMessage);

        static void Reflect(AZ::ReflectContext* context);
        static unsigned int MessageType();

        unsigned int GetMessageType() const override;

        //! Unique ID assigned to this builder to identify it
        AZ::Uuid m_uuid = AZ::Uuid::CreateNull();
    };

    //! BuilderHelloResponse contains the AssetProcessor's response to a builder connection attempt, indicating if it is accepted and the ID
    //! that it was assigned
    class BuilderHelloResponse : public AzFramework::AssetSystem::BaseAssetProcessorMessage
    {
    public:
        AZ_CLASS_ALLOCATOR(BuilderHelloResponse, AZ::OSAllocator);
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
        AZ_CLASS_ALLOCATOR(CreateJobsNetRequest, AZ::OSAllocator);
        AZ_RTTI(CreateJobsNetRequest, "{97fa717d-3a09-4d21-95c6-b2eafd773f1c}", BaseAssetProcessorMessage);

        static void Reflect(AZ::ReflectContext* context);
        static unsigned int MessageType();

        unsigned int GetMessageType() const override;

        AssetBuilderSDK::CreateJobsRequest m_request;
    };

    class CreateJobsNetResponse : public AzFramework::AssetSystem::BaseAssetProcessorMessage
    {
    public:
        AZ_CLASS_ALLOCATOR(CreateJobsNetResponse, AZ::OSAllocator);
        AZ_RTTI(CreateJobsNetResponse, "{b2c7c2d3-b60e-4b27-b699-43e0ba991c33}", BaseAssetProcessorMessage);

        static void Reflect(AZ::ReflectContext* context);

        unsigned int GetMessageType() const override;

        AssetBuilderSDK::CreateJobsResponse m_response;
    };

    class ProcessJobNetRequest : public AzFramework::AssetSystem::BaseAssetProcessorMessage
    {
    public:
        AZ_CLASS_ALLOCATOR(ProcessJobNetRequest, AZ::OSAllocator);
        AZ_RTTI(ProcessJobNetRequest, "{05288de1-020b-48db-b9de-715f17284efa}", BaseAssetProcessorMessage);

        static void Reflect(AZ::ReflectContext* context);
        static unsigned int MessageType();

        unsigned int GetMessageType() const override;

        AssetBuilderSDK::ProcessJobRequest m_request;
    };

    class ProcessJobNetResponse : public AzFramework::AssetSystem::BaseAssetProcessorMessage
    {
    public:
        AZ_CLASS_ALLOCATOR(ProcessJobNetResponse, AZ::OSAllocator);
        AZ_RTTI(ProcessJobNetResponse, "{26ddf882-246c-4cfb-912f-9b8e389df4f6}", BaseAssetProcessorMessage);

        static void Reflect(AZ::ReflectContext* context);

        unsigned int GetMessageType() const override;

        AssetBuilderSDK::ProcessJobResponse m_response;
    };

    //////////////////////////////////////////////////////////////////////////
    struct BuilderRegistration
    {
        AZ_CLASS_ALLOCATOR(BuilderRegistration, AZ::OSAllocator);
        AZ_TYPE_INFO(BuilderRegistration, "{36E785C3-5046-4568-870A-336C8249E453}");

        static void Reflect(AZ::ReflectContext* context);

        AZStd::string m_name;
        AZStd::vector<AssetBuilderSDK::AssetBuilderPattern> m_patterns;
        AZ::Uuid m_busId;
        int m_version = 0;
        AZStd::string m_analysisFingerprint;
        AZ::u8 m_flags = 0;
        AZStd::unordered_map<AZStd::string, AZ::u8> m_flagsByJobKey;
        AZStd::unordered_map<AZStd::string, AZStd::unordered_set<AZ::u32>> m_productsToKeepOnFailure;
    };

    class BuilderRegistrationRequest : public AzFramework::AssetSystem::BaseAssetProcessorMessage
    {
    public:
        AZ_CLASS_ALLOCATOR(BuilderRegistrationRequest, AZ::OSAllocator);
        AZ_RTTI(BuilderRegistrationRequest, "{FA9CF2D5-C847-47F3-979D-6C3AE061715C}", BaseAssetProcessorMessage);
        static void Reflect(AZ::ReflectContext* context);
        static constexpr unsigned int MessageType = AZ_CRC_CE("AssetSystem::BuilderRegistrationRequest");

        BuilderRegistrationRequest() = default;
        unsigned int GetMessageType() const override;

        AZStd::vector<BuilderRegistration> m_builders;
    };
} // namespace AssetBuilder
