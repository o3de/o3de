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
#include "CryNetwork_precompiled.h"

#include "GridMateNetSerializeAspectProfiles.h"
#include <GridMate/Serialize/CompressionMarshal.h>

namespace GridMate
{
    namespace NetSerialize
    {
        //-----------------------------------------------------------------------------
        EntityAspectProfiles::EntityAspectProfiles()
            : m_profilesMask(0)
        {

        }
        //-----------------------------------------------------------------------------
        void EntityAspectProfiles::SetAspectProfile(size_t aspectIndex, AspectProfile profile)
        {
            GM_ASSERT_TRACE(aspectIndex < kNumAspectSlots, "Invalid aspect index: %u", aspectIndex);

            m_aspectProfiles[ aspectIndex ] = profile;

            if (profile != kUnsetAspectProfile)
            {
                m_profilesMask |= (1 << aspectIndex);
            }
            else
            {
                m_profilesMask &= ~(1 << aspectIndex);
            }
        }

        //-----------------------------------------------------------------------------
        AspectProfile EntityAspectProfiles::GetAspectProfile(size_t aspectIndex) const
        {
            GM_ASSERT_TRACE(aspectIndex < kNumAspectSlots, "Invalid aspect index: %u", aspectIndex);

            return m_aspectProfiles[ aspectIndex ];
        }

        //-----------------------------------------------------------------------------
        bool EntityAspectProfiles::operator == (const EntityAspectProfiles& other) const
        {
            for (size_t i = 0; i < NetSerialize::kNumAspectSlots; ++i)
            {
                if (m_aspectProfiles[ i ] != other.m_aspectProfiles[ i ])
                {
                    return false;
                }
            }

            return true;
        }

        //-----------------------------------------------------------------------------
        void EntityAspectProfiles::Marshaler::SetChangeDelegate(ChangeDelegate changeDelegate)
        {
            m_changeDelegate = changeDelegate;
        }

        //-----------------------------------------------------------------------------
        void EntityAspectProfiles::Marshaler::Marshal(GridMate::WriteBuffer& wb, const EntityAspectProfiles& s)
        {
            AZ::u32 profilesMask = s.m_profilesMask;
            wb.Write(profilesMask, VlqU32Marshaler());

            unsigned i = 0;
            while (profilesMask)
            {
                if (profilesMask & 1)
                {
                    m_profileMarshaler.Marshal(wb, s.m_aspectProfiles[i]);
                }

                profilesMask >>= 1;
                ++i;
            }
        }

        //-----------------------------------------------------------------------------
        void EntityAspectProfiles::Marshaler::Unmarshal(EntityAspectProfiles& s, GridMate::ReadBuffer& rb)
        {
            AZ::u32 profilesMask = 0;
            if (rb.Read(profilesMask, VlqU32Marshaler()))
            {
                s.m_profilesMask = profilesMask;
                for (size_t i = 0; i < NetSerialize::kNumAspectSlots; ++i, profilesMask >>= 1)
                {
                    const AspectProfile oldValue = s.m_aspectProfiles[i];

                    if (profilesMask & 1)
                    {
                        m_profileMarshaler.Unmarshal(s.m_aspectProfiles[i], rb);
                    }
                    else
                    {
                        s.m_aspectProfiles[i] = kUnsetAspectProfile;
                    }


                    if (oldValue != s.m_aspectProfiles[i] && m_changeDelegate)
                    {
                        m_changeDelegate(i, oldValue, s.m_aspectProfiles[i]);
                    }
                }
            }
        }
    } // namespace NetSerialize
} // namespace GridMate
