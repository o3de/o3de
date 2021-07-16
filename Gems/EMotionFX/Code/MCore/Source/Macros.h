/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace MCore
{
    /**
     * Forward declare an MCore class.
     */
#define MCORE_FORWARD_DECLARE(className) \
    namespace MCore { class className; }

}   // namespace MCore
