/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/


#include <MeshletsModuleInterface.h>
#include <MeshletsSystemComponent.h>

namespace AZ
{
    namespace Meshlets
    {
        class MeshletsModule
            : public MeshletsModuleInterface
        {
        public:
            AZ_RTTI(MeshletsModule, "{19bbf909-a4fc-48ec-915a-316046feb2f9}", MeshletsModuleInterface);
            AZ_CLASS_ALLOCATOR(MeshletsModule, AZ::SystemAllocator);
        };
    } // namespace Meshlets
} // namespace AZ

AZ_DECLARE_MODULE_CLASS(Gem_Meshlets, AZ::Meshlets::MeshletsModule)
