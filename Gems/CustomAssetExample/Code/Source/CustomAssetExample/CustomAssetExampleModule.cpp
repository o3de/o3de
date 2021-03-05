/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#if !defined(CUSTOM_ASSET_EXAMPLE_EDITOR)
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Module/Module.h>

/**
* Currently, our example asset does not have any runtime components.
* Therefore, this is just a stub module with the minimum amount of initialization to register properly in the AZ::Module system
*/
namespace CustomAssetExample
{
    class CustomAssetExampleModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(CustomAssetExampleModule, "{B986F478-FDC1-4A7A-A7D5-D72246C47017}", AZ::Module);
        CustomAssetExampleModule() = default;
    };
} // namespace CustomAssetExample

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_CustomAssetExample, CustomAssetExample::CustomAssetExampleModule)

#endif // !defined(CUSTOM_ASSET_EXAMPLE_EDITOR)
