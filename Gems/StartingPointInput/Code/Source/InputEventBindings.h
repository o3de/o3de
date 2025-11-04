/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/RTTI/TypeInfo.h>
#include <AzFramework/Asset/SimpleAsset.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/Memory.h>
#include "InputEventGroup.h"

namespace StartingPointInput
{
    /*!
    * InputEventBinding asset type configuration.
    * Reflect as: AzFramework::SimpleAssetReference<InputEventBindings>
    * This base class holds a list of InputEventGroups which organizes raw input processors by the 
    * gameplay events they generate, Ex. Held(eKI_Space) -> "Jump"
    */
    class InputEventBindings
    {
    public:
        virtual ~InputEventBindings()
        {
        }

        AZ_CLASS_ALLOCATOR(InputEventBindings, AZ::SystemAllocator);
        AZ_RTTI(InputEventBindings, "{14FFD4A8-AE46-4E23-B45B-6A7C4F787A91}")
       
        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<InputEventBindings>()
                    ->Version(1)
                    ->Field("Input Event Groups", &InputEventBindings::m_inputEventGroups);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<InputEventBindings>("Input Event Bindings", "Holds InputEventBindings")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->DataElement(0, &InputEventBindings::m_inputEventGroups, "Input Event Groups", "Input Event Groups");
                }
            }
        }

        void Activate(const AzFramework::LocalUserId& localUserId)
        {
            for (InputEventGroup& inputEventGroup : m_inputEventGroups)
            {
                inputEventGroup.Activate(localUserId);
            }
        }

        void Deactivate(const AzFramework::LocalUserId& localUserId)
        {
            for (InputEventGroup& inputEventGroup : m_inputEventGroups)
            {
                inputEventGroup.Deactivate(localUserId);
            }
        }

        void Cleanup()
        {
            for (InputEventGroup& inputEventGroup : m_inputEventGroups)
            {
                inputEventGroup.Cleanup();
            }
        }

        void Swap(InputEventBindings* other)
        {
            m_inputEventGroups.swap(other->m_inputEventGroups);
        }

    protected:
        AZStd::vector<InputEventGroup> m_inputEventGroups;
    };

    class InputEventBindingsAsset
        : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(InputEventBindingsAsset, AZ::SystemAllocator);
        AZ_RTTI(InputEventBindingsAsset, "{25971C7A-26E2-4D08-A146-2EFCC1C36B0C}", AZ::Data::AssetData);
        InputEventBindingsAsset() = default;

        virtual ~InputEventBindingsAsset()
        {
            m_bindings.Cleanup();
        }

        InputEventBindings m_bindings;

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<InputEventBindingsAsset>()
                    ->Attribute(AZ::Edit::Attributes::EnableForAssetEditor, true)
                    ->Field("Bindings", &InputEventBindingsAsset::m_bindings)
                ;

                AZ::EditContext* edit = serialize->GetEditContext();
                if (edit)
                {
                    edit->Class<InputEventBindingsAsset>("Input to Event Bindings Asset", "")
                        ->DataElement(0, &InputEventBindingsAsset::m_bindings, "Bindings", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC_CE("PropertyVisibility_ShowChildrenOnly"))
                    ;
                }
            }
        }

    private:
        InputEventBindingsAsset(const InputEventBindingsAsset&) = delete;
    };
} // namespace Input
