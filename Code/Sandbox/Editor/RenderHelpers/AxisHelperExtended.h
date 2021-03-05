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

#ifndef CRYINCLUDE_EDITOR_RENDERHELPERS_AXISHELPEREXTENDED_H
#define CRYINCLUDE_EDITOR_RENDERHELPERS_AXISHELPEREXTENDED_H
#pragma once

#include "Objects/BaseObject.h"
#include "Include/IObjectManager.h"

struct DisplayContext;
struct HitContext;

class CAxisHelperExtended
{
public:
    CAxisHelperExtended();
    void DrawAxes(DisplayContext& dc, const Matrix34& matrix, bool bUsePhysicalProxy);

private:
    void DrawAxis(DisplayContext& dc, const Vec3& vDirAxis, const Vec3& vUpAxis, const Vec3& col, bool bUsePhysicalProxy);

private:
    Matrix34 m_matrix;
    Vec3 m_vPos;
    std::vector<CBaseObjectPtr> m_objects;
    CBaseObjectsArray m_objectsForPicker;
    CBaseObject* m_pCurObject;
    DWORD m_dwLastUpdateTime;

    float m_fMaxDist;
};

#endif // CRYINCLUDE_EDITOR_RENDERHELPERS_AXISHELPEREXTENDED_H
