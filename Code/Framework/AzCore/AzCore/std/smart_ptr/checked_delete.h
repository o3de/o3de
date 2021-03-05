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
