/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>

#include <AzQtComponents/Utilities/MouseHider.h>

namespace AzQtComponents
{

    MouseHider::MouseHider(QObject* parent /* = nullptr */)
        : QObject(parent)
    {
        ShowCursor(false);
    }

    MouseHider::~MouseHider()
    {
        ShowCursor(true);
    }

} // namespace AzQtComponents
