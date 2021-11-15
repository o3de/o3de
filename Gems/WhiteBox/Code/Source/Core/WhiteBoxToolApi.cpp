/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Util/WhiteBoxMathUtil.h"
#include "Util/WhiteBoxTextureUtil.h"

#include <AzCore/Console/Console.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/ToString.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/hash.h>
#include <AzCore/std/numeric.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/string/conversions.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace OpenMesh
{
    // Overload methods need to be declared before including OpenMesh so their definitions are found

    inline AZ::Vector3 normalize(const AZ::Vector3& v)
    {
        AZ::Vector3 vret = v;
        vret.Normalize();
        return vret;
    }

    inline float dot(const AZ::Vector3& v1, const AZ::Vector3& v2)
    {
        return v1.Dot(v2);
    }

    inline float norm(const AZ::Vector3& v)
    {
        return v.GetLength();
    }

    inline AZ::Vector3 cross(const AZ::Vector3& v1, const AZ::Vector3& v2)
    {
        return v1.Cross(v2);
    }

    inline AZ::Vector3 vectorize(AZ::Vector3& v, float s)
    {
        v = AZ::Vector3(s);
        return v;
    }

    inline void newell_norm(AZ::Vector3& n, const AZ::Vector3& a, const AZ::Vector3& b)
    {
        n.SetX(n.GetX() + (a.GetY() * b.GetZ()));
        n.SetY(n.GetY() + (a.GetZ() * b.GetX()));
        n.SetZ(n.GetZ() + (a.GetX() * b.GetY()));
    }
}

// OpenMesh includes
AZ_PUSH_DISABLE_WARNING(4702, "-Wunknown-warning-option") // OpenMesh\Core\Utils\Property.hh has unreachable code
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/IO/SR_binary.hh>
#include <OpenMesh/Core/IO/importer/ImporterT.hh>
#include <OpenMesh/Core/Mesh/Traits.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <OpenMesh/Core/Utils/GenProg.hh>
#include <OpenMesh/Core/Utils/vector_traits.hh>
AZ_POP_DISABLE_WARNING

AZ_DECLARE_BUDGET(AzToolsFramework);

namespace OpenMesh
{
    template<>
    struct vector_traits<AZ::Vector3>
    {
        //! Type of the vector class
        using vector_type = AZ::Vector3;

        //! Type of the scalar value
        using value_type = float;

        //! size/dimension of the vector
        static const size_t size_ = 3;

        //! size/dimension of the vector
        static size_t size()
        {
            return size_;
        }
    };

    template<>
    struct vector_traits<AZ::Vector2>
    {
        //! Type of the vector class
        using vector_type = AZ::Vector2;

        //! Type of the scalar value
        using value_type = float;

        //! size/dimension of the vector
        static const size_t size_ = 2;

        //! size/dimension of the vector
        static size_t size()
        {
            return size_;
        }
    };

    template<>
    inline void vector_cast(const AZ::Vector3& src, OpenMesh::Vec3f& dst, GenProg::Int2Type<3> /*unused*/)
    {
        dst[0] = static_cast<vector_traits<Vec3f>::value_type>(src.GetX());
        dst[1] = static_cast<vector_traits<Vec3f>::value_type>(src.GetY());
        dst[2] = static_cast<vector_traits<Vec3f>::value_type>(src.GetZ());
    }

    template<>
    inline void vector_cast(const AZ::Vector2& src, OpenMesh::Vec2f& dst, GenProg::Int2Type<2> /*unused*/)
    {
        dst[0] = static_cast<vector_traits<Vec2f>::value_type>(src.GetX());
        dst[1] = static_cast<vector_traits<Vec2f>::value_type>(src.GetY());
    }

    template<>
    inline void vector_cast(const OpenMesh::Vec3f& src, AZ::Vector3& dst, GenProg::Int2Type<3> /*unused*/)
    {
        dst.SetX(static_cast<vector_traits<Vec3f>::value_type>(src[0]));
        dst.SetY(static_cast<vector_traits<Vec3f>::value_type>(src[1]));
        dst.SetZ(static_cast<vector_traits<Vec3f>::value_type>(src[2]));
    }

    template<>
    inline void vector_cast(const OpenMesh::Vec2f& src, AZ::Vector2& dst, GenProg::Int2Type<2> /*unused*/)
    {
        dst.SetX(static_cast<vector_traits<Vec2f>::value_type>(src[0]));
        dst.SetY(static_cast<vector_traits<Vec2f>::value_type>(src[1]));
    }

    template<>
    inline void vector_cast(const AZ::Vector3& src, OpenMesh::Vec3d& dst, GenProg::Int2Type<3> /*unused*/)
    {
        dst[0] = static_cast<vector_traits<Vec3d>::value_type>(src.GetX());
        dst[1] = static_cast<vector_traits<Vec3d>::value_type>(src.GetY());
        dst[2] = static_cast<vector_traits<Vec3d>::value_type>(src.GetZ());
    }

    template<>
    inline void vector_cast(const AZ::Vector2& src, OpenMesh::Vec2d& dst, GenProg::Int2Type<2> /*unused*/)
    {
        dst[0] = static_cast<vector_traits<Vec2d>::value_type>(src.GetX());
        dst[1] = static_cast<vector_traits<Vec2d>::value_type>(src.GetY());
    }

    template<>
    inline void vector_cast(const OpenMesh::Vec3d& src, AZ::Vector3& dst, GenProg::Int2Type<3> /*unused*/)
    {
        dst.SetX(static_cast<float>(src[0]));
        dst.SetY(static_cast<float>(src[1]));
        dst.SetZ(static_cast<float>(src[2]));
    }

    template<>
    inline void vector_cast(const OpenMesh::Vec2d& src, AZ::Vector2& dst, GenProg::Int2Type<2> /*unused*/)
    {
        dst.SetX(static_cast<float>(src[0]));
        dst.SetY(static_cast<float>(src[1]));
    }
} // namespace OpenMesh

#ifdef AZ_ENABLE_TRACING
#define WHITEBOX_LOG(str, ...)                                                                                         \
    do                                                                                                                 \
    {                                                                                                                  \
        if (cl_whiteBoxLogMessages)                                                                                    \
        {                                                                                                              \
            AZ_Printf(str, __VA_ARGS__)                                                                                \
        }                                                                                                              \
    } while (0);
#else
#define WHITEBOX_LOG(str, ...)
#endif

namespace WhiteBox
{
    // cvar for logging debug messages
    AZ_CVAR(bool, cl_whiteBoxLogMessages, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Log debug messages.");

    struct WhiteBoxTraits : public OpenMesh::DefaultTraits
    {
        using Point = AZ::Vector3;
        using Normal = AZ::Vector3;
        using TexCoord2D = AZ::Vector2;
        using TexCoord3D = AZ::Vector3;
    };

    using Mesh = OpenMesh::TriMesh_ArrayKernelT<WhiteBoxTraits>;

} // namespace WhiteBox

namespace AZStd
{
    template<>
    struct hash<WhiteBox::Mesh::FaceHandle>
    {
    public:
        size_t operator()(const WhiteBox::Mesh::FaceHandle& faceHandle) const
        {
            size_t h{0};
            AZStd::hash_combine(h, faceHandle.idx());
            return h;
        }
    };

} // namespace AZStd

namespace WhiteBox
{
    // alias for vector of OpenMesh FaceHandles
    using FaceHandlesInternal = AZStd::vector<Mesh::FaceHandle>;

    // a property to map from a FaceHandle to the Polygon it corresponds to
    // note: PolygonHandle will include the FaceHandle used to do the lookup
    using FaceHandlePolygonMapping = AZStd::unordered_map<Mesh::FaceHandle, FaceHandlesInternal>;
    using PolygonPropertyHandle = OpenMesh::MPropHandleT<FaceHandlePolygonMapping>;
    // unique string to lookup the polygon custom property via get_property_handle
    static const char* const PolygonProps = "polygon-props";
    // a property to track the hidden state of a vertex
    using VertexBoolPropertyHandle = OpenMesh::VPropHandleT<bool>;
    // unique string to lookup the vertex custom property via get_property_handle
    static const char* const VertexHiddenProp = "vertex-hidden-props";
} // namespace WhiteBox

namespace OpenMesh::IO
{
    template<>
    struct binary<WhiteBox::FaceHandlesInternal>
    {
        using value_type = WhiteBox::FaceHandlesInternal;
        static const bool is_streamable = true;

        // return generic binary size of self, if known
        static size_t size_of()
        {
            return UnknownSize;
        }

        // return binary size of the value
        static size_t size_of(const value_type& _v)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            if (_v.empty())
            {
                return sizeof(uint32_t);
            }

            value_type::const_iterator it = _v.begin();
            const auto count = static_cast<uint32_t>(_v.size());
            size_t bytes = IO::size_of(count);

            for (; it != _v.end(); ++it)
            {
                bytes += IO::size_of(it->idx());
            }

            return bytes;
        }

        static size_t store(std::ostream& _os, const value_type& _v, bool _swap = false)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            size_t bytes = 0;
            const auto count = static_cast<uint32_t>(_v.size());
            value_type::const_iterator it = _v.begin();
            bytes += IO::store(_os, count, _swap);

            for (; it != _v.end() && _os.good(); ++it)
            {
                bytes += IO::store(_os, (*it).idx(), _swap);
            }

            return _os.good() ? bytes : 0;
        }

        static size_t restore(std::istream& _is, value_type& _v, bool _swap = false)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            size_t bytes = 0;
            uint32_t count = 0;
            _v.clear();
            bytes += IO::restore(_is, count, _swap);
            _v.reserve(count);

            for (size_t i = 0; i < count && _is.good(); ++i)
            {
                int elem; // value_type::value_type -> Mesh::FaceHandle (underlying type int)
                bytes += IO::restore(_is, elem, _swap);
                _v.push_back(value_type::value_type(elem));
            }

            return _is.good() ? bytes : 0;
        }
    };

    template<>
    struct binary<WhiteBox::FaceHandlePolygonMapping>
    {
        using value_type = WhiteBox::FaceHandlePolygonMapping;
        static const bool is_streamable = true;

        // return generic binary size of self, if known
        static size_t size_of()
        {
            return UnknownSize;
        }

        // return binary size of the value
        static size_t size_of(const value_type& _v)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            if (_v.empty())
            {
                return sizeof(uint32_t);
            }

            value_type::const_iterator it = _v.begin();
            const auto count = static_cast<uint32_t>(_v.size());
            size_t bytes = IO::size_of(count);

            for (; it != _v.end(); ++it)
            {
                bytes += IO::size_of(it->first.idx());
                bytes += IO::size_of(it->second);
            }

            return bytes;
        }

        static size_t store(std::ostream& _os, const value_type& _v, bool _swap = false)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            size_t bytes = 0;
            const auto count = static_cast<uint32_t>(_v.size());
            value_type::const_iterator it = _v.begin();
            bytes += IO::store(_os, count, _swap);

            for (; it != _v.end() && _os.good(); ++it)
            {
                bytes += IO::store(_os, it->first.idx(), _swap);
                bytes += IO::store(_os, it->second, _swap);
            }

            return _os.good() ? bytes : 0;
        }

        static size_t restore(std::istream& _is, value_type& _v, bool _swap = false)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            size_t bytes = 0;
            uint32_t count = 0;
            _v.clear();
            bytes += IO::restore(_is, count, _swap);
            value_type::mapped_type val;

            for (size_t i = 0; i < count && _is.good(); ++i)
            {
                int key; // value_type::key_type -> Mesh::FaceHandle (underlying type int)
                bytes += IO::restore(_is, key, _swap);
                bytes += IO::restore(_is, val, _swap);
                _v[value_type::key_type(key)] = val;
            }

            return _is.good() ? bytes : 0;
        }
    };

} // namespace OpenMesh::IO

namespace WhiteBox
{
    static const float NormalTolerance = 0.99f;
    static const float AdjacentPolygonNormalTolerance = 0.0001f;

    // A wrapper for the OpenMesh source data.
    struct WhiteBoxMesh
    {
        AZ_CLASS_ALLOCATOR(WhiteBoxMesh, AZ::SystemAllocator, 0);

        WhiteBoxMesh() = default;
        WhiteBoxMesh(WhiteBoxMesh&&) = default;
        WhiteBoxMesh& operator=(WhiteBoxMesh&&) = default;

        Mesh mesh; //!< The OpenMesh triangle mesh kernel (with customized AZ traits).
    };

    // 0,0 is tl - 1,1 is br
    // 3 is 0,0
    // 2 is 1,0
    // 1 is 1,1
    // 0 is 0,1
    const Mesh::TexCoord2D g_quadUVs[] = {
        Mesh::TexCoord2D(0.0f, 1.0f),
        Mesh::TexCoord2D(1.0f, 1.0f),
        Mesh::TexCoord2D(1.0f, 0.0f),
        Mesh::TexCoord2D(0.0f, 0.0f),
    };

    // conversion functions between OpenMesh and AZ types

    // convert WhiteBox face handle to OpenMesh face handle
    static Mesh::FaceHandle om_fh(const Api::FaceHandle fh)
    {
        return Mesh::FaceHandle{fh.Index()};
    }

    // convert WhiteBox vertex handle to OpenMesh vertex handle
    static Mesh::VertexHandle om_vh(const Api::VertexHandle vh)
    {
        return Mesh::VertexHandle{vh.Index()};
    }

    // convert WhiteBox edge handle to OpenMesh edge handle
    static Mesh::EdgeHandle om_eh(const Api::EdgeHandle eh)
    {
        return Mesh::EdgeHandle{eh.Index()};
    }

    // convert WhiteBox halfedge handle to OpenMesh halfedge handle
    static Mesh::HalfedgeHandle om_heh(const Api::HalfedgeHandle heh)
    {
        return Mesh::HalfedgeHandle{heh.Index()};
    }

    // convert OpenMesh face handle to WhiteBox face handle
    static Api::FaceHandle wb_fh(const Mesh::FaceHandle fh)
    {
        return Api::FaceHandle{fh.idx()};
    }

    // convert OpenMesh vertex handle to WhiteBox vertex handle
    static Api::VertexHandle wb_vh(const Mesh::VertexHandle vh)
    {
        return Api::VertexHandle{vh.idx()};
    }

    // convert OpenMesh halfedge handle to WhiteBox halfedge handle
    static Api::HalfedgeHandle wb_heh(const Mesh::HalfedgeHandle heh)
    {
        return Api::HalfedgeHandle{heh.idx()};
    }

    // convert OpenMesh edge handle to WhiteBox edge handle
    static Api::EdgeHandle wb_eh(const Mesh::EdgeHandle eh)
    {
        return Api::EdgeHandle{eh.idx()};
    }

    // map from internal handles to external handles
    Api::PolygonHandle PolygonHandleFromInternal(const FaceHandlesInternal& faceHandlesInternal)
    {
        Api::PolygonHandle polygonHandle;
        polygonHandle.m_faceHandles.reserve(faceHandlesInternal.size());
        AZStd::transform(
            faceHandlesInternal.begin(), faceHandlesInternal.end(), AZStd::back_inserter(polygonHandle.m_faceHandles),
            &wb_fh);
        return polygonHandle;
    }

