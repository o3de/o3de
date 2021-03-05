/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright EntityRef license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Component/Component.h>
#include <ScriptCanvas/Bus/ScriptCanvasExecutionBus.h>
#include <ScriptCanvas/Bus/UnitTestVerificationBus.h>

namespace ScriptCanvasTesting
{

    class ScriptCanvasTestingSystemComponent
        : public AZ::Component
        , public ScriptCanvasEditor::UnitTestVerificationBus::Handler
    {
    public:
        AZ_COMPONENT(ScriptCanvasTestingSystemComponent, "{4D0AA0FD-8451-4AA3-883E-82ADB1C44568}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        ScriptCanvasEditor::UnitTestResult Verify(ScriptCanvasEditor::Reporter reporter) override;

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    };
}
