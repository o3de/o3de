/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <OpenParticleSystem/ParticlePropertyUtils.h>

namespace OpenParticle
{
    AZStd::string_view RemoveNameSpace(const AZStd::string_view& str)
    {
        auto pos = str.find_last_of(":");
        if (pos == AZStd::string_view::npos || pos == 0)
        {
            return str;
        }
        return str.substr(pos + 1);
    }
} // namespace OpenParticle
