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
        class ActorSystemComponent
            : public Component
        {
        public:
            AZ_COMPONENT(ActorSystemComponent, "{F055EF7C-1C66-4CEB-879C-6871F3347FF9}");

            ActorSystemComponent();
            ~ActorSystemComponent();
            
            static void Reflect(ReflectContext* context);

            static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(ComponentDescriptor::DependencyArrayType& required);

        protected:

            ////////////////////////////////////////////////////////////////////////
            // Component interface implementation
            void Activate() override;
            void Deactivate() override;
            ////////////////////////////////////////////////////////////////////////
        };
    } // End Render namespace
} // End AZ namespace
