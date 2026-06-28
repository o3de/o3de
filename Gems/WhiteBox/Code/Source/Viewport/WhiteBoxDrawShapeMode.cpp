/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxDrawShapeMode.h"

#include "WhiteBoxShapeBuilders.h"

#include "EditorWhiteBoxComponentModeTypes.h"
#include "Viewport/WhiteBoxManipulatorBounds.h"
#include "Viewport/WhiteBoxViewportConstants.h"
#include "Util/WhiteBoxMathUtil.h"

#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Math/Plane.h>
#include <AzCore/Math/MathStringConversions.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Maths/TransformUtils.h>
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>
#include <WhiteBox/WhiteBoxToolApi.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>

#include <AzFramework/Render/GeometryIntersectionStructures.h>
#include <AzFramework/Visibility/EntityVisibilityBoundsUnionSystem.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <AzFramework/Render/Intersector.h>           // IntersectorBus / IntersectorInterface
#include <AzFramework/Render/GeometryIntersectionStructures.h>  // RayRequest / RayResult
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>
#include <AzToolsFramework/API/ComponentModeCollectionInterface.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/containers/array.h>
#include "EditorWhiteBoxComponent.h"
#include <cmath>


namespace WhiteBox
{
    AZ_CLASS_ALLOCATOR_IMPL(DrawShapeMode, AZ::SystemAllocator)
    // Helper: build a right-handed basis with `up` as the Z-equivalent (the normal).
    void Detail::BasisFromNormal(const AZ::Vector3& n, AZ::Vector3& right, AZ::Vector3& fwd, AZ::Vector3& up)
    {
        up = n.GetNormalized();

        const AZ::Vector3 ref = (AZStd::abs(up.GetZ()) < 0.99f)
            ? AZ::Vector3::CreateAxisZ()
            : AZ::Vector3::CreateAxisY();   // use Y (not X) so a vertical normal still gives a stable frame

        right = ref.Cross(up).GetNormalized();
        fwd   = up.Cross(right).GetNormalized();
    }

    // Re-assign the in-plane axes for a staircase so the SAME drawn footprint is
    // re-oriented in 90-degree steps: which axis is the run (ascent) and which way
    // it climbs. The footprint rectangle (and its centre) is unchanged; only the
    // roles/signs of the axes change, so 1 = swap run/width, 2 = flip the climb to
    // the first-clicked corner, 3 = swap + flip.
    void Detail::ApplyStairRotation(AZ::Vector3& uAxis, AZ::Vector3& vAxis, int rotation)
    {
        const int k = ((rotation % 4) + 4) % 4;
        const AZ::Vector3 u = uAxis;
        const AZ::Vector3 v = vAxis;
        switch (k)
        {
        case 1: uAxis =  v; vAxis =  u; break;
        case 2: uAxis =  u; vAxis = -v; break;
        case 3: uAxis =  v; vAxis = -u; break;
        default: break; // 0
        }
    }

    const char* Detail::DrawShapeName(DrawShapeType shape)
    {
        switch (shape)
        {
        case DrawShapeType::Box:       return "Box";
        case DrawShapeType::Cylinder:  return "Cylinder";
        case DrawShapeType::Pyramid:   return "Pyramid";
        case DrawShapeType::Cone:      return "Cone";
        case DrawShapeType::Sphere:    return "Sphere";
        case DrawShapeType::Staircase: return "Staircase";
        default:                       return "Shape";
        }
    }

    // Add one (convex, planar) face from an ordered loop of vertex handles,
    // fan-triangulated and wound so its normal points AWAY from the solid centre
    // (outward). Robust for any convex polygon - no manual per-shape winding.
    void Detail::AddOutwardFace(
        WhiteBoxMesh& whiteBox,
        const AZStd::vector<Api::VertexHandle>& handles,
        const AZStd::vector<AZ::Vector3>& localPositions,
        const AZStd::vector<AZ::u32>& loop,
        const AZ::Vector3& solidCentroid)
    {
        if (loop.size() < 3)
        {
            return;
        }

        AZ::Vector3 faceCentroid = AZ::Vector3::CreateZero();
        for (const AZ::u32 idx : loop)
        {
            faceCentroid += localPositions[idx];
        }
        faceCentroid /= aznumeric_cast<float>(loop.size());

        const AZ::Vector3 a = localPositions[loop[0]];
        const AZ::Vector3 b = localPositions[loop[1]];
        const AZ::Vector3 c = localPositions[loop[2]];
        const AZ::Vector3 normal = (b - a).Cross(c - a);
        const bool flip = normal.Dot(faceCentroid - solidCentroid) < 0.0f;

        Api::FaceVertHandlesList faces;
        faces.reserve(loop.size() - 2);
        for (size_t i = 1; i + 1 < loop.size(); ++i)
        {
            Api::VertexHandle v0 = handles[loop[0]];
            Api::VertexHandle v1 = handles[loop[i]];
            Api::VertexHandle v2 = handles[loop[i + 1]];
            if (flip)
            {
                AZStd::swap(v1, v2);
            }
            faces.push_back(Api::FaceVertHandles{ {v0, v1, v2} });
        }
        Api::AddPolygon(whiteBox, faces);
    }

    // Add a round cap as one polygon, triangulated as an "umbrella" from a centre
    // vertex (uniform triangles, no slivers - unlike a corner fan). The centre and
    // the spokes are interior to the polygon, so they don't render as edges; the
    // cap still reads as a clean disc but has good underlying topology.
    void Detail::AddOutwardCap(
        WhiteBoxMesh& whiteBox,
        const AZStd::vector<Api::VertexHandle>& handles,
        const AZStd::vector<AZ::Vector3>& localPositions,
        const AZStd::vector<AZ::u32>& rim,
        const AZ::u32 centerIdx,
        const AZ::Vector3& solidCentroid)
    {
        const size_t n = rim.size();
        if (n < 3)
        {
            return;
        }

        const AZ::Vector3 a = localPositions[centerIdx];
        const AZ::Vector3 b = localPositions[rim[0]];
        const AZ::Vector3 c = localPositions[rim[1]];
        const AZ::Vector3 normal = (b - a).Cross(c - a);
        const bool flip = normal.Dot(localPositions[centerIdx] - solidCentroid) < 0.0f;

        Api::FaceVertHandlesList faces;
        faces.reserve(n);
        for (size_t i = 0; i < n; ++i)
        {
            Api::VertexHandle v0 = handles[centerIdx];
            Api::VertexHandle v1 = handles[rim[i]];
            Api::VertexHandle v2 = handles[rim[(i + 1) % n]];
            if (flip)
            {
                AZStd::swap(v1, v2);
            }
            faces.push_back(Api::FaceVertHandles{ {v0, v1, v2} });
        }
        Api::AddPolygon(whiteBox, faces);
    }

    // Generate the N-gon footprint ring (world space) for the current shape:
    // round shapes inscribe the ellipse; angular shapes fill the rectangle (4 =
    // exact corners). ru/rv are the half-extent vectors along the two in-plane axes.
    AZStd::vector<AZ::Vector3> Detail::ComputeFootprintRing(
        const AZ::Vector3& center, const AZ::Vector3& ru, const AZ::Vector3& rv, const bool round, const int sidesIn)
    {
        const int sides = AZ::GetClamp(sidesIn, 3, 256);
        AZStd::vector<AZ::Vector3> ring;
        ring.reserve(sides);
        if (round)
        {
            for (int i = 0; i < sides; ++i)
            {
                const float t = (AZ::Constants::TwoPi * static_cast<float>(i)) / static_cast<float>(sides);
                ring.push_back(center + ru * std::cos(t) + rv * std::sin(t));
            }
        }
        else
        {
            const float phase = AZ::Constants::Pi / static_cast<float>(sides);
            float maxX = 0.0f, maxY = 0.0f;
            for (int i = 0; i < sides; ++i)
            {
                const float t = phase + (AZ::Constants::TwoPi * static_cast<float>(i)) / static_cast<float>(sides);
                maxX = AZ::GetMax(maxX, std::abs(std::cos(t)));
                maxY = AZ::GetMax(maxY, std::abs(std::sin(t)));
            }
            for (int i = 0; i < sides; ++i)
            {
                const float t = phase + (AZ::Constants::TwoPi * static_cast<float>(i)) / static_cast<float>(sides);
                ring.push_back(center + ru * (std::cos(t) / maxX) + rv * (std::sin(t) / maxY));
            }
        }
        return ring;
    }

