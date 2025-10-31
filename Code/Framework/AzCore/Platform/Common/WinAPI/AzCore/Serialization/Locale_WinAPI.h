/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/fixed_string.h>

// must be able to store all possible locale strings, which are not usually very long, 
// usually something like "English_United States.UTF8" which is one of the longest

// Note that this is also the max defined in Windows.h but we'd like to avoid using windows.h
// in headers if we can possibly avoid it.
#define AZ_LOCALE_NAME_MAX_LENGTH 100 

namespace AZ
{
    namespace Locale
    {
        class ScopedSerializationLocale_Platform
        {
            public:
                void Activate();
                void Deactivate();
            private:
                AZStd::fixed_string<AZ_LOCALE_NAME_MAX_LENGTH> m_previousLocale;
                int m_previousThreadLocalSetting = 0;
                bool m_isActive = false;
        };
    }
}
