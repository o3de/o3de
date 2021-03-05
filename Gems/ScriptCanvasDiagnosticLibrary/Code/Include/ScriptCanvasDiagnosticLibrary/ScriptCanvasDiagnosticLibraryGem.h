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

#include <AzCore/Module/Module.h>
#include <AzCore/Module/Environment.h>

namespace ScriptCanvasDiagnostics
{
    class Module
        : public AZ::Module
    {
    public:
        AZ_RTTI(Module, "{4855DE9C-9804-4CE5-BA32-B0123356F48E}", AZ::Module);

        Module();

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
