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

#include <Include/SandboxAPI.h>
#include <AzCore/Component/Component.h>

namespace AzToolsFramework
{
    //! A component to reflect scriptable commands for materials in the Editor
    class MaterialPythonFuncsHandler
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MaterialPythonFuncsHandler, "{E437BCF2-DE71-43E1-A7EC-DD243EB41F0B}")

        SANDBOX_API static void Reflect(AZ::ReflectContext* context);

        // AZ::Component ...
        void Activate() override {}
        void Deactivate() override {}
    };

} // namespace AzToolsFramework
