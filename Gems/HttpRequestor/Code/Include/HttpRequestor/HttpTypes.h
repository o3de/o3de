/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// The AWS Native SDK AWSAllocator triggers a warning due to accessing members of std::allocator directly.
// AWSAllocator.h(70): warning C4996: 'std::allocator<T>::pointer': warning STL4010: Various members of std::allocator are deprecated in C++17.
// Use std::allocator_traits instead of accessing these members directly.
// You can define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received this warning.
AZ_PUSH_DISABLE_WARNING(4251 4996, "-Wunknown-warning-option")
#include <aws/core/http/HttpTypes.h>
#include <aws/core/http/HttpResponse.h>
#include <aws/core/utils/json/JsonSerializer.h>
AZ_POP_DISABLE_WARNING
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/functional.h>

namespace HttpRequestor
{
    //
    // the call back function for http requests.
    //
    using Callback = AZStd::function<void(const Aws::Utils::Json::JsonView&, Aws::Http::HttpResponseCode)>;

    
    //
    // the call back function for any http text requests.
    //
    using TextCallback = AZStd::function<void(const AZStd::string&, Aws::Http::HttpResponseCode)>;


    //
    // a map of REST headers.
    //
    using Headers = AZStd::map<AZStd::string, AZStd::string>;
}
