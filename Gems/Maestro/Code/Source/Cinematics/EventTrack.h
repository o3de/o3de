/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <IMovieSystem.h>
#include "AnimKey.h"
#include "AnimTrack.h"

namespace Maestro
{

    /** EntityTrack contains entity keys, when time reach event key, it fires script event or start animation etc...
     */
    class CEventTrack : public TAnimTrack<IEventKey>
    {
    public:
        AZ_CLASS_ALLOCATOR(CEventTrack, AZ::SystemAllocator);
        AZ_RTTI(CEventTrack, "{CA9D004F-7003-46E7-AB85-7D3846E8C10B}", IAnimTrack);

        CEventTrack();
        explicit CEventTrack(IAnimStringTable* pStrings);

        //////////////////////////////////////////////////////////////////////////
        // Overrides of IAnimTrack.
        //////////////////////////////////////////////////////////////////////////
        void GetKeyInfo(int key, const char*& description, float& duration) override;
        void SerializeKey(IEventKey& key, XmlNodeRef& keyNode, bool bLoading) override;
        void SetKey(int index, IKey* key) override;
        void InitPostLoad(IAnimSequence* sequence) override;

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::intrusive_ptr<IAnimStringTable> m_pStrings;
    };

} // namespace Maestro
