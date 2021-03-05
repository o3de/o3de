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

#include "EditorDefs.h"

#include "Grid.h"

// Editor
#include "Settings.h"
#include "Objects/SelectionGroup.h"

//////////////////////////////////////////////////////////////////////////
CGrid::CGrid()
{
    scale = 1;
    size = 1;
    majorLine = 16;
    bEnabled = true;
    rotationAngles = Ang3(0.0f, 0.0f, 0.0f);
    translation = Vec3(0.0f, 0.0f, 0.0f);

    bAngleSnapEnabled = true;
    angleSnap = 5;
}

//////////////////////////////////////////////////////////////////////////
Vec3 CGrid::Snap(const Vec3& vec) const
{
    if (!bEnabled || size < 0.001)
    {
        return vec;
    }
    Vec3 snapped;
    snapped.x = floor((vec.x / size) / scale + 0.5) * size * scale;
    snapped.y = floor((vec.y / size) / scale + 0.5) * size * scale;
    snapped.z = floor((vec.z / size) / scale + 0.5) * size * scale;
    return snapped;
}

//////////////////////////////////////////////////////////////////////////
Vec3 CGrid::Snap(const Vec3& vec, double fZoom) const
{
    if (!bEnabled || size < 0.001f)
    {
        return vec;
    }

    Matrix34 tm = GetMatrix();

    double zoomscale = scale * fZoom;
    Vec3 snapped;

    Matrix34 invtm = tm.GetInverted();

    snapped = invtm * vec;

    snapped.x = floor((snapped.x / size) / zoomscale + 0.5) * size * zoomscale;
    snapped.y = floor((snapped.y / size) / zoomscale + 0.5) * size * zoomscale;
    snapped.z = floor((snapped.z / size) / zoomscale + 0.5) * size * zoomscale;

    snapped = tm * snapped;

    return snapped;
}

//////////////////////////////////////////////////////////////////////////
double CGrid::SnapAngle(double angle) const
{
    if (!bAngleSnapEnabled)
    {
        return angle;
    }
    return floor(angle / angleSnap + 0.5) * angleSnap;
}

//////////////////////////////////////////////////////////////////////////
Ang3 CGrid::SnapAngle(const Ang3& vec) const
{
    if (!bAngleSnapEnabled)
    {
        return vec;
    }
    Ang3 snapped;
    snapped.x = floor(vec.x / angleSnap + 0.5) * angleSnap;
    snapped.y = floor(vec.y / angleSnap + 0.5) * angleSnap;
    snapped.z = floor(vec.z / angleSnap + 0.5) * angleSnap;
    return snapped;
}

//////////////////////////////////////////////////////////////////////////
void CGrid::Serialize(XmlNodeRef& xmlNode, bool bLoading)
{
    if (bLoading)
    {
        // Loading.
        xmlNode->getAttr("Size", size);
        xmlNode->getAttr("Scale", scale);
        xmlNode->getAttr("Enabled", bEnabled);
        xmlNode->getAttr("MajorSize", majorLine);
        xmlNode->getAttr("AngleSnap", angleSnap);
        xmlNode->getAttr("AngleSnapEnabled", bAngleSnapEnabled);
        if (size < 0.01)
        {
            size = 0.01;
        }
    }
    else
    {
        // Saving.
        xmlNode->setAttr("Size", size);
        xmlNode->setAttr("Scale", scale);
        xmlNode->setAttr("Enabled", bEnabled);
        xmlNode->setAttr("MajorSize", majorLine);
        xmlNode->setAttr("AngleSnap", angleSnap);
        xmlNode->setAttr("AngleSnapEnabled", bAngleSnapEnabled);
    }
}

//////////////////////////////////////////////////////////////////////////
Matrix34 CGrid::GetMatrix() const
{
    Matrix34 tm;

    if (gSettings.snap.bGridUserDefined)
    {
        Ang3 angles = Ang3(rotationAngles.x * gf_PI / 180.0, rotationAngles.y * gf_PI / 180.0, rotationAngles.z * gf_PI / 180.0);

        tm = Matrix33::CreateRotationXYZ(angles);

        if (gSettings.snap.bGridGetFromSelected)
        {
            CSelectionGroup* sel = GetIEditor()->GetSelection();
            if (sel->GetCount() > 0)
            {
                CBaseObject* obj = sel->GetObject(0);
                tm = obj->GetWorldTM();
                tm.OrthonormalizeFast();
                tm.SetTranslation(Vec3(0, 0, 0));
            }
        }
    }
    else if (GetIEditor()->GetReferenceCoordSys() == COORDS_LOCAL)
    {
        tm.SetIdentity();

        CSelectionGroup* sel = GetIEditor()->GetSelection();
        if (sel->GetCount() > 0)
        {
            CBaseObject* obj = sel->GetObject(0);
            tm = obj->GetWorldTM();
            tm.OrthonormalizeFast();
            tm.SetTranslation(Vec3(0, 0, 0));
        }
    }
    else
    {
        tm.SetIdentity();
    }

    return tm;
}
