/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
