/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "LineGizmo.h"

// Editor
#include "Include/IObjectManager.h"
#include "DisplayContext.h"
#include "DisplaySettings.h"
#include "Objects/EntityObject.h"
#include "GizmoManager.h"


//////////////////////////////////////////////////////////////////////////
CLineGizmo::CLineGizmo()
{
    m_color[0] = ColorF(0, 1, 1, 1);
    m_color[1] = ColorF(0, 1, 1, 1);
}

//////////////////////////////////////////////////////////////////////////
CLineGizmo::~CLineGizmo()
{
    if (m_object[0])
    {
        m_object[0]->RemoveEventListener(this);
    }
    if (m_object[1])
    {
        m_object[1]->RemoveEventListener(this);
    }
    m_object[0] = nullptr;
    m_object[1] = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CLineGizmo::SetObjects(CBaseObject* pObject1, CBaseObject* pObject2, const QString& boneName)
{
    assert(pObject1);
    assert(pObject2);
    m_object[0] = pObject1;
    m_object[1] = pObject2;
    m_boneName = boneName;

    m_object[0]->AddEventListener(this);
    m_object[1]->AddEventListener(this);
    CalcBounds();
}

//////////////////////////////////////////////////////////////////////////
void CLineGizmo::OnObjectEvent([[maybe_unused]] CBaseObject* object, int event)
{
    if (event == CBaseObject::ON_TRANSFORM)
    {
        // One of objects transformed, recalc gizmo bounds.
        CalcBounds();
        return;
    }
    if (event == CBaseObject::ON_DELETE)
    {
        // This gizmo must be deleted as well if one of the objects is deleted.
        GetIEditor()->GetObjectManager()->GetGizmoManager()->RemoveGizmo(this);
        return;
    }
    if (event == CBaseObject::ON_VISIBILITY)
    {
        // Check visibility of gizmo.
        bool bVisible = !m_object[0]->CheckFlags(OBJFLAG_INVISIBLE) && !m_object[1]->CheckFlags(OBJFLAG_INVISIBLE);
        if (bVisible)
        {
            SetFlags(GetFlags() & (~EGIZMO_HIDDEN));
        }
        else
        {
            SetFlags(GetFlags() | EGIZMO_HIDDEN);
        }
        return;
    }
}

//////////////////////////////////////////////////////////////////////////
void CLineGizmo::Display(DisplayContext& dc)
{
    if (dc.flags & DISPLAY_LINKS)
    {
        dc.DrawLine(m_point[0], m_point[1], m_color[0], m_color[1]);

        //if (!(dc.flags & DISPLAY_HIDENAMES))
        {
            Vec3 pos = 0.5f * (m_point[0] + m_point[1]);
            //dc.renderer->DrawLabelEx( p3+Vec3(0,0,0.3f),1.2f,col,true,true,m_name );

            float camDist = dc.camera->GetPosition().GetDistance(pos);
            float maxDist = dc.settings->GetLabelsDistance();
            if (camDist < dc.settings->GetLabelsDistance())
            {
                float range = maxDist / 2.0f;
                float col[4] = { m_color[0].r, m_color[0].g, m_color[0].b, m_color[0].a };
                if (camDist > range)
                {
                    col[3] = col[3] * (1.0f - (camDist - range) / range);
                }
                dc.SetColor(col[0], col[1], col[2], col[3]);
                dc.DrawTextLabel(pos + Vec3(0, 0, 0.2f), 1.2f, m_name.toUtf8().data());
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CLineGizmo::CalcBounds()
{
    m_bbox.Reset();

    for (int i = 0; i < 2; i++)
    {
        bool IsObjectLight = false;
        if (qobject_cast<CEntityObject*>(m_object[i]))
        {
            CEntityObject* entityobject = static_cast<CEntityObject*>(m_object[i].get());
            if (entityobject->IsLight())
            {
                m_point[i] = entityobject->GetWorldPos();
                IsObjectLight = true;
            }
        }
        if (IsObjectLight == false)
        {
            AABB box;
            m_object[i]->GetBoundBox(box);
            m_point[i] = 0.5f * Vec3(box.max + box.min);
        }
        m_bbox.Add(m_point[i]);
    }
}

//////////////////////////////////////////////////////////////////////////
void CLineGizmo::GetWorldBounds(AABB& bbox)
{
    bbox = m_bbox;
}

//////////////////////////////////////////////////////////////////////////
void CLineGizmo::SetColor(const Vec3& color1, const Vec3& color2, float alpha1, float alpha2)
{
    m_color[0] = ColorF(color1.x, color1.y, color1.z, alpha1);
    m_color[1] = ColorF(color2.x, color2.y, color2.z, alpha2);
}

//////////////////////////////////////////////////////////////////////////
void CLineGizmo::SetName(const char* sName)
{
    m_name = sName;
}

//////////////////////////////////////////////////////////////////////////
bool CLineGizmo::HitTest([[maybe_unused]] HitContext& hc)
{
    return false;
    /*
    if (hc.distanceTollerance != 0)
        return 0;

    Vec3 org = m_object->GetWorldPos();

    float fScreenScale = hc.view->GetScreenScaleFactor(org);
    float size = gSettings.gizmo.axisGizmoSize * fScreenScale;

    Vec3 x(size,0,0);
    Vec3 y(0,size,0);
    Vec3 z(0,0,size);

    float hitDist = 0.01f * fScreenScale;

    //////////////////////////////////////////////////////////////////////////
    // Calculate ray in local space of axis.
    //////////////////////////////////////////////////////////////////////////
    Matrix34 tm;
    RefCoordSys refCoordSys = GetIEditor()->GetReferenceCoordSys();
    if (refCoordSys == COORDS_LOCAL)
    {
        tm = m_object->GetWorldTM();
        tm.NoScale();
    }
    else
    {
        tm.SetIdentity();
        tm.SetTranslation( m_object->GetWorldPos() );
    }
    tm.Invert44();
    Vec3 raySrc = tm.TransformPoint( hc.raySrc );
    //CHANGED_BY_IVO
    //Vec3 rayDir = tm.TransformVector( hc.rayDir );
    Vec3 rayDir = GetTransposed44(tm) * ( hc.rayDir );
    //////////////////////////////////////////////////////////////////////////

    Vec3 pnt;
    BBox box;
    box.min = - Vec3(hitDist*2,hitDist*2,hitDist*2);
    box.max = Vec3(size+hitDist*2,size+hitDist*2,size+hitDist*2);
    if (!box.IsIntersectRay( raySrc,rayDir,pnt ))
    {
        m_highlightAxis = 0;
        return false;
    }


    float axisdist[10];
    Vec3 np[10];

    Vec3 rayTrg = raySrc + rayDir*10000.0f;

    size *= 0.3f;
    Vec3 p1(size,size,0);
    Vec3 p2(size,0,size);
    Vec3 p3(0,size,size);

    axisdist[AXIS_X] = RayToLineDistance( raySrc,rayTrg,Vec3(0,0,0),x,np[AXIS_X] );
    axisdist[AXIS_Y] = RayToLineDistance( raySrc,rayTrg,Vec3(0,0,0),y,np[AXIS_Y] );
    axisdist[AXIS_Z] = RayToLineDistance( raySrc,rayTrg,Vec3(0,0,0),z,np[AXIS_Z] );
    axisdist[AXIS_XY] = RayToLineDistance( raySrc,rayTrg,p1,p1-x*0.3f,np[AXIS_XY] );
    axisdist[AXIS_XZ] = RayToLineDistance( raySrc,rayTrg,p2,p2-x*0.3f,np[AXIS_XZ] );
    axisdist[AXIS_YZ] = RayToLineDistance( raySrc,rayTrg,p3,p3-y*0.3f,np[AXIS_YZ] );

    float mindist = hitDist;
    int axis = 0;
    for (int i = AXIS_X; i <= AXIS_XZ; i++)
    {
        if (axisdist[i] < mindist)
        {
            mindist = axisdist[i];
            axis = i;
        }
    }

    if (axis != 0)
    {
        hc.axis = axis;
        hc.object = m_object;
        hc.dist = GetDistance(raySrc,np[axis]);
    }

    m_highlightAxis = axis;

    return axis != 0;
    */
}
