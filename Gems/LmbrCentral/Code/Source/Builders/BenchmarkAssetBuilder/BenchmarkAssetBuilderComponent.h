/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Builders/BenchmarkAssetBuilder/BenchmarkAssetBuilderWorker.h>

namespace BenchmarkAssetBuilder
{
    //! This component manages the lifetime of the BenchmarkAssetBuilderWorker.
    class BenchmarkAssetBuilderComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(BenchmarkAssetBuilderComponent, "{A1875238-C884-4600-BF89-F5D512F9EE0D}");

        BenchmarkAssetBuilderComponent() = default;
        ~BenchmarkAssetBuilderComponent() override = default;

        void Init() override;
        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    private:
        BenchmarkAssetBuilderWorker m_benchmarkAssetBuilder;
    };
} // namespace AssetBenchmark
