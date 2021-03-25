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

#include <Framework/AWSApiClientJobConfig.h>

namespace AWSCore
{
    // Currently request jobs don't need any configuration beyond what
    // is needed for a client job. If there is ever a need these types
    // can be defined.

    template<class RequestTraits>
    using IAwsApiRequestJobConfig = IAwsApiClientJobConfig<typename RequestTraits::ClientType>;

    template<class RequestTraits>
    using AwsApiRequestJobConfig = AwsApiClientJobConfig<typename RequestTraits::ClientType>;

} // namespace AWSCore

