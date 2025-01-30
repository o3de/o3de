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

    /** Look at target track, keys represent new lookat targets for entity.
     */
    class CLookAtTrack : public TAnimTrack<ILookAtKey>
    {
    public:
        AZ_CLASS_ALLOCATOR(CLookAtTrack, AZ::SystemAllocator);
        AZ_RTTI(CLookAtTrack, "{30A5C53C-F158-4CCE-A7A0-1A902D13B91C}", IAnimTrack);

        CLookAtTrack()
            : m_iAnimationLayer(-1)
        {
        }

        bool Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks) override;

        void GetKeyInfo(int key, const char*& description, float& duration) override;
        void SerializeKey(ILookAtKey& key, XmlNodeRef& keyNode, bool bLoading) override;

        int GetAnimationLayerIndex() const override
        {
            return m_iAnimationLayer;
        }
        void SetAnimationLayerIndex(int index) override
        {
            m_iAnimationLayer = index;
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        int m_iAnimationLayer;
    };

} // namespace Maestro
