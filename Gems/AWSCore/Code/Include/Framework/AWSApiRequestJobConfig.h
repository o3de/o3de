/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

