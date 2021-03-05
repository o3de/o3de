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
// Based on boost 1.39.0
#ifndef AZSTD_FUNCTION_FWD_H
#define AZSTD_FUNCTION_FWD_H

#include <AzCore/std/base.h>

namespace AZStd
{
    class bad_function_call;

    // Preferred syntax
    template<typename Signature>
    class function;

    template<typename Signature>
    inline void swap(function<Signature>& f1, function<Signature>& f2)    { f1.swap(f2); }

    // Portable syntax
    //template<typename R> class function0;
    //template<typename R, typename T1> class function1;
    //template<typename R, typename T1, typename T2> class function2;
    //template<typename R, typename T1, typename T2, typename T3> class function3;
    //template<typename R, typename T1, typename T2, typename T3, typename T4> class function4;
    //template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5> class function5;
    //template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6> class function6;
    //template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7> class function7;
    //template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8> class function8;
    //template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9> class function9;
    //template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10> class function10;
}

#endif // AZSTD_FUNCTION_FWD_H
#pragma once
