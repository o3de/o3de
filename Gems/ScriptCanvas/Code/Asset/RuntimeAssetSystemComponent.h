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
#include <ScriptCanvas/Asset/AssetRegistry.h>

namespace ScriptCanvas
{
    class RuntimeAssetSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(RuntimeAssetSystemComponent, "{521BF54E-29A9-4367-B9E5-19736AA3A957}");

        RuntimeAssetSystemComponent() = default;
        ~RuntimeAssetSystemComponent() override;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        AssetRegistry& GetAssetRegistry();

    private:
        RuntimeAssetSystemComponent(const RuntimeAssetSystemComponent&) = delete;

        AssetRegistry m_runtimeAssetRegistry;
    };
}
