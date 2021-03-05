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
AZ_DECLARE_MODULE_CLASS(Blast_414bd211c99d4f74aef3a266b9ca208c, Blast::BlastModule)
