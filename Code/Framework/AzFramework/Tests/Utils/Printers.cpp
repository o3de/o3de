/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Printers.h"

#include <AzFramework/Input/Channels/InputChannelId.h>
#include <AzFramework/Viewport/ScreenGeometry.h>

#include <ostream>
#include <string>

namespace AzFramework
{
    void PrintTo(const ScreenPoint& screenPoint, std::ostream* os)
    {
        *os << "(x: " << screenPoint.m_x << ", y: " << screenPoint.m_y << ")";
    }

    void PrintTo(const ScreenVector& screenVector, std::ostream* os)
    {
        *os << "(x: " << screenVector.m_x << ", y: " << screenVector.m_y << ")";
    }

    void PrintTo(const ScreenSize& screenSize, std::ostream* os)
    {
        *os << "(width: " << screenSize.m_width << ", height: " << screenSize.m_height << ")";
    }

    void PrintTo(const InputChannelId& inputChannelId, std::ostream* os)
    {
        *os << "(input channel id name: " << inputChannelId.GetName() << ")";
    }
} // namespace AzFramework
