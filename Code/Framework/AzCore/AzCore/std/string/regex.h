/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/base.h>
#include <AzCore/std/string/regex_impl.h>

namespace AZStd
{
    AZCORE_API_EXTERN template struct AZCORE_API ClassNames<char>;
    AZCORE_API_EXTERN template struct AZCORE_API ClassNames<wchar_t>;
    AZCORE_API_EXTERN template class AZCORE_API RegexTraits<char>;
    AZCORE_API_EXTERN template class AZCORE_API RegexTraits<wchar_t>;
} // namespace AZStd
