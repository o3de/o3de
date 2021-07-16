/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionFX/Source/EventDataSyncable.h>

#include <AzCore/std/string/string.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    class EMFX_API TwoStringEventData
        : public EventDataSyncable
    {
    public:
        AZ_RTTI(TwoStringEventData, "{A437CD65-4012-47DE-BC60-4F9EC2A9ACEE}", EventDataSyncable);
        AZ_CLASS_ALLOCATOR_DECL

        TwoStringEventData()
            : EventDataSyncable(0)
            , m_mirrorHash(0)
        {
        }

        TwoStringEventData(const AZStd::string& subject, const AZStd::string& parameters = "", const AZStd::string& mirrorSubject = "")
            : EventDataSyncable(AZStd::hash<AZStd::string>()(subject))
            , m_subject(subject)
            , m_parameters(parameters)
            , m_mirrorSubject(mirrorSubject)
            , m_mirrorHash(AZStd::hash<AZStd::string>()(mirrorSubject))
        {
        }

        static void Reflect(AZ::ReflectContext* context);

        const AZStd::string& GetSubject() const;

        const AZStd::string& GetParameters() const;

        const AZStd::string& GetMirrorSubject() const;

        bool Equal(const EventData& rhs, bool ignoreEmptyFields = false) const override;

        size_t HashForSyncing(bool isMirror) const override;

    private:
        const AZStd::string m_subject;
        const AZStd::string m_parameters;
        const AZStd::string m_mirrorSubject;
        mutable size_t m_mirrorHash;
    };
} // end namespace EMotionFX
