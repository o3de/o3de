/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Cry_Geo.h>
#include "DisplayContext.h"
#include "IRenderAuxGeom.h"
#include "IEditor.h"
#include "Include/IIconManager.h"
#include "Include/IDisplayViewport.h"
#include <Editor/Util/EditorUtils.h>

#include <QDateTime>
#include <QPoint>

#include <AzFramework/Terrain/TerrainDataRequestBus.h>

#define FREEZE_COLOR QColor(100, 100, 100)

//////////////////////////////////////////////////////////////////////////
DisplayContext::DisplayContext()
{
    view = 0;
    flags = 0;
    settings = 0;
    pIconManager = 0;
    m_renderState = 0;

    m_currentMatrix = 0;
    m_matrixStack[m_currentMatrix].SetIdentity();
    pRenderAuxGeom = nullptr; // ToDo: Remove DisplayContext or update to work with Atom: LYN-3670
    m_thickness = 0;

    m_width = 0;
    m_height = 0;

    m_textureLabels.reserve(100);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::SetView(IDisplayViewport* pView)
{
    view = pView;
    int width, height;
    view->GetDimensions(&width, &height);
    m_width = static_cast<float>(width);
    m_height = static_cast<float>(height);
    m_textureLabels.resize(0);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::InternalDrawLine(const Vec3& v0, const ColorB& colV0, const Vec3& v1, const ColorB& colV1)
{
    pRenderAuxGeom->DrawLine(v0, colV0, v1, colV1, m_thickness);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawPoint(const Vec3& p, int nSize)
{
    pRenderAuxGeom->DrawPoint(ToWorldSpacePosition(p), m_color4b, static_cast<uint8>(nSize));
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawTri(const Vec3& p1, const Vec3& p2, const Vec3& p3)
{
    pRenderAuxGeom->DrawTriangle(ToWorldSpacePosition(p1), m_color4b, ToWorldSpacePosition(p2), m_color4b, ToWorldSpacePosition(p3), m_color4b);
}

void DisplayContext::DrawTriangles(const AZStd::vector<Vec3>& vertices, const ColorB& color)
{
    pRenderAuxGeom->DrawTriangles(vertices.begin(), static_cast<uint32>(vertices.size()), color);
}

void DisplayContext::DrawTrianglesIndexed(const AZStd::vector<Vec3>& vertices, const AZStd::vector<vtx_idx>& indices, const ColorB& color)
{
    pRenderAuxGeom->DrawTriangles(vertices.begin(), static_cast<uint32>(vertices.size()), indices.begin(), static_cast<uint32_t>(indices.size()), color);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawQuad(const Vec3& p1, const Vec3& p2, const Vec3& p3, const Vec3& p4)
{
    Vec3 p[4] = { ToWorldSpacePosition(p1), ToWorldSpacePosition(p2), ToWorldSpacePosition(p3), ToWorldSpacePosition(p4) };
    pRenderAuxGeom->DrawTriangle(p[0], m_color4b, p[1], m_color4b, p[2], m_color4b);
    pRenderAuxGeom->DrawTriangle(p[2], m_color4b, p[3], m_color4b, p[0], m_color4b);
}

void DisplayContext::DrawQuad(float width, float height)
{
    pRenderAuxGeom->DrawQuad(width, height, m_matrixStack[m_currentMatrix], m_color4b);
}

void DisplayContext::DrawWireQuad(const Vec3& p1, const Vec3& p2, const Vec3& p3, const Vec3& p4)
{
    Vec3 p[4] = { ToWorldSpacePosition(p1), ToWorldSpacePosition(p2), ToWorldSpacePosition(p3), ToWorldSpacePosition(p4) };
    pRenderAuxGeom->DrawPolyline(p, 4, true, m_color4b);
}

void DisplayContext::DrawWireQuad(float width, float height)
{
    const float halfWidth = width * 0.5f;
    const float halfHeight = height * 0.5f;

    const Vec3 p[4] =
    {
        ToWorldSpacePosition(Vec3(-halfWidth, 0.0f,  halfHeight)),
        ToWorldSpacePosition(Vec3( halfWidth, 0.0f,  halfHeight)),
        ToWorldSpacePosition(Vec3( halfWidth, 0.0f, -halfHeight)),
        ToWorldSpacePosition(Vec3(-halfWidth, 0.0f, -halfHeight)),
    };
    pRenderAuxGeom->DrawPolyline(p, 4, true, m_color4b);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawCylinder(const Vec3& p1, const Vec3& p2, float radius, float height)
{
    Vec3 p[2] = { ToWorldSpacePosition(p1), ToWorldSpacePosition(p2) };
    Vec3 dir = p[1] - p[0];
    pRenderAuxGeom->DrawCylinder(p[0], dir, radius, height, m_color4b);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawCone(const Vec3& pos, const Vec3& dir, float radius, float height, bool drawShaded /*= true*/)
{
    const Vec3 worldPos = ToWorldSpacePosition(pos);
    const Vec3 worldDir = ToWorldSpaceVector(dir);
    pRenderAuxGeom->DrawCone(worldPos, worldDir, radius, height, m_color4b, drawShaded);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawWireCylinder(const Vec3& center, const Vec3& axis, float radius, float height)
{
    if (radius > FLT_EPSILON && height > FLT_EPSILON && axis.GetLengthSquared() > FLT_EPSILON)
    {
        Vec3 axisNormalized = axis.GetNormalized();

        // Draw circles at bottom & top of cylinder
        Vec3 centerToTop = axisNormalized * height * 0.5f;
        Vec3 circle1Center = center - centerToTop;
        Vec3 circle2Center = center + centerToTop;
        // DrawArc() takes local coordinates
        DrawArc(circle1Center, radius, 0.0f, 360.0f, 22.5f, axisNormalized);
        DrawArc(circle2Center, radius, 0.0f, 360.0f, 22.5f, axisNormalized);

        // Draw 4 lines up side of cylinder
        Vec3 rightDirNormalized, frontDirNormalized;
        GetBasisVectors(axisNormalized, rightDirNormalized, frontDirNormalized);
        Vec3 centerToRightEdge = rightDirNormalized * radius;
        Vec3 centerToFrontEdge = frontDirNormalized * radius;
        // InternalDrawLine() takes world coordinates
        InternalDrawLine(ToWorldSpacePosition(circle1Center + centerToRightEdge), m_color4b,
            ToWorldSpacePosition(circle2Center + centerToRightEdge), m_color4b);
        InternalDrawLine(ToWorldSpacePosition(circle1Center - centerToRightEdge), m_color4b,
            ToWorldSpacePosition(circle2Center - centerToRightEdge), m_color4b);
        InternalDrawLine(ToWorldSpacePosition(circle1Center + centerToFrontEdge), m_color4b,
            ToWorldSpacePosition(circle2Center + centerToFrontEdge), m_color4b);
        InternalDrawLine(ToWorldSpacePosition(circle1Center - centerToFrontEdge), m_color4b,
            ToWorldSpacePosition(circle2Center - centerToFrontEdge), m_color4b);
    }
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawSolidCylinder(const Vec3& center, const Vec3& axis, float radius, float height, bool drawShaded)
{
    if (radius > FLT_EPSILON && height > FLT_EPSILON && axis.GetLengthSquared() > FLT_EPSILON)
    {
        // transform everything to world space
        Vec3 wsCenter = ToWorldSpacePosition(center);

        // determine scale in dir direction, apply to height
        Vec3 axisNormalized = axis.GetNormalized();
        Vec3 wsAxis = ToWorldSpaceVector(axisNormalized);
        float wsAxisLength = wsAxis.GetLength();
        float wsHeight = height * wsAxisLength;

        // determine scale in orthogonal direction, apply to radius
        Vec3 radiusDirNormalized = axisNormalized.GetOrthogonal();
        radiusDirNormalized.Normalize();
        Vec3 wsRadiusDir = ToWorldSpaceVector(radiusDirNormalized);
        float wsRadiusDirLen = wsRadiusDir.GetLength();
        float wsRadius = radius * wsRadiusDirLen;

        pRenderAuxGeom->DrawCylinder(wsCenter, wsAxis, wsRadius, wsHeight, m_color4b, drawShaded);
    }
}

//////////////////////////////////////////////////////////////////////////

void DisplayContext::DrawWireCapsule(const Vec3& center, const Vec3& axis, float radius, float heightStraightSection)
{
    if (radius > FLT_EPSILON && axis.GetLengthSquared() > FLT_EPSILON)
    {
        Vec3 axisNormalized = axis.GetNormalizedFast();

        // Draw cylinder part (or just a circle around the middle)
        if (heightStraightSection > FLT_EPSILON)
        {
            DrawWireCylinder(center, axis, radius, heightStraightSection);
        }
        else
        {
            DrawArc(center, radius, 0.0f, 360.0f, 22.5f, axisNormalized);
        }

        // Draw top cap as two criss-crossing 180deg arcs
        Vec3 ortho1Normalized, ortho2Normalized;
        GetBasisVectors(axisNormalized, ortho1Normalized, ortho2Normalized);
        Vec3 centerToTopCircleCenter = axisNormalized * heightStraightSection * 0.5f;
        DrawArc(center + centerToTopCircleCenter, radius,  90.0f, 180.0f, 22.5f, ortho1Normalized);
        DrawArc(center + centerToTopCircleCenter, radius, 180.0f, 180.0f, 22.5f, ortho2Normalized);

        // Draw bottom cap
        DrawArc(center - centerToTopCircleCenter, radius, -90.0f, 180.0f, 22.5f, ortho1Normalized);
        DrawArc(center - centerToTopCircleCenter, radius,   0.0f, 180.0f, 22.5f, ortho2Normalized);
    }
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawWireBox(const Vec3& min, const Vec3& max)
{
    pRenderAuxGeom->DrawAABB(AABB(min, max), m_matrixStack[m_currentMatrix], false, m_color4b, eBBD_Faceted);
}

void DisplayContext::DrawWireBox(const AZ::Vector3& min, const AZ::Vector3& max)
{
    pRenderAuxGeom->DrawAABB(
        AABB(Vec3(min.GetX(), min.GetY(), min.GetZ()), Vec3(max.GetX(), max.GetY(), max.GetZ())),
        m_matrixStack[m_currentMatrix], false, m_color4b, eBBD_Faceted);
}
//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawSolidBox(const Vec3& min, const Vec3& max)
{
    pRenderAuxGeom->DrawAABB(AABB(min, max), m_matrixStack[m_currentMatrix], true, m_color4b, eBBD_Faceted);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawSolidOBB(const Vec3& center, const Vec3& axisX, const Vec3& axisY, const Vec3& axisZ, const Vec3& halfExtents)
{
    OBB obb;
    obb.m33 = Matrix33::CreateFromVectors(axisX, axisY, axisZ);
    obb.c = Vec3(0, 0, 0);
    obb.h = halfExtents;
    pRenderAuxGeom->DrawOBB(obb, center, true, m_color4b, eBBD_Faceted);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawLine(const Vec3& p1, const Vec3& p2)
{
    InternalDrawLine(ToWorldSpacePosition(p1), m_color4b, ToWorldSpacePosition(p2), m_color4b);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawPolyLine(const Vec3* pnts, int numPoints, bool cycled)
{
    MAKE_SURE(numPoints >= 2, return );
    MAKE_SURE(pnts != 0, return );

    int numSegments = cycled ? numPoints : (numPoints - 1);
    Vec3 p1 = ToWorldSpacePosition(pnts[0]);
    Vec3 p2;
    for (int i = 0; i < numSegments; i++)
    {
        int j = (i + 1) % numPoints;
        p2 = ToWorldSpacePosition(pnts[j]);
        InternalDrawLine(p1, m_color4b, p2, m_color4b);
        p1 = p2;
    }
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawTerrainCircle(const Vec3& worldPos, float radius, float height)
{
    auto terrain = AzFramework::Terrain::TerrainDataRequestBus::FindFirstHandler();
    // Draw circle with default radius.
    Vec3 p0, p1;
    p0.x = worldPos.x + radius * sin(0.0f);
    p0.y = worldPos.y + radius * cos(0.0f);
    const float defaultTerrainElevation = AzFramework::Terrain::TerrainDataRequests::GetDefaultTerrainHeight();
    float terrainElevation = terrain ? terrain->GetHeightFromFloats(p0.x, p0.y) : defaultTerrainElevation;
    p0.z = terrainElevation + height;

    float step = 20.0f / 180 * gf_PI;
    for (float angle = step; angle < 360.0f / 180 * gf_PI + step; angle += step)
    {
        p1.x = worldPos.x + radius * sin(angle);
        p1.y = worldPos.y + radius * cos(angle);
        terrainElevation = terrain ? terrain->GetHeightFromFloats(p1.x, p1.y) : defaultTerrainElevation;
        p1.z = terrainElevation + height;

        InternalDrawLine(ToWorldSpacePosition(p0), m_color4b, ToWorldSpacePosition(p1), m_color4b);
        p0 = p1;
    }
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawTerrainCircle(const Vec3& worldPos, float radius, float angle1, float angle2, float height)
{
    auto terrain = AzFramework::Terrain::TerrainDataRequestBus::FindFirstHandler();
    // Draw circle with default radius.
    Vec3 p0, p1;
    p0.x = worldPos.x + radius * sin(angle1);
    p0.y = worldPos.y + radius * cos(angle1);
    float terrainElevation = terrain ? terrain->GetHeightFromFloats(p0.x, p0.y) : 0.0f;
    p0.z = terrainElevation + height;

    float step = 20.0f / 180 * gf_PI;
    for (float angle = step + angle1; angle < angle2; angle += step)
    {
        p1.x = worldPos.x + radius * sin(angle);
        p1.y = worldPos.y + radius * cos(angle);
        terrainElevation = terrain ? terrain->GetHeightFromFloats(p1.x, p1.y) : 0.0f;
        p1.z = terrainElevation + height;

        InternalDrawLine(ToWorldSpacePosition(p0), m_color4b, ToWorldSpacePosition(p1), m_color4b);
        p0 = p1;
    }

    p1.x = worldPos.x + radius * sin(angle2);
    p1.y = worldPos.y + radius * cos(angle2);
    terrainElevation = terrain ? terrain->GetHeightFromFloats(p1.x, p1.y) : 0.0f;
    p1.z = terrainElevation + height;

    InternalDrawLine(ToWorldSpacePosition(p0), m_color4b, ToWorldSpacePosition(p1), m_color4b);
}


void DisplayContext::DrawArc(const Vec3& pos, float radius, float startAngleDegrees, float sweepAngleDegrees, float angularStepDegrees, int referenceAxis)
{
    // angularStepDegrees cannot be zero as it is used to divide sweepAngleDegrees
    if (fabs(angularStepDegrees) < FLT_EPSILON)
    {
        return;
    }

    float startAngleRadians = DEG2RAD(startAngleDegrees);
    float sweepAngleRadians = DEG2RAD(sweepAngleDegrees);

    float angle = startAngleRadians;

    const uint referenceAxis0 = referenceAxis % 3;
    const uint referenceAxis1 = (referenceAxis + 1) % 3;
    const uint referenceAxis2 = (referenceAxis + 2) % 3;

    Vec3 p0;
    p0[referenceAxis0] = pos[referenceAxis0];
    p0[referenceAxis1] = pos[referenceAxis1] + radius * sin(angle);
    p0[referenceAxis2] = pos[referenceAxis2] + radius * cos(angle);
    p0 = ToWorldSpacePosition(p0);

    float step = DEG2RAD(angularStepDegrees);
    int numSteps = std::abs(static_cast<int>(std::ceil(sweepAngleRadians / step)));

    Vec3 p1;
    for (int i = 0; i < numSteps; ++i)
    {
        angle += std::min(step, sweepAngleRadians); // Don't go past sweepAngleRadians when stepping or the arc will be too long.
        sweepAngleRadians -= step;

        p1[referenceAxis0] = pos[referenceAxis0];
        p1[referenceAxis1] = pos[referenceAxis1] + radius * sin(angle);
        p1[referenceAxis2] = pos[referenceAxis2] + radius * cos(angle);
        p1 = ToWorldSpacePosition(p1);

        InternalDrawLine(p0, m_color4b, p1, m_color4b);
        p0 = p1;
    }
}

void DisplayContext::DrawArc(const Vec3& pos, float radius, float startAngleDegrees, float sweepAngleDegrees, float angularStepDegrees, const Vec3& fixedAxis)
{
    // angularStepDegrees cannot be zero as it is used to divide sweepAngleDegrees
    if (fabs(angularStepDegrees) < FLT_EPSILON)
    {
        return;
    }

    float startAngleRadians = DEG2RAD(startAngleDegrees);
    float sweepAngleRadians = DEG2RAD(sweepAngleDegrees);

    Vec3 a;
    Vec3 b;
    GetBasisVectors(fixedAxis, a, b);

    float angle = startAngleRadians;

    float cosAngle = cos(angle) * radius;
    float sinAngle = sin(angle) * radius;

    Vec3 p0;
    p0.x = pos.x + cosAngle * a.x + sinAngle * b.x;
    p0.y = pos.y + cosAngle * a.y + sinAngle * b.y;
    p0.z = pos.z + cosAngle * a.z + sinAngle * b.z;
    p0 = ToWorldSpacePosition(p0);

    float step = DEG2RAD(angularStepDegrees);
    int numSteps = std::abs(static_cast<int>(std::ceil(sweepAngleRadians / step)));

    Vec3 p1;
    for (int i = 0; i < numSteps; ++i)
    {
        angle += std::min(step, sweepAngleRadians); // Don't go past sweepAngleRadians when stepping or the arc will be too long.
        sweepAngleRadians -= step;

        float cosAngle2 = cos(angle) * radius;
        float sinAngle2 = sin(angle) * radius;

        p1.x = pos.x + cosAngle2 * a.x + sinAngle2 * b.x;
        p1.y = pos.y + cosAngle2 * a.y + sinAngle2 * b.y;
        p1.z = pos.z + cosAngle2 * a.z + sinAngle2 * b.z;
        p1 = ToWorldSpacePosition(p1);

        InternalDrawLine(p0, m_color4b, p1, m_color4b);

        p0 = p1;
    }
}

//////////////////////////////////////////////////////////////////////////
// Vera, Confetti
void DisplayContext::DrawArcWithArrow(const Vec3& pos, float radius, float startAngleDegrees, float sweepAngleDegrees, float angularStepDegrees, const Vec3& fixedAxis)
{
    // angularStepDegrees cannot be zero as it is used to divide sweepAngleDegrees
    // Grab the code from draw arc to get the final p0 and p1;
    if (fabs(angularStepDegrees) < FLT_EPSILON)
    {
        return;
    }

    float startAngleRadians = DEG2RAD(startAngleDegrees);
    float sweepAngleRadians = DEG2RAD(sweepAngleDegrees);

    Vec3 a;
    Vec3 b;
    GetBasisVectors(fixedAxis, a, b);

    float angle = startAngleRadians;

    float cosAngle = cos(angle) * radius;
    float sinAngle = sin(angle) * radius;

    Vec3 p0;
    p0.x = pos.x + cosAngle * a.x + sinAngle * b.x;
    p0.y = pos.y + cosAngle * a.y + sinAngle * b.y;
    p0.z = pos.z + cosAngle * a.z + sinAngle * b.z;
    p0 = ToWorldSpacePosition(p0);

    float step = DEG2RAD(angularStepDegrees);
    int numSteps = std::abs(static_cast<int>(std::ceil(sweepAngleRadians / step)));

    Vec3 p1;
    for (int i = 0; i < numSteps; ++i)
    {
        angle += step;
        float cosAngle2 = cos(angle) * radius;
        float sinAngle2 = sin(angle) * radius;

        p1.x = pos.x + cosAngle2 * a.x + sinAngle2 * b.x;
        p1.y = pos.y + cosAngle2 * a.y + sinAngle2 * b.y;
        p1.z = pos.z + cosAngle2 * a.z + sinAngle2 * b.z;
        p1 = ToWorldSpacePosition(p1);

        if (i + 1 >= numSteps) // For last step, draw an arrow
        {
            // p0 and p1 are global position. We could like to map it to local
            Matrix34 inverseMat =  m_matrixStack[m_currentMatrix].GetInverted();
            Vec3 localP0 = inverseMat.TransformPoint(p0);
            Vec3 localP1 = inverseMat.TransformPoint(p1);
            DrawArrow(localP0, localP1, m_thickness);
        }
        else
        {
            InternalDrawLine(p0, m_color4b, p1, m_color4b);
        }

        p0 = p1;
    }
}


//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawCircle(const Vec3& pos, float radius, int nUnchangedAxis)
{
    // Draw circle with default radius.
    Vec3 p0, p1;
    p0[nUnchangedAxis] = pos[nUnchangedAxis];
    p0[(nUnchangedAxis + 1) % 3] = pos[(nUnchangedAxis + 1) % 3] + radius * sin(0.0f);
    p0[(nUnchangedAxis + 2) % 3] = pos[(nUnchangedAxis + 2) % 3] + radius * cos(0.0f);
    p0 = ToWorldSpacePosition(p0);
    const float step = 10.0f / 180.0f * gf_PI;
    for (float angle = step; angle < 360.0f / 180.0f * gf_PI + step; angle += step)
    {
        p1[nUnchangedAxis] = pos[nUnchangedAxis];
        p1[(nUnchangedAxis + 1) % 3] = pos[(nUnchangedAxis + 1) % 3] + radius * sin(angle);
        p1[(nUnchangedAxis + 2) % 3] = pos[(nUnchangedAxis + 2) % 3] + radius * cos(angle);
        p1 = ToWorldSpacePosition(p1);
        InternalDrawLine(p0, m_color4b, p1, m_color4b);
        p0 = p1;
    }
}

void DisplayContext::DrawHalfDottedCircle(const Vec3& pos, float radius, const Vec3& viewPos, int nUnchangedAxis)
{
    // Draw circle with default radius
    Vec3 p0, p1;
    p0[nUnchangedAxis] = pos[nUnchangedAxis];
    p0[(nUnchangedAxis + 1) % 3] = pos[(nUnchangedAxis + 1) % 3] + radius * sin(0.0f);
    p0[(nUnchangedAxis + 2) % 3] = pos[(nUnchangedAxis + 2) % 3] + radius * cos(0.0f);
    p0 = ToWorldSpacePosition(p0);
    const Vec3 worldPos = ToWorldSpacePosition(pos);
    const Vec3 worldView = ToWorldSpacePosition(viewPos);
    const float step = 10.0f / 180.0f * gf_PI;
    size_t count = 0;
    for (float angle = step; angle < 360.0f / 180.0f * gf_PI + step; angle += step)
    {
        p1[nUnchangedAxis] = pos[nUnchangedAxis];
        p1[(nUnchangedAxis + 1) % 3] = pos[(nUnchangedAxis + 1) % 3] + radius * sin(angle);
        p1[(nUnchangedAxis + 2) % 3] = pos[(nUnchangedAxis + 2) % 3] + radius * cos(angle);
        p1 = ToWorldSpacePosition(p1);
        // is circle edge facing away from us or not
        const float dot = (p0 - worldPos).Dot(worldView - worldPos);
        const bool facing = dot > 0.0f;
        // if so skip every other line to produce a dotted effect
        if (facing || count % 2 == 0)
        {
            InternalDrawLine(p0, m_color4b, p1, m_color4b);
        }
        count++;
        p0 = p1;
    }
}

//////////////////////////////////////////////////////////////////////////
// Vera, Confetti
void DisplayContext::DrawDottedCircle(const Vec3& pos, float radius, const Vec3& nUnchangedAxis, int numberOfArrows /*=0*/, float stepDegree /*= 1*/)
{
    float startAngleRadians = 0;

    Vec3 a;
    Vec3 b;
    GetBasisVectors(nUnchangedAxis, a, b);

    float angle = startAngleRadians;

    float cosAngle = cos(angle) * radius;
    float sinAngle = sin(angle) * radius;

    Vec3 p0;
    p0.x = pos.x + cosAngle * a.x + sinAngle * b.x;
    p0.y = pos.y + cosAngle * a.y + sinAngle * b.y;
    p0.z = pos.z + cosAngle * a.z + sinAngle * b.z;
    p0 = ToWorldSpacePosition(p0);

    float step = DEG2RAD(stepDegree); // one degree each step
    int numSteps = aznumeric_cast<int>(2.0f * gf_PI / step);

    // Prepare for drawing arrow
    float arrowStep = 0;
    float arrowAngle = 0;
    if (numberOfArrows > 0)
    {
        arrowStep = 2 * gf_PI / numberOfArrows;
        arrowAngle = arrowStep;
    }

    Vec3 p1;
    for (int i = 0; i < numSteps; ++i)
    {
        angle += step;
        float cosAngle2 = cos(angle) * radius;
        float sinAngle2 = sin(angle) * radius;

        p1.x = pos.x + cosAngle2 * a.x + sinAngle2 * b.x;
        p1.y = pos.y + cosAngle2 * a.y + sinAngle2 * b.y;
        p1.z = pos.z + cosAngle2 * a.z + sinAngle2 * b.z;
        p1 = ToWorldSpacePosition(p1);


        if (arrowStep > 0) // If user want to draw arrow on circle
        {
            // if the arraw should be drawn between current angel and next angel
            if (angle <= arrowAngle && angle + step * 2 > arrowAngle)
            {
                // Get local position from global position
                Matrix34 inverseMat = m_matrixStack[m_currentMatrix].GetInverted();
                Vec3 localP0 = inverseMat.TransformPoint(p0);
                Vec3 localP1 = inverseMat.TransformPoint(p1);
                DrawArrow(localP0, localP1, m_thickness);
                arrowAngle += arrowStep;
                if (arrowAngle > 2 * gf_PI) // if the next arrow angle is greater than 2PI. Stop adding arrow.
                {
                    arrowStep = 0;
                }
            }
        }

        InternalDrawLine(p0, m_color4b, p1, m_color4b);

        // Skip a step
        angle += step;
        cosAngle2 = cos(angle) * radius;
        sinAngle2 = sin(angle) * radius;

        p1.x = pos.x + cosAngle2 * a.x + sinAngle2 * b.x;
        p1.y = pos.y + cosAngle2 * a.y + sinAngle2 * b.y;
        p1.z = pos.z + cosAngle2 * a.z + sinAngle2 * b.z;
        p1 = ToWorldSpacePosition(p1);

        p0 = p1;
    }
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawWireCircle2d(const QPoint& center, float radius, float z)
{
    Vec3 p0, p1, pos;
    pos.x = static_cast<float>(center.x());
    pos.y = static_cast<float>(center.y());
    pos.z = z;
    p0.x = pos.x + radius * sin(0.0f);
    p0.y = pos.y + radius * cos(0.0f);
    p0.z = z;
    float step = 10.0f / 180 * gf_PI;

    int prevState = GetState();
    //SetState( (prevState|e_Mode2D) & (~e_Mode3D) );
    for (float angle = step; angle < 360.0f / 180 * gf_PI + step; angle += step)
    {
        p1.x = pos.x + radius * sin(angle);
        p1.y = pos.y + radius * cos(angle);
        p1.z = z;
        InternalDrawLine(p0, m_color4b, p1, m_color4b);
        p0 = p1;
    }
    SetState(prevState);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawWireSphere(const Vec3& pos, float radius)
{
    Vec3 p0, p1;
    float step = 10.0f / 180 * gf_PI;
    float angle;

    // Z Axis
    p0 = pos;
    p1 = pos;
    p0.x += radius * sin(0.0f);
    p0.y += radius * cos(0.0f);
    p0 = ToWorldSpacePosition(p0);
    for (angle = step; angle < 360.0f / 180 * gf_PI + step; angle += step)
    {
        p1.x = pos.x + radius * sin(angle);
        p1.y = pos.y + radius * cos(angle);
        p1.z = pos.z;
        p1 = ToWorldSpacePosition(p1);
        InternalDrawLine(p0, m_color4b, p1, m_color4b);
        p0 = p1;
    }

    // X Axis
    p0 = pos;
    p1 = pos;
    p0.y += radius * sin(0.0f);
    p0.z += radius * cos(0.0f);
    p0 = ToWorldSpacePosition(p0);
    for (angle = step; angle < 360.0f / 180 * gf_PI + step; angle += step)
    {
        p1.x = pos.x;
        p1.y = pos.y + radius * sin(angle);
        p1.z = pos.z + radius * cos(angle);
        p1 = ToWorldSpacePosition(p1);
        InternalDrawLine(p0, m_color4b, p1, m_color4b);
        p0 = p1;
    }

    // Y Axis
    p0 = pos;
    p1 = pos;
    p0.x += radius * sin(0.0f);
    p0.z += radius * cos(0.0f);
    p0 = ToWorldSpacePosition(p0);
    for (angle = step; angle < 360.0f / 180 * gf_PI + step; angle += step)
    {
        p1.x = pos.x + radius * sin(angle);
        p1.y = pos.y;
        p1.z = pos.z + radius * cos(angle);
        p1 = ToWorldSpacePosition(p1);
        InternalDrawLine(p0, m_color4b, p1, m_color4b);
        p0 = p1;
    }
}


//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawWireSphere(const Vec3& pos, const Vec3 radius)
{
    Vec3 p0, p1;
    float step = 10.0f / 180 * gf_PI;
    float angle;

    // Z Axis
    p0 = pos;
    p1 = pos;
    p0.x += radius.x * sin(0.0f);
    p0.y += radius.y * cos(0.0f);
    p0 = ToWorldSpacePosition(p0);
    for (angle = step; angle < 360.0f / 180 * gf_PI + step; angle += step)
    {
        p1.x = pos.x + radius.x * sin(angle);
        p1.y = pos.y + radius.y * cos(angle);
        p1.z = pos.z;
        p1 = ToWorldSpacePosition(p1);
        InternalDrawLine(p0, m_color4b, p1, m_color4b);
        p0 = p1;
    }

    // X Axis
    p0 = pos;
    p1 = pos;
    p0.y += radius.y * sin(0.0f);
    p0.z += radius.z * cos(0.0f);
    p0 = ToWorldSpacePosition(p0);
    for (angle = step; angle < 360.0f / 180 * gf_PI + step; angle += step)
    {
        p1.x = pos.x;
        p1.y = pos.y + radius.y * sin(angle);
        p1.z = pos.z + radius.z * cos(angle);
        p1 = ToWorldSpacePosition(p1);
        InternalDrawLine(p0, m_color4b, p1, m_color4b);
        p0 = p1;
    }

    // Y Axis
    p0 = pos;
    p1 = pos;
    p0.x += radius.x * sin(0.0f);
    p0.z += radius.z * cos(0.0f);
    p0 = ToWorldSpacePosition(p0);
    for (angle = step; angle < 360.0f / 180 * gf_PI + step; angle += step)
    {
        p1.x = pos.x + radius.x * sin(angle);
        p1.y = pos.y;
        p1.z = pos.z + radius.z * cos(angle);
        p1 = ToWorldSpacePosition(p1);
        InternalDrawLine(p0, m_color4b, p1, m_color4b);
        p0 = p1;
    }
}

void DisplayContext::DrawWireDisk(const Vec3& pos, const Vec3& dir, float radius)
{
    // Draw circle
    DrawArc(pos, radius, 0.0f, 360.0f, 11.25f, dir);

    // Draw disk direction normal from center.
    DrawLine(pos, pos + dir * radius * 0.2f);
}

void DisplayContext::DrawWireQuad2d(const QPoint& pmin, const QPoint& pmax, float z)
{
    int prevState = GetState();
    SetState((prevState | e_Mode2D) & (~e_Mode3D));
    float minX = static_cast<float>(pmin.x());
    float minY = static_cast<float>(pmin.y());
    float maxX = static_cast<float>(pmax.x());
    float maxY = static_cast<float>(pmax.y());
    InternalDrawLine(Vec3(minX, minY, z), m_color4b, Vec3(maxX, minY, z), m_color4b);
    InternalDrawLine(Vec3(maxX, minY, z), m_color4b, Vec3(maxX, maxY, z), m_color4b);
    InternalDrawLine(Vec3(maxX, maxY, z), m_color4b, Vec3(minX, maxY, z), m_color4b);
    InternalDrawLine(Vec3(minX, maxY, z), m_color4b, Vec3(minX, minY, z), m_color4b);
    SetState(prevState);
}

void DisplayContext::DrawLine2d(const QPoint& p1, const QPoint& p2, float z)
{
    int prevState = GetState();

    SetState((prevState | e_Mode2D) & (~e_Mode3D));

    // If we don't have correct information, we try to get it, but while we
    // don't, we skip rendering this frame.
    if (m_width == 0 || m_height == 0)
    {
        if (view)
        {
            // We tell the window to update itself, as it might be needed to
            // get correct information.
            view->Update();
            int width, height;
            view->GetDimensions(&width, &height);
            m_width = static_cast<float>(width);
            m_height = static_cast<float>(height);
        }
    }
    else
    {
        InternalDrawLine(Vec3(p1.x() / m_width, p1.y() / m_height, z), m_color4b, Vec3(p2.x() / m_width, p2.y() / m_height, z), m_color4b);
    }

    SetState(prevState);
}

void DisplayContext::DrawLine2dGradient(const QPoint& p1, const QPoint& p2, float z, ColorB firstColor, ColorB secondColor)
{
    int prevState = GetState();

    SetState((prevState | e_Mode2D) & (~e_Mode3D));
    InternalDrawLine(Vec3(p1.x() / m_width, p1.y() / m_height, z), firstColor, Vec3(p2.x() / m_width, p2.y() / m_height, z), secondColor);
    SetState(prevState);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawQuadGradient(const Vec3& p1, const Vec3& p2, const Vec3& p3, const Vec3& p4, ColorB firstColor, ColorB secondColor)
{
    Vec3 p[4] = { ToWorldSpacePosition(p1), ToWorldSpacePosition(p2), ToWorldSpacePosition(p3), ToWorldSpacePosition(p4) };
    pRenderAuxGeom->DrawTriangle(p[0], firstColor, p[1], firstColor, p[2], secondColor);
    pRenderAuxGeom->DrawTriangle(p[2], secondColor, p[3], secondColor, p[0], firstColor);
}

//////////////////////////////////////////////////////////////////////////
QColor DisplayContext::GetSelectedColor()
{
    float t = aznumeric_cast<float>(QDateTime::currentMSecsSinceEpoch() / 1000);
    float r1 = fabs(sin(t * 8.0f));
    if (r1 > 255)
    {
        r1 = 255;
    }
    return QColor(255, 0, aznumeric_cast<int>(r1 * 255));
    //          float r2 = cos(t*3);
    //dc.renderer->SetMaterialColor( 1,0,r1,0.5f );
}

QColor DisplayContext::GetFreezeColor()
{
    return FREEZE_COLOR;
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::SetSelectedColor(float fAlpha)
{
    SetColor(GetSelectedColor(), fAlpha);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::SetFreezeColor()
{
    SetColor(FREEZE_COLOR, 0.5f);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawLine(const Vec3& p1, const Vec3& p2, const ColorF& col1, const ColorF& col2)
{
    InternalDrawLine(ToWorldSpacePosition(p1), ColorB(col1), ToWorldSpacePosition(p2), ColorB(col2));
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawLine(const Vec3& p1, const Vec3& p2, const QColor& rgb1, const QColor& rgb2)
{
    InternalDrawLine(ToWorldSpacePosition(p1), 
        ColorB(static_cast<uint8>(rgb1.red()), static_cast<uint8>(rgb1.green()), static_cast<uint8>(rgb1.blue()), 255), 
        ToWorldSpacePosition(p2), 
        ColorB(static_cast<uint8>(rgb2.red()), static_cast<uint8>(rgb2.green()), static_cast<uint8>(rgb2.blue()), 255));
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawLines(const AZStd::vector<Vec3>& points, const ColorF& color)
{
    pRenderAuxGeom->DrawLines(points.begin(), static_cast<uint32>(points.size()), color, m_thickness);
}

//////////////////////////////////////////////////////////////////////////
// Vera, Confetti
void DisplayContext::DrawDottedLine(const Vec3& p1, const Vec3& p2, [[maybe_unused]] const ColorF& col1, [[maybe_unused]] const ColorF& col2, const float numOfSteps)
{
    Vec3 direction =  Vec3(p2.x - p1.x, p2.y - p1.y, p2.z - p1.z);
    //We only draw half of a step and leave the other half empty to make it a dotted line.
    Vec3 halfstep = (direction / numOfSteps) * 0.5f;
    Vec3 startPoint = p1;

    for (int stepCount = 0; stepCount < numOfSteps; stepCount++)
    {
        InternalDrawLine(ToWorldSpacePosition(startPoint), m_color4b, ToWorldSpacePosition(startPoint + halfstep), m_color4b);
        startPoint += 2 * halfstep; //skip a half step to make it dotted
    }
}


//////////////////////////////////////////////////////////////////////////
void DisplayContext::PushMatrix(const Matrix34& tm)
{
    assert(m_currentMatrix < 31);
    if (m_currentMatrix < 31)
    {
        m_currentMatrix++;
        m_matrixStack[m_currentMatrix] = m_matrixStack[m_currentMatrix - 1] * tm;
    }
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::PopMatrix()
{
    assert(m_currentMatrix > 0);
    if (m_currentMatrix > 0)
    {
        m_currentMatrix--;
    }
}

//////////////////////////////////////////////////////////////////////////
const Matrix34& DisplayContext::GetMatrix()
{
    return m_matrixStack[m_currentMatrix];
}

float DisplayContext::ToWorldSpaceMaxScale(float value)
{
    // get the max scaled value in case the transform on the stack is scaled non-uniformly
    const float transformedValueX = ToWorldSpaceVector(Vec3(value, 0.0f, 0.0f)).GetLength();
    const float transformedValueY = ToWorldSpaceVector(Vec3(0.0f, value, 0.0f)).GetLength();
    const float transformedValueZ = ToWorldSpaceVector(Vec3(0.0f, 0.0f, value)).GetLength();
    return AZ::GetMax(transformedValueX, AZ::GetMax(transformedValueY, transformedValueZ));
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawBall(const Vec3& pos, float radius, bool drawShaded)
{
    pRenderAuxGeom->DrawSphere(
        ToWorldSpacePosition(pos), ToWorldSpaceMaxScale(radius), m_color4b, drawShaded);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawDisk(const Vec3& pos, const Vec3& dir, float radius)
{
    pRenderAuxGeom->DrawDisk(
        ToWorldSpacePosition(pos), ToWorldSpaceVector(dir), ToWorldSpaceMaxScale(radius), m_color4b);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawArrow(const Vec3& src, const Vec3& trg, float fHeadScale, bool b2SidedArrow)
{
    float f2dScale = 1.0f;
    float arrowLen = 0.4f * fHeadScale;
    float arrowRadius = 0.1f * fHeadScale;
    if (flags & DISPLAY_2D)
    {
        f2dScale = 1.2f * ToWorldSpaceVector(Vec3(1, 0, 0)).GetLength();
    }
    Vec3 dir = trg - src;
    dir = ToWorldSpaceVector(dir.GetNormalized());
    Vec3 p0 = ToWorldSpacePosition(src);
    Vec3 p1 = ToWorldSpacePosition(trg);
    if (!b2SidedArrow)
    {
        p1 = p1 - dir * arrowLen;
        InternalDrawLine(p0, m_color4b, p1, m_color4b);
        pRenderAuxGeom->DrawCone(p1, dir, arrowRadius * f2dScale, arrowLen * f2dScale, m_color4b);
    }
    else
    {
        p0 = p0 + dir * arrowLen;
        p1 = p1 - dir * arrowLen;
        InternalDrawLine(p0, m_color4b, p1, m_color4b);
        pRenderAuxGeom->DrawCone(p0, -dir, arrowRadius * f2dScale, arrowLen * f2dScale, m_color4b);
        pRenderAuxGeom->DrawCone(p1, dir, arrowRadius * f2dScale, arrowLen * f2dScale, m_color4b);
    }
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::RenderObject(int objectType, const Vec3& pos, float scale)
{
    Matrix34 tm;
    tm.SetIdentity();

    tm = Matrix33::CreateScale(Vec3(scale, scale, scale)) * tm;

    tm.SetTranslation(pos);
    RenderObject(objectType, tm);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::RenderObject(int, const Matrix34&)
{
}

/////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawTerrainRect(float x1, float y1, float x2, float y2, float height)
{
    AzFramework::Terrain::TerrainDataRequests* terrain = AzFramework::Terrain::TerrainDataRequestBus::FindFirstHandler();
    if (terrain == nullptr)
    {
        return;
    }

    Vec3 p1, p2;
    float x, y;

    float step = MAX(y2 - y1, x2 - x1);
    if (step < 0.1)
    {
        return;
    }
    step = step / 100.0f;
    if (step > 10)
    {
        step /= 10;
    }

    for (y = y1; y < y2; y += step)
    {
        float ye = min(y + step, y2);

        p1.x = x1;
        p1.y = y;
        p1.z = terrain->GetHeightFromFloats(p1.x, p1.y) + height;

        p2.x = x1;
        p2.y = ye;
        p2.z = terrain->GetHeightFromFloats(p2.x, p2.y) + height;
        DrawLine(p1, p2);

        p1.x = x2;
        p1.y = y;
        p1.z = terrain->GetHeightFromFloats(p1.x, p1.y) + height;

        p2.x = x2;
        p2.y = ye;
        p2.z = terrain->GetHeightFromFloats(p2.x, p2.y) + height;
        DrawLine(p1, p2);
    }
    for (x = x1; x < x2; x += step)
    {
        float xe = min(x + step, x2);

        p1.x = x;
        p1.y = y1;
        p1.z = terrain->GetHeightFromFloats(p1.x, p1.y) + height;

        p2.x = xe;
        p2.y = y1;
        p2.z = terrain->GetHeightFromFloats(p2.x, p2.y) + height;
        DrawLine(p1, p2);

        p1.x = x;
        p1.y = y2;
        p1.z = terrain->GetHeightFromFloats(p1.x, p1.y) + height;

        p2.x = xe;
        p2.y = y2;
        p2.z = terrain->GetHeightFromFloats(p2.x, p2.y) + height;
        DrawLine(p1, p2);
    }
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawTerrainLine(Vec3 worldPos1, Vec3 worldPos2)
{
    worldPos1.z = worldPos2.z = 0;

    int steps = static_cast<int>((worldPos2 - worldPos1).GetLength() / 4);
    if (steps == 0)
    {
        steps = 1;
    }

    Vec3 step = (worldPos2 - worldPos1) / static_cast<float>(steps);

    AzFramework::Terrain::TerrainDataRequests* terrain = AzFramework::Terrain::TerrainDataRequestBus::FindFirstHandler();
    AZ_Assert(terrain != nullptr, "If there's no terrain, %s shouldn't be called", __FUNCTION__);

    Vec3 p1 = worldPos1;
    p1.z = terrain->GetHeightFromFloats(worldPos1.x, worldPos1.y);
    for (int i = 0; i < steps; ++i)
    {
        Vec3 p2 = p1 + step;
        p2.z = 0.1f + terrain->GetHeightFromFloats(p2.x, p2.y);

        DrawLine(p1, p2);

        p1 = p2;
    }
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawTextLabel(const Vec3& pos, float size, const char* text, const bool bCenter, [[maybe_unused]] int srcOffsetX, [[maybe_unused]] int scrOffsetY)
{
    AZ_ErrorOnce(nullptr, false, "DisplayContext::DrawTextLabel needs to be removed/ported to use Atom");

#if 0
      ColorF col(m_color4b.r * (1.0f / 255.0f), m_color4b.g * (1.0f / 255.0f), m_color4b.b * (1.0f / 255.0f), m_color4b.a * (1.0f / 255.0f));

    float fCol[4] = { col.r, col.g, col.b, col.a };
    if (flags & DISPLAY_2D)
    {
        //We need to pass Screen coordinates to Draw2dLabel Function
        Vec3 screenPos = GetView()->GetScreenTM().TransformPoint(pos);
        renderer->Draw2dLabel(screenPos.x, screenPos.y, size, fCol, bCenter, "%s", text);
    }
    else
    {
        renderer->DrawLabelEx(pos, size, fCol, true, true, text);
    }
#else
    AZ_UNUSED(pos);
    AZ_UNUSED(size);
    AZ_UNUSED(text);
    AZ_UNUSED(bCenter);
#endif
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::Draw2dTextLabel(float x, float y, float size, const char* text, bool bCenter)
{
    AZ_ErrorOnce(nullptr, false, "DisplayContext::Draw2dTextLabel needs to be removed/ported to use Atom");
#if 0
    float col[4] = { m_color4b.r * (1.0f / 255.0f), m_color4b.g * (1.0f / 255.0f), m_color4b.b * (1.0f / 255.0f), m_color4b.a * (1.0f / 255.0f) };
    renderer->Draw2dLabel(x, y, size, col, bCenter, "%s", text);
#else
    AZ_UNUSED(x);
    AZ_UNUSED(y);
    AZ_UNUSED(size);
    AZ_UNUSED(text);
    AZ_UNUSED(bCenter);
#endif
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::SetLineWidth(float width)
{
    m_thickness = width;
}

//////////////////////////////////////////////////////////////////////////
bool DisplayContext::IsVisible(const AABB& bounds)
{
    if (flags & DISPLAY_2D)
    {
        if (box.IsIntersectBox(bounds))
        {
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
uint32 DisplayContext::GetState() const
{
    return m_renderState;
}

//! Set a new render state flags.
//! @param returns previous render state.
uint32 DisplayContext::SetState(uint32 state)
{
    uint32 old = m_renderState;
    m_renderState = state;
    pRenderAuxGeom->SetRenderFlags(m_renderState);
    return old;
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DepthTestOff()
{
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
    pRenderAuxGeom->SetRenderFlags((m_renderState | e_DepthTestOff) & (~e_DepthTestOn));
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DepthTestOn()
{
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
    pRenderAuxGeom->SetRenderFlags((m_renderState | e_DepthTestOn) & (~e_DepthTestOff));
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DepthWriteOff()
{
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
    pRenderAuxGeom->SetRenderFlags((m_renderState | e_DepthWriteOff) & (~e_DepthWriteOn));
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DepthWriteOn()
{
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
    pRenderAuxGeom->SetRenderFlags((m_renderState | e_DepthWriteOn) & (~e_DepthWriteOff));
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::CullOff()
{
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
    pRenderAuxGeom->SetRenderFlags((m_renderState | e_CullModeNone) & (~(e_CullModeBack | e_CullModeFront)));
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::CullOn()
{
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
    pRenderAuxGeom->SetRenderFlags((m_renderState | e_CullModeBack) & (~(e_CullModeNone | e_CullModeFront)));
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
}

//////////////////////////////////////////////////////////////////////////
bool DisplayContext::SetDrawInFrontMode(bool bOn)
{
    int prevState = m_renderState;
    SAuxGeomRenderFlags renderFlags = m_renderState;
    renderFlags.SetDrawInFrontMode((bOn) ? e_DrawInFrontOn : e_DrawInFrontOff);
    pRenderAuxGeom->SetRenderFlags(renderFlags);
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
    return (prevState & e_DrawInFrontOn) != 0;
}

int DisplayContext::SetFillMode(int nFillMode)
{
    int prevState = m_renderState;
    SAuxGeomRenderFlags renderFlags = m_renderState;
    renderFlags.SetFillMode((EAuxGeomPublicRenderflags_FillMode)nFillMode);
    pRenderAuxGeom->SetRenderFlags(renderFlags);
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
    return prevState;
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawTextureLabel(const Vec3& pos, int nWidth, int nHeight, int nTexId, int nTexIconFlags, int srcOffsetX, int srcOffsetY, bool bDistanceScaleIcons, float fDistanceScale)
{
    const float fLabelDepthPrecision = 0.05f;
    Vec3 scrpos = view->WorldToView3D(pos);

    float fWidth = (float)nWidth;
    float fHeight = (float)nHeight;

    if (bDistanceScaleIcons)
    {
        float fScreenScale = view->GetScreenScaleFactor(pos);

        fWidth *= fDistanceScale / fScreenScale;
        fHeight *= fDistanceScale / fScreenScale;
    }

    STextureLabel tl;
    tl.x = scrpos.x + srcOffsetX;
    tl.y = scrpos.y + srcOffsetY;
    if (nTexIconFlags & TEXICON_ALIGN_BOTTOM)
    {
        tl.y -= fHeight / 2;
    }
    else if (nTexIconFlags & TEXICON_ALIGN_TOP)
    {
        tl.y += fHeight / 2;
    }
    tl.z = scrpos.z - (1.0f - scrpos.z) * fLabelDepthPrecision;
    tl.w = fWidth;
    tl.h = fHeight;
    tl.nTexId = nTexId;
    tl.flags = nTexIconFlags;
    tl.color[0] = m_color4b.r * (1.0f / 255.0f);
    tl.color[1] = m_color4b.g * (1.0f / 255.0f);
    tl.color[2] = m_color4b.b * (1.0f / 255.0f);
    tl.color[3] = m_color4b.a * (1.0f / 255.0f);

    // Try to not overflood memory with labels.
    if (m_textureLabels.size() < 100000)
    {
        m_textureLabels.push_back(tl);
    }
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::Flush2D()
{
    if (m_textureLabels.empty())
    {
        return;
    }

    int rcw, rch;
    view->GetDimensions(&rcw, &rch);

    AZ_ErrorOnce(nullptr, false, "DisplayContext::Flush2D needs to be removed/ported to use Atom");
#if 0

    TransformationMatrices backupSceneMatrices;

    renderer->Set2DMode(rcw, rch, backupSceneMatrices, 0.0f, 1.0f);

    renderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);
    //renderer->SetCullMode( R_CULL_NONE );

    float uvs[4], uvt[4];
    uvs[0] = 0;
    uvt[0] = 1;
    uvs[1] = 1;
    uvt[1] = 1;
    uvs[2] = 1;
    uvt[2] = 0;
    uvs[3] = 0;
    uvt[3] = 0;

    const size_t nLabels = m_textureLabels.size();
    for (size_t i = 0; i < nLabels; i++)
    {
        STextureLabel& t = m_textureLabels[i];
        float w2 = t.w * 0.5f;
        float h2 = t.h * 0.5f;
        if (t.flags & TEXICON_ADDITIVE)
        {
            renderer->SetState(GS_BLSRC_ONE | GS_BLDST_ONE);
        }
        else if (t.flags & TEXICON_ON_TOP)
        {
            renderer->SetState(GS_NODEPTHTEST | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);
        }

        renderer->DrawImageWithUV(t.x - w2, t.y + h2, t.z, t.w, -t.h, t.nTexId, uvs, uvt, t.color[0], t.color[1], t.color[2], t.color[3]);

        if (t.flags & (TEXICON_ADDITIVE | TEXICON_ON_TOP)) // Restore state.
        {
            renderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);
        }
    }

    renderer->Unset2DMode(backupSceneMatrices);
#endif

    m_textureLabels.clear();
}
