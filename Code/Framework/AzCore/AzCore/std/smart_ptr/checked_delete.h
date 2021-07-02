/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_CHECKED_DELETE_H
#define AZSTD_CHECKED_DELETE_H

#include <AzCore/std/base.h>

namespace AZStd
{
    //////////////////////////////////////////////////////////////////////////
    // verify that types are complete for increased safety
    template<class T>
    inline void checked_delete(T* x)
    {
        // intentionally complex - simplification causes regressions
        typedef char type_must_be_complete[ sizeof(T) ? 1 : -1 ];
        (void) sizeof(type_must_be_complete);
        delete x;
    }

    template<class T>
    inline void checked_array_delete(T* x)
    {
        typedef char type_must_be_complete[ sizeof(T) ? 1 : -1 ];
        (void) sizeof(type_must_be_complete);
        delete [] x;
    }

    template<class T>
    struct checked_deleter
    {
        typedef void result_type;
        typedef T* argument_type;

        void operator()(T* x) const
        {
            AZStd::checked_delete(x);
        }
    };

    template<class T>
    struct checked_array_deleter
    {
        typedef void result_type;
        typedef T* argument_type;

        void operator()(T* x) const
        {
            AZStd::checked_array_delete(x);
        }
    };
} // namespace AZStd

#endif  // #ifndef AZSTD_CHECKED_DELETE_H
#pragma once
