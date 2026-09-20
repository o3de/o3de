/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include "WhiteBoxShapeBuilders.h"
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/array.h>
#include <cmath>

namespace WhiteBox::Detail
{
    void BuildAdditionalShape(
        WhiteBoxMesh& mesh, const AZ::Transform& localFromWorld, const AZ::Vector3& center,
        const AZ::Vector3& uAxis, const AZ::Vector3& vAxis, const AZ::Vector3& up,
        float baseUp, float topUp, DrawShapeType shape, int sides, float holeRatio, int tubeSides)
    {
        const int n = AZStd::clamp(sides, 3, 128);
        const float hole = AZStd::clamp(holeRatio, 0.05f, 0.95f);
        AZStd::vector<AZ::Vector3> positions;
        AZStd::vector<Api::VertexHandle> handles;
        const auto add = [&](const AZ::Vector3& p)
        {
            positions.push_back(localFromWorld.TransformPoint(p));
            handles.push_back(Api::AddVertex(mesh, positions.back()));
        };
        const bool flip = uAxis.Cross(vAxis).Dot(up) * (shape == DrawShapeType::Plane ? 1.0f : topUp - baseUp) < 0.0f;
        const auto quad = [&](int a, int b, int c, int d)
        {
            if (flip) { AZStd::swap(b, d); }
            Api::AddQuadPolygon(mesh, handles[a], handles[b], handles[c], handles[d]);
        };
        if (shape == DrawShapeType::Plane)
        {
            add(center - uAxis * 0.5f - vAxis * 0.5f + up * baseUp);
            add(center + uAxis * 0.5f - vAxis * 0.5f + up * baseUp);
            add(center + uAxis * 0.5f + vAxis * 0.5f + up * baseUp);
            add(center - uAxis * 0.5f + vAxis * 0.5f + up * baseUp);
            quad(0, 1, 2, 3);
            return;
        }
        if (shape == DrawShapeType::Pipe)
        {
            for (int ring = 0; ring < 4; ++ring)
            {
                const float radius = ring % 2 == 0 ? 1.0f : hole;
                const float height = ring < 2 ? baseUp : topUp;
                for (int i = 0; i < n; ++i)
                {
                    const float angle = AZ::Constants::TwoPi * i / n;
                    add(center + (uAxis * std::cos(angle) + vAxis * std::sin(angle)) * (0.5f * radius) + up * height);
                }
            }
            for (int i = 0; i < n; ++i)
            {
                const int j = (i + 1) % n;
                quad(i, j, 2 * n + j, 2 * n + i); // outer wall
                quad(n + j, n + i, 3 * n + i, 3 * n + j); // inner wall
                quad(i, n + i, n + j, j); // bottom annulus
                quad(2 * n + j, 3 * n + j, 3 * n + i, 2 * n + i); // top annulus
            }
            return;
        }
        // Elliptical torus: Width/Depth are outer diameters, Height the tube height, tube tessellation separate.
        const int tubeCount = AZStd::clamp(tubeSides, MinTubeSides, MaxTubeSides);
        const float major = (1.0f + hole) * 0.5f;
        const float minor = (1.0f - hole) * 0.5f;
        for (int i = 0; i < n; ++i)
        {
            const float angle = AZ::Constants::TwoPi * i / n;
            for (int j = 0; j < tubeCount; ++j)
            {
                const float tube = AZ::Constants::TwoPi * j / tubeCount;
                const float radius = major + minor * std::cos(tube);
                add(center + (uAxis * std::cos(angle) + vAxis * std::sin(angle)) * (0.5f * radius)
                    + up * ((baseUp + topUp) * 0.5f + (topUp - baseUp) * 0.5f * std::sin(tube)));
            }
        }
        for (int i = 0; i < n; ++i)
        {
            for (int j = 0; j < tubeCount; ++j)
            {
                const int nextI = (i + 1) % n;
                const int nextJ = (j + 1) % tubeCount;
                quad(i * tubeCount + j, nextI * tubeCount + j, nextI * tubeCount + nextJ, i * tubeCount + nextJ);
            }
        }
    }

