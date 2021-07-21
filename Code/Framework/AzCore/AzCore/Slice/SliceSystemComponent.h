/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include "SliceAssetHandler.h"

namespace AZ
{
    class SliceAssetHandler;

    /**
     * Performs management for the Slice (aka Prefab) system.
     */
    class SliceSystemComponent : public Component
    {
    public:
        AZ_COMPONENT(SliceSystemComponent, "{1712C054-1C84-4995-BA06-AFE99CCFE08B}");
        SliceSystemComponent() = default;
        static void Reflect(ReflectContext* context);

        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& services);
        static void GetDependentServices(ComponentDescriptor::DependencyArrayType& services);

    protected:
        SliceSystemComponent(const SliceSystemComponent&) = delete;
        void Activate() override;
        void Deactivate() override;

        SliceAssetHandler m_assetHandler;
    };
} // namespace AZ
