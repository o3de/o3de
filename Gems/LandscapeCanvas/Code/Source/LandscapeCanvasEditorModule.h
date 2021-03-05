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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

namespace LandscapeCanvas
{
    class LandscapeCanvasEditorModule
        : public  AZ::Module
    {
    public:
        AZ_RTTI(LandscapeCanvasEditorModule, "{5E539B81-792E-4BE5-BCA2-95C5D826E75B}", AZ::Module);
        AZ_CLASS_ALLOCATOR(LandscapeCanvasEditorModule, AZ::SystemAllocator, 0);

        LandscapeCanvasEditorModule();

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };

    class LandscapeCanvasEditorSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(LandscapeCanvasEditorSystemComponent, "{11402EA3-57FF-4086-A980-228EEA0CDAF3}");

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
