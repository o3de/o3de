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
