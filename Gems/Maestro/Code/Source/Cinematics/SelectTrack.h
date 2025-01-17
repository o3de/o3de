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

    /** Select track. Used to select Cameras on a Director's Camera Track
     */
    class CSelectTrack : public TAnimTrack<ISelectKey>
    {
    public:
        AZ_CLASS_ALLOCATOR(CSelectTrack, AZ::SystemAllocator);
        AZ_RTTI(CSelectTrack, "{D05D53BF-86D1-4D38-A3C6-4EFC09C16431}", IAnimTrack);

        AnimValueType GetValueType() override;

        void GetKeyInfo(int key, const char*& description, float& duration) override;
        void SerializeKey(ISelectKey& key, XmlNodeRef& keyNode, bool bLoading) override;

        static void Reflect(AZ::ReflectContext* context);
    };

} // namespace Maestro
