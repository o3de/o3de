/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#ifdef __APPLE__
#include <xlocale.h>
#else
#include <locale.h>
#endif

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
                locale_t m_createdLocale;
                bool m_isActive = false;
        };
    }
}
