/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include "SelectTrack.h"
#include "Maestro/Types/AnimValueType.h"

namespace Maestro
{

    void CSelectTrack::SerializeKey(ISelectKey& key, XmlNodeRef& keyNode, bool bLoading)
    {
        if (bLoading)
        {
            const char* szSelection;
            szSelection = keyNode->getAttr("node");
            key.szSelection = szSelection;
            AZ::u64 id64;
            if (keyNode->getAttr("CameraAzEntityId", id64))
            {
                key.cameraAzEntityId = AZ::EntityId(id64);
            }
            keyNode->getAttr("BlendTime", key.fBlendTime);
        }
        else
        {
            keyNode->setAttr("node", key.szSelection.c_str());
            if (key.cameraAzEntityId.IsValid())
            {
                AZ::u64 id64 = static_cast<AZ::u64>(key.cameraAzEntityId);
                keyNode->setAttr("CameraAzEntityId", id64);
            }
            keyNode->setAttr("BlendTime", key.fBlendTime);
        }
    }

    AnimValueType CSelectTrack::GetValueType()
    {
        return AnimValueType::Select;
    };

    void CSelectTrack::GetKeyInfo(int key, const char*& description, float& duration)
    {
        AZ_Assert(key >= 0 && key < (int)m_keys.size(), "Key index %i is out of range", key);
        CheckValid();
        description = 0;
        duration = m_keys[key].fDuration;
        if (!m_keys[key].szSelection.empty())
        {
            description = m_keys[key].szSelection.c_str();
        }
    }

    static bool SelectTrackVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 3)
        {
            rootElement.AddElement(serializeContext, "BaseClass1", azrtti_typeid<IAnimTrack>());
        }

        return true;
    }

    template<>
    inline void TAnimTrack<ISelectKey>::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TAnimTrack<ISelectKey>, IAnimTrack>()
                ->Version(3, &SelectTrackVersionConverter)
                ->Field("Flags", &TAnimTrack<ISelectKey>::m_flags)
                ->Field("Range", &TAnimTrack<ISelectKey>::m_timeRange)
                ->Field("ParamType", &TAnimTrack<ISelectKey>::m_nParamType)
                ->Field("Keys", &TAnimTrack<ISelectKey>::m_keys)
                ->Field("Id", &TAnimTrack<ISelectKey>::m_id);
        }
    }

    void CSelectTrack::Reflect(AZ::ReflectContext* context)
    {
        TAnimTrack<ISelectKey>::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CSelectTrack, TAnimTrack<ISelectKey>>()->Version(1);
        }
    }

} // namespace Maestro
