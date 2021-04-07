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
#include <PrefabBuilderComponent.h>

namespace AZ::Prefab
{
    class PrefabBuilderModule
        : public Module
    {
    public:
        AZ_RTTI(AZ::Prefab::PrefabBuilderModule, "{088B2BA8-9F19-469C-A0B5-1DD523879C70}", Module);
        AZ_CLASS_ALLOCATOR(PrefabBuilderModule, AZ::SystemAllocator, 0);

        PrefabBuilderModule()
            : Module()
        {
            m_descriptors.insert(m_descriptors.end(), {
                PrefabBuilderComponent::CreateDescriptor()
            });
        }
    };
} // namespace AZ::Prefab

AZ_DECLARE_MODULE_CLASS(Gem_Prefab_PrefabBuilder, AZ::Prefab::PrefabBuilderModule)
