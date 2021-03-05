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
    return s.toUtf8().data();
}


// convert from a QString into an AZStd::string
MCORE_INLINE void FromQtString(const QString& s, AZStd::string* result)
{
    *result = s.toUtf8().data();
}

// forward declare a MysticQt class
#define MYSTICQT_FORWARD_DECLARE(className) \
    namespace MysticQt { class className; }
