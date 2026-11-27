/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

#include "OpenXRVkAssetsBuilder.h"

namespace OpenXRVkBuilders
{        
    class OpenXRAssetsBuilderSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(OpenXRAssetsBuilderSystemComponent, "{B046B553-CAB4-4AE5-9192-5E002771979B}");
    
        static void Reflect(AZ::ReflectContext* context);
    
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
    
    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    
    private:
        OpenXRAssetsBuilder m_actionSetsAssetBuilder;
    
    };
        
} // namespace OpenXRVkBuilders
