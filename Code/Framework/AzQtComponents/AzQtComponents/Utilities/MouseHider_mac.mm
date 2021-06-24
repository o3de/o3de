/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MouseHider.h"

#include <CoreGraphics/CoreGraphics.h>

namespace AzQtComponents
{

    MouseHider::MouseHider(QObject* parent /* = nullptr */)
        : QObject(parent)
    {
        CGDisplayHideCursor(kCGDirectMainDisplay);
    }

    MouseHider::~MouseHider()
    {
        CGDisplayShowCursor(kCGDirectMainDisplay);
    }

} // namespace AzQtComponents
