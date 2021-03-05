/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

#include "ChatPlay_precompiled.h"
#include <IGem.h>

#include "ChatPlaySystemComponent.h"

#include "ChatPlay/ChatPlayCVars.h"
#include "Broadcast/BroadcastCVars.h"
#include <ChatPlay_Traits_Platform.h>
#if AZ_TRAIT_CHATPLAY_JOIN_AND_BROADCAST
#include "JoinIn/JoinInCVars.h"
#endif // AZ_TRAIT_CHATPLAY_JOIN_AND_BROADCAST


namespace ChatPlay
{

    class ChatPlayModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(ChatPlayModule, "{E1788926-A994-4D68-A118-B9548ABA2929}", CryHooksModule);

        ChatPlayModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                ChatPlaySystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<ChatPlaySystemComponent>(),
            };
        }

        void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams)
        {
            CryHooksModule::OnCrySystemInitialized(system, systemInitParams);
            ChatPlayCVars::GetInstance()->RegisterCVars();
#if AZ_TRAIT_CHATPLAY_JOIN_AND_BROADCAST
            JoinInCVars::GetInstance()->RegisterCVars();
            BroadcastCVars::GetInstance()->RegisterCVars();
#endif // AZ_TRAIT_CHATPLAY_JOIN_AND_BROADCAST
        }

        void OnSystemEvent(ESystemEvent systemEvent, UINT_PTR wparam, UINT_PTR lparam)
        {
            (void)wparam;
            (void)lparam;

            switch (systemEvent)
            {
            case ESYSTEM_EVENT_FULL_SHUTDOWN:
            case ESYSTEM_EVENT_FAST_SHUTDOWN:
                ChatPlayCVars::GetInstance()->UnregisterCVars();
#if AZ_TRAIT_CHATPLAY_JOIN_AND_BROADCAST
                JoinInCVars::GetInstance()->UnregisterCVars();
                BroadcastCVars::GetInstance()->UnregisterCVars();
#endif // AZ_TRAIT_CHATPLAY_JOIN_AND_BROADCAST
                break;
            default:
                break;
            }
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_ChatPlay, ChatPlay::ChatPlayModule)