    // Build a watertight stepped "staircase" solid that rises along +v (the drawn
    // footprint's second axis) and spans the vertical range [baseUp, topUp].
    //
    // Unlike a naive stack of boxes, only the OUTER shell is emitted - bottom, back,
    // each step's tread + riser, and two stepped side caps - so there are no internal
    // walls between steps and the result reads as one merged solid. Every vertex lies
    // on an (N+1) x (N+1) grid and is shared between faces (created on demand), which
    // also avoids T-junctions, keeping the topology clean and manifold.
    void Detail::BuildStaircaseSolid(
        WhiteBoxMesh& mesh, const AZ::Transform& localFromWorld,
        const AZ::Vector3& center, const AZ::Vector3& uAxis, const AZ::Vector3& vAxis, const AZ::Vector3& up,
        const float baseUp, const float topUp, const int stepsIn)
    {
        const int N = AZ::GetClamp(stepsIn, 1, 256);

        const AZ::Vector3 origin = center - uAxis * 0.5f - vAxis * 0.5f + up * baseUp;
        const float totalRise = topUp - baseUp;                 // signed: follows the pull direction
        const float signR = (totalRise >= 0.0f) ? 1.0f : -1.0f; // up/down orientation of the steps

        const AZ::Vector3 uHat  = uAxis.GetNormalizedSafe();
        const AZ::Vector3 vHat  = vAxis.GetNormalizedSafe();
        const AZ::Vector3 upHat = up.GetNormalizedSafe();

        // Shared vertex grid, indexed by (uSide, vi, hi) and created on demand.
        AZStd::vector<AZ::Vector3> localPos;
        AZStd::vector<Api::VertexHandle> vh;
        const int stride = N + 1;
        AZStd::vector<int> vertIndex(static_cast<size_t>(2) * stride * stride, -1);

        const auto worldPos = [&](int uSide, int vi, int hi) -> AZ::Vector3
        {
            return origin + uAxis * static_cast<float>(uSide)
                + vAxis * (static_cast<float>(vi) / static_cast<float>(N))
                + up * (totalRise * static_cast<float>(hi) / static_cast<float>(N));
        };
        const auto getV = [&](int uSide, int vi, int hi) -> AZ::u32
        {
            const int key = (uSide * stride + vi) * stride + hi;
            if (vertIndex[key] < 0)
            {
                const AZ::Vector3 local = localFromWorld.TransformPoint(worldPos(uSide, vi, hi));
                vertIndex[key] = aznumeric_cast<int>(vh.size());
                localPos.push_back(local);
                vh.push_back(Api::AddVertex(mesh, local));
            }
            return aznumeric_cast<AZ::u32>(vertIndex[key]);
        };

        // Add one convex polygon (ordered grid points) wound so its outward normal
        // matches @p outwardWorld. Fan-triangulated, emitted as a single white box
        // polygon so the internal triangulation edges are not shown.
        const auto addPoly =
            [&](const AZStd::vector<AZStd::array<int, 3>>& pts, const AZ::Vector3& outwardWorld)
        {
            AZStd::vector<AZ::u32> loop;
            loop.reserve(pts.size());
            for (const auto& p : pts)
            {
                loop.push_back(getV(p[0], p[1], p[2]));
            }
            const AZ::Vector3 a = localPos[loop[0]];
            const AZ::Vector3 b = localPos[loop[1]];
            const AZ::Vector3 c = localPos[loop[2]];
            const AZ::Vector3 nrm = (b - a).Cross(c - a);
            const AZ::Vector3 outLocal = AzToolsFramework::TransformDirectionNoScaling(localFromWorld, outwardWorld);
            const bool flip = nrm.Dot(outLocal) < 0.0f;

            Api::FaceVertHandlesList faces;
            faces.reserve(loop.size() - 2);
            for (size_t i = 1; i + 1 < loop.size(); ++i)
            {
                Api::VertexHandle v0 = vh[loop[0]];
                Api::VertexHandle v1 = vh[loop[i]];
                Api::VertexHandle v2 = vh[loop[i + 1]];
                if (flip)
                {
                    AZStd::swap(v1, v2);
                }
                faces.push_back(Api::FaceVertHandles{ { v0, v1, v2 } });
            }
            Api::AddPolygon(mesh, faces);
        };

        for (int i = 0; i < N; ++i)
        {
            // Stepped side caps. The left edge of each cap (i >= 1) is split at (i,i)
            // so it matches the neighbouring cap column below + the riser above (no
            // T-junction). Ordered from the bottom-right corner so the fan never makes
            // a degenerate (collinear) triangle.
            AZStd::vector<AZStd::array<int, 3>> capL, capR;
            if (i == 0)
            {
                capL = { {0, 1, 0}, {0, 1, 1}, {0, 0, 1}, {0, 0, 0} };
                capR = { {1, 1, 0}, {1, 1, 1}, {1, 0, 1}, {1, 0, 0} };
            }
            else
            {
                capL = { {0, i + 1, 0}, {0, i + 1, i + 1}, {0, i, i + 1}, {0, i, i}, {0, i, 0} };
                capR = { {1, i + 1, 0}, {1, i + 1, i + 1}, {1, i, i + 1}, {1, i, i}, {1, i, 0} };
            }
            addPoly(capL, -uHat);
            addPoly(capR, uHat);

            // Bottom slab under this column (outward away from the steps).
            addPoly({ {0, i, 0}, {1, i, 0}, {1, i + 1, 0}, {0, i + 1, 0} }, upHat * (-signR));
            // Tread (the top surface of this step).
            addPoly({ {0, i, i + 1}, {1, i, i + 1}, {1, i + 1, i + 1}, {0, i + 1, i + 1} }, upHat * signR);
            // Riser (the vertical front face of this step).
            addPoly({ {0, i, i}, {1, i, i}, {1, i, i + 1}, {0, i, i + 1} }, -vHat);
        }

        // Back face (rear of the tallest step).
        addPoly({ {0, N, 0}, {1, N, 0}, {1, N, N}, {0, N, N} }, vHat);
    }

    // Build a UV ellipsoid ("sphere") inscribed in the drawn footprint: in-plane
    // radii from uAxis/vAxis, vertical radius from the [baseUp, topUp] span. The
    // resolution is driven by @p segments (longitude); latitude rings derive from it.
    void Detail::BuildSphereSolid(
        WhiteBoxMesh& mesh, const AZ::Transform& localFromWorld,
        const AZ::Vector3& center, const AZ::Vector3& uAxis, const AZ::Vector3& vAxis, const AZ::Vector3& up,
        const float baseUp, const float topUp, const int segmentsIn)
    {
        const int segments = AZ::GetClamp(segmentsIn, 3, 256);          // longitude (around up)
        const int rings    = AZ::GetClamp(segmentsIn / 2, 2, 128);     // latitude (pole to pole)

        const AZ::Vector3 uHat = uAxis.GetNormalizedSafe();
        const AZ::Vector3 vHat = vAxis.GetNormalizedSafe();
        const AZ::Vector3 wHat = up.GetNormalizedSafe();

        const float a = uAxis.GetLength() * 0.5f;            // radius along u
        const float b = vAxis.GetLength() * 0.5f;            // radius along v
        const float c = (topUp - baseUp) * 0.5f;             // radius along up (signed span ok)
        const AZ::Vector3 sphereCenter = center + up * ((baseUp + topUp) * 0.5f);

        // Direction on the unit sphere for ring i (latitude) and segment j (longitude).
        const auto dirAt = [&](int i, int j) -> AZ::Vector3
        {
            const float phi = -AZ::Constants::Pi * 0.5f +
                AZ::Constants::Pi * (static_cast<float>(i) / static_cast<float>(rings)); // -pi/2 .. +pi/2
            const float lon = AZ::Constants::TwoPi * (static_cast<float>(j) / static_cast<float>(segments));
            const float cosPhi = std::cos(phi);
            return sphereCenter
                + uHat * (a * std::cos(lon) * cosPhi)
                + vHat * (b * std::sin(lon) * cosPhi)
                + wHat * (c * std::sin(phi));
        };

        // Build a (rings+1) x segments grid of vertices (pole rows duplicated).
        AZStd::vector<AZ::Vector3> localPos;
        AZStd::vector<Api::VertexHandle> vh;
        localPos.reserve(static_cast<size_t>(rings + 1) * segments);
        vh.reserve(static_cast<size_t>(rings + 1) * segments);

        const auto addVertex = [&](const AZ::Vector3& world) -> AZ::u32
        {
            const AZ::Vector3 local = localFromWorld.TransformPoint(world);
            localPos.push_back(local);
            vh.push_back(Api::AddVertex(mesh, local));
            return aznumeric_cast<AZ::u32>(vh.size() - 1);
        };

        AZStd::vector<AZStd::vector<AZ::u32>> grid;
        grid.resize(rings + 1);
        for (int i = 0; i <= rings; ++i)
        {
            grid[i].resize(segments);
            for (int j = 0; j < segments; ++j)
            {
                grid[i][j] = addVertex(dirAt(i, j));
            }
        }

        const AZ::Vector3 localCenter = localFromWorld.TransformPoint(sphereCenter);

        for (int i = 0; i < rings; ++i)
        {
            for (int j = 0; j < segments; ++j)
            {
                const int jn = (j + 1) % segments;
                if (i == 0)
                {
                    // South pole cap -> triangle.
                    const AZStd::vector<AZ::u32> tri = { grid[0][j], grid[1][jn], grid[1][j] };
                    AddOutwardFace(mesh, vh, localPos, tri, localCenter);
                }
                else if (i == rings - 1)
                {
                    // North pole cap -> triangle.
                    const AZStd::vector<AZ::u32> tri = { grid[i][j], grid[i][jn], grid[i + 1][j] };
                    AddOutwardFace(mesh, vh, localPos, tri, localCenter);
                }
                else
                {
                    const AZStd::vector<AZ::u32> quad = { grid[i][j], grid[i][jn], grid[i + 1][jn], grid[i + 1][j] };
                    AddOutwardFace(mesh, vh, localPos, quad, localCenter);
                }
            }
        }
    }

