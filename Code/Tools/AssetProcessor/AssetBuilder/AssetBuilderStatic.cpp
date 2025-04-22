/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetBuilderStatic.h>
#include <AzCore/Component/ComponentApplicationBus.h>

namespace AssetBuilder
{
    void Reflect(AZ::ReflectContext* context)
    {
        BuilderRegistrationRequest::Reflect(context);

        BuilderHelloRequest::Reflect(context);
        BuilderHelloResponse::Reflect(context);
        CreateJobsNetRequest::Reflect(context);
        CreateJobsNetResponse::Reflect(context);
        ProcessJobNetRequest::Reflect(context);
        ProcessJobNetResponse::Reflect(context);
    }

    void InitializeSerializationContext()
    {
        AZ::SerializeContext* serializeContext = nullptr;

        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(serializeContext, "Unable to retrieve serialize context.");

        Reflect(serializeContext);
    }

    void BuilderHelloRequest::Reflect(AZ::ReflectContext* context)
    {
        auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<BuilderHelloRequest>()->Version(1)->Field("UUID", &BuilderHelloRequest::m_uuid);
        }
    }

    unsigned int BuilderHelloRequest::MessageType()
    {
        static unsigned int messageType = AZ_CRC_CE("AssetBuilderSDK::BuilderHelloRequest");

        return messageType;
    }

    unsigned int BuilderHelloRequest::GetMessageType() const
    {
        return MessageType();
    }

    void BuilderHelloResponse::Reflect(AZ::ReflectContext* context)
    {
        auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<BuilderHelloResponse>()
                ->Version(1)
                ->Field("Accepted", &BuilderHelloResponse::m_accepted)
                ->Field("UUID", &BuilderHelloResponse::m_uuid);
        }
    }

    unsigned int BuilderHelloResponse::GetMessageType() const
    {
        return BuilderHelloRequest::MessageType();
    }

    //////////////////////////////////////////////////////////////////////////

    void CreateJobsNetRequest::Reflect(AZ::ReflectContext* context)
    {
        auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<CreateJobsNetRequest>()->Version(1)->Field("Request", &CreateJobsNetRequest::m_request);
        }
    }

    unsigned int CreateJobsNetRequest::MessageType()
    {
        static unsigned int messageType = AZ_CRC_CE("AssetBuilderSDK::CreateJobsNetRequest");

        return messageType;
    }

    unsigned int CreateJobsNetRequest::GetMessageType() const
    {
        return MessageType();
    }

    void CreateJobsNetResponse::Reflect(AZ::ReflectContext* context)
    {
        auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<CreateJobsNetResponse>()->Version(1)->Field("Response", &CreateJobsNetResponse::m_response);
        }
    }

    unsigned int CreateJobsNetResponse::GetMessageType() const
    {
        return CreateJobsNetRequest::MessageType();
    }

    void ProcessJobNetRequest::Reflect(AZ::ReflectContext* context)
    {
        auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<ProcessJobNetRequest>()->Version(1)->Field("Request", &ProcessJobNetRequest::m_request);
        }
    }

    unsigned int ProcessJobNetRequest::MessageType()
    {
        static unsigned int messageType = AZ_CRC_CE("AssetBuilderSDK::ProcessJobNetRequest");

        return messageType;
    }

    unsigned int ProcessJobNetRequest::GetMessageType() const
    {
        return MessageType();
    }

    void ProcessJobNetResponse::Reflect(AZ::ReflectContext* context)
    {
        auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<ProcessJobNetResponse>()->Version(1)->Field("Response", &ProcessJobNetResponse::m_response);
        }
    }

    unsigned int ProcessJobNetResponse::GetMessageType() const
    {
        return ProcessJobNetRequest::MessageType();
    }

    //---------------------------------------------------------------------
    void BuilderRegistration::Reflect(AZ::ReflectContext* context)
    {
        auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<BuilderRegistration>()
                ->Version(1)
                ->Field("Name", &BuilderRegistration::m_name)
                ->Field("Patterns", &BuilderRegistration::m_patterns)
                ->Field("BusId", &BuilderRegistration::m_busId)
                ->Field("Version", &BuilderRegistration::m_version)
                ->Field("AnalysisFingerprint", &BuilderRegistration::m_analysisFingerprint)
                ->Field("Flags", &BuilderRegistration::m_flags)
                ->Field("FlagsByJobKey", &BuilderRegistration::m_flagsByJobKey)
                ->Field("ProductsToKeepOnFailure", &BuilderRegistration::m_productsToKeepOnFailure);
        }
    }

    void BuilderRegistrationRequest::Reflect(AZ::ReflectContext* context)
    {
        BuilderRegistration::Reflect(context);

        auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<BuilderRegistrationRequest, BaseAssetProcessorMessage>()->Version(1)->Field(
                "Builders", &BuilderRegistrationRequest::m_builders);
        }
    }

    unsigned int BuilderRegistrationRequest::GetMessageType() const
    {
        return BuilderRegistrationRequest::MessageType;
    }
    
} // namespace AssetBuilder
