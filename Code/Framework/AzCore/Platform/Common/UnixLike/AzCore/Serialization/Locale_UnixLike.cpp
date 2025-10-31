/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Locale_Platform.h>
#include <langinfo.h>

namespace AZ::Locale
{
    void ScopedSerializationLocale_Platform::Activate()
    {
        if (m_isActive)
        {
            Deactivate();
        }

        // The actual cost to create a new locale is extremely low on linux/unix type systems, and doing complex things to 
        // try to avoid doing so is probably not worth the time it costs to actually do so. If this ever shows up in a profiler,
        // then it might be better to see if there's a way to avoid calling this object at all or call it fewer times, 
        // rather than to try to optimize the innards of this actual call.
        m_createdLocale = newlocale(LC_ALL_MASK, "C", (locale_t)0);
        uselocale(m_createdLocale);
        m_isActive = true;
    }

    void ScopedSerializationLocale_Platform::Deactivate()
    {
        if (m_isActive)
        {
            uselocale(LC_GLOBAL_LOCALE);
            freelocale(m_createdLocale);
            m_isActive = false;
        }
    }
} // namespace AZ::Locale