    // Build a closed shape solid into @p mesh from a footprint in the surface
    // plane (centre + in-plane axes uAxis/vAxis) extruded along @p up between the
    // base plane (centre + up*baseUp) and the top plane / apex (centre + up*topUp).
    // Shared by the draw-commit (visible shape) and the carve/add cutter so they
    // always produce the same geometry. All faces are wound outward.
    void Detail::BuildShapeSolid(
        WhiteBoxMesh& mesh, const AZ::Transform& localFromWorld,
        const AZ::Vector3& center, const AZ::Vector3& uAxis, const AZ::Vector3& vAxis, const AZ::Vector3& up,
        const float baseUp, const float topUp, const DrawShapeType shapeType, const int sidesIn, const int steps)
    {
        // Sphere and Staircase have dedicated builders (not prism/pyramid based).
        if (shapeType == DrawShapeType::Sphere)
        {
            BuildSphereSolid(mesh, localFromWorld, center, uAxis, vAxis, up, baseUp, topUp, sidesIn);
            return;
        }
        if (shapeType == DrawShapeType::Staircase)
        {
            BuildStaircaseSolid(mesh, localFromWorld, center, uAxis, vAxis, up, baseUp, topUp, steps);
            return;
        }

        const bool pointed = (shapeType == DrawShapeType::Pyramid || shapeType == DrawShapeType::Cone);
        const bool round   = (shapeType == DrawShapeType::Cylinder || shapeType == DrawShapeType::Cone);
        const int  sides   = AZ::GetClamp(sidesIn, 3, 256);

        const AZ::Vector3 ru = uAxis * 0.5f;
        const AZ::Vector3 rv = vAxis * 0.5f;
        const AZ::Vector3 baseCenter = center + up * baseUp;
        const AZ::Vector3 topPos     = center + up * topUp;   // top-cap centre OR apex
        const AZ::Vector3 ext        = up * (topUp - baseUp);

        // footprint ring at the base plane
        const AZStd::vector<AZ::Vector3> baseWorld = Detail::ComputeFootprintRing(baseCenter, ru, rv, round, sides);

        const size_t n = baseWorld.size();
        const bool centerCap = (n > 4);

        AZStd::vector<AZ::Vector3> localPos;
        AZStd::vector<Api::VertexHandle> vh;
        localPos.reserve(n * 2 + 2);
        vh.reserve(n * 2 + 2);

        const auto addVertex = [&](const AZ::Vector3& world) -> AZ::u32
        {
            const AZ::Vector3 local = localFromWorld.TransformPoint(world);
            localPos.push_back(local);
            vh.push_back(Api::AddVertex(mesh, local));
            return aznumeric_cast<AZ::u32>(vh.size() - 1);
        };

        AZStd::vector<AZ::u32> baseRing;
        baseRing.reserve(n);
        for (const AZ::Vector3& w : baseWorld) { baseRing.push_back(addVertex(w)); }

        if (pointed)
        {
            const AZ::u32 apex = addVertex(topPos);
            const AZ::u32 baseCenterIdx = centerCap ? addVertex(baseCenter) : 0u;

            AZ::Vector3 centroid = AZ::Vector3::CreateZero();
            for (const AZ::Vector3& p : localPos) { centroid += p; }
            centroid /= aznumeric_cast<float>(localPos.size());

            if (centerCap) { AddOutwardCap(mesh, vh, localPos, baseRing, baseCenterIdx, centroid); }
            else           { AddOutwardFace(mesh, vh, localPos, baseRing, centroid); }
            for (size_t i = 0; i < n; ++i)
            {
                const AZStd::vector<AZ::u32> tri = { baseRing[i], baseRing[(i + 1) % n], apex };
                AddOutwardFace(mesh, vh, localPos, tri, centroid);
            }
        }
        else
        {
            AZStd::vector<AZ::u32> topRing;
            topRing.reserve(n);
            for (const AZ::Vector3& w : baseWorld) { topRing.push_back(addVertex(w + ext)); }
            const AZ::u32 baseCenterIdx = centerCap ? addVertex(baseCenter)       : 0u;
            const AZ::u32 topCenterIdx  = centerCap ? addVertex(baseCenter + ext) : 0u;

            AZ::Vector3 centroid = AZ::Vector3::CreateZero();
            for (const AZ::Vector3& p : localPos) { centroid += p; }
            centroid /= aznumeric_cast<float>(localPos.size());

            if (centerCap)
            {
                AddOutwardCap(mesh, vh, localPos, baseRing, baseCenterIdx, centroid);
                AddOutwardCap(mesh, vh, localPos, topRing,  topCenterIdx,  centroid);
            }
            else
            {
                AddOutwardFace(mesh, vh, localPos, baseRing, centroid);
                AddOutwardFace(mesh, vh, localPos, topRing,  centroid);
            }
            for (size_t i = 0; i < n; ++i)
            {
                const size_t j = (i + 1) % n;
                const AZStd::vector<AZ::u32> quad = { baseRing[i], baseRing[j], topRing[j], topRing[i] };
                AddOutwardFace(mesh, vh, localPos, quad, centroid);
            }
        }
    }

