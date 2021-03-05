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

#include "GradientSignal_precompiled.h"
#include <GradientSignalModule.h>

namespace GradientSignal
{
    class GradientSignalEditorModule
        : public GradientSignalModule
    {
    public:
        AZ_RTTI(GradientSignalEditorModule, "{F8AB732B-3563-4727-9326-3DF2AC42A6D8}", GradientSignalModule);
        AZ_CLASS_ALLOCATOR(GradientSignalEditorModule, AZ::SystemAllocator, 0);

        GradientSignalEditorModule();

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };

    class GradientSignalEditorSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(GradientSignalEditorSystemComponent, "{A3F1E796-7C69-441C-8FA1-3A4001EF2DE3}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    private:
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
    };
}