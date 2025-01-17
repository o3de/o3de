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

    class CCommentTrack : public TAnimTrack<ICommentKey>
    {
    public:
        AZ_CLASS_ALLOCATOR(CCommentTrack, AZ::SystemAllocator);
        AZ_RTTI(CCommentTrack, "{A28FE42D-5B42-4E47-9813-4290D275D5A9}", IAnimTrack);

        CCommentTrack();

        //-----------------------------------------------------------------------------
        //! IAnimTrack Method Overriding.
        //-----------------------------------------------------------------------------
        void GetKeyInfo(int key, const char*& description, float& duration) override;
        void SerializeKey(ICommentKey& key, XmlNodeRef& keyNode, bool bLoading) override;

        void ValidateKeyOrder()
        {
            CheckValid();
        }

        static void Reflect(AZ::ReflectContext* context);
    };

} // namespace Maestro