    DrawShapeMode::DrawShapeMode(const AZ::EntityComponentIdPair& entityComponentIdPair)
        : m_entityComponentIdPair(entityComponentIdPair)
    {
        // Connect to the global viewport rendering bus
        AzFramework::ViewportDebugDisplayEventBus::Handler::BusConnect(AzToolsFramework::GetEntityContextId());
        // Receive numeric-depth keyboard actions dispatched to this component.
        EditorWhiteBoxDrawShapeModeRequestBus::Handler::BusConnect(entityComponentIdPair);
    }
    DrawShapeMode::~DrawShapeMode()
    {
        // Disconnect from the bus when mode is exited
        EditorWhiteBoxDrawShapeModeRequestBus::Handler::BusDisconnect();
        AzFramework::ViewportDebugDisplayEventBus::Handler::BusDisconnect();
    }
    AZ::Vector3 DrawShapeMode::RaycastToSurface(
        const AzToolsFramework::ViewportInteraction::MouseInteraction& mouseInteraction,
        const AZ::Transform& worldFromLocal,
        const IntersectionAndRenderData& intersectionData,
        AZ::Vector3& outWorldNormal) const
    {
        const AZ::Vector3 rayOriginWorld = mouseInteraction.m_mousePick.m_rayOrigin;
        const AZ::Vector3 rayDirWorld    = mouseInteraction.m_mousePick.m_rayDirection;

        // Track the closest hit across every surface type so white box meshes,
        // other white box meshes, and ordinary scene geometry are all candidates
        // and all use a consistent flat (per-triangle) surface normal.
        float bestDist = AZStd::numeric_limits<float>::max();
        AZ::Vector3 bestHit = AZ::Vector3::CreateZero();
        AZ::Vector3 bestNormal = AZ::Vector3::CreateAxisZ();
        bool found = false;

        const auto consider = [&](const AZ::Vector3& worldHit, AZ::Vector3 worldNormal)
        {
            const float d = (worldHit - rayOriginWorld).Dot(rayDirWorld); // distance along the ray
            if (d < 0.0f || d >= bestDist)
            {
                return;
            }
            bestDist = d;
            bestHit = worldHit;
            worldNormal = worldNormal.GetNormalizedSafe();
            if (worldNormal.Dot(rayDirWorld) > 0.0f) // face back toward the camera
            {
                worldNormal = -worldNormal;
            }
            bestNormal = worldNormal;
            found = true;
        };

        // Ray-test a white box mesh's local triangles (flat normals) and report hits.
        const auto raycastWhiteBox = [&](WhiteBoxMesh& mesh, const AZ::Transform& meshWorldFromLocal)
        {
            const AZ::Transform localFromWorld = meshWorldFromLocal.GetInverse();
            const AZ::Vector3 lo = localFromWorld.TransformPoint(rayOriginWorld);
            const AZ::Vector3 ld = AzToolsFramework::TransformDirectionNoScaling(localFromWorld, rayDirWorld);
            const float rayLen = 100000.0f;
            const AZ::Vector3 le = lo + ld * rayLen;

            AZ::Intersect::SegmentTriangleHitTester hitTester(lo, le);
            for (const Api::Face& f : Api::MeshFaces(mesh))
            {
                float t = 0.0f;
                AZ::Vector3 n;
                if (hitTester.IntersectSegmentTriangle(f[0], f[1], f[2], n, t))
                {
                    const AZ::Vector3 localHit = lo + (le - lo) * t;
                    consider(
                        meshWorldFromLocal.TransformPoint(localHit),
                        AzToolsFramework::TransformDirectionNoScaling(meshWorldFromLocal, n));
                }
            }
        };

        // 1. the current white box mesh (its precomputed intersection data, flat normal)
        {
            const AZ::Transform localFromWorld = worldFromLocal.GetInverse();
            const AZ::Vector3 lo = localFromWorld.TransformPoint(rayOriginWorld);
            const AZ::Vector3 ld = AzToolsFramework::TransformDirectionNoScaling(localFromWorld, rayDirWorld);

            for (const auto& polyBound : intersectionData.m_whiteBoxIntersectionData.m_polygonBounds)
            {
                float dist = AZStd::numeric_limits<float>::max();
                int64_t triIdx = 0;
                if (IntersectRayPolygon(polyBound.m_bound, lo, ld, dist, triIdx))
                {
                    const AZ::Vector3 localHit = lo + ld * dist;
                    AZ::Vector3 localNormal = AZ::Vector3::CreateAxisZ();
                    const auto& tris = polyBound.m_bound.m_triangles;
                    const size_t base = static_cast<size_t>(triIdx) * 3;
                    if (base + 2 < tris.size())
                    {
                        localNormal =
                            (tris[base + 1] - tris[base + 0]).Cross(tris[base + 2] - tris[base + 0]).GetNormalizedSafe();
                    }
                    consider(
                        worldFromLocal.TransformPoint(localHit),
                        AzToolsFramework::TransformDirectionNoScaling(worldFromLocal, localNormal));
                }
            }
        }

        // 2. every OTHER white box mesh in the scene (not in the scene intersector)
        {
            const AZ::EntityId selfEntity = m_entityComponentIdPair.GetEntityId();
            AZ::ComponentApplicationBus::Broadcast(
                &AZ::ComponentApplicationRequests::EnumerateEntities,
                [&](AZ::Entity* entity)
                {
                    if (entity == nullptr || entity->GetId() == selfEntity)
                    {
                        return;
                    }
                    for (EditorWhiteBoxComponent* comp : entity->FindComponents<EditorWhiteBoxComponent>())
                    {
                        WhiteBoxMesh* mesh = comp->GetWhiteBoxMesh();
                        if (mesh == nullptr)
                        {
                            continue;
                        }
                        AZ::Transform worldTM = AZ::Transform::CreateIdentity();
                        AZ::TransformBus::EventResult(worldTM, entity->GetId(), &AZ::TransformBus::Events::GetWorldTM);
                        raycastWhiteBox(*mesh, worldTM);
                    }
                });
        }

        // 3. ordinary scene render geometry (non white box)
        {
            // Cast a parallel ray offset by `offset`; returns true + the world hit.
            const auto castScene = [&](const AZ::Vector3& offset, AZ::Vector3& outHit) -> bool
            {
                AzFramework::RenderGeometry::RayRequest request;
                request.m_startWorldPosition = rayOriginWorld + offset;
                request.m_endWorldPosition   = rayOriginWorld + offset + rayDirWorld * 100000.0f;
                request.m_onlyVisible        = true;

                AzFramework::RenderGeometry::RayResult rayResult;
                AzFramework::RenderGeometry::IntersectorBus::EventResult(
                    rayResult, AzToolsFramework::GetEntityContextId(),
                    &AzFramework::RenderGeometry::IntersectorInterface::RayIntersect, request);
                if (rayResult)
                {
                    outHit = rayResult.m_worldPosition;
                    return true;
                }
                return false;
            };

            AZ::Vector3 hit0;
            if (castScene(AZ::Vector3::CreateZero(), hit0))
            {
                // Derive the surface normal from world-space positions of two nearby
                // parallel rays. This reflects the true surface orientation regardless
                // of how the intersector reports its normal (which can come back in
                // model space, so it ignores the entity's rotation).
                const float dist = (hit0 - rayOriginWorld).GetLength();
                const float eps = AZ::GetMax(0.01f, dist * 0.004f);

                AZ::Vector3 perpA = rayDirWorld.Cross(AZ::Vector3::CreateAxisX());
                if (perpA.GetLengthSq() < 1e-6f)
                {
                    perpA = rayDirWorld.Cross(AZ::Vector3::CreateAxisY());
                }
                perpA.Normalize();
                const AZ::Vector3 perpB = rayDirWorld.Cross(perpA).GetNormalizedSafe();

                AZ::Vector3 hitA, hitB;
                AZ::Vector3 sceneNormal = AZ::Vector3::CreateAxisZ();
                if (castScene(perpA * eps, hitA) && castScene(perpB * eps, hitB))
                {
                    const AZ::Vector3 n = (hitA - hit0).Cross(hitB - hit0);
                    if (n.GetLengthSq() > 1e-10f)
                    {
                        sceneNormal = n.GetNormalizedSafe();
                    }
                    else
                    {
                        sceneNormal = -rayDirWorld; // degenerate (e.g. grazing) - face the camera
                    }
                }
                else
                {
                    sceneNormal = -rayDirWorld; // offset rays missed (near an edge) - face the camera
                }

                consider(hit0, sceneNormal);
            }
        }

        if (found)
        {
            outWorldNormal = bestNormal;
            return bestHit;
        }

        // 4. ground plane fallback
        outWorldNormal = AZ::Vector3::CreateAxisZ();
        const AZ::Vector3 planePos(0.f, 0.f, m_groundZ);
        const AZ::Vector3 planeNormal = AZ::Vector3::CreateAxisZ();
        float t = 0.f;
        AZ::Intersect::IntersectRayPlane(rayOriginWorld, rayDirWorld, planePos, planeNormal, t);
        if (t < 0.f) { t = 0.f; }
        return rayOriginWorld + rayDirWorld * t;
    }
    
    

    float DrawShapeMode::RaycastToHeightPlane(
        const AzToolsFramework::ViewportInteraction::MouseInteraction& mouseInteraction,
        [[maybe_unused]] const AZ::Transform& worldFromLocal,
        const AZ::Vector3& baseCenterWorld) const
    {
        const AZ::Vector3 rayOriginWorld = mouseInteraction.m_mousePick.m_rayOrigin;
        const AZ::Vector3 rayDirWorld    = mouseInteraction.m_mousePick.m_rayDirection;
    
        const AZ::Vector3 up = m_surfaceNormal.GetNormalized();
    
        // A fallback in-plane vector, perpendicular to up.
        const AZ::Vector3 ref = (AZStd::abs(up.GetZ()) < 0.99f)
            ? AZ::Vector3::CreateAxisZ()
            : AZ::Vector3::CreateAxisX();
        const AZ::Vector3 inPlane = ref.Cross(up).GetNormalized();
    
        // Plane that contains `up` and faces the camera: remove the up-component of the ray dir.
        AZ::Vector3 planeNormal = rayDirWorld - up * rayDirWorld.Dot(up);
        if (planeNormal.GetLengthSq() < 0.0001f)
        {
            planeNormal = inPlane;
        }
        planeNormal.Normalize();
    
        float t = 0.f;
        AZ::Intersect::IntersectRayPlane(rayOriginWorld, rayDirWorld, baseCenterWorld, planeNormal, t);
    
        if (t <= 0.f)
        {
            return m_height; // keep current height rather than snapping
        }
    
        const AZ::Vector3 hitWorld = rayOriginWorld + rayDirWorld * t;
        const float heightDelta = (hitWorld - baseCenterWorld).Dot(up); // signed
        return heightDelta;
    }

