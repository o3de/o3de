/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
