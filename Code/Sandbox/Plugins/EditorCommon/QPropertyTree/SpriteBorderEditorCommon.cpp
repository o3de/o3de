//-------------------------------------------------------------------------------
// Copyright (C) Amazon.com, Inc. or its affiliates.
// All Rights Reserved.
//
// Licensed under the terms set out in the LICENSE.HTML file included at the
// root of the distribution; you may not use this file except in compliance
// with the License.
//
// Do not remove or modify this notice or the LICENSE.HTML file.  This file
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// either express or implied. See the License for the specific language
// governing permissions and limitations under the License.
//-------------------------------------------------------------------------------

// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "EditorCommon_precompiled.h"
#include "SpriteBorderEditorCommon.h"

bool IsBorderVertical(SpriteBorder border)
{
    return ( ( border == SpriteBorder::Left ) ||
                ( border == SpriteBorder::Right ) );
}

float GetBorderValueInPixels(ISprite* sprite, SpriteBorder b, float totalSizeInPixels)
{
    // IMPORTANT: We CAN'T replace totalSizeInPixels with
    // sprite->GetTexture()->GetWidth()/GetHeight() because
    // it DOESN'T return the original texture file's size.

    ISprite::Borders sb = sprite->GetBorders();

    float *f = nullptr;

    if( b == SpriteBorder::Top )
    {
        f = &sb.m_top;
    }
    else if( b == SpriteBorder::Bottom )
    {
        f = &sb.m_bottom;
    }
    else if( b == SpriteBorder::Left )
    {
        f = &sb.m_left;
    }
    else if( b == SpriteBorder::Right )
    {
        f = &sb.m_right;
    }
    else
    {
        CRY_ASSERT( 0 );
        f = nullptr;
    }

    return ( *f * totalSizeInPixels );
}

void SetBorderValue(ISprite* sprite, SpriteBorder b, float pixelPosition, float totalSizeInPixels)
{
    // IMPORTANT: We CAN'T replace totalSizeInPixels with
    // sprite->GetTexture()->GetWidth()/GetHeight() because
    // it DOESN'T return the original texture file's size.

    ISprite::Borders sb = sprite->GetBorders();

    float *f = nullptr;

    if( b == SpriteBorder::Top )
    {
        f = &sb.m_top;
    }
    else if( b == SpriteBorder::Bottom )
    {
        f = &sb.m_bottom;
    }
    else if( b == SpriteBorder::Left )
    {
        f = &sb.m_left;
    }
    else if( b == SpriteBorder::Right )
    {
        f = &sb.m_right;
    }
    else
    {
        CRY_ASSERT( 0 );
        f = nullptr;
    }

    *f = ( pixelPosition / totalSizeInPixels );
    sprite->SetBorders( sb );
}

const char* SpriteBorderToString(SpriteBorder b)
{
    if( b == SpriteBorder::Top )
    {
        return "Top";
    }
    else if( b == SpriteBorder::Bottom )
    {
        return "Bottom";
    }
    else if( b == SpriteBorder::Left )
    {
        return "Left";
    }
    else if( b == SpriteBorder::Right )
    {
        return "Right";
    }
    else
    {
        CRY_ASSERT( 0 );
        return "UNKNOWN";
    }
}
