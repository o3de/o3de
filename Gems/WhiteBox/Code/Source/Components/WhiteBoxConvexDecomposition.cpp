/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxConvexDecomposition.h"
#include "WhiteBoxColliderConfiguration.h"

#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/hash.h>
#include <AzCore/std/numeric.h>
#include <AzCore/std/parallel/conditional_variable.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/thread.h>
#include <cmath>
#include <numeric>

#include <AzCore/std/limits.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/sort.h>
#include <manifold/manifold.h>

#define ENABLE_VHACD_IMPLEMENTATION 1
#include <VHACD.h>

namespace WhiteBox
{
    namespace
    {
        // PhysX refuses hulls with more than this many vertices.
        constexpr size_t PhysXHullVertexLimit = 255;

        struct Shell
        {
            AZStd::vector<AZ::Vector3> m_points;
            AZStd::vector<AZ::u32> m_triangles; //!< Indices into m_points.
        };

        // Triangles sharing a corner belong to one shell; separate layers and islands come apart.
        AZStd::vector<Shell> SplitShells(const AZStd::vector<AZ::Vector3>& vertices, const AZStd::vector<AZ::u32>& indices)
        {
            AZStd::vector<AZ::u32> parent(vertices.size());
            std::iota(parent.begin(), parent.end(), 0u);
            const auto find = [&parent](AZ::u32 v)
            {
                while (parent[v] != v)
                {
                    parent[v] = parent[parent[v]];
                    v = parent[v];
                }
                return v;
            };
            for (size_t i = 0; i + 2 < indices.size(); i += 3)
            {
                const AZ::u32 a = find(indices[i]);
                parent[find(indices[i + 1])] = a;
                parent[find(indices[i + 2])] = a;
            }

            AZStd::unordered_map<AZ::u32, size_t> shellOf;
            AZStd::vector<AZStd::unordered_map<AZ::u32, AZ::u32>> localOf;
            AZStd::vector<Shell> shells;
            for (size_t i = 0; i + 2 < indices.size(); i += 3)
            {
                const AZ::u32 root = find(indices[i]);
                auto [it, added] = shellOf.emplace(root, shells.size());
                if (added)
                {
                    shells.emplace_back();
                    localOf.emplace_back();
                }
                Shell& shell = shells[it->second];
                auto& local = localOf[it->second];
                for (size_t c = 0; c < 3; ++c)
                {
                    const AZ::u32 global = indices[i + c];
                    auto [slot, fresh] = local.emplace(global, static_cast<AZ::u32>(shell.m_points.size()));
                    if (fresh)
                    {
                        shell.m_points.push_back(vertices[global]);
                    }
                    shell.m_triangles.push_back(slot->second);
                }
            }
            return shells;
        }

        float Extent(const AZStd::vector<AZ::Vector3>& points)
        {
            AZ::Aabb bounds = AZ::Aabb::CreateNull();
            for (const AZ::Vector3& point : points)
            {
                bounds.AddPoint(point);
            }
            return AZ::GetMax(bounds.GetExtents().GetLength(), 1e-4f);
        }

        // Every point lies on or behind every face plane (winding gives the outside); each distinct plane is tested once.
        bool IsConvex(const Shell& shell, const float tolerance)
        {
            AZStd::unordered_map<size_t, AZStd::pair<AZ::Vector3, float>> planes;
            for (size_t i = 0; i + 2 < shell.m_triangles.size(); i += 3)
            {
                const AZ::Vector3& a = shell.m_points[shell.m_triangles[i]];
                const AZ::Vector3 normal =
                    (shell.m_points[shell.m_triangles[i + 1]] - a).Cross(shell.m_points[shell.m_triangles[i + 2]] - a);
                if (normal.GetLengthSq() <= 1e-16f)
                {
                    continue;
                }
                const AZ::Vector3 unit = normal.GetNormalized();
                const float offset = unit.Dot(a);
                size_t key = 0;
                AZStd::hash_combine(key, static_cast<AZ::s64>(std::llround(unit.GetX() * 1e4)));
                AZStd::hash_combine(key, static_cast<AZ::s64>(std::llround(unit.GetY() * 1e4)));
                AZStd::hash_combine(key, static_cast<AZ::s64>(std::llround(unit.GetZ() * 1e4)));
                AZStd::hash_combine(key, static_cast<AZ::s64>(std::llround(offset / AZ::GetMax(tolerance, 1e-6f))));
                planes.emplace(key, AZStd::make_pair(unit, offset));
            }
            for (const auto& entry : planes)
            {
                for (const AZ::Vector3& point : shell.m_points)
                {
                    if (entry.second.first.Dot(point) - entry.second.second > tolerance)
                    {
                        return false;
                    }
                }
            }
            return true;
        }

