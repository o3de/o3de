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

/*
This class exposes functionality from the renderer to lua
*/

namespace LmbrCentral
{
    class StereoRendererComponent :
        public AZ::Component
    {
    public:
        AZ_COMPONENT(StereoRendererComponent, "{BBFE0965-5564-4739-8219-AFE8209A5E57}");

        StereoRendererComponent() {};
        ~StereoRendererComponent() override {};

        void Activate() override {};
        void Deactivate() override {};

        static void Reflect(AZ::ReflectContext* context);
    };
}
