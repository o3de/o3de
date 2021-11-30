/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ActorSystemComponent.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Module/Module.h>
#ifdef EMOTIONFXATOM_EDITOR
#include <Editor/EditorSystemComponent.h>
#endif

namespace AZ
{
    namespace Render
    {
        //! Some Atom projects will not include EMotionFX, and some projects using EMotionFX will not include Atom. This module exists to prevent creating a hard dependency in either direction.
        class ActorModule
            : public Module
        {
        public:
            AZ_RTTI(ActorModule, "{84DCA4A9-39A1-4A04-A7DE-66FF62A3B7AD}", Module);
            AZ_CLASS_ALLOCATOR(ActorModule, SystemAllocator, 0);

            ActorModule()
                : Module()
            {
                m_descriptors.insert(m_descriptors.end(), {
                    ActorSystemComponent::CreateDescriptor(),
#ifdef EMOTIONFXATOM_EDITOR
                    EMotionFXAtom::EditorSystemComponent::CreateDescriptor(),
#endif
                });
            }

            //!
            //! Add required SystemComponents to the SystemEntity.
            //!
            ComponentTypeList GetRequiredSystemComponents() const override
            {
                return ComponentTypeList{
                    azrtti_typeid<ActorSystemComponent>(),
#ifdef EMOTIONFXATOM_EDITOR
                    azrtti_typeid<EMotionFXAtom::EditorSystemComponent>(),
#endif
                };
            }
        };
    } // end Render namespace
} // end AZ namespace

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_EMotionFX_Atom, AZ::Render::ActorModule)