        // The normal of a shell whose points all lie in one plane, or zero when it has volume.
        AZ::Vector3 FlatNormal(const Shell& shell, const float tolerance)
        {
            AZ::Vector3 sum = AZ::Vector3::CreateZero();
            for (size_t i = 0; i + 2 < shell.m_triangles.size(); i += 3)
            {
                const AZ::Vector3& a = shell.m_points[shell.m_triangles[i]];
                sum += (shell.m_points[shell.m_triangles[i + 1]] - a).Cross(shell.m_points[shell.m_triangles[i + 2]] - a);
            }
            if (sum.GetLengthSq() <= 1e-16f)
            {
                return AZ::Vector3::CreateZero();
            }
            const AZ::Vector3 normal = sum.GetNormalized();
            const AZ::Vector3& origin = shell.m_points.front();
            for (const AZ::Vector3& point : shell.m_points)
            {
                if (AZStd::abs(normal.Dot(point - origin)) > tolerance)
                {
                    return AZ::Vector3::CreateZero();
                }
            }
            return normal;
        }

        size_t HashShell(const Shell& shell, const WhiteBoxColliderConfiguration& configuration)
        {
            size_t seed = 0;
            for (const AZ::Vector3& point : shell.m_points)
            {
                // Quantised to a tenth of a millimetre so float noise from re-evaluation does not miss the cache.
                AZStd::hash_combine(seed, static_cast<AZ::s64>(std::llround(point.GetX() * 1e4)));
                AZStd::hash_combine(seed, static_cast<AZ::s64>(std::llround(point.GetY() * 1e4)));
                AZStd::hash_combine(seed, static_cast<AZ::s64>(std::llround(point.GetZ() * 1e4)));
            }
            for (const AZ::u32 index : shell.m_triangles)
            {
                AZStd::hash_combine(seed, index);
            }
            AZStd::hash_combine(seed, configuration.m_maxHullsPerShell);
            AZStd::hash_combine(seed, configuration.m_decompositionResolution);
            AZStd::hash_combine(seed, configuration.m_maxVerticesPerHull);
            return seed;
        }

        // ---- Exact split: cut closed shells along their own face planes until every piece is convex ----

        manifold::MeshGL64 ShellMesh(const Shell& shell)
        {
            manifold::MeshGL64 mesh;
            mesh.numProp = 3;
            mesh.vertProperties.reserve(shell.m_points.size() * 3);
            for (const AZ::Vector3& point : shell.m_points)
            {
                mesh.vertProperties.insert(mesh.vertProperties.end(), { point.GetX(), point.GetY(), point.GetZ() });
            }
            mesh.triVerts.assign(shell.m_triangles.begin(), shell.m_triangles.end());
            mesh.Merge();
            return mesh;
        }

        struct Plane
        {
            manifold::vec3 m_normal;
            double m_offset = 0.0;
            double m_area = 0.0;
        };

        manifold::vec3 Corner(const manifold::MeshGL64& mesh, const size_t vertex)
        {
            const size_t stride = static_cast<size_t>(mesh.numProp);
            return manifold::vec3(
                mesh.vertProperties[vertex * stride], mesh.vertProperties[vertex * stride + 1], mesh.vertProperties[vertex * stride + 2]);
        }

        // Beyond this many distinct face planes a shell is curved rather than built from flat walls; V-HACD suits it better.
        constexpr size_t MaxExactPlanes = 256;
        // Shells bigger than this skip the exact cuts: each cut re-splits the whole piece.
        constexpr size_t MaxExactTriangles = 3000;

