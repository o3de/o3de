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
#include <Vegetation/Ebuses/DebugSystemDataBus.h>

namespace Vegetation
{

    /**
    * The system component that manages debug data
    */
    class DebugSystemComponent
        : public AZ::Component
        , public DebugSystemDataBus::Handler
    {
    public:
        AZ_COMPONENT(DebugSystemComponent, "{A8E3D8D4-B6A5-48A6-90D6-153F9EFDF75E}");
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        DebugSystemComponent();
        ~DebugSystemComponent() override;

        //////////////////////////////////////////////////////////////////////////
        // DebugSystemDataBus
        DebugData* GetDebugData() override;

    private:
        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        DebugData m_debugData;
    };

} // namespace Vegetation

