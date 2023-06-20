/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <iosfwd>

namespace AzFramework
{
    struct ScreenPoint;
    struct ScreenVector;
    struct ScreenSize;
    class InputChannelId;

    void PrintTo(const ScreenPoint& screenPoint, std::ostream* os);
    void PrintTo(const ScreenVector& screenVector, std::ostream* os);
    void PrintTo(const ScreenSize& screenSize, std::ostream* os);
    void PrintTo(const InputChannelId& inputChannelId, std::ostream* os);
} // namespace AzFramework
