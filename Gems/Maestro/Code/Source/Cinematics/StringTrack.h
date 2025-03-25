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
    /** String track, a key on this track has a string value.
     */
    class CStringTrack : public TAnimTrack<IStringKey>
    {
    public:
        AZ_CLASS_ALLOCATOR(CStringTrack, AZ::SystemAllocator);
        AZ_RTTI(CStringTrack, "{FEF911E3-30A4-4D22-BFFB-8EF4FB7CD4DB}", IAnimTrack);

        // IAnimTrack -> TAnimTrack<IStringKey> overrides
        AnimValueType GetValueType() override;

        int CreateKey(float time) override; // override to use default value
        void GetValue(float time, AZStd::string& value) override;
        void SetValue(float time, const AZStd::string& value, bool bDefault = false) override;
        void SerializeKey([[maybe_unused]] IStringKey& key, [[maybe_unused]] XmlNodeRef& keyNode, [[maybe_unused]] bool bLoading) override {}
        void GetKeyInfo(int key, const char*& description, float& duration) override;
        // ~ IAnimTrack overrides

        void SetDefaultValue(const AZStd::string& defaultValue) { m_defaultValue = defaultValue; }
        const AZStd::string& GetDefaultValue() const { return m_defaultValue; }

        static void Reflect(AZ::ReflectContext* context);

    private:
        void SetKeyAtTime(float time, IKey* key);

        AZStd::string m_defaultValue;
    };

} // namespace Maestro

