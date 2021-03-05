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

#ifndef CRYINCLUDE_EDITOR_INCLUDE_ITRANSFORMMANIPULATOR_H
#define CRYINCLUDE_EDITOR_INCLUDE_ITRANSFORMMANIPULATOR_H
#pragma once

#include "IEditor.h"

struct IDisplayViewport;
struct HitContext;
class CViewport;

//////////////////////////////////////////////////////////////////////////
// ITransformManipulator implementation.
//////////////////////////////////////////////////////////////////////////
struct ITransformManipulator
{
    virtual Matrix34 GetTransformation(RefCoordSys coordSys, IDisplayViewport* view = nullptr) const = 0;
    virtual void SetTransformation(RefCoordSys coordSys, const Matrix34& tm) = 0;
    virtual bool HitTestManipulator(HitContext& hc) = 0;
    virtual bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags) = 0;
    virtual void SetAlwaysUseLocal(bool on) = 0;
};

#endif // CRYINCLUDE_EDITOR_INCLUDE_ITRANSFORMMANIPULATOR_H