        // Face planes with some of the piece in front of them: the dents a cut has to remove. Largest first, one per plane.
        //! False when the piece has too many distinct planes to cut exactly.
        bool DentPlanes(const manifold::MeshGL64& mesh, const double tolerance, const size_t limit, AZStd::vector<Plane>& dents)
        {
            dents.clear();
            const size_t vertexCount = mesh.vertProperties.size() / static_cast<size_t>(mesh.numProp);
            AZStd::unordered_map<size_t, Plane> planes;
            for (size_t i = 0; i + 2 < mesh.triVerts.size(); i += 3)
            {
                const manifold::vec3 a = Corner(mesh, static_cast<size_t>(mesh.triVerts[i]));
                const manifold::vec3 cross = manifold::la::cross(
                    Corner(mesh, static_cast<size_t>(mesh.triVerts[i + 1])) - a, Corner(mesh, static_cast<size_t>(mesh.triVerts[i + 2])) - a);
                const double length = manifold::la::length(cross);
                if (length <= 1e-12)
                {
                    continue;
                }
                const manifold::vec3 normal = cross / length;
                const double offset = manifold::la::dot(normal, a);
                size_t key = 0;
                AZStd::hash_combine(key, static_cast<AZ::s64>(std::llround(normal.x * 1e4)));
                AZStd::hash_combine(key, static_cast<AZ::s64>(std::llround(normal.y * 1e4)));
                AZStd::hash_combine(key, static_cast<AZ::s64>(std::llround(normal.z * 1e4)));
                AZStd::hash_combine(key, static_cast<AZ::s64>(std::llround(offset * 1e4)));
                Plane& plane = planes[key];
                plane.m_normal = normal;
                plane.m_offset = offset;
                plane.m_area += length * 0.5;
                if (planes.size() > MaxExactPlanes)
                {
                    return false;
                }
            }
            // Each distinct plane is tested against the vertices once, stopping at the first vertex in front of it.
            for (const auto& entry : planes)
            {
                const Plane& plane = entry.second;
                for (size_t v = 0; v < vertexCount; ++v)
                {
                    if (manifold::la::dot(plane.m_normal, Corner(mesh, v)) - plane.m_offset > tolerance)
                    {
                        dents.push_back(plane);
                        break;
                    }
                }
            }
            AZStd::sort(dents.begin(), dents.end(), [](const Plane& lhs, const Plane& rhs) { return lhs.m_area > rhs.m_area; });
            if (dents.size() > limit)
            {
                dents.resize(limit);
            }
            return true;
        }

        // Volume the convex hull adds on top of the piece; zero for a convex piece.
        double Concavity(const manifold::Manifold& piece)
        {
            return piece.IsEmpty() ? 0.0 : AZStd::max(0.0, piece.Hull().Volume() - piece.Volume());
        }

        HullPoints PiecePoints(const manifold::MeshGL64& mesh)
        {
            HullPoints points;
            const size_t vertexCount = mesh.vertProperties.size() / static_cast<size_t>(mesh.numProp);
            points.reserve(vertexCount);
            for (size_t v = 0; v < vertexCount; ++v)
            {
                const manifold::vec3 corner = Corner(mesh, v);
                points.emplace_back(static_cast<float>(corner.x), static_cast<float>(corner.y), static_cast<float>(corner.z));
            }
            return points;
        }