    bool BuildPolygonFace(
        WhiteBoxMesh& mesh, const AZ::Transform& localFromWorld,
        const AZStd::vector<AZ::Vector3>& points, const AZ::Vector3& normal, const float extrusionHeight)
    {
        if (!AZ::IsFiniteFloat(extrusionHeight)) { return false; }
        if (points.size() < 3 || points.size() > 256) { return false; }
        AZ::Vector3 right, forward, up;
        BasisFromNormal(normal, right, forward, up);
        AZStd::vector<AZ::Vector2> flat;
        float scale = 0.0f;
        for (const auto& p : points)
        {
            const auto delta = p - points.front();
            flat.emplace_back(delta.Dot(right), delta.Dot(forward));
            scale = AZStd::max(scale, flat.back().GetLength());
            if (AZStd::abs(delta.Dot(up)) > 0.001f) { return false; }
        }
        if (scale < 0.0001f) { return false; }
        for (auto& p : flat) { p /= scale; }
        constexpr float epsilon = 1e-7f;
        const auto cross = [](const AZ::Vector2& a, const AZ::Vector2& b, const AZ::Vector2& c)
        {
            const auto ab = b - a;
            const auto ac = c - a;
            return ab.GetX() * ac.GetY() - ab.GetY() * ac.GetX();
        };
        const auto onSegment = [&](const AZ::Vector2& a, const AZ::Vector2& b, const AZ::Vector2& p)
        {
            return AZStd::abs(cross(a, b, p)) <= epsilon &&
                p.GetX() >= AZStd::min(a.GetX(), b.GetX()) - epsilon &&
                p.GetX() <= AZStd::max(a.GetX(), b.GetX()) + epsilon &&
                p.GetY() >= AZStd::min(a.GetY(), b.GetY()) - epsilon &&
                p.GetY() <= AZStd::max(a.GetY(), b.GetY()) + epsilon;
        };
        const size_t count = points.size();
        float area = 0.0f;
        for (size_t i = 0; i < count; ++i)
        {
            const auto& a = flat[i];
            const auto& b = flat[(i + 1) % count];
            area += a.GetX() * b.GetY() - b.GetX() * a.GetY();
            for (size_t j = i + 1; j < count; ++j)
            {
                if ((a - flat[j]).GetLengthSq() < epsilon * epsilon) { return false; }
                if (j == i + 1 || (i == 0 && j == count - 1)) { continue; }
                const auto& c = flat[j];
                const auto& d = flat[(j + 1) % count];
                if ((cross(a, b, c) * cross(a, b, d) < 0.0f && cross(c, d, a) * cross(c, d, b) < 0.0f) ||
                    onSegment(a, b, c) || onSegment(a, b, d) || onSegment(c, d, a) || onSegment(c, d, b))
                {
                    return false;
                }
            }
        }
        if (AZStd::abs(area) < epsilon) { return false; }
        AZStd::vector<size_t> remaining;
        for (size_t i = 0; i < count; ++i) { remaining.push_back(area > 0.0f ? i : count - 1 - i); }
        AZStd::vector<AZStd::array<size_t, 3>> triangles;
        while (remaining.size() > 3)
        {
            bool clipped = false;
            for (size_t i = 0; i < remaining.size(); ++i)
            {
                const auto a = remaining[(i + remaining.size() - 1) % remaining.size()];
                const auto b = remaining[i];
                const auto c = remaining[(i + 1) % remaining.size()];
                if (cross(flat[a], flat[b], flat[c]) <= epsilon) { continue; }
                bool contains = false;
                for (const auto p : remaining)
                {
                    if (p == a || p == b || p == c) { continue; }
                    if (cross(flat[a], flat[b], flat[p]) >= -epsilon &&
                        cross(flat[b], flat[c], flat[p]) >= -epsilon &&
                        cross(flat[c], flat[a], flat[p]) >= -epsilon) { contains = true; break; }
                }
                if (contains) { continue; }
                triangles.push_back({a, b, c});
                remaining.erase(remaining.begin() + i);
                clipped = true;
                break;
            }
            if (!clipped) { return false; }
        }
        if (cross(flat[remaining[0]], flat[remaining[1]], flat[remaining[2]]) <= epsilon) { return false; }
        triangles.push_back({remaining[0], remaining[1], remaining[2]});
        // Insert adjacent triangles together. Inserting disconnected ears that touch
        // only at a vertex can make OpenMesh reject an otherwise valid polygon.
        AZStd::vector<AZStd::array<size_t, 3>> ordered{triangles.front()};
        AZStd::vector<bool> inserted(triangles.size(), false);
        inserted[0] = true;
        while (ordered.size() < triangles.size())
        {
            bool found = false;
            for (size_t i = 0; i < triangles.size() && !found; ++i)
            {
                if (inserted[i]) { continue; }
                for (size_t j = 0; j < ordered.size(); ++j)
                {
                    int shared = 0;
                    for (const auto vertex : triangles[i])
                    {
                        for (const auto other : ordered[j]) { shared += vertex == other ? 1 : 0; }
                    }
                    if (shared == 2)
                    {
                        ordered.push_back(triangles[i]);
                        inserted[i] = true;
                        found = true;
                        break;
                    }
                }
            }
            if (!found) { return false; }
        }
        AZStd::vector<Api::VertexHandle> handles;
        for (const auto& point : points) { handles.push_back(Api::AddVertex(mesh, localFromWorld.TransformPoint(point))); }
        const bool extrude = extrusionHeight != 0.0f;
        const bool positive = extrusionHeight > 0.0f;
        Api::FaceVertHandlesList faces;
        for (const auto& triangle : ordered)
        {
            const auto a = handles[triangle[0]];
            const auto b = handles[triangle[1]];
            const auto c = handles[triangle[2]];
            faces.push_back(extrude && positive ? Api::FaceVertHandles{{a, c, b}} : Api::FaceVertHandles{{a, b, c}});
        }
        Api::AddPolygon(mesh, faces);
        if (extrude)
        {
            AZStd::vector<Api::VertexHandle> top;
            for (const auto& point : points)
            {
                top.push_back(Api::AddVertex(mesh, localFromWorld.TransformPoint(point + up * extrusionHeight)));
            }
            faces.clear();
            for (const auto& triangle : ordered)
            {
                const auto a = top[triangle[0]];
                const auto b = top[triangle[1]];
                const auto c = top[triangle[2]];
                faces.push_back(positive ? Api::FaceVertHandles{{a, b, c}} : Api::FaceVertHandles{{a, c, b}});
            }
            Api::AddPolygon(mesh, faces);
            for (size_t i = 0; i < count; ++i)
            {
                const size_t j = (i + 1) % count;
                // Input outlines may be clockwise or counterclockwise, independently of pull direction.
                if ((area > 0.0f) == positive)
                {
                    Api::AddQuadPolygon(mesh, handles[i], handles[j], top[j], top[i]);
                }
                else
                {
                    Api::AddQuadPolygon(mesh, handles[j], handles[i], top[i], top[j]);
                }
            }
        }
        return true;
    }
}
