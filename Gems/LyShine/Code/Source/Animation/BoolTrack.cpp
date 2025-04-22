/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "BoolTrack.h"
#include <AzCore/Serialization/SerializeContext.h>
#include "UiAnimSerialize.h"

//////////////////////////////////////////////////////////////////////////
UiBoolTrack::UiBoolTrack()
    : m_bDefaultValue(true)
{
}

//////////////////////////////////////////////////////////////////////////
void UiBoolTrack::GetKeyInfo([[maybe_unused]] int index, const char*& description, float& duration)
{
    description = 0;
    duration = 0;
}

//////////////////////////////////////////////////////////////////////////
void UiBoolTrack::GetValue(float time, bool& value)
{
    value = m_bDefaultValue;

    CheckValid();

    int nkeys = static_cast<int>(m_keys.size());
    if (nkeys < 1)
    {
        return;
    }

    int key = 0;
    while ((key < nkeys) && (time >= m_keys[key].time))
    {
        key++;
    }

    if (m_bDefaultValue)
    {
        value = !(key & 1); // True if even key.
    }
    else
    {
        value = (key & 1);  // False if even key.
    }
}

//////////////////////////////////////////////////////////////////////////
void UiBoolTrack::SetValue([[maybe_unused]] float time, [[maybe_unused]] const bool& value, [[maybe_unused]] bool bDefault)
{
    Invalidate();
}

//////////////////////////////////////////////////////////////////////////
void UiBoolTrack::SetDefaultValue(const bool bDefaultValue)
{
    m_bDefaultValue = bDefaultValue;
}

//////////////////////////////////////////////////////////////////////////
template<>
inline void TUiAnimTrack<IBoolKey>::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->ClassDeprecate("TUiAnimTrack_IBoolKey", AZ::Uuid("{7C2942C1-0ACE-404E-BF2B-E095A1B69A5B}"),
        [](AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement)
    {
        AZStd::vector<AZ::SerializeContext::DataElementNode> childNodeElements;
        for (int index = 0; index < rootElement.GetNumSubElements(); ++index)
        {
            childNodeElements.push_back(rootElement.GetSubElement(index));
        }
        // Convert the rootElement now, the existing child DataElmentNodes are now removed
        rootElement.Convert<TUiAnimTrack<IBoolKey>>(context);
        for (AZ::SerializeContext::DataElementNode& childNodeElement : childNodeElements)
        {
            rootElement.AddElement(AZStd::move(childNodeElement));
        }
        return true;
    });

    serializeContext->Class<TUiAnimTrack<IBoolKey> >()
        ->Version(1)
        ->Field("Flags", &TUiAnimTrack<IBoolKey>::m_flags)
        ->Field("Range", &TUiAnimTrack<IBoolKey>::m_timeRange)
        ->Field("ParamType", &TUiAnimTrack<IBoolKey>::m_nParamType)
        ->Field("ParamData", &TUiAnimTrack<IBoolKey>::m_componentParamData)
        ->Field("Keys", &TUiAnimTrack<IBoolKey>::m_keys);
}

//////////////////////////////////////////////////////////////////////////
void UiBoolTrack::Reflect(AZ::SerializeContext* serializeContext)
{
    TUiAnimTrack<IBoolKey>::Reflect(serializeContext);

    serializeContext->Class<UiBoolTrack, TUiAnimTrack<IBoolKey> >()
        ->Version(1)
        ;
}
