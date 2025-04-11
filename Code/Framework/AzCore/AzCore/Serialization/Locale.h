/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Serialization/Locale_Platform.h>

namespace AZ
{
    namespace Locale
    {
        //! This class allows you to activate a culture invariant locale for the current thread, so that numbers and currency
        //! serialize using the same format regardless of the machine locale.
        //! 
        //! Note that there are some cases in the engine where a serialization context object is created for serialize operations,
        //! which is a great place to store the scoped serialization locale object, but often such a context object is reserved/created
        //! on a different thread from where the serializing actually happens.  This is why you can opt to not auto-activate the locale
        //! on construction.
        //!
        //! You can repeatedly call Activate and Deactivate.
        //!
        //! Platform-specific implementations will not allocate on the heap, so this object can be used before the heap is ready.
        class ScopedSerializationLocale final : public ScopedSerializationLocale_Platform
        {
            public:
                ScopedSerializationLocale(bool autoActivate = true);
                ~ScopedSerializationLocale();

                //! Activate the invariant serialization locale for this thread.
                //! This does not happen automatically.  You must call this to enter the invariant serialization locale.
                void Activate();

                //! Deactivate the invariant serialization locale for this thread.
                void Deactivate();
        };
    } // namespace Locale
} // namespace AZ
