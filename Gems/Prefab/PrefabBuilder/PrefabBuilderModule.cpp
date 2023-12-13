/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Module/Module.h>
#include <PrefabBuilderComponent.h>
#include <PrefabGroup/PrefabGroupBehavior.h>

namespace AZ::Prefab
{
    class PrefabBuilderModule
        : public Module
    {
    public:
        AZ_RTTI(AZ::Prefab::PrefabBuilderModule, "{088B2BA8-9F19-469C-A0B5-1DD523879C70}", Module);
        AZ_CLASS_ALLOCATOR(PrefabBuilderModule, AZ::SystemAllocator);

        PrefabBuilderModule()
            : Module()
        {
            m_descriptors.insert(m_descriptors.end(), {
                PrefabBuilderComponent::CreateDescriptor(),
                AZ::SceneAPI::Behaviors::PrefabGroupBehavior::CreateDescriptor()
            });
        }
    };
} // namespace AZ::Prefab

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Builders), AZ::Prefab::PrefabBuilderModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_PrefabBuilder_Builders, AZ::Prefab::PrefabBuilderModule)
#endif
