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

#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RHI.Reflect/AttachmentLoadStoreAction.h>

namespace AZ
{
    namespace RHI
    {
        void AttachmentLoadStoreAction::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<AttachmentLoadStoreAction>()
                    ->Version(1)
                    ->Field("ClearValue", &AttachmentLoadStoreAction::m_clearValue)
                    ->Field("LoadAction", &AttachmentLoadStoreAction::m_loadAction)
                    ->Field("StoreAction", &AttachmentLoadStoreAction::m_storeAction)
                    ->Field("LoadActionStencil", &AttachmentLoadStoreAction::m_loadActionStencil)
                    ->Field("StoreActionStencil", &AttachmentLoadStoreAction::m_storeActionStencil)
                    ;
            }
        }

        AttachmentLoadStoreAction::AttachmentLoadStoreAction(
            const ClearValue& clearValue,
            AttachmentLoadAction loadAction,
            AttachmentStoreAction storeAction,
            AttachmentLoadAction loadActionStencil,
            AttachmentStoreAction storeActionStencil)
            : m_clearValue{ clearValue }
            , m_loadAction{ loadAction }
            , m_storeAction{ storeAction }
            , m_loadActionStencil{ loadActionStencil }
            , m_storeActionStencil{ storeActionStencil }
        {}
    
        bool AttachmentLoadStoreAction::operator==(const AttachmentLoadStoreAction& other) const
        {
            return
                m_clearValue == other.m_clearValue &&
                m_loadAction == other.m_loadAction &&
                m_storeAction == other.m_storeAction &&
                m_loadActionStencil == other.m_loadActionStencil &&
                m_storeActionStencil == other.m_storeActionStencil;
        }
    }
}