        //! False when the shell is not a closed solid, or would need more than maxHulls pieces; V-HACD takes over then.
        bool ExactDecompose(
            const Shell& shell, const AZ::u32 maxHulls, const float extent, const AZStd::atomic_bool& cancel,
            AZStd::vector<HullPoints>& out)
        {
            if (shell.m_triangles.size() / 3 > MaxExactTriangles)
            {
                return false;
            }
            const manifold::Manifold solid(ShellMesh(shell));
            if (solid.Status() != manifold::Manifold::Error::NoError || solid.IsEmpty())
            {
                return false;
            }
            const double tolerance = static_cast<double>(extent) * 1e-5;
            const double minimumVolume = std::pow(static_cast<double>(extent) * 1e-4, 3.0);
            AZStd::vector<manifold::Manifold> todo{ solid };
            AZStd::vector<manifold::MeshGL64> pieces;
            size_t cuts = 0;
            AZStd::vector<Plane> planes;
            while (!todo.empty())
            {
                if (cancel)
                {
                    return false;
                }
                const manifold::Manifold piece = AZStd::move(todo.back());
                todo.pop_back();
                if (piece.IsEmpty() || piece.Volume() <= minimumVolume)
                {
                    continue; // a sliver left by a cut
                }
                manifold::MeshGL64 mesh = piece.GetMeshGL64();
                if (!DentPlanes(mesh, tolerance, 8, planes))
                {
                    return false;
                }
                if (planes.empty())
                {
                    pieces.push_back(AZStd::move(mesh));
                    if (pieces.size() > maxHulls)
                    {
                        return false;
                    }
                    continue;
                }
                if (++cuts > static_cast<size_t>(maxHulls) * 4)
                {
                    return false;
                }
                // The cut leaving the least concavity wins; a cut leaving none ends the search.
                double bestScore = AZStd::numeric_limits<double>::max();
                std::pair<manifold::Manifold, manifold::Manifold> best;
                for (const Plane& plane : planes)
                {
                    auto split = piece.SplitByPlane(plane.m_normal, plane.m_offset);
                    if (split.first.IsEmpty() || split.second.IsEmpty())
                    {
                        continue;
                    }
                    const double score = Concavity(split.first) + Concavity(split.second);
                    if (score < bestScore)
                    {
                        bestScore = score;
                        best = AZStd::move(split);
                        if (score <= minimumVolume)
                        {
                            break;
                        }
                    }
                }
                if (bestScore == AZStd::numeric_limits<double>::max())
                {
                    return false; // no plane separates it (numerically), so leave it to V-HACD
                }
                for (const manifold::Manifold* half : { &best.first, &best.second })
                {
                    for (manifold::Manifold& component : half->Decompose())
                    {
                        todo.push_back(AZStd::move(component));
                    }
                }
            }
            if (pieces.empty())
            {
                return false;
            }
            for (const manifold::MeshGL64& mesh : pieces)
            {
                out.push_back(PiecePoints(mesh));
            }
            return true;
        }

        struct DecompositionSettings
        {
            bool m_tryExact = true; //!< Off for convex shells that only need V-HACD to cap their vertex count.
            float m_extent = 1.0f;  //!< Shell size, which scales the exact cuts' tolerances.
            AZ::u32 m_maxHulls = 1;
            AZ::u32 m_resolution = 100000;
            AZ::u32 m_maxVertices = 64;
        };

        // Runs one shell through V-HACD on the calling thread, reusing the given instance.
        AZStd::vector<HullPoints> RunVhacd(VHACD::IVHACD& decomposer, const Shell& shell, const DecompositionSettings& settings)
        {
            AZStd::vector<float> flat;
            flat.reserve(shell.m_points.size() * 3);
            for (const AZ::Vector3& point : shell.m_points)
            {
                flat.insert(flat.end(), { point.GetX(), point.GetY(), point.GetZ() });
            }

            VHACD::IVHACD::Parameters parameters;
            parameters.m_maxConvexHulls = settings.m_maxHulls;
            parameters.m_resolution = AZ::GetClamp(settings.m_resolution, 10000u, 10000000u);
            parameters.m_maxNumVerticesPerCH = AZ::GetClamp(settings.m_maxVertices, 8u, static_cast<AZ::u32>(PhysXHullVertexLimit));
            parameters.m_asyncACD = false; // the worker thread is already off the editor's main thread
            parameters.m_shrinkWrap = true;

            AZStd::vector<HullPoints> hulls;
            if (decomposer.Compute(
                    flat.data(), static_cast<uint32_t>(shell.m_points.size()), shell.m_triangles.data(),
                    static_cast<uint32_t>(shell.m_triangles.size() / 3), parameters))
            {
                for (uint32_t i = 0; i < decomposer.GetNConvexHulls(); ++i)
                {
                    VHACD::IVHACD::ConvexHull convexHull;
                    if (!decomposer.GetConvexHull(i, convexHull) || convexHull.m_points.size() < 4)
                    {
                        continue;
                    }
                    HullPoints hull;
                    hull.reserve(convexHull.m_points.size());
                    for (const VHACD::Vertex& vertex : convexHull.m_points)
                    {
                        hull.emplace_back(static_cast<float>(vertex[0]), static_cast<float>(vertex[1]), static_cast<float>(vertex[2]));
                    }
                    hulls.push_back(AZStd::move(hull));
                }
            }
            decomposer.Clean();
            return hulls;
        }