    FaceHandlesInternal InternalFaceHandlesFromPolygon(const Api::PolygonHandle& polygonHandle)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        FaceHandlesInternal faceHandlesInternal;
        faceHandlesInternal.reserve(polygonHandle.m_faceHandles.size());
        AZStd::transform(
            polygonHandle.m_faceHandles.begin(), polygonHandle.m_faceHandles.end(),
            AZStd::back_inserter(faceHandlesInternal), &om_fh);
        return faceHandlesInternal;
    }

    namespace Api
    {
        AZStd::mutex g_omSerializationLock; // serialization lock required when using Open Mesh IOManager

        namespace Internal
        {
            // when performing an append (extrusion or impression) new vertices will
            // be added to the mesh - this struct maps from the existing vertex and the
            // newly added one at the same location.
            // note: it is possible that as part of an impression, m_existing and m_added
            // both refer to the same vertex handle as the same vertex will be reused
            struct VertexHandlePair
            {
                VertexHandle m_existing;
                VertexHandle m_added;

                VertexHandlePair() = default;
                VertexHandlePair(const VertexHandle existing, const VertexHandle added)
                    : m_existing(existing)
                    , m_added(added)
                {
                }
            };

            // a collection of VertexHandlePairs
            // generated as part of an append (extrusion or impression)
            struct AppendedVerts
            {
                AZStd::vector<VertexHandlePair> m_vertexHandlePairs;
            };

            // intermediate data to use when appending an edge (performing an 'edge extrusion')
            struct EdgeAppendVertexHandles
            {
                PolygonHandle m_existingPolygonHandle; // the polygon to be replaced by the new edge extrusion

                // the vertices to use when 'appending' new geometry to the mesh while performing an edge extrusion.
                VertexHandle m_toVertexHandle;
                VertexHandle m_fromVertexHandle;
                VertexHandle m_addedFromVertexHandle;
                VertexHandle m_addedToVertexHandle;
                VertexHandle m_afterToVertexHandle;
                VertexHandle m_beforeFromVertexHandle;
            };

            // intermediate data to use when appending an edge (performing an 'edge extrusion')
            struct EdgeAppendPolygonHandles
            {
                PolygonHandle m_nearPolygonHandle;
                PolygonHandle m_farPolygonHandle;
                PolygonHandle m_topPolygonHandle;
                PolygonHandle m_bottomPolygonHandle;
            };
        } // namespace Internal

        // forward declarations
        AZStd::vector<FaceVertHandles> BuildNewVertexFaceHandles(
            WhiteBoxMesh& whiteBox, const Internal::AppendedVerts& appendedVerts, const FaceHandles& existingFaces);
        void RemoveFaces(WhiteBoxMesh& whiteBox, const FaceHandles& faceHandles);
        void CalculatePlanarUVs(WhiteBoxMesh& whiteBox, const FaceHandles& faceHandles);

        // restores WhiteBoxMesh properties, use when properties have not been initialized or have been cleared
        static void InitializeWhiteBoxMesh(WhiteBoxMesh& whiteBox)
        {
            // add default properties for all white box meshes
            PolygonPropertyHandle polygonPropsHandle;
            whiteBox.mesh.add_property(polygonPropsHandle, PolygonProps);
            whiteBox.mesh.mproperty(polygonPropsHandle).set_persistent(true);

            VertexBoolPropertyHandle vertexPropsHiddenHandle;
            whiteBox.mesh.add_property(vertexPropsHiddenHandle, VertexHiddenProp);
            whiteBox.mesh.property(vertexPropsHiddenHandle).set_persistent(true);

            // request default properties required for all white box meshes
            whiteBox.mesh.request_face_normals();
            whiteBox.mesh.request_halfedge_texcoords2D();
        }

        WhiteBoxMeshPtr CreateWhiteBoxMesh()
        {
            auto whiteBox = WhiteBoxMeshPtr(aznew WhiteBoxMesh());
            InitializeWhiteBoxMesh(*whiteBox);

            return whiteBox;
        }

        void WhiteBoxMeshDeleter::DestroyWhiteBoxMesh(WhiteBoxMesh* whiteBox)
        {
            delete whiteBox;
        }

        VertexHandles MeshVertexHandles(const WhiteBoxMesh& whiteBox)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            VertexHandles vertexHandles;
            vertexHandles.reserve(whiteBox.mesh.n_vertices());
            for (const auto& vertexHandle : whiteBox.mesh.vertices())
            {
                vertexHandles.push_back(wb_vh(vertexHandle));
            }

            return vertexHandles;
        }

        FaceHandles MeshFaceHandles(const WhiteBoxMesh& whiteBox)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            FaceHandles faceHandles;
            faceHandles.reserve(whiteBox.mesh.n_faces());
            for (const auto& faceHandle : whiteBox.mesh.faces())
            {
                faceHandles.push_back(wb_fh(faceHandle));
            }

            return faceHandles;
        }

        PolygonHandles MeshPolygonHandles(const WhiteBoxMesh& whiteBox)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            PolygonPropertyHandle polygonPropsHandle;
            whiteBox.mesh.get_property_handle(polygonPropsHandle, PolygonProps);

            const auto& polygonProps = whiteBox.mesh.property(polygonPropsHandle);

            AZStd::vector<PolygonHandle> polygonHandles;
            for (const auto& polygonProp : polygonProps)
            {
                // don't add duplicate polygons
                PolygonHandle polygonHandle = PolygonHandleFromInternal(polygonProp.second);
                if (AZStd::find(polygonHandles.begin(), polygonHandles.end(), polygonHandle) == polygonHandles.end())
                {
                    polygonHandles.push_back(AZStd::move(polygonHandle));
                }
            }

            return polygonHandles;
        }

        EdgeHandlesCollection PolygonBorderEdgeHandles(const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const HalfedgeHandlesCollection halfedgeHandlesCollection =
                PolygonBorderHalfedgeHandles(whiteBox, polygonHandle);

            EdgeHandlesCollection orderedEdgeHandlesCollection;
            orderedEdgeHandlesCollection.reserve(halfedgeHandlesCollection.size());

            for (const auto& halfedgeHandles : halfedgeHandlesCollection)
            {
                EdgeHandles orderedEdgeHandles;
                orderedEdgeHandles.reserve(halfedgeHandles.size());

                for (const auto& halfedgeHandle : halfedgeHandles)
                {
                    orderedEdgeHandles.push_back(HalfedgeEdgeHandle(whiteBox, halfedgeHandle));
                }

                orderedEdgeHandlesCollection.push_back(orderedEdgeHandles);
            }

            return orderedEdgeHandlesCollection;
        }

        EdgeHandles PolygonBorderEdgeHandlesFlattened(const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const EdgeHandlesCollection borderEdgeHandlesCollection = PolygonBorderEdgeHandles(whiteBox, polygonHandle);

            EdgeHandles polygonBorderEdgeHandles;
            for (const auto& borderEdgeHandles : borderEdgeHandlesCollection)
            {
                polygonBorderEdgeHandles.insert(
                    polygonBorderEdgeHandles.end(), borderEdgeHandles.cbegin(), borderEdgeHandles.cend());
            }

            return polygonBorderEdgeHandles;
        }

        EdgeHandles MeshPolygonEdgeHandles(const WhiteBoxMesh& whiteBox)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            auto polygonHandles = MeshPolygonHandles(whiteBox);

            EdgeHandles allEdgeHandles;
            for (const auto& polygonHandle : polygonHandles)
            {
                auto polygonEdgeHandles = PolygonBorderEdgeHandlesFlattened(whiteBox, polygonHandle);
                allEdgeHandles.insert(allEdgeHandles.end(), polygonEdgeHandles.begin(), polygonEdgeHandles.end());
            }

            // remove duplicates
            AZStd::sort(allEdgeHandles.begin(), allEdgeHandles.end());
            allEdgeHandles.erase(AZStd::unique(allEdgeHandles.begin(), allEdgeHandles.end()), allEdgeHandles.end());
            return allEdgeHandles;
        }

        EdgeHandles MeshEdgeHandles(const WhiteBoxMesh& whiteBox)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            EdgeHandles edgeHandles;
            edgeHandles.reserve(whiteBox.mesh.n_edges());
            for (const auto& edgeHandle : whiteBox.mesh.edges())
            {
                edgeHandles.push_back(wb_eh(edgeHandle));
            }

            return edgeHandles;
        }

        EdgeTypes MeshUserEdgeHandles(const WhiteBoxMesh& whiteBox)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            EdgeHandles userEdgeHandles = MeshPolygonEdgeHandles(whiteBox);
            AZStd::sort(userEdgeHandles.begin(), userEdgeHandles.end());

            EdgeHandles allEdgeHandles = MeshEdgeHandles(whiteBox);
            AZStd::sort(allEdgeHandles.begin(), allEdgeHandles.end());

            EdgeHandles meshEdgeHandles;
            meshEdgeHandles.reserve(allEdgeHandles.size()); // over reserve vector

            AZStd::set_difference(
                allEdgeHandles.begin(), allEdgeHandles.end(), userEdgeHandles.begin(), userEdgeHandles.end(),
                AZStd::back_inserter(meshEdgeHandles));

            return {AZStd::move(userEdgeHandles), AZStd::move(meshEdgeHandles)};
        }

        AZStd::vector<AZ::Vector3> MeshVertexPositions(const WhiteBoxMesh& whiteBox)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            return VertexPositions(whiteBox, MeshVertexHandles(whiteBox));
        }

        HalfedgeHandles FaceHalfedgeHandles(const WhiteBoxMesh& whiteBox, const FaceHandle faceHandle)
        {
            HalfedgeHandles halfedgeHandles;
            halfedgeHandles.reserve(3);

            for (Mesh::ConstFaceHalfedgeCCWIter faceHalfedgeIt = whiteBox.mesh.cfh_ccwiter(om_fh(faceHandle));
                 faceHalfedgeIt.is_valid(); ++faceHalfedgeIt)
            {
                halfedgeHandles.push_back(wb_heh(*faceHalfedgeIt));
            }

            return halfedgeHandles;
        }

        EdgeHandles FaceEdgeHandles(const WhiteBoxMesh& whiteBox, const FaceHandle faceHandle)
        {
            if (!faceHandle.IsValid())
            {
                return {};
            }

            EdgeHandles edgeHandles;
            edgeHandles.reserve(3);

            for (const auto& halfedgeHandle : FaceHalfedgeHandles(whiteBox, faceHandle))
            {
                edgeHandles.push_back(HalfedgeEdgeHandle(whiteBox, halfedgeHandle));
            }

            return edgeHandles;
        }

        VertexHandles FaceVertexHandles(const WhiteBoxMesh& whiteBox, const FaceHandle faceHandle)
        {
            if (!faceHandle.IsValid())
            {
                return {};
            }

            VertexHandles vertexHandles;
            vertexHandles.reserve(3);

            for (const auto& halfedgeHandle : FaceHalfedgeHandles(whiteBox, faceHandle))
            {
                vertexHandles.emplace_back(HalfedgeVertexHandleAtTip(whiteBox, halfedgeHandle));
            }

            return vertexHandles;
        }

        AZStd::vector<AZ::Vector3> FaceVertexPositions(const WhiteBoxMesh& whiteBox, const FaceHandle faceHandle)
        {
            return VertexPositions(whiteBox, FaceVertexHandles(whiteBox, faceHandle));
        }

        AZStd::vector<AZ::Vector3> FacesPositions(const WhiteBoxMesh& whiteBox, const FaceHandles& faceHandles)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            AZStd::vector<AZ::Vector3> triangles;
            triangles.reserve(faceHandles.size() * 3);

            for (const auto& faceHandle : faceHandles)
            {
                const auto corners = FaceVertexPositions(whiteBox, faceHandle);
                triangles.insert(triangles.end(), corners.begin(), corners.end());
            }

            return triangles;
        }

        FaceHandle HalfedgeFaceHandle(const WhiteBoxMesh& whiteBox, const HalfedgeHandle halfedgeHandle)
        {
            if (halfedgeHandle.IsValid())
            {
                return wb_fh(whiteBox.mesh.face_handle(om_heh(halfedgeHandle)));
            }

            return {};
        }

        HalfedgeHandle HalfedgeOppositeHalfedgeHandle(const WhiteBoxMesh& whiteBox, const HalfedgeHandle halfedgeHandle)
        {
            if (halfedgeHandle.IsValid())
            {
                return wb_heh(whiteBox.mesh.opposite_halfedge_handle(om_heh(halfedgeHandle)));
            }

            return {};
        }

        FaceHandle HalfedgeOppositeFaceHandle(const WhiteBoxMesh& whiteBox, const HalfedgeHandle halfedgeHandle)
        {
            if (halfedgeHandle.IsValid())
            {
                return wb_fh(whiteBox.mesh.opposite_face_handle(om_heh(halfedgeHandle)));
            }

            return {};
        }

        HalfedgeHandles VertexOutgoingHalfedgeHandles(const WhiteBoxMesh& whiteBox, const VertexHandle vertexHandle)
        {
            HalfedgeHandles outgoingHalfEdgeHandles;

            for (auto oheh = whiteBox.mesh.cvoh_ccwbegin(om_vh(vertexHandle));
                 oheh != whiteBox.mesh.cvoh_ccwend(om_vh(vertexHandle)); ++oheh)
            {
                outgoingHalfEdgeHandles.push_back(wb_heh(*oheh));
            }

            return outgoingHalfEdgeHandles;
        }

        HalfedgeHandles VertexIncomingHalfedgeHandles(const WhiteBoxMesh& whiteBox, const VertexHandle vertexHandle)
        {
            HalfedgeHandles incomingHalfEdgeHandles;

            for (auto iheh = whiteBox.mesh.cvih_ccwbegin(om_vh(vertexHandle));
                 iheh != whiteBox.mesh.cvih_ccwend(om_vh(vertexHandle)); ++iheh)
            {
                incomingHalfEdgeHandles.push_back(wb_heh(*iheh));
            }

            return incomingHalfEdgeHandles;
        }

        HalfedgeHandles VertexHalfedgeHandles(const WhiteBoxMesh& whiteBox, const VertexHandle vertexHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            HalfedgeHandles outgoingHandles = VertexOutgoingHalfedgeHandles(whiteBox, vertexHandle);
            HalfedgeHandles incomingHandles = VertexIncomingHalfedgeHandles(whiteBox, vertexHandle);

            auto allHandles = AZStd::move(outgoingHandles);
            allHandles.insert(
                allHandles.end(), AZStd::make_move_iterator(incomingHandles.begin()),
                AZStd::make_move_iterator(incomingHandles.end()));

            return allHandles;
        }

        EdgeHandles VertexEdgeHandles(const WhiteBoxMesh& whiteBox, const VertexHandle vertexHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const auto omVertexHandle = om_vh(vertexHandle);

            return AZStd::accumulate(
                whiteBox.mesh.cve_ccwbegin(omVertexHandle), whiteBox.mesh.cve_ccwend(omVertexHandle), EdgeHandles{},
                [](EdgeHandles edgeHandles, const auto edgeHandle)
                {
                    edgeHandles.push_back(wb_eh(edgeHandle));
                    return edgeHandles;
                });
        }

        static bool BuildFaceHandles(
            const WhiteBoxMesh& whiteBox, const FaceHandle faceHandle, FaceHandles& faceHandles,
            const AZ::Vector3& normal)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const auto* const found_fh = AZStd::find(faceHandles.cbegin(), faceHandles.cend(), faceHandle);

            if (found_fh == faceHandles.cend())
            {
                const AZ::Vector3 nextNormal = FaceNormal(whiteBox, faceHandle).GetNormalized();
                if (OpenMesh::dot(nextNormal, normal) > NormalTolerance)
                {
                    faceHandles.push_back(faceHandle);
                    return true;
                }
            }

            return false;
        }

        static FaceHandle OppositeFaceHandle(const WhiteBoxMesh& whiteBox, const HalfedgeHandle halfedgeHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const HalfedgeHandle oppositeHalfedgeHandle = HalfedgeOppositeHalfedgeHandle(whiteBox, halfedgeHandle);

            if (HalfedgeIsBoundary(whiteBox, oppositeHalfedgeHandle))
            {
                return {};
            }

            // note: oppositeFaceHandle will be invalid if oppositeHalfedgeHandle is a boundary
            const FaceHandle oppositeFaceHandle = HalfedgeFaceHandle(whiteBox, oppositeHalfedgeHandle);

            return oppositeFaceHandle;
        }

        static void SideFaceHandlesInternal(
            const WhiteBoxMesh& whiteBox, const FaceHandle faceHandle, FaceHandles& faceHandles,
            const AZ::Vector3& normal)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            if (BuildFaceHandles(whiteBox, faceHandle, faceHandles, normal))
            {
                // all halfedges for a given face
                const auto halfedges = FaceHalfedgeHandles(whiteBox, faceHandle);

                for (const HalfedgeHandle& halfedgeHandle : halfedges)
                {
                    const FaceHandle oppositeFaceHandle = OppositeFaceHandle(whiteBox, halfedgeHandle);

                    if (oppositeFaceHandle.IsValid())
                    {
                        SideFaceHandlesInternal(whiteBox, oppositeFaceHandle, faceHandles, normal);
                    }
                }
            }
        }

        FaceHandles SideFaceHandles(const WhiteBoxMesh& whiteBox, const FaceHandle faceHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            FaceHandles faceHandles;
            SideFaceHandlesInternal(
                whiteBox, faceHandle, faceHandles, FaceNormal(whiteBox, faceHandle).GetNormalized());

            return faceHandles;
        }

        static HalfedgeHandlesCollection BorderHalfedgeHandles(
            const WhiteBoxMesh& whiteBox, const FaceHandles& faceHandles)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            // build all possible halfedge handles
            HalfedgeHandles halfedgeHandles;
            for (const auto& faceHandle : faceHandles)
            {
                // find all vertices for a given face
                const auto vertexHandles = FaceVertexHandles(whiteBox, faceHandle);
                for (const auto& vertexHandle : vertexHandles)
                {
                    // find all outgoing halfedges from vertex
                    const auto outgoingHalfedgeHandles = VertexOutgoingHalfedgeHandles(whiteBox, vertexHandle);
                    for (const auto& halfedgeHandle : outgoingHalfedgeHandles)
                    {
                        // find what face corresponds to this halfedge
                        const FaceHandle halfedgeFaceHandle = HalfedgeFaceHandle(whiteBox, halfedgeHandle);

                        // if the halfedge corresponds to a face on this side
                        if (AZStd::find(faceHandles.cbegin(), faceHandles.cend(), halfedgeFaceHandle) !=
                            faceHandles.end())
                        {
                            // check the opposite face handle
                            const FaceHandle oppositeFaceHandle = HalfedgeOppositeFaceHandle(whiteBox, halfedgeHandle);

                            // if the opposite face handle isn't on this side, we know it is a 'boundary' halfedge
                            if (AZStd::find(faceHandles.cbegin(), faceHandles.cend(), oppositeFaceHandle) ==
                                faceHandles.end())
                            {
                                // check we haven't already stored this halfedge
                                if (AZStd::find(halfedgeHandles.cbegin(), halfedgeHandles.cend(), halfedgeHandle) ==
                                    halfedgeHandles.cend())
                                {
                                    // add to border halfedges
                                    halfedgeHandles.push_back(halfedgeHandle);
                                }
                            }
                        }
                    }
                }
            }

            // handle potentially pathological case where all edges have
            // been hidden and no halfedge loop can be found
            if (halfedgeHandles.empty())
            {
                return {};
            }

            HalfedgeHandlesCollection orderHalfedgeHandlesCollection;

            // can sort based on tip/tail
            HalfedgeHandles orderedHalfedgeHandles;
            orderedHalfedgeHandles.push_back(halfedgeHandles.back());
            halfedgeHandles.pop_back();

            // empty our list of unordered border side halfedge handles
            while (!halfedgeHandles.empty())
            {
                // use next vertex to get halfedges in order
                const VertexHandle nextVertex = HalfedgeVertexHandleAtTip(whiteBox, orderedHalfedgeHandles.back());

                // find next ordered halfedge
                const auto* const nextHalfedge = AZStd::find_if(
                    halfedgeHandles.cbegin(), halfedgeHandles.cend(),
                    [nextVertex, &whiteBox](auto halfedgeHandle)
                    {
                        return nextVertex == HalfedgeVertexHandleAtTail(whiteBox, halfedgeHandle);
                    });

                // if we found it
                if (nextHalfedge != halfedgeHandles.end())
                {
                    // add it to the ordered list and remove it from the unordered list
                    orderedHalfedgeHandles.push_back(*nextHalfedge);
                    halfedgeHandles[nextHalfedge - halfedgeHandles.begin()] = halfedgeHandles.back();
                    halfedgeHandles.pop_back();
                }
                else
                {
                    // cycle detected, start a new list
                    orderHalfedgeHandlesCollection.push_back(orderedHalfedgeHandles);
                    orderedHalfedgeHandles.clear();

                    orderedHalfedgeHandles.push_back(halfedgeHandles.back());
                    halfedgeHandles.pop_back();
                }
            }

            if (halfedgeHandles.empty())
            {
                AZ_Assert(!orderedHalfedgeHandles.empty(), "No ordered halfedges generated");
                orderHalfedgeHandlesCollection.push_back(orderedHalfedgeHandles);
            }

            // finally return the ordered list
            return orderHalfedgeHandlesCollection;
        }

        HalfedgeHandlesCollection SideBorderHalfedgeHandles(const WhiteBoxMesh& whiteBox, const FaceHandle faceHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            // find all face handles for a side
            return BorderHalfedgeHandles(whiteBox, SideFaceHandles(whiteBox, faceHandle));
        }

        static VertexHandlesCollection BorderVertexHandles(
            const WhiteBoxMesh& whiteBox, const HalfedgeHandlesCollection& halfedgeHandlesCollection)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            VertexHandlesCollection orderedVertexHandlesCollection;
            orderedVertexHandlesCollection.reserve(halfedgeHandlesCollection.size());

            for (const auto& halfedgeHandles : halfedgeHandlesCollection)
            {
                VertexHandles orderedVertexHandles;
                orderedVertexHandles.reserve(halfedgeHandles.size());

                for (const auto& halfedgeHandle : halfedgeHandles)
                {
                    orderedVertexHandles.push_back(HalfedgeVertexHandleAtTip(whiteBox, halfedgeHandle));
                }

                orderedVertexHandlesCollection.push_back(orderedVertexHandles);
            }

            return orderedVertexHandlesCollection;
        }

        VertexHandlesCollection SideBorderVertexHandles(const WhiteBoxMesh& whiteBox, const FaceHandle faceHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            return BorderVertexHandles(whiteBox, SideBorderHalfedgeHandles(whiteBox, faceHandle));
        }

        static VertexHandles FacesVertexHandles(const WhiteBoxMesh& whiteBox, const FaceHandles& faceHandles)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            VertexHandles vertexHandles;
            for (const FaceHandle& faceHandle : faceHandles)
            {
                const auto faceVertexHandles = FaceVertexHandles(whiteBox, faceHandle);
                for (const VertexHandle& faceVertexHandle : faceVertexHandles)
                {
                    const auto* const vertexIt =
                        AZStd::find(vertexHandles.cbegin(), vertexHandles.cend(), faceVertexHandle);

                    // ensure we do not add duplicate vertices
                    if (vertexIt == vertexHandles.end())
                    {
                        vertexHandles.push_back(faceVertexHandle);
                    }
                }
            }

            return vertexHandles;
        }

        VertexHandles SideVertexHandles(const WhiteBoxMesh& whiteBox, const FaceHandle faceHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            return FacesVertexHandles(whiteBox, SideFaceHandles(whiteBox, faceHandle));
        }

        VertexHandle HalfedgeVertexHandleAtTip(const WhiteBoxMesh& whiteBox, const HalfedgeHandle halfedgeHandle)
        {
            return wb_vh(whiteBox.mesh.to_vertex_handle(om_heh(halfedgeHandle)));
        }

        VertexHandle HalfedgeVertexHandleAtTail(const WhiteBoxMesh& whiteBox, const HalfedgeHandle halfedgeHandle)
        {
            return wb_vh(whiteBox.mesh.from_vertex_handle(om_heh(halfedgeHandle)));
        }

        AZ::Vector3 HalfedgeVertexPositionAtTip(const WhiteBoxMesh& whiteBox, HalfedgeHandle halfedgeHandle)
        {
            return VertexPosition(whiteBox, HalfedgeVertexHandleAtTip(whiteBox, halfedgeHandle));
        }

        AZ::Vector3 HalfedgeVertexPositionAtTail(const WhiteBoxMesh& whiteBox, HalfedgeHandle halfedgeHandle)
        {
            return VertexPosition(whiteBox, HalfedgeVertexHandleAtTail(whiteBox, halfedgeHandle));
        }

        EdgeHandle HalfedgeEdgeHandle(const WhiteBoxMesh& whiteBox, const HalfedgeHandle halfedgeHandle)
        {
            return wb_eh(whiteBox.mesh.edge_handle(om_heh(halfedgeHandle)));
        }

        bool HalfedgeIsBoundary(const WhiteBoxMesh& whiteBox, const HalfedgeHandle halfedgeHandle)
        {
            return whiteBox.mesh.is_boundary(om_heh(halfedgeHandle));
        }

        HalfedgeHandle HalfedgeHandleNext(const WhiteBoxMesh& whiteBox, const HalfedgeHandle halfedgeHandle)
        {
            return wb_heh(whiteBox.mesh.next_halfedge_handle(om_heh(halfedgeHandle)));
        }

        HalfedgeHandle HalfedgeHandlePrevious(const WhiteBoxMesh& whiteBox, const HalfedgeHandle halfedgeHandle)
        {
            return wb_heh(whiteBox.mesh.prev_halfedge_handle(om_heh(halfedgeHandle)));
        }

        static AZStd::array<AZ::Vector3, 2> EdgeVertexPositions(
            const WhiteBoxMesh& whiteBox, const EdgeHandle edgeHandle, const VertexHandle vertexHandle)
        {
            if (const auto vertexHandles = EdgeVertexHandles(whiteBox, edgeHandle); vertexHandle.IsValid())
            {
                const auto otherVertexHandle = vertexHandles[0] == vertexHandle ? vertexHandles[1] : vertexHandles[0];
                return {VertexPosition(whiteBox, vertexHandle), VertexPosition(whiteBox, otherVertexHandle)};
            }
            else
            {
                return {VertexPosition(whiteBox, vertexHandles[0]), VertexPosition(whiteBox, vertexHandles[1])};
            }
        }

        AZStd::array<AZ::Vector3, 2> EdgeVertexPositions(const WhiteBoxMesh& whiteBox, const EdgeHandle edgeHandle)
        {
            return EdgeVertexPositions(whiteBox, edgeHandle, VertexHandle{});
        }

        AZStd::array<VertexHandle, 2> EdgeVertexHandles(const WhiteBoxMesh& whiteBox, const EdgeHandle edgeHandle)
        {
            // note: first halfedge handle should always exist
            if (const auto halfedgeHandle = EdgeHalfedgeHandle(whiteBox, edgeHandle, EdgeHalfedge::First);
                halfedgeHandle.IsValid())
            {
                return {
                    HalfedgeVertexHandleAtTail(whiteBox, halfedgeHandle),
                    HalfedgeVertexHandleAtTip(whiteBox, halfedgeHandle)};
            }

            AZ_Assert(false, "Could not find Vertex Handles for Edge Handle %d", edgeHandle.Index());

            return {VertexHandle{}, VertexHandle{}};
        }

        // provide the ability to pass a vertex handle to explicitly determine the direction of the axis
        static AZ::Vector3 EdgeVectorWithStartingVertexHandle(
            const WhiteBoxMesh& whiteBox, const EdgeHandle edgeHandle, const VertexHandle vertexHandle)
        {
            const auto edgeVertexPositions = EdgeVertexPositions(whiteBox, edgeHandle, vertexHandle);
            return (edgeVertexPositions[1] - edgeVertexPositions[0]);
        }

        AZ::Vector3 EdgeVector(const WhiteBoxMesh& whiteBox, const EdgeHandle edgeHandle)
        {
            return EdgeVectorWithStartingVertexHandle(whiteBox, edgeHandle, VertexHandle{});
        }

        // provide the ability to pass a vertex handle to explicitly determine the direction of the axis
        static AZ::Vector3 EdgeAxisWithStartingVertexHandle(
            const WhiteBoxMesh& whiteBox, const EdgeHandle edgeHandle, const VertexHandle vertexHandle)
        {
            if (const AZ::Vector3 edgeVector = EdgeVectorWithStartingVertexHandle(whiteBox, edgeHandle, vertexHandle);
                edgeVector.GetLength() > 0.0f)
            {
                return edgeVector / edgeVector.GetLength();
            }

            return AZ::Vector3::CreateZero();
        }

        AZ::Vector3 EdgeAxis(const WhiteBoxMesh& whiteBox, const EdgeHandle edgeHandle)
        {
            return EdgeAxisWithStartingVertexHandle(whiteBox, edgeHandle, VertexHandle{});
        }

        bool EdgeIsBoundary(const WhiteBoxMesh& whiteBox, const EdgeHandle edgeHandle)
        {
            return whiteBox.mesh.is_boundary(om_eh(edgeHandle));
        }

        // note: halfedge handle must be from the edge handle passed in
        static bool EdgeIsUser(
            const WhiteBoxMesh& whiteBox, const HalfedgeHandle halfedgeHandle, const EdgeHandle edgeHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const auto polygonEdgeHandles = PolygonBorderEdgeHandlesFlattened(
                whiteBox, FacePolygonHandle(whiteBox, HalfedgeFaceHandle(whiteBox, halfedgeHandle)));

            return AZStd::find(polygonEdgeHandles.cbegin(), polygonEdgeHandles.cend(), edgeHandle) !=
                polygonEdgeHandles.cend();
        }

        // note: overload of EdgeIsUser that does not require halfedgeHande to be
        // passed in but does slightly more work
        static bool EdgeIsUser(const WhiteBoxMesh& whiteBox, const EdgeHandle edgeHandle)
        {
            const auto halfedgeHandle = EdgeHalfedgeHandles(whiteBox, edgeHandle);
            return AZStd::any_of(
                AZStd::cbegin(halfedgeHandle), AZStd::cend(halfedgeHandle),
                [&whiteBox, edgeHandle](const HalfedgeHandle halfedgeHandle)
                {
                    return EdgeIsUser(whiteBox, halfedgeHandle, edgeHandle);
                });
        }

        EdgeHandles EdgeGrouping(const WhiteBoxMesh& whiteBox, const EdgeHandle edgeHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            // a non-user ('mesh') edge is never part of a grouping so if one is passed
            // in ensure we return an empty group
            if (!EdgeIsUser(whiteBox, edgeHandle))
            {
                return EdgeHandles{};
            }

            // the edge group to begin building
            EdgeHandles edgeGrouping = {edgeHandle};

            // get vertex handles from the hovered/selected edge
            const auto initialVertexHandles = Api::EdgeVertexHandles(whiteBox, edgeHandle);
            auto vertexHandles = VertexHandles{initialVertexHandles.cbegin(), initialVertexHandles.cend()};

            // track all vertices we've already seen
            auto visitedVertexHandles = VertexHandles{};
            while (!vertexHandles.empty())
            {
                const auto vertexHandle = vertexHandles.back();
                vertexHandles.pop_back();

                // if the vertex is not hidden this is where the search ends
                if (!VertexIsHidden(whiteBox, vertexHandle))
                {
                    continue;
                }

                visitedVertexHandles.push_back(vertexHandle);

                // for all connected vertex handles to this edge
                for (const auto& vertexEdgeHandle : VertexEdgeHandles(whiteBox, vertexHandle))
                {
                    // check all halfedges in the edge
                    for (const auto& halfedgeHandle : EdgeHalfedgeHandles(whiteBox, vertexEdgeHandle))
                    {
                        // only track the edge if it's a 'user' edge (selectable - not a 'mesh' edge)
                        if (!EdgeIsUser(whiteBox, halfedgeHandle, vertexEdgeHandle))
                        {
                            continue;
                        }

                        // check if have we already added the edge to the grouping
                        if (AZStd::find(edgeGrouping.cbegin(), edgeGrouping.cend(), vertexEdgeHandle) !=
                            edgeGrouping.cend())
                        {
                            continue;
                        }

                        // store the edge to the grouping
                        edgeGrouping.push_back(vertexEdgeHandle);

                        for (const auto& nextVertexHandle : Api::EdgeVertexHandles(whiteBox, vertexEdgeHandle))
                        {
                            // if we haven't seen this vertex yet, add it to
                            // the vertex handles to explore
                            if (AZStd::find(
                                    visitedVertexHandles.cbegin(), visitedVertexHandles.cend(), nextVertexHandle) ==
                                visitedVertexHandles.cend())
                            {
                                vertexHandles.push_back(nextVertexHandle);
                            }
                        }
                    }
                }
            }

            return edgeGrouping;
        }

        bool EdgeIsHidden(const WhiteBoxMesh& whiteBox, const EdgeHandle edgeHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const EdgeHandles userEdgeHandles = MeshPolygonEdgeHandles(whiteBox);
            return AZStd::find(userEdgeHandles.cbegin(), userEdgeHandles.cend(), edgeHandle) == userEdgeHandles.cend();
        }

        AZStd::vector<FaceHandle> EdgeFaceHandles(const WhiteBoxMesh& whiteBox, const EdgeHandle edgeHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const auto openMeshEdgeHandle = om_eh(edgeHandle);
            const auto firstHalfedgeHandle = whiteBox.mesh.halfedge_handle(openMeshEdgeHandle, 0);
            const auto secondHalfedgeHandle = whiteBox.mesh.halfedge_handle(openMeshEdgeHandle, 1);

            AZ_Assert(
                firstHalfedgeHandle.is_valid() || secondHalfedgeHandle.is_valid(),
                "There should be at least one valid half edge handle for any given edge");

            AZStd::vector<FaceHandle> validFaceHandles;
            // only one face handle is valid at mesh boundaries
            if (const auto firstFaceHandle = whiteBox.mesh.face_handle(firstHalfedgeHandle); firstFaceHandle.is_valid())
            {
                validFaceHandles.push_back(wb_fh(firstFaceHandle));
            }

            if (const auto secondFaceHandle = whiteBox.mesh.face_handle(secondHalfedgeHandle);
                secondFaceHandle.is_valid())
            {
                validFaceHandles.push_back(wb_fh(secondFaceHandle));
            }

            return validFaceHandles;
        }

        static size_t EdgeHalfedgeMapping(const EdgeHalfedge edgeHalfedge)
        {
            switch (edgeHalfedge)
            {
            case EdgeHalfedge::First:
                return 0;
            case EdgeHalfedge::Second:
                return 1;
            default:
                AZ_Assert(false, "Invalid EdgeHalfedge type passed");
                return 2;
            }
        }

        HalfedgeHandle EdgeHalfedgeHandle(
            const WhiteBoxMesh& whiteBox, const EdgeHandle edgeHandle, const EdgeHalfedge edgeHalfedge)
        {
            return wb_heh(whiteBox.mesh.halfedge_handle(om_eh(edgeHandle), static_cast<unsigned int>(EdgeHalfedgeMapping(edgeHalfedge))));
        }

        HalfedgeHandles EdgeHalfedgeHandles(const WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const AZStd::array<HalfedgeHandle, 2> halfedgeHandles = {
                EdgeHalfedgeHandle(whiteBox, edgeHandle, EdgeHalfedge::First),
                EdgeHalfedgeHandle(whiteBox, edgeHandle, EdgeHalfedge::Second)};

            return AZStd::accumulate(
                halfedgeHandles.cbegin(), halfedgeHandles.cend(), HalfedgeHandles{},
                [&whiteBox](HalfedgeHandles halfedgeHandles, const HalfedgeHandle halfedgeHandle)
                {
                    if (!HalfedgeIsBoundary(whiteBox, halfedgeHandle))
                    {
                        halfedgeHandles.push_back(halfedgeHandle);
                    }

                    return halfedgeHandles;
                });
        }

        void TranslateEdge(WhiteBoxMesh& whiteBox, const EdgeHandle edgeHandle, const AZ::Vector3& displacement)
        {
            WHITEBOX_LOG(
                "White Box", "TranslateEdge eh(%s) %s", ToString(edgeHandle).c_str(),
                AZ::ToString(displacement).c_str());
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const auto vertexHandles = EdgeVertexHandles(whiteBox, edgeHandle);
            for (const auto& vertexHandle : vertexHandles)
            {
                auto position = VertexPosition(whiteBox, vertexHandle);
                position += displacement;
                SetVertexPosition(whiteBox, vertexHandle, position);
            }

            CalculateNormals(whiteBox);
            CalculatePlanarUVs(whiteBox);
        }

        // Given a displacement in local space applied to an edge, find the halfedge handle that the
        // edge is most likely moving towards. We're attempting to infer the user's intention which is
        // never perfect so there's a chance we may not return the edge the user expects. On the
        // whole the heuristic used (delta distance moved towards a connected face midpoint) is pretty stable.
        static HalfedgeHandle FindBestFitHalfedge(
            WhiteBoxMesh& whiteBox, const EdgeHandle edgeHandle, const AZ::Vector3& displacement)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            // get both halfedge handles for the edge (0 and 1 just correspond to each halfedge)
            const HalfedgeHandle firstHalfedgeHandle = EdgeHalfedgeHandle(whiteBox, edgeHandle, EdgeHalfedge::First);
            const HalfedgeHandle secondHalfedgeHandle = EdgeHalfedgeHandle(whiteBox, edgeHandle, EdgeHalfedge::Second);

            // get all vertices for each face (triangle) that each halfedge corresponds to
            const auto firstFaceVertexHandles =
                FaceVertexHandles(whiteBox, HalfedgeFaceHandle(whiteBox, firstHalfedgeHandle));
            const auto secondFaceVertexHandles =
                FaceVertexHandles(whiteBox, HalfedgeFaceHandle(whiteBox, secondHalfedgeHandle));

            // calculate the midpoint of each face
            const auto firstFaceMidpoint = VerticesMidpoint(whiteBox, firstFaceVertexHandles);
            const auto secondFaceMidpoint = VerticesMidpoint(whiteBox, secondFaceVertexHandles);

            // calculate the midpoint of the edge we wish to append to and where it will be after the displacement
            const auto edgeMidpoint = EdgeMidpoint(whiteBox, edgeHandle);
            const auto nextEdgePosition = edgeMidpoint + displacement;

            // calculate how far the center of each face is from the edge midpoint
            const auto distanceFromFirstFace = (firstFaceMidpoint - edgeMidpoint).GetLength();
            const auto distanceFromSecondFace = (secondFaceMidpoint - edgeMidpoint).GetLength();

            // then calculate how far the center of each face is from the edge midpoint plus the displacement
            const auto nextDistanceFromFirstFace = (nextEdgePosition - firstFaceMidpoint).GetLength();
            const auto nextDistanceFromSecondFace = (nextEdgePosition - secondFaceMidpoint).GetLength();

            // next see what the delta is from next and current positions
            // this is to determine did the displacement move us towards the first or second face
            // i.e. infer which way the user dragged
            const auto nextDeltaFromFirstFace = nextDistanceFromFirstFace - distanceFromFirstFace;
            const auto nextDeltaFromSecondFace = nextDistanceFromSecondFace - distanceFromSecondFace;

            // pick the best halfedge we inferred
            const EdgeHalfedge halfedge =
                nextDeltaFromFirstFace < nextDeltaFromSecondFace ? EdgeHalfedge::First : EdgeHalfedge::Second;

            return EdgeHalfedgeHandle(whiteBox, edgeHandle, halfedge);
        }

        // Determine the vertices required to append the new edge geometry being created
        static Internal::EdgeAppendVertexHandles CalculateEdgeAppendVertexHandles(
            WhiteBoxMesh& whiteBox, const EdgeHandle edgeHandle, const AZ::Vector3& displacement)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            // based on the displacement find which halfedge is a better fit (which direction did we move in)
            const HalfedgeHandle halfedgeHandle = FindBestFitHalfedge(whiteBox, edgeHandle, displacement);

            const FaceHandle faceHandle = HalfedgeFaceHandle(whiteBox, halfedgeHandle);

            // find the polygon this face handle corresponds to
            const PolygonHandle polygonHandle = FacePolygonHandle(whiteBox, faceHandle);
            // find all border vertex handles for this polygon
            const VertexHandlesCollection polygonBorderVertexHandlesCollection =
                PolygonBorderVertexHandles(whiteBox, polygonHandle);

            // following the direction of the halfedge, what vertex is it pointing to
            const VertexHandle toVertexHandle = HalfedgeVertexHandleAtTip(whiteBox, halfedgeHandle);
            // following the direction of the halfedge, what vertex is coming from
            const VertexHandle fromVertexHandle = HalfedgeVertexHandleAtTail(whiteBox, halfedgeHandle);

            // find which vertex loop the vertex is in based on the halfedge we've selected
            const VertexHandles borderVertexHandles = AZStd::accumulate(
                AZStd::begin(polygonBorderVertexHandlesCollection), AZStd::end(polygonBorderVertexHandlesCollection),
                VertexHandles{},
                [toVertexHandle](VertexHandles borderVertexHandles, const VertexHandles& vertexHandles)
                {
                    // check if the vertex is in this loop of the collection
                    // (there may be 1 - * loops in the collection)
                    if (AZStd::find(AZStd::begin(vertexHandles), AZStd::end(vertexHandles), toVertexHandle) !=
                        AZStd::end(vertexHandles))
                    {
                        // if so add the vertex handles for this loop to be returned
                        borderVertexHandles.insert(
                            borderVertexHandles.end(), vertexHandles.begin(), vertexHandles.end());
                    }

                    return borderVertexHandles;
                });

            // find the index of the vertex handle in the polygon handle collection
            const auto toVertexHandlePolygonIndex =
                AZStd::find(borderVertexHandles.begin(), borderVertexHandles.end(), toVertexHandle) -
                borderVertexHandles.begin();
            const auto fromVertexHandlePolygonIndex =
                AZStd::find(borderVertexHandles.begin(), borderVertexHandles.end(), fromVertexHandle) -
                borderVertexHandles.begin();

            // we then want to find the vertex after the 'to' vertex, and the vertex before the 'from' vertex
            const VertexHandle afterToVertexHandle = borderVertexHandles
                [((toVertexHandlePolygonIndex + borderVertexHandles.size()) + 1) % borderVertexHandles.size()];
            const VertexHandle beforeFromVertexHandle = borderVertexHandles
                [((fromVertexHandlePolygonIndex + borderVertexHandles.size()) - 1) % borderVertexHandles.size()];

            // find the position of the 'to' and 'from' vertex handle
            const Mesh::Point toVertexPosition = VertexPosition(whiteBox, toVertexHandle);
            const Mesh::Point fromVertexPosition = VertexPosition(whiteBox, fromVertexHandle);

            // find the next position by moving the previous positions by the displacement
            const Mesh::Point nextToVertexPosition = toVertexPosition + displacement;
            const Mesh::Point nextFromVertexPosition = fromVertexPosition + displacement;

            // add two new vertices in the new positions
            const VertexHandle addedToVertexHandle = AddVertex(whiteBox, nextToVertexPosition);
            const VertexHandle addedFromVertexHandle = AddVertex(whiteBox, nextFromVertexPosition);

            // populate data for the next stage
            Internal::EdgeAppendVertexHandles edgeAppendHandles;
            edgeAppendHandles.m_existingPolygonHandle = polygonHandle;
            edgeAppendHandles.m_toVertexHandle = toVertexHandle;
            edgeAppendHandles.m_fromVertexHandle = fromVertexHandle;
            edgeAppendHandles.m_addedFromVertexHandle = addedFromVertexHandle;
            edgeAppendHandles.m_addedToVertexHandle = addedToVertexHandle;
            edgeAppendHandles.m_afterToVertexHandle = afterToVertexHandle;
            edgeAppendHandles.m_beforeFromVertexHandle = beforeFromVertexHandle;

            return edgeAppendHandles;
        }

        // After determining the vertex handles required, build the polygons for the new appended edge
        static Internal::EdgeAppendPolygonHandles AddNewPolygonsForEdgeAppend(
            WhiteBoxMesh& whiteBox, const Internal::EdgeAppendVertexHandles& edgeAppendVertexHandles)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            Internal::EdgeAppendPolygonHandles edgeAppendPolygonHandles;

            // build two faces to make up the polygon on the 'near' side of the edge
            AZStd::vector<FaceVertHandles> nearFaceHandles = {
                FaceVertHandles{
                    edgeAppendVertexHandles.m_fromVertexHandle, edgeAppendVertexHandles.m_toVertexHandle,
                    edgeAppendVertexHandles.m_addedToVertexHandle},
                FaceVertHandles{
                    edgeAppendVertexHandles.m_fromVertexHandle, edgeAppendVertexHandles.m_addedToVertexHandle,
                    edgeAppendVertexHandles.m_addedFromVertexHandle}};

            edgeAppendPolygonHandles.m_nearPolygonHandle = AddPolygon(whiteBox, nearFaceHandles);

            AZStd::vector<FaceVertHandles> farFaceHandles;
            // note: need to check the number of faces for the polygon we'll be replacing with the edge append
            if (edgeAppendVertexHandles.m_existingPolygonHandle.m_faceHandles.size() > 1)
            {
                // build two faces to make up the polygon on the 'far' side of the edge
                farFaceHandles = {
                    FaceVertHandles{
                        edgeAppendVertexHandles.m_addedFromVertexHandle, edgeAppendVertexHandles.m_addedToVertexHandle,
                        edgeAppendVertexHandles.m_afterToVertexHandle},
                    FaceVertHandles{
                        edgeAppendVertexHandles.m_addedFromVertexHandle, edgeAppendVertexHandles.m_afterToVertexHandle,
                        edgeAppendVertexHandles.m_beforeFromVertexHandle}};
            }
            else
            {
                // build one face to make up the polygon on the 'far' side of the edge
                // if we're extruding an edge on a triangle not a quad
                farFaceHandles = {FaceVertHandles{
                    edgeAppendVertexHandles.m_addedFromVertexHandle, edgeAppendVertexHandles.m_addedToVertexHandle,
                    edgeAppendVertexHandles.m_afterToVertexHandle}};
            }

            edgeAppendPolygonHandles.m_farPolygonHandle = AddPolygon(whiteBox, farFaceHandles);

            // add the top triangle for the edge extrusion
            const AZStd::vector<FaceVertHandles> topFaceHandles = {FaceVertHandles{
                edgeAppendVertexHandles.m_fromVertexHandle, edgeAppendVertexHandles.m_addedFromVertexHandle,
                edgeAppendVertexHandles.m_beforeFromVertexHandle}};

            edgeAppendPolygonHandles.m_topPolygonHandle = AddPolygon(whiteBox, topFaceHandles);

            // add the bottom triangle for the edge extrusion
            const AZStd::vector<FaceVertHandles> bottomFaceHandles = {FaceVertHandles{
                edgeAppendVertexHandles.m_toVertexHandle, edgeAppendVertexHandles.m_afterToVertexHandle,
                edgeAppendVertexHandles.m_addedToVertexHandle}};

            edgeAppendPolygonHandles.m_bottomPolygonHandle = AddPolygon(whiteBox, bottomFaceHandles);

            return edgeAppendPolygonHandles;
        }

        // given two polygon handles, return the (first) edge that is shared between the two polygons
        // note: this may not always give expected results for polygons with greater than two faces
        static EdgeHandle FindSelectedEdgeHandle(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& nearPolygonHandle, const PolygonHandle& farPolygonHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            // actually find the new edge we created
            const EdgeHandles nearEdgeHandles = PolygonBorderEdgeHandlesFlattened(whiteBox, nearPolygonHandle);
            const EdgeHandles farEdgeHandles = PolygonBorderEdgeHandlesFlattened(whiteBox, farPolygonHandle);

            // add all edges and find the one duplicate (this will be the new edge we want to return to the caller)
            EdgeHandles allEdgeHandles;
            allEdgeHandles.reserve(nearEdgeHandles.size() + farEdgeHandles.size());
            allEdgeHandles.insert(allEdgeHandles.end(), nearEdgeHandles.begin(), nearEdgeHandles.end());
            allEdgeHandles.insert(allEdgeHandles.end(), farEdgeHandles.begin(), farEdgeHandles.end());
            AZStd::sort(allEdgeHandles.begin(), allEdgeHandles.end());

            auto* edgeIt = AZStd::adjacent_find(allEdgeHandles.begin(), allEdgeHandles.end());
            if (edgeIt != allEdgeHandles.end())
            {
                return *edgeIt;
            }

            return EdgeHandle{};
        }

        static bool EdgeExtrusionAllowed(const PolygonHandle& polygonHandle)
        {
            // currently only allow edge extrusion for quad polygons
            return polygonHandle.m_faceHandles.size() <= 2;
        }

        EdgeHandle TranslateEdgeAppend(
            WhiteBoxMesh& whiteBox, const EdgeHandle edgeHandle, const AZ::Vector3& displacement)
        {
            WHITEBOX_LOG(
                "White Box", "TranslateEdgeAppend eh(%s) %s", ToString(edgeHandle).c_str(),
                AZ::ToString(displacement).c_str());
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            // the new and existing handles required for an edge append
            const Internal::EdgeAppendVertexHandles edgeAppendVertexHandles =
                CalculateEdgeAppendVertexHandles(whiteBox, edgeHandle, displacement);

            // if edge extrusion is not allowed simply return the previous edge handle
            if (!EdgeExtrusionAllowed(edgeAppendVertexHandles.m_existingPolygonHandle))
            {
                return edgeHandle;
            }

            // remove the current polygon (two new polygons will later be inserted in its place)
            RemoveFaces(whiteBox, edgeAppendVertexHandles.m_existingPolygonHandle.m_faceHandles);

            const Internal::EdgeAppendPolygonHandles edgeAppendPolygonHandles =
                AddNewPolygonsForEdgeAppend(whiteBox, edgeAppendVertexHandles);

            // update internal state
            CalculateNormals(whiteBox);
            CalculatePlanarUVs(whiteBox);

            return FindSelectedEdgeHandle(
                whiteBox, edgeAppendPolygonHandles.m_nearPolygonHandle, edgeAppendPolygonHandles.m_farPolygonHandle);
        }

        AZ::Vector3 PolygonNormal(const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            return AZStd::accumulate(
                       polygonHandle.m_faceHandles.cbegin(), polygonHandle.m_faceHandles.cend(),
                       AZ::Vector3::CreateZero(),
                       [&whiteBox](const AZ::Vector3& normal, const FaceHandle faceHandle)
                       {
                           return normal + FaceNormal(whiteBox, faceHandle);
                       })
                .GetNormalizedSafe();
        }

        PolygonHandle FacePolygonHandle(const WhiteBoxMesh& whiteBox, const FaceHandle faceHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            PolygonPropertyHandle polygonPropsHandle;
            whiteBox.mesh.get_property_handle(polygonPropsHandle, PolygonProps);

            const auto& polygonProps = whiteBox.mesh.property(polygonPropsHandle);
            const auto polygonIt = polygonProps.find(om_fh(faceHandle));

            if (polygonIt != polygonProps.end())
            {
                return PolygonHandleFromInternal(polygonIt->second);
            }

            return {};
        }

        VertexHandles PolygonVertexHandles(const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            return FacesVertexHandles(whiteBox, polygonHandle.m_faceHandles);
        }

        VertexHandlesCollection PolygonBorderVertexHandles(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            return BorderVertexHandles(whiteBox, PolygonBorderHalfedgeHandles(whiteBox, polygonHandle));
        }

        VertexHandles PolygonBorderVertexHandlesFlattened(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const VertexHandlesCollection borderVertexHandlesCollection =
                BorderVertexHandles(whiteBox, PolygonBorderHalfedgeHandles(whiteBox, polygonHandle));

            VertexHandles polygonBorderVertexHandles;
            for (const auto& borderVertexHandles : borderVertexHandlesCollection)
            {
                polygonBorderVertexHandles.insert(
                    polygonBorderVertexHandles.end(), borderVertexHandles.cbegin(), borderVertexHandles.cend());
            }

            return polygonBorderVertexHandles;
        }

        HalfedgeHandles PolygonBorderHalfedgeHandlesFlattened(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const HalfedgeHandlesCollection borderHalfedgeHandlesCollection =
                PolygonBorderHalfedgeHandles(whiteBox, polygonHandle);

            HalfedgeHandles polygonBorderHalfedgeHandles;
            for (const auto& borderHalfedgeHandles : borderHalfedgeHandlesCollection)
            {
                polygonBorderHalfedgeHandles.insert(
                    polygonBorderHalfedgeHandles.end(), borderHalfedgeHandles.cbegin(), borderHalfedgeHandles.cend());
            }

            return polygonBorderHalfedgeHandles;
        }

        HalfedgeHandles PolygonHalfedgeHandles(const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            return AZStd::accumulate(
                polygonHandle.m_faceHandles.cbegin(), polygonHandle.m_faceHandles.cend(), HalfedgeHandles{},
                [&whiteBox](HalfedgeHandles halfedges, const FaceHandle faceHandle)
                {
                    const HalfedgeHandles nextHalfedgeHandles = FaceHalfedgeHandles(whiteBox, faceHandle);
                    halfedges.insert(halfedges.end(), nextHalfedgeHandles.cbegin(), nextHalfedgeHandles.cend());
                    return halfedges;
                });
        }

        HalfedgeHandlesCollection PolygonBorderHalfedgeHandles(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle)
        {
            return BorderHalfedgeHandles(whiteBox, polygonHandle.m_faceHandles);
        }

        AZStd::vector<AZ::Vector3> PolygonVertexPositions(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            return VertexPositions(whiteBox, PolygonVertexHandles(whiteBox, polygonHandle));
        }

        VertexPositionsCollection PolygonBorderVertexPositions(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const auto polygonBorderVertexHandlesCollection = PolygonBorderVertexHandles(whiteBox, polygonHandle);
            VertexPositionsCollection polygonBorderVertexPositionsCollection;
            polygonBorderVertexPositionsCollection.reserve(polygonBorderVertexHandlesCollection.size());

            for (const auto& polygonBorderVertexHandles : polygonBorderVertexHandlesCollection)
            {
                polygonBorderVertexPositionsCollection.push_back(VertexPositions(whiteBox, polygonBorderVertexHandles));
            }

            return polygonBorderVertexPositionsCollection;
        }

        AZStd::vector<AZ::Vector3> PolygonFacesPositions(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            return FacesPositions(whiteBox, polygonHandle.m_faceHandles);
        }

        AZ::Vector3 VertexPosition(const WhiteBoxMesh& whiteBox, const VertexHandle vertexHandle)
        {
            return whiteBox.mesh.point(om_vh(vertexHandle));
        }

        AZStd::vector<AZ::Vector3> VertexPositions(const WhiteBoxMesh& whiteBox, const VertexHandles& vertexHandles)
        {
            AZStd::vector<AZ::Vector3> positions;
            positions.reserve(vertexHandles.size());

            AZStd::transform(
                vertexHandles.cbegin(), vertexHandles.cend(), AZStd::back_inserter(positions),
                [&whiteBox](const auto vertexHandle)
                {
                    return VertexPosition(whiteBox, vertexHandle);
                });

            return positions;
        }

        EdgeHandles VertexUserEdgeHandles(const WhiteBoxMesh& whiteBox, const VertexHandle vertexHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            auto vertexEdgeHandles = VertexEdgeHandles(whiteBox, vertexHandle);

            vertexEdgeHandles.erase(
                AZStd::remove_if(
                    AZStd::begin(vertexEdgeHandles), AZStd::end(vertexEdgeHandles),
                    [&whiteBox](const EdgeHandle edgeHandle)
                    {
                        return !EdgeIsUser(whiteBox, edgeHandle);
                    }),
                AZStd::end(vertexEdgeHandles));

            return vertexEdgeHandles;
        }

        template<typename EdgeFn>
        static AZStd::vector<AZ::Vector3> VertexUserEdges(
            const WhiteBoxMesh& whiteBox, const VertexHandle vertexHandle, EdgeFn&& edgeFn)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const auto vertexEdgeHandles = VertexUserEdgeHandles(whiteBox, vertexHandle);

            AZStd::vector<AZ::Vector3> edgeVectors;
            edgeVectors.reserve(vertexEdgeHandles.size());

            AZStd::transform(
                AZStd::cbegin(vertexEdgeHandles), AZStd::cend(vertexEdgeHandles), AZStd::back_inserter(edgeVectors),
                [&whiteBox, edgeFn, vertexHandle](const EdgeHandle edgeHandle)
                {
                    return edgeFn(whiteBox, edgeHandle, vertexHandle);
                });

            // filter out any invalid edges
            edgeVectors.erase(
                AZStd::remove_if(
                    AZStd::begin(edgeVectors), AZStd::end(edgeVectors),
                    [](const AZ::Vector3& edge)
                    {
                        return AZ::IsCloseMag<float>(edge.GetLengthSq(), 0.0f);
                    }),
                AZStd::end(edgeVectors));

            return edgeVectors;
        }

        AZStd::vector<AZ::Vector3> VertexUserEdgeVectors(const WhiteBoxMesh& whiteBox, const VertexHandle vertexHandle)
        {
            return VertexUserEdges(whiteBox, vertexHandle, &EdgeVectorWithStartingVertexHandle);
        }

        AZStd::vector<AZ::Vector3> VertexUserEdgeAxes(const WhiteBoxMesh& whiteBox, const VertexHandle vertexHandle)
        {
            return VertexUserEdges(whiteBox, vertexHandle, &EdgeAxisWithStartingVertexHandle);
        }

        bool VertexIsHidden(const WhiteBoxMesh& whiteBox, const VertexHandle vertexHandle)
        {
            VertexBoolPropertyHandle vertexPropsHiddenHandle;
            whiteBox.mesh.get_property_handle(vertexPropsHiddenHandle, VertexHiddenProp);
            return whiteBox.mesh.property(vertexPropsHiddenHandle, om_vh(vertexHandle));
        }

        bool VertexIsIsolated(const WhiteBoxMesh& whiteBox, const VertexHandle vertexHandle)
        {
            const auto connectedEdgeHandles = VertexEdgeHandles(whiteBox, vertexHandle);
            return AZStd::all_of(
                AZStd::cbegin(connectedEdgeHandles), AZStd::cend(connectedEdgeHandles),
                [&whiteBox](const EdgeHandle edgeHandle)
                {
                    return !EdgeIsUser(whiteBox, edgeHandle);
                });
        }

        AZ::Vector3 FaceNormal(const WhiteBoxMesh& whiteBox, const FaceHandle faceHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);
            return whiteBox.mesh.normal(om_fh(faceHandle));
        }

        AZ::Vector2 HalfedgeUV(const WhiteBoxMesh& whiteBox, const HalfedgeHandle halfedgeHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);
            return whiteBox.mesh.texcoord2D(om_heh(halfedgeHandle));
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // note: MeshXXXCount functions are only valid if garbage_collection is called after
        // each face/vertex removal. If garbage_collection is deferred, the faces()/vertices()/halfedges()
        // range must be used to count iterations via skipping iterator to ignore deleted faces

        size_t MeshFaceCount(const WhiteBoxMesh& whiteBox)
        {
            return whiteBox.mesh.n_faces();
        }

        size_t MeshHalfedgeCount(const WhiteBoxMesh& whiteBox)
        {
            return whiteBox.mesh.n_halfedges();
        }

        size_t MeshVertexCount(const WhiteBoxMesh& whiteBox)
        {
            return whiteBox.mesh.n_vertices();
        }
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        Faces MeshFaces(const WhiteBoxMesh& whiteBox)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            Faces faces;
            faces.reserve(MeshFaceCount(whiteBox));
            for (const auto& faceHandle : MeshFaceHandles(whiteBox))
            {
                const auto halfEdgeHandles = FaceHalfedgeHandles(whiteBox, faceHandle);

                Face face;
                AZStd::transform(
                    halfEdgeHandles.begin(), halfEdgeHandles.end(), face.begin(),
                    [&whiteBox](const auto halfedgeHandle)
                    {
                        // calculate the position of each vertex at the tip of each vertex handle
                        return VertexPosition(whiteBox, HalfedgeVertexHandleAtTip(whiteBox, halfedgeHandle));
                    });

                faces.push_back(face);
            }

            return faces;
        }

        void CalculatePlanarUVs(WhiteBoxMesh& whiteBox, const FaceHandles& faceHandles)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            auto& mesh = whiteBox.mesh;
            for (const auto& faceHandle : faceHandles)
            {
                for (Mesh::ConstFaceHalfedgeCCWIter faceHalfedgeIt = mesh.fh_ccwiter(om_fh(faceHandle));
                     faceHalfedgeIt.is_valid(); ++faceHalfedgeIt)
                {
                    const Mesh::HalfedgeHandle heh = *faceHalfedgeIt;
                    const Mesh::VertexHandle vh = mesh.to_vertex_handle(heh);

                    const AZ::Vector3 position = mesh.point(vh);
                    const AZ::Vector3 normal = FaceNormal(whiteBox, faceHandle);
                    const Mesh::TexCoord2D uv = CreatePlanarUVFromVertex(normal, position);

                    mesh.set_texcoord2D(heh, uv);
                }
            }
        }

        void CalculatePlanarUVs(WhiteBoxMesh& whiteBox)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            CalculatePlanarUVs(whiteBox, MeshFaceHandles(whiteBox));
        }

        static PolygonHandle MergeFaces(
            const WhiteBoxMesh& whiteBox, const HalfedgeHandle halfedgeHandle,
            const HalfedgeHandle oppositeHalfedgeHandle, const HalfedgeHandles& borderHalfedgeHandles,
            const EdgeHandles& buildingEdgeHandles)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            // the polygon handle to build
            PolygonHandle polygonHandle;

            // begin populating halfedges to visit to build a polygon
            HalfedgeHandles halfedgesToVisit{halfedgeHandle};
            // store already visited halfedges
            HalfedgeHandles visitedHalfedges;

            while (!halfedgesToVisit.empty())
            {
                const HalfedgeHandle halfedgeToVisit = halfedgesToVisit.back();
                halfedgesToVisit.pop_back();

                visitedHalfedges.push_back(halfedgeToVisit);

                const FaceHandle faceHandleToVisit = HalfedgeFaceHandle(whiteBox, halfedgeToVisit);
                const HalfedgeHandles faceHalfedges = FaceHalfedgeHandles(whiteBox, faceHandleToVisit);

                // check we have not already visited this face handle
                if (AZStd::find(
                        polygonHandle.m_faceHandles.cbegin(), polygonHandle.m_faceHandles.cend(), faceHandleToVisit) !=
                    polygonHandle.m_faceHandles.cend())
                {
                    continue;
                }

                // store the face handle in this polygon
                polygonHandle.m_faceHandles.push_back(faceHandleToVisit);

                // for all halfedges
                for (const auto& faceHalfedgeHandle : faceHalfedges)
                {
                    const EdgeHandle edgeHandle = HalfedgeEdgeHandle(whiteBox, faceHalfedgeHandle);
                    // if we haven't seen this halfedge before and we want to track it,
                    // store it in visited halfedges
                    if (faceHalfedgeHandle != oppositeHalfedgeHandle
                        // ignore border halfedges (not inside the polygon)
                        && AZStd::find(borderHalfedgeHandles.cbegin(), borderHalfedgeHandles.cend(), faceHalfedgeHandle) ==
                            borderHalfedgeHandles.cend()
                        // ensure we do not visit the same halfedge again
                        && AZStd::find(visitedHalfedges.cbegin(), visitedHalfedges.cend(), faceHalfedgeHandle) ==
                            visitedHalfedges.cend()
                        // ignore the halfedge if we've already tracked it in our 'building' list
                        && AZStd::find(buildingEdgeHandles.cbegin(), buildingEdgeHandles.cend(), edgeHandle) ==
                            buildingEdgeHandles.cend())
                    {
                        halfedgesToVisit.push_back(HalfedgeOppositeHalfedgeHandle(whiteBox, faceHalfedgeHandle));
                    }
                }
            }

            // return the polygon we've built by traversing all connected face handles
            // (by following the connected halfedges)
            return polygonHandle;
        }

        static void PopulatePolygonProps(FaceHandlePolygonMapping& polygonProps, const FaceHandles& faceHandles)
        {
            for (const auto& faceHandle : faceHandles)
            {
                auto polygonIt = polygonProps.find(om_fh(faceHandle));
                for (const auto& innerFaceHandle : faceHandles)
                {
                    polygonIt->second.push_back(om_fh(innerFaceHandle));
                }
            }
        }

        static void ClearPolygonProps(FaceHandlePolygonMapping& polygonProps, const FaceHandles& faceHandles)
        {
            for (const auto& faceHandle : faceHandles)
            {
                if (auto polygonIt = polygonProps.find(om_fh(faceHandle)); polygonIt != polygonProps.end())
                {
                    polygonIt->second.clear();
                }
            }
        }

        // restore all vertices along the restored edges (after creating a new polygon)
        static void RestoreVertexHandlesForEdges(WhiteBoxMesh& whiteBox, const EdgeHandles& restoredEdgeHandles)
        {
            for (const auto& edgeHandle : restoredEdgeHandles)
            {
                for (const auto& vertexHandle : EdgeVertexHandles(whiteBox, edgeHandle))
                {
                    RestoreVertex(whiteBox, vertexHandle);
                }
            }
        }

        AZStd::optional<AZStd::array<PolygonHandle, 2>> RestoreEdge(
            WhiteBoxMesh& whiteBox, const EdgeHandle edgeHandle, EdgeHandles& restoringEdgeHandles)
        {
            WHITEBOX_LOG("White Box", "RestoreEdge eh(%s)", ToString(edgeHandle).c_str());
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            // check we're not selecting an existing user edge
            if (!EdgeIsHidden(whiteBox, edgeHandle))
            {
                // do nothing
                return {};
            }

            // attempt to make a new polygon if possible
            const HalfedgeHandle firstHalfedgeHandle = EdgeHalfedgeHandle(whiteBox, edgeHandle, EdgeHalfedge::First);
            const HalfedgeHandle secondHalfedgeHandle = EdgeHalfedgeHandle(whiteBox, edgeHandle, EdgeHalfedge::Second);

            // the existing polygon, containing the edge we've selected
            const PolygonHandle polygonHandle =
                FacePolygonHandle(whiteBox, HalfedgeFaceHandle(whiteBox, firstHalfedgeHandle));
            // all border halfedges (not necessarily contiguous)
            const HalfedgeHandles polygonBorderHalfedgeHandles =
                PolygonBorderHalfedgeHandlesFlattened(whiteBox, polygonHandle);

            const auto firstPolygon = MergeFaces(
                whiteBox, firstHalfedgeHandle, secondHalfedgeHandle, polygonBorderHalfedgeHandles,
                restoringEdgeHandles);
            const auto secondPolygon = MergeFaces(
                whiteBox, secondHalfedgeHandle, firstHalfedgeHandle, polygonBorderHalfedgeHandles,
                restoringEdgeHandles);

            // check if the first and second polygons are identical,
            // this can happen if the vertex list forms a loop
            const bool identical = [first = firstPolygon, second = secondPolygon]() mutable
            {
                AZStd::sort(first.m_faceHandles.begin(), first.m_faceHandles.end());
                AZStd::sort(second.m_faceHandles.begin(), second.m_faceHandles.end());
                return first == second;
            }();

            // if the number of face handles in at least one of the new polygons is the
            // same as the existing polygon, we know a new polygon has not been formed
            // (the restored edge has not connected to another edge and created a new polygon)
            if (firstPolygon.m_faceHandles.size() == polygonHandle.m_faceHandles.size() || identical)
            {
                restoringEdgeHandles.push_back(edgeHandle);
                return {};
            }

            // get polygon property handle from mesh
            PolygonPropertyHandle polygonPropsHandle;
            whiteBox.mesh.get_property_handle(polygonPropsHandle, PolygonProps);
            auto& polygonProps = whiteBox.mesh.property(polygonPropsHandle);

            // update all face handles to refer to the new face handles in the group
            // clear existing face handles
            ClearPolygonProps(polygonProps, polygonHandle.m_faceHandles);
            // populate face handles for first polygon
            PopulatePolygonProps(polygonProps, firstPolygon.m_faceHandles);
            // populate face handles for second polygon
            PopulatePolygonProps(polygonProps, secondPolygon.m_faceHandles);

            // get all edges
            const auto firstPolygonEdges = Api::PolygonBorderEdgeHandlesFlattened(whiteBox, firstPolygon);
            const auto secondPolygonEdges = Api::PolygonBorderEdgeHandlesFlattened(whiteBox, secondPolygon);

            Api::EdgeHandles allPolygonEdges;
            allPolygonEdges.reserve(firstPolygonEdges.size() + secondPolygonEdges.size());
            allPolygonEdges.insert(allPolygonEdges.end(), firstPolygonEdges.begin(), firstPolygonEdges.end());
            allPolygonEdges.insert(allPolygonEdges.end(), secondPolygonEdges.begin(), secondPolygonEdges.end());
            AZStd::sort(allPolygonEdges.begin(), allPolygonEdges.end());
            allPolygonEdges.erase(AZStd::unique(allPolygonEdges.begin(), allPolygonEdges.end()), allPolygonEdges.end());

            RestoreVertexHandlesForEdges(whiteBox, restoringEdgeHandles);

            // remove all edges that make up the new polygons from the ones currently being restored
            restoringEdgeHandles.erase(
                AZStd::remove_if(
                    restoringEdgeHandles.begin(), restoringEdgeHandles.end(),
                    [&allPolygonEdges](const Api::EdgeHandle edgeHandle)
                    {
                        return AZStd::find(allPolygonEdges.begin(), allPolygonEdges.end(), edgeHandle) !=
                            allPolygonEdges.end();
                    }),
                restoringEdgeHandles.end());

            return AZStd::make_optional<AZStd::array<PolygonHandle, 2>>(
                AZStd::array<PolygonHandle, 2>{firstPolygon, secondPolygon});
        }

        void RestoreVertex(WhiteBoxMesh& whiteBox, const VertexHandle vertexHandle)
        {
            WHITEBOX_LOG("White Box", "RestoreVertex vh(%s)", ToString(vertexHandle).c_str());

            VertexBoolPropertyHandle vertexPropsHiddenHandle;
            whiteBox.mesh.get_property_handle(vertexPropsHiddenHandle, VertexHiddenProp);
            whiteBox.mesh.property(vertexPropsHiddenHandle, om_vh(vertexHandle)) = false;
        }

        bool TryRestoreVertex(WhiteBoxMesh& whiteBox, const VertexHandle vertexHandle)
        {
            WHITEBOX_LOG("White Box", "TryRestoreVertex vh(%s)", ToString(vertexHandle).c_str());

            // if none of the connected edge handles are user edges then the vertex should not be restored
            if (!VertexIsIsolated(whiteBox, vertexHandle))
            {
                RestoreVertex(whiteBox, vertexHandle);
                return true;
            }

            return false;
        }

        PolygonHandle HideEdge(WhiteBoxMesh& whiteBox, const EdgeHandle edgeHandle)
        {
            WHITEBOX_LOG("White Box", "HideEdge eh(%s)", ToString(edgeHandle).c_str());
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            if (MeshHalfedgeCount(whiteBox) == 0)
            {
                return {};
            }

            // get halfedge handles
            const HalfedgeHandle firstHalfedgeHandle = EdgeHalfedgeHandle(whiteBox, edgeHandle, EdgeHalfedge::First);
            const HalfedgeHandle secondHalfedgeHandle = EdgeHalfedgeHandle(whiteBox, edgeHandle, EdgeHalfedge::Second);

            // get face handles from each halfedge
            const FaceHandle firstFaceHandle = HalfedgeFaceHandle(whiteBox, firstHalfedgeHandle);
            const FaceHandle secondFaceHandle = HalfedgeFaceHandle(whiteBox, secondHalfedgeHandle);

            // get polygon handle from each face handle
            const PolygonHandle firstPolygonHandle = FacePolygonHandle(whiteBox, firstFaceHandle);
            const PolygonHandle secondPolygonHandle = FacePolygonHandle(whiteBox, secondFaceHandle);

            // get all vertex handles associated with the first polygon
            const VertexHandles firstPolygonVertexHandles = PolygonVertexHandles(whiteBox, firstPolygonHandle);

            // union of all face handles
            FaceHandles combinedFaceHandles;
            combinedFaceHandles.reserve(
                firstPolygonHandle.m_faceHandles.size() + secondPolygonHandle.m_faceHandles.size());
            combinedFaceHandles.insert(
                combinedFaceHandles.end(), firstPolygonHandle.m_faceHandles.begin(),
                firstPolygonHandle.m_faceHandles.end());
            combinedFaceHandles.insert(
                combinedFaceHandles.end(), secondPolygonHandle.m_faceHandles.begin(),
                secondPolygonHandle.m_faceHandles.end());

            // get polygon property handle from mesh
            PolygonPropertyHandle polygonPropsHandle;
            whiteBox.mesh.get_property_handle(polygonPropsHandle, PolygonProps);
            auto& polygonProps = whiteBox.mesh.property(polygonPropsHandle);

            // update all face handles to refer to the new face handles in the group
            for (const auto& faceHandle : combinedFaceHandles)
            {
                auto polygonIt = polygonProps.find(om_fh(faceHandle));
                polygonIt->second.clear();

                for (const auto& innerFaceHandle : combinedFaceHandles)
                {
                    polygonIt->second.push_back(om_fh(innerFaceHandle));
                }
            }

            // hide any vertices that are not connected to a 'user' edge
            for (const auto& vertexHandle : firstPolygonVertexHandles)
            {
                if (VertexIsIsolated(whiteBox, vertexHandle))
                {
                    HideVertex(whiteBox, vertexHandle);
                }
            }

            return PolygonHandle{combinedFaceHandles};
        }

        VertexHandle SplitFace(WhiteBoxMesh& whiteBox, const FaceHandle faceHandle, const AZ::Vector3& position)
        {
            WHITEBOX_LOG("White Box", "SplitFace fh(%s)", ToString(faceHandle).c_str());
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const auto omFaceHandle = om_fh(faceHandle);
            const auto omVertexHandle = whiteBox.mesh.split_copy(omFaceHandle, position);
            const VertexHandle splitVertexHandle = wb_vh(omVertexHandle);

            // as all new edges will be by default hidden, ensure
            // the newly added vertex is also hidden
            HideVertex(whiteBox, splitVertexHandle);

            // build collection of current face handles for newly inserted vertex
            auto omFaceHandles = AZStd::accumulate(
                whiteBox.mesh.vf_ccwbegin(omVertexHandle), whiteBox.mesh.vf_ccwend(omVertexHandle),
                AZStd::vector<Mesh::FaceHandle>{},
                [](AZStd::vector<Mesh::FaceHandle> faceHandles, const Mesh::FaceHandle faceHandle)
                {
                    faceHandles.push_back(faceHandle);
                    return faceHandles;
                });

            // get polygon property handle from mesh
            PolygonPropertyHandle polygonPropsHandle;
            whiteBox.mesh.get_property_handle(polygonPropsHandle, PolygonProps);
            auto& polygonProps = whiteBox.mesh.property(polygonPropsHandle);

            // get all faces associated with the split face handle and added the newly split faces
            // ensuring we do not have any duplicates
            const auto polygonIt = polygonProps.find(om_fh(faceHandle));
            omFaceHandles.insert(omFaceHandles.end(), polygonIt->second.begin(), polygonIt->second.end());
            AZStd::sort(omFaceHandles.begin(), omFaceHandles.end());
            omFaceHandles.erase(AZStd::unique(omFaceHandles.begin(), omFaceHandles.end()), omFaceHandles.end());

            // update all face handles to point to the new polygon grouping
            for (const auto& omFaceHandle2 : omFaceHandles)
            {
                polygonProps[omFaceHandle2] = omFaceHandles;
            }

            return splitVertexHandle;
        }

        VertexHandle SplitEdge(WhiteBoxMesh& whiteBox, const EdgeHandle edgeHandle, const AZ::Vector3& position)
        {
            WHITEBOX_LOG("White Box", "SplitEdge eh(%s)", ToString(edgeHandle).c_str());
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const HalfedgeHandle halfedgeHandle = EdgeHalfedgeHandle(whiteBox, edgeHandle, EdgeHalfedge::First);
            const VertexHandle tailVertexHandle = HalfedgeVertexHandleAtTail(whiteBox, halfedgeHandle);
            const VertexHandle tipVertexHandle = HalfedgeVertexHandleAtTip(whiteBox, halfedgeHandle);
            const VertexHandle existingConnectedVerts[] = {tailVertexHandle, tipVertexHandle};

            // determine if the edge is a user edge or not before the split
            const bool userEdge = EdgeIsUser(whiteBox, halfedgeHandle, edgeHandle);

            const auto omEdgeHandle = om_eh(edgeHandle);
            const auto omVertexHandle = whiteBox.mesh.add_vertex(position);
            whiteBox.mesh.split_copy(omEdgeHandle, omVertexHandle);

            const VertexHandle splitVertexHandle = wb_vh(omVertexHandle);

            // if the edge that was split was not a 'user' edge we should ensure the
            // newly added vertex is also hidden
            if (!userEdge)
            {
                HideVertex(whiteBox, splitVertexHandle);
            }

            // get all outing edge handles from the new inserted vertex
            const EdgeHandles splitEdgeHandles = VertexEdgeHandles(whiteBox, splitVertexHandle);

            AZStd::for_each(
                splitEdgeHandles.cbegin(), splitEdgeHandles.cend(),
                [&whiteBox, &existingConnectedVerts](const EdgeHandle edgeHandle)
                {
                    const auto vertexHandles = EdgeVertexHandles(whiteBox, edgeHandle);
                    const bool alreadyConnectedVertex = AZStd::any_of(
                        AZStd::cbegin(existingConnectedVerts), AZStd::cend(existingConnectedVerts),
                        [&vertexHandles](const VertexHandle vertexHandle)
                        {
                            return AZStd::find(
                                       AZStd::cbegin(vertexHandles), AZStd::cend(vertexHandles), vertexHandle) !=
                                AZStd::cend(vertexHandles);
                        });

                    // find if the edge was added or is part of the existing edge which was split
                    if (!alreadyConnectedVertex)
                    {
                        const FaceHandles faceHandles = EdgeFaceHandles(whiteBox, edgeHandle);
                        const PolygonHandle polygonHandle = FacePolygonHandle(whiteBox, faceHandles[0]);

                        // if the edge was not already connected to on of the existing verts,
                        // find the associated polygon handle update them with the newly split faces
                        const PolygonHandle existingPolygonHandle = polygonHandle.m_faceHandles.empty()
                            ? FacePolygonHandle(whiteBox, faceHandles[1])
                            : polygonHandle;

                        const FaceHandle newFaceHandle =
                            polygonHandle.m_faceHandles.empty() ? faceHandles[0] : faceHandles[1];

                        PolygonPropertyHandle polygonPropsHandle;
                        whiteBox.mesh.get_property_handle(polygonPropsHandle, PolygonProps);
                        auto& polygonProps = whiteBox.mesh.property(polygonPropsHandle);

                        auto omExistingPolygonHandle = InternalFaceHandlesFromPolygon(existingPolygonHandle);
                        omExistingPolygonHandle.push_back(om_fh(newFaceHandle));

                        // update all face handles to point to the new polygon grouping
                        for (const Mesh::FaceHandle& faceHandle : omExistingPolygonHandle)
                        {
                            polygonProps[faceHandle] = omExistingPolygonHandle;
                        }
                    }
                });

            return splitVertexHandle;
        }

        bool FlipEdge(WhiteBoxMesh& whiteBox, const EdgeHandle edgeHandle)
        {
            WHITEBOX_LOG("White Box", "FlipEdge eh(%s)", ToString(edgeHandle).c_str());

            const auto omEdgeHandle = om_eh(edgeHandle);

            // check if edge can be flipped
            const bool canFlip = whiteBox.mesh.is_flip_ok(omEdgeHandle) && EdgeIsHidden(whiteBox, edgeHandle);

            if (canFlip)
            {
                whiteBox.mesh.flip(omEdgeHandle);
            }

            return canFlip;
        }

        void HideVertex(WhiteBoxMesh& whiteBox, const VertexHandle vertexHandle)
        {
            WHITEBOX_LOG("White Box", "HideVertex vh(%s)", ToString(vertexHandle).c_str());

            VertexBoolPropertyHandle vertexPropsHiddenHandle;
            whiteBox.mesh.get_property_handle(vertexPropsHiddenHandle, VertexHiddenProp);
            whiteBox.mesh.property(vertexPropsHiddenHandle, om_vh(vertexHandle)) = true;
        }

        void Clear(WhiteBoxMesh& whiteBox)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            PolygonPropertyHandle polygonPropsHandle;
            whiteBox.mesh.get_property_handle(polygonPropsHandle, PolygonProps);
            whiteBox.mesh.remove_property(polygonPropsHandle);

            VertexBoolPropertyHandle vertexPropsHiddenHandle;
            whiteBox.mesh.get_property_handle(vertexPropsHiddenHandle, VertexHiddenProp);
            whiteBox.mesh.remove_property(vertexPropsHiddenHandle);

            whiteBox.mesh.clear();

            InitializeWhiteBoxMesh(whiteBox);
        }

        PolygonHandle AddTriPolygon(
            WhiteBoxMesh& whiteBox, const VertexHandle vh0, const VertexHandle vh1, const VertexHandle vh2)
        {
            WHITEBOX_LOG(
                "White Box", "AddTriPolygon vh(%s), vh(%s), vh(%s)", ToString(vh0).c_str(), ToString(vh1).c_str(),
                ToString(vh2).c_str());
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            return AddPolygon(whiteBox, AZStd::vector<FaceVertHandles>{{vh0, vh1, vh2}});
        }

        PolygonHandle AddQuadPolygon(
            WhiteBoxMesh& whiteBox, const VertexHandle vh0, const VertexHandle vh1, const VertexHandle vh2,
            const VertexHandle vh3)
        {
            WHITEBOX_LOG(
                "White Box", "AddQuadPolygon vh(%s), vh(%s), vh(%s), vh(%s)", ToString(vh0).c_str(),
                ToString(vh1).c_str(), ToString(vh2).c_str(), ToString(vh3).c_str());
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            return AddPolygon(whiteBox, AZStd::vector<FaceVertHandles>{{vh0, vh1, vh2}, {vh0, vh2, vh3}});
        }

        PolygonHandle AddPolygon(WhiteBoxMesh& whiteBox, const FaceVertHandlesList& faceVertHandles)
        {
            WHITEBOX_LOG("White Box", "AddPolygon [%s]", ToString(faceVertHandles).c_str());
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            PolygonPropertyHandle polygonPropsHandle;
            whiteBox.mesh.get_property_handle(polygonPropsHandle, PolygonProps);

            auto polygon = FaceHandlesInternal{};
            polygon.reserve(faceVertHandles.size());

            for (const auto& face : faceVertHandles)
            {
                polygon.push_back(whiteBox.mesh.add_face(
                    om_vh(face.m_vertexHandles[0]), om_vh(face.m_vertexHandles[1]), om_vh(face.m_vertexHandles[2])));
            }

            PolygonHandle polygonHandle = PolygonHandleFromInternal(polygon);

            auto& polygonProps = whiteBox.mesh.property(polygonPropsHandle);
            // multiple face handles map to a polygon handle
            for (const auto& faceHandle : polygon)
            {
                polygonProps[faceHandle] = polygon;
            }

            return polygonHandle;
        }

        PolygonHandles InitializeAsUnitCube(WhiteBoxMesh& whiteBox)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            // generate vertices
            VertexHandle vertexHandles[8];

            // top verts
            vertexHandles[0] = AddVertex(whiteBox, AZ::Vector3(-0.5f, -0.5f, 0.5f));
            vertexHandles[1] = AddVertex(whiteBox, AZ::Vector3(0.5f, -0.5f, 0.5f));
            vertexHandles[2] = AddVertex(whiteBox, AZ::Vector3(0.5f, 0.5f, 0.5f));
            vertexHandles[3] = AddVertex(whiteBox, AZ::Vector3(-0.5f, 0.5f, 0.5f));

            // bottom verts
            vertexHandles[4] = AddVertex(whiteBox, AZ::Vector3(-0.5f, -0.5f, -0.5f));
            vertexHandles[5] = AddVertex(whiteBox, AZ::Vector3(0.5f, -0.5f, -0.5f));
            vertexHandles[6] = AddVertex(whiteBox, AZ::Vector3(0.5f, 0.5f, -0.5f));
            vertexHandles[7] = AddVertex(whiteBox, AZ::Vector3(-0.5f, 0.5f, -0.5f));

            // generate faces
            PolygonHandles polygonHandles{
                // top
                AddQuadPolygon(whiteBox, vertexHandles[0], vertexHandles[1], vertexHandles[2], vertexHandles[3]),
                // bottom
                AddQuadPolygon(whiteBox, vertexHandles[7], vertexHandles[6], vertexHandles[5], vertexHandles[4]),
                // front
                AddQuadPolygon(whiteBox, vertexHandles[4], vertexHandles[5], vertexHandles[1], vertexHandles[0]),
                // right
                AddQuadPolygon(whiteBox, vertexHandles[5], vertexHandles[6], vertexHandles[2], vertexHandles[1]),
                // back
                AddQuadPolygon(whiteBox, vertexHandles[6], vertexHandles[7], vertexHandles[3], vertexHandles[2]),
                // left
                AddQuadPolygon(whiteBox, vertexHandles[7], vertexHandles[4], vertexHandles[0], vertexHandles[3])};

            CalculateNormals(whiteBox);
            CalculatePlanarUVs(whiteBox);

            return polygonHandles;
        }

        PolygonHandle InitializeAsUnitQuad(WhiteBoxMesh& whiteBox)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            // generate vertices
            VertexHandle vertexHandles[4];

            // front face
            vertexHandles[0] = AddVertex(whiteBox, AZ::Vector3(-0.5f, 0.0f, -0.5f)); // bottom left
            vertexHandles[1] = AddVertex(whiteBox, AZ::Vector3(0.5f, 0.0f, -0.5f)); // bottom right
            vertexHandles[2] = AddVertex(whiteBox, AZ::Vector3(0.5f, 0.0f, 0.5f)); // top right
            vertexHandles[3] = AddVertex(whiteBox, AZ::Vector3(-0.5f, 0.0f, 0.5f)); // top left

            // generate faces - front
            auto polygonHandle =
                AddQuadPolygon(whiteBox, vertexHandles[0], vertexHandles[1], vertexHandles[2], vertexHandles[3]);

            CalculateNormals(whiteBox);
            CalculatePlanarUVs(whiteBox);

            return polygonHandle;
        }

        PolygonHandle InitializeAsUnitTriangle(WhiteBoxMesh& whiteBox)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            // generate vertices
            VertexHandle vertexHandles[3];

            const auto pointOnCircle = [](const float angle)
            {
                return AZ::Vector3{cosf(angle), sinf(angle), 0.0f};
            };

            // front face
            vertexHandles[0] = AddVertex(whiteBox, pointOnCircle(AZ::DegToRad(90.0f))); // top
            vertexHandles[1] = AddVertex(whiteBox, pointOnCircle(AZ::DegToRad(-150.0f))); // bottom left
            vertexHandles[2] = AddVertex(whiteBox, pointOnCircle(AZ::DegToRad(-30.0f))); // bottom right

            // generate faces - front
            auto polygonHandle = AddTriPolygon(whiteBox, vertexHandles[0], vertexHandles[1], vertexHandles[2]);

            CalculateNormals(whiteBox);
            CalculatePlanarUVs(whiteBox);

            return polygonHandle;
        }

        void SetVertexPosition(WhiteBoxMesh& whiteBox, const VertexHandle vertexHandle, const AZ::Vector3& position)
        {
            WHITEBOX_LOG(
                "White Box", "SetVertexPosition vh(%s) %s", ToString(vertexHandle).c_str(),
                AZ::ToString(position).c_str());
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            whiteBox.mesh.set_point(om_vh(vertexHandle), position);
        }

        void SetVertexPositionAndUpdateUVs(
            WhiteBoxMesh& whiteBox, const VertexHandle vertexHandle, const AZ::Vector3& position)
        {
            WHITEBOX_LOG(
                "White Box", "SetVertexPositionAndUpdateUVs vh(%s) %s", ToString(vertexHandle).c_str(),
                AZ::ToString(position).c_str());
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            SetVertexPosition(whiteBox, vertexHandle, position);
            CalculatePlanarUVs(whiteBox);
        }

        VertexHandle AddVertex(WhiteBoxMesh& whiteBox, const AZ::Vector3& vertex)
        {
            WHITEBOX_LOG("White Box", "AddVertex %s", AZ::ToString(vertex).c_str());
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            return wb_vh(whiteBox.mesh.add_vertex(vertex));
        }

        FaceHandle AddFace(WhiteBoxMesh& whiteBox, const VertexHandle v0, const VertexHandle v1, const VertexHandle v2)
        {
            WHITEBOX_LOG(
                "White Box", "AddFace vh(%s), vh(%s), vh(%s)", ToString(v0).c_str(), ToString(v1).c_str(),
                ToString(v2).c_str());
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            return wb_fh(whiteBox.mesh.add_face(om_vh(v0), om_vh(v1), om_vh(v2)));
        }

        void CalculateNormals(WhiteBoxMesh& whiteBox)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            whiteBox.mesh.update_normals();
        }

        void ZeroUVs(WhiteBoxMesh& whiteBox)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            for (const Mesh::FaceHandle& faceHandle : whiteBox.mesh.faces())
            {
                for (Mesh::FaceHalfedgeCCWIter faceHalfedgeIt = whiteBox.mesh.fh_ccwiter(faceHandle);
                     faceHalfedgeIt.is_valid(); ++faceHalfedgeIt)
                {
                    whiteBox.mesh.set_texcoord2D(*faceHalfedgeIt, AZ::Vector2::CreateZero());
                }
            }
        }

        AZ::Vector3 EdgeMidpoint(const WhiteBoxMesh& whiteBox, const EdgeHandle edgeHandle)
        {
            const auto vertexPositions = EdgeVertexPositions(whiteBox, edgeHandle);
            return (vertexPositions[0] + vertexPositions[1]) * 0.5f;
        }

        AZ::Vector3 FaceMidpoint(const WhiteBoxMesh& whiteBox, const FaceHandle faceHandle)
        {
            const auto vertexPositions = FaceVertexPositions(whiteBox, faceHandle);
            return AZStd::accumulate(
                       AZStd::begin(vertexPositions), AZStd::end(vertexPositions), AZ::Vector3::CreateZero(),
                       [](const AZ::Vector3& acc, const AZ::Vector3& position)
                       {
                           return acc + position;
                       }) /
                3;
        }

        AZ::Vector3 PolygonMidpoint(const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle)
        {
            // first attempt using border vertex handles (this is usually what we want, but it might
            // fail if all edges of a polygon have been hidden)
            if (const auto polygonBorderVertexHandles = PolygonBorderVertexHandlesFlattened(whiteBox, polygonHandle);
                !polygonBorderVertexHandles.empty())
            {
                return VerticesMidpoint(whiteBox, polygonBorderVertexHandles);
            }
            // if that fails, fall back to all vertex handles in the polygon to calculate the midpoint

            return VerticesMidpoint(whiteBox, PolygonVertexHandles(whiteBox, polygonHandle));
        }

        AZ::Vector3 VerticesMidpoint(const WhiteBoxMesh& whiteBox, const VertexHandles& vertexHandles)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            AzToolsFramework::MidpointCalculator midpointCalculator;
            for (const auto& vertexHandle : vertexHandles)
            {
                midpointCalculator.AddPosition(VertexPosition(whiteBox, vertexHandle));
            }

            return midpointCalculator.CalculateMidpoint();
        }

        static HalfedgeHandle FindHalfedgeInAdjacentPolygon(
            const WhiteBoxMesh& whiteBox, const Internal::VertexHandlePair vertexHandlePair,
            const PolygonHandle& selectedPolygonHandle, const PolygonHandle& adjacentPolygonHandle)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const auto selectedPolygonEdgeHandles = PolygonBorderEdgeHandlesFlattened(whiteBox, selectedPolygonHandle);
            const auto adjacentPolygonEdgeHandles = PolygonBorderEdgeHandlesFlattened(whiteBox, adjacentPolygonHandle);

            // iterate over all halfedges in the adjacent polygon
            for (const auto& edgeHandle : adjacentPolygonEdgeHandles)
            {
                const auto* const foundEdgeHandleInSelectedPolygon =
                    AZStd::find(selectedPolygonEdgeHandles.cbegin(), selectedPolygonEdgeHandles.cend(), edgeHandle);

                // did not find edge handle in selected polygon
                if (foundEdgeHandleInSelectedPolygon == selectedPolygonEdgeHandles.cend())
                {
                    // find outgoing edge handles
                    for (const auto& halfedgeHandle :
                         VertexOutgoingHalfedgeHandles(whiteBox, vertexHandlePair.m_existing))
                    {
                        // attempt to find one of the outgoing halfedge handles in the adjacent polygon
                        if (HalfedgeEdgeHandle(whiteBox, halfedgeHandle) == edgeHandle)
                        {
                            return halfedgeHandle;
                        }
                    }
                }
            }

            return HalfedgeHandle{};
        }

        // add 'linking/connecting' faces for when an 'impression' happens
        // note: temporary measure before triangulation support is added to the white box tool
        static void AddLinkingFace(
            const WhiteBoxMesh& whiteBox, const Internal::VertexHandlePair vertexHandlePair,
            const PolygonHandle& selectedPolygonHandle, const PolygonHandle& adjacentPolygonHandle,
            FaceVertHandlesCollection& vertsForLinkingAdjacentPolygons)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            // if we found a valid halfedge
            if (const HalfedgeHandle foundHalfedgeHandle = FindHalfedgeInAdjacentPolygon(
                    whiteBox, vertexHandlePair, selectedPolygonHandle, adjacentPolygonHandle);
                foundHalfedgeHandle.IsValid())
            {
                // find the 'to' vertex
                const auto toVertexHandle = HalfedgeVertexHandleAtTip(whiteBox, foundHalfedgeHandle);

                // find if the face handle of the halfedge is 'in' the adjacent polygon
                const auto* const faceHandleInAdjacentPolygon = AZStd::find(
                    adjacentPolygonHandle.m_faceHandles.cbegin(), adjacentPolygonHandle.m_faceHandles.cend(),
                    HalfedgeFaceHandle(whiteBox, foundHalfedgeHandle));

                // adjust winding order if the outgoing halfedge is in the adjacent polygon or not
                FaceVertHandles linkingPolygonVertexHandles =
                    faceHandleInAdjacentPolygon != adjacentPolygonHandle.m_faceHandles.cend()
                    ? FaceVertHandles{vertexHandlePair.m_existing, toVertexHandle, vertexHandlePair.m_added}
                    : FaceVertHandles{vertexHandlePair.m_existing, vertexHandlePair.m_added, toVertexHandle};

                // store verts for new polygon
                vertsForLinkingAdjacentPolygons.push_back(
                    AZStd::vector<FaceVertHandles>{AZStd::move(linkingPolygonVertexHandles)});
            }
        }

        // return true if existing verts were reused and linking faces were added
        // return false if a new adjacent polygon must be created (new verts have
        // been added and will be used)
        static bool TryAddLinkingFaces(
            WhiteBoxMesh& whiteBox, const EdgeHandle edgeHandle, const Internal::AppendedVerts& appendedVerts,
            const PolygonHandle& selectedPolygonHandle, const Internal::VertexHandlePair& currentVertexHandlePair,
            const Internal::VertexHandlePair& nextVertexHandlePair,
            AZStd::vector<PolygonHandle>& polygonHandlesToRemove,
            FaceVertHandlesCollection& vertsForExistingAdjacentPolygons,
            FaceVertHandlesCollection& vertsForLinkingAdjacentPolygons)
        {
            // find all faces connected to this edge
            for (const auto& faceHandle : EdgeFaceHandles(whiteBox, edgeHandle))
            {
                // find a face that is _not_ part of the polygon being appended/selected
                if (AZStd::find(
                        selectedPolygonHandle.m_faceHandles.cbegin(), selectedPolygonHandle.m_faceHandles.cend(),
                        faceHandle) == selectedPolygonHandle.m_faceHandles.cend())
                {
                    // the polygon handle of the face connected to one of the top edges
                    const auto adjacentPolygonHandle = FacePolygonHandle(whiteBox, faceHandle);
                    const auto selectedPolygonNormal = PolygonNormal(whiteBox, selectedPolygonHandle);
                    // the normal of the adjacent (connected) polygon
                    const auto adjacentPolygonNormal = PolygonNormal(whiteBox, adjacentPolygonHandle);
                    const float angleCosine = adjacentPolygonNormal.Dot(selectedPolygonNormal);
                    // check if the adjacent polygon is completely orthogonal to the
                    // selected polygon - if so reuse the existing verts and do not
                    // create a new adjacent polygon as part of the operation
                    if (AZ::IsClose(angleCosine, 0.0f, AdjacentPolygonNormalTolerance))
                    {
                        // if the current or next vertex on the border have had a new vertex added
                        if (currentVertexHandlePair.m_added != currentVertexHandlePair.m_existing ||
                            nextVertexHandlePair.m_added != nextVertexHandlePair.m_existing)
                        {
                            // remove the existing adjacent polygon (a new one will be added
                            // that is connected to the newly added verts)
                            polygonHandlesToRemove.push_back(adjacentPolygonHandle);

                            // calculate new verts for faces to be created
                            const auto adjacentPolygonVerts =
                                BuildNewVertexFaceHandles(whiteBox, appendedVerts, adjacentPolygonHandle.m_faceHandles);

                            // store the face verts to be added later after existing faces have been removed
                            vertsForExistingAdjacentPolygons.push_back(adjacentPolygonVerts);
                        }

                        // first linking face
                        if (currentVertexHandlePair.m_added != currentVertexHandlePair.m_existing)
                        {
                            AddLinkingFace(
                                whiteBox, currentVertexHandlePair, selectedPolygonHandle, adjacentPolygonHandle,
                                vertsForLinkingAdjacentPolygons);
                        }

                        // second linking face
                        if (nextVertexHandlePair.m_added != nextVertexHandlePair.m_existing)
                        {
                            AddLinkingFace(
                                whiteBox, nextVertexHandlePair, selectedPolygonHandle, adjacentPolygonHandle,
                                vertsForLinkingAdjacentPolygons);
                        }

                        return true;
                    }
                }
            }

            return false;
        }

        // build the adjacent walls of an extrusion
        // note: borderVertexHandles must be ordered correctly (CCW)
        static void AddAdjacentFaces(
            WhiteBoxMesh& whiteBox, const Internal::AppendedVerts& appendedVerts, const bool appendAll,
            const PolygonHandle& selectedPolygonHandle, const VertexHandles& borderVertexHandles,
            const EdgeHandles& borderEdgeHandles, AZStd::vector<PolygonHandle>& polygonHandlesToRemove,
            FaceVertHandlesCollection& vertsForNewAdjacentPolygons,
            FaceVertHandlesCollection& vertsForExistingAdjacentPolygons,
            FaceVertHandlesCollection& vertsForLinkingAdjacentPolygons)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            // adjacent faces
            for (size_t index = 0; index < borderVertexHandles.size(); ++index)
            {
                const size_t nextIndexWrapped = (index + 1) % borderVertexHandles.size();

                const VertexHandle existingBorderVertexHandle = borderVertexHandles[index];
                const VertexHandle nextExistingBorderVertexHandle = borderVertexHandles[nextIndexWrapped];

                const auto* const currentVertexHandlePairIt = AZStd::find_if(
                    appendedVerts.m_vertexHandlePairs.cbegin(), appendedVerts.m_vertexHandlePairs.cend(),
                    [existingBorderVertexHandle](const Internal::VertexHandlePair vertexHandlePair)
                    {
                        return vertexHandlePair.m_existing == existingBorderVertexHandle;
                    });

                const auto* const nextVertexHandlePairIt = AZStd::find_if(
                    appendedVerts.m_vertexHandlePairs.cbegin(), appendedVerts.m_vertexHandlePairs.cend(),
                    [nextExistingBorderVertexHandle](const Internal::VertexHandlePair vertexHandlePair)
                    {
                        return vertexHandlePair.m_existing == nextExistingBorderVertexHandle;
                    });

                // find the edge on the border of the polygon we're appending
                const auto* const edgeHandleIt = AZStd::find_if(
                    borderEdgeHandles.cbegin(), borderEdgeHandles.cend(),
                    [&whiteBox, existingBorderVertexHandle, nextExistingBorderVertexHandle](const EdgeHandle edgeHandle)
                    {
                        const auto edgeVertexHandles = EdgeVertexHandles(whiteBox, edgeHandle);
                        return (existingBorderVertexHandle == edgeVertexHandles[0] &&
                                nextExistingBorderVertexHandle == edgeVertexHandles[1]) ||
                            (existingBorderVertexHandle == edgeVertexHandles[1] &&
                             nextExistingBorderVertexHandle == edgeVertexHandles[0]);
                    });

                // does a new side need to be created (new verts added) or should we reuse existing verts
                const bool createNewAdjacentPolygon = appendAll
                    // short circuit if appendAll is true (no linking faces are required)
                    || !TryAddLinkingFaces(whiteBox, *edgeHandleIt, appendedVerts, selectedPolygonHandle,
                                           *currentVertexHandlePairIt, *nextVertexHandlePairIt, polygonHandlesToRemove,
                                           vertsForExistingAdjacentPolygons, vertsForLinkingAdjacentPolygons);

                // a new full side must be created (we're not reusing existing verts for the new polygon)
                if (createNewAdjacentPolygon)
                {
                    vertsForNewAdjacentPolygons.push_back(AZStd::vector<FaceVertHandles>{
                        {existingBorderVertexHandle, nextExistingBorderVertexHandle, nextVertexHandlePairIt->m_added},
                        {existingBorderVertexHandle, nextVertexHandlePairIt->m_added,
                         currentVertexHandlePairIt->m_added}});
                }
            }
        }

        // note: it is important to collect all face handles to remove and call RemoveFaces
        // once for a given operation (for example to do not call RemoveFaces in a loop,
        // instead build the collection of face handles in a loop and then call RemoveFaces)
        // this is to ensure the face handles remain stable as they may be invalidated/changed
        // during garbage_collect
        void RemoveFaces(WhiteBoxMesh& whiteBox, const FaceHandles& faceHandles)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            whiteBox.mesh.request_face_status();
            whiteBox.mesh.request_edge_status();
            whiteBox.mesh.request_vertex_status();

            PolygonPropertyHandle polygonPropsHandle;
            whiteBox.mesh.get_property_handle(polygonPropsHandle, PolygonProps);
            auto& polygonProps = whiteBox.mesh.property(polygonPropsHandle);

            // erase face handles from the polygon map and
            // delete the faces from OpenMesh
            for (const auto& faceHandle : faceHandles)
            {
                polygonProps.erase(om_fh(faceHandle));
                whiteBox.mesh.delete_face(om_fh(faceHandle), false);
            }

            using VertexHandlePtrs = AZStd::vector<Mesh::VertexHandle*>;
            using FaceHandlePtrs = AZStd::vector<Mesh::FaceHandle*>;
            using HalfedgeHandlePtrs = AZStd::vector<Mesh::HalfedgeHandle*>;

            // store pointers to all face handles stored within the map (the values - e.g. it.second)
            FaceHandlePtrs faceHandlePtrs;
            for (auto& polygonProp : polygonProps)
            {
                faceHandlePtrs = AZStd::accumulate(
                    polygonProp.second.begin(), polygonProp.second.end(), faceHandlePtrs,
                    [](FaceHandlePtrs ptrs, Mesh::FaceHandle& faceHandle)
                    {
                        ptrs.push_back(&faceHandle);
                        return ptrs;
                    });
            }

            // make a copy of the face handles to compare against after the garbage_collect
            FaceHandlesInternal faceHandlesCopy;
            faceHandlesCopy.reserve(faceHandlePtrs.size());
            AZStd::transform(
                faceHandlePtrs.begin(), faceHandlePtrs.end(), AZStd::back_inserter(faceHandlesCopy),
                [](const Mesh::FaceHandle* faceHandle)
                {
                    return *faceHandle;
                });

            // actually delete faces from mesh
            auto vhs = VertexHandlePtrs{};
            auto hehs = HalfedgeHandlePtrs{};
            whiteBox.mesh.garbage_collection(vhs, hehs, faceHandlePtrs);

            using ModifiedFaceHandle = AZStd::pair<Mesh::FaceHandle, Mesh::FaceHandle>;
            using ModifiedFaceHandles = AZStd::vector<ModifiedFaceHandle>;

            const ModifiedFaceHandles modifiedFaceHandles = AZStd::inner_product(
                faceHandlesCopy.begin(), faceHandlesCopy.end(), faceHandlePtrs.begin(), ModifiedFaceHandles{},
                //reduce 
                [](ModifiedFaceHandles modifiedFaceHandles, const ModifiedFaceHandle& fh)
                {
                    if (fh.first.is_valid())
                    {
                        modifiedFaceHandles.push_back(fh);
                    }

                    return modifiedFaceHandles;
                },
                //transform
                [](const Mesh::FaceHandle lhs, const Mesh::FaceHandle* rhs)
                {
                    // if any of the faceHandlePtrs differ, we know the handles
                    // have been invalidated during the garbage collect
                    if (lhs != *rhs)
                    {
                        return ModifiedFaceHandle{lhs, *rhs};
                    }

                    return ModifiedFaceHandle{};
                });

            // iterate over all modified face handles
            for (const ModifiedFaceHandle& modifiedFaceHandle : modifiedFaceHandles)
            {
                // find the value in the map using the old key
                // e.g. faceHandle 10 -> polygon was 10, 11 -> now 4, 5
                auto found = polygonProps.find(modifiedFaceHandle.first);
                if (found != polygonProps.end())
                {
                    // copy the updated faceHandle value (e.g was 10, now 4)
                    auto faceHandle = modifiedFaceHandle.second;
                    // pull the values out of the map (e.g. 4, 5) - make a copy
                    auto val = found->second;
                    // erase the old key/value -> key 10, value 4, 5
                    polygonProps.erase(found);
                    // reinsert the values back into the map with the right key (key 4, value 4, 5)
                    polygonProps.insert({faceHandle, val});
                }
            }
        }

        AZStd::vector<FaceVertHandles> BuildNewVertexFaceHandles(
            WhiteBoxMesh& whiteBox, const Internal::AppendedVerts& appendedVerts, const FaceHandles& existingFaces)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            AZStd::vector<FaceVertHandles> faces;
            faces.reserve(existingFaces.size());

            // for each face
            for (const FaceHandle& faceHandle : existingFaces)
            {
                VertexHandles vertexHandlesForFace;
                vertexHandlesForFace.reserve(3);

                const auto vertexHandles = FaceVertexHandles(whiteBox, faceHandle);
                // for each vertex handle
                for (const VertexHandle& vertexHandle : vertexHandles)
                {
                    // find vertex handle in vertices list
                    const auto* const vertexHandlePairIt = AZStd::find_if(
                        appendedVerts.m_vertexHandlePairs.cbegin(), appendedVerts.m_vertexHandlePairs.cend(),
                        [vertexHandle](const Internal::VertexHandlePair vertexHandlePair)
                        {
                            return vertexHandle == vertexHandlePair.m_existing;
                        });

                    // record corresponding (added) vertex
                    if (vertexHandlePairIt != appendedVerts.m_vertexHandlePairs.cend())
                    {
                        // store vertex
                        vertexHandlesForFace.push_back(vertexHandlePairIt->m_added);
                    }
                    // or existing vertex if one was not added
                    else
                    {
                        vertexHandlesForFace.push_back(vertexHandle);
                    }
                }

                // add face using vertex stored vertex
                FaceVertHandles face;
                face.m_vertexHandles[0] = vertexHandlesForFace[0];
                face.m_vertexHandles[1] = vertexHandlesForFace[1];
                face.m_vertexHandles[2] = vertexHandlesForFace[2];
                faces.push_back(face);
            }

            return faces;
        }

        // determine whether new or existing verts be returned based on the type of
        // append (extrude -> out, impression -> in)
        template<typename AppendVertFn>
        static AZStd::tuple<Internal::AppendedVerts, bool> AddVertsForAppend(
            WhiteBoxMesh& whiteBox, const VertexHandles& existingVertexHandles, const PolygonHandle& polygonHandle,
            AppendVertFn&& appendFn)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const AZ::Vector3 polygonNormal = PolygonNormal(whiteBox, polygonHandle);
            const auto polygonHalfedgeHandles = PolygonHalfedgeHandles(whiteBox, polygonHandle);

            const AZ::Vector3 extrudeDirection = appendFn(AZ::Vector3::CreateZero());
            const float angleCosine = extrudeDirection.Dot(polygonNormal);
            // detect if the user is extruding the polygon (pulling out) - if so we
            // always want to append new vertices for every existing vertex
            const bool appendAll = angleCosine >= 0.0f;

            Internal::AppendedVerts appendedVerts;
            appendedVerts.m_vertexHandlePairs.reserve(existingVertexHandles.size());

            for (const VertexHandle& existingVertexHandle : existingVertexHandles)
            {
                bool vertexHandleAdded = false;
                // visit all connected halfedge handles
                for (const auto& halfedgeHandle : VertexHalfedgeHandles(whiteBox, existingVertexHandle))
                {
                    const auto edgeHandle = HalfedgeEdgeHandle(whiteBox, halfedgeHandle);
                    const bool boundaryEdge = EdgeIsBoundary(whiteBox, edgeHandle);

                    // is the edge not contained in selected polygon (we want to only check adjacent polygons)
                    // or is the edge on a boundary (no adjacent face)
                    if (boundaryEdge ||
                        AZStd::find(polygonHalfedgeHandles.cbegin(), polygonHalfedgeHandles.cend(), halfedgeHandle) ==
                            polygonHalfedgeHandles.cend())
                    {
                        const auto nextHalfedgeHandle = HalfedgeHandleNext(whiteBox, halfedgeHandle);
                        const auto nextEdgeHandle = HalfedgeEdgeHandle(whiteBox, nextHalfedgeHandle);

                        const AZ::Vector3 edgeAxis = EdgeAxis(whiteBox, edgeHandle);
                        const AZ::Vector3 nextEdgeAxis = EdgeAxis(whiteBox, nextEdgeHandle);

                        // calculate face normal from two edges
                        const AZ::Vector3 faceNormal = edgeAxis.Cross(nextEdgeAxis).GetNormalizedSafe();
                        const bool adjacentFaceAndPolygonNormalOrthogonal = AZ::IsClose(
                            AZ::GetAbs(faceNormal.Dot(polygonNormal)), 0.0f, AdjacentPolygonNormalTolerance);

                        // if the polygon normal and edge direction are not parallel, we should
                        // add a new vertex for the polygon to be later created
                        if (appendAll || boundaryEdge || !adjacentFaceAndPolygonNormalOrthogonal)
                        {
                            vertexHandleAdded = true;

                            const AZ::Vector3 localPoint = VertexPosition(whiteBox, existingVertexHandle);
                            const AZ::Vector3 extrudedPoint = appendFn(localPoint);
                            const VertexHandle addedVertex = AddVertex(whiteBox, extrudedPoint);

                            // vertex pairs differ
                            appendedVerts.m_vertexHandlePairs.emplace_back(existingVertexHandle, addedVertex);

                            break;
                        }
                    }
                }

                if (!vertexHandleAdded)
                {
                    const AZ::Vector3 localPoint = VertexPosition(whiteBox, existingVertexHandle);
                    const AZ::Vector3 extrudedPoint = appendFn(localPoint);
                    SetVertexPosition(whiteBox, existingVertexHandle, extrudedPoint);

                    // vertex pairs match
                    appendedVerts.m_vertexHandlePairs.emplace_back(existingVertexHandle, existingVertexHandle);
                }
            }

            return {appendedVerts, appendAll};
        }

        //! @AppendVertexFn The way vertices should be translated as an append happens
        //! note: This is a customization point for scale and translation types of append
        template<typename AppendVertexFn>
        static AppendedPolygonHandles Extrude(
            WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle, AppendVertexFn&& appendFn)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            // find border vertex handles for polygon
            const auto polygonBorderVertexHandlesCollection = PolygonBorderVertexHandles(whiteBox, polygonHandle);

            // handle potentially pathological case where all edges have
            // been hidden and no border vertex loop can be found
            if (polygonBorderVertexHandlesCollection.empty())
            {
                AppendedPolygonHandles appendedPolygonHandles;
                appendedPolygonHandles.m_appendedPolygonHandle = polygonHandle;

                return appendedPolygonHandles;
            }

            // find all vertex handles for polygon
            const auto polygonVertexHandles = PolygonVertexHandles(whiteBox, polygonHandle);
            const auto borderEdgeHandlesCollection = PolygonBorderEdgeHandles(whiteBox, polygonHandle);

            // the vertices to use for the new append (vertex handle pairs may both be existing, or new and existing)
            const auto [appendedVerts, appendAll] =
                AddVertsForAppend(whiteBox, polygonVertexHandles, polygonHandle, appendFn);

            // the face vertex combinations to use for the new polygon begin appended
            const AZStd::vector<FaceVertHandles> topFacesToAdd =
                BuildNewVertexFaceHandles(whiteBox, appendedVerts, polygonHandle.m_faceHandles);

            // polygons that will be removed as part of this operation
            AZStd::vector<PolygonHandle> polygonHandlesToRemove;
            // all new faces to be added
            FaceVertHandlesCollection vertsForNewAdjacentPolygons;
            FaceVertHandlesCollection vertsForExistingAdjacentPolygons;
            FaceVertHandlesCollection vertsForLinkingAdjacentPolygons;

            for (size_t index = 0; index < polygonBorderVertexHandlesCollection.size(); ++index)
            {
                // note: quads/walls of extrusion
                AddAdjacentFaces(
                    whiteBox, appendedVerts, appendAll, polygonHandle, polygonBorderVertexHandlesCollection[index],
                    borderEdgeHandlesCollection[index], polygonHandlesToRemove, vertsForNewAdjacentPolygons,
                    vertsForExistingAdjacentPolygons, vertsForLinkingAdjacentPolygons);
            }

            // <missing> - add bottom faces if mesh was 2d previously (reverse winding order)

            FaceHandles allFacesToRemove = polygonHandle.m_faceHandles;
            for (const auto& polygonHandleToRemove : polygonHandlesToRemove)
            {
                allFacesToRemove.insert(
                    allFacesToRemove.end(), polygonHandleToRemove.m_faceHandles.cbegin(), polygonHandleToRemove.m_faceHandles.cend());
            }

            // remove all faces that were already there
            // note: it is very important to not use the existing polygon handle after remove
            // has been called as this will invalidate all existing face handles
            RemoveFaces(whiteBox, allFacesToRemove);

            PolygonHandles polygonHandlesToRestore;
            // re-add existing adjacent polygons
            for (const auto& verts : vertsForExistingAdjacentPolygons)
            {
                polygonHandlesToRestore.push_back(AddPolygon(whiteBox, verts));
            }

            AZ_Assert(
                polygonHandlesToRestore.size() == polygonHandlesToRemove.size(),
                "PolygonHandles to restore and PolygonHandles to remove have different sizes");

            AppendedPolygonHandles appendedPolygonHandles;
            appendedPolygonHandles.m_restoredPolygonHandles.reserve(polygonHandlesToRestore.size());
            for (size_t index = 0; index < polygonHandlesToRestore.size(); ++index)
            {
                RestoredPolygonHandlePair restoredPair;
                restoredPair.m_before = polygonHandlesToRemove[index];
                restoredPair.m_after = polygonHandlesToRestore[index];

                appendedPolygonHandles.m_restoredPolygonHandles.push_back(AZStd::move(restoredPair));
            }

            // add linking polygons
            for (const auto& verts : vertsForLinkingAdjacentPolygons)
            {
                AddPolygon(whiteBox, verts);
            }

            // add top polygon
            PolygonHandle newPolygonHandle = AddPolygon(whiteBox, topFacesToAdd);

            // add new adjacent polygons
            for (const auto& verts : vertsForNewAdjacentPolygons)
            {
                AddPolygon(whiteBox, verts);
            }

            whiteBox.mesh.update_normals();

            appendedPolygonHandles.m_appendedPolygonHandle = newPolygonHandle;

            return appendedPolygonHandles;
        }

        using AppendFn = AZStd::function<AZ::Vector3(const AZ::Vector3&)>;

        static AppendFn TranslatePoint(const AZ::Vector3& direction, const float distance)
        {
            return [direction, distance](const AZ::Vector3& point)
            {
                return point + (direction * distance);
            };
        }

        PolygonHandle TranslatePolygonAppend(
            WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle, const float distance)
        {
            WHITEBOX_LOG("White Box", "TranslatePolygonAppend ph(%s) %f", ToString(polygonHandle).c_str(), distance)
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            return TranslatePolygonAppendAdvanced(whiteBox, polygonHandle, distance).m_appendedPolygonHandle;
        }

        AppendedPolygonHandles TranslatePolygonAppendAdvanced(
            WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle, const float distance)
        {
            WHITEBOX_LOG(
                "White Box", "TranslatePolygonAppendAdvanced ph(%s) %f", ToString(polygonHandle).c_str(), distance)
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            // check mesh has faces
            if (whiteBox.mesh.n_faces() == 0)
            {
                return {};
            }

            auto appendedPolygonHandles =
                Extrude(whiteBox, polygonHandle, TranslatePoint(PolygonNormal(whiteBox, polygonHandle), distance));

            return appendedPolygonHandles;
        }

        void TranslatePolygon(WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle, const float distance)
        {
            WHITEBOX_LOG("White Box", "TranslatePolygon ph(%s) %d", ToString(polygonHandle).c_str(), distance)
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const auto vertexHandles = PolygonVertexHandles(whiteBox, polygonHandle);
            const auto vertexPositions = VertexPositions(whiteBox, vertexHandles);
            const auto normal = PolygonNormal(whiteBox, polygonHandle);

            for (size_t index = 0; index < vertexHandles.size(); ++index)
            {
                SetVertexPosition(whiteBox, vertexHandles[index], vertexPositions[index] + normal * distance);
            }

            CalculatePlanarUVs(whiteBox);
        }

        PolygonHandle ScalePolygonAppendRelative(
            WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle, const float scale)
        {
            WHITEBOX_LOG("White Box", "ScalePolygonAppendRelative ph(%s) %f", ToString(polygonHandle).c_str(), scale);
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            // check mesh has faces
            if (whiteBox.mesh.n_faces() == 0)
            {
                return {};
            }

            const AZ::Transform polygonSpace =
                PolygonSpace(whiteBox, polygonHandle, PolygonMidpoint(whiteBox, polygonHandle));

            const auto scalePolygonFn = [polygonSpace, scale](const AZ::Vector3& localPosition)
            {
                return ScalePosition(1.0f + scale, localPosition, polygonSpace);
            };

            auto appendedPolygonHandles = Extrude(whiteBox, polygonHandle, scalePolygonFn);

            return appendedPolygonHandles.m_appendedPolygonHandle;
        }

        static AZ::Transform BuildSpace(const AZ::Vector3& normal, const AZ::Vector3& pivot)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            AZ::Vector3 axis1;
            AZ::Vector3 axis2;
            CalculateOrthonormalBasis(normal, axis1, axis2);

            AZ::Matrix3x4 matrix = AZ::Matrix3x4::CreateFromColumns(axis1, axis2, normal, pivot);

            return AZ::Transform::CreateFromMatrix3x4(matrix);
        }

        AZ::Transform PolygonSpace(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle, const AZ::Vector3& pivot)
        {
            return BuildSpace(PolygonNormal(whiteBox, polygonHandle), pivot);
        }

        AZ::Transform EdgeSpace(const WhiteBoxMesh& whiteBox, const EdgeHandle edgeHandle, const AZ::Vector3& pivot)
        {
            const auto vertexPositions = EdgeVertexPositions(whiteBox, edgeHandle);
            return BuildSpace((vertexPositions[1] - vertexPositions[0]).GetNormalizedSafe(), pivot);
        }

        void ScalePolygonRelative(
            WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle, const AZ::Vector3& pivot,
            const float scaleDelta)
        {
            WHITEBOX_LOG(
                "White Box", "ScalePolygonRelative ph(%s) pivot %s scale: %f", ToString(polygonHandle).c_str(),
                AZ::ToString(pivot).c_str(), scaleDelta);
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const AZ::Transform polygonSpace = PolygonSpace(whiteBox, polygonHandle, pivot);
            for (const auto& vertexHandle : PolygonVertexHandles(whiteBox, polygonHandle))
            {
                SetVertexPosition(
                    whiteBox, vertexHandle,
                    ScalePosition(1.0f + scaleDelta, VertexPosition(whiteBox, vertexHandle), polygonSpace));
            }

            CalculateNormals(whiteBox);
            CalculatePlanarUVs(whiteBox);
        }

        bool WriteMesh(const WhiteBoxMesh& whiteBox, WhiteBoxMeshStream& output)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            AZStd::lock_guard lg(g_omSerializationLock);

            std::stringstream whiteBoxStream;
            if (OpenMesh::IO::write_mesh(
                    whiteBox.mesh, whiteBoxStream, ".om",
                    OpenMesh::IO::Options::Binary | OpenMesh::IO::Options::FaceTexCoord |
                        OpenMesh::IO::Options::FaceNormal))
            {
                const std::string outputStr = whiteBoxStream.str();
                output.clear();
                output.reserve(outputStr.size());
                AZStd::copy(outputStr.cbegin(), outputStr.cend(), AZStd::back_inserter(output));

                return true;
            }

            // handle error
            return false;
        }

        ReadResult ReadMesh(WhiteBoxMesh& whiteBox, const WhiteBoxMeshStream& input)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            if (input.empty())
            {
                return ReadResult::Empty;
            }

            std::string inputStr;
            inputStr.reserve(input.size());
            AZStd::copy(input.cbegin(), input.cend(), AZStd::back_inserter(inputStr));

            std::stringstream whiteBoxStream;
            whiteBoxStream.str(inputStr);
            whiteBoxStream >> std::noskipws;

            return ReadMesh(whiteBox, whiteBoxStream);
        }

        ReadResult ReadMesh(WhiteBoxMesh& whiteBox, std::istream& input)
        {
            const auto skipws = input.flags() & std::ios_base::skipws;
            AZ_Assert(skipws == 0, "Input stream must not skip white space characters");

            if (skipws != 0)
            {
                return ReadResult::Error;
            }

            AZStd::lock_guard lg(g_omSerializationLock);
            OpenMesh::IO::Options options{OpenMesh::IO::Options::FaceTexCoord | OpenMesh::IO::Options::FaceNormal};
            return OpenMesh::IO::read_mesh(whiteBox.mesh, input, ".om", options) ? ReadResult::Full : ReadResult::Error;
        }

        WhiteBoxMeshPtr CloneMesh(const WhiteBoxMesh& whiteBox)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            WhiteBoxMeshStream clonedData;
            if (!WriteMesh(whiteBox, clonedData))
            {
                return nullptr;
            }

            WhiteBoxMeshPtr newMesh = CreateWhiteBoxMesh();
            if (ReadMesh(*newMesh, clonedData) != ReadResult::Full)
            {
                return nullptr;
            }

            return newMesh;
        }

        bool SaveToObj(const WhiteBoxMesh& whiteBox, const AZStd::string& filePath)
        {
            OpenMesh::IO::Options options{OpenMesh::IO::Options::FaceTexCoord};
            OpenMesh::IO::ExporterT<WhiteBox::Mesh> exporter{whiteBox.mesh};
            return OpenMesh::IO::OBJWriter().write(filePath.c_str(), exporter, options);
        }

        bool SaveToWbm(const WhiteBoxMesh& whiteBox, AZ::IO::GenericStream& stream)
        {
            WhiteBoxMeshStream buffer;
            const bool success = WhiteBox::Api::WriteMesh(whiteBox, buffer);

            const auto bytesWritten = stream.Write(buffer.size(), buffer.data());
            return success && bytesWritten == buffer.size();
        }

        bool SaveToWbm(const WhiteBoxMesh& whiteBox, const AZStd::string& filePath)
        {
            AZ::IO::FileIOStream fileStream(filePath.c_str(), AZ::IO::OpenMode::ModeWrite);
            if (!fileStream.IsOpen())
            {
                return false;
            }

            return SaveToWbm(whiteBox, fileStream);
        }

        static AZStd::string TrimLastChar(const AZStd::string& str)
        {
            if (str.empty())
            {
                return str;
            }

            return str.substr(0, str.length() - 1);
        }

        AZStd::string ToString(const PolygonHandle& polygonHandle)
        {
            AZStd::string str = "";
            for (auto faceHandle : polygonHandle.m_faceHandles)
            {
                str.append(ToString(faceHandle)).append(",");
            }

            return TrimLastChar(str);
        }

        AZStd::string ToString(const FaceVertHandles& faceVertHandles)
        {
            AZStd::string str = "";
            for (auto vertexHandle : faceVertHandles.m_vertexHandles)
            {
                str.append(ToString(vertexHandle)).append(",");
            }

            return TrimLastChar(str);
        }

        AZStd::string ToString(const FaceVertHandlesList& faceVertHandlesList)
        {
            AZStd::string str = "";
            for (auto faceVertHandles : faceVertHandlesList)
            {
                str.append("fvh(").append(ToString(faceVertHandles)).append("),");
            }

            return TrimLastChar(str);
        }

        AZStd::string ToString(const VertexHandle vertexHandle)
        {
            return AZStd::to_string(vertexHandle.Index());
        }

        AZStd::string ToString(const FaceHandle faceHandle)
        {
            return AZStd::to_string(faceHandle.Index());
        }

        AZStd::string ToString(const EdgeHandle edgeHandle)
        {
            return AZStd::to_string(edgeHandle.Index());
        }

        AZStd::string ToString(const HalfedgeHandle halfedgeHandle)
        {
            return AZStd::to_string(halfedgeHandle.Index());
        }
    } // namespace Api

} // namespace WhiteBox
