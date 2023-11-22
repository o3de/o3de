/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <SaveDataSystemComponent.h>

namespace SaveData
{
    class SaveDataModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(SaveDataModule, "{4FD9776B-0C36-476F-A7C4-161404BCCCF3}", AZ::Module);
        AZ_CLASS_ALLOCATOR(SaveDataModule, AZ::SystemAllocator);

        SaveDataModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                SaveDataSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<SaveDataSystemComponent>(),
            };
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), SaveData::SaveDataModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_SaveData, SaveData::SaveDataModule)
#endif
