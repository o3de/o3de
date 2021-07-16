/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <CustomAssetExample/Builder/CustomAssetExampleBuilderWorker.h>

namespace CustomAssetExample
{
    //! Here's an example of the lifecycle Component you must implement.
    //! You must have at least one component to handle the lifecycle of your builder classes.
    //! This could be a builder class if you implement the the builder bus handler, and register itself as the builder class
    //! But for the purposes of clarity, we will make it just be the lifecycle component in this example
    class ExampleBuilderComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(ExampleBuilderComponent, "{8872211E-F704-48A9-B7EB-7B80596D871D}");

        ExampleBuilderComponent();
        ~ExampleBuilderComponent() override;

        void Init() override;
        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    private:
        ExampleBuilderWorker m_exampleBuilder;
    };
} // namespace CustomAssetExample