        // Exact cuts along the shell's own faces when they work, otherwise V-HACD's approximation.
        AZStd::vector<HullPoints> DecomposeShell(
            VHACD::IVHACD& decomposer, const Shell& shell, const DecompositionSettings& settings, const AZStd::atomic_bool& cancel)
        {
            AZStd::vector<HullPoints> hulls;
            if (settings.m_tryExact && ExactDecompose(shell, settings.m_maxHulls, settings.m_extent, cancel, hulls))
            {
                return hulls;
            }
            hulls.clear();
            if (cancel)
            {
                return hulls;
            }
            return RunVhacd(decomposer, shell, settings);
        }

        // One worker thread and one result cache for every collider in the session, so recreated entities (undo) reuse results.
        class Worker
        {
        public:
            static Worker& Get()
            {
                static Worker worker;
                return worker;
            }

            ~Worker()
            {
                // Shutdown joins the thread before the module unloads; this only avoids std::terminate if it was skipped.
                if (m_thread.joinable())
                {
                    m_thread.detach();
                }
            }

            bool Find(const size_t key, AZStd::vector<HullPoints>& hulls)
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                const auto found = m_done.find(key);
                if (found == m_done.end())
                {
                    return false;
                }
                hulls = found->second;
                return true;
            }

            void Store(const size_t key, AZStd::vector<HullPoints> hulls)
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                StoreLocked(key, AZStd::move(hulls));
            }

