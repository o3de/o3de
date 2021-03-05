/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Component/Component.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include "SliceBuilderWorker.h"

namespace SliceBuilder
{
    //! SliceBuilder is responsible for compiling slices
    class BuilderPluginComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(BuilderPluginComponent, "{092f00f2-aa25-43a9-a8c9-2421531ea31a}")
        static void Reflect(AZ::ReflectContext* context);

        BuilderPluginComponent() = default;
        virtual ~BuilderPluginComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

    private:
        BuilderPluginComponent(const BuilderPluginComponent&) = delete;

        AZStd::unique_ptr<SliceBuilderWorker> m_sliceBuilder;
    };
}
