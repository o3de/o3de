/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <QtCore/QString>
#include <MCore/Source/Config.h>

// when we use DLL files, setup the EMFX_API macro
    #if defined(MYSTICQT_DLL_EXPORT)
        #define MYSTICQT_API MCORE_EXPORT
    #else
        #if defined(MYSTICQT_DLL)
            #define MYSTICQT_API MCORE_IMPORT
        #else
            #define MYSTICQT_API
        #endif
    #endif


// memory categories
enum
{
    MEMCATEGORY_MYSTICQT                        = 1491,
    MEMCATEGORY_MYSTICQT_CUSTOMWIDGETS          = 1492
};


// convert from a QString into an AZStd::string
MCORE_INLINE AZStd::string FromQtString(const QString& s)
{
    return {s.toUtf8().data(), static_cast<size_t>(s.size())};
}


// convert from a QString into an AZStd::string
MCORE_INLINE void FromQtString(const QString& s, AZStd::string* result)
{
    *result = AZStd::string{s.toUtf8().data(), static_cast<size_t>(s.size())};
}

inline QString FromStdString(AZStd::string_view s)
{
    return QString::fromUtf8(s.data(), static_cast<int>(s.size()));
}

// forward declare a MysticQt class
#define MYSTICQT_FORWARD_DECLARE(className) \
    namespace MysticQt { class className; }
