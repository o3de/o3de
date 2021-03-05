/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITORCOMMON_QVIEWPORTEVENTS_H
#define CRYINCLUDE_EDITORCOMMON_QVIEWPORTEVENTS_H
#pragma once

#include "EditorCommonAPI.h"
class EDITOR_COMMON_API QViewport;

struct SMouseEvent
{
    enum EType
    {
        TYPE_NONE,
        TYPE_PRESS,
        TYPE_RELEASE,
        TYPE_MOVE
    };

    enum EButton
    {
        BUTTON_NONE,
        BUTTON_LEFT,
        BUTTON_RIGHT,
        BUTTON_MIDDLE
    };

    EType type;
    int x;
    int y;
    EButton button;
    bool shift;
    bool control;
    QViewport* viewport;

    SMouseEvent()
        : type(TYPE_NONE)
        , x(INT_MIN)
        , y(INT_MIN)
        , button(BUTTON_NONE)
        , viewport(0)
        , shift(false)
        , control(false)
    {
    }
};

struct SSelectionID
{
};

struct SInteractionEvent
{
    enum EType
    {
        TYPE_NONE,
        TYPE_ENTER,
        TYPE_LEAVE,
        TYPE_DRAG
    };

    SSelectionID selection;
    Vec3 start;
    Vec3 end;
};

struct SKeyEvent
{
    enum EType
    {
        TYPE_NONE,
        TYPE_PRESS,
        TYPE_RELEASE
    };

    EType type;
    int key;

    SKeyEvent()
        : type(TYPE_NONE)
    {
    }
};

#endif // CRYINCLUDE_EDITORCOMMON_QVIEWPORTEVENTS_H
