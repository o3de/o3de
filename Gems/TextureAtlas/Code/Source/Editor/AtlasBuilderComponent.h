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
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include "AtlasBuilderWorker.h"

namespace TextureAtlasBuilder
{
    class AtlasBuilderComponent : public AZ::Component
    {
    public:
        AZ_COMPONENT(AtlasBuilderComponent, "{F49987FB-3375-4417-AB83-97B44C78B335}");

        AtlasBuilderComponent();
        ~AtlasBuilderComponent() override;

        void Init() override;
        void Activate() override;
        void Deactivate() override;

        //! Reflect formats for input and output
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    private:
        AtlasBuilderWorker m_atlasBuilder;
    };
} // namespace TextureAtlasBuilder
