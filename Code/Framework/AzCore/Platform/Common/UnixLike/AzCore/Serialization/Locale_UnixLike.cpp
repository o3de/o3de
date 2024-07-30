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
    constexpr const locale_t NULL_LOCALE = ((locale_t)0);

    void ScopedSerializationLocale_Platform::Activate()
    {
        if (m_isActive)
        {
            Deactivate();
        }

        locale_t currentLocale = uselocale(NULL_LOCALE); 
        // passing nullptr to uselocale will either
        // * return nullptr in an edge case
        // * return a locale_t object
        // * return LC_GLOBAL_LOCALE - meaning, there is no thread override and the thread is using the global locale.
        // If we are using the global locale, or were unable to fetch the thread locale for some other reason, we need to
        // apply our own locale to this thread.
        // If we are using a per-thread-locale override already, we can check to see if its the "C" locale, and if it is, 
        // we can skip setting our own thread-locale for "C" since it is already set, and already thread-local.
        if ((currentLocale != NULL_LOCALE) && (currentLocale != LC_GLOBAL_LOCALE)) 
        {
            // we have a locale, and its not the null locale nor the global one, check it
            const char* currentLocaleName = nl_langinfo_l(_NL_IDENTIFICATION_ABBREVIATION, currentLocale);
            if ( (currentLocaleName[0] == 'C') && (currentLocaleName[1] == '\0') )
            {
                // The locale is compatible with the "C" Locale when it comes to number formatting, so we don't need to do anything.
                return;
            }
        }

        // if we get here, we made some sort of change, and must set the locale and also remember to reset it back.
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
