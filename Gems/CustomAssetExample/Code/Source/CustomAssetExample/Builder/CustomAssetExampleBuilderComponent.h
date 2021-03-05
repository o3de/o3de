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
