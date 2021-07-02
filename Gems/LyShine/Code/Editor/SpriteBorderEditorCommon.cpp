/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiCanvasEditor_precompiled.h"
#include "SpriteBorderEditorCommon.h"

bool IsBorderVertical(SpriteBorder border)
{
    return ((border == SpriteBorder::Left) ||
            (border == SpriteBorder::Right));
}

float GetBorderValueInPixels(ISprite* sprite, SpriteBorder b, float totalSizeInPixels, AZ::u32 cellIndex)
{
    // IMPORTANT: We CAN'T replace totalSizeInPixels with
    // sprite->GetTexture()->GetWidth()/GetHeight() because
    // it DOESN'T return the original texture file's size.

    ISprite::Borders sb = sprite->GetCellUvBorders(cellIndex);

    float* f = nullptr;

    if (b == SpriteBorder::Top)
    {
        f = &sb.m_top;
    }
    else if (b == SpriteBorder::Bottom)
    {
        f = &sb.m_bottom;
    }
    else if (b == SpriteBorder::Left)
    {
        f = &sb.m_left;
    }
    else if (b == SpriteBorder::Right)
    {
        f = &sb.m_right;
    }
    else
    {
        AZ_Assert(false, "Invalid border enum value");
        f = nullptr;
    }

    return (*f * totalSizeInPixels);
}

void SetBorderValue(ISprite* sprite, SpriteBorder b, float pixelPosition, float totalSizeInPixels, AZ::u32 cellIndex)
{
    // IMPORTANT: We CAN'T replace totalSizeInPixels with
    // sprite->GetTexture()->GetWidth()/GetHeight() because
    // it DOESN'T return the original texture file's size.

    ISprite::Borders sb = sprite->GetCellUvBorders(cellIndex);

    float* f = nullptr;

    if (b == SpriteBorder::Top)
    {
        f = &sb.m_top;
    }
    else if (b == SpriteBorder::Bottom)
    {
        f = &sb.m_bottom;
    }
    else if (b == SpriteBorder::Left)
    {
        f = &sb.m_left;
    }
    else if (b == SpriteBorder::Right)
    {
        f = &sb.m_right;
    }
    else
    {
        AZ_Assert(false, "Invalid border enum value");
        f = nullptr;
    }

    *f = AZ::GetMin<float>(pixelPosition / totalSizeInPixels, 1.0f);
    sprite->SetCellBorders(cellIndex, sb);
}

const char* SpriteBorderToString(SpriteBorder b)
{
    if (b == SpriteBorder::Top)
    {
        return "Top";
    }
    else if (b == SpriteBorder::Bottom)
    {
        return "Bottom";
    }
    else if (b == SpriteBorder::Left)
    {
        return "Left";
    }
    else if (b == SpriteBorder::Right)
    {
        return "Right";
    }
    else
    {
        AZ_Assert(false, "Invalid border enum value");
        return "UNKNOWN";
    }
}
