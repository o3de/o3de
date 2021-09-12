/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Common types used across the LyShine UI layout cell system
namespace LyShine
{
    const float UiLayoutCellUnspecifiedSize = -1.0f;

    inline bool IsUiLayoutCellSizeSpecified(float size)
    {
        return size >= 0.0f;
    }
};
