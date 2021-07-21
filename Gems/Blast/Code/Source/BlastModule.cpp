/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "StdAfx.h"

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <Components/BlastFamilyComponent.h>
#include <Components/BlastMeshDataComponent.h>
#include <Components/BlastSystemComponent.h>
#include <IGem.h>

#ifdef BLAST_EDITOR
#include <Editor/EditorBlastFamilyComponent.h>
#include <Editor/EditorBlastMeshDataComponent.h>
#include <Editor/EditorBlastSliceAssetHandler.h>
#include <Editor/EditorSystemComponent.h>
#endif

namespace Blast
{
    class BlastModule : public CryHooksModule
    {
    public:
        AZ_RTTI(BlastModule, "{897CCA50-FBAF-4F5A-A859-1951091E0555}", CryHooksModule);

        BlastModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(
                m_descriptors.end(),
                {
                    BlastSystemComponent::CreateDescriptor(),
                    BlastFamilyComponent::CreateDescriptor(),
                    BlastMeshDataComponent::CreateDescriptor(),
#ifdef BLAST_EDITOR
                    EditorSystemComponent::CreateDescriptor(),
                    EditorBlastFamilyComponent::CreateDescriptor(),
                    EditorBlastMeshDataComponent::CreateDescriptor(),
                    BlastSliceAssetStorageComponent::CreateDescriptor(),
#endif
                });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<BlastSystemComponent>(),
#ifdef BLAST_EDITOR
                azrtti_typeid<EditorSystemComponent>(),
#endif
            };
        }
    };
} // namespace Blast

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_Blast, Blast::BlastModule)
