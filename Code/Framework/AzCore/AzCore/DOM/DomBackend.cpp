/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomBackend.h>

namespace AZ::Dom
{
    Visitor::Result Backend::ReadFromBufferInPlace(char* buffer, Visitor& visitor)
    {
        return ReadFromBuffer(buffer, strlen(buffer), AZ::Dom::Lifetime::Persistent, visitor);
    }
} // namespace AZ::Dom
