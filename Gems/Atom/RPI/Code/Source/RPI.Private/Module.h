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

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Module/Module.h>

#include <RPI.Private/RPISystemComponent.h>

namespace AZ
{
    namespace RPI
    {
        class Module
            : public AZ::Module
        {
        public:
            AZ_RTTI(Module, "{CDB54E96-717D-4DFC-BEA6-F809BDE601AE}", AZ::Module);

            Module();
            ~Module() override = default;

            AZ::ComponentTypeList GetRequiredSystemComponents() const override;
        };
    } // namespace RPI
} // namespace RPI