    bool DrawShapeMode::HandleMouseInteraction(const ModeMouseInteraction& mouse)
    {
        using MouseEvent = AzToolsFramework::ViewportInteraction::MouseEvent;

        const auto& mouseInteraction = mouse.m_mouseInteraction;
        const AZ::Transform& worldFromLocal = mouse.m_worldFromLocal;
        const IntersectionAndRenderData& intersectionData = mouse.m_intersectionData;

        // Cache so a keyboard-driven numeric confirm (which carries no transform)
        // can commit using the current frame's transform.
        m_worldFromLocal = worldFromLocal;

        const auto& mi     = mouseInteraction.m_mouseInteraction;
        const bool  leftDown  = mi.m_mouseButtons.Left() && mouseInteraction.m_mouseEvent == MouseEvent::Down;
        const bool  leftUp    = mi.m_mouseButtons.Left() && mouseInteraction.m_mouseEvent == MouseEvent::Up;
        const bool  rightDown = mi.m_mouseButtons.Right() && mouseInteraction.m_mouseEvent == MouseEvent::Down;
        const bool  moved     = mouseInteraction.m_mouseEvent == MouseEvent::Move;

        // Unit-cube stamp mode: single click places/removes a grid-snapped cube;
        // no click-drag-pull state machine.
        if (UnitCubeMode())
        {
            const bool carve = mi.m_keyboardModifiers.Ctrl() || CurrentCarve();
            m_unitCubeCarve = carve;
            if (moved || leftDown)
            {
                AZ::Vector3 hitNormal;
                const AZ::Vector3 hitWorld = RaycastToSurface(mi, worldFromLocal, intersectionData, hitNormal);
                m_unitCubeHoverValid = UnitCubeCell(worldFromLocal, hitWorld, hitNormal, carve, m_unitCubeMinLocal);

                if (leftDown)
                {
                    StampUnitCube(worldFromLocal, hitWorld, hitNormal, carve);
                    return true;
                }
            }
            return false; // let moves/other buttons pass through (camera etc.)
        }
        m_unitCubeHoverValid = false;

        if (rightDown)
        {
            Cancel();
            return m_state != DrawState::Idle;
        }

        switch (m_state)
        {
        case DrawState::Idle:
        {
            if (leftDown)
            {
                // Ctrl+click = carve a hole through the polygon under the cursor.
                m_carveMode = mi.m_keyboardModifiers.Ctrl() || CurrentCarve();   // Ctrl OR the Carve toggle
                AZ::Vector3 hitNormal;
                const AZ::Vector3 hitWorld = RaycastToSurface(mi, worldFromLocal, intersectionData, hitNormal);
                m_surfaceNormal = hitNormal;            // remember the surface orientation
                m_groundZ = hitWorld.GetZ();
                m_worldP0 = hitWorld;
                m_worldP1 = hitWorld;
                m_height  = 0.f;
                m_state   = DrawState::DraggingBase;
                return true;
            }
            return false;
        }

        case DrawState::DraggingBase:
        {
            if (moved)
            {
                AZ::Vector3 dummyNormal;
                const AZ::Vector3 rawHit = RaycastToSurface(mi, worldFromLocal, intersectionData, dummyNormal);

                // Project the raw hit onto the anchor's surface plane so the base rectangle
                // always lies flat on that surface — works for horizontal, vertical, or tilted.
                const AZ::Vector3 up = m_surfaceNormal.GetNormalized();
                const float distFromPlane = (rawHit - m_worldP0).Dot(up);
                m_worldP1 = rawHit - up * distFromPlane;

                return true;
            }
            if (leftUp)
            {
                // (no SetZ — P1 is already on the surface plane from the move handler)
                const AZ::Vector3 baseDelta = m_worldP1 - m_worldP0;

                // Reject a near-zero base measured IN THE SURFACE PLANE, not by world XZ.
                if (baseDelta.GetLength() < cl_whiteBoxMouseClickDeltaThreshold)
                {
                    Cancel();
                    return false;
                }

                m_height = 0.f;
                m_state  = DrawState::PullingHeight;
                return true;
            }
            return true;
        }

        case DrawState::PullingHeight:
        {
            if (moved)
            {
                // Once the depth is being typed, the mouse no longer drives it.
                if (!m_numericInput.IsActive())
                {
                    const AZ::Vector3 baseCenterWorld = (m_worldP0 + m_worldP1) * 0.5f;
                    m_height = RaycastToHeightPlane(mi, worldFromLocal, baseCenterWorld);
                }
                return true;
            }
            if (leftDown)
            {
                // A click also commits (using the typed depth if numeric is active).
                m_numericInput.Reset();

                if (AZStd::abs(m_height) < cl_whiteBoxMouseClickDeltaThreshold)
                {
                    Cancel();
                    return false;
                }

                if (m_carveMode)
                {
                    // Ctrl + draw = CSG boolean. Pull direction decides:
                    // pull in -> carve (subtract), pull out -> add (union).
                    BooleanAtPolygon(worldFromLocal, m_height);
                }
                else
                {
                    CommitBox(worldFromLocal);
                }

                m_state = DrawState::Idle;
                return true;
            }
            return true;
        }
        }

        return false;
    }
    
    

