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

#ifndef AZFRAMEWORK_NET_BINDING_SYSTEM_COMPONENT_H
#define AZFRAMEWORK_NET_BINDING_SYSTEM_COMPONENT_H

#include <AzFramework/Network/NetBindingSystemImpl.h>
#include <AzCore/Component/Component.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzFramework
{
    /**
    * NetBindingSystemComponent exposes NetBindingSystemImpl as a component
    */
    class NetBindingSystemComponent
        : public AZ::Component
        , public NetBindingSystemImpl
    {
        friend class NetBindingSystemContextData;
    public:
        AZ_COMPONENT(NetBindingSystemComponent, "{B96548CC-0866-4BB3-A87B-BF0C4F69E8AC}");

        NetBindingSystemComponent();
        ~NetBindingSystemComponent() override;

        //////////////////////////////////////////////////////////////////////////
        // Component overrides
        void Activate() override;
        void Deactivate() override;
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        //////////////////////////////////////////////////////////////////////////
    };
} // namespace AzFramework

#endif // AZFRAMEWORK_NET_BINDING_SYSTEM_COMPONENT_H
#pragma once

