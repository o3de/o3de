/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>

namespace AZ
{
    namespace Render
    {
        class AtomLyIntegrationCommonFeaturesSystemComponent
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(AtomLyIntegrationCommonFeaturesSystemComponent, "{E29DAE7C-BF0F-42A9-9570-037FBB689A0A}");

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

        };
    } // namespace Render
} // namespace AZ