    void DrawShapeMode::CommitBox(const AZ::Transform& worldFromLocal)
    {
        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair,
            &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        if (!whiteBox)
        {
            return;
        }

        AzToolsFramework::ScopedUndoBatch undoBatch("Draw White Box Shape");

        // Surface basis (up = outward normal of the surface drawn on).
        AZ::Vector3 right, fwd, up;
        Detail::BasisFromNormal(m_surfaceNormal, right, fwd, up);

        // Drag extents in the surface plane.
        const AZ::Vector3 drag = m_worldP1 - m_worldP0;
        AZ::Vector3 uAxis = right * drag.Dot(right);
        AZ::Vector3 vAxis = fwd   * drag.Dot(fwd);

        // Reject degenerate base / height.
        if (uAxis.GetLength() < 0.0001f || vAxis.GetLength() < 0.0001f ||
            AZStd::abs(m_height) < 0.0001f)
        {
            return;
        }

        // Force CCW winding about +up so the in-plane orientation is consistent.
        if (uAxis.Cross(vAxis).Dot(up) < 0.f)
        {
            AZStd::swap(uAxis, vAxis);
        }

        // Footprint centre from the original (un-rotated) drag.
        const AZ::Vector3 center = m_worldP0 + (uAxis + vAxis) * 0.5f;

        // Staircase: re-orient the run/width axes within the same footprint.
        if (CurrentShape() == DrawShapeType::Staircase)
        {
            Detail::ApplyStairRotation(uAxis, vAxis, CurrentStairInfo().m_rotation);
        }

        // Build the chosen shape directly into the mesh: base on the surface
        // (up offset 0), top/apex at the pull height.
        Detail::BuildShapeSolid(
            *whiteBox, worldFromLocal.GetInverse(), center, uAxis, vAxis, up,
            0.0f, m_height, CurrentShape(), CurrentSides(), EffectiveStairSteps());

        Api::CalculateNormals(*whiteBox);
        Api::CalculatePlanarUVs(*whiteBox);

        EditorWhiteBoxComponentRequestBus::Event(
            m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::SerializeWhiteBox);
        EditorWhiteBoxComponentRequestBus::Event(
            m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::RebuildWhiteBox);
        EditorWhiteBoxComponentModeRequestBus::Event(
            m_entityComponentIdPair,
            &EditorWhiteBoxComponentModeRequestBus::Events::MarkWhiteBoxIntersectionDataDirty);
        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity,
            m_entityComponentIdPair.GetEntityId());
    }

    // Apply a CSG boolean using a cutter prism built from the drawn footprint
    // (m_worldP0..m_worldP1) extruded along the surface normal by the pull depth.
    // The PULL DIRECTION picks the operation:
    //   pull INTO the surface (height < 0) -> Subtraction (carve a pocket/hole)
    //   pull OUT of the surface (height > 0) -> Union       (add a boss/extrusion)
    // Uses Manifold rather than the legacy inset/extrude/remove-cap trick, so a
    // carve becomes a closed pocket when shallow and a clean through-hole when it
    // exceeds the wall thickness - automatically, no special-casing.
    void DrawShapeMode::BooleanAtPolygon(const AZ::Transform& worldFromLocal, float height)
    {
        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair,
            &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);
        if (!whiteBox)
        {
            return;
        }

        // Surface basis: up = outward normal of the face the user drew on.
        AZ::Vector3 right, fwd, up;
        Detail::BasisFromNormal(m_surfaceNormal, right, fwd, up);

        // Drawn rectangle in the surface plane.
        const AZ::Vector3 drag = m_worldP1 - m_worldP0;
        AZ::Vector3 uAxis = right * drag.Dot(right);
        AZ::Vector3 vAxis = fwd   * drag.Dot(fwd);

        const float depth = AZStd::abs(height);
        if (uAxis.GetLength() < 0.0001f || vAxis.GetLength() < 0.0001f || depth < 0.0001f)
        {
            return;
        }

        // Force CCW winding about +up so the cutter is wound outward consistently
        // (same convention as CommitBox).
        if (uAxis.Cross(vAxis).Dot(up) < 0.f)
        {
            AZStd::swap(uAxis, vAxis);
        }

        // Pull direction decides the operation.
        const bool carve = (height < 0.f);
        const Api::BooleanOperation operation =
            carve ? Api::BooleanOperation::Subtraction : Api::BooleanOperation::Union;

        // The cutter is the SELECTED shape (box/cylinder/pyramid/cone, N sides),
        // not just a box. It must overlap the surface slightly (never sit exactly
        // coplanar with the target face - coplanar faces make the boolean fragile)
        // and overshoot the far end so the cut/weld is clean:
        //   carve: base just OUTSIDE the surface (+eps), extrude to -(depth+eps)
        //   add:   base just INSIDE the surface (-eps), extrude to +(depth+eps)
        const float surfaceSpan = AZ::GetMax(uAxis.GetLength(), vAxis.GetLength());
        const float eps = AZ::GetMax(surfaceSpan * 0.02f, 0.001f);

        const AZ::Vector3 center = m_worldP0 + (uAxis + vAxis) * 0.5f;
        const float baseUp = carve ?  eps           : -eps;
        const float topUp  = carve ? -(depth + eps) : (depth + eps);

        // Staircase: re-orient the run/width axes within the same footprint.
        if (CurrentShape() == DrawShapeType::Staircase)
        {
            Detail::ApplyStairRotation(uAxis, vAxis, CurrentStairInfo().m_rotation);
        }

        // Build the cutter as its own watertight white box mesh, expressed in the
        // TARGET's local space so we can boolean with an identity transform.
        Api::WhiteBoxMeshPtr cutter = Api::CreateWhiteBoxMesh();
        Detail::BuildShapeSolid(
            *cutter, worldFromLocal.GetInverse(), center, uAxis, vAxis, up,
            baseUp, topUp, CurrentShape(), CurrentSides(), EffectiveStairSteps());
        Api::CalculateNormals(*cutter);

        AzToolsFramework::ScopedUndoBatch undoBatch(carve ? "Carve White Box" : "Add White Box");

        // Apply the boolean (identity transform: the cutter is already built in
        // the target's local space).
        if (!Api::ApplyMeshBoolean(*whiteBox, *cutter, AZ::Transform::CreateIdentity(), operation))
        {
            // No intersection / empty result - nothing to do.
            return;
        }

        Api::CalculateNormals(*whiteBox);
        Api::CalculatePlanarUVs(*whiteBox);

        EditorWhiteBoxComponentRequestBus::Event(
            m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::SerializeWhiteBox);
        EditorWhiteBoxComponentRequestBus::Event(
            m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::RebuildWhiteBox);
        EditorWhiteBoxComponentModeRequestBus::Event(
            m_entityComponentIdPair,
            &EditorWhiteBoxComponentModeRequestBus::Events::MarkWhiteBoxIntersectionDataDirty);
        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity,
            m_entityComponentIdPair.GetEntityId());
    }

    void DrawShapeMode::Cancel()
    {
        m_numericInput.Reset();
        m_state  = DrawState::Idle;
        m_height = 0.f;
    }

    bool DrawShapeMode::BeginNumericIfPulling()
    {
        // Numeric depth is only meaningful once the base is locked and we're
        // pulling height.
        if (m_state != DrawState::PullingHeight)
        {
            return false;
        }
        if (!m_numericInput.IsActive())
        {
            m_numericInput.Begin(NumericOpMode::Draw);
        }
        return true;
    }

    void DrawShapeMode::SyncPreviewHeight()
    {
        // Live-preview the typed expression as the pull depth.
        if (m_numericInput.IsActive() && !m_numericInput.IsEmpty())
        {
            m_height = m_numericInput.GetValue();
        }
    }

    void DrawShapeMode::NumericConfirm()
    {
        if (!m_numericInput.IsActive() || m_state != DrawState::PullingHeight)
        {
            return;
        }

        const float value = m_numericInput.GetValue();
        m_numericInput.Reset();
        m_height = value;

        if (AZStd::abs(m_height) < 0.0001f)
        {
            Cancel();
            return;
        }

        // Ctrl gates the boolean; the sign of the depth picks add vs carve.
        if (m_carveMode)
        {
            BooleanAtPolygon(m_worldFromLocal, m_height);
        }
        else
        {
            CommitBox(m_worldFromLocal);
        }
        m_state = DrawState::Idle;
    }

    void DrawShapeMode::NumericCancel()
    {
        // First Escape drops just the numeric entry (back to mouse pull);
        // a second one (nothing typed) cancels the whole draw.
        if (m_numericInput.IsActive())
        {
            m_numericInput.Reset();
        }
        else
        {
            Cancel();
        }
    }

    void DrawShapeMode::DisplayViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        // Unit-cube stamp hover ghost (drawn even when the draw state is Idle).
        if (m_unitCubeHoverValid)
        {
            AZ::Vector3 c[8];
            for (int i = 0; i < 8; ++i)
            {
                const AZ::Vector3 localCorner = m_unitCubeMinLocal +
                    AZ::Vector3(static_cast<float>(i & 1), static_cast<float>((i >> 1) & 1), static_cast<float>((i >> 2) & 1));
                c[i] = m_worldFromLocal.TransformPoint(localCorner);
            }
            const AZ::Color color = m_unitCubeCarve ? AZ::Color(1.0f, 0.25f, 0.25f, 1.0f) : AZ::Color(0.3f, 1.0f, 0.3f, 1.0f);
            debugDisplay.DepthTestOff();
            debugDisplay.SetColor(color);
            debugDisplay.SetLineWidth(static_cast<float>(cl_whiteBoxEdgeVisualWidth));
            // 12 edges of the cube (corner index bits = x,y,z)
            const int edges[12][2] = {
                {0,1},{2,3},{4,5},{6,7}, {0,2},{1,3},{4,6},{5,7}, {0,4},{1,5},{2,6},{3,7} };
            for (const auto& e : edges)
            {
                debugDisplay.DrawLine(c[e[0]], c[e[1]]);
            }
        }

        if (m_state == DrawState::Idle)
        {
            return;
        }

        // Build the surface-aligned frame from the normal captured at the anchor.
        AZ::Vector3 right, fwd, up;
        Detail::BasisFromNormal(m_surfaceNormal, right, fwd, up);

        const AZ::Vector3 drag = m_worldP1 - m_worldP0;
        AZ::Vector3 uAxis = right * drag.Dot(right);
        AZ::Vector3 vAxis = fwd   * drag.Dot(fwd);
        if (uAxis.Cross(vAxis).Dot(up) < 0.f)
        {
            AZStd::swap(uAxis, vAxis);
        }

        const AZ::Vector3 center = m_worldP0 + (uAxis + vAxis) * 0.5f;

        const DrawShapeType shape = CurrentShape();
        // Staircase: re-orient run/width within the same footprint for the preview.
        if (shape == DrawShapeType::Staircase)
        {
            Detail::ApplyStairRotation(uAxis, vAxis, CurrentStairInfo().m_rotation);
        }

        const AZ::Vector3 ru     = uAxis * 0.5f;
        const AZ::Vector3 rv     = vAxis * 0.5f;
        const AZ::Vector3 ext    = up * m_height;   // signed — preview follows pull direction

        const bool isSphere = (shape == DrawShapeType::Sphere);
        const bool isStair  = (shape == DrawShapeType::Staircase);
        const bool pointed  = (shape == DrawShapeType::Pyramid || shape == DrawShapeType::Cone);
        const bool round    = (shape == DrawShapeType::Cylinder || shape == DrawShapeType::Cone || isSphere);

        // Preview the ACTUAL shape footprint, matching what will be built. The
        // staircase footprint is the drawn rectangle (4 corners).
        const int sides = CurrentSides();
        const int steps = EffectiveStairSteps();
        const int footprintSides = isStair ? 4 : sides;
        const AZStd::vector<AZ::Vector3> ring = Detail::ComputeFootprintRing(center, ru, rv, round, footprintSides);
        const size_t n = ring.size();

        const bool pullingHeight = (m_state == DrawState::PullingHeight);

        const AZ::Color fillColor = pullingHeight
            ? AZ::Color(static_cast<AZ::Color>(ed_whiteBoxPolygonSelection).GetR(),
                        static_cast<AZ::Color>(ed_whiteBoxPolygonSelection).GetG(),
                        static_cast<AZ::Color>(ed_whiteBoxPolygonSelection).GetB(), 0.25f)
            : AZ::Color(static_cast<AZ::Color>(ed_whiteBoxPolygonHover).GetR(),
                        static_cast<AZ::Color>(ed_whiteBoxPolygonHover).GetG(),
                        static_cast<AZ::Color>(ed_whiteBoxPolygonHover).GetB(), 0.25f);

        const AZ::Color outlineColor = m_carveMode
        ? AZ::Color(1.0f, 0.25f, 0.25f, 1.0f)   // red = carve
        : (pullingHeight
            ? static_cast<AZ::Color>(ed_whiteBoxOutlineSelection)
            : static_cast<AZ::Color>(ed_whiteBoxOutlineHover));

        debugDisplay.DepthTestOff();
        debugDisplay.SetLineWidth(static_cast<float>(cl_whiteBoxEdgeVisualWidth));

        // --- Base footprint outline + fill (always drawn) ---
        debugDisplay.SetColor(outlineColor);
        for (size_t i = 0; i < n; ++i)
        {
            debugDisplay.DrawLine(ring[i], ring[(i + 1) % n]);
        }
        {
            AZStd::vector<AZ::Vector3> baseTris;
            baseTris.reserve(n * 3);
            for (size_t i = 0; i < n; ++i)
            {
                baseTris.push_back(center);
                baseTris.push_back(ring[i]);
                baseTris.push_back(ring[(i + 1) % n]);
            }
            debugDisplay.DrawTriangles(baseTris, fillColor);
        }

        // --- During height pull, draw the extrusion / apex / sphere / staircase ---
        if (pullingHeight)
        {
            debugDisplay.SetColor(outlineColor);
            if (isStair)
            {
                // Wireframe of every step box, matching BuildStaircaseSolid.
                const AZ::Vector3 origin = center - ru - rv; // base-plane min corner
                const int stepCount = AZ::GetClamp(steps, 1, 256);
                const int edges[12][2] = {
                    {0,1},{2,3},{4,5},{6,7}, {0,2},{1,3},{4,6},{5,7}, {0,4},{1,5},{2,6},{3,7} };
                for (int s = 0; s < stepCount; ++s)
                {
                    const float v0 = static_cast<float>(s) / static_cast<float>(stepCount);
                    const float v1 = static_cast<float>(s + 1) / static_cast<float>(stepCount);
                    const float h1 = v1 * m_height;
                    AZ::Vector3 c[8];
                    for (int k = 0; k < 8; ++k)
                    {
                        const float fu = static_cast<float>(k & 1);
                        const float fv = ((k >> 1) & 1) ? v1 : v0;
                        const float fh = ((k >> 2) & 1) ? h1 : 0.0f;
                        c[k] = origin + uAxis * fu + vAxis * fv + up * fh;
                    }
                    for (const auto& e : edges)
                    {
                        debugDisplay.DrawLine(c[e[0]], c[e[1]]);
                    }
                }
            }
            else if (isSphere)
            {
                // Three great-circle ellipses give a clear sphere gizmo.
                const AZ::Vector3 sc = center + ext * 0.5f; // sphere centre (mid-height)
                const AZ::Vector3 au = ru;                  // half-extent along u
                const AZ::Vector3 av = rv;                  // half-extent along v
                const AZ::Vector3 aw = ext * 0.5f;          // half-extent along up
                const int segs = AZ::GetClamp(sides, 8, 64);
                const auto drawEllipse = [&](const AZ::Vector3& ea, const AZ::Vector3& eb)
                {
                    AZ::Vector3 prev = sc + ea;
                    for (int i = 1; i <= segs; ++i)
                    {
                        const float t = AZ::Constants::TwoPi * static_cast<float>(i) / static_cast<float>(segs);
                        const AZ::Vector3 cur = sc + ea * std::cos(t) + eb * std::sin(t);
                        debugDisplay.DrawLine(prev, cur);
                        prev = cur;
                    }
                };
                drawEllipse(au, av); // equator
                drawEllipse(au, aw); // u-up meridian
                drawEllipse(av, aw); // v-up meridian
            }
            else if (pointed)
            {
                const AZ::Vector3 apex = center + ext;
                for (size_t i = 0; i < n; ++i)
                {
                    debugDisplay.DrawLine(ring[i], apex);
                }
            }
            else
            {
                for (size_t i = 0; i < n; ++i)
                {
                    const size_t j = (i + 1) % n;
                    debugDisplay.DrawLine(ring[i] + ext, ring[j] + ext); // top outline
                    debugDisplay.DrawLine(ring[i], ring[i] + ext);       // vertical edge
                }
                const AZ::Vector3 topCenter = center + ext;
                AZStd::vector<AZ::Vector3> topTris;
                topTris.reserve(n * 3);
                for (size_t i = 0; i < n; ++i)
                {
                    topTris.push_back(topCenter);
                    topTris.push_back(ring[i] + ext);
                    topTris.push_back(ring[(i + 1) % n] + ext);
                }
                debugDisplay.DrawTriangles(topTris, fillColor);
            }
        }

        // --- Dimension cues: width / length / height on the matching edges ---
        // (axis-coloured, like a Lumberyard-style measuring overlay).
        {
            const float width  = uAxis.GetLength();    // along u (red)
            const float length = vAxis.GetLength();    // along v (green)
            const float height = AZStd::abs(m_height); // along up (blue)

            const AZ::Color widthColor (1.0f, 0.30f, 0.30f, 1.0f);
            const AZ::Color lengthColor(0.35f, 1.0f, 0.35f, 1.0f);
            const AZ::Color heightColor(0.45f, 0.60f, 1.0f, 1.0f);

            // Highlight the measured edges in their axis colour.
            debugDisplay.SetLineWidth(static_cast<float>(cl_whiteBoxEdgeVisualWidth) + 1.0f);
            const AZ::Vector3 cu0 = center - ru - rv;          // origin corner
            debugDisplay.SetColor(widthColor);
            debugDisplay.DrawLine(cu0, cu0 + uAxis);           // width edge (v-)
            debugDisplay.SetColor(lengthColor);
            debugDisplay.DrawLine(cu0, cu0 + vAxis);           // length edge (u-)

            // Push each number OUTSIDE its edge (along the outward in-plane
            // direction) so the dimensions never sit on the geometry or each other.
            const AZ::Vector3 uHat = uAxis.GetNormalizedSafe();
            const AZ::Vector3 vHat = vAxis.GetNormalizedSafe();
            const float gap = 0.4f + 0.06f * AZ::GetMax(width, AZ::GetMax(length, height));

            debugDisplay.SetColor(widthColor);
            debugDisplay.DrawTextLabel(center - rv - vHat * gap, 1.1f, AZStd::string::format("%.3f", width).c_str(), true, 0, 0);
            debugDisplay.SetColor(lengthColor);
            debugDisplay.DrawTextLabel(center - ru - uHat * gap, 1.1f, AZStd::string::format("%.3f", length).c_str(), true, 0, 0);

            if (pullingHeight)
            {
                const AZ::Vector3 hBase = center + ru + rv;    // far corner rises with the pull
                debugDisplay.SetColor(heightColor);
                debugDisplay.DrawLine(hBase, hBase + ext);     // height edge
                debugDisplay.DrawTextLabel(
                    hBase + ext * 0.5f + (uHat + vHat) * (gap * 0.5f), 1.1f,
                    AZStd::string::format("%.3f", height).c_str(), true, 0, 0);
            }
            debugDisplay.SetLineWidth(static_cast<float>(cl_whiteBoxEdgeVisualWidth));
        }

        // --- Numeric depth entry (Blender-style), lifted above the shape ---
        if (m_numericInput.IsActive())
        {
            const float sizeRef = AZ::GetMax(uAxis.GetLength(), AZ::GetMax(vAxis.GetLength(), AZStd::abs(m_height)));
            const float lift = AZ::GetMax(sizeRef * 0.4f, 1.0f);
            const AZ::Vector3 labelPos = center + ext + up * (lift * 1.5f);
            const AZStd::string status = m_carveMode
                ? (AZStd::string("[Ctrl] ") + m_numericInput.GetStatusText())
                : m_numericInput.GetStatusText();
            debugDisplay.SetColor(AZ::Color(1.0f, 1.0f, 1.0f, 1.0f));
            debugDisplay.DrawTextLabel(labelPos, 1.3f, status.c_str(), true, 0, 0);
        }
    }



    AZStd::vector<AzToolsFramework::ActionOverride> DrawShapeMode::PopulateActions(
        [[maybe_unused]] const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        return {};
    }

    // ----------------------------------------------------------------------- //
    // Shape + side count (sourced from the component's Inspector properties)    //
    // ----------------------------------------------------------------------- //
    DrawShapeType DrawShapeMode::CurrentShape() const
    {
        DrawShapeType shape = DrawShapeType::Box;
        EditorWhiteBoxComponentRequestBus::EventResult(
            shape, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetDrawShape);
        return shape;
    }

    int DrawShapeMode::CurrentSides() const
    {
        int sides = 4;
        EditorWhiteBoxComponentRequestBus::EventResult(
            sides, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetDrawSides);
        return AZ::GetClamp(sides, 3, 256);
    }

    DrawStairInfo DrawShapeMode::CurrentStairInfo() const
    {
        DrawStairInfo stair;
        EditorWhiteBoxComponentRequestBus::EventResult(
            stair, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetDrawStairInfo);

        // sanitise to safe ranges before use
        stair.m_steps = AZ::GetClamp(stair.m_steps, 1, 256);
        stair.m_stepHeight = AZ::GetMax(stair.m_stepHeight, 0.0001f);
        stair.m_rotation = ((stair.m_rotation % 4) + 4) % 4; // normalise to 0..3
        return stair;
    }

    int DrawShapeMode::EffectiveStairSteps() const
    {
        const DrawStairInfo stair = CurrentStairInfo();
        if (!stair.m_byHeight)
        {
            return stair.m_steps;
        }
        // Derive the count from the pull height so the riser stays a fixed size.
        const float rise = AZStd::abs(m_height);
        const int derived = aznumeric_cast<int>(std::lround(rise / stair.m_stepHeight));
        return AZ::GetClamp(derived, 1, 256);
    }

    bool DrawShapeMode::CurrentCarve() const
    {
        bool carve = false;
        EditorWhiteBoxComponentRequestBus::EventResult(
            carve, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetDrawCarve);
        return carve;
    }

    bool DrawShapeMode::UnitCubeMode() const
    {
        bool unitCube = false;
        EditorWhiteBoxComponentRequestBus::EventResult(
            unitCube, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetDrawUnitCube);
        return unitCube;
    }

    bool DrawShapeMode::UnitCubeCell(
        const AZ::Transform& worldFromLocal, const AZ::Vector3& hitWorld, const AZ::Vector3& hitNormal, const bool carve,
        AZ::Vector3& outMinLocal) const
    {
        const AZ::Transform localFromWorld = worldFromLocal.GetInverse();
        const AZ::Vector3 localHit = localFromWorld.TransformPoint(hitWorld);
        const AZ::Vector3 localNormal =
            AzToolsFramework::TransformDirectionNoScaling(localFromWorld, hitNormal).GetNormalizedSafe();

        // Nudge half a unit off the surface to pick a cell unambiguously:
        //   add   -> the empty cell on the +normal side (where the new cube appears)
        //   carve -> the solid cell on the -normal side (the block you clicked)
        const AZ::Vector3 sample = localHit + localNormal * (carve ? -0.5f : 0.5f);
        outMinLocal = AZ::Vector3(std::floor(sample.GetX()), std::floor(sample.GetY()), std::floor(sample.GetZ()));
        return true;
    }

    void DrawShapeMode::StampUnitCube(
        const AZ::Transform& worldFromLocal, const AZ::Vector3& hitWorld, const AZ::Vector3& hitNormal, const bool carve)
    {
        AZ::Vector3 minLocal;
        if (!UnitCubeCell(worldFromLocal, hitWorld, hitNormal, carve, minLocal))
        {
            return;
        }

        // Fill (add) or clear (carve) the targeted voxel cell. The component
        // regenerates a clean, watertight, merged surface from its voxel set - no
        // CSG round-trip, so no slivers/cracks/non-manifolds from grid geometry.
        EditorWhiteBoxComponentRequestBus::Event(
            m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::SetVoxelCell, minLocal, !carve);
    }

    // ----------------------------------------------------------------------- //
    // Numeric-depth keyboard actions (Blender-style), scoped to the draw mode  //
    // ----------------------------------------------------------------------- //
    namespace
    {
        constexpr AZStd::string_view DrawNumericConfirmId   = "o3de.action.whiteBoxDraw.numeric.confirm";
        constexpr AZStd::string_view DrawNumericCancelId    = "o3de.action.whiteBoxDraw.numeric.cancel";
        constexpr AZStd::string_view DrawNumericBackspaceId = "o3de.action.whiteBoxDraw.numeric.backspace";
        constexpr AZStd::string_view DrawNumericDecimalId   = "o3de.action.whiteBoxDraw.numeric.decimal";
        constexpr AZStd::string_view DrawNumericNegateId    = "o3de.action.whiteBoxDraw.numeric.negate";
        constexpr AZStd::string_view DrawNumericOpPlusId    = "o3de.action.whiteBoxDraw.numeric.opPlus";
        constexpr AZStd::string_view DrawNumericOpMultId    = "o3de.action.whiteBoxDraw.numeric.opMult";
        constexpr AZStd::string_view DrawNumericOpDivId     = "o3de.action.whiteBoxDraw.numeric.opDiv";
        constexpr AZStd::string_view DrawNumericDigitIds[10] = {
            "o3de.action.whiteBoxDraw.numeric.digit0", "o3de.action.whiteBoxDraw.numeric.digit1",
            "o3de.action.whiteBoxDraw.numeric.digit2", "o3de.action.whiteBoxDraw.numeric.digit3",
            "o3de.action.whiteBoxDraw.numeric.digit4", "o3de.action.whiteBoxDraw.numeric.digit5",
            "o3de.action.whiteBoxDraw.numeric.digit6", "o3de.action.whiteBoxDraw.numeric.digit7",
            "o3de.action.whiteBoxDraw.numeric.digit8", "o3de.action.whiteBoxDraw.numeric.digit9",
        };
    } // namespace

    void DrawShapeMode::RegisterActions()
    {
        auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        AZ_Assert(actionManagerInterface, "WhiteBoxDrawShapeMode - could not get ActionManagerInterface on RegisterActions.");
        auto hotKeyManagerInterface = AZ::Interface<AzToolsFramework::HotKeyManagerInterface>::Get();
        AZ_Assert(hotKeyManagerInterface, "WhiteBoxDrawShapeMode - could not get HotKeyManagerInterface on RegisterActions.");

        // Dispatch a no-arg request to every active White Box component in draw mode.
        auto dispatchToDrawModes = [](auto fn)
        {
            auto cmci = AZ::Interface<AzToolsFramework::ComponentModeCollectionInterface>::Get();
            AZ_Assert(cmci, "Could not retrieve component mode collection.");
            cmci->EnumerateActiveComponents(
                [fn](const AZ::EntityComponentIdPair& id, const AZ::Uuid&)
                {
                    EditorWhiteBoxDrawShapeModeRequestBus::Event(id, fn);
                });
        };

        const auto registerSimple =
            [&](AZStd::string_view id, const char* name, const char* hotkey, auto fn)
        {
            AzToolsFramework::ActionProperties p;
            p.m_name = name;
            p.m_category = "White Box Component Mode - Draw Shape";
            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier, id, p,
                [dispatchToDrawModes, fn] { dispatchToDrawModes(fn); });
            hotKeyManagerInterface->SetActionHotKey(id, hotkey);
        };

        registerSimple(DrawNumericConfirmId,   "Draw Numeric Confirm",   "Return",    &EditorWhiteBoxDrawShapeModeRequests::NumericConfirm);
        registerSimple(DrawNumericCancelId,    "Draw Numeric Cancel",    "Escape",    &EditorWhiteBoxDrawShapeModeRequests::NumericCancel);
        registerSimple(DrawNumericBackspaceId, "Draw Numeric Backspace", "Backspace", &EditorWhiteBoxDrawShapeModeRequests::NumericBackspace);
        registerSimple(DrawNumericDecimalId,   "Draw Numeric Decimal",   ".",         &EditorWhiteBoxDrawShapeModeRequests::NumericAppendDecimal);
        registerSimple(DrawNumericNegateId,    "Draw Numeric Minus",     "-",         &EditorWhiteBoxDrawShapeModeRequests::NumericNegate);
        registerSimple(DrawNumericOpPlusId,    "Draw Numeric Plus",      "+",         &EditorWhiteBoxDrawShapeModeRequests::NumericAppendOperatorPlus);
        registerSimple(DrawNumericOpMultId,    "Draw Numeric Multiply",  "*",         &EditorWhiteBoxDrawShapeModeRequests::NumericAppendOperatorMult);
        registerSimple(DrawNumericOpDivId,     "Draw Numeric Divide",    "/",         &EditorWhiteBoxDrawShapeModeRequests::NumericAppendOperatorDiv);

        // Digits 0-9 (NumericAppendDigit takes a char, so inline the dispatch).
        const char* digitKeys[10] = {"0","1","2","3","4","5","6","7","8","9"};
        for (int d = 0; d <= 9; ++d)
        {
            AzToolsFramework::ActionProperties p;
            p.m_name = AZStd::string::format("Draw Numeric Digit %d", d).c_str();
            p.m_category = "White Box Component Mode - Draw Shape";
            const char digit = static_cast<char>('0' + d);
            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier, DrawNumericDigitIds[d], p,
                [digit]
                {
                    auto cmci = AZ::Interface<AzToolsFramework::ComponentModeCollectionInterface>::Get();
                    AZ_Assert(cmci, "Could not retrieve component mode collection.");
                    cmci->EnumerateActiveComponents(
                        [digit](const AZ::EntityComponentIdPair& id, const AZ::Uuid&)
                        {
                            EditorWhiteBoxDrawShapeModeRequestBus::Event(
                                id, &EditorWhiteBoxDrawShapeModeRequests::NumericAppendDigit, digit);
                        });
                });
            hotKeyManagerInterface->SetActionHotKey(DrawNumericDigitIds[d], digitKeys[d]);
        }
    }

    void DrawShapeMode::BindActionsToModes(const AZStd::string& modeIdentifier)
    {
        auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        AZ_Assert(actionManagerInterface, "WhiteBoxDrawShapeMode - could not get ActionManagerInterface on BindActionsToModes.");

        for (const auto& id :
             { DrawNumericConfirmId, DrawNumericCancelId, DrawNumericBackspaceId, DrawNumericDecimalId,
               DrawNumericNegateId, DrawNumericOpPlusId, DrawNumericOpMultId, DrawNumericOpDivId })
        {
            actionManagerInterface->AssignModeToAction(modeIdentifier, id);
        }
        for (const auto& digitId : DrawNumericDigitIds)
        {
            actionManagerInterface->AssignModeToAction(modeIdentifier, digitId);
        }
    }

} // namespace WhiteBox