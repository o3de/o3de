/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <locale.h>
#include <AzCore/Serialization/Locale_Platform.h>

namespace AZ::Locale
{
    void ScopedSerializationLocale_Platform::Activate()
    {
        if (m_isActive)
        {
            Deactivate();
        }

        // Save the previous "are threads allowed to have per-thread locale?" setting:
        m_previousThreadLocalSetting = _configthreadlocale(_ENABLE_PER_THREAD_LOCALE);

        // Save the current locale. The return value of setlocale depends on the above call happening first.
        m_previousLocale = setlocale(LC_ALL, nullptr);

        // shortcut here, if we won't have any effect, then don't make any further system calls.
        if ( (m_previousLocale == "C")&&(m_previousThreadLocalSetting == _ENABLE_PER_THREAD_LOCALE) )
        {
            return;
        }

        // if we get here, we made some sort of change, and must set the locale and also remember to reset it back.
        setlocale(LC_ALL, "C");
        m_isActive = true;
    }

    void ScopedSerializationLocale_Platform::Deactivate()
    {
        if (m_isActive)
        {
            setlocale(LC_ALL, m_previousLocale.c_str());
            _configthreadlocale(m_previousThreadLocalSetting);
            m_isActive = false;
        }
    }

} // namespace AZ::Locale
