/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <jni.h>


namespace AZ { namespace Android { namespace JNI { namespace Internal
{
    //! Converts a jstring to a string type
    //! \param stringValue A local or global reference to a jstring object
    //! \return A copy of the converted string
    template<typename StringType>
    StringType ConvertJstringToStringImpl(jstring stringValue);

    //! Converts a string to a jstring
    //! \param stringValue The native string value to be converted
    //! \return A global reference to the converted jstring.  The caller is responsible for
    //!         deleting it when no longer needed
    template<typename StringType>
    jstring ConvertStringToJstringImpl(const StringType& stringValue);

} // namespace Internal
} // namespace JNI
} // namespace Android
} // namespace AZ

#include <AzCore/Android/JNI/Internal/JStringUtils_impl.h>
