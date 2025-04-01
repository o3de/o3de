
/*
* Modifications Copyright (c) Contributors to the Open 3D Engine Project. 
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
* 
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <MeshletsSystemComponent.h>

namespace AZ
{
    namespace Meshlets
    {
        class MeshletsModuleInterface
            : public AZ::Module
        {
        public:
            AZ_RTTI(MeshletsModuleInterface, "{78d5f887-59e1-4ffd-b3d5-b1b2b3e94039}", AZ::Module);
            AZ_CLASS_ALLOCATOR(MeshletsModuleInterface, AZ::SystemAllocator);

            MeshletsModuleInterface()
            {
                // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
                // Add ALL components descriptors associated with this gem to m_descriptors.
                // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
                // This happens through the [MyComponent]::Reflect() function.
                m_descriptors.insert(m_descriptors.end(), {
                    MeshletsSystemComponent::CreateDescriptor(),
                    });
            }

            /**
             * Add required SystemComponents to the SystemEntity.
             */
            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return AZ::ComponentTypeList{
                    azrtti_typeid<MeshletsSystemComponent>(),
                };
            }
        };
    } // namespace Meshlets
} // namespace AZ

