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
