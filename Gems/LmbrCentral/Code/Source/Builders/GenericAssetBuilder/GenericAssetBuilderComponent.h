/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Builders/GenericAssetBuilder/GenericAssetBuilderWorker.h>

namespace LmbrCentral::GenericAssetBuilder
{
    //! This component manages the lifetime of the GenericAssetBuilder.
    class GenericAssetBuilderComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(GenericAssetBuilderComponent, "{F54C592B-99CA-44BF-8060-B8182AE949F4}");

        GenericAssetBuilderComponent() = default;
        ~GenericAssetBuilderComponent() override = default;

        void Init() override;
        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    private:
        GenericAssetBuilderWorker m_genericAssetBuilder;
    };
} // namespace LmbrCentral::GenericAssetBuilder
