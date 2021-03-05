/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Component/Component.h>
#include <HttpRequestor/HttpRequestorBus.h>
#include "HttpRequestManager.h"

namespace HttpRequestor
{
    class HttpRequestorSystemComponent
        : public AZ::Component
        , protected HttpRequestorRequestBus::Handler
    {
    public:
        AZ_COMPONENT(HttpRequestorSystemComponent, "{CF29468F-1F67-497F-B4FF-C0F123584864}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        
    protected:
        ////////////////////////////////////////////////////////////////////////
        // HttpRequestorRequestBus interface implementation
        ////////////////////////////////////////////////////////////////////////
        void AddRequest(const AZStd::string& URI, Aws::Http::HttpMethod method, const Callback& callback) override;
        void AddRequestWithHeaders(const AZStd::string& URI, Aws::Http::HttpMethod method, const Headers & headers, const Callback& callback) override;
        void AddRequestWithHeadersAndBody(const AZStd::string& URI, Aws::Http::HttpMethod method, const Headers & headers, const AZStd::string& body, const Callback& callback) override;

        void AddTextRequest(const AZStd::string& URI, Aws::Http::HttpMethod method, const TextCallback& callback) override;
        void AddTextRequestWithHeaders(const AZStd::string& URI, Aws::Http::HttpMethod method, const Headers & headers, const TextCallback& callback) override;
        void AddTextRequestWithHeadersAndBody(const AZStd::string& URI, Aws::Http::HttpMethod method, const Headers & headers, const AZStd::string& body, const TextCallback& callback) override;

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    private:
        ManagerPtr              m_httpManager;
    };
}