            // Queue the shell unless it already is; every requester is told once when it lands.
            void Enqueue(
                const size_t key, const Shell& shell, const DecompositionSettings& settings, const AZ::EntityId requester,
                const AZStd::function<void()>& onReady)
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                if (m_stopping)
                {
                    return;
                }
                auto pending = m_pending.find(key);
                if (pending == m_pending.end())
                {
                    pending = m_pending.emplace(key, Job{ shell, settings, {} }).first;
                    m_queue.push_back(key);
                }
                auto& waiters = pending->second.m_waiters;
                const bool known = AZStd::any_of(
                    waiters.begin(), waiters.end(), [requester](const auto& waiter) { return waiter.first == requester; });
                if (!known)
                {
                    waiters.emplace_back(requester, onReady);
                }
                if (!m_thread.joinable())
                {
                    AZStd::thread_desc description;
                    description.m_name = "WhiteBox V-HACD";
                    m_thread = AZStd::thread(description, [this]() { Run(); });
                }
                m_wake.notify_one();
            }

            void Shutdown()
            {
                {
                    AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                    m_stopping = true;
                    m_cancel = true;
                    m_queue.clear();
                    if (m_running != nullptr)
                    {
                        m_running->Cancel(); // V-HACD returns early, so the join below is quick
                    }
                }
                m_wake.notify_all();
                if (m_thread.joinable())
                {
                    m_thread.join();
                }
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                m_pending.clear();
                m_stopping = false; // a later activation may start the worker again
                m_cancel = false;
            }

        private:
            struct Job
            {
                Shell m_shell;
                DecompositionSettings m_settings;
                AZStd::vector<AZStd::pair<AZ::EntityId, AZStd::function<void()>>> m_waiters;
            };

            void StoreLocked(const size_t key, AZStd::vector<HullPoints> hulls)
            {
                if (m_done.size() > 512)
                {
                    m_done.clear(); // bounded; a dropped entry only costs a recompute
                }
                m_done[key] = AZStd::move(hulls);
            }

            void Run()
            {
                VHACD::IVHACD* decomposer = VHACD::CreateVHACD();
                while (decomposer != nullptr)
                {
                    size_t key = 0;
                    Shell shell;
                    DecompositionSettings settings;
                    {
                        AZStd::unique_lock<AZStd::mutex> lock(m_mutex);
                        m_wake.wait(lock, [this]() { return m_stopping || !m_queue.empty(); });
                        if (m_stopping)
                        {
                            break;
                        }
                        key = m_queue.front();
                        m_queue.pop_front();
                        const Job& job = m_pending[key];
                        shell = job.m_shell;
                        settings = job.m_settings;
                        m_running = decomposer;
                    }

                    AZStd::vector<HullPoints> hulls = DecomposeShell(*decomposer, shell, settings, m_cancel);

                    AZStd::vector<AZStd::pair<AZ::EntityId, AZStd::function<void()>>> waiters;
                    {
                        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                        m_running = nullptr;
                        if (m_stopping)
                        {
                            break; // cancelled mid-run; the partial result is not kept
                        }
                        StoreLocked(key, AZStd::move(hulls));
                        waiters = AZStd::move(m_pending[key].m_waiters);
                        m_pending.erase(key);
                    }
                    // Colliders rebuild on the main thread; each looks itself up again, so a deleted entity is skipped.
                    AZ::TickBus::QueueFunction(
                        [waiters = AZStd::move(waiters)]()
                        {
                            for (const auto& waiter : waiters)
                            {
                                if (waiter.second)
                                {
                                    waiter.second();
                                }
                            }
                        });
                }
                if (decomposer != nullptr)
                {
                    decomposer->Release();
                }
            }

            AZStd::mutex m_mutex;
            AZStd::condition_variable m_wake;
            AZStd::deque<size_t> m_queue;
            AZStd::unordered_map<size_t, Job> m_pending;
            AZStd::unordered_map<size_t, AZStd::vector<HullPoints>> m_done;
            AZStd::thread m_thread;
            VHACD::IVHACD* m_running = nullptr;
            bool m_stopping = false;
            AZStd::atomic_bool m_cancel{ false }; //!< Read by the exact cuts between steps, so Shutdown need not wait them out.

        public:
            static const AZStd::atomic_bool& NotCancelled()
            {
                static const AZStd::atomic_bool never{ false };
                return never;
            }
        };
    } // namespace

    AZStd::vector<HullPoints> ConvexDecomposer::Decompose(
        const AZStd::vector<AZ::Vector3>& vertices, const AZStd::vector<AZ::u32>& indices,
        const WhiteBoxColliderConfiguration& configuration, const DecomposeWait wait, const AZ::EntityId requester,
        const AZStd::function<void()>& onReady, bool* pending)
    {
        if (pending != nullptr)
        {
            *pending = false;
        }
        AZStd::vector<HullPoints> hulls;
        for (Shell& shell : SplitShells(vertices, indices))
        {
            const float extent = Extent(shell.m_points);
            const float tolerance = extent * 1e-4f;
            if (const AZ::Vector3 normal = FlatNormal(shell, tolerance); !normal.IsZero())
            {
                // A plane has no volume to wrap, so it gets a thin backing below its surface; the top stays exact.
                const float thickness = AZ::GetClamp(extent * 0.01f, 0.01f, 0.1f);
                HullPoints hull = shell.m_points;
                for (const AZ::Vector3& point : shell.m_points)
                {
                    hull.push_back(point - normal * thickness);
                }
                hulls.push_back(AZStd::move(hull));
                continue;
            }
            const bool convex = IsConvex(shell, tolerance);
            if (convex && shell.m_points.size() <= PhysXHullVertexLimit)
            {
                hulls.push_back(AZStd::move(shell.m_points));
                continue;
            }
            // Concave, or convex with too many points for one PhysX hull (V-HACD then simplifies it to one).
            DecompositionSettings settings;
            settings.m_tryExact = !convex;
            settings.m_extent = extent;
            settings.m_maxHulls = convex ? 1u : AZ::GetMax(configuration.m_maxHullsPerShell, 1u);
            settings.m_resolution = configuration.m_decompositionResolution;
            settings.m_maxVertices = configuration.m_maxVerticesPerHull;
            const size_t key = HashShell(shell, configuration);

            AZStd::vector<HullPoints> parts;
            if (!Worker::Get().Find(key, parts))
            {
                // Only a game build waits here; the editor never cuts or voxelises on its own thread.
                if (wait == DecomposeWait::Block)
                {
                    if (VHACD::IVHACD* decomposer = VHACD::CreateVHACD())
                    {
                        parts = DecomposeShell(*decomposer, shell, settings, Worker::NotCancelled());
                        decomposer->Release();
                        Worker::Get().Store(key, parts);
                    }
                }
                else
                {
                    Worker::Get().Enqueue(key, shell, settings, requester, onReady);
                    if (pending != nullptr)
                    {
                        *pending = true;
                    }
                }
            }
            if (parts.empty())
            {
                // Still queued, or V-HACD gave up: one hull around the shell collides, just less tightly.
                hulls.push_back(AZStd::move(shell.m_points));
                continue;
            }
            hulls.insert(hulls.end(), parts.begin(), parts.end());
        }
        return hulls;
    }

    void ConvexDecomposer::Shutdown()
    {
        Worker::Get().Shutdown();
    }
} // namespace WhiteBox
