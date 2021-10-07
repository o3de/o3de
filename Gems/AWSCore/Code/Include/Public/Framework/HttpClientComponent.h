/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>



#include <Framework/AWSApiClientJob.h>


namespace AWSCore
{
    /////////////////////////////////////////////
    // EBus Definitions
    /////////////////////////////////////////////
    class HttpClientComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~HttpClientComponentRequests() {}
        virtual void MakeHttpRequest(AZStd::string url, AZStd::string method, AZStd::string jsonBody) {}
    };

    using HttpClientComponentRequestBus = AZ::EBus<HttpClientComponentRequests>;

    class HttpClientComponentNotifications
        : public AZ::ComponentBus
    {
    public:
        ~HttpClientComponentNotifications() override = default;
        virtual void OnHttpRequestSuccess(int responseCode, AZStd::string responseBody) {}
        virtual void OnHttpRequestFailure(int responseCode) {}
    };

    using HttpClientComponentNotificationBus = AZ::EBus<HttpClientComponentNotifications>;

    /////////////////////////////////////////////
    // Entity Component
    /////////////////////////////////////////////
    class HttpClientComponent
        : public AZ::Component
        , public HttpClientComponentRequestBus::Handler
    {
    public:
        AZ_COMPONENT(HttpClientComponent, "{23ECDBDF-129A-4670-B9B4-1E0B541ACD61}");
        ~HttpClientComponent() override = default;

        void Init() override;
        void Activate() override;
        void Deactivate() override;

        void MakeHttpRequest(AZStd::string, AZStd::string, AZStd::string) override;

        static void Reflect(AZ::ReflectContext*);
    };

} // namespace AWSCore
