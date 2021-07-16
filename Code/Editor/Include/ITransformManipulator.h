/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
