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

#include <AzCore/Module/Module.h>

namespace AZ
{
    namespace DX12
    {
        class PlatformModule final
            : public AZ::Module
        {
        public:
            AZ_RTTI(PlatformModule, "{A5C04CF5-715E-4456-ABF3-A8DB30868D77}", AZ::Module);

            PlatformModule() = default;
            ~PlatformModule() override = default;

            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return AZ::ComponentTypeList();
            }
        };
    } // namespace DX12
} // namespace AZ

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_Atom_RHI_DX12_Private, AZ::DX12::PlatformModule)
