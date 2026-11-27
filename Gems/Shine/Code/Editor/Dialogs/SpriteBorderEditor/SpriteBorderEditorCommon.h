/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <type_traits>

class ISprite;

#define UICANVASEDITOR_SPRITEBORDEREDITOR_SCENE_WIDTH   (256)
#define UICANVASEDITOR_SPRITEBORDEREDITOR_SCENE_HEIGHT  (256)

enum class SpriteBorder
{
    Top,
    Bottom,
    Left,
    Right
};

// This allows iterating over an enum class.
#define ADD_ENUM_CLASS_ITERATION_OPERATORS(CLASS_NAME, FIRST_VALUE, LAST_VALUE)                                             \
                                                                                                                            \
    inline CLASS_NAME operator++(CLASS_NAME & m){ return m = (CLASS_NAME)(std::underlying_type<CLASS_NAME>::type(m) + 1); } \
    inline CLASS_NAME operator*(CLASS_NAME m){ return m; }                                                                  \
    inline CLASS_NAME begin([[maybe_unused]] CLASS_NAME m){ return FIRST_VALUE; }                                                            \
    inline CLASS_NAME end([[maybe_unused]] CLASS_NAME m){ return (CLASS_NAME)(std::underlying_type<CLASS_NAME>::type(LAST_VALUE) + 1); }

ADD_ENUM_CLASS_ITERATION_OPERATORS(SpriteBorder,
    SpriteBorder::Top,
    SpriteBorder::Right);

bool IsBorderVertical(SpriteBorder border);
float GetBorderValueInPixels(ISprite* sprite, SpriteBorder b, float totalSizeInPixels, AZ::u32 cellIndex = 0);
void SetBorderValue(ISprite* sprite, SpriteBorder b, float pixelPosition, float totalSizeInPixels, AZ::u32 cellIndex = 0);
const char* SpriteBorderToString(SpriteBorder b);
