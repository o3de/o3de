/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        // AZ::Component...
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
