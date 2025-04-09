
/*
* Modifications Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#include <MeshletsModuleInterface.h>
#include <MeshletsEditorSystemComponent.h>

namespace AZ
{
    namespace Meshlets
    {
        class MeshletsEditorModule
            : public MeshletsModuleInterface
        {
        public:
            AZ_RTTI(MeshletsEditorModule, "{19bbf909-a4fc-48ec-915a-316046feb2f9}", MeshletsModuleInterface);
            AZ_CLASS_ALLOCATOR(MeshletsEditorModule, AZ::SystemAllocator);

            MeshletsEditorModule()
            {
                // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
                // Add ALL components descriptors associated with this gem to m_descriptors.
                // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
                // This happens through the [MyComponent]::Reflect() function.
                m_descriptors.insert(m_descriptors.end(), {
                    MeshletsEditorSystemComponent::CreateDescriptor(),
                });
            }

            /**
             * Add required SystemComponents to the SystemEntity.
             * Non-SystemComponents should not be added here
             */
            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return AZ::ComponentTypeList {
                    azrtti_typeid<MeshletsEditorSystemComponent>(),
                };
            }
        };
    } // namespace Meshlets
} // namespace AZ

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), AZ::Meshlets::MeshletsEditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Meshlets_Editor, AZ::Meshlets::MeshletsEditorModule)
#endif
