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

#include <functional>

namespace AZStd
{
    namespace placeholders
    {
        using std::placeholders::_1;
        using std::placeholders::_2;
        using std::placeholders::_3;
        using std::placeholders::_4;
        using std::placeholders::_5;
        using std::placeholders::_6;
        using std::placeholders::_7;
        using std::placeholders::_8;
        using std::placeholders::_9;
        using std::placeholders::_10;
    }

    using std::bind;

    using std::is_bind_expression;
    template<class T>
    constexpr bool is_bind_expression_v = std::is_bind_expression<T>::value;

    using std::is_placeholder;
    template<class T>
    constexpr size_t is_placeholder_v = is_placeholder<T>::value;
}