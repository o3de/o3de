/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/stringbuffer.h>

namespace TestImpact
{
    struct ChangeList;

    void SerializeChangeList(const ChangeList& changeList, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer);
}
