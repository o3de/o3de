/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <IMovieSystem.h>
#include "AnimTrack.h"

namespace Maestro
{

    /** CCharacterTrack contains entity keys, when time reach event key, it fires script event or start animation etc...
     */
    class CCharacterTrack : public TAnimTrack<ICharacterKey>
    {
    public:
        AZ_CLASS_ALLOCATOR(CCharacterTrack, AZ::SystemAllocator);
        AZ_RTTI(CCharacterTrack, "{3F701860-78BC-451A-B1DD-90F75DB9A7A2}", IAnimTrack);

        CCharacterTrack()
            : m_iAnimationLayer(-1)
        {
        }

        //////////////////////////////////////////////////////////////////////////
        // Overrides of IAnimTrack.
        //////////////////////////////////////////////////////////////////////////
        bool Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks) override;

        void GetKeyInfo(int key, const char*& description, float& duration) override;
        void SerializeKey(ICharacterKey& key, XmlNodeRef& keyNode, bool bLoading) override;

        //! Gets the duration of an animation key. If it's a looped animation,
        //! a special consideration is required to compute the actual duration.
        float GetKeyDuration(int key) const;

        int GetAnimationLayerIndex() const override
        {
            return m_iAnimationLayer;
        }

        void SetAnimationLayerIndex(int index) override
        {
            m_iAnimationLayer = index;
        }

        AnimValueType GetValueType() override;

        float GetEndTime() const
        {
            return m_timeRange.end;
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        int m_iAnimationLayer;
    };

} // namespace Maestro
