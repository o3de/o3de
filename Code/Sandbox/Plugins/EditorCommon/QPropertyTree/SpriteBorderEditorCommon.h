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

#pragma once
#ifndef CRYINCLUDE_EDITORCOMMON_SPRITEBORDEREDITORCOMMON_H
#define CRYINCLUDE_EDITORCOMMON_SPRITEBORDEREDITORCOMMON_H

#include <vector> // required to be included before platform.h
#include <platform.h>
#include <ISystem.h>
#include <IRenderer.h>
#include <IEditor.h>
#include <LyShine/ILyShine.h>
#include <LyShine/ISprite.h>

#include <QDialog>
#include <QDoubleValidator>
#include <QGraphicsRectItem>
#include <QGraphicsView>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QMessageBox>

class SlicerEdit;
class SlicerManipulator;
class SlicerView;
class SpriteBorderEditor;

enum class SpriteBorder
{
    Top,
    Bottom,
    Left,
    Right
};

// This allows iterating over an enum class.
#define ADD_ENUM_CLASS_ITERATION_OPERATORS( CLASS_NAME, FIRST_VALUE, LAST_VALUE ) \
    \
    inline CLASS_NAME operator++(CLASS_NAME &m){ return m = (CLASS_NAME)(std::underlying_type<CLASS_NAME>::type(m) + 1); } \
    inline CLASS_NAME operator*(CLASS_NAME m){ return m; } \
    inline CLASS_NAME begin([[maybe_unused]] CLASS_NAME m){ return FIRST_VALUE; } \
    inline CLASS_NAME end([[maybe_unused]] CLASS_NAME m){ return (CLASS_NAME)(std::underlying_type<CLASS_NAME>::type(LAST_VALUE) + 1); }

ADD_ENUM_CLASS_ITERATION_OPERATORS( SpriteBorder,
                                    SpriteBorder::Top,
                                    SpriteBorder::Right );

#include "SlicerEdit.h"
#include "SlicerManipulator.h"
#include "SlicerView.h"
#include "SpriteBorderEditor.h"

bool IsBorderVertical(SpriteBorder border);
float GetBorderValueInPixels(ISprite* sprite, SpriteBorder b, float totalSizeInPixels);
void SetBorderValue(ISprite* sprite, SpriteBorder b, float pixelPosition, float totalSizeInPixels);
const char* SpriteBorderToString(SpriteBorder b);

#endif // CRYINCLUDE_EDITORCOMMON_SPRITEBORDEREDITORCOMMON_H
