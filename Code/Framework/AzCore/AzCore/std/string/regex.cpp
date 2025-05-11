/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/regex.h>

namespace AZStd
{
    template struct AZCORE_API_EXPORT ClassNames<char>;
    template struct AZCORE_API_EXPORT ClassNames<wchar_t>;
    template class AZCORE_API_EXPORT RegexTraits<char>;
    template class AZCORE_API_EXPORT RegexTraits<wchar_t>;
} // namespace AZStd
