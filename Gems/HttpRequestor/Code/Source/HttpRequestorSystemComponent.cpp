/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "HttpRequestorSystemComponent.h"

namespace HttpRequestor
{
    void HttpRequestorSystemComponent::AddRequest(const AZStd::string& URI, Aws::Http::HttpMethod method, const Callback& callback)
    {
        if(m_httpManager != nullptr)
        {
            m_httpManager->AddRequest( Parameters(URI, method, callback) );
        }
    }

    void HttpRequestorSystemComponent::AddRequestWithHeaders(const AZStd::string& URI, Aws::Http::HttpMethod method, const Headers & headers, const Callback& callback)
    {
        if (m_httpManager != nullptr)
        {
            m_httpManager->AddRequest(Parameters(URI, method, headers, callback));
        }
    }

    void HttpRequestorSystemComponent::AddRequestWithHeadersAndBody(const AZStd::string& URI, Aws::Http::HttpMethod method, const Headers & headers, const AZStd::string& body, const Callback& callback)
    {
        if (m_httpManager != nullptr)
        {
            m_httpManager->AddRequest(Parameters(URI, method, headers, body, callback));
        }
    }

    void HttpRequestorSystemComponent::AddTextRequest(const AZStd::string& URI, Aws::Http::HttpMethod method, const TextCallback& callback)
    {
        if (m_httpManager != nullptr)
        {
            m_httpManager->AddTextRequest( TextParameters(URI, method, callback));
        }
    }

    void HttpRequestorSystemComponent::AddTextRequestWithHeaders(const AZStd::string& URI, Aws::Http::HttpMethod method, const Headers & headers, const TextCallback& callback)
    {
        if (m_httpManager != nullptr)
        {
            m_httpManager->AddTextRequest(TextParameters(URI, method, headers, callback));
        }
    }

    void HttpRequestorSystemComponent::AddTextRequestWithHeadersAndBody(const AZStd::string& URI, Aws::Http::HttpMethod method, const Headers & headers, const AZStd::string& body, const TextCallback& callback)
    {
        if (m_httpManager != nullptr)
        {
            m_httpManager->AddTextRequest(TextParameters(URI, method, headers, body, callback));
        }
    }
    
    void HttpRequestorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<HttpRequestorSystemComponent, AZ::Component>()->Version(1);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<HttpRequestorSystemComponent>("HttpRequestor", "Will make HTTP Rest calls")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    // ->Attribute(AZ::Edit::Attributes::Category, "") Set a category
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    void HttpRequestorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("HttpRequestorService"));
    }

    void HttpRequestorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("HttpRequestorService"));
    }

    void HttpRequestorSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void HttpRequestorSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void HttpRequestorSystemComponent::Init()
    {
    }

    void HttpRequestorSystemComponent::Activate()
    {
        m_httpManager = AZStd::make_shared<Manager>();
        
        HttpRequestorRequestBus::Handler::BusConnect();
    }

    void HttpRequestorSystemComponent::Deactivate()
    {
        HttpRequestorRequestBus::Handler::BusDisconnect();

        m_httpManager = nullptr;
    }
}
