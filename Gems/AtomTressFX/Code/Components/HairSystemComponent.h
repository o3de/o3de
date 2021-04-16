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
#include <AzCore/Asset/AssetCommon.h>

namespace AZ
{
    namespace Render
    {
        class SharedBuffer;

        namespace Hair
        {
            class HairSystemComponent final
                : public Component
            {
            public:
                AZ_COMPONENT(HairSystemComponent, "{F3A56326-1D2F-462D-A9E8-0405A76601A5}");

                HairSystemComponent();
                ~HairSystemComponent();

                static void Reflect(ReflectContext* context);

                static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
                static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible);
                static void GetRequiredServices(ComponentDescriptor::DependencyArrayType& required);

            private:

                ////////////////////////////////////////////////////////////////////////
                // Component interface implementation
                void Init() override;
                void Activate() override;
                void Deactivate() override;
                ////////////////////////////////////////////////////////////////////////
            };
        } // namespace Hair
    } // End Render namespace
} // End AZ namespace
