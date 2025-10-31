/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ATLUtils.h>

namespace Audio
{
#if !defined(AUDIO_RELEASE)
    bool AudioDebugDrawFilter(const AZStd::string_view objectName, const AZStd::string_view filter)
    {
        bool result = false;
        result = result || filter.empty();
        result = result || (filter.compare("0") == 0);
        result = result || (objectName.find(filter) != AZStd::string_view::npos);
        return result;
    }
#endif // !AUDIO_RELEASE

} // namespace Audio
