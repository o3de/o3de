/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxDrawShapeMode.h"

#include "WhiteBoxShapeBuilders.h"

#include "EditorWhiteBoxComponentModeBus.h"
#include "EditorWhiteBoxComponentModeTypes.h"
#include "Viewport/WhiteBoxManipulatorBounds.h"
#include "Viewport/WhiteBoxViewportConstants.h"
#include "Util/WhiteBoxMathUtil.h"
#include "Util/WhiteBoxEditorUtil.h"
#include "Util/WhiteBoxSnapUtil.h"
#include "Util/WhiteBoxMeshUtil.h"

#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/optional.h>
#include <AzCore/Math/Plane.h>
#include <AzCore/Math/MathStringConversions.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
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
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/array.h>
#include "EditorWhiteBoxComponent.h"
#include <cmath>


namespace WhiteBox
{
    AZ_CLASS_ALLOCATOR_IMPL(DrawShapeMode, AZ::SystemAllocator)
    // Helper: build a right-handed basis with `up` as the Z-equivalent (the normal).
    void Detail::BasisFromNormal(const AZ::Vector3& n, AZ::Vector3& right, AZ::Vector3& fwd, AZ::Vector3& up)
    {
        up = n.GetNormalizedSafe();
        if (up.IsZero())
        {
            up = AZ::Vector3::CreateAxisZ(); // a degenerate surface normal falls back to drawing on the ground
        }

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
        case DrawShapeType::Room:      return "Room";
        case DrawShapeType::Door:      return "Door";
        case DrawShapeType::CircularStairs: return "Circular Stairs";
        case DrawShapeType::Plane: return "Plane";
        case DrawShapeType::Torus: return "Torus";
        case DrawShapeType::Pipe: return "Pipe";
        case DrawShapeType::Polygon: return "Freeform Polygon";
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

    // Build a parametric hollow room with double (cavity) walls and optional slabs as one watertight shell, without CSG.
    void Detail::BuildRoomSolid(
        WhiteBoxMesh& mesh, const float interiorWidth, const float interiorDepth, const float interiorHeight,
        const float wallThicknessIn, const float cavityGapIn, const bool floor, const bool ceiling)
    {
        const float iw = AZ::GetMax(interiorWidth, 0.01f);
        const float id = AZ::GetMax(interiorDepth, 0.01f);
        const float ih = AZ::GetMax(interiorHeight, 0.01f);
        const float t = AZ::GetMax(wallThicknessIn, 0.001f);
        const float gap = AZ::GetMax(cavityGapIn, 0.0f);

        const float ihx = iw * 0.5f; // interior half-extent along X
        const float ihy = id * 0.5f; // interior half-extent along Y

        // Nested rectangles, innermost first: the interior, then each wall leaf and the cavity between them.
        const bool singleLeaf = gap < 1e-4f; // a zero gap is one solid wall, not two leaves pressed together
        AZStd::vector<float> offsets = { 0.0f, t };
        if (!singleLeaf)
        {
            offsets.push_back(t + gap);
            offsets.push_back(2.0f * t + gap);
        }
        const size_t boundaries = offsets.size();

        // Heights shared by every region, so faces meeting at a level meet on the same vertices.
        const float levels[4] = { -t, 0.0f, ih, ih + t };
        // Region r lies inside boundary r (and outside boundary r - 1); each is solid over some level ranges.
        using Span = AZStd::pair<int, int>;
        AZStd::vector<Span> slabs;
        if (floor)
        {
            slabs.emplace_back(0, 1);
        }
        if (ceiling)
        {
            slabs.emplace_back(2, 3);
        }
        const Span leaf(floor ? 0 : 1, ceiling ? 3 : 2); // a leaf runs through the slabs it stands between
        AZStd::vector<AZStd::vector<Span>> regions;
        for (size_t region = 0; region < boundaries; ++region)
        {
            // Even regions are open space (interior, cavity) holding only the slabs; odd ones are wall leaves.
            regions.push_back(region % 2 == 0 ? slabs : AZStd::vector<Span>{ leaf });
        }
        const auto solid = [&regions](const size_t region, const int level)
        {
            if (region >= regions.size())
            {
                return false; // outside the room
            }
            for (const Span& span : regions[region])
            {
                if (span.first <= level && level < span.second)
                {
                    return true;
                }
            }
            return false;
        };

        const float sx[4] = { -1.0f, 1.0f, 1.0f, -1.0f };
        const float sy[4] = { -1.0f, -1.0f, 1.0f, 1.0f };
        AZStd::vector<Api::VertexHandle> cache(boundaries * 4 * 4);
        const auto corner = [&](const size_t boundary, const int k, const int level)
        {
            Api::VertexHandle& handle = cache[(boundary * 4 + static_cast<size_t>(k)) * 4 + static_cast<size_t>(level)];
            if (!handle.IsValid())
            {
                handle = Api::AddVertex(
                    mesh, AZ::Vector3(sx[k] * (ihx + offsets[boundary]), sy[k] * (ihy + offsets[boundary]), levels[level]));
            }
            return handle;
        };
        // Two triangles for a quad in ring order, wound so the normal follows outward.
        const auto quad = [&mesh](Api::FaceVertHandlesList& faces, const AZStd::array<Api::VertexHandle, 4>& v, const AZ::Vector3& outward)
        {
            const AZ::Vector3 a = Api::VertexPosition(mesh, v[0]);
            const bool flip = (Api::VertexPosition(mesh, v[1]) - a).Cross(Api::VertexPosition(mesh, v[2]) - a).Dot(outward) < 0.0f;
            faces.push_back(flip ? Api::FaceVertHandles{ { v[0], v[2], v[1] } } : Api::FaceVertHandles{ { v[0], v[1], v[2] } });
            faces.push_back(flip ? Api::FaceVertHandles{ { v[0], v[3], v[2] } } : Api::FaceVertHandles{ { v[0], v[2], v[3] } });
        };

        // Horizontal faces: a region's span ends at a level. Neighbouring regions ending at the same level form one polygon.
        const auto horizontal = [&](const int level, const bool up)
        {
            const auto ends = [&](const size_t region)
            {
                return solid(region, up ? level - 1 : level) && !solid(region, up ? level : level - 1);
            };
            const AZ::Vector3 outward = up ? AZ::Vector3::CreateAxisZ() : -AZ::Vector3::CreateAxisZ();
            Api::FaceVertHandlesList faces;
            const auto flush = [&mesh, &faces]()
            {
                if (!faces.empty())
                {
                    Api::AddPolygon(mesh, faces);
                    faces.clear();
                }
            };
            for (size_t region = 0; region < boundaries; ++region)
            {
                if (!ends(region))
                {
                    flush();
                    continue;
                }
                if (region == 0)
                {
                    quad(faces, { corner(0, 0, level), corner(0, 1, level), corner(0, 2, level), corner(0, 3, level) }, outward);
                    continue;
                }
                for (int k = 0; k < 4; ++k)
                {
                    const int k1 = (k + 1) % 4;
                    quad(faces,
                        { corner(region - 1, k, level), corner(region - 1, k1, level), corner(region, k1, level),
                          corner(region, k, level) },
                        outward);
                }
            }
            flush();
        };

        // Vertical faces: wherever the regions either side of a boundary differ, one quad per side per run of levels.
        const AZ::Vector3 sideOut[4] = {
            AZ::Vector3(0.0f, -1.0f, 0.0f), AZ::Vector3(1.0f, 0.0f, 0.0f), AZ::Vector3(0.0f, 1.0f, 0.0f), AZ::Vector3(-1.0f, 0.0f, 0.0f)
        };
        const auto vertical = [&](const size_t boundary)
        {
            int level = 0;
            while (level < 3)
            {
                const bool inside = solid(boundary, level);
                if (inside == solid(boundary + 1, level))
                {
                    ++level;
                    continue;
                }
                // A run never passes a level where either side's span ends, so no horizontal edge meets it midway.
                int end = level + 1;
                while (end < 3 && solid(boundary, end) == inside && solid(boundary + 1, end) != inside)
                {
                    ++end;
                }
                for (int k = 0; k < 4; ++k)
                {
                    const int k1 = (k + 1) % 4;
                    Api::FaceVertHandlesList faces;
                    quad(faces,
                        { corner(boundary, k, level), corner(boundary, k1, level), corner(boundary, k1, end),
                          corner(boundary, k, end) },
                        inside ? sideOut[k] : -sideOut[k]);
                    Api::AddPolygon(mesh, faces);
                }
                level = end;
            }
        };

        // Bottom up, so each face lands against ones already in and the shell grows as one piece.
        for (int level = 0; level < 4; ++level)
        {
            horizontal(level, false);
        }
        for (size_t boundary = 0; boundary < boundaries; ++boundary)
        {
            vertical(boundary);
        }
        for (int level = 0; level < 4; ++level)
        {
            horizontal(level, true);
        }
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
        const float baseUp, const float topUp, const DrawShapeType shapeType, const int sidesIn, const int steps,
        const float holeRatio, const int tubeSides, const float wallThickness, const float cavityGap,
        const bool floor, const bool ceiling, const bool doorFrame, const float archHeight,
        const float innerRadius, const float sweepAngle)
    {
        if (shapeType == DrawShapeType::Plane || shapeType == DrawShapeType::Torus || shapeType == DrawShapeType::Pipe)
        {
            BuildAdditionalShape(
                mesh, localFromWorld, center, uAxis, vAxis, up, baseUp, topUp, shapeType, sidesIn, holeRatio,
                tubeSides);
            return;
        }
        // Room, Door and Circular Stairs are shells with their own builders: build at the origin from the
        // footprint's size, then map onto the footprint.
        if (shapeType == DrawShapeType::Room || shapeType == DrawShapeType::Door ||
            shapeType == DrawShapeType::CircularStairs)
        {
            const float width = AZ::GetMax(uAxis.GetLength(), 0.01f);
            const float depth = AZ::GetMax(vAxis.GetLength(), 0.01f);
            const float height = AZ::GetMax(std::abs(topUp - baseUp), 0.01f);

            const Api::WhiteBoxMeshPtr shape = BuildParametricShapeMesh(
                shapeType, width, depth, height, sidesIn, steps, wallThickness, cavityGap, floor, ceiling,
                doorFrame, archHeight, innerRadius, sweepAngle, false, 0.25f, holeRatio, tubeSides);
            if (!shape)
            {
                return;
            }

            // Those builders work in their own frame: X and Y centred on the origin, Z rising from the
            // base plane. Move that onto the drawn footprint's axes, then into the mesh's local space.
            const AZ::Vector3 u = uAxis.GetNormalizedSafe();
            const AZ::Vector3 v = vAxis.GetNormalizedSafe();
            const AZ::Vector3 base = center + up * AZ::GetMin(baseUp, topUp);
            for (const Api::VertexHandle vertexHandle : Api::MeshVertexHandles(*shape))
            {
                const AZ::Vector3 local = Api::VertexPosition(*shape, vertexHandle);
                Api::SetVertexPosition(
                    *shape, vertexHandle,
                    localFromWorld.TransformPoint(
                        base + u * local.GetX() + v * local.GetY() + up * local.GetZ()));
            }
            Api::CalculateNormals(*shape);
            Api::CalculatePlanarUVs(*shape);
            AppendMesh(mesh, *shape);
            return;
        }

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
            if (worldNormal.IsZero())
            {
                worldNormal = -rayDirWorld; // a degenerate triangle has no normal of its own
            }
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

        // 1. The editable mesh's cached bounds are in active-layer storage space.
        // Unlike the evaluated mesh and the draw builders, they need the layer transform.
        {
            const AZ::Transform worldFromLayer = EditorSpaceFromLocal(m_entityComponentIdPair);
            const AZ::Transform localFromWorld = worldFromLayer.GetInverse();
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
                        worldFromLayer.TransformPoint(localHit),
                        AzToolsFramework::TransformDirectionNoScaling(worldFromLayer, localNormal));
                }
            }
        }

        // 1b. Also raycast the evaluated mesh (with stamps), with the entity transform only.
        {
            WhiteBoxMesh* evaluated = nullptr;
            EditorWhiteBoxComponentRequestBus::EventResult(
                evaluated, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetEvaluatedWhiteBoxMesh);
            if (evaluated != nullptr)
            {
                raycastWhiteBox(*evaluated, worldFromLocal);
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
                        // The evaluated mesh includes every visible layer at its displayed
                        // position; the editable mesh is still in active-layer storage space.
                        WhiteBoxMesh* mesh = comp->GetEvaluatedWhiteBoxMesh();
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
        // Entity-local, without the layer transform: MapMeshToActiveLayerSpace applies its inverse later.
        const AZ::Transform worldFromLocal =
            AzToolsFramework::WorldFromLocalWithUniformScale(m_entityComponentIdPair.GetEntityId());
        const IntersectionAndRenderData& intersectionData = mouse.m_intersectionData;

        // Cache so a keyboard-driven numeric confirm (which carries no transform)
        // can commit using the current frame's transform.
        m_worldFromLocal = worldFromLocal;

        const auto& mi     = mouseInteraction.m_mouseInteraction;
        const bool  leftDown  = mi.m_mouseButtons.Left() && mouseInteraction.m_mouseEvent == MouseEvent::Down;
        const bool  leftUp    = mi.m_mouseButtons.Left() && mouseInteraction.m_mouseEvent == MouseEvent::Up;
        const bool  rightDown = mi.m_mouseButtons.Right() && mouseInteraction.m_mouseEvent == MouseEvent::Down;
        const bool  moved     = mouseInteraction.m_mouseEvent == MouseEvent::Move;

        if ((m_state == DrawState::DrawingPolygon || m_state == DrawState::PullingPolygonHeight) &&
            !PolygonContextValid())
        {
            Cancel();
        }
        if (!UnitCubeMode() && CurrentShape() == DrawShapeType::Polygon)
        {
            if (m_state != DrawState::Idle && m_state != DrawState::DrawingPolygon &&
                m_state != DrawState::PullingPolygonHeight) { Cancel(); }
            if (mi.m_keyboardModifiers.Alt()) { return false; }
            if (rightDown)
            {
                const bool active = m_state != DrawState::Idle;
                Cancel();
                return active;
            }
            if (m_state == DrawState::Idle && leftDown)
            {
                AZ::Vector3 normal;
                const auto hit = RaycastToSurface(mi, worldFromLocal, intersectionData, normal);
                m_surfaceNormal = normal.IsZero() ? AZ::Vector3::CreateAxisZ() : normal.GetNormalized();
                const auto firstSnap = SnapTargetUnderCursor(mi.m_interactionId.m_viewportId);
                m_worldP0 = firstSnap.has_value() ? *firstSnap : SnapToGrid(hit, hit, mi.m_interactionId.m_viewportId);
                m_worldP1 = m_worldP0;
                auto* component = FindWhiteBoxComponent(m_entityComponentIdPair);
                if (!component) { return false; }
                m_polygonLayerId = component->GetActiveLayerId();
                m_carveMode = mi.m_keyboardModifiers.Ctrl();
                m_polygonBooleanFailed = false;
                m_polygonPoints = {m_worldP0};
                m_polygonInvalid = false;
                m_state = DrawState::DrawingPolygon;
                return true;
            }
            if (m_state == DrawState::DrawingPolygon && (moved || leftDown))
            {
                const auto& pick = mi.m_mousePick;
                // Test the raw cursor in pixels, before plane/grid snapping. This
                // keeps closing easy at any zoom and when the drawing plane is edge-on.
                const auto camera = AzToolsFramework::GetCameraState(mi.m_interactionId.m_viewportId);
                const auto firstCorner = AzFramework::WorldToScreen(m_worldP0, camera);
                const float dx = static_cast<float>(firstCorner.m_x - pick.m_screenCoordinates.m_x);
                const float dy = static_cast<float>(firstCorner.m_y - pick.m_screenCoordinates.m_y);
                constexpr float closeRadiusPixels = 16.0f;
                m_polygonCloseHovered = m_polygonPoints.size() >= 3 &&
                    (m_worldP0 - camera.m_position).Dot(camera.m_forward) > camera.m_nearClip &&
                    dx * dx + dy * dy <= closeRadiusPixels * closeRadiusPixels;
                if (m_polygonCloseHovered)
                {
                    m_worldP1 = m_worldP0;
                    SnapUtil::SetActiveSnapTarget(m_worldP0);
                    if (leftDown) { FinishPolygonOutline(); }
                    return true;
                }
                const float denominator = pick.m_rayDirection.Dot(m_surfaceNormal);
                if (AZStd::abs(denominator) > 1e-6f)
                {
                    const float distance = (m_worldP0 - pick.m_rayOrigin).Dot(m_surfaceNormal) / denominator;
                    if (distance < 0.0f) { return true; }
                    // A vertex under the cursor wins; otherwise the angle and grid settings shape the point.
                    const auto snap = SnapTargetUnderCursor(mi.m_interactionId.m_viewportId);
                    const AZ::Vector3 point = snap.has_value()
                        ? *snap
                        : SnapPolygonPoint(pick.m_rayOrigin + pick.m_rayDirection * distance, mi.m_interactionId.m_viewportId);
                    const AZ::Vector3 onPlane = point - m_surfaceNormal * (point - m_worldP0).Dot(m_surfaceNormal);
                    if (!onPlane.IsFinite())
                    {
                        return true; // keep the last good point rather than feed the preview a broken one
                    }
                    m_worldP1 = onPlane;
                    SnapUtil::SetActiveSnapTarget(snap);
                    if (leftDown)
                    {
                        if (m_polygonPoints.size() < 256 && (m_worldP1 - m_polygonPoints.back()).GetLength() > 0.0001f)
                        {
                            m_polygonPoints.push_back(m_worldP1);
                            m_polygonInvalid = false;
                        }
                    }
                }
                return true;
            }
            if (m_state == DrawState::PullingPolygonHeight)
            {
                if (moved && !m_numericInput.IsActive())
                {
                    const float height = SnapHeightToGrid(
                        RaycastToHeightPlane(mi, worldFromLocal, m_polygonHeightAnchor), mi.m_interactionId.m_viewportId);
                    if (AZ::IsFiniteFloat(height))
                    {
                        m_height = height;
                    }
                    m_polygonBooleanFailed = false;
                }
                if (leftDown && AZStd::abs(m_height) >= 0.0001f)
                {
                    CommitPolygon(m_height);
                }
                return true;
            }
            return m_state == DrawState::DrawingPolygon;
        }

        // Unit-cube stamp: press anchors, drag stretches a footprint of cells, release stamps it. Ctrl
        // (or Carve) subtracts.
        if (UnitCubeMode())
        {
            // Right-click aborts an in-progress preview drag (and is consumed only then).
            if (rightDown)
            {
                const bool wasDragging = m_unitCubeDragging;
                m_unitCubeDragging = false;
                m_unitCubeDragMoved = false;
                m_unitCubeAcrossGrow = false;
                m_unitCubeHoverValid = false;
                return wasDragging;
            }

            const bool liveCarve = mi.m_keyboardModifiers.Ctrl() || CurrentCarve();
            m_unitCubeSize = CurrentUnitCubeSize();
            m_unitCubeCellSize = CurrentCellSize();

            if (leftDown)
            {
                // Anchor the region: capture the corner cell, the surface's thickness
                // axis/sign, the anchor plane, and the carve sign so none flip mid-drag.
                m_unitCubeCarve = liveCarve;
                m_unitCubeDragging = true;
                m_unitCubeDragMoved = false;
                m_unitCubeAcrossGrow = false; // default to depth extrude
                m_unitCubeCtrlPrev = mi.m_keyboardModifiers.Ctrl();

                AZ::Vector3 hitNormal;
                const AZ::Vector3 hitWorld = RaycastToSurface(mi, worldFromLocal, intersectionData, hitNormal);
                AZ::Vector3 baseMin;
                const auto snapTarget = SnapTargetUnderCursor(mi.m_interactionId.m_viewportId);
                m_unitCubeHoverValid =
                    UnitCubeCell(worldFromLocal, hitWorld, hitNormal, m_unitCubeCarve, baseMin, snapTarget);
                SnapUtil::SetActiveSnapTarget(snapTarget);
                m_unitCubeAnchorMin = baseMin;
                m_unitCubeCursorCell = baseMin;
                m_unitCubeAnchorWorld = hitWorld;
                m_unitCubeAnchorNormalWorld = hitNormal;
                UnitCubeAxisFromNormal(worldFromLocal, hitNormal, m_unitCubeAxis, m_unitCubeSign);
                UpdateUnitCubeRegion(baseMin);

                
                return true;
            }

            if (moved)
            {
                if (m_unitCubeDragging)
                {
                    // Ctrl changed since press: grow across the surface; unchanged: extrude along the
                    // normal. Carve stays as it was at press time.
                    const bool ctrlNow = mi.m_keyboardModifiers.Ctrl();
                    m_unitCubeAcrossGrow = (ctrlNow != m_unitCubeCtrlPrev);

                    const AZ::Vector3 rayOrigin = mi.m_mousePick.m_rayOrigin;
                    const AZ::Vector3 rayDir = mi.m_mousePick.m_rayDirection;
                    const AZ::Transform localFromWorld = worldFromLocal.GetInverse();
                    const AZ::Vector3 up = m_unitCubeAnchorNormalWorld.GetNormalizedSafe();
                    constexpr float hysteresis = 0.45f;

                    if (m_unitCubeAcrossGrow)
                    {
                        // ACROSS: project the cursor onto the fixed anchor (surface) plane and
                        // grow the two footprint axes contiguously (can't hop to another face).
                        float t = 0.f;
                        AZ::Intersect::IntersectRayPlane(rayOrigin, rayDir, m_unitCubeAnchorWorld, up, t);
                        if (t > 0.f)
                        {
                            const AZ::Vector3 planeHit = rayOrigin + rayDir * t;
                            if (!m_unitCubeDragMoved &&
                                (planeHit - m_unitCubeAnchorWorld).GetLength() > m_unitCubeCellSize)
                            {
                                m_unitCubeDragMoved = true;
                            }
                            if (m_unitCubeDragMoved)
                            {
                                const AZ::Vector3 localHitCell = localFromWorld.TransformPoint(planeHit) / m_unitCubeCellSize;
                                const AZ::Vector3 localNormal =
                                    AzToolsFramework::TransformDirectionNoScaling(localFromWorld, up).GetNormalizedSafe();
                                const AZ::Vector3 sample = localHitCell + localNormal * (m_unitCubeCarve ? -0.5f : 0.5f);
                                for (int a = 0; a < 3; ++a)
                                {
                                    if (a == m_unitCubeAxis)
                                    {
                                        continue; // depth axis is driven only in depth mode
                                    }
                                    const float s = sample.GetElement(a);
                                    const float last = m_unitCubeCursorCell.GetElement(a);
                                    if (s < last - hysteresis || s >= last + 1.0f + hysteresis)
                                    {
                                        m_unitCubeCursorCell.SetElement(a, std::floor(s));
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        // DEPTH: project onto a camera-facing plane through the anchor so the
                        // cursor pulls the block out/in along the surface normal.
                        AZ::Vector3 planeN = rayDir - up * rayDir.Dot(up);
                        if (planeN.GetLengthSq() > 1e-4f)
                        {
                            planeN.Normalize();
                            float t = 0.f;
                            AZ::Intersect::IntersectRayPlane(rayOrigin, rayDir, m_unitCubeAnchorWorld, planeN, t);
                            if (t > 0.f)
                            {
                                const AZ::Vector3 hit = rayOrigin + rayDir * t;
                                if (!m_unitCubeDragMoved &&
                                    AZStd::abs((hit - m_unitCubeAnchorWorld).Dot(up)) > m_unitCubeCellSize)
                                {
                                    m_unitCubeDragMoved = true;
                                }
                                if (m_unitCubeDragMoved)
                                {
                                    const float depthCells = (hit - m_unitCubeAnchorWorld).Dot(up) / m_unitCubeCellSize;
                                    const float anchorAxis = m_unitCubeAnchorMin.GetElement(m_unitCubeAxis);
                                    const float target = anchorAxis + static_cast<float>(m_unitCubeSign) * depthCells;
                                    const float last = m_unitCubeCursorCell.GetElement(m_unitCubeAxis);
                                    if (target < last - hysteresis || target >= last + 1.0f + hysteresis)
                                    {
                                        m_unitCubeCursorCell.SetElement(m_unitCubeAxis, std::floor(target));
                                    }
                                }
                            }
                        }
                    }

                    m_unitCubeHoverValid = true;
                    const AZ::Vector3 prevMin = m_unitCubeMinLocal;
                    const AZ::Vector3 prevExt = m_unitCubeExtent;
                    UpdateUnitCubeRegion(m_unitCubeCursorCell);

                    // Only log when the previewed region actually changes (avoid per-frame spam).
                    if (!m_unitCubeMinLocal.IsClose(prevMin) || !m_unitCubeExtent.IsClose(prevExt))
                    {
                        
                    }
                    return true; // keep the drag captured so the preview follows the cursor
                }

                // Not dragging: single-cube hover preview under the cursor.
                AZ::Vector3 hitNormal;
                const AZ::Vector3 hitWorld = RaycastToSurface(mi, worldFromLocal, intersectionData, hitNormal);
                AZ::Vector3 baseMin;
                const auto snapTarget = SnapTargetUnderCursor(mi.m_interactionId.m_viewportId);
                m_unitCubeHoverValid =
                    UnitCubeCell(worldFromLocal, hitWorld, hitNormal, m_unitCubeCarve, baseMin, snapTarget);
                SnapUtil::SetActiveSnapTarget(snapTarget);
                m_unitCubeCarve = liveCarve;
                m_unitCubeAnchorMin = baseMin;
                m_unitCubeCursorCell = baseMin;
                UnitCubeAxisFromNormal(worldFromLocal, hitNormal, m_unitCubeAxis, m_unitCubeSign);
                UpdateUnitCubeRegion(baseMin);
                return false; // let hover/camera pass through
            }

            // Release commits the previewed region.
            if (leftUp && m_unitCubeDragging)
            {
                m_unitCubeDragging = false;
                
                if (m_unitCubeHoverValid)
                {
                    StampUnitCubeRegion();
                }

                SnapUtil::ClearActiveSnapTarget();
                return true;
            }

            return false; // let other buttons pass through (camera etc.)
        }
        m_unitCubeHoverValid = false;
        m_unitCubeDragging = false;
        m_unitCubeDragMoved = false;
        m_unitCubeAcrossGrow = false;
        // left unit cube mode entirely - drop any lingering snap highlight
        SnapUtil::ClearActiveSnapTarget();

        if (rightDown)
        {
            const bool active = m_state != DrawState::Idle;
            Cancel();
            return active;
        }

        switch (m_state)
        {
        case DrawState::DrawingPolygon:
        case DrawState::PullingPolygonHeight:
            return true; // Handled before the rectangle/height workflow.
        case DrawState::Idle:
        {
            if (leftDown)
            {
                // Ctrl+click = carve a hole through the polygon under the cursor.
                m_carveMode = mi.m_keyboardModifiers.Ctrl() || CurrentCarve();   // Ctrl OR the Carve toggle
                AZ::Vector3 hitNormal;
                const AZ::Vector3 hitWorld = RaycastToSurface(mi, worldFromLocal, intersectionData, hitNormal);
                m_surfaceNormal = hitNormal;            // remember the surface orientation

                // Snap the anchor onto a vertex under the cursor; the orientation still comes from the hit surface.
                const auto anchorSnap = SnapTargetUnderCursor(mi.m_interactionId.m_viewportId);
                const AZ::Vector3 anchorWorld =
                    anchorSnap.has_value() ? *anchorSnap : SnapToGrid(hitWorld, hitWorld, mi.m_interactionId.m_viewportId);
                SnapUtil::SetActiveSnapTarget(anchorSnap);

                m_groundZ = anchorWorld.GetZ();
                m_worldP0 = anchorWorld;
                m_worldP1 = anchorWorld;
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

                // Snap the far corner too; it is projected onto the base plane below.
                const auto cornerSnap = SnapTargetUnderCursor(mi.m_interactionId.m_viewportId);
                SnapUtil::SetActiveSnapTarget(cornerSnap);

                // Project the raw hit onto the anchor's surface plane so the base rectangle
                // always lies flat on that surface -- works for horizontal, vertical, or tilted.
                const AZ::Vector3 up = m_surfaceNormal.GetNormalized();
                const AZ::Vector3 cornerWorld =
                    cornerSnap.has_value() ? *cornerSnap : SnapToGrid(rawHit, m_worldP0, mi.m_interactionId.m_viewportId);
                const float distFromPlane = (cornerWorld - m_worldP0).Dot(up);
                const AZ::Vector3 corner = cornerWorld - up * distFromPlane;
                if (corner.IsFinite())
                {
                    m_worldP1 = corner; // a ray grazing the plane keeps the last good corner
                }

                return true;
            }
            if (leftUp)
            {
                // (no SetZ -- P1 is already on the surface plane from the move handler)
                const AZ::Vector3 baseDelta = m_worldP1 - m_worldP0;

                // Reject a near-zero base measured IN THE SURFACE PLANE, not by world XZ.
                if (baseDelta.GetLength() < cl_whiteBoxMouseClickDeltaThreshold)
                {
                    Cancel();
                    return false;
                }

                m_height = 0.f;
                if (CurrentShape() == DrawShapeType::Plane)
                {
                    if (!CurrentParametric() || !CommitParametricLayer(worldFromLocal, LayerCombineMode::Separate))
                    {
                        CommitBox(worldFromLocal);
                    }
                    Cancel();
                    return true;
                }
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
                    const float height =
                        SnapHeightToGrid(RaycastToHeightPlane(mi, worldFromLocal, baseCenterWorld), mi.m_interactionId.m_viewportId);
                    if (AZ::IsFiniteFloat(height))
                    {
                        m_height = height;
                    }
                }
                return true;
            }
            if (leftDown)
            {
                // A click also commits (using the typed depth if numeric is active).
                m_numericInput.Reset();
                // the shape is about to be committed (or abandoned) - drop the snap highlight
                SnapUtil::ClearActiveSnapTarget();

                if (AZStd::abs(m_height) < cl_whiteBoxMouseClickDeltaThreshold)
                {
                    Cancel();
                    return false;
                }

                CommitDrawnShape(worldFromLocal);
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
            (CurrentShape() != DrawShapeType::Plane && AZStd::abs(m_height) < 0.0001f))
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
        const size_t vertsBeforeCommit = Api::MeshVertexHandles(*whiteBox).size();
        Detail::BuildShapeSolid(
            *whiteBox, worldFromLocal.GetInverse(), center, uAxis, vAxis, up,
            0.0f, m_height, CurrentShape(), CurrentSides(), EffectiveStairSteps(), CurrentHoleRatio(),
            CurrentTubeSides());

        // Map the new geometry into the active layer's space so it shows where it was drawn.
        EditorWhiteBoxComponentRequestBus::Event(
            m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::MapMeshToActiveLayerSpace, *whiteBox,
            vertsBeforeCommit);

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

    void DrawShapeMode::CommitDrawnShape(const AZ::Transform& worldFromLocal)
    {
        if (CurrentParametric())
        {
            // Carve / Merge become the layer's combine mode.
            LayerCombineMode combineMode = LayerCombineMode::Separate;
            if (m_carveMode)
            {
                combineMode = m_height < 0.0f ? LayerCombineMode::Subtract : LayerCombineMode::Union;
            }
            else if (CurrentMergeUnion())
            {
                combineMode = LayerCombineMode::Union;
            }
            if (CommitParametricLayer(worldFromLocal, combineMode))
            {
                return;
            }
        }

        if (m_carveMode)
        {
            // Ctrl + draw = CSG boolean. Pull in -> carve (subtract), pull out -> add (union).
            BooleanAtPolygon(worldFromLocal, m_height);
        }
        else if (CurrentMergeUnion())
        {
            // Merge Draw Shape toggle: always CSG-union the shape into the mesh.
            BooleanAtPolygon(worldFromLocal, m_height, true);
        }
        else
        {
            CommitBox(worldFromLocal);
        }
    }

    bool DrawShapeMode::CommitParametricLayer(const AZ::Transform& worldFromLocal, const LayerCombineMode combineMode)
    {
        EditorWhiteBoxComponent* component = FindWhiteBoxComponent(m_entityComponentIdPair);
        const DrawShapeType shape = CurrentShape();
        if (component == nullptr || shape == DrawShapeType::Polygon)
        {
            return false;
        }

        AZ::Vector3 right, fwd, up;
        Detail::BasisFromNormal(m_surfaceNormal, right, fwd, up);
        const AZ::Vector3 drag = m_worldP1 - m_worldP0;
        AZ::Vector3 uAxis = right * drag.Dot(right);
        AZ::Vector3 vAxis = fwd * drag.Dot(fwd);
        if (uAxis.GetLength() < 0.0001f || vAxis.GetLength() < 0.0001f ||
            (shape != DrawShapeType::Plane && AZStd::abs(m_height) < 0.0001f))
        {
            return false;
        }
        if (uAxis.Cross(vAxis).Dot(up) < 0.f)
        {
            AZStd::swap(uAxis, vAxis);
        }
        const AZ::Vector3 center = m_worldP0 + (uAxis + vAxis) * 0.5f;
        if (shape == DrawShapeType::Staircase)
        {
            Detail::ApplyStairRotation(uAxis, vAxis, CurrentStairInfo().m_rotation);
        }

        // Parametric shapes are built X = width, Y = depth, Z = height, so the footprint axes are the rotation.
        const AZ::Transform localFromWorld = worldFromLocal.GetInverse();
        AZ::Vector3 axisX = localFromWorld.TransformVector(uAxis);
        AZ::Vector3 axisY = localFromWorld.TransformVector(vAxis);
        AZ::Vector3 axisZ = localFromWorld.TransformVector(up).GetNormalizedSafe();
        AZ::Vector3 position = localFromWorld.TransformPoint(center);
        const float height = localFromWorld.TransformVector(up * m_height).GetLength();

        EditorWhiteBoxComponent::ShapeParams params;
        params.m_shape = shape;
        params.m_width = axisX.GetLength();
        params.m_depth = axisY.GetLength();
        params.m_height = shape == DrawShapeType::Plane ? 0.01f : height;
        params.m_sides = CurrentSides();
        params.m_holeRatio = CurrentHoleRatio();
        params.m_tubeSides = CurrentTubeSides();
        const DrawStairInfo stair = CurrentStairInfo();
        params.m_steps = stair.m_steps;
        params.m_stepsByHeight = stair.m_byHeight;
        params.m_stepHeight = stair.m_stepHeight;

        axisX.NormalizeSafe();
        axisY.NormalizeSafe();
        // Undo a stair-rotation mirror; stairs are symmetric across X.
        if (axisX.Cross(axisY).Dot(axisZ) < 0.0f)
        {
            axisX = -axisX;
        }
        if (m_height < 0.0f)
        {
            const bool upright = shape == DrawShapeType::Room || shape == DrawShapeType::Door ||
                shape == DrawShapeType::CircularStairs;
            if (upright)
            {
                position -= axisZ * height; // always upright, based on the lower plane
            }
            else
            {
                // Pulled inward: flip about Y so stairs still descend along Y.
                axisX = -axisX;
                axisZ = -axisZ;
            }
        }

        const AZ::Quaternion rotation =
            AZ::Quaternion::CreateFromMatrix3x3(AZ::Matrix3x3::CreateFromColumns(axisX, axisY, axisZ)).GetNormalized();
        component->AddParametricShapeLayer(params, position, rotation.GetEulerDegrees(), combineMode);
        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, m_entityComponentIdPair.GetEntityId());
        return true;
    }

    // Apply a CSG boolean using a cutter prism built from the drawn footprint
    // (m_worldP0..m_worldP1) extruded along the surface normal by the pull depth.
    // The PULL DIRECTION picks the operation:
    //   pull INTO the surface (height < 0) -> Subtraction (carve a pocket/hole)
    //   pull OUT of the surface (height > 0) -> Union       (add a boss/extrusion)
    // Uses Manifold rather than the legacy inset/extrude/remove-cap trick, so a
    // carve becomes a closed pocket when shallow and a clean through-hole when it
    // exceeds the wall thickness - automatically, no special-casing.
    void DrawShapeMode::BooleanAtPolygon(const AZ::Transform& worldFromLocal, float height, const bool forceUnion)
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

        // Pull direction decides the operation, unless a union is forced (Merge Draw Shape
        // toggle), in which case the shape is always added regardless of pull direction.
        const bool carve = !forceUnion && (height < 0.f);
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
            baseUp, topUp, CurrentShape(), CurrentSides(), EffectiveStairSteps(), CurrentHoleRatio(),
            CurrentTubeSides());
        ApplyDrawBoolean(*cutter, carve);
    }

    bool DrawShapeMode::ApplyDrawBoolean(WhiteBoxMesh& cutter, const bool carve)
    {
        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);
        if (!whiteBox) { return false; }
        const auto operation = carve ? Api::BooleanOperation::Subtraction : Api::BooleanOperation::Union;
        // Map into the active layer's storage space (it may display transformed) so the
        // carve/boss lands exactly where it was drawn.
        EditorWhiteBoxComponentRequestBus::Event(
            m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::MapMeshToActiveLayerSpace, cutter, size_t{ 0 });
        Api::CalculateNormals(cutter);

        AzToolsFramework::ScopedUndoBatch undoBatch(carve ? "Carve White Box" : "Add White Box");

        // Identity transform: the cutter is in local space. No early out - it may still hit stamped cubes.
        const bool freeformChanged =
            Api::ApplyMeshBoolean(*whiteBox, cutter, AZ::Transform::CreateIdentity(), operation);
        if (freeformChanged)
        {
            Api::CalculateNormals(*whiteBox);
            Api::CalculatePlanarUVs(*whiteBox);
        }

        // A carve also cuts through the stamped-cube grids so shapes can carve cubes.
        bool gridsChanged = false;
        if (carve)
        {
            EditorWhiteBoxComponentRequestBus::EventResult(
                gridsChanged, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::CarveCubeGrids, cutter,
                AZ::Transform::CreateIdentity());
        }

        if (!freeformChanged && !gridsChanged)
        {
            return false; // nothing intersected (neither freeform nor cubes)
        }

        EditorWhiteBoxComponentRequestBus::Event(
            m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::SerializeWhiteBox);
        EditorWhiteBoxComponentRequestBus::Event(
            m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::RebuildWhiteBox);
        EditorWhiteBoxComponentModeRequestBus::Event(
            m_entityComponentIdPair,
            &EditorWhiteBoxComponentModeRequestBus::Events::MarkWhiteBoxIntersectionDataDirty);
        undoBatch.MarkEntityDirty(m_entityComponentIdPair.GetEntityId());
        return true;
    }

    void DrawShapeMode::Cancel()
    {
        m_polygonPoints.clear();
        m_polygonLayerId = 0;
        m_polygonBooleanFailed = false;
        m_polygonInvalid = false;
        m_carveMode = false;
        m_polygonCloseHovered = false;
        m_numericInput.Reset();
        m_state  = DrawState::Idle;
        m_height = 0.f;
        // Escape / right click already return the shape to nothing, so the drag is effectively
        // cancelled here - just drop the snap highlight with it.
        SnapUtil::ClearActiveSnapTarget();
    }

    bool DrawShapeMode::BeginNumericIfPulling()
    {
        // Numeric depth is only meaningful once the base is locked and we're
        // pulling height.
        if (m_state != DrawState::PullingHeight && m_state != DrawState::PullingPolygonHeight)
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
            m_polygonBooleanFailed = false;
        }
    }

    void DrawShapeMode::NumericConfirm()
    {
        if (m_state == DrawState::DrawingPolygon)
        {
            FinishPolygonOutline();
            return;
        }
        if (m_state == DrawState::PullingPolygonHeight)
        {
            if (!PolygonContextValid()) { Cancel(); return; }
            SyncPreviewHeight();
            if (AZStd::abs(m_height) >= 0.0001f) { CommitPolygon(m_height); }
            return;
        }
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

        CommitDrawnShape(m_worldFromLocal);
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

    bool DrawShapeMode::HandleEscape()
    {
        // Only consume the Escape when there is something to cancel - with nothing in progress it
        // has to fall through and leave component mode as usual.
        if (!m_numericInput.IsActive() && m_state == DrawState::Idle)
        {
            return false;
        }

        NumericCancel();
        return true;
    }

    void DrawShapeMode::NumericBackspace()
    {
        if (m_state == DrawState::DrawingPolygon)
        {
            if (m_polygonPoints.size() > 1) { m_polygonPoints.pop_back(); }
            else { Cancel(); }
            m_polygonInvalid = false;
            m_polygonCloseHovered = false;
        }
        else if (m_numericInput.IsActive())
        {
            m_numericInput.Backspace();
            SyncPreviewHeight();
        }
    }

    bool DrawShapeMode::PolygonContextValid() const
    {
        auto* component = FindWhiteBoxComponent(m_entityComponentIdPair);
        return component && component->GetActiveLayerId() == m_polygonLayerId &&
            component->GetDrawShape() == DrawShapeType::Polygon && !component->GetDrawUnitCube();
    }

    bool DrawShapeMode::FinishPolygonOutline()
    {
        if (!PolygonContextValid()) { Cancel(); return false; }
        bool extrude = false;
        EditorWhiteBoxComponentRequestBus::EventResult(
            extrude, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetDrawPolygonExtrude);
        if (!extrude) { return CommitPolygon(); }
        // Validate before entering the height phase. Keep invalid outlines editable.
        auto polygon = Api::CreateWhiteBoxMesh();
        if (!Detail::BuildPolygonFace(*polygon, m_worldFromLocal.GetInverse(), m_polygonPoints, m_surfaceNormal))
        {
            m_polygonInvalid = true;
            return false;
        }
        m_polygonHeightAnchor = m_worldP1;
        m_height = 0.0f;
        m_numericInput.Reset();
        m_polygonCloseHovered = false;
        SnapUtil::ClearActiveSnapTarget();
        m_state = DrawState::PullingPolygonHeight;
        return true;
    }

    bool DrawShapeMode::CommitPolygon(const float height)
    {
        if (!PolygonContextValid()) { Cancel(); return false; }
        const bool directionalBoolean = m_carveMode || CurrentCarve();
        const bool boolean = height != 0.0f && (directionalBoolean || CurrentMergeUnion());
        const bool carve = directionalBoolean && height < 0.0f;
        auto points = m_polygonPoints;
        float solidHeight = height;
        if (boolean)
        {
            // Match the primitive cutters' slight surface overlap, while retaining the actual
            // signed pull direction (including inward extrusions with Merge enabled).
            float span = 0.0f;
            for (const auto& point : points) { span = AZStd::max(span, (point - points.front()).GetLength()); }
            const float epsilon = AZStd::max(span * 0.02f, 0.001f);
            const float direction = height < 0.0f ? -1.0f : 1.0f;
            for (auto& point : points) { point -= m_surfaceNormal * (direction * epsilon); }
            solidHeight += direction * (2.0f * epsilon);
        }
        auto polygon = Api::CreateWhiteBoxMesh();
        if (!Detail::BuildPolygonFace(*polygon, m_worldFromLocal.GetInverse(), points, m_surfaceNormal, solidHeight))
        {
            m_polygonInvalid = true;
            return false;
        }
        WhiteBoxMesh* mesh = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            mesh, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);
        if (!mesh) { return false; }
        if (boolean)
        {
            if (!ApplyDrawBoolean(*polygon, carve))
            {
                m_polygonBooleanFailed = true;
                return false; // Preserve the outline and depth so the user can adjust or cancel.
            }
            Cancel();
            return true;
        }
        AzToolsFramework::ScopedUndoBatch undo(height == 0.0f ? "Draw White Box Polygon" : "Draw Extruded White Box Polygon");
        const size_t firstVertex = Api::MeshVertexHandles(*mesh).size();
        AppendMesh(*mesh, *polygon);
        EditorWhiteBoxComponentRequestBus::Event(
            m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::MapMeshToActiveLayerSpace, *mesh, firstVertex);
        Api::CalculateNormals(*mesh);
        Api::CalculatePlanarUVs(*mesh);
        EditorWhiteBoxComponentRequestBus::Event(m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::SerializeWhiteBox);
        EditorWhiteBoxComponentRequestBus::Event(m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::RebuildWhiteBox);
        EditorWhiteBoxComponentModeRequestBus::Event(
            m_entityComponentIdPair, &EditorWhiteBoxComponentModeRequests::MarkWhiteBoxIntersectionDataDirty);
        undo.MarkEntityDirty(m_entityComponentIdPair.GetEntityId());
        Cancel();
        return true;
    }

    float DrawShapeMode::CurrentHoleRatio() const
    {
        float ratio = 0.5f;
        EditorWhiteBoxComponentRequestBus::EventResult(
            ratio, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetDrawHoleRatio);
        return AZStd::clamp(ratio, 0.05f, 0.95f);
    }

    int DrawShapeMode::CurrentTubeSides() const
    {
        int tubeSides = DefaultTubeSides;
        EditorWhiteBoxComponentRequestBus::EventResult(
            tubeSides, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetDrawTubeSides);
        return AZStd::clamp(tubeSides, MinTubeSides, MaxTubeSides);
    }

    // The font projects labels itself and asserts on a point it cannot place, so broken or behind-camera positions are skipped.
    static void DrawLabel(
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Vector3& position,
        const float size, const char* text)
    {
        if (!position.IsFinite())
        {
            return;
        }
        const AzFramework::CameraState camera = AzToolsFramework::GetCameraState(viewportInfo.m_viewportId);
        if ((position - camera.m_position).Dot(camera.m_forward) <= camera.m_nearClip)
        {
            return;
        }
        debugDisplay.DrawTextLabel(position, size, text, true, 0, 0);
    }

    void DrawShapeMode::DisplayViewport(
        const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if ((m_state == DrawState::DrawingPolygon || m_state == DrawState::PullingPolygonHeight) &&
            !PolygonContextValid())
        {
            Cancel();
        }
        if (m_state == DrawState::PullingPolygonHeight && !m_polygonPoints.empty())
        {
            const auto offset = m_surfaceNormal * m_height;
            debugDisplay.DepthTestOff();
            debugDisplay.SetColor(AZ::Color(0.3f, 1.0f, 0.5f, 1.0f));
            for (size_t i = 0; i < m_polygonPoints.size(); ++i)
            {
                const auto& a = m_polygonPoints[i];
                const auto& b = m_polygonPoints[(i + 1) % m_polygonPoints.size()];
                debugDisplay.DrawLine(a, b);
                debugDisplay.DrawLine(a + offset, b + offset);
                debugDisplay.DrawLine(a, a + offset);
            }
            const bool directionalBoolean = m_carveMode || CurrentCarve();
            const char* operation = directionalBoolean
                ? (m_height < 0.0f ? "Carve" : "Union")
                : (CurrentMergeUnion() ? "Union" : "Extrude");
            const auto depthText = m_numericInput.IsActive() ? m_numericInput.GetStatusText() :
                AZStd::string::format("Depth %.3f", m_height);
            const auto label = m_polygonBooleanFailed
                ? AZStd::string("Boolean not applied: check overlap / closed target mesh, adjust depth or Esc to cancel")
                : AZStd::string::format("%s | %s | Click / Enter finish | Esc cancel", operation, depthText.c_str());
            DrawLabel(viewportInfo, debugDisplay, m_polygonHeightAnchor + offset, 1.0f, label.c_str());
            debugDisplay.DepthTestOn();
            return;
        }
        if (m_state == DrawState::DrawingPolygon && !m_polygonPoints.empty())
        {
            debugDisplay.DepthTestOff();
            debugDisplay.SetColor(m_polygonInvalid ? AZ::Color(1.0f, 0.2f, 0.2f, 1.0f) : AZ::Color(0.3f, 1.0f, 0.5f, 1.0f));
            for (size_t i = 1; i < m_polygonPoints.size(); ++i)
            {
                debugDisplay.DrawLine(m_polygonPoints[i - 1], m_polygonPoints[i]);
            }
            debugDisplay.DrawLine(m_polygonPoints.back(), m_worldP1);
            debugDisplay.DrawLine(m_worldP1, m_polygonPoints.front());
            const auto camera = AzToolsFramework::GetCameraState(viewportInfo.m_viewportId);
            const float markerRadius = 0.08f *
                AzToolsFramework::CalculateScreenToWorldMultiplier(m_worldP0, camera);
            debugDisplay.SetColor(m_polygonCloseHovered
                ? AZ::Color(1.0f, 0.85f, 0.15f, 1.0f) : AZ::Color(0.3f, 1.0f, 0.5f, 1.0f));
            debugDisplay.DrawBall(m_worldP0, markerRadius);
            DrawLabel(viewportInfo, debugDisplay, m_worldP0, 1.0f, m_polygonInvalid
                ? "Invalid outline: remove crossing/duplicate corners with Backspace"
                : (m_polygonCloseHovered ? "Click to close polygon"
                    : "Click corners | Enter or click first corner to finish | Backspace undo | Esc cancel"));
            debugDisplay.DepthTestOn();
            return;
        }
        // Unit-cube stamp hover ghost, showing the individual cubes.
        if (m_unitCubeHoverValid)
        {
            const int mx = static_cast<int>(std::lround(m_unitCubeMinLocal.GetX()));
            const int my = static_cast<int>(std::lround(m_unitCubeMinLocal.GetY()));
            const int mz = static_cast<int>(std::lround(m_unitCubeMinLocal.GetZ()));
            const int nx = AZ::GetMax(1, static_cast<int>(std::lround(m_unitCubeExtent.GetX())));
            const int ny = AZ::GetMax(1, static_cast<int>(std::lround(m_unitCubeExtent.GetY())));
            const int nz = AZ::GetMax(1, static_cast<int>(std::lround(m_unitCubeExtent.GetZ())));

            const AZ::Color color =
                m_unitCubeCarve ? AZ::Color(1.0f, 0.25f, 0.25f, 1.0f) : AZ::Color(0.3f, 1.0f, 0.3f, 1.0f);
            debugDisplay.DepthTestOff();
            debugDisplay.SetColor(color);
            debugDisplay.SetLineWidth(static_cast<float>(cl_whiteBoxEdgeVisualWidth));

            const auto cellToWorld = [&](int cx, int cy, int cz)
            {
                return m_worldFromLocal.TransformPoint(
                    AZ::Vector3(static_cast<float>(cx), static_cast<float>(cy), static_cast<float>(cz)) * m_unitCubeCellSize);
            };

            // Grid lines at Cube Size boundaries (one cell per stamped cube), capped for large blocks.
            const int step = AZ::GetMax(1, m_unitCubeSize);
            const int cubesX = nx / step;
            const int cubesY = ny / step;
            const int cubesZ = nz / step;
            const bool showGrid = UnitCubeShowGrid() &&
                (static_cast<AZ::s64>(cubesX + 1) * (cubesY + 1) * (cubesZ + 1)) <= 8192;

            if (showGrid)
            {
                for (int j = 0; j <= ny; j += step)
                {
                    for (int k = 0; k <= nz; k += step)
                    {
                        debugDisplay.DrawLine(cellToWorld(mx, my + j, mz + k), cellToWorld(mx + nx, my + j, mz + k));
                    }
                }
                for (int i = 0; i <= nx; i += step)
                {
                    for (int k = 0; k <= nz; k += step)
                    {
                        debugDisplay.DrawLine(cellToWorld(mx + i, my, mz + k), cellToWorld(mx + i, my + ny, mz + k));
                    }
                }
                for (int i = 0; i <= nx; i += step)
                {
                    for (int j = 0; j <= ny; j += step)
                    {
                        debugDisplay.DrawLine(cellToWorld(mx + i, my + j, mz), cellToWorld(mx + i, my + j, mz + nz));
                    }
                }
            }
            else
            {
                AZ::Vector3 c[8];
                for (int i = 0; i < 8; ++i)
                {
                    c[i] = cellToWorld(
                        mx + ((i & 1) ? nx : 0), my + ((i >> 1) & 1 ? ny : 0), mz + ((i >> 2) & 1 ? nz : 0));
                }
                const int edges[12][2] = {
                    {0, 1}, {2, 3}, {4, 5}, {6, 7}, {0, 2}, {1, 3}, {4, 6}, {5, 7}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
                for (const auto& e : edges)
                {
                    debugDisplay.DrawLine(c[e[0]], c[e[1]]);
                }
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
        if (shape == DrawShapeType::Plane || shape == DrawShapeType::Pipe || shape == DrawShapeType::Torus)
        {
            debugDisplay.DepthTestOff();
            debugDisplay.SetColor(AZ::Color(0.3f, 1.0f, 0.5f, 1.0f));
            const auto point = [&](float x, float y, float z)
            {
                return center + uAxis * (0.5f * x) + vAxis * (0.5f * y) + up * z;
            };
            if (shape == DrawShapeType::Plane)
            {
                const AZ::Vector3 corners[] = {point(-1, -1, 0), point(1, -1, 0), point(1, 1, 0), point(-1, 1, 0)};
                for (int i = 0; i < 4; ++i) { debugDisplay.DrawLine(corners[i], corners[(i + 1) % 4]); }
            }
            else
            {
                const int n = CurrentSides();
                const float hole = CurrentHoleRatio();
                const int tubeSides = AZStd::clamp(n / 2, 8, 32);
                const auto ringPoint = [&](int i, float radius, float z)
                {
                    const float a = AZ::Constants::TwoPi * i / n;
                    return point(radius * std::cos(a), radius * std::sin(a), z);
                };
                for (int i = 0; i < n; ++i)
                {
                    if (shape == DrawShapeType::Pipe)
                    {
                        for (float radius : {hole, 1.0f})
                        {
                            debugDisplay.DrawLine(ringPoint(i, radius, 0), ringPoint(i + 1, radius, 0));
                            debugDisplay.DrawLine(ringPoint(i, radius, m_height), ringPoint(i + 1, radius, m_height));
                            debugDisplay.DrawLine(ringPoint(i, radius, 0), ringPoint(i, radius, m_height));
                        }
                    }
                    else
                    {
                        const auto torusPoint = [&](int ring, int tube)
                        {
                            const float a = AZ::Constants::TwoPi * tube / tubeSides;
                            return ringPoint(ring, (1 + hole) * 0.5f + (1 - hole) * 0.5f * std::cos(a),
                                m_height * (0.5f + 0.5f * std::sin(a)));
                        };
                        for (int j = 0; j < tubeSides; ++j)
                        {
                            debugDisplay.DrawLine(torusPoint(i, j), torusPoint(i + 1, j));
                            debugDisplay.DrawLine(torusPoint(i, j), torusPoint(i, j + 1));
                        }
                    }
                }
            }
            if (m_numericInput.IsActive())
            {
                DrawLabel(viewportInfo, debugDisplay, center + up * m_height, 1.0f, m_numericInput.GetStatusText().c_str());
            }
            debugDisplay.DepthTestOn();
            return;
        }
        // Staircase: re-orient run/width within the same footprint for the preview.
        if (shape == DrawShapeType::Staircase)
        {
            Detail::ApplyStairRotation(uAxis, vAxis, CurrentStairInfo().m_rotation);
        }

        const AZ::Vector3 ru     = uAxis * 0.5f;
        const AZ::Vector3 rv     = vAxis * 0.5f;
        const AZ::Vector3 ext    = up * m_height;   // signed -- preview follows pull direction

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
            DrawLabel(viewportInfo, debugDisplay, center - rv - vHat * gap, 1.1f, AZStd::string::format("%.3f", width).c_str());
            debugDisplay.SetColor(lengthColor);
            DrawLabel(viewportInfo, debugDisplay, center - ru - uHat * gap, 1.1f, AZStd::string::format("%.3f", length).c_str());

            if (pullingHeight)
            {
                const AZ::Vector3 hBase = center + ru + rv;    // far corner rises with the pull
                debugDisplay.SetColor(heightColor);
                debugDisplay.DrawLine(hBase, hBase + ext);     // height edge
                DrawLabel(viewportInfo, debugDisplay,
                    hBase + ext * 0.5f + (uHat + vHat) * (gap * 0.5f), 1.1f,
                    AZStd::string::format("%.3f", height).c_str());
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
            DrawLabel(viewportInfo, debugDisplay, labelPos, 1.3f, status.c_str());
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

    bool DrawShapeMode::CurrentParametric() const
    {
        const EditorWhiteBoxComponent* component = FindWhiteBoxComponent(m_entityComponentIdPair);
        return component != nullptr && component->GetDrawParametric();
    }

    bool DrawShapeMode::CurrentCarve() const
    {
        bool carve = false;
        EditorWhiteBoxComponentRequestBus::EventResult(
            carve, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetDrawCarve);
        return carve;
    }

    bool DrawShapeMode::CurrentMergeUnion() const
    {
        bool mergeUnion = false;
        EditorWhiteBoxComponentRequestBus::EventResult(
            mergeUnion, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetDrawMergeUnion);
        return mergeUnion;
    }

    bool DrawShapeMode::UnitCubeMode() const
    {
        bool unitCube = false;
        EditorWhiteBoxComponentRequestBus::EventResult(
            unitCube, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetDrawUnitCube);
        return unitCube;
    }

    int DrawShapeMode::CurrentUnitCubeSize() const
    {
        // Each stamped cube is exactly one grid cell now, so the block is measured in
        // whole cubes at 1 cell each.
        return 1;
    }

    float DrawShapeMode::CurrentCellSize() const
    {
        // World-space size of one cube/cell (the grid spacing) from the component.
        float size = 1.0f;
        EditorWhiteBoxComponentRequestBus::EventResult(
            size, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetDrawUnitCubeSize);
        return size < 0.05f ? 0.05f : size;
    }

    bool DrawShapeMode::UnitCubeShowGrid() const
    {
        bool showGrid = true;
        EditorWhiteBoxComponentRequestBus::EventResult(
            showGrid, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetDrawUnitCubeShowGrid);
        return showGrid;
    }

    AZStd::optional<AZ::Vector3> DrawShapeMode::SnapTargetUnderCursor(const int viewportId) const
    {
        // Nothing is excluded: the stamp is new geometry, so every existing vertex - including
        // this mesh's own - is a legitimate target.
        return SnapUtil::FindSnapTargetWorld(m_entityComponentIdPair.GetEntityId(), viewportId, {});
    }

    AZ::Vector3 DrawShapeMode::SnapToGrid(const AZ::Vector3& world, const AZ::Vector3& planePoint, const int viewportId) const
    {
        const AzToolsFramework::GridSnapParameters grid = AzToolsFramework::GridSnapSettings(viewportId);
        if (!grid.m_gridSnap || grid.m_gridSize <= AZ::Constants::FloatEpsilon)
        {
            return world;
        }
        const float size = grid.m_gridSize;
        const auto round = [size](const float value)
        {
            return std::round(value / size) * size;
        };
        // The world grid the viewport draws, so shapes line up with it and with each other.
        AZ::Vector3 snapped(round(world.GetX()), round(world.GetY()), round(world.GetZ()));
        // Back onto the drawing plane: on a floor or a wall only the in-plane coordinates change.
        const AZ::Vector3 normal = m_surfaceNormal.GetNormalizedSafe();
        if (!normal.IsZero())
        {
            snapped -= normal * (snapped - planePoint).Dot(normal);
        }
        return snapped;
    }

    float DrawShapeMode::SnapHeightToGrid(const float height, const int viewportId) const
    {
        const AzToolsFramework::GridSnapParameters grid = AzToolsFramework::GridSnapSettings(viewportId);
        if (!grid.m_gridSnap || grid.m_gridSize <= AZ::Constants::FloatEpsilon)
        {
            return height;
        }
        return std::round(height / grid.m_gridSize) * grid.m_gridSize;
    }

    AZ::Vector3 DrawShapeMode::SnapPolygonPoint(const AZ::Vector3& onPlane, const int viewportId) const
    {
        const float step = AzToolsFramework::AngleStep(viewportId);
        if (m_polygonPoints.empty() || !AzToolsFramework::AngleSnapping(viewportId) || step < 0.01f)
        {
            return SnapToGrid(onPlane, m_worldP0, viewportId);
        }
        // Angles are measured in the drawing plane from its own right axis, the same basis the other shapes use.
        AZ::Vector3 right, fwd, up;
        Detail::BasisFromNormal(m_surfaceNormal, right, fwd, up);
        const AZ::Vector3& previous = m_polygonPoints.back();
        const AZ::Vector3 delta = onPlane - previous;
        const float across = delta.Dot(right);
        const float along = delta.Dot(fwd);
        float length = std::sqrt(across * across + along * along);
        if (length < 1e-6f)
        {
            return onPlane;
        }
        const float stepRadians = AZ::DegToRad(step);
        const float angle = std::round(std::atan2(along, across) / stepRadians) * stepRadians;
        const AzToolsFramework::GridSnapParameters grid = AzToolsFramework::GridSnapSettings(viewportId);
        if (grid.m_gridSnap && grid.m_gridSize > AZ::Constants::FloatEpsilon)
        {
            length = std::round(length / grid.m_gridSize) * grid.m_gridSize;
        }
        return previous + (right * std::cos(angle) + fwd * std::sin(angle)) * length;
    }

    bool DrawShapeMode::UnitCubeCell(
        const AZ::Transform& worldFromLocal, const AZ::Vector3& hitWorld, const AZ::Vector3& hitNormal, const bool carve,
        AZ::Vector3& outMinLocal, const AZStd::optional<AZ::Vector3>& snapTargetWorld) const
    {
        const AZ::Transform localFromWorld = worldFromLocal.GetInverse();

        // Snap the cell corner onto the target, rounded (a corner, not a point inside a cell).
        if (snapTargetWorld.has_value())
        {
            const AZ::Vector3 snapCell = localFromWorld.TransformPoint(snapTargetWorld.value()) / m_unitCubeCellSize;
            outMinLocal = AZ::Vector3(
                std::round(snapCell.GetX()), std::round(snapCell.GetY()), std::round(snapCell.GetZ()));
            return true;
        }

        // Work in grid-cell index space (local units divided by the cell size).
        const AZ::Vector3 localHitCell = localFromWorld.TransformPoint(hitWorld) / m_unitCubeCellSize;
        const AZ::Vector3 localNormal =
            AzToolsFramework::TransformDirectionNoScaling(localFromWorld, hitNormal).GetNormalizedSafe();

        // Nudge half a cell off the surface to pick a cell unambiguously:
        //   add   -> the empty cell on the +normal side (where the new cube appears)
        //   carve -> the solid cell on the -normal side (the block you clicked)
        const AZ::Vector3 sample = localHitCell + localNormal * (carve ? -0.5f : 0.5f);
        outMinLocal = AZ::Vector3(std::floor(sample.GetX()), std::floor(sample.GetY()), std::floor(sample.GetZ()));
        return true;
    }

    void DrawShapeMode::UnitCubeAxisFromNormal(
        const AZ::Transform& worldFromLocal, const AZ::Vector3& worldNormal, int& outAxis, int& outSign) const
    {
        const AZ::Transform localFromWorld = worldFromLocal.GetInverse();
        const AZ::Vector3 n =
            AzToolsFramework::TransformDirectionNoScaling(localFromWorld, worldNormal).GetNormalizedSafe();
        const float ax = AZStd::abs(n.GetX());
        const float ay = AZStd::abs(n.GetY());
        const float az = AZStd::abs(n.GetZ());
        if (ax >= ay && ax >= az)
        {
            outAxis = 0;
            outSign = n.GetX() >= 0.f ? 1 : -1;
        }
        else if (ay >= az)
        {
            outAxis = 1;
            outSign = n.GetY() >= 0.f ? 1 : -1;
        }
        else
        {
            outAxis = 2;
            outSign = n.GetZ() >= 0.f ? 1 : -1;
        }
    }

    void DrawShapeMode::UpdateUnitCubeRegion(const AZ::Vector3& cursorCell)
    {
        const int cube = m_unitCubeSize < 1 ? 1 : m_unitCubeSize;
        const auto toCell = [](float v) { return static_cast<int>(std::lround(v)); };
        const int anchor[3] = {
            toCell(m_unitCubeAnchorMin.GetX()), toCell(m_unitCubeAnchorMin.GetY()), toCell(m_unitCubeAnchorMin.GetZ())};
        const int cur[3] = {toCell(cursorCell.GetX()), toCell(cursorCell.GetY()), toCell(cursorCell.GetZ())};
        const int step = m_unitCubeCarve ? -m_unitCubeSign : m_unitCubeSign;

        int boxMin[3];
        int ext[3];
        for (int a = 0; a < 3; ++a)
        {
            // The fixed base cube along this axis. The normal axis runs "cube" cells
            // outward (add) or inward (carve); footprint axes run "cube" cells in +.
            int baseA;
            int baseB;
            if (a == m_unitCubeAxis)
            {
                baseA = anchor[a];
                baseB = anchor[a] + step * (cube - 1);
            }
            else
            {
                baseA = anchor[a];
                baseB = anchor[a] + (cube - 1);
            }
            const int baseLo = AZStd::min(baseA, baseB);
            const int baseHi = AZStd::max(baseA, baseB);

            // Grow toward the cursor in whole cubes; the base cube is always kept.
            int lo = baseLo;
            int hi = baseHi;
            if (cur[a] > baseHi)
            {
                const int cubes = (cur[a] - baseHi + cube - 1) / cube; // whole cubes past the base
                hi = baseHi + cubes * cube;
            }
            else if (cur[a] < baseLo)
            {
                const int cubes = (baseLo - cur[a] + cube - 1) / cube;
                lo = baseLo - cubes * cube;
            }
            boxMin[a] = lo;
            ext[a] = hi - lo + 1;
        }

        m_unitCubeMinLocal = AZ::Vector3(
            static_cast<float>(boxMin[0]), static_cast<float>(boxMin[1]), static_cast<float>(boxMin[2]));
        m_unitCubeExtent =
            AZ::Vector3(static_cast<float>(ext[0]), static_cast<float>(ext[1]), static_cast<float>(ext[2]));
    }

    void DrawShapeMode::StampUnitCubeRegion()
    {
        const auto toCount = [](float v) { return AZStd::max(0, static_cast<int>(std::lround(v))); };
        const int nx = toCount(m_unitCubeExtent.GetX());
        const int ny = toCount(m_unitCubeExtent.GetY());
        const int nz = toCount(m_unitCubeExtent.GetZ());
        if (nx == 0 || ny == 0 || nz == 0)
        {
            return;
        }

        AZStd::vector<AZ::Vector3> cells;
        cells.reserve(static_cast<size_t>(nx) * static_cast<size_t>(ny) * static_cast<size_t>(nz));
        for (int x = 0; x < nx; ++x)
        {
            for (int y = 0; y < ny; ++y)
            {
                for (int z = 0; z < nz; ++z)
                {
                    cells.push_back(
                        m_unitCubeMinLocal +
                        AZ::Vector3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)));
                }
            }
        }

        

        // Fill (add) or clear (carve) every cell in one undo batch. The component
        // regenerates a clean, watertight surface (coplanar faces grouped) from its voxel set.
        EditorWhiteBoxComponentRequestBus::Event(
            m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::SetVoxelCells, cells, !m_unitCubeCarve);
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
