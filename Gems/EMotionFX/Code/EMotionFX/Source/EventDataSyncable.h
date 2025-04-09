/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionFX/Source/EventData.h>

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Memory/Memory.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    /** @brief A description of Event parameters to be used to synchronize
     * blended motions
     *
     * This class extends the functionality of the base EventData class to
     * enable events that drive motion synchronization behavior. The
     * synchronization code compares the result of HashForSyncing between two
     * different Motion's Sync tracks, finding events that are equal based on
     * their hash value.
     *
     * @section mirroring Mirroring
     * EMotionFX supports mirroring motions programatically, and when a motion
     * is being mirrored, the sync events also needs to be mirrored. For this
     * purpose, the HashForSyncing method accepts an @p isMirror parameter.
     *
     * For example, if an EventDataSyncable subclass is made to support a horse
     * walk, it could be done using an integer field for the foot number:
     * 0=>left rear, 1=>right rear, 2=>left front, 3=>right front. In this
     * case, HashForSyncing could be implemented like this:
     *
     * @code
     * size_t HashForSyncing(bool isMirror) const
     * {
     *     if (!isMirror)
     *     {
     *         return m_footIndex;
     *     }
     *     // Translate left foot (an even foot index) to right foot, and vice
     *     // versa
     *     return (m_footIndex % 2 == 0) ? m_footIndex + 1 : m_footIndex - 1;
     * }
     * @endcode
     *
     * The default implementation for HashForSyncing returns the hash of the
     * typeid of the type, and ignores the isMirror parameter.
     */
    class EventDataSyncable : public EventData
    {
    public:
        AZ_RTTI(EventDataSyncable, "{18A0050C-D05A-424C-B645-8E3B31120CBA}", EventData);

        EventDataSyncable();

        static void Reflect(AZ::ReflectContext* context);

        virtual size_t HashForSyncing(bool isMirror) const;

    protected:
        EventDataSyncable(const size_t hash);

        mutable size_t m_hash;
    };
} // end namespace EMotionFX
