/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <cmath>
#include "Util/WhiteBoxMathUtil.h"
#include "Util/WhiteBoxTextureUtil.h"

#include <AzCore/Console/Console.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/MathStringConversions.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
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
AZ_PUSH_DISABLE_WARNING(4996, "-Wdeprecated-declarations") // OpenMesh\Core\Utils\PropertyManager.hh uses deprecated functions
AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option") // Conditional expression is constant.
AZ_PUSH_DISABLE_WARNING(, "-Wdeprecated-literal-operator") // OpenMesh\Core\Geometry\Vector11T.hh uses deprecated literal operator
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/IO/SR_binary.hh>
#include <OpenMesh/Core/IO/importer/ImporterT.hh>
#include <OpenMesh/Core/Mesh/Traits.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <OpenMesh/Core/Utils/GenProg.hh>
#include <OpenMesh/Core/Utils/vector_traits.hh>
AZ_POP_DISABLE_WARNING
AZ_POP_DISABLE_WARNING
AZ_POP_DISABLE_WARNING
AZ_POP_DISABLE_WARNING

AZ_DECLARE_BUDGET_SHARED(AzToolsFramework);
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
    // stored form of Api::UvProjection; new faces value-initialise to the original mapping
    struct FaceUvProjectionInternal
    {
        AZ::u32 m_mode = 0;
        float m_scaleU = 1.0f;
        float m_scaleV = 1.0f;
        float m_offsetU = 0.0f;
        float m_offsetV = 0.0f;
        float m_rotation = 0.0f;
    };
    using FaceUvProjectionPropertyHandle = OpenMesh::FPropHandleT<FaceUvProjectionInternal>;
    static const char* const FaceUvProjectionProp = "WhiteBoxFaceUvProjection";
    // per-face smoothing group bitmask; new faces value-initialise to zero (flat)
    static const char* const FaceSmoothingProp = "WhiteBoxFaceSmoothingGroups";
    // stored form of Api::ManualUvMap: origin xyz, origin uv, u gradient xyz, v gradient xyz, normal xyz; zeros mean no map
    struct FaceManualUvInternal
    {
        float m_values[14] = {};
    };
    using FaceManualUvPropertyHandle = OpenMesh::FPropHandleT<FaceManualUvInternal>;
    static const char* const FaceManualUvProp = "WhiteBoxFaceManualUv";
    // per-corner flag: a Manual face's UV here is authored; false (the default for new corners) means fill it from the map
    using HalfedgeBoolPropertyHandle = OpenMesh::HPropHandleT<bool>;
    static const char* const HalfedgeUvAuthoredProp = "WhiteBoxHalfedgeUvAuthored";
    // per-vertex blend weights packed as RGB bytes (R low) with the top byte set once painted; zero means never painted
    using VertexUintPropertyHandle = OpenMesh::VPropHandleT<unsigned int>;
    static const char* const VertexBlendProp = "WhiteBoxVertexBlend";
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

        static std::string type_identifier(void) { return "WhiteBox::FaceHandlePolygonMapping"; }

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

    template<>
    struct binary<WhiteBox::FaceUvProjectionInternal>
    {
        using value_type = WhiteBox::FaceUvProjectionInternal;
        static const bool is_streamable = true;

        static std::string type_identifier(void) { return "WhiteBox::FaceUvProjection"; }

        // UnknownSize keeps PropertyT on its per-element path instead of a raw memory copy.
        static size_t size_of()
        {
            return UnknownSize;
        }

        static size_t size_of(const value_type& /*_v*/)
        {
            return sizeof(uint32_t) + 5 * sizeof(float);
        }

        static size_t store(std::ostream& _os, const value_type& _v, bool _swap = false)
        {
            size_t bytes = IO::store(_os, static_cast<uint32_t>(_v.m_mode), _swap);
            for (const float value : { _v.m_scaleU, _v.m_scaleV, _v.m_offsetU, _v.m_offsetV, _v.m_rotation })
            {
                bytes += IO::store(_os, value, _swap);
            }
            return _os.good() ? bytes : 0;
        }

        static size_t restore(std::istream& _is, value_type& _v, bool _swap = false)
        {
            uint32_t mode = 0;
            size_t bytes = IO::restore(_is, mode, _swap);
            _v.m_mode = mode;
            for (float* value : { &_v.m_scaleU, &_v.m_scaleV, &_v.m_offsetU, &_v.m_offsetV, &_v.m_rotation })
            {
                bytes += IO::restore(_is, *value, _swap);
            }
            return _is.good() ? bytes : 0;
        }
    };

    template<>
    struct binary<WhiteBox::FaceManualUvInternal>
    {
        using value_type = WhiteBox::FaceManualUvInternal;
        static const bool is_streamable = true;

        static std::string type_identifier(void) { return "WhiteBox::FaceManualUv"; }

        static size_t size_of()
        {
            return UnknownSize;
        }

        static size_t size_of(const value_type& /*_v*/)
        {
            return 14 * sizeof(float);
        }

        static size_t store(std::ostream& _os, const value_type& _v, bool _swap = false)
        {
            size_t bytes = 0;
            for (const float value : _v.m_values)
            {
                bytes += IO::store(_os, value, _swap);
            }
            return _os.good() ? bytes : 0;
        }

        static size_t restore(std::istream& _is, value_type& _v, bool _swap = false)
        {
            size_t bytes = 0;
            for (float& value : _v.m_values)
            {
                bytes += IO::restore(_is, value, _swap);
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
        AZ_CLASS_ALLOCATOR(WhiteBoxMesh, AZ::SystemAllocator);

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

            OpenMesh::FPropHandleT<std::string> materialProperty;
            if (!whiteBox.mesh.get_property_handle(materialProperty, "WhiteBoxFaceMaterial"))
            {
                whiteBox.mesh.add_property(materialProperty, "WhiteBoxFaceMaterial");
            }
            whiteBox.mesh.property(materialProperty).set_persistent(true);

            OpenMesh::FPropHandleT<unsigned int> colorProperty;
            if (!whiteBox.mesh.get_property_handle(colorProperty, "WhiteBoxFacePaintColor"))
            {
                whiteBox.mesh.add_property(colorProperty, "WhiteBoxFacePaintColor");
            }
            whiteBox.mesh.property(colorProperty).set_persistent(true);

            FaceUvProjectionPropertyHandle uvProjectionProperty;
            if (!whiteBox.mesh.get_property_handle(uvProjectionProperty, FaceUvProjectionProp))
            {
                whiteBox.mesh.add_property(uvProjectionProperty, FaceUvProjectionProp);
            }
            whiteBox.mesh.property(uvProjectionProperty).set_persistent(true);

            OpenMesh::FPropHandleT<unsigned int> smoothingProperty;
            if (!whiteBox.mesh.get_property_handle(smoothingProperty, FaceSmoothingProp))
            {
                whiteBox.mesh.add_property(smoothingProperty, FaceSmoothingProp);
            }
            whiteBox.mesh.property(smoothingProperty).set_persistent(true);

            FaceManualUvPropertyHandle manualUvProperty;
            if (!whiteBox.mesh.get_property_handle(manualUvProperty, FaceManualUvProp))
            {
                whiteBox.mesh.add_property(manualUvProperty, FaceManualUvProp);
            }
            whiteBox.mesh.property(manualUvProperty).set_persistent(true);

            HalfedgeBoolPropertyHandle authoredProperty;
            if (!whiteBox.mesh.get_property_handle(authoredProperty, HalfedgeUvAuthoredProp))
            {
                whiteBox.mesh.add_property(authoredProperty, HalfedgeUvAuthoredProp);
            }
            whiteBox.mesh.property(authoredProperty).set_persistent(true);

            VertexUintPropertyHandle blendProperty;
            if (!whiteBox.mesh.get_property_handle(blendProperty, VertexBlendProp))
            {
                whiteBox.mesh.add_property(blendProperty, VertexBlendProp);
            }
            whiteBox.mesh.property(blendProperty).set_persistent(true);

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
            // A polygon's first face represents it; keep first-encounter order without a linear search.
            AZStd::unordered_set<int> seenPolygons;
            seenPolygons.reserve(polygonProps.size());
            for (const auto& polygonProp : polygonProps)
            {
                const int representative = polygonProp.second.empty() ? -1 : polygonProp.second.front().idx();
                if (seenPolygons.insert(representative).second)
                {
                    polygonHandles.push_back(PolygonHandleFromInternal(polygonProp.second));
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
                "White Box", "TranslateEdge eh(%s) %s", ToString(edgeHandle).c_str(), AZStd::to_string(displacement).c_str());
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

        // Only use on a disposable candidate: a failed insertion can leave one triangle.
        // Unlike AddPolygon this must never silently duplicate vertices to bypass a topology error.
        static PolygonHandle AddQuadWithoutFallback(
            WhiteBoxMesh& whiteBox, const AZStd::array<VertexHandle, 4>& vertices)
        {
            FaceHandlesInternal faces;
            for (size_t i = 1; i < 3; ++i)
            {
                const auto face = whiteBox.mesh.add_face(om_vh(vertices[0]), om_vh(vertices[i]), om_vh(vertices[i + 1]));
                if (!face.is_valid()) { return {}; }
                faces.push_back(face);
            }
            PolygonPropertyHandle property;
            whiteBox.mesh.get_property_handle(property, PolygonProps);
            auto& groups = whiteBox.mesh.property(property);
            for (const auto face : faces) { groups[face] = faces; }
            return PolygonHandleFromInternal(faces);
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

            const auto firstFace = HalfedgeFaceHandle(whiteBox, firstHalfedgeHandle);
            const auto secondFace = HalfedgeFaceHandle(whiteBox, secondHalfedgeHandle);
            if (!firstFace.IsValid()) { return secondFace.IsValid() ? secondHalfedgeHandle : HalfedgeHandle{}; }
            if (!secondFace.IsValid()) { return firstHalfedgeHandle; }

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
            if (!halfedgeHandle.IsValid()) { return {}; }

            const FaceHandle faceHandle = HalfedgeFaceHandle(whiteBox, halfedgeHandle);

            // find the polygon this face handle corresponds to
            const PolygonHandle polygonHandle = FacePolygonHandle(whiteBox, faceHandle);
            if (polygonHandle.m_faceHandles.empty() || polygonHandle.m_faceHandles.size() > 2) { return {}; }
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

            // Empty/malformed groups and internal triangulation edges cannot be extruded.
            // Validate before modulo/indexing, and before allocating any new vertices.
            if ((borderVertexHandles.size() != 3 && borderVertexHandles.size() != 4) ||
                static_cast<size_t>(toVertexHandlePolygonIndex) >= borderVertexHandles.size() ||
                static_cast<size_t>(fromVertexHandlePolygonIndex) >= borderVertexHandles.size() ||
                (static_cast<size_t>(fromVertexHandlePolygonIndex) + 1) % borderVertexHandles.size() !=
                    static_cast<size_t>(toVertexHandlePolygonIndex))
            {
                return {};
            }

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
            return !polygonHandle.m_faceHandles.empty() && polygonHandle.m_faceHandles.size() <= 2;
        }

        EdgeHandle TranslateEdgeAppend(
            WhiteBoxMesh& whiteBox, const EdgeHandle edgeHandle, const AZ::Vector3& displacement)
        {
            WHITEBOX_LOG(
                "White Box", "TranslateEdgeAppend eh(%s) %s", ToString(edgeHandle).c_str(), AZStd::to_string(displacement).c_str());
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            if (!edgeHandle.IsValid() || static_cast<size_t>(edgeHandle.Index()) >= whiteBox.mesh.n_edges() ||
                (whiteBox.mesh.has_edge_status() && whiteBox.mesh.status(om_eh(edgeHandle)).deleted()))
            {
                return edgeHandle;
            }
            if (EdgeIsBoundary(whiteBox, edgeHandle))
            {
                // Extend an open surface with one quad. The solid-edge path below replaces
                // a neighboring polygon and is not appropriate for a missing face.
                const auto neighbors = EdgeFaceHandles(whiteBox, edgeHandle);
                if (neighbors.size() != 1) { return edgeHandle; }
                auto boundary = EdgeHalfedgeHandle(whiteBox, edgeHandle, EdgeHalfedge::First);
                if (!HalfedgeIsBoundary(whiteBox, boundary))
                {
                    boundary = HalfedgeOppositeHalfedgeHandle(whiteBox, boundary);
                }
                const auto from = HalfedgeVertexHandleAtTail(whiteBox, boundary);
                const auto to = HalfedgeVertexHandleAtTip(whiteBox, boundary);
                const auto fromPosition = VertexPosition(whiteBox, from);
                const auto toPosition = VertexPosition(whiteBox, to);
                if ((toPosition - fromPosition).Cross(displacement).GetLengthSq() <= 1e-12f) { return edgeHandle; }
                auto candidate = CloneMesh(whiteBox);
                const auto newTo = AddVertex(*candidate, toPosition + displacement);
                const auto newFrom = AddVertex(*candidate, fromPosition + displacement);
                const auto polygon = AddQuadWithoutFallback(*candidate, {{from, to, newTo, newFrom}});
                if (polygon.m_faceHandles.size() != 2) { return edgeHandle; }
                for (const auto face : polygon.m_faceHandles)
                {
                    CopyFaceAttributes(*candidate, face, whiteBox, neighbors[0]);
                }
                CalculateNormals(*candidate);
                CalculatePlanarUVs(*candidate, polygon.m_faceHandles);
                const auto outer = candidate->mesh.find_halfedge(om_vh(newTo), om_vh(newFrom));
                if (!outer.is_valid()) { return edgeHandle; }
                const EdgeHandle result{candidate->mesh.edge_handle(outer).idx()};
                whiteBox.mesh = candidate->mesh;
                return result;
            }

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

        AZ::u64 MeshFaceCount(const WhiteBoxMesh& whiteBox)
        {
            return static_cast<AZ::u64>(whiteBox.mesh.n_faces());
        }

        AZ::u64 MeshHalfedgeCount(const WhiteBoxMesh& whiteBox)
        {
            return static_cast<AZ::u64>(whiteBox.mesh.n_halfedges());
        }

        AZ::u64 MeshVertexCount(const WhiteBoxMesh& whiteBox)
        {
            return static_cast<AZ::u64>(whiteBox.mesh.n_vertices());
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

        static UvProjection ToUvProjection(const FaceUvProjectionInternal& stored)
        {
            UvProjection projection;
            projection.m_mode = stored.m_mode == static_cast<AZ::u32>(UvProjectionMode::Planar)   ? UvProjectionMode::Planar
                : stored.m_mode == static_cast<AZ::u32>(UvProjectionMode::Manual) ? UvProjectionMode::Manual
                                                                                  : UvProjectionMode::World;
            projection.m_scale = AZ::Vector2(stored.m_scaleU, stored.m_scaleV);
            projection.m_offset = AZ::Vector2(stored.m_offsetU, stored.m_offsetV);
            projection.m_rotationDegrees = stored.m_rotation;
            return projection;
        }

        static FaceUvProjectionInternal ToFaceUvProjectionInternal(const UvProjection& projection)
        {
            FaceUvProjectionInternal stored;
            stored.m_mode = static_cast<AZ::u32>(projection.m_mode);
            stored.m_scaleU = projection.m_scale.GetX();
            stored.m_scaleV = projection.m_scale.GetY();
            stored.m_offsetU = projection.m_offset.GetX();
            stored.m_offsetV = projection.m_offset.GetY();
            stored.m_rotation = projection.m_rotationDegrees;
            return stored;
        }

        static ManualUvMap ToManualUvMap(const FaceManualUvInternal& stored)
        {
            const float* v = stored.m_values;
            ManualUvMap map;
            map.m_origin = AZ::Vector3(v[0], v[1], v[2]);
            map.m_originUv = AZ::Vector2(v[3], v[4]);
            map.m_uGradient = AZ::Vector3(v[5], v[6], v[7]);
            map.m_vGradient = AZ::Vector3(v[8], v[9], v[10]);
            map.m_normal = AZ::Vector3(v[11], v[12], v[13]);
            return map;
        }

        static FaceManualUvInternal ToFaceManualUvInternal(const ManualUvMap& map)
        {
            FaceManualUvInternal stored;
            const float values[14] = { map.m_origin.GetX(),    map.m_origin.GetY(),    map.m_origin.GetZ(),    map.m_originUv.GetX(),
                                       map.m_originUv.GetY(),  map.m_uGradient.GetX(), map.m_uGradient.GetY(), map.m_uGradient.GetZ(),
                                       map.m_vGradient.GetX(), map.m_vGradient.GetY(), map.m_vGradient.GetZ(), map.m_normal.GetX(),
                                       map.m_normal.GetY(),    map.m_normal.GetZ() };
            for (size_t i = 0; i < 14; ++i)
            {
                stored.m_values[i] = values[i];
            }
            return stored;
        }

        AZ::Vector2 ManualUvMap::Evaluate(const AZ::Vector3& position) const
        {
            const AZ::Vector3 local = position - m_origin;
            return m_originUv + AZ::Vector2(local.Dot(m_uGradient), local.Dot(m_vGradient));
        }

        // The affine map a triangle's corner UVs define; invalid for a collapsed triangle.
        static ManualUvMap ManualUvMapFromTriangle(
            const AZ::Vector3& p0, const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector2& t0, const AZ::Vector2& t1,
            const AZ::Vector2& t2)
        {
            ManualUvMap map;
            const AZ::Vector3 e1 = p1 - p0;
            const AZ::Vector3 e2 = p2 - p0;
            const AZ::Vector3 normal = e1.Cross(e2);
            // The gradients lie in the triangle's plane: solve the 2x2 Gram system for each of U and V.
            const float a = e1.Dot(e1);
            const float b = e1.Dot(e2);
            const float c = e2.Dot(e2);
            const float determinant = a * c - b * b;
            if (normal.GetLengthSq() <= 1e-16f || AZStd::abs(determinant) <= 1e-20f)
            {
                return map;
            }
            const AZ::Vector2 d1 = t1 - t0;
            const AZ::Vector2 d2 = t2 - t0;
            const auto gradient = [&](const float g1, const float g2)
            {
                const float x = (c * g1 - b * g2) / determinant;
                const float y = (a * g2 - b * g1) / determinant;
                return e1 * x + e2 * y;
            };
            map.m_origin = p0;
            map.m_originUv = t0;
            map.m_uGradient = gradient(d1.GetX(), d2.GetX());
            map.m_vGradient = gradient(d1.GetY(), d2.GetY());
            map.m_normal = normal.GetNormalized();
            return map;
        }

        // Exact field test, cheap enough for every face of every recalculation.
        static bool IsDefaultFaceUvProjection(const FaceUvProjectionInternal& stored)
        {
            return stored.m_mode == 0 && stored.m_scaleU == 1.0f && stored.m_scaleV == 1.0f && stored.m_offsetU == 0.0f &&
                stored.m_offsetV == 0.0f && stored.m_rotation == 0.0f;
        }

        // Maps one face's positions to UVs, with the per-face work done once.
        class FaceUvMapper
        {
        public:
            FaceUvMapper(const UvProjection& projection, const AZ::Vector3& normal)
                : m_projection(projection)
                , m_normal(normal)
                , m_default(projection.IsDefault())
            {
                if (projection.m_mode == UvProjectionMode::Planar)
                {
                    // Sign the normal by its largest component so both windings of a face map alike.
                    AZ::Vector3 facing = normal.GetNormalizedSafe();
                    if (facing.GetElement(FindLargestElement(facing)) < 0.0f)
                    {
                        facing = -facing;
                    }
                    // U stays horizontal so wall textures stand upright; a flat face falls back to X.
                    m_uAxis = AZ::Vector3::CreateAxisZ().Cross(facing);
                    m_uAxis = m_uAxis.GetLengthSq() < 1e-6f ? AZ::Vector3::CreateAxisX() : m_uAxis.GetNormalized();
                    m_vAxis = facing.Cross(m_uAxis);
                }
                if (!m_default && projection.m_rotationDegrees != 0.0f)
                {
                    const float radians = AZ::DegToRad(projection.m_rotationDegrees);
                    m_cos = std::cos(radians);
                    m_sin = std::sin(radians);
                }
            }

            // The projected coordinate before scale and offset.
            AZ::Vector2 Rotated(const AZ::Vector3& position) const
            {
                // Image rows run downward, so V is negated to keep up on the surface up in the texture.
                const AZ::Vector2 raw = m_projection.m_mode == UvProjectionMode::Planar
                    ? AZ::Vector2(position.Dot(m_uAxis), -position.Dot(m_vAxis))
                    : CreatePlanarUVFromVertex(m_normal, position);
                return AZ::Vector2(raw.GetX() * m_cos - raw.GetY() * m_sin, raw.GetX() * m_sin + raw.GetY() * m_cos);
            }

            AZ::Vector2 operator()(const AZ::Vector3& position) const
            {
                return m_default ? CreatePlanarUVFromVertex(m_normal, position)
                                 : Rotated(position) * m_projection.m_scale + m_projection.m_offset;
            }

        private:
            UvProjection m_projection;
            AZ::Vector3 m_normal;
            AZ::Vector3 m_uAxis = AZ::Vector3::CreateAxisX();
            AZ::Vector3 m_vAxis = AZ::Vector3::CreateAxisY();
            float m_cos = 1.0f;
            float m_sin = 0.0f;
            bool m_default = true;
        };

        void CalculatePlanarUVs(WhiteBoxMesh& whiteBox, const FaceHandles& faceHandles)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            auto& mesh = whiteBox.mesh;
            FaceUvProjectionPropertyHandle projectionProperty;
            const bool hasProjections = mesh.get_property_handle(projectionProperty, FaceUvProjectionProp);
            FaceManualUvPropertyHandle manualProperty;
            HalfedgeBoolPropertyHandle authoredProperty;
            const bool hasManual = mesh.get_property_handle(manualProperty, FaceManualUvProp) &&
                mesh.get_property_handle(authoredProperty, HalfedgeUvAuthoredProp);
            for (const auto& faceHandle : faceHandles)
            {
                const AZ::Vector3 normal = FaceNormal(whiteBox, faceHandle);
                FaceUvProjectionInternal* stored = hasProjections ? &mesh.property(projectionProperty, om_fh(faceHandle)) : nullptr;
                if (stored != nullptr && hasManual && stored->m_mode == static_cast<AZ::u32>(UvProjectionMode::Manual))
                {
                    FaceManualUvInternal& storedMap = mesh.property(manualProperty, om_fh(faceHandle));
                    const ManualUvMap map = ToManualUvMap(storedMap);
                    bool missing = false;
                    for (auto it = mesh.fh_ccwiter(om_fh(faceHandle)); it.is_valid() && !missing; ++it)
                    {
                        missing = !mesh.property(authoredProperty, *it);
                    }
                    // A new face well off the map's plane (an extruded wall) would smear, so it is projected instead.
                    if (missing && (!map.IsValid() || AZStd::abs(map.m_normal.Dot(normal)) < 0.5f))
                    {
                        *stored = FaceUvProjectionInternal{};
                        stored->m_mode = static_cast<AZ::u32>(UvProjectionMode::Planar);
                        storedMap = FaceManualUvInternal{};
                    }
                    else
                    {
                        AZStd::array<AZ::Vector3, 3> corners;
                        AZStd::array<AZ::Vector2, 3> uvs;
                        size_t corner = 0;
                        for (auto it = mesh.fh_ccwiter(om_fh(faceHandle)); it.is_valid(); ++it)
                        {
                            const Mesh::HalfedgeHandle heh = *it;
                            const AZ::Vector3 position = mesh.point(mesh.to_vertex_handle(heh));
                            if (!mesh.property(authoredProperty, heh))
                            {
                                mesh.set_texcoord2D(heh, map.Evaluate(position));
                                mesh.property(authoredProperty, heh) = true;
                            }
                            if (corner < 3)
                            {
                                corners[corner] = position;
                                uvs[corner] = mesh.texcoord2D(heh);
                                ++corner;
                            }
                        }
                        // The map follows the face as it moves, so a copy taken later still lines up with it.
                        if (corner == 3)
                        {
                            if (const ManualUvMap refreshed = ManualUvMapFromTriangle(corners[0], corners[1], corners[2], uvs[0], uvs[1], uvs[2]);
                                refreshed.IsValid())
                            {
                                storedMap = ToFaceManualUvInternal(refreshed);
                            }
                        }
                        continue;
                    }
                }
                // Nearly every face uses the default, so it skips the mapper and its trigonometry entirely.
                if (stored == nullptr || IsDefaultFaceUvProjection(*stored))
                {
                    for (Mesh::ConstFaceHalfedgeCCWIter faceHalfedgeIt = mesh.fh_ccwiter(om_fh(faceHandle));
                         faceHalfedgeIt.is_valid(); ++faceHalfedgeIt)
                    {
                        const Mesh::HalfedgeHandle heh = *faceHalfedgeIt;
                        const AZ::Vector3 position = mesh.point(mesh.to_vertex_handle(heh));
                        const Mesh::TexCoord2D uv = CreatePlanarUVFromVertex(normal, position);
                        mesh.set_texcoord2D(heh, uv);
                    }
                    continue;
                }
                const FaceUvMapper mapper(ToUvProjection(*stored), normal);
                for (Mesh::ConstFaceHalfedgeCCWIter faceHalfedgeIt = mesh.fh_ccwiter(om_fh(faceHandle));
                     faceHalfedgeIt.is_valid(); ++faceHalfedgeIt)
                {
                    const Mesh::HalfedgeHandle heh = *faceHalfedgeIt;
                    const AZ::Vector3 position = mesh.point(mesh.to_vertex_handle(heh));
                    const Mesh::TexCoord2D uv = mapper(position);
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
            // Closing a ring hides an edge whose sides are already one polygon; appending would double it.
            for (const auto& faceHandle : secondPolygonHandle.m_faceHandles)
            {
                if (AZStd::find(combinedFaceHandles.begin(), combinedFaceHandles.end(), faceHandle) ==
                    combinedFaceHandles.end())
                {
                    combinedFaceHandles.push_back(faceHandle);
                }
            }

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
            // The split re-points an existing corner at the new vertex; its authored UV belonged to the far end, so refill it.
            if (HalfedgeBoolPropertyHandle authored; whiteBox.mesh.get_property_handle(authored, HalfedgeUvAuthoredProp))
            {
                for (auto incoming = whiteBox.mesh.vih_iter(omVertexHandle); incoming.is_valid(); ++incoming)
                {
                    whiteBox.mesh.property(authored, *incoming) = false;
                }
            }

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
                // Both halves of the flipped edge now end at other vertices, so their corners take fresh UVs.
                FaceHandles faces;
                for (int side = 0; side < 2; ++side)
                {
                    const auto halfedge = whiteBox.mesh.halfedge_handle(omEdgeHandle, side);
                    if (HalfedgeBoolPropertyHandle authored; whiteBox.mesh.get_property_handle(authored, HalfedgeUvAuthoredProp))
                    {
                        whiteBox.mesh.property(authored, halfedge) = false;
                    }
                    if (const auto face = whiteBox.mesh.face_handle(halfedge); face.is_valid())
                    {
                        faces.push_back(wb_fh(face));
                    }
                }
                CalculatePlanarUVs(whiteBox, faces);
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
                const auto v0 = om_vh(face.m_vertexHandles[0]);
                const auto v1 = om_vh(face.m_vertexHandles[1]);
                const auto v2 = om_vh(face.m_vertexHandles[2]);

                auto faceHandle = whiteBox.mesh.add_face(v0, v1, v2);
                if (!faceHandle.is_valid())
                {
                    // Inserting this triangle in the current order would create a
                    // non-manifold edge/vertex, so OpenMesh refuses it. Rather than
                    // silently DROP the face (which leaves the mesh open and makes a
                    // later CSG boolean fail with 'mesh is not a valid manifold'),
                    // duplicate its vertices and add it as an isolated patch.
                    // Coincident vertices are re-welded by consumers that need a
                    // closed mesh (the CSG MeshBoolean welds on input).
                    const auto d0 = whiteBox.mesh.add_vertex(whiteBox.mesh.point(v0));
                    const auto d1 = whiteBox.mesh.add_vertex(whiteBox.mesh.point(v1));
                    const auto d2 = whiteBox.mesh.add_vertex(whiteBox.mesh.point(v2));
                    faceHandle = whiteBox.mesh.add_face(d0, d1, d2);
                }

                if (faceHandle.is_valid())
                {
                    polygon.push_back(faceHandle);
                }
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
                "White Box", "SetVertexPosition vh(%s) %s", ToString(vertexHandle).c_str(), AZStd::to_string(position).c_str());
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            whiteBox.mesh.set_point(om_vh(vertexHandle), position);
        }

        void SetVertexPositionAndUpdateUVs(
            WhiteBoxMesh& whiteBox, const VertexHandle vertexHandle, const AZ::Vector3& position)
        {
            WHITEBOX_LOG(
                "White Box", "SetVertexPositionAndUpdateUVs vh(%s) %s", ToString(vertexHandle).c_str(), AZStd::to_string(position).c_str());
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            SetVertexPosition(whiteBox, vertexHandle, position);
            CalculatePlanarUVs(whiteBox);
        }

        VertexHandle AddVertex(WhiteBoxMesh& whiteBox, const AZ::Vector3& vertex)
        {
            WHITEBOX_LOG("White Box", "AddVertex %s", AZStd::to_string(vertex).c_str());
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

        void RemoveIsolatedVertices(WhiteBoxMesh& whiteBox)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            whiteBox.mesh.request_vertex_status();
            whiteBox.mesh.request_edge_status();
            whiteBox.mesh.request_face_status();

            bool removedAny = false;
            for (const auto& vertexHandle : whiteBox.mesh.vertices())
            {
                // RemoveFaces leaves isolated vertices behind, which would show as manipulator dots.
                if (whiteBox.mesh.is_isolated(vertexHandle))
                {
                    whiteBox.mesh.delete_vertex(vertexHandle, false);
                    removedAny = true;
                }
            }

            if (removedAny)
            {
                // Purge deleted vertices (renumbers handles; callers rebuild afterwards).
                whiteBox.mesh.garbage_collection();
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
                AZStd::to_string(pivot).c_str(), scaleDelta);
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

        AZ::u32 FacePaintColor(const WhiteBoxMesh& whiteBox, const FaceHandle face)
        {
            OpenMesh::FPropHandleT<unsigned int> property;
            return whiteBox.mesh.get_property_handle(property, "WhiteBoxFacePaintColor")
                ? whiteBox.mesh.property(property, om_fh(face)) : 0;
        }

        void SetFacePaintColor(WhiteBoxMesh& whiteBox, const FaceHandle face, const AZ::u32 color)
        {
            OpenMesh::FPropHandleT<unsigned int> property;
            if (!whiteBox.mesh.get_property_handle(property, "WhiteBoxFacePaintColor"))
            {
                whiteBox.mesh.add_property(property, "WhiteBoxFacePaintColor");
            }
            whiteBox.mesh.property(property).set_persistent(true);
            whiteBox.mesh.property(property, om_fh(face)) = color;
        }

        AZ::Data::AssetId FaceMaterial(const WhiteBoxMesh& whiteBox, const FaceHandle face)
        {
            OpenMesh::FPropHandleT<std::string> property;
            if (!whiteBox.mesh.get_property_handle(property, "WhiteBoxFaceMaterial"))
            {
                return {};
            }
            const std::string& value = whiteBox.mesh.property(property, om_fh(face));
            return value.empty() ? AZ::Data::AssetId{} : AZ::Data::AssetId::CreateString(value.c_str());
        }

        void SetFaceMaterial(WhiteBoxMesh& whiteBox, const FaceHandle face, const AZ::Data::AssetId& material)
        {
            OpenMesh::FPropHandleT<std::string> property;
            if (!whiteBox.mesh.get_property_handle(property, "WhiteBoxFaceMaterial"))
            {
                whiteBox.mesh.add_property(property, "WhiteBoxFaceMaterial");
            }
            whiteBox.mesh.property(property).set_persistent(true);
            whiteBox.mesh.property(property, om_fh(face)) =
                material.IsValid() ? material.ToString<AZStd::string>().c_str() : "";
        }

        void SetPolygonMaterial(
            WhiteBoxMesh& whiteBox, const PolygonHandle& polygon, const AZ::Data::AssetId& material)
        {
            for (const FaceHandle face : polygon.m_faceHandles)
            {
                SetFaceMaterial(whiteBox, face, material);
            }
        }

        AZ::u32 FaceSmoothingGroups(const WhiteBoxMesh& whiteBox, const FaceHandle face)
        {
            OpenMesh::FPropHandleT<unsigned int> property;
            return whiteBox.mesh.get_property_handle(property, FaceSmoothingProp) ? whiteBox.mesh.property(property, om_fh(face)) : 0u;
        }

        void SetFaceSmoothingGroups(WhiteBoxMesh& whiteBox, const FaceHandle face, const AZ::u32 groups)
        {
            OpenMesh::FPropHandleT<unsigned int> property;
            if (!whiteBox.mesh.get_property_handle(property, FaceSmoothingProp))
            {
                whiteBox.mesh.add_property(property, FaceSmoothingProp);
            }
            whiteBox.mesh.property(property).set_persistent(true);
            whiteBox.mesh.property(property, om_fh(face)) = groups;
        }

        void SetPolygonSmoothingGroups(
            WhiteBoxMesh& whiteBox, const PolygonHandles& polygons, const AZ::u32 groups, const SmoothingEdit edit)
        {
            for (const auto& polygon : polygons)
            {
                for (const FaceHandle face : polygon.m_faceHandles)
                {
                    const AZ::u32 current = FaceSmoothingGroups(whiteBox, face);
                    SetFaceSmoothingGroups(
                        whiteBox, face,
                        edit == SmoothingEdit::Set ? groups : edit == SmoothingEdit::Add ? (current | groups) : (current & ~groups));
                }
            }
        }

        namespace
        {
            constexpr unsigned int BlendPaintedBit = 0x01000000u;

            AZ::Vector3 UnpackBlend(const unsigned int packed)
            {
                return AZ::Vector3(float(packed & 255u), float((packed >> 8) & 255u), float((packed >> 16) & 255u)) / 255.0f;
            }
        } // namespace

        bool MeshHasVertexBlend(const WhiteBoxMesh& whiteBox)
        {
            VertexUintPropertyHandle property;
            if (!whiteBox.mesh.get_property_handle(property, VertexBlendProp))
            {
                return false;
            }
            for (const auto vertex : whiteBox.mesh.vertices())
            {
                if (whiteBox.mesh.property(property, vertex) != 0u)
                {
                    return true;
                }
            }
            return false;
        }

        bool VertexBlendPainted(const WhiteBoxMesh& whiteBox, const VertexHandle vertex)
        {
            VertexUintPropertyHandle property;
            return whiteBox.mesh.get_property_handle(property, VertexBlendProp) && vertex.IsValid() &&
                whiteBox.mesh.property(property, om_vh(vertex)) != 0u;
        }

        AZ::Vector3 VertexBlend(const WhiteBoxMesh& whiteBox, const VertexHandle vertex)
        {
            VertexUintPropertyHandle property;
            if (!whiteBox.mesh.get_property_handle(property, VertexBlendProp) || !vertex.IsValid())
            {
                return AZ::Vector3::CreateZero();
            }
            const unsigned int packed = whiteBox.mesh.property(property, om_vh(vertex));
            if (packed != 0u)
            {
                return UnpackBlend(packed);
            }
            // Never painted (often a vertex an edit just added): blend in from whatever painted vertices it joins.
            AZ::Vector3 sum = AZ::Vector3::CreateZero();
            int count = 0;
            for (auto neighbour = whiteBox.mesh.cvv_iter(om_vh(vertex)); neighbour.is_valid(); ++neighbour)
            {
                const unsigned int value = whiteBox.mesh.property(property, *neighbour);
                if (value != 0u)
                {
                    sum += UnpackBlend(value);
                    ++count;
                }
            }
            return count > 0 ? sum / static_cast<float>(count) : AZ::Vector3::CreateZero();
        }

        void SetVertexBlend(WhiteBoxMesh& whiteBox, const VertexHandle vertex, const AZ::Vector3& weights)
        {
            VertexUintPropertyHandle property;
            if (!whiteBox.mesh.get_property_handle(property, VertexBlendProp))
            {
                whiteBox.mesh.add_property(property, VertexBlendProp);
                whiteBox.mesh.property(property).set_persistent(true);
            }
            const auto channel = [](const float value)
            {
                return static_cast<unsigned int>(AZ::GetClamp(value, 0.0f, 1.0f) * 255.0f + 0.5f);
            };
            whiteBox.mesh.property(property, om_vh(vertex)) =
                BlendPaintedBit | channel(weights.GetX()) | (channel(weights.GetY()) << 8) | (channel(weights.GetZ()) << 16);
        }

        void BakeVertexBlend(WhiteBoxMesh& whiteBox)
        {
            // Read every borrowed value before writing any, or baked vertices would feed their neighbours' averages.
            AZStd::vector<AZStd::pair<VertexHandle, AZ::Vector3>> baked;
            for (const auto vertex : MeshVertexHandles(whiteBox))
            {
                if (!VertexBlendPainted(whiteBox, vertex))
                {
                    baked.emplace_back(vertex, VertexBlend(whiteBox, vertex));
                }
            }
            for (const auto& [vertex, weights] : baked)
            {
                SetVertexBlend(whiteBox, vertex, weights);
            }
        }

        void CopyVertexBlend(WhiteBoxMesh& target, const VertexHandle to, const WhiteBoxMesh& source, const VertexHandle from)
        {
            VertexUintPropertyHandle sourceProperty;
            if (!to.IsValid() || !from.IsValid() || !source.mesh.get_property_handle(sourceProperty, VertexBlendProp))
            {
                return;
            }
            const unsigned int packed = source.mesh.property(sourceProperty, om_vh(from));
            if (packed == 0u)
            {
                return;
            }
            VertexUintPropertyHandle targetProperty;
            if (!target.mesh.get_property_handle(targetProperty, VertexBlendProp))
            {
                target.mesh.add_property(targetProperty, VertexBlendProp);
                target.mesh.property(targetProperty).set_persistent(true);
            }
            target.mesh.property(targetProperty, om_vh(to)) = packed;
        }

        AZStd::array<AZ::Vector3, 3> FaceCornerNormals(const WhiteBoxMesh& whiteBox, const FaceHandle face)
        {
            const AZ::Vector3 flat = FaceNormal(whiteBox, face);
            AZStd::array<AZ::Vector3, 3> normals{ { flat, flat, flat } };
            OpenMesh::FPropHandleT<unsigned int> property;
            if (!whiteBox.mesh.get_property_handle(property, FaceSmoothingProp))
            {
                return normals;
            }
            const auto omFace = om_fh(face);
            const AZ::u32 groups = whiteBox.mesh.property(property, omFace);
            if (groups == 0)
            {
                return normals;
            }
            const auto halfedges = FaceHalfedgeHandles(whiteBox, face);
            for (size_t corner = 0; corner < halfedges.size() && corner < 3; ++corner)
            {
                const auto vertex = whiteBox.mesh.to_vertex_handle(om_heh(halfedges[corner]));
                const AZ::Vector3 centre = VertexPosition(whiteBox, wb_vh(vertex));
                AZ::Vector3 sum = AZ::Vector3::CreateZero();
                // Every face around the corner that shares a group, weighted by its angle there so triangulation does not matter.
                for (auto incoming = whiteBox.mesh.cvih_iter(vertex); incoming.is_valid(); ++incoming)
                {
                    const auto neighbour = whiteBox.mesh.face_handle(*incoming);
                    if (!neighbour.is_valid() || (neighbour != omFace && (whiteBox.mesh.property(property, neighbour) & groups) == 0))
                    {
                        continue;
                    }
                    const AZ::Vector3 previous =
                        VertexPosition(whiteBox, wb_vh(whiteBox.mesh.from_vertex_handle(*incoming))) - centre;
                    const AZ::Vector3 next = VertexPosition(
                        whiteBox, wb_vh(whiteBox.mesh.to_vertex_handle(whiteBox.mesh.next_halfedge_handle(*incoming)))) - centre;
                    if (previous.GetLengthSq() <= 1e-20f || next.GetLengthSq() <= 1e-20f)
                    {
                        continue;
                    }
                    const float angle = acosf(AZ::GetClamp(previous.GetNormalized().Dot(next.GetNormalized()), -1.0f, 1.0f));
                    sum += FaceNormal(whiteBox, wb_fh(neighbour)) * angle;
                }
                if (sum.GetLengthSq() > 1e-12f)
                {
                    normals[corner] = sum.GetNormalized();
                }
            }
            return normals;
        }

        bool UvProjection::IsDefault() const
        {
            return *this == UvProjection{};
        }

        bool UvProjection::operator==(const UvProjection& other) const
        {
            return m_mode == other.m_mode && m_scale.IsClose(other.m_scale, 1e-6f) && m_offset.IsClose(other.m_offset, 1e-6f) &&
                AZ::IsClose(m_rotationDegrees, other.m_rotationDegrees, 1e-6f);
        }

        UvProjection FaceUvProjection(const WhiteBoxMesh& whiteBox, const FaceHandle face)
        {
            FaceUvProjectionPropertyHandle property;
            UvProjection projection = whiteBox.mesh.get_property_handle(property, FaceUvProjectionProp)
                ? ToUvProjection(whiteBox.mesh.property(property, om_fh(face))) : UvProjection{};
            FaceManualUvPropertyHandle manualProperty;
            if (projection.m_mode == UvProjectionMode::Manual && whiteBox.mesh.get_property_handle(manualProperty, FaceManualUvProp))
            {
                projection.m_manual = ToManualUvMap(whiteBox.mesh.property(manualProperty, om_fh(face)));
            }
            return projection;
        }

        void SetFaceUvProjection(WhiteBoxMesh& whiteBox, const FaceHandle face, const UvProjection& projection)
        {
            FaceUvProjectionPropertyHandle property;
            if (!whiteBox.mesh.get_property_handle(property, FaceUvProjectionProp))
            {
                whiteBox.mesh.add_property(property, FaceUvProjectionProp);
            }
            whiteBox.mesh.property(property).set_persistent(true);
            whiteBox.mesh.property(property, om_fh(face)) = ToFaceUvProjectionInternal(projection);
            FaceManualUvPropertyHandle manualProperty;
            if (!whiteBox.mesh.get_property_handle(manualProperty, FaceManualUvProp))
            {
                whiteBox.mesh.add_property(manualProperty, FaceManualUvProp);
                whiteBox.mesh.property(manualProperty).set_persistent(true);
            }
            whiteBox.mesh.property(manualProperty, om_fh(face)) =
                projection.m_mode == UvProjectionMode::Manual ? ToFaceManualUvInternal(projection.m_manual) : FaceManualUvInternal{};
        }

        static HalfedgeBoolPropertyHandle AuthoredProperty(WhiteBoxMesh& whiteBox)
        {
            HalfedgeBoolPropertyHandle property;
            if (!whiteBox.mesh.get_property_handle(property, HalfedgeUvAuthoredProp))
            {
                whiteBox.mesh.add_property(property, HalfedgeUvAuthoredProp);
                whiteBox.mesh.property(property).set_persistent(true);
            }
            return property;
        }

        // Every corner of the faces is marked authored as it stands, and each face takes the map its UVs define.
        static void FreezeFaceUvs(WhiteBoxMesh& whiteBox, const FaceHandles& faces)
        {
            const HalfedgeBoolPropertyHandle authored = AuthoredProperty(whiteBox);
            for (const FaceHandle face : faces)
            {
                UvProjection projection;
                projection.m_mode = UvProjectionMode::Manual;
                const auto halfedges = FaceHalfedgeHandles(whiteBox, face);
                if (halfedges.size() >= 3)
                {
                    const auto point = [&whiteBox](const HalfedgeHandle h) { return VertexPosition(whiteBox, HalfedgeVertexHandleAtTip(whiteBox, h)); };
                    projection.m_manual = ManualUvMapFromTriangle(
                        point(halfedges[0]), point(halfedges[1]), point(halfedges[2]), HalfedgeUV(whiteBox, halfedges[0]),
                        HalfedgeUV(whiteBox, halfedges[1]), HalfedgeUV(whiteBox, halfedges[2]));
                }
                SetFaceUvProjection(whiteBox, face, projection);
                for (const HalfedgeHandle halfedge : halfedges)
                {
                    whiteBox.mesh.property(authored, om_heh(halfedge)) = true;
                }
            }
        }

        void MakePolygonUvsManual(WhiteBoxMesh& whiteBox, const PolygonHandles& polygons)
        {
            FaceHandles faces;
            for (const auto& polygon : polygons)
            {
                for (const FaceHandle face : polygon.m_faceHandles)
                {
                    if (FaceUvProjection(whiteBox, face).m_mode != UvProjectionMode::Manual)
                    {
                        faces.push_back(face);
                    }
                }
            }
            FreezeFaceUvs(whiteBox, faces);
        }

        void SetHalfedgeManualUv(WhiteBoxMesh& whiteBox, const HalfedgeHandle halfedge, const AZ::Vector2& uv)
        {
            const FaceHandle face = HalfedgeFaceHandle(whiteBox, halfedge);
            if (!face.IsValid())
            {
                return;
            }
            if (FaceUvProjection(whiteBox, face).m_mode != UvProjectionMode::Manual)
            {
                FreezeFaceUvs(whiteBox, { face });
            }
            whiteBox.mesh.set_texcoord2D(om_heh(halfedge), uv);
            whiteBox.mesh.property(AuthoredProperty(whiteBox), om_heh(halfedge)) = true;
            CalculatePlanarUVs(whiteBox, { face }); // refreshes the face's map from its corners
        }

        bool HalfedgeUvAuthored(const WhiteBoxMesh& whiteBox, const HalfedgeHandle halfedge)
        {
            HalfedgeBoolPropertyHandle property;
            return whiteBox.mesh.get_property_handle(property, HalfedgeUvAuthoredProp) &&
                whiteBox.mesh.property(property, om_heh(halfedge));
        }

        void SetPolygonUvProjection(WhiteBoxMesh& whiteBox, const PolygonHandles& polygons, const UvProjection& projection)
        {
            if (projection.m_mode == UvProjectionMode::Manual && !projection.m_manual.IsValid())
            {
                MakePolygonUvsManual(whiteBox, polygons);
                return;
            }
            // A pasted Manual map: every corner is refilled from it, so the pasted UVs line up across the polygons.
            const bool pasted = projection.m_mode == UvProjectionMode::Manual;
            const HalfedgeBoolPropertyHandle authored = AuthoredProperty(whiteBox);
            FaceHandles faces;
            for (const auto& polygon : polygons)
            {
                for (const FaceHandle face : polygon.m_faceHandles)
                {
                    SetFaceUvProjection(whiteBox, face, projection);
                    faces.push_back(face);
                    for (const HalfedgeHandle halfedge : FaceHalfedgeHandles(whiteBox, face))
                    {
                        whiteBox.mesh.property(authored, om_heh(halfedge)) = !pasted && whiteBox.mesh.property(authored, om_heh(halfedge));
                    }
                }
            }
            CalculatePlanarUVs(whiteBox, faces);
        }

        void FitPolygonUvProjection(WhiteBoxMesh& whiteBox, const PolygonHandles& polygons)
        {
            FaceHandles faces;
            for (const auto& polygon : polygons)
            {
                if (polygon.m_faceHandles.empty())
                {
                    continue;
                }
                // Mode and rotation come from the polygon; only the placement is fitted.
                UvProjection projection = FaceUvProjection(whiteBox, polygon.m_faceHandles.front());
                if (projection.m_mode == UvProjectionMode::Manual)
                {
                    continue; // authored UVs are not refitted
                }
                projection.m_scale = AZ::Vector2(1.0f, 1.0f);
                projection.m_offset = AZ::Vector2::CreateZero();
                AZ::Vector2 low(AZ::Constants::FloatMax, AZ::Constants::FloatMax);
                AZ::Vector2 high(-AZ::Constants::FloatMax, -AZ::Constants::FloatMax);
                for (const FaceHandle face : polygon.m_faceHandles)
                {
                    const FaceUvMapper mapper(projection, FaceNormal(whiteBox, face));
                    for (const auto& position : FaceVertexPositions(whiteBox, face))
                    {
                        const AZ::Vector2 uv = mapper.Rotated(position);
                        low = low.GetMin(uv);
                        high = high.GetMax(uv);
                    }
                }
                const AZ::Vector2 extent = high - low;
                // A polygon seen edge-on has no extent on one axis; leave that axis unscaled rather than divide by zero.
                projection.m_scale = AZ::Vector2(
                    extent.GetX() > 1e-6f ? 1.0f / extent.GetX() : 1.0f, extent.GetY() > 1e-6f ? 1.0f / extent.GetY() : 1.0f);
                projection.m_offset = -low * projection.m_scale;
                for (const FaceHandle face : polygon.m_faceHandles)
                {
                    SetFaceUvProjection(whiteBox, face, projection);
                    faces.push_back(face);
                }
            }
            CalculatePlanarUVs(whiteBox, faces);
        }

        void NormalizePolygonTexelDensity(WhiteBoxMesh& whiteBox, const PolygonHandles& polygons, const float repeatsPerMetre)
        {
            const float density = AZ::GetMax(repeatsPerMetre, 1e-4f);
            FaceHandles faces;
            for (const auto& polygon : polygons)
            {
                for (const FaceHandle face : polygon.m_faceHandles)
                {
                    UvProjection projection = FaceUvProjection(whiteBox, face);
                    if (projection.m_mode == UvProjectionMode::Manual)
                    {
                        continue; // authored UVs keep their own density
                    }
                    // Offset scales with the tiling so the point at UV zero stays put; the default mapping is one per metre.
                    const AZ::Vector2 ratio(density / AZ::GetMax(projection.m_scale.GetX(), 1e-6f), density / AZ::GetMax(projection.m_scale.GetY(), 1e-6f));
                    projection.m_offset = projection.m_offset * ratio;
                    projection.m_scale = AZ::Vector2(density, density);
                    SetFaceUvProjection(whiteBox, face, projection);
                    faces.push_back(face);
                }
            }
            CalculatePlanarUVs(whiteBox, faces);
        }

        float PolygonTexelDensity(const WhiteBoxMesh& whiteBox, const PolygonHandles& polygons)
        {
            float total = 0.0f;
            size_t count = 0;
            for (const auto& polygon : polygons)
            {
                for (const FaceHandle face : polygon.m_faceHandles)
                {
                    const UvProjection projection = FaceUvProjection(whiteBox, face);
                    total += (projection.m_scale.GetX() + projection.m_scale.GetY()) * 0.5f;
                    ++count;
                }
            }
            return count > 0 ? total / static_cast<float>(count) : 1.0f;
        }

        // Raw per-face attribute storage resolved once, so copies skip the name lookups and the asset id round trip.
        struct FaceAttributeProperties
        {
            OpenMesh::FPropHandleT<std::string> m_material;
            OpenMesh::FPropHandleT<unsigned int> m_paint;
            FaceUvProjectionPropertyHandle m_uvProjection;
            OpenMesh::FPropHandleT<unsigned int> m_smoothing;
            FaceManualUvPropertyHandle m_manualUv;
            bool m_hasManualUv = false;
            bool m_hasMaterial = false;
            bool m_hasPaint = false;
            bool m_hasUvProjection = false;
            bool m_hasSmoothing = false;

            explicit FaceAttributeProperties(const WhiteBoxMesh& whiteBox)
            {
                m_hasMaterial = whiteBox.mesh.get_property_handle(m_material, "WhiteBoxFaceMaterial");
                m_hasPaint = whiteBox.mesh.get_property_handle(m_paint, "WhiteBoxFacePaintColor");
                m_hasUvProjection = whiteBox.mesh.get_property_handle(m_uvProjection, FaceUvProjectionProp);
                m_hasSmoothing = whiteBox.mesh.get_property_handle(m_smoothing, FaceSmoothingProp);
                m_hasManualUv = whiteBox.mesh.get_property_handle(m_manualUv, FaceManualUvProp);
            }

            // Target side: every property exists and persists, as the Set functions would leave it.
            explicit FaceAttributeProperties(WhiteBoxMesh& whiteBox)
                : FaceAttributeProperties(static_cast<const WhiteBoxMesh&>(whiteBox))
            {
                if (!m_hasMaterial)
                {
                    whiteBox.mesh.add_property(m_material, "WhiteBoxFaceMaterial");
                    whiteBox.mesh.property(m_material).set_persistent(true);
                }
                if (!m_hasPaint)
                {
                    whiteBox.mesh.add_property(m_paint, "WhiteBoxFacePaintColor");
                    whiteBox.mesh.property(m_paint).set_persistent(true);
                }
                if (!m_hasUvProjection)
                {
                    whiteBox.mesh.add_property(m_uvProjection, FaceUvProjectionProp);
                    whiteBox.mesh.property(m_uvProjection).set_persistent(true);
                }
                if (!m_hasSmoothing)
                {
                    whiteBox.mesh.add_property(m_smoothing, FaceSmoothingProp);
                    whiteBox.mesh.property(m_smoothing).set_persistent(true);
                }
                if (!m_hasManualUv)
                {
                    whiteBox.mesh.add_property(m_manualUv, FaceManualUvProp);
                    whiteBox.mesh.property(m_manualUv).set_persistent(true);
                }
                m_hasMaterial = m_hasPaint = m_hasUvProjection = m_hasSmoothing = m_hasManualUv = true;
            }
        };

        static void CopyFaceAttributes(
            WhiteBoxMesh& target, const FaceAttributeProperties& to, const FaceHandle targetFace, const WhiteBoxMesh& source,
            const FaceAttributeProperties& from, const FaceHandle sourceFace)
        {
            const auto sourceHandle = om_fh(sourceFace);
            const auto targetHandle = om_fh(targetFace);
            target.mesh.property(to.m_material, targetHandle) =
                from.m_hasMaterial ? source.mesh.property(from.m_material, sourceHandle) : std::string{};
            target.mesh.property(to.m_paint, targetHandle) = from.m_hasPaint ? source.mesh.property(from.m_paint, sourceHandle) : 0u;
            target.mesh.property(to.m_uvProjection, targetHandle) =
                from.m_hasUvProjection ? source.mesh.property(from.m_uvProjection, sourceHandle) : FaceUvProjectionInternal{};
            target.mesh.property(to.m_smoothing, targetHandle) = from.m_hasSmoothing ? source.mesh.property(from.m_smoothing, sourceHandle) : 0u;
            target.mesh.property(to.m_manualUv, targetHandle) =
                from.m_hasManualUv ? source.mesh.property(from.m_manualUv, sourceHandle) : FaceManualUvInternal{};
        }

        void CopyFaceAttributes(
            WhiteBoxMesh& target, const FaceHandle targetFace, const WhiteBoxMesh& source, const FaceHandle sourceFace)
        {
            const FaceAttributeProperties from(source);
            const FaceAttributeProperties to(target);
            CopyFaceAttributes(target, to, targetFace, source, from, sourceFace);
        }

        void CopyFaceAttributes(
            WhiteBoxMesh& target, const FaceHandles& targetFaces, const WhiteBoxMesh& source, const FaceHandles& sourceFaces)
        {
            const FaceAttributeProperties from(source);
            const FaceAttributeProperties to(target);
            for (size_t i = 0; i < targetFaces.size() && i < sourceFaces.size(); ++i)
            {
                CopyFaceAttributes(target, to, targetFaces[i], source, from, sourceFaces[i]);
            }
        }

        bool WriteMesh(const WhiteBoxMesh& whiteBox, WhiteBoxMeshStream& output)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            AZStd::lock_guard lg(g_omSerializationLock);

            std::stringstream whiteBoxStream;
            // OpenMesh 11.x + requires you to specify "Custom" options to write custom properties such as our polygons or those will be skipped
            if (OpenMesh::IO::write_mesh(
                    whiteBox.mesh, whiteBoxStream, ".om",
                    OpenMesh::IO::Options::Binary | OpenMesh::IO::Options::FaceTexCoord | OpenMesh::IO::Options::FaceNormal | OpenMesh::IO::Options::Custom))
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

            // OpenMesh 11.x + requires you to specify "Custom" options to read custom properties such as our polygons or those will be skipped
            AZStd::lock_guard lg(g_omSerializationLock);
            OpenMesh::IO::Options options{OpenMesh::IO::Options::FaceTexCoord | OpenMesh::IO::Options::FaceNormal | OpenMesh::IO::Options::Custom};
            return OpenMesh::IO::read_mesh(whiteBox.mesh, input, ".om", options) ? ReadResult::Full : ReadResult::Error;
        }

        WhiteBoxMeshPtr CloneMesh(const WhiteBoxMesh& whiteBox)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            // OpenMesh deep-copies connectivity and all properties - far cheaper than a write/read round-trip.
            WhiteBoxMeshPtr newMesh(aznew WhiteBoxMesh());
            newMesh->mesh = whiteBox.mesh;
            return newMesh;
        }

        bool BridgeSelection(
            WhiteBoxMesh& whiteBox, const PolygonHandles& polygons, const EdgeHandles& edges, AZStd::string& error)
        {
            error.clear();
            const auto fail = [&error](const char* message)
            {
                error = message;
                return false;
            };
            const bool caps = polygons.size() == 2 && edges.empty();
            if (!caps && !(edges.size() == 2 && polygons.empty()))
            {
                return fail("Select exactly two polygons or two open boundary edges in Transform mode.");
            }

            VertexHandles first;
            VertexHandles second;
            FaceHandles removedFaces;
            FaceHandle sourceFace;
            if (caps)
            {
                const auto allPolygons = MeshPolygonHandles(whiteBox);
                for (const auto& polygon : polygons)
                {
                    if (polygon.m_faceHandles.empty() ||
                        AZStd::find(allPolygons.begin(), allPolygons.end(), polygon) == allPolygons.end())
                    {
                        return fail("The selection is stale. Select the polygons again.");
                    }
                }
                const auto firstBorders = PolygonBorderVertexHandles(whiteBox, polygons[0]);
                const auto secondBorders = PolygonBorderVertexHandles(whiteBox, polygons[1]);
                if (firstBorders.size() != 1 || secondBorders.size() != 1 ||
                    firstBorders[0].size() < 3 || secondBorders[0].size() < 3)
                {
                    return fail("Choose two polygons without holes, each with at least three boundary corners.");
                }
                first = firstBorders[0];
                second = secondBorders[0];
                sourceFace = polygons[0].m_faceHandles.front();
                removedFaces = polygons[0].m_faceHandles;
                removedFaces.insert(removedFaces.end(), polygons[1].m_faceHandles.begin(), polygons[1].m_faceHandles.end());
            }
            else
            {
                const auto allEdges = MeshEdgeHandles(whiteBox);
                for (const auto edge : edges)
                {
                    if (AZStd::find(allEdges.begin(), allEdges.end(), edge) == allEdges.end() ||
                        EdgeFaceHandles(whiteBox, edge).size() != 1)
                    {
                        return fail("Edge Bridge needs two open boundary edges, each belonging to one face.");
                    }
                }
                // Use the empty halfedge direction, so the new quad opposes its existing neighbors.
                for (size_t i = 0; i < edges.size(); ++i)
                {
                    // EdgeHalfedgeHandles deliberately returns only face-bearing halfedges.
                    // Query both directions explicitly to find the missing-face side.
                    for (const auto side : {EdgeHalfedge::First, EdgeHalfedge::Second})
                    {
                        const auto halfedge = EdgeHalfedgeHandle(whiteBox, edges[i], side);
                        if (HalfedgeIsBoundary(whiteBox, halfedge))
                        {
                            auto& boundary = i == 0 ? first : second;
                            boundary = {HalfedgeVertexHandleAtTail(whiteBox, halfedge),
                                        HalfedgeVertexHandleAtTip(whiteBox, halfedge)};
                            break;
                        }
                    }
                }
                if (first.size() != 2 || second.size() != 2)
                {
                    return fail("Could not find two open boundary edges.");
                }
                sourceFace = EdgeFaceHandles(whiteBox, edges[0]).front();
            }
            for (const auto vertex : first)
            {
                if (AZStd::find(second.begin(), second.end(), vertex) != second.end())
                {
                    return fail("Bridge needs separate selections that do not share vertices.");
                }
            }

            if (caps)
            {
                AZ::Vector3 firstCenter = AZ::Vector3::CreateZero();
                AZ::Vector3 secondCenter = AZ::Vector3::CreateZero();
                for (const auto vertex : first) { firstCenter += VertexPosition(whiteBox, vertex); }
                for (const auto vertex : second) { secondCenter += VertexPosition(whiteBox, vertex); }
                const auto direction = secondCenter / static_cast<float>(second.size()) -
                    firstCenter / static_cast<float>(first.size());
                const auto hasOpenBoundary = [&whiteBox](const PolygonHandle& polygon)
                {
                    for (const auto halfedge : PolygonBorderHalfedgeHandlesFlattened(whiteBox, polygon))
                    {
                        if (!HalfedgeOppositeFaceHandle(whiteBox, halfedge).IsValid()) { return true; }
                    }
                    return false;
                };
                // Topology, not face orientation, decides whether these are removable caps.
                // Facing freeform planes are still surfaces and must never be deleted.
                if (hasOpenBoundary(polygons[0]) || hasOpenBoundary(polygons[1]))
                {
                    // Open, side-by-side surfaces (including freeform planes) have no caps
                    // to remove. Connect their closest compatible boundary edges instead.
                    struct OpenSide
                    {
                        EdgeHandle m_edge;
                        AZ::Vector3 m_from;
                        AZ::Vector3 m_to;
                        AZ::Vector3 m_normal;
                    };
                    const auto openSides = [&whiteBox](const PolygonHandle& polygon)
                    {
                        AZStd::vector<OpenSide> sides;
                        for (const auto inner : PolygonBorderHalfedgeHandlesFlattened(whiteBox, polygon))
                        {
                            const auto outer = HalfedgeOppositeHalfedgeHandle(whiteBox, inner);
                            if (HalfedgeIsBoundary(whiteBox, outer))
                            {
                                sides.push_back({HalfedgeEdgeHandle(whiteBox, outer),
                                    HalfedgeVertexPositionAtTail(whiteBox, outer),
                                    HalfedgeVertexPositionAtTip(whiteBox, outer),
                                    FaceNormal(whiteBox, HalfedgeFaceHandle(whiteBox, inner))});
                            }
                        }
                        return sides;
                    };
                    const auto firstSides = openSides(polygons[0]);
                    const auto secondSides = openSides(polygons[1]);
                    EdgeHandles nearest;
                    float nearestDistance = AZ::Constants::FloatMax;
                    for (const auto& a : firstSides)
                    {
                        for (const auto& b : secondSides)
                        {
                            const auto normal0 = (a.m_to - a.m_from).Cross(b.m_from - a.m_from);
                            const auto normal1 = (b.m_from - a.m_from).Cross(b.m_to - a.m_from);
                            // Preserve the orientation of both source surfaces, and reject
                            // collapsed/folded strips before choosing the shortest connection.
                            if (normal0.GetLengthSq() <= 1e-12f || normal1.GetLengthSq() <= 1e-12f ||
                                normal0.Dot(normal1) <= 0.0f ||
                                normal0.Dot(a.m_normal) < -1e-6f || normal1.Dot(b.m_normal) < -1e-6f)
                            {
                                continue;
                            }
                            const float distance = (a.m_from - b.m_to).GetLengthSq() + (a.m_to - b.m_from).GetLengthSq();
                            if (distance < nearestDistance)
                            {
                                nearestDistance = distance;
                                nearest = {a.m_edge, b.m_edge};
                            }
                        }
                    }
                    if (!nearest.empty()) { return BridgeSelection(whiteBox, {}, nearest, error); }
                    return fail("Choose facing caps, or separate open polygons with compatible boundary edges.");
                }
                if (PolygonNormal(whiteBox, polygons[0]).Dot(direction) <= 1e-5f ||
                    PolygonNormal(whiteBox, polygons[1]).Dot(direction) >= -1e-5f)
                {
                    return fail("Choose separated solid caps that face toward each other.");
                }
                if (first.size() != second.size())
                {
                    return fail("Facing caps must have the same number of boundary corners.");
                }
                // Reverse the second loop and find the shortest total correspondence.
                // Reversal ensures both removed caps are replaced with consistent winding.
                VertexHandles aligned;
                float bestDistance = AZ::Constants::FloatMax;
                for (size_t offset = 0; offset < second.size(); ++offset)
                {
                    float distance = 0.0f;
                    for (size_t i = 0; i < first.size(); ++i)
                    {
                        distance += (VertexPosition(whiteBox, first[i]) -
                            VertexPosition(whiteBox, second[(offset + second.size() - i) % second.size()])).GetLengthSq();
                    }
                    if (distance < bestDistance)
                    {
                        bestDistance = distance;
                        aligned.clear();
                        for (size_t i = 0; i < first.size(); ++i)
                        {
                            aligned.push_back(second[(offset + second.size() - i) % second.size()]);
                        }
                    }
                }
                second = AZStd::move(aligned);
            }
            else
            {
                AZStd::reverse(second.begin(), second.end());
            }

            auto candidate = CloneMesh(whiteBox);
            if (!removedFaces.empty())
            {
                // RemoveFaces retains vertices, so the saved boundary vertex handles stay valid.
                RemoveFaces(*candidate, removedFaces);
            }
            FaceHandles addedFaces;
            const size_t spanCount = caps ? first.size() : 1;
            for (size_t i = 0; i < spanCount; ++i)
            {
                const size_t next = (i + 1) % first.size();
                const AZStd::array<VertexHandle, 4> quad{{first[i], first[next], second[next], second[i]}};
                const auto a = VertexPosition(*candidate, quad[0]);
                const auto b = VertexPosition(*candidate, quad[1]);
                const auto c = VertexPosition(*candidate, quad[2]);
                const auto d = VertexPosition(*candidate, quad[3]);
                const auto normal0 = (b - a).Cross(c - a);
                const auto normal1 = (c - a).Cross(d - a);
                if (normal0.GetLengthSq() <= 1e-12f || normal1.GetLengthSq() <= 1e-12f ||
                    normal0.Dot(normal1) <= 0.0f)
                {
                    return fail("The bridge would contain collapsed or folded faces. Reposition the selections.");
                }
                // Do not use AddPolygon's duplicate-vertex fallback: a bridge must actually
                // join its neighbors. Discard the candidate if OpenMesh refuses either triangle.
                const auto newPolygon = AddQuadWithoutFallback(*candidate, quad);
                if (newPolygon.m_faceHandles.size() != 2)
                {
                    return fail("The bridge would create non-manifold geometry. Choose another pair.");
                }
                for (const auto face : newPolygon.m_faceHandles)
                {
                    addedFaces.push_back(face);
                    CopyFaceAttributes(*candidate, face, whiteBox, sourceFace);
                }
            }
            CalculateNormals(*candidate);
            CalculatePlanarUVs(*candidate, addedFaces);
            RemoveIsolatedVertices(*candidate);
            whiteBox.mesh = candidate->mesh;
            return true;
        }

        // The outline is scaled to unit extent before clipping, so both thresholds are absolute.
        static constexpr float EarMinimumTurn = 1e-10f;
        static constexpr float PlanarTolerance = 1e-3f;

        // Ear clipping in the hole's plane; a self-crossing outline runs out of ears and is reported.
        static bool TriangulateLoop(
            const AZStd::vector<AZ::Vector2>& points, AZStd::vector<AZStd::array<size_t, 3>>& triangles)
        {
            const size_t count = points.size();
            if (count < 3)
            {
                return false;
            }
            const auto turn = [&points](const size_t a, const size_t b, const size_t c)
            {
                const auto edge0 = points[b] - points[a];
                const auto edge1 = points[c] - points[a];
                return edge0.GetX() * edge1.GetY() - edge0.GetY() * edge1.GetX();
            };

            AZStd::vector<size_t> remaining;
            remaining.reserve(count);
            for (size_t i = 0; i < count; ++i)
            {
                remaining.push_back(i);
            }
            triangles.clear();
            triangles.reserve(count - 2);
            const auto corner = [&remaining](const size_t i)
            {
                return AZStd::array<size_t, 3>{{remaining[(i + remaining.size() - 1) % remaining.size()],
                                                remaining[i], remaining[(i + 1) % remaining.size()]}};
            };
            while (remaining.size() > 3)
            {
                size_t chosen = remaining.size();
                float chosenQuality = -1.0f;
                for (size_t i = 0; i < remaining.size(); ++i)
                {
                    const auto ear = corner(i);
                    const float area = turn(ear[0], ear[1], ear[2]);
                    if (area <= EarMinimumTurn)
                    {
                        continue; // a reflex or collinear corner is never an ear
                    }
                    // Only a corner with no other corner inside it can be cut off. Corners exactly
                    // on the candidate's edges count as inside, so an ear never spans the border.
                    bool occupied = false;
                    for (const size_t other : remaining)
                    {
                        if (other == ear[0] || other == ear[1] || other == ear[2])
                        {
                            continue;
                        }
                        if (turn(ear[0], ear[1], other) >= 0.0f && turn(ear[1], ear[2], other) >= 0.0f &&
                            turn(ear[2], ear[0], other) >= 0.0f)
                        {
                            occupied = true;
                            break;
                        }
                    }
                    if (occupied)
                    {
                        continue;
                    }
                    // Take the squattest ear: corners on a straight border can never be ears, and greedy
                    // clipping would strand them in a zero-area sliver.
                    const float spread = (points[ear[1]] - points[ear[0]]).GetLengthSq() +
                        (points[ear[2]] - points[ear[1]]).GetLengthSq() +
                        (points[ear[0]] - points[ear[2]]).GetLengthSq();
                    if (const float quality = area / spread; quality > chosenQuality)
                    {
                        chosenQuality = quality;
                        chosen = i;
                    }
                }
                if (chosen == remaining.size())
                {
                    return false;
                }
                triangles.push_back(corner(chosen));
                remaining.erase(remaining.begin() + static_cast<ptrdiff_t>(chosen));
            }
            triangles.push_back({{remaining[0], remaining[1], remaining[2]}});
            return true;
        }

        bool FillHole(WhiteBoxMesh& whiteBox, const EdgeHandles& edges, AZStd::string& error, PolygonHandle* filled)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            error.clear();
            const auto fail = [&error](const char* message)
            {
                error = message;
                return false;
            };
            if (edges.empty())
            {
                return fail("Select an edge on the hole's border in Transform mode.");
            }

            const auto allEdges = MeshEdgeHandles(whiteBox);
            for (const auto edge : edges)
            {
                if (!edge.IsValid() || AZStd::find(allEdges.begin(), allEdges.end(), edge) == allEdges.end())
                {
                    return fail("The selection is stale. Select the hole's border edge again.");
                }
                // Exactly one face, so a loose edge is turned away here rather than later, when the
                // face to take the material from is read.
                if (const auto faces = EdgeFaceHandles(whiteBox, edge); faces.size() != 1)
                {
                    return fail(
                        faces.empty()
                            ? "The selection includes a loose edge with no face on either side of it."
                            : "Fill Hole needs the open border of a hole. One of the selected edges already has a "
                              "face on both sides.");
                }
            }

            // In OpenMesh the face-less halfedge of a border edge is linked into its own hole, so
            // following it around returns the border in the direction the filling faces need.
            HalfedgeHandle start;
            for (const auto side : {EdgeHalfedge::First, EdgeHalfedge::Second})
            {
                const auto halfedge = EdgeHalfedgeHandle(whiteBox, edges[0], side);
                if (halfedge.IsValid() && HalfedgeIsBoundary(whiteBox, halfedge))
                {
                    start = halfedge;
                    break;
                }
            }
            if (!start.IsValid())
            {
                return fail("Could not find the open side of the selected edge.");
            }

            VertexHandles border;
            EdgeHandles borderEdges;
            for (auto halfedge = start;;)
            {
                border.push_back(HalfedgeVertexHandleAtTail(whiteBox, halfedge));
                borderEdges.push_back(HalfedgeEdgeHandle(whiteBox, halfedge));
                halfedge = HalfedgeHandleNext(whiteBox, halfedge);
                if (!halfedge.IsValid() || border.size() > allEdges.size())
                {
                    return fail("The border around the selected edge does not close.");
                }
                if (halfedge == start)
                {
                    break;
                }
            }
            if (border.size() < 3)
            {
                return fail("A hole needs at least three border edges before it can be filled.");
            }
            for (const auto edge : edges)
            {
                if (AZStd::find(borderEdges.begin(), borderEdges.end(), edge) == borderEdges.end())
                {
                    return fail("The selected edges border more than one hole. Select edges from a single hole.");
                }
            }
            // A border that visits a vertex twice pinches there; filling it would leave that vertex
            // non-manifold and OpenMesh would refuse the faces part way through.
            for (size_t i = 0; i < border.size(); ++i)
            {
                for (size_t j = i + 1; j < border.size(); ++j)
                {
                    if (border[i] == border[j])
                    {
                        return fail(
                            "The hole's border touches itself at a vertex. Separate it into single holes first.");
                    }
                }
            }

            AZStd::vector<AZ::Vector3> positions;
            positions.reserve(border.size());
            AZ::Vector3 centre = AZ::Vector3::CreateZero();
            for (const auto vertex : border)
            {
                positions.push_back(VertexPosition(whiteBox, vertex));
                centre += positions.back();
            }
            centre /= static_cast<float>(positions.size());

            // Newell's normal is the area-weighted plane of the whole border rather than of any
            // three corners, so a hole is not judged flat or not by its worst corner alone.
            AZ::Vector3 normal = AZ::Vector3::CreateZero();
            float extent = 0.0f;
            for (size_t i = 0; i < positions.size(); ++i)
            {
                const auto current = positions[i] - centre;
                normal += current.Cross(positions[(i + 1) % positions.size()] - centre);
                extent = AZ::GetMax(extent, current.GetLength());
            }
            if (extent <= AZ::Constants::FloatEpsilon || normal.GetLengthSq() <= 1e-20f)
            {
                return fail("The hole's border is collapsed and encloses no area to fill.");
            }
            normal.Normalize();

            // Flatness is measured against the hole's own size, so a small hole is held to the same
            // shape tolerance as a large one.
            for (const auto& position : positions)
            {
                if (AZStd::abs((position - centre).Dot(normal)) > extent * PlanarTolerance)
                {
                    return fail(
                        "The hole's border is not planar. Fill Hole needs a flat border - flatten the vertices, or "
                        "use Bridge to close it in steps.");
                }
            }

            // u, v and the normal form a right handed basis, so the border stays wound the same way
            // in 2d as it is around the normal and the clipped triangles need no reordering.
            const AZ::Vector3 reference =
                AZStd::abs(normal.GetZ()) < 0.9f ? AZ::Vector3::CreateAxisZ() : AZ::Vector3::CreateAxisX();
            const AZ::Vector3 uAxis = normal.Cross(reference).GetNormalized();
            const AZ::Vector3 vAxis = normal.Cross(uAxis);
            AZStd::vector<AZ::Vector2> points;
            points.reserve(positions.size());
            for (const auto& position : positions)
            {
                const auto offset = (position - centre) / extent;
                points.emplace_back(offset.Dot(uAxis), offset.Dot(vAxis));
            }

            AZStd::vector<AZStd::array<size_t, 3>> triangles;
            if (!TriangulateLoop(points, triangles))
            {
                return fail(
                    "The hole's outline could not be filled. It crosses itself or doubles back; simplify the border "
                    "and try again.");
            }

            const auto sourceFace = EdgeFaceHandles(whiteBox, edges[0]).front();
            auto candidate = CloneMesh(whiteBox);
            FaceHandlesInternal faces;
            faces.reserve(triangles.size());
            for (const auto& triangle : triangles)
            {
                // Border order follows the hole's halfedges, so these wind with their neighbours. Not
                // AddPolygon: its duplicate-vertex fallback would leave a loose patch.
                const auto face = candidate->mesh.add_face(
                    om_vh(border[triangle[0]]), om_vh(border[triangle[1]]), om_vh(border[triangle[2]]));
                if (!face.is_valid())
                {
                    return fail(
                        "Filling this hole would create non-manifold geometry. Check the border for faces that meet "
                        "at a single vertex.");
                }
                faces.push_back(face);
            }

            PolygonPropertyHandle property;
            candidate->mesh.get_property_handle(property, PolygonProps);
            auto& groups = candidate->mesh.property(property);
            FaceHandles added;
            added.reserve(faces.size());
            for (const auto face : faces)
            {
                groups[face] = faces; // one polygon, so the fill selects and textures as a single face
                added.push_back(wb_fh(face));
                CopyFaceAttributes(*candidate, added.back(), whiteBox, sourceFace);
            }
            CalculateNormals(*candidate);
            CalculatePlanarUVs(*candidate, added);
            whiteBox.mesh = candidate->mesh;
            if (filled != nullptr)
            {
                *filled = PolygonHandleFromInternal(faces);
            }
            return true;
        }

        bool DeleteFaces(WhiteBoxMesh& whiteBox, const FaceHandles& selection, AZStd::string& error)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            error.clear();
            const auto fail = [&error](const char* message)
            {
                error = message;
                return false;
            };
            if (selection.empty())
            {
                return fail("Select what to delete in Transform mode first.");
            }

            const auto allFaces = MeshFaceHandles(whiteBox);
            FaceHandles faces;
            faces.reserve(selection.size());
            for (const auto face : selection)
            {
                if (!face.IsValid() || AZStd::find(allFaces.begin(), allFaces.end(), face) == allFaces.end())
                {
                    return fail("The selection is stale. Make the selection again.");
                }
                // A polygon selection can name the same triangle twice when two of them overlap.
                if (AZStd::find(faces.begin(), faces.end(), face) == faces.end())
                {
                    faces.push_back(face);
                }
            }
            if (faces.size() >= allFaces.size())
            {
                return fail(
                    "That would delete every face and leave only loose vertices. Keep at least one, or reset the "
                    "White Box mesh instead.");
            }

            auto candidate = CloneMesh(whiteBox);
            // RemoveFaces keeps the vertices and drops only the edges that are left without a face,
            // which is what Blender's delete-faces does and what makes the opening refillable.
            RemoveFaces(*candidate, faces);
            CalculateNormals(*candidate);
            whiteBox.mesh = candidate->mesh;
            return true;
        }

        bool DeletePolygons(WhiteBoxMesh& whiteBox, const PolygonHandles& polygons, AZStd::string& error)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            error.clear();
            if (polygons.empty())
            {
                error = "Select one or more polygons in Transform mode.";
                return false;
            }

            const auto allPolygons = MeshPolygonHandles(whiteBox);
            FaceHandles faces;
            for (const auto& polygon : polygons)
            {
                if (polygon.m_faceHandles.empty() ||
                    AZStd::find(allPolygons.begin(), allPolygons.end(), polygon) == allPolygons.end())
                {
                    error = "The selection is stale. Select the polygons again.";
                    return false;
                }
                faces.insert(faces.end(), polygon.m_faceHandles.begin(), polygon.m_faceHandles.end());
            }
            return DeleteFaces(whiteBox, faces, error);
        }

        PolygonHandles FindCoplanarRegion(
            const WhiteBoxMesh& whiteBox, const PolygonHandles& seeds, const float toleranceDegrees)
        {
            const auto allPolygons = MeshPolygonHandles(whiteBox);
            PolygonHandles region;
            for (const auto& seed : seeds)
            {
                if (!seed.m_faceHandles.empty() &&
                    AZStd::find(allPolygons.begin(), allPolygons.end(), seed) != allPolygons.end() &&
                    AZStd::find(region.begin(), region.end(), seed) == region.end())
                {
                    region.push_back(seed);
                }
            }
            if (region.empty())
            {
                return region;
            }

            const float limit = cosf(AZ::DegToRad(AZ::GetMax(toleranceDegrees, 0.0f)));
            // Compared against each region member, not the previous step, so a curve cannot creep away.
            for (size_t i = 0; i < region.size(); ++i)
            {
                const auto normal = PolygonNormal(whiteBox, region[i]);
                for (const auto edge : PolygonBorderEdgeHandlesFlattened(whiteBox, region[i]))
                {
                    for (const auto face : EdgeFaceHandles(whiteBox, edge))
                    {
                        const auto neighbour = FacePolygonHandle(whiteBox, face);
                        if (neighbour.m_faceHandles.empty() ||
                            AZStd::find(region.begin(), region.end(), neighbour) != region.end())
                        {
                            continue;
                        }
                        if (PolygonNormal(whiteBox, neighbour).Dot(normal) >= limit)
                        {
                            region.push_back(neighbour);
                        }
                    }
                }
            }
            return region;
        }

        // Breadth first across shared edges only, so shells touching at one vertex stay separate.
        static FaceHandles FloodLinkedFaces(const WhiteBoxMesh& whiteBox, const FaceHandles& seeds)
        {
            AZStd::vector<bool> visited(whiteBox.mesh.n_faces(), false);
            FaceHandles linked;
            const auto push = [&visited, &linked](const FaceHandle face)
            {
                if (!face.IsValid() || static_cast<size_t>(face.Index()) >= visited.size() ||
                    visited[static_cast<size_t>(face.Index())])
                {
                    return;
                }
                visited[static_cast<size_t>(face.Index())] = true;
                linked.push_back(face);
            };
            for (const auto face : seeds)
            {
                push(face);
            }
            // linked doubles as the queue, so indexing rather than iterating keeps it valid as it grows.
            for (size_t i = 0; i < linked.size(); ++i)
            {
                for (const auto halfedge : FaceHalfedgeHandles(whiteBox, linked[i]))
                {
                    push(HalfedgeOppositeFaceHandle(whiteBox, halfedge));
                }
            }
            return linked;
        }

        PolygonHandles FindLinkedPolygons(const WhiteBoxMesh& whiteBox, const PolygonHandles& seeds)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            FaceHandles seedFaces;
            for (const auto& polygon : seeds)
            {
                seedFaces.insert(seedFaces.end(), polygon.m_faceHandles.begin(), polygon.m_faceHandles.end());
            }

            AZStd::vector<bool> claimed(whiteBox.mesh.n_faces(), false);
            PolygonHandles linked;
            for (const auto face : FloodLinkedFaces(whiteBox, seedFaces))
            {
                if (claimed[static_cast<size_t>(face.Index())])
                {
                    continue;
                }
                const auto polygon = FacePolygonHandle(whiteBox, face);
                if (polygon.m_faceHandles.empty())
                {
                    continue;
                }
                // Claim the whole group at once, so a polygon is never added twice.
                for (const auto member : polygon.m_faceHandles)
                {
                    claimed[static_cast<size_t>(member.Index())] = true;
                }
                linked.push_back(polygon);
            }
            return linked;
        }

        EdgeHandles FindLinkedEdges(const WhiteBoxMesh& whiteBox, const EdgeHandles& seeds)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            FaceHandles seedFaces;
            for (const auto edge : seeds)
            {
                const auto faces = EdgeFaceHandles(whiteBox, edge);
                seedFaces.insert(seedFaces.end(), faces.begin(), faces.end());
            }

            AZStd::vector<bool> inside(whiteBox.mesh.n_faces(), false);
            for (const auto face : FloodLinkedFaces(whiteBox, seedFaces))
            {
                inside[static_cast<size_t>(face.Index())] = true;
            }
            // Only the borders a click could have picked; internal triangulation edges are not offered.
            EdgeHandles linked;
            for (const auto edge : MeshPolygonEdgeHandles(whiteBox))
            {
                for (const auto face : EdgeFaceHandles(whiteBox, edge))
                {
                    if (inside[static_cast<size_t>(face.Index())])
                    {
                        linked.push_back(edge);
                        break;
                    }
                }
            }
            return linked;
        }

        VertexHandles FindLinkedVertices(const WhiteBoxMesh& whiteBox, const VertexHandles& seeds)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            FaceHandles seedFaces;
            for (const auto vertex : seeds)
            {
                for (const auto edge : VertexEdgeHandles(whiteBox, vertex))
                {
                    const auto faces = EdgeFaceHandles(whiteBox, edge);
                    seedFaces.insert(seedFaces.end(), faces.begin(), faces.end());
                }
            }

            AZStd::vector<bool> taken(whiteBox.mesh.n_vertices(), false);
            VertexHandles linked;
            for (const auto face : FloodLinkedFaces(whiteBox, seedFaces))
            {
                for (const auto vertex : FaceVertexHandles(whiteBox, face))
                {
                    const size_t index = static_cast<size_t>(vertex.Index());
                    // Hidden corners cannot be clicked, so linking must not hand them back either.
                    if (!taken[index] && !VertexIsHidden(whiteBox, vertex))
                    {
                        taken[index] = true;
                        linked.push_back(vertex);
                    }
                }
            }
            return linked;
        }

        bool MergePolygons(
            WhiteBoxMesh& whiteBox, const PolygonHandles& polygons, AZStd::string& error, PolygonHandle* merged)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            error.clear();
            const auto fail = [&error](const char* message)
            {
                error = message;
                return false;
            };
            if (polygons.size() < 2)
            {
                return fail("Select two or more polygons that share an edge.");
            }

            const auto allPolygons = MeshPolygonHandles(whiteBox);
            FaceHandles selected;
            for (const auto& polygon : polygons)
            {
                if (polygon.m_faceHandles.empty() ||
                    AZStd::find(allPolygons.begin(), allPolygons.end(), polygon) == allPolygons.end())
                {
                    return fail("The selection is stale. Select the polygons again.");
                }
                for (const auto face : polygon.m_faceHandles)
                {
                    if (AZStd::find(selected.begin(), selected.end(), face) == selected.end())
                    {
                        selected.push_back(face);
                    }
                }
            }

            // Hiding an edge leaves it in the mesh as an internal one, so these handles stay valid.
            const auto isSelected = [&selected](const FaceHandle face)
            {
                return AZStd::find(selected.begin(), selected.end(), face) != selected.end();
            };
            EdgeHandles seams;
            for (const auto edge : MeshPolygonEdgeHandles(whiteBox))
            {
                const auto faces = EdgeFaceHandles(whiteBox, edge);
                if (faces.size() == 2 && isSelected(faces[0]) && isSelected(faces[1]) &&
                    FacePolygonHandle(whiteBox, faces[0]) != FacePolygonHandle(whiteBox, faces[1]))
                {
                    seams.push_back(edge);
                }
            }
            if (seams.empty())
            {
                return fail("Those polygons do not touch. Select polygons that share an edge.");
            }

            auto candidate = CloneMesh(whiteBox);
            for (const auto edge : seams)
            {
                HideEdge(*candidate, edge);
            }
            // Read the group off the mesh: the last hide says nothing useful for an already-merged seam.
            const auto result = FacePolygonHandle(*candidate, selected.front());
            if (result.m_faceHandles.size() != selected.size())
            {
                return fail("Those polygons are not all joined. Select one region that connects edge to edge.");
            }
            CalculateNormals(*candidate);
            whiteBox.mesh = candidate->mesh;
            if (merged != nullptr)
            {
                *merged = result;
            }
            return true;
        }

        bool WeldVertices(
            WhiteBoxMesh& whiteBox, const VertexHandles& vertices, const bool toLastSelected, AZStd::string& error)
        {
            error.clear();
            const auto fail = [&error](const char* message) { error = message; return false; };
            const auto originalVertices = MeshVertexHandles(whiteBox);
            AZStd::unordered_set<int> selected;
            AZ::Vector3 target = AZ::Vector3::CreateZero();
            for (const auto vertex : vertices)
            {
                if (AZStd::find(originalVertices.begin(), originalVertices.end(), vertex) == originalVertices.end())
                {
                    return fail("The vertex selection is stale. Select the vertices again.");
                }
                if (selected.insert(vertex.Index()).second) { target += VertexPosition(whiteBox, vertex); }
            }
            if (selected.size() < 2) { return fail("Ctrl-select at least two vertices in Transform mode."); }
            target = toLastSelected ? VertexPosition(whiteBox, vertices.back()) : target / static_cast<float>(selected.size());

            // Tell an intended corner collapse (open patches meeting at the merged vertex) from joining unrelated surfaces.
            AZStd::unordered_set<int> connectedSelection;
            VertexHandles selectedPending{vertices.front()};
            while (!selectedPending.empty())
            {
                const auto vertex = selectedPending.back();
                selectedPending.pop_back();
                if (!connectedSelection.insert(vertex.Index()).second) { continue; }
                for (const auto edge : VertexEdgeHandles(whiteBox, vertex))
                {
                    for (const auto neighbor : EdgeVertexHandles(whiteBox, edge))
                    {
                        if (selected.count(neighbor.Index()) && !connectedSelection.count(neighbor.Index()))
                        {
                            selectedPending.push_back(neighbor);
                        }
                    }
                }
            }
            const bool collapsingConnectedSelection = connectedSelection.size() == selected.size();

            auto candidate = CreateWhiteBoxMesh();
            const auto welded = AddVertex(*candidate, target);
            AZStd::vector<VertexHandle> remap(whiteBox.mesh.n_vertices());
            for (const auto vertex : originalVertices)
            {
                if (selected.count(vertex.Index()))
                {
                    remap[vertex.Index()] = welded;
                }
                else
                {
                    const auto added = AddVertex(*candidate, VertexPosition(whiteBox, vertex));
                    remap[vertex.Index()] = added;
                    if (VertexIsHidden(whiteBox, vertex)) { HideVertex(*candidate, added); }
                    CopyVertexBlend(*candidate, added, whiteBox, vertex);
                }
            }
            const auto polygons = MeshPolygonHandles(whiteBox);
            AZStd::vector<size_t> sourceGroups(whiteBox.mesh.n_faces(), polygons.size());
            for (size_t group = 0; group < polygons.size(); ++group)
            {
                for (const auto face : polygons[group].m_faceHandles) { sourceGroups[face.Index()] = group; }
            }
            struct WeldFace
            {
                AZStd::array<VertexHandle, 3> m_vertices;
                AZStd::array<AZ::Vector2, 3> m_uvs;
                FaceHandle m_source;
                FaceHandle m_result;
                size_t m_group;
            };
            AZStd::vector<WeldFace> faces;
            using EdgeKey = AZStd::pair<int, int>;
            const auto edgeKey = [](const VertexHandle a, const VertexHandle b)
            {
                return EdgeKey{AZStd::min(a.Index(), b.Index()), AZStd::max(a.Index(), b.Index())};
            };
            AZStd::map<EdgeKey, AZStd::vector<size_t>> edgeFaces;
            AZStd::map<AZStd::pair<int, EdgeKey>, bool> uniqueTriangles;
            AZStd::vector<AZStd::vector<size_t>> vertexFaces(candidate->mesh.n_vertices());
            for (const auto source : MeshFaceHandles(whiteBox))
            {
                WeldFace face{};
                face.m_source = source;
                face.m_group = sourceGroups[source.Index()];
                if (face.m_group == polygons.size()) { return fail("Repair the mesh's polygon groups before welding."); }
                const auto halfedges = FaceHalfedgeHandles(whiteBox, source);
                if (halfedges.size() != 3) { return fail("Weld requires valid triangle faces."); }
                AZStd::array<AZ::Vector3, 3> before;
                AZStd::array<AZ::Vector3, 3> after;
                for (size_t i = 0; i < 3; ++i)
                {
                    const auto original = HalfedgeVertexHandleAtTip(whiteBox, halfedges[i]);
                    face.m_vertices[i] = remap[original.Index()];
                    face.m_uvs[i] = HalfedgeUV(whiteBox, halfedges[i]);
                    before[i] = VertexPosition(whiteBox, original);
                    after[i] = VertexPosition(*candidate, face.m_vertices[i]);
                }
                const auto& v = face.m_vertices;
                if (v[0] == v[1] || v[1] == v[2] || v[2] == v[0]) { continue; }
                const auto oldNormal = (before[1] - before[0]).Cross(before[2] - before[0]);
                const auto newNormal = (after[1] - after[0]).Cross(after[2] - after[0]);
                if (newNormal.GetLengthSq() <= 1e-12f || oldNormal.Dot(newNormal) <= 0.0f)
                {
                    return fail("Weld would flatten or reverse a surviving face. Try another target or selection.");
                }
                AZStd::array<int, 3> sorted{{v[0].Index(), v[1].Index(), v[2].Index()}};
                AZStd::sort(sorted.begin(), sorted.end());
                if (!uniqueTriangles.emplace(AZStd::make_pair(sorted[0], EdgeKey{sorted[1], sorted[2]}), true).second)
                {
                    return fail("Weld would create duplicate faces. Choose a smaller selection.");
                }
                const size_t index = faces.size();
                faces.push_back(face);
                for (size_t i = 0; i < 3; ++i)
                {
                    auto& incident = edgeFaces[edgeKey(v[i], v[(i + 1) % 3])];
                    incident.push_back(index);
                    if (incident.size() > 2) { return fail("Weld would create an edge with more than two faces."); }
                    vertexFaces[v[i].Index()].push_back(index);
                }
            }
            if (faces.empty() || vertexFaces[welded.Index()].empty())
            {
                return fail("Weld would remove all faces around the selection.");
            }
            // One connected face fan per vertex, except separate open patches at the merged point.
            for (size_t vertex = 0; vertex < vertexFaces.size(); ++vertex)
            {
                const auto& incident = vertexFaces[vertex];
                if (incident.empty()) { continue; }
                AZStd::unordered_set<size_t> visited;
                AZStd::vector<size_t> pending{incident.front()};
                while (!pending.empty())
                {
                    const auto index = pending.back();
                    pending.pop_back();
                    if (!visited.insert(index).second) { continue; }
                    const auto& v = faces[index].m_vertices;
                    for (size_t i = 0; i < 3; ++i)
                    {
                        if (static_cast<size_t>(v[i].Index()) == vertex ||
                            static_cast<size_t>(v[(i + 1) % 3].Index()) == vertex)
                        {
                            for (const auto next : edgeFaces[edgeKey(v[i], v[(i + 1) % 3])])
                            {
                                if (!visited.count(next)) { pending.push_back(next); }
                            }
                        }
                    }
                }
                if (visited.size() != incident.size() &&
                    !(vertex == static_cast<size_t>(welded.Index()) && collapsingConnectedSelection))
                {
                    return fail("Weld would join separate surfaces at only a point. Select vertices within a connected surface.");
                }
            }
            for (auto& face : faces)
            {
                const auto& v = face.m_vertices;
                const auto added = candidate->mesh.add_face(om_vh(v[0]), om_vh(v[1]), om_vh(v[2]));
                if (!added.is_valid()) { return fail("Weld would create invalid face connections. The mesh was left unchanged."); }
                face.m_result = wb_fh(added);
                CopyFaceAttributes(*candidate, face.m_result, whiteBox, face.m_source);
                for (size_t i = 0; i < 3; ++i)
                {
                    const auto halfedge = candidate->mesh.find_halfedge(om_vh(v[(i + 2) % 3]), om_vh(v[i]));
                    candidate->mesh.set_texcoord2D(halfedge, face.m_uvs[i]);
                }
            }
            // Keep original polygon boundaries, splitting a group if deleted triangles
            // separated it into distinct patches.
            PolygonPropertyHandle property;
            candidate->mesh.get_property_handle(property, PolygonProps);
            auto& groups = candidate->mesh.property(property);
            AZStd::unordered_set<size_t> grouped;
            for (size_t start = 0; start < faces.size(); ++start)
            {
                if (grouped.count(start)) { continue; }
                FaceHandlesInternal group;
                AZStd::vector<size_t> pending{start};
                while (!pending.empty())
                {
                    const auto index = pending.back();
                    pending.pop_back();
                    if (!grouped.insert(index).second) { continue; }
                    group.push_back(om_fh(faces[index].m_result));
                    const auto& v = faces[index].m_vertices;
                    for (size_t i = 0; i < 3; ++i)
                    {
                        for (const auto next : edgeFaces[edgeKey(v[i], v[(i + 1) % 3])])
                        {
                            if (faces[next].m_group == faces[start].m_group && !grouped.count(next)) { pending.push_back(next); }
                        }
                    }
                }
                for (const auto face : group) { groups[face] = group; }
            }
            CalculateNormals(*candidate);
            whiteBox.mesh = candidate->mesh;
            return true;
        }

        static EdgeHandles FindEdgePattern(const WhiteBoxMesh& whiteBox, const EdgeHandles& seeds, bool ring)
        {
            struct SelectionPolygon
            {
                EdgeHandles m_edges;
                bool m_quad = false;
            };
            AZStd::vector<SelectionPolygon> polygons;
            AZStd::map<int, AZStd::vector<size_t>> edgePolygons;
            AZStd::map<int, EdgeHandles> vertexEdges;
            // Only real polygon boundaries participate. Internal triangulation
            // diagonals must not affect either valence or traversal direction.
            for (const auto& polygon : MeshPolygonHandles(whiteBox))
            {
                const auto borders = PolygonBorderHalfedgeHandles(whiteBox, polygon);
                SelectionPolygon data;
                data.m_quad = borders.size() == 1 && borders.front().size() == 4;
                const size_t index = polygons.size();
                for (const auto& border : borders)
                {
                    for (const auto halfedge : border)
                    {
                        const auto edge = HalfedgeEdgeHandle(whiteBox, halfedge);
                        data.m_edges.push_back(edge);
                        auto& adjacent = edgePolygons[edge.Index()];
                        if (AZStd::find(adjacent.begin(), adjacent.end(), index) == adjacent.end()) { adjacent.push_back(index); }
                        if (adjacent.size() == 1)
                        {
                            for (const auto vertex : EdgeVertexHandles(whiteBox, edge))
                            {
                                auto& incident = vertexEdges[vertex.Index()];
                                if (AZStd::find(incident.begin(), incident.end(), edge) == incident.end()) { incident.push_back(edge); }
                            }
                        }
                    }
                }
                polygons.push_back(AZStd::move(data));
            }
            EdgeHandles result;
            AZStd::unordered_set<int> seen;
            const auto append = [&](EdgeHandle edge)
            {
                if (edgePolygons.count(edge.Index()) && seen.insert(edge.Index()).second) { result.push_back(edge); }
            };
            for (const auto seed : seeds) { append(seed); }
            for (size_t cursor = 0; cursor < result.size(); ++cursor)
            {
                const auto edge = result[cursor];
                const auto& faces = edgePolygons.find(edge.Index())->second;
                if (faces.empty() || faces.size() > 2) { continue; }
                if (ring)
                {
                    for (const auto index : faces)
                    {
                        const auto& polygon = polygons[index];
                        if (!polygon.m_quad) { continue; }
                        const auto found = AZStd::find(polygon.m_edges.begin(), polygon.m_edges.end(), edge);
                        const size_t offset = static_cast<size_t>(found - polygon.m_edges.begin());
                        append(polygon.m_edges[(offset + 2) % 4]);
                    }
                    continue;
                }
                for (const auto vertex : EdgeVertexHandles(whiteBox, edge))
                {
                    const auto& incident = vertexEdges.find(vertex.Index())->second;
                    EdgeHandle next;
                    bool ambiguous = false;
                    if (faces.size() == 1)
                    {
                        // An open border is its own loop, including its corners.
                        for (const auto candidate : incident)
                        {
                            if (candidate != edge && edgePolygons.find(candidate.Index())->second.size() == 1)
                            {
                                if (next.IsValid()) { ambiguous = true; break; }
                                next = candidate;
                            }
                        }
                    }
                    else if (incident.size() == 4)
                    {
                        // At a regular quad junction, the continuation is the
                        // sole edge that shares neither adjacent polygon.
                        bool regular = true;
                        AZStd::unordered_set<size_t> vertexPolygons;
                        for (const auto candidate : incident)
                        {
                            const auto& adjacent = edgePolygons.find(candidate.Index())->second;
                            if (adjacent.size() != 2) { regular = false; break; }
                            bool sharesFace = false;
                            for (const auto index : adjacent)
                            {
                                vertexPolygons.insert(index);
                                regular = regular && polygons[index].m_quad;
                                sharesFace = sharesFace || AZStd::find(faces.begin(), faces.end(), index) != faces.end();
                            }
                            if (!sharesFace)
                            {
                                if (next.IsValid()) { ambiguous = true; }
                                next = candidate;
                            }
                        }
                        ambiguous = ambiguous || !regular || vertexPolygons.size() != 4;
                    }
                    if (!ambiguous && next.IsValid()) { append(next); }
                }
            }
            return result;
        }

        EdgeHandles FindEdgeLoop(const WhiteBoxMesh& whiteBox, const EdgeHandles& seeds)
        {
            return FindEdgePattern(whiteBox, seeds, false);
        }

        EdgeHandles FindEdgeRing(const WhiteBoxMesh& whiteBox, const EdgeHandles& seeds)
        {
            return FindEdgePattern(whiteBox, seeds, true);
        }

        // Faces sit between the edges of a ring, so the ring walk decides where the strip stops.
        static PolygonHandles FindPolygonStrip(
            const WhiteBoxMesh& whiteBox, const PolygonHandles& seeds, const bool perpendicular)
        {
            PolygonHandles strip;
            const auto append = [&strip](const PolygonHandle& polygon)
            {
                if (!polygon.m_faceHandles.empty() &&
                    AZStd::find(strip.begin(), strip.end(), polygon) == strip.end())
                {
                    strip.push_back(polygon);
                }
            };
            for (const auto& seed : seeds)
            {
                append(seed);
                const auto borders = PolygonBorderEdgeHandles(whiteBox, seed);
                // Only a plain quad has a strip through it; anything else just keeps its seed.
                if (borders.size() != 1 || borders.front().size() != 4)
                {
                    continue;
                }
                for (const auto edge : FindEdgeRing(whiteBox, {borders.front()[perpendicular ? 1 : 0]}))
                {
                    for (const auto face : EdgeFaceHandles(whiteBox, edge))
                    {
                        append(FacePolygonHandle(whiteBox, face));
                    }
                }
            }
            return strip;
        }

        PolygonHandles FindPolygonLoop(const WhiteBoxMesh& whiteBox, const PolygonHandles& seeds)
        {
            return FindPolygonStrip(whiteBox, seeds, false);
        }

        PolygonHandles FindPolygonRing(const WhiteBoxMesh& whiteBox, const PolygonHandles& seeds)
        {
            return FindPolygonStrip(whiteBox, seeds, true);
        }

        // Polygon index of every face, so adjacency questions become array lookups.
        static AZStd::vector<int> FacePolygonIndices(const WhiteBoxMesh& whiteBox, const PolygonHandles& polygons)
        {
            AZStd::vector<int> owner(whiteBox.mesh.n_faces(), -1);
            for (size_t i = 0; i < polygons.size(); ++i)
            {
                for (const auto face : polygons[i].m_faceHandles)
                {
                    if (face.IsValid() && static_cast<size_t>(face.Index()) < owner.size())
                    {
                        owner[face.Index()] = static_cast<int>(i);
                    }
                }
            }
            return owner;
        }

        // Index of a seed among the mesh's polygons, or -1 when the seed is stale.
        static int PolygonIndex(const PolygonHandles& polygons, const AZStd::vector<int>& owner, const PolygonHandle& seed)
        {
            if (seed.m_faceHandles.empty() || !seed.m_faceHandles.front().IsValid() ||
                static_cast<size_t>(seed.m_faceHandles.front().Index()) >= owner.size())
            {
                return -1;
            }
            const int index = owner[seed.m_faceHandles.front().Index()];
            return index >= 0 && polygons[index] == seed ? index : -1;
        }

        // Seeds first in their own order, then every other marked polygon in mesh order.
        static PolygonHandles MarkedPolygons(
            const PolygonHandles& polygons, const AZStd::vector<int>& owner, const PolygonHandles& seeds,
            const AZStd::vector<bool>& marked)
        {
            PolygonHandles result;
            AZStd::vector<bool> taken(polygons.size(), false);
            for (const auto& seed : seeds)
            {
                const int index = PolygonIndex(polygons, owner, seed);
                if (index >= 0 && marked[index] && !taken[index])
                {
                    taken[index] = true;
                    result.push_back(polygons[index]);
                }
            }
            for (size_t i = 0; i < polygons.size(); ++i)
            {
                if (marked[i] && !taken[i])
                {
                    result.push_back(polygons[i]);
                }
            }
            return result;
        }

        // Calls visit with the index of every polygon sharing a corner with the given one, itself included.
        template<typename Visit>
        static void ForEachCornerNeighbour(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& polygon, const AZStd::vector<int>& owner, Visit&& visit)
        {
            for (const auto face : polygon.m_faceHandles)
            {
                for (const auto vertex : FaceVertexHandles(whiteBox, face))
                {
                    for (const auto halfedge : VertexOutgoingHalfedgeHandles(whiteBox, vertex))
                    {
                        const auto neighbour = HalfedgeFaceHandle(whiteBox, halfedge);
                        if (neighbour.IsValid() && owner[neighbour.Index()] >= 0)
                        {
                            visit(owner[neighbour.Index()]);
                        }
                    }
                }
            }
        }

        PolygonHandles GrowPolygonSelection(const WhiteBoxMesh& whiteBox, const PolygonHandles& seeds)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const auto polygons = MeshPolygonHandles(whiteBox);
            const auto owner = FacePolygonIndices(whiteBox, polygons);
            AZStd::vector<bool> grown(polygons.size(), false);
            for (const auto& seed : seeds)
            {
                if (const int index = PolygonIndex(polygons, owner, seed); index >= 0)
                {
                    grown[index] = true;
                    ForEachCornerNeighbour(
                        whiteBox, polygons[index], owner,
                        [&grown](const int neighbour)
                        {
                            grown[neighbour] = true;
                        });
                }
            }
            return MarkedPolygons(polygons, owner, seeds, grown);
        }

        PolygonHandles ShrinkPolygonSelection(const WhiteBoxMesh& whiteBox, const PolygonHandles& seeds)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const auto polygons = MeshPolygonHandles(whiteBox);
            const auto owner = FacePolygonIndices(whiteBox, polygons);
            AZStd::vector<bool> chosen(polygons.size(), false);
            for (const auto& seed : seeds)
            {
                if (const int index = PolygonIndex(polygons, owner, seed); index >= 0)
                {
                    chosen[index] = true;
                }
            }
            AZStd::vector<bool> kept = chosen;
            for (size_t i = 0; i < polygons.size(); ++i)
            {
                if (chosen[i])
                {
                    ForEachCornerNeighbour(
                        whiteBox, polygons[i], owner,
                        [&chosen, &kept, i](const int neighbour)
                        {
                            if (!chosen[neighbour])
                            {
                                kept[i] = false;
                            }
                        });
                }
            }
            return MarkedPolygons(polygons, owner, seeds, kept);
        }

        // Polygon border edges: the only edges an edge selection can hold.
        static AZStd::vector<bool> PolygonEdgeMask(const WhiteBoxMesh& whiteBox)
        {
            AZStd::vector<bool> mask(whiteBox.mesh.n_edges(), false);
            for (const auto edge : MeshPolygonEdgeHandles(whiteBox))
            {
                mask[edge.Index()] = true;
            }
            return mask;
        }

        static bool EdgeInMask(const AZStd::vector<bool>& mask, const EdgeHandle edge)
        {
            return edge.IsValid() && static_cast<size_t>(edge.Index()) < mask.size() && mask[edge.Index()];
        }

        // Seeds first in their own order, then every other marked element in index order.
        template<typename Handle>
        static AZStd::vector<Handle> MarkedHandles(const AZStd::vector<Handle>& seeds, const AZStd::vector<bool>& marked)
        {
            AZStd::vector<Handle> result;
            AZStd::vector<bool> taken(marked.size(), false);
            for (const auto seed : seeds)
            {
                const size_t index = static_cast<size_t>(seed.Index());
                if (seed.IsValid() && index < marked.size() && marked[index] && !taken[index])
                {
                    taken[index] = true;
                    result.push_back(seed);
                }
            }
            for (size_t i = 0; i < marked.size(); ++i)
            {
                if (marked[i] && !taken[i])
                {
                    result.push_back(Handle{ static_cast<int>(i) });
                }
            }
            return result;
        }

        EdgeHandles GrowEdgeSelection(const WhiteBoxMesh& whiteBox, const EdgeHandles& seeds)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const auto mask = PolygonEdgeMask(whiteBox);
            AZStd::vector<bool> grown(mask.size(), false);
            for (const auto edge : seeds)
            {
                if (!EdgeInMask(mask, edge))
                {
                    continue;
                }
                grown[edge.Index()] = true;
                for (const auto vertex : EdgeVertexHandles(whiteBox, edge))
                {
                    for (const auto neighbour : VertexEdgeHandles(whiteBox, vertex))
                    {
                        if (EdgeInMask(mask, neighbour))
                        {
                            grown[neighbour.Index()] = true;
                        }
                    }
                }
            }
            return MarkedHandles(seeds, grown);
        }

        EdgeHandles ShrinkEdgeSelection(const WhiteBoxMesh& whiteBox, const EdgeHandles& seeds)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const auto mask = PolygonEdgeMask(whiteBox);
            AZStd::vector<bool> chosen(mask.size(), false);
            for (const auto edge : seeds)
            {
                if (EdgeInMask(mask, edge))
                {
                    chosen[edge.Index()] = true;
                }
            }
            AZStd::vector<bool> kept = chosen;
            for (size_t i = 0; i < chosen.size(); ++i)
            {
                if (!chosen[i])
                {
                    continue;
                }
                for (const auto vertex : EdgeVertexHandles(whiteBox, EdgeHandle{ static_cast<int>(i) }))
                {
                    for (const auto neighbour : VertexEdgeHandles(whiteBox, vertex))
                    {
                        if (EdgeInMask(mask, neighbour) && !chosen[neighbour.Index()])
                        {
                            kept[i] = false;
                        }
                    }
                }
            }
            return MarkedHandles(seeds, kept);
        }

        // Cross edge from a vertex and carry on through hidden border vertices to the next visible one.
        static VertexHandle WalkToVisibleVertex(
            const WhiteBoxMesh& whiteBox, const AZStd::vector<bool>& mask, EdgeHandle edge, VertexHandle from)
        {
            for (size_t step = 0; step < mask.size(); ++step)
            {
                const auto ends = EdgeVertexHandles(whiteBox, edge);
                const VertexHandle next = ends[0] == from ? ends[1] : ends[0];
                if (!VertexIsHidden(whiteBox, next))
                {
                    return next;
                }
                // A hidden border vertex sits on a straight run, so exactly one other border edge carries on.
                EdgeHandle onward;
                int count = 0;
                for (const auto candidate : VertexEdgeHandles(whiteBox, next))
                {
                    if (candidate != edge && EdgeInMask(mask, candidate))
                    {
                        onward = candidate;
                        ++count;
                    }
                }
                if (count != 1)
                {
                    return {};
                }
                from = next;
                edge = onward;
            }
            return {};
        }

        static VertexHandles VisibleVertexNeighbours(
            const WhiteBoxMesh& whiteBox, const AZStd::vector<bool>& mask, const VertexHandle vertex)
        {
            VertexHandles neighbours;
            for (const auto edge : VertexEdgeHandles(whiteBox, vertex))
            {
                if (!EdgeInMask(mask, edge))
                {
                    continue;
                }
                const auto neighbour = WalkToVisibleVertex(whiteBox, mask, edge, vertex);
                if (neighbour.IsValid() && neighbour != vertex &&
                    AZStd::find(neighbours.begin(), neighbours.end(), neighbour) == neighbours.end())
                {
                    neighbours.push_back(neighbour);
                }
            }
            return neighbours;
        }

        static bool SelectableVertex(const WhiteBoxMesh& whiteBox, const VertexHandle vertex)
        {
            return vertex.IsValid() && static_cast<size_t>(vertex.Index()) < whiteBox.mesh.n_vertices() &&
                !VertexIsHidden(whiteBox, vertex);
        }

        VertexHandles GrowVertexSelection(const WhiteBoxMesh& whiteBox, const VertexHandles& seeds)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const auto mask = PolygonEdgeMask(whiteBox);
            AZStd::vector<bool> grown(whiteBox.mesh.n_vertices(), false);
            for (const auto vertex : seeds)
            {
                if (!SelectableVertex(whiteBox, vertex))
                {
                    continue;
                }
                grown[vertex.Index()] = true;
                for (const auto neighbour : VisibleVertexNeighbours(whiteBox, mask, vertex))
                {
                    grown[neighbour.Index()] = true;
                }
            }
            return MarkedHandles(seeds, grown);
        }

        VertexHandles ShrinkVertexSelection(const WhiteBoxMesh& whiteBox, const VertexHandles& seeds)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const auto mask = PolygonEdgeMask(whiteBox);
            AZStd::vector<bool> chosen(whiteBox.mesh.n_vertices(), false);
            for (const auto vertex : seeds)
            {
                if (SelectableVertex(whiteBox, vertex))
                {
                    chosen[vertex.Index()] = true;
                }
            }
            AZStd::vector<bool> kept = chosen;
            for (const auto vertex : seeds)
            {
                if (!SelectableVertex(whiteBox, vertex))
                {
                    continue;
                }
                for (const auto neighbour : VisibleVertexNeighbours(whiteBox, mask, vertex))
                {
                    if (!chosen[neighbour.Index()])
                    {
                        kept[vertex.Index()] = false;
                    }
                }
            }
            return MarkedHandles(seeds, kept);
        }

        void AutoSmoothPolygons(WhiteBoxMesh& whiteBox, const PolygonHandles& selection, const float angleDegrees)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const auto polygons = MeshPolygonHandles(whiteBox);
            const auto owner = FacePolygonIndices(whiteBox, polygons);
            AZStd::vector<bool> chosen(polygons.size(), false);
            PolygonHandles live;
            for (const auto& seed : selection)
            {
                if (const int index = PolygonIndex(polygons, owner, seed); index >= 0 && !chosen[index])
                {
                    chosen[index] = true;
                    live.push_back(polygons[index]);
                }
            }
            // Old groups are cleared first so they neither block a bit nor leak into the new regions.
            SetPolygonSmoothingGroups(whiteBox, live, 0);

            AZStd::vector<AZ::Vector3> normals(polygons.size(), AZ::Vector3::CreateZero());
            for (size_t i = 0; i < polygons.size(); ++i)
            {
                if (chosen[i])
                {
                    normals[i] = PolygonNormal(whiteBox, polygons[i]);
                }
            }
            const float limit = cosf(AZ::DegToRad(AZ::GetClamp(angleDegrees, 0.0f, 180.0f)));

            // Flood across shared edges no sharper than the angle; neighbour to neighbour, so a curve stays one region.
            AZStd::vector<int> region(polygons.size(), -1);
            AZStd::vector<AZStd::vector<int>> regions;
            for (size_t start = 0; start < polygons.size(); ++start)
            {
                if (!chosen[start] || region[start] >= 0)
                {
                    continue;
                }
                const int id = static_cast<int>(regions.size());
                regions.push_back({ static_cast<int>(start) });
                region[start] = id;
                for (size_t k = 0; k < regions[id].size(); ++k)
                {
                    const int current = regions[id][k];
                    for (const auto face : polygons[current].m_faceHandles)
                    {
                        for (const auto halfedge : FaceHalfedgeHandles(whiteBox, face))
                        {
                            const auto opposite = whiteBox.mesh.opposite_face_handle(om_heh(halfedge));
                            if (!opposite.is_valid())
                            {
                                continue;
                            }
                            const int next = owner[opposite.idx()];
                            if (next < 0 || next == current || !chosen[next] || region[next] >= 0 ||
                                normals[current].Dot(normals[next]) < limit)
                            {
                                continue;
                            }
                            region[next] = id;
                            regions[id].push_back(next);
                        }
                    }
                }
            }

            // Each multi-polygon region takes the lowest bit no polygon touching it uses; lone polygons stay flat.
            for (size_t id = 0; id < regions.size(); ++id)
            {
                if (regions[id].size() < 2)
                {
                    continue;
                }
                AZ::u32 used = 0;
                PolygonHandles members;
                for (const int member : regions[id])
                {
                    members.push_back(polygons[member]);
                    ForEachCornerNeighbour(
                        whiteBox, polygons[member], owner,
                        [&](const int neighbour)
                        {
                            if (region[neighbour] != static_cast<int>(id))
                            {
                                used |= FaceSmoothingGroups(whiteBox, polygons[neighbour].m_faceHandles.front());
                            }
                        });
                }
                AZ::u32 bit = 0;
                for (AZ::u32 candidate = 0; candidate < 32; ++candidate)
                {
                    if ((used & (1u << candidate)) == 0)
                    {
                        bit = 1u << candidate;
                        break;
                    }
                }
                SetPolygonSmoothingGroups(whiteBox, members, bit);
            }
        }

        PolygonHandles FindSimilarPolygons(
            const WhiteBoxMesh& whiteBox, const PolygonHandles& seeds, const SimilarBy similarBy, const float toleranceDegrees)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const auto polygons = MeshPolygonHandles(whiteBox);
            const auto owner = FacePolygonIndices(whiteBox, polygons);
            // Only the compared property is filled in.
            struct Key
            {
                AZ::Data::AssetId m_material;
                AZ::Vector3 m_normal = AZ::Vector3::CreateZero();
                float m_area = 0.0f;
                size_t m_sides = 0;
                AZ::u32 m_smoothing = 0;
            };
            const auto keyOf = [&whiteBox, similarBy](const PolygonHandle& polygon)
            {
                Key key;
                const FaceHandle front = polygon.m_faceHandles.front();
                switch (similarBy)
                {
                case SimilarBy::Material:
                    key.m_material = FaceMaterial(whiteBox, front);
                    break;
                case SimilarBy::Normal:
                    key.m_normal = PolygonNormal(whiteBox, polygon);
                    break;
                case SimilarBy::Area:
                    for (const auto face : polygon.m_faceHandles)
                    {
                        const auto corners = FaceVertexPositions(whiteBox, face);
                        key.m_area += 0.5f * (corners[1] - corners[0]).Cross(corners[2] - corners[0]).GetLength();
                    }
                    break;
                case SimilarBy::Sides:
                    for (const auto vertex : PolygonBorderVertexHandlesFlattened(whiteBox, polygon))
                    {
                        key.m_sides += VertexIsHidden(whiteBox, vertex) ? 0 : 1;
                    }
                    break;
                case SimilarBy::SmoothingGroups:
                    key.m_smoothing = FaceSmoothingGroups(whiteBox, front);
                    break;
                }
                return key;
            };
            const float limit = cosf(AZ::DegToRad(AZ::GetMax(toleranceDegrees, 0.0f)));
            const auto matches = [similarBy, limit](const Key& a, const Key& b)
            {
                switch (similarBy)
                {
                case SimilarBy::Material:
                    return a.m_material == b.m_material;
                case SimilarBy::Normal:
                    return a.m_normal.Dot(b.m_normal) >= limit;
                case SimilarBy::Area:
                    return AZStd::abs(a.m_area - b.m_area) <= 0.05f * AZ::GetMax(a.m_area, b.m_area) + 1e-8f;
                case SimilarBy::Sides:
                    return a.m_sides == b.m_sides;
                case SimilarBy::SmoothingGroups:
                    return a.m_smoothing == b.m_smoothing || (a.m_smoothing & b.m_smoothing) != 0;
                }
                return false;
            };

            AZStd::vector<Key> seedKeys;
            for (const auto& seed : seeds)
            {
                if (const int index = PolygonIndex(polygons, owner, seed); index >= 0)
                {
                    seedKeys.push_back(keyOf(polygons[index]));
                }
            }
            AZStd::vector<bool> marked(polygons.size(), false);
            if (!seedKeys.empty())
            {
                for (size_t i = 0; i < polygons.size(); ++i)
                {
                    const Key key = keyOf(polygons[i]);
                    marked[i] = AZStd::any_of(seedKeys.begin(), seedKeys.end(), [&](const Key& seed) { return matches(seed, key); });
                }
            }
            return MarkedPolygons(polygons, owner, seeds, marked);
        }

        EdgeHandles FindSimilarEdges(const WhiteBoxMesh& whiteBox, const EdgeHandles& seeds)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const auto mask = PolygonEdgeMask(whiteBox);
            const auto lengthOf = [&whiteBox](const EdgeHandle edge)
            {
                const auto ends = EdgeVertexPositions(whiteBox, edge);
                return (ends[1] - ends[0]).GetLength();
            };
            AZStd::vector<float> lengths;
            for (const auto edge : seeds)
            {
                if (EdgeInMask(mask, edge))
                {
                    lengths.push_back(lengthOf(edge));
                }
            }
            AZStd::vector<bool> marked(mask.size(), false);
            for (size_t i = 0; i < mask.size() && !lengths.empty(); ++i)
            {
                if (!mask[i])
                {
                    continue;
                }
                const float length = lengthOf(EdgeHandle{ static_cast<int>(i) });
                marked[i] = AZStd::any_of(lengths.begin(), lengths.end(), [length](const float seed)
                {
                    return AZStd::abs(seed - length) <= 0.05f * AZ::GetMax(seed, length) + 1e-6f;
                });
            }
            return MarkedHandles(seeds, marked);
        }

        VertexHandles FindSimilarVertices(const WhiteBoxMesh& whiteBox, const VertexHandles& seeds)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            const auto mask = PolygonEdgeMask(whiteBox);
            AZStd::vector<size_t> valences;
            for (const auto vertex : seeds)
            {
                if (SelectableVertex(whiteBox, vertex))
                {
                    valences.push_back(VisibleVertexNeighbours(whiteBox, mask, vertex).size());
                }
            }
            AZStd::vector<bool> marked(whiteBox.mesh.n_vertices(), false);
            for (size_t i = 0; i < marked.size() && !valences.empty(); ++i)
            {
                const VertexHandle vertex{ static_cast<int>(i) };
                if (!SelectableVertex(whiteBox, vertex))
                {
                    continue;
                }
                const size_t valence = VisibleVertexNeighbours(whiteBox, mask, vertex).size();
                marked[i] = valence > 0 && AZStd::find(valences.begin(), valences.end(), valence) != valences.end();
            }
            return MarkedHandles(seeds, marked);
        }

        ElementSelection ConvertSelection(
            const WhiteBoxMesh& whiteBox, const ElementSelection& source, const SelectionElement target, const bool touching)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            ElementSelection result;
            const auto mask = PolygonEdgeMask(whiteBox);
            const auto addVertex = [&whiteBox, &result](const VertexHandle vertex)
            {
                if (SelectableVertex(whiteBox, vertex) &&
                    AZStd::find(result.m_vertices.begin(), result.m_vertices.end(), vertex) == result.m_vertices.end())
                {
                    result.m_vertices.push_back(vertex);
                }
            };
            const auto addEdge = [&mask, &result](const EdgeHandle edge)
            {
                if (EdgeInMask(mask, edge) && AZStd::find(result.m_edges.begin(), result.m_edges.end(), edge) == result.m_edges.end())
                {
                    result.m_edges.push_back(edge);
                }
            };
            // Going up keeps a candidate when all of its parts are selected, or any of them when touching.
            const auto qualifies = [touching](const size_t selectedParts, const size_t parts)
            {
                return parts > 0 && (touching ? selectedParts > 0 : selectedParts == parts);
            };

            if (!source.m_polygons.empty())
            {
                const auto allPolygons = MeshPolygonHandles(whiteBox);
                for (const auto& polygon : source.m_polygons)
                {
                    if (AZStd::find(allPolygons.begin(), allPolygons.end(), polygon) == allPolygons.end())
                    {
                        continue; // stale
                    }
                    if (target == SelectionElement::Polygon)
                    {
                        result.m_polygons.push_back(polygon);
                    }
                    else if (target == SelectionElement::Edge)
                    {
                        for (const auto edge : PolygonBorderEdgeHandlesFlattened(whiteBox, polygon))
                        {
                            addEdge(edge);
                        }
                    }
                    else
                    {
                        for (const auto vertex : PolygonBorderVertexHandlesFlattened(whiteBox, polygon))
                        {
                            addVertex(vertex);
                        }
                    }
                }
            }
            else if (!source.m_edges.empty())
            {
                AZStd::vector<bool> selected(mask.size(), false);
                for (const auto edge : source.m_edges)
                {
                    if (EdgeInMask(mask, edge))
                    {
                        selected[edge.Index()] = true;
                    }
                }
                if (target == SelectionElement::Edge)
                {
                    result.m_edges = MarkedHandles(source.m_edges, selected);
                }
                else if (target == SelectionElement::Vertex)
                {
                    for (const auto edge : source.m_edges)
                    {
                        if (EdgeInMask(mask, edge))
                        {
                            for (const auto vertex : EdgeVertexHandles(whiteBox, edge))
                            {
                                addVertex(vertex);
                            }
                        }
                    }
                }
                else
                {
                    for (const auto& polygon : MeshPolygonHandles(whiteBox))
                    {
                        const auto borders = PolygonBorderEdgeHandlesFlattened(whiteBox, polygon);
                        size_t count = 0;
                        for (const auto edge : borders)
                        {
                            count += static_cast<size_t>(EdgeInMask(selected, edge));
                        }
                        if (qualifies(count, borders.size()))
                        {
                            result.m_polygons.push_back(polygon);
                        }
                    }
                }
            }
            else if (!source.m_vertices.empty())
            {
                AZStd::vector<bool> selected(whiteBox.mesh.n_vertices(), false);
                for (const auto vertex : source.m_vertices)
                {
                    if (SelectableVertex(whiteBox, vertex))
                    {
                        selected[vertex.Index()] = true;
                    }
                }
                if (target == SelectionElement::Vertex)
                {
                    result.m_vertices = MarkedHandles(source.m_vertices, selected);
                }
                else if (target == SelectionElement::Edge)
                {
                    for (size_t i = 0; i < mask.size(); ++i)
                    {
                        if (!mask[i])
                        {
                            continue;
                        }
                        // A run through hidden vertices is judged by the visible corners at its two ends.
                        const EdgeHandle edge{ static_cast<int>(i) };
                        const auto ends = EdgeVertexHandles(whiteBox, edge);
                        const auto first = WalkToVisibleVertex(whiteBox, mask, edge, ends[1]);
                        const auto second = WalkToVisibleVertex(whiteBox, mask, edge, ends[0]);
                        const size_t count = static_cast<size_t>(first.IsValid() && selected[first.Index()]) +
                            static_cast<size_t>(second.IsValid() && selected[second.Index()]);
                        if (qualifies(count, 2))
                        {
                            addEdge(edge);
                        }
                    }
                }
                else
                {
                    for (const auto& polygon : MeshPolygonHandles(whiteBox))
                    {
                        size_t corners = 0;
                        size_t count = 0;
                        for (const auto vertex : PolygonBorderVertexHandlesFlattened(whiteBox, polygon))
                        {
                            if (!VertexIsHidden(whiteBox, vertex))
                            {
                                ++corners;
                                count += static_cast<size_t>(selected[vertex.Index()]);
                            }
                        }
                        if (qualifies(count, corners))
                        {
                            result.m_polygons.push_back(polygon);
                        }
                    }
                }
            }
            return result;
        }

        bool DetachPolygons(
            WhiteBoxMesh& whiteBox, const PolygonHandles& polygons, WhiteBoxMesh& detached, AZStd::string& error)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            error.clear();
            const auto fail = [&error](const char* message)
            {
                error = message;
                return false;
            };
            if (polygons.empty())
            {
                return fail("Select one or more polygons to detach.");
            }

            const auto allPolygons = MeshPolygonHandles(whiteBox);
            PolygonHandles chosen;
            FaceHandles faces;
            for (const auto& polygon : polygons)
            {
                if (polygon.m_faceHandles.empty() ||
                    AZStd::find(allPolygons.begin(), allPolygons.end(), polygon) == allPolygons.end())
                {
                    return fail("The selection is stale. Select the polygons again.");
                }
                if (AZStd::find(chosen.begin(), chosen.end(), polygon) == chosen.end())
                {
                    chosen.push_back(polygon);
                    faces.insert(faces.end(), polygon.m_faceHandles.begin(), polygon.m_faceHandles.end());
                }
            }
            if (faces.size() >= MeshFaceCount(whiteBox))
            {
                return fail("That is every polygon on the layer. Duplicate the layer instead, or leave some polygons behind.");
            }

            auto part = CreateWhiteBoxMesh();
            AZStd::unordered_map<int, VertexHandle> copies;
            const auto copyOf = [&whiteBox, &part, &copies](const VertexHandle vertex)
            {
                if (const auto found = copies.find(vertex.Index()); found != copies.end())
                {
                    return found->second;
                }
                const auto added = AddVertex(*part, VertexPosition(whiteBox, vertex));
                if (VertexIsHidden(whiteBox, vertex))
                {
                    HideVertex(*part, added);
                }
                CopyVertexBlend(*part, added, whiteBox, vertex);
                copies.emplace(vertex.Index(), added);
                return added;
            };
            for (const auto& polygon : chosen)
            {
                FaceVertHandlesList triangles;
                triangles.reserve(polygon.m_faceHandles.size());
                for (const auto face : polygon.m_faceHandles)
                {
                    const auto halfedges = FaceHalfedgeHandles(whiteBox, face);
                    triangles.push_back(FaceVertHandles{ { copyOf(HalfedgeVertexHandleAtTip(whiteBox, halfedges[0])),
                                                           copyOf(HalfedgeVertexHandleAtTip(whiteBox, halfedges[1])),
                                                           copyOf(HalfedgeVertexHandleAtTip(whiteBox, halfedges[2])) } });
                }
                const auto added = AddPolygon(*part, triangles);
                if (added.m_faceHandles.size() != polygon.m_faceHandles.size())
                {
                    return fail("The selection could not be copied cleanly. The mesh was left unchanged.");
                }
                for (size_t i = 0; i < added.m_faceHandles.size(); ++i)
                {
                    const auto sourceFace = polygon.m_faceHandles[i];
                    CopyFaceAttributes(*part, added.m_faceHandles[i], whiteBox, sourceFace);
                    // Match corners by vertex, since the new face need not start at the same corner.
                    const auto sourceHalfedges = FaceHalfedgeHandles(whiteBox, sourceFace);
                    for (const auto halfedge : FaceHalfedgeHandles(*part, added.m_faceHandles[i]))
                    {
                        const auto tip = HalfedgeVertexHandleAtTip(*part, halfedge);
                        for (const auto sourceHalfedge : sourceHalfedges)
                        {
                            if (copies.find(HalfedgeVertexHandleAtTip(whiteBox, sourceHalfedge).Index())->second == tip)
                            {
                                part->mesh.set_texcoord2D(om_heh(halfedge), HalfedgeUV(whiteBox, sourceHalfedge));
                                break;
                            }
                        }
                    }
                }
            }
            CalculateNormals(*part);

            auto remaining = CloneMesh(whiteBox);
            RemoveFaces(*remaining, faces);
            RemoveIsolatedVertices(*remaining);
            CalculateNormals(*remaining);
            whiteBox.mesh = remaining->mesh;
            detached.mesh = part->mesh;
            return true;
        }

        // Whether two segments meet, touching included; inputs are in a unit-sized frame.
        static bool SegmentsTouch(const AZ::Vector2& p, const AZ::Vector2& q, const AZ::Vector2& r, const AZ::Vector2& s)
        {
            constexpr float Epsilon = 1e-6f;
            const auto turn = [](const AZ::Vector2& a, const AZ::Vector2& b, const AZ::Vector2& c)
            {
                return (b.GetX() - a.GetX()) * (c.GetY() - a.GetY()) - (b.GetY() - a.GetY()) * (c.GetX() - a.GetX());
            };
            const auto within = [](const AZ::Vector2& a, const AZ::Vector2& b, const AZ::Vector2& c)
            {
                return c.GetX() >= AZ::GetMin(a.GetX(), b.GetX()) - Epsilon && c.GetX() <= AZ::GetMax(a.GetX(), b.GetX()) + Epsilon &&
                    c.GetY() >= AZ::GetMin(a.GetY(), b.GetY()) - Epsilon && c.GetY() <= AZ::GetMax(a.GetY(), b.GetY()) + Epsilon;
            };
            const float d1 = turn(r, s, p);
            const float d2 = turn(r, s, q);
            const float d3 = turn(p, q, r);
            const float d4 = turn(p, q, s);
            if (((d1 > Epsilon && d2 < -Epsilon) || (d1 < -Epsilon && d2 > Epsilon)) &&
                ((d3 > Epsilon && d4 < -Epsilon) || (d3 < -Epsilon && d4 > Epsilon)))
            {
                return true;
            }
            return (AZStd::abs(d1) <= Epsilon && within(r, s, p)) || (AZStd::abs(d2) <= Epsilon && within(r, s, q)) ||
                (AZStd::abs(d3) <= Epsilon && within(p, q, r)) || (AZStd::abs(d4) <= Epsilon && within(p, q, s));
        }

        // Replace a polygon with the halves either side of a straight cut between two corners; only vertex handles survive.
        static bool SplitPolygonBetween(
            WhiteBoxMesh& whiteBox, const PolygonHandle& polygon, const VertexHandle a, const VertexHandle b, AZStd::string& error)
        {
            const auto fail = [&error](const char* message)
            {
                error = message;
                return false;
            };
            const auto borders = PolygonBorderVertexHandles(whiteBox, polygon);
            if (borders.size() != 1)
            {
                return fail("Connect cannot split a polygon with a hole in it.");
            }
            const auto& loop = borders.front();
            const size_t count = loop.size();
            const size_t first = static_cast<size_t>(AZStd::find(loop.begin(), loop.end(), a) - loop.begin());
            const size_t second = static_cast<size_t>(AZStd::find(loop.begin(), loop.end(), b) - loop.begin());
            if (first >= count || second >= count)
            {
                return fail("Both vertices must be corners of the same polygon.");
            }

            // Newell's normal and a right handed frame keep the loop wound the same way in 2d as around the normal.
            AZStd::vector<AZ::Vector3> positions;
            positions.reserve(count);
            AZ::Vector3 centre = AZ::Vector3::CreateZero();
            for (const auto vertex : loop)
            {
                positions.push_back(VertexPosition(whiteBox, vertex));
                centre += positions.back();
            }
            centre /= static_cast<float>(count);
            AZ::Vector3 normal = AZ::Vector3::CreateZero();
            float extent = 0.0f;
            for (size_t i = 0; i < count; ++i)
            {
                normal += (positions[i] - centre).Cross(positions[(i + 1) % count] - centre);
                extent = AZ::GetMax(extent, (positions[i] - centre).GetLength());
            }
            if (extent <= AZ::Constants::FloatEpsilon || normal.GetLengthSq() <= 1e-20f)
            {
                return fail("The polygon is collapsed and has no area to split.");
            }
            normal.Normalize();
            const AZ::Vector3 reference = AZStd::abs(normal.GetZ()) < 0.9f ? AZ::Vector3::CreateAxisZ() : AZ::Vector3::CreateAxisX();
            const AZ::Vector3 uAxis = normal.Cross(reference).GetNormalized();
            const AZ::Vector3 vAxis = normal.Cross(uAxis);
            const auto flatten = [&centre, extent, &uAxis, &vAxis](const AZ::Vector3& position)
            {
                const auto offset = (position - centre) / extent;
                return AZ::Vector2(offset.Dot(uAxis), offset.Dot(vAxis));
            };
            AZStd::vector<AZ::Vector2> points;
            points.reserve(count);
            for (const auto& position : positions)
            {
                points.push_back(flatten(position));
            }

            // The cut must cross no other border edge and run through the inside of the outline.
            const AZ::Vector2 start = points[first];
            const AZ::Vector2 end = points[second];
            for (size_t i = 0; i < count; ++i)
            {
                const size_t next = (i + 1) % count;
                if (i == first || i == second || next == first || next == second)
                {
                    continue;
                }
                if (SegmentsTouch(start, end, points[i], points[next]))
                {
                    return fail("The straight cut would leave the polygon or pass through another corner.");
                }
            }
            const AZ::Vector2 middle = (start + end) * 0.5f;
            bool inside = false;
            for (size_t i = 0, j = count - 1; i < count; j = i++)
            {
                if ((points[i].GetY() > middle.GetY()) != (points[j].GetY() > middle.GetY()) &&
                    middle.GetX() < (points[j].GetX() - points[i].GetX()) * (middle.GetY() - points[i].GetY()) /
                            (points[j].GetY() - points[i].GetY()) + points[i].GetX())
                {
                    inside = !inside;
                }
            }
            if (!inside)
            {
                return fail("The straight cut would run outside the polygon. Connect corners that can see each other.");
            }

            // Both halves keep the loop's winding: first to second, then second back to first.
            AZStd::array<VertexHandles, 2> halves;
            AZStd::array<AZStd::vector<AZ::Vector2>, 2> halfPoints;
            for (size_t side = 0; side < 2; ++side)
            {
                const size_t from = side == 0 ? first : second;
                const size_t to = side == 0 ? second : first;
                for (size_t i = from;; i = (i + 1) % count)
                {
                    halves[side].push_back(loop[i]);
                    halfPoints[side].push_back(points[i]);
                    if (i == to)
                    {
                        break;
                    }
                }
            }
            AZStd::array<AZStd::vector<AZStd::array<size_t, 3>>, 2> triangles;
            for (size_t side = 0; side < 2; ++side)
            {
                if (!TriangulateLoop(halfPoints[side], triangles[side]))
                {
                    return fail("One side of the cut could not be triangulated. Move the vertices so both halves are simple.");
                }
            }

            // Paint is per triangle, so each new face takes the attributes of the old face it lies in.
            struct SourceFace
            {
                AZStd::array<AZ::Vector2, 3> m_points;
                AZ::Data::AssetId m_material;
                AZ::u32 m_paint = 0;
                UvProjection m_uvProjection;
                AZ::u32 m_smoothing = 0;
            };
            AZStd::vector<SourceFace> sources;
            for (const auto face : polygon.m_faceHandles)
            {
                SourceFace source;
                source.m_material = FaceMaterial(whiteBox, face);
                source.m_paint = FacePaintColor(whiteBox, face);
                source.m_uvProjection = FaceUvProjection(whiteBox, face);
                source.m_smoothing = FaceSmoothingGroups(whiteBox, face);
                const auto corners = FaceVertexPositions(whiteBox, face);
                for (size_t i = 0; i < 3; ++i)
                {
                    source.m_points[i] = flatten(corners[i]);
                }
                sources.push_back(source);
            }
            const auto sourceFor = [&sources](const AZ::Vector2& point) -> const SourceFace&
            {
                const auto turn = [](const AZ::Vector2& p, const AZ::Vector2& q, const AZ::Vector2& r)
                {
                    return (q.GetX() - p.GetX()) * (r.GetY() - p.GetY()) - (q.GetY() - p.GetY()) * (r.GetX() - p.GetX());
                };
                for (const auto& source : sources)
                {
                    const auto& t = source.m_points;
                    const float sign = turn(t[0], t[1], t[2]) < 0.0f ? -1.0f : 1.0f;
                    if (sign * turn(t[0], t[1], point) >= -1e-6f && sign * turn(t[1], t[2], point) >= -1e-6f &&
                        sign * turn(t[2], t[0], point) >= -1e-6f)
                    {
                        return source;
                    }
                }
                return sources.front();
            };
            AZStd::array<AZStd::vector<const SourceFace*>, 2> origins;
            for (size_t side = 0; side < 2; ++side)
            {
                for (const auto& triangle : triangles[side])
                {
                    const auto centroid =
                        (halfPoints[side][triangle[0]] + halfPoints[side][triangle[1]] + halfPoints[side][triangle[2]]) / 3.0f;
                    origins[side].push_back(&sourceFor(centroid));
                }
            }

            RemoveFaces(whiteBox, polygon.m_faceHandles);
            // OpenMesh can refuse a triangle until its neighbours are in, so refused ones are retried until none go in.
            AZStd::vector<AZStd::pair<size_t, size_t>> pending;
            for (size_t side = 0; side < 2; ++side)
            {
                for (size_t i = 0; i < triangles[side].size(); ++i)
                {
                    pending.emplace_back(side, i);
                }
            }
            AZStd::array<FaceHandlesInternal, 2> groups;
            while (!pending.empty())
            {
                AZStd::vector<AZStd::pair<size_t, size_t>> refused;
                for (const auto& item : pending)
                {
                    const size_t side = item.first;
                    const size_t i = item.second;
                    const auto& triangle = triangles[side][i];
                    // Not AddPolygon: its duplicate-vertex fallback would tear the surface instead of refusing.
                    const auto face = whiteBox.mesh.add_face(
                        om_vh(halves[side][triangle[0]]), om_vh(halves[side][triangle[1]]), om_vh(halves[side][triangle[2]]));
                    if (!face.is_valid())
                    {
                        refused.emplace_back(side, i);
                        continue;
                    }
                    groups[side].push_back(face);
                    SetFaceMaterial(whiteBox, wb_fh(face), origins[side][i]->m_material);
                    SetFacePaintColor(whiteBox, wb_fh(face), origins[side][i]->m_paint);
                    SetFaceUvProjection(whiteBox, wb_fh(face), origins[side][i]->m_uvProjection);
                    SetFaceSmoothingGroups(whiteBox, wb_fh(face), origins[side][i]->m_smoothing);
                }
                if (refused.size() == pending.size())
                {
                    return fail("The cut would create invalid face connections. The mesh was left unchanged.");
                }
                pending = AZStd::move(refused);
            }
            PolygonPropertyHandle property;
            whiteBox.mesh.get_property_handle(property, PolygonProps);
            auto& mapping = whiteBox.mesh.property(property);
            for (const auto& group : groups)
            {
                for (const auto face : group)
                {
                    mapping[face] = group;
                }
            }
            return true;
        }

        bool ConnectVertices(WhiteBoxMesh& whiteBox, const VertexHandles& vertices, AZStd::string& error, EdgeHandles* created)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            error.clear();
            const auto fail = [&error](const char* message)
            {
                error = message;
                return false;
            };
            VertexHandles corners;
            for (const auto vertex : vertices)
            {
                if (!SelectableVertex(whiteBox, vertex))
                {
                    return fail("The selection is stale or includes a hidden vertex. Select the vertices again.");
                }
                if (AZStd::find(corners.begin(), corners.end(), vertex) == corners.end())
                {
                    corners.push_back(vertex);
                }
            }
            if (corners.size() < 2)
            {
                return fail("Select two or more vertices on the same polygon.");
            }

            // Every polygon holding both corners; usually one, two when they already share an edge.
            const auto sharedPolygons = [](const WhiteBoxMesh& mesh, const VertexHandle a, const VertexHandle b)
            {
                PolygonHandles shared;
                for (const auto halfedge : VertexOutgoingHalfedgeHandles(mesh, a))
                {
                    const auto face = HalfedgeFaceHandle(mesh, halfedge);
                    if (!face.IsValid())
                    {
                        continue;
                    }
                    const auto polygon = FacePolygonHandle(mesh, face);
                    if (polygon.m_faceHandles.empty() || AZStd::find(shared.begin(), shared.end(), polygon) != shared.end())
                    {
                        continue;
                    }
                    const auto loop = PolygonBorderVertexHandlesFlattened(mesh, polygon);
                    if (AZStd::find(loop.begin(), loop.end(), b) != loop.end())
                    {
                        shared.push_back(polygon);
                    }
                }
                return shared;
            };
            // Joined means a visible edge; a hidden diagonal inside one polygon still leaves something to cut.
            const auto joined = [](const WhiteBoxMesh& mesh, const VertexHandle a, const VertexHandle b)
            {
                const auto halfedge = mesh.mesh.find_halfedge(om_vh(a), om_vh(b));
                if (!halfedge.is_valid())
                {
                    return false;
                }
                const auto faces = EdgeFaceHandles(mesh, wb_eh(mesh.mesh.edge_handle(halfedge)));
                return faces.size() < 2 || FacePolygonHandle(mesh, faces[0]) != FacePolygonHandle(mesh, faces[1]);
            };

            // A path in selection order when every step shares a polygon, otherwise two corners per polygon.
            AZStd::vector<AZStd::pair<VertexHandle, VertexHandle>> pairs;
            bool path = true;
            for (size_t i = 0; i + 1 < corners.size() && path; ++i)
            {
                path = !sharedPolygons(whiteBox, corners[i], corners[i + 1]).empty();
            }
            if (path)
            {
                for (size_t i = 0; i + 1 < corners.size(); ++i)
                {
                    if (!joined(whiteBox, corners[i], corners[i + 1]))
                    {
                        pairs.emplace_back(corners[i], corners[i + 1]);
                    }
                }
            }
            else
            {
                for (const auto& polygon : MeshPolygonHandles(whiteBox))
                {
                    VertexHandles held;
                    for (const auto vertex : PolygonBorderVertexHandlesFlattened(whiteBox, polygon))
                    {
                        if (AZStd::find(corners.begin(), corners.end(), vertex) != corners.end() &&
                            AZStd::find(held.begin(), held.end(), vertex) == held.end())
                        {
                            held.push_back(vertex);
                        }
                    }
                    if (held.size() == 2 && !joined(whiteBox, held[0], held[1]))
                    {
                        pairs.emplace_back(held[0], held[1]);
                    }
                }
            }
            if (pairs.empty())
            {
                return fail("Nothing to connect. Select corners of one polygon that are not already joined by an edge.");
            }

            auto candidate = CloneMesh(whiteBox);
            for (const auto& pair : pairs)
            {
                // Earlier cuts replace polygons, so the pair is looked up again in the edited mesh.
                if (joined(*candidate, pair.first, pair.second))
                {
                    continue;
                }
                const auto shared = sharedPolygons(*candidate, pair.first, pair.second);
                if (shared.empty())
                {
                    return fail("An earlier cut separated two of the selected vertices. Select them in a different order.");
                }
                AZStd::string reason;
                bool split = false;
                for (const auto& polygon : shared)
                {
                    auto attempt = CloneMesh(*candidate);
                    if (SplitPolygonBetween(*attempt, polygon, pair.first, pair.second, reason))
                    {
                        AssignMesh(*candidate, *attempt);
                        split = true;
                        break;
                    }
                }
                if (!split)
                {
                    error = reason;
                    return false;
                }
            }
            EdgeHandles made;
            for (const auto& pair : pairs)
            {
                const auto halfedge = candidate->mesh.find_halfedge(om_vh(pair.first), om_vh(pair.second));
                if (halfedge.is_valid())
                {
                    made.push_back(wb_eh(candidate->mesh.edge_handle(halfedge)));
                }
            }
            // Interior hidden vertices of a split polygon are left without faces; nothing else refers to them.
            RemoveIsolatedVertices(*candidate);
            CalculateNormals(*candidate);
            CalculatePlanarUVs(*candidate);
            whiteBox.mesh = candidate->mesh;
            if (created != nullptr)
            {
                *created = AZStd::move(made);
            }
            return true;
        }

        VertexHandle InsertVertexOnEdge(WhiteBoxMesh& whiteBox, const EdgeHandle edge, const float fraction, AZStd::string& error)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            error.clear();
            if (!edge.IsValid() || static_cast<size_t>(edge.Index()) >= whiteBox.mesh.n_edges())
            {
                error = "The edge is stale. Hover it again.";
                return {};
            }
            if (!AZ::IsFiniteFloat(fraction) || fraction <= 0.001f || fraction >= 0.999f)
            {
                error = "Place the new vertex between the edge's two ends.";
                return {};
            }
            const auto polygonEdges = MeshPolygonEdgeHandles(whiteBox);
            if (AZStd::find(polygonEdges.begin(), polygonEdges.end(), edge) == polygonEdges.end())
            {
                error = "Pick a polygon border edge, not a triangle diagonal inside a polygon.";
                return {};
            }
            const auto ends = EdgeVertexPositions(whiteBox, edge);
            if ((ends[1] - ends[0]).GetLengthSq() <= 1e-12f)
            {
                error = "The edge has no length to split.";
                return {};
            }
            auto candidate = CloneMesh(whiteBox);
            // A border edge's new vertex stays visible, and split_copy carries each face's attributes onto its halves.
            const auto vertex = SplitEdge(*candidate, edge, ends[0].Lerp(ends[1], fraction));
            CalculateNormals(*candidate);
            FaceHandles touched;
            for (const auto halfedge : VertexOutgoingHalfedgeHandles(*candidate, vertex))
            {
                if (const auto face = HalfedgeFaceHandle(*candidate, halfedge); face.IsValid())
                {
                    touched.push_back(face);
                }
            }
            CalculatePlanarUVs(*candidate, touched);
            whiteBox.mesh = candidate->mesh;
            return vertex;
        }

        bool SubdividePolygons(WhiteBoxMesh& whiteBox, const PolygonHandles& polygons, AZStd::string& error, PolygonHandles* created)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            error.clear();
            const auto fail = [&error](const char* message)
            {
                error = message;
                return false;
            };
            const auto all = MeshPolygonHandles(whiteBox);
            // Border loops in winding order, one per distinct polygon.
            PolygonHandles chosen;
            AZStd::vector<VertexHandles> loops;
            for (const auto& polygon : polygons)
            {
                if (polygon.m_faceHandles.empty() || AZStd::find(all.begin(), all.end(), polygon) == all.end())
                {
                    return fail("The selection is stale. Select the polygons again.");
                }
                if (AZStd::find(chosen.begin(), chosen.end(), polygon) != chosen.end())
                {
                    continue;
                }
                const auto borders = PolygonBorderVertexHandles(whiteBox, polygon);
                if (borders.size() != 1 || borders.front().size() < 3)
                {
                    return fail("Subdivide cannot split a polygon with a hole in it.");
                }
                chosen.push_back(polygon);
                loops.push_back(borders.front());
            }
            if (chosen.empty())
            {
                return fail("Select one or more polygons to subdivide.");
            }

            // Each corner becomes the quad (previous midpoint, corner, next midpoint, centre); both halves must face out.
            AZStd::vector<AZ::Vector3> centres;
            for (size_t p = 0; p < loops.size(); ++p)
            {
                const auto& loop = loops[p];
                const size_t count = loop.size();
                AZ::Vector3 centre = AZ::Vector3::CreateZero();
                for (const auto vertex : loop)
                {
                    centre += VertexPosition(whiteBox, vertex);
                }
                centre /= static_cast<float>(count);
                const AZ::Vector3 normal = PolygonNormal(whiteBox, chosen[p]);
                for (size_t i = 0; i < count; ++i)
                {
                    const AZ::Vector3 corner = VertexPosition(whiteBox, loop[i]);
                    const AZ::Vector3 previous = (VertexPosition(whiteBox, loop[(i + count - 1) % count]) + corner) * 0.5f;
                    const AZ::Vector3 next = (corner + VertexPosition(whiteBox, loop[(i + 1) % count])) * 0.5f;
                    if ((corner - previous).Cross(centre - previous).Dot(normal) <= 1e-10f ||
                        (next - corner).Cross(centre - corner).Dot(normal) <= 1e-10f)
                    {
                        return fail("Subdivide needs convex polygons. Split concave ones with Connect first.");
                    }
                }
                centres.push_back(centre);
            }

            auto candidate = CloneMesh(whiteBox);
            // Every border edge splits once at its midpoint; an unselected neighbour gains that vertex on its border.
            AZStd::unordered_map<AZ::u64, VertexHandle> midpoints;
            const auto edgeKey = [](const VertexHandle a, const VertexHandle b)
            {
                const auto low = static_cast<AZ::u64>(AZStd::min(a.Index(), b.Index()));
                const auto high = static_cast<AZ::u64>(AZStd::max(a.Index(), b.Index()));
                return (low << 32) | high;
            };
            for (const auto& loop : loops)
            {
                for (size_t i = 0; i < loop.size(); ++i)
                {
                    const VertexHandle a = loop[i];
                    const VertexHandle b = loop[(i + 1) % loop.size()];
                    auto& midpoint = midpoints[edgeKey(a, b)];
                    if (midpoint.IsValid())
                    {
                        continue;
                    }
                    const auto halfedge = candidate->mesh.find_halfedge(om_vh(a), om_vh(b));
                    if (!halfedge.is_valid())
                    {
                        return fail("A polygon border is not a mesh edge. The mesh was left unchanged.");
                    }
                    midpoint = SplitEdge(
                        *candidate, wb_eh(candidate->mesh.edge_handle(halfedge)),
                        (VertexPosition(*candidate, a) + VertexPosition(*candidate, b)) * 0.5f);
                }
            }

            struct Quad
            {
                AZStd::array<VertexHandle, 4> m_vertices; //!< previous midpoint, corner, next midpoint, centre
                FaceHandle m_source; //!< Original face nearest the quad, for its attributes.
            };
            AZStd::vector<Quad> quads;
            FaceHandles removed;
            VertexHandles shown;
            for (size_t p = 0; p < loops.size(); ++p)
            {
                const auto& loop = loops[p];
                const size_t count = loop.size();
                // split_copy keeps the original face handle, so it still finds the (now wider) polygon.
                const auto updated = FacePolygonHandle(*candidate, chosen[p].m_faceHandles.front());
                removed.insert(removed.end(), updated.m_faceHandles.begin(), updated.m_faceHandles.end());
                const VertexHandle centre = AddVertex(*candidate, centres[p]);
                shown.push_back(centre);
                for (size_t i = 0; i < count; ++i)
                {
                    Quad quad;
                    quad.m_vertices = { { midpoints[edgeKey(loop[(i + count - 1) % count], loop[i])], loop[i],
                                          midpoints[edgeKey(loop[i], loop[(i + 1) % count])], centre } };
                    shown.push_back(quad.m_vertices[0]);
                    AZ::Vector3 middle = AZ::Vector3::CreateZero();
                    for (const auto vertex : quad.m_vertices)
                    {
                        middle += VertexPosition(*candidate, vertex) * 0.25f;
                    }
                    float nearest = AZ::Constants::FloatMax;
                    for (const auto face : chosen[p].m_faceHandles)
                    {
                        const float distance = FaceMidpoint(whiteBox, face).GetDistanceSq(middle);
                        if (distance < nearest)
                        {
                            nearest = distance;
                            quad.m_source = face;
                        }
                    }
                    quads.push_back(quad);
                }
            }

            RemoveFaces(*candidate, removed);
            // OpenMesh can refuse a triangle until its neighbours are in, so refused ones are retried until none go in.
            AZStd::vector<AZStd::pair<size_t, size_t>> pending;
            for (size_t q = 0; q < quads.size(); ++q)
            {
                pending.emplace_back(q, 0);
                pending.emplace_back(q, 1);
            }
            AZStd::vector<FaceHandlesInternal> groups(quads.size());
            while (!pending.empty())
            {
                AZStd::vector<AZStd::pair<size_t, size_t>> refused;
                for (const auto& item : pending)
                {
                    const auto& v = quads[item.first].m_vertices;
                    const auto face = item.second == 0 ? candidate->mesh.add_face(om_vh(v[0]), om_vh(v[1]), om_vh(v[3]))
                                                       : candidate->mesh.add_face(om_vh(v[1]), om_vh(v[2]), om_vh(v[3]));
                    if (!face.is_valid())
                    {
                        refused.push_back(item);
                        continue;
                    }
                    groups[item.first].push_back(face);
                    CopyFaceAttributes(*candidate, wb_fh(face), whiteBox, quads[item.first].m_source);
                }
                if (refused.size() == pending.size())
                {
                    return fail("Subdivide would create invalid face connections. The mesh was left unchanged.");
                }
                pending = AZStd::move(refused);
            }

            PolygonPropertyHandle polygonProperty;
            candidate->mesh.get_property_handle(polygonProperty, PolygonProps);
            auto& mapping = candidate->mesh.property(polygonProperty);
            PolygonHandles made;
            for (const auto& group : groups)
            {
                PolygonHandle polygon;
                for (const auto face : group)
                {
                    mapping[face] = group;
                    polygon.m_faceHandles.push_back(wb_fh(face));
                }
                made.push_back(AZStd::move(polygon));
            }
            // A split on an edge that was not a user edge hides its vertex, but every quad corner must stay selectable.
            VertexBoolPropertyHandle hiddenProperty;
            candidate->mesh.get_property_handle(hiddenProperty, VertexHiddenProp);
            for (const auto vertex : shown)
            {
                candidate->mesh.property(hiddenProperty, om_vh(vertex)) = false;
            }
            CalculateNormals(*candidate);
            CalculatePlanarUVs(*candidate);
            whiteBox.mesh = candidate->mesh;
            if (created != nullptr)
            {
                *created = AZStd::move(made);
            }
            return true;
        }

        void AssignMesh(WhiteBoxMesh& target, const WhiteBoxMesh& source)
        {
            target.mesh = source.mesh;
        }

        bool ExtrudeEdgeSelection(
            WhiteBoxMesh& mesh, const EdgeHandles& edges, const AZ::Vector3& offset,
            EdgeHandles& result, PolygonHandles& preview, AZStd::string& error)
        {
            result.clear();
            preview.clear();
            const auto fail = [&error](const char* message) { error = message; return false; };
            if (edges.empty() || !offset.IsFinite() || offset.IsZero(1e-5f)) { return fail("Drag away from the edge to extrude."); }
            const auto valid = MeshPolygonEdgeHandles(mesh);
            bool boundary = true;
            for (const auto edge : edges)
            {
                if (AZStd::find(valid.begin(), valid.end(), edge) == valid.end()) { return fail("Select polygon boundary edges."); }
                boundary = boundary && EdgeIsBoundary(mesh, edge);
            }
            auto candidate = CloneMesh(mesh);
            if (!boundary)
            {
                if (edges.size() != 1) { return fail("Select one solid edge, or a connected set of open boundary edges."); }
                const auto edge = TranslateEdgeAppend(*candidate, edges.front(), offset);
                if (!edge.IsValid() || edge == edges.front()) { return fail("This edge cannot extrude in that direction."); }
                result.push_back(edge);
                preview = MeshPolygonHandles(*candidate);
            }
            else
            {
                AZStd::map<int, VertexHandle> top;
                for (const auto edge : edges)
                {
                    for (const auto vertex : EdgeVertexHandles(mesh, edge))
                    {
                        if (!top.count(vertex.Index())) { top.emplace(vertex.Index(), AddVertex(*candidate, VertexPosition(mesh, vertex) + offset)); }
                    }
                }
                for (const auto edge : edges)
                {
                    auto halfedge = EdgeHalfedgeHandle(mesh, edge, EdgeHalfedge::First);
                    if (!HalfedgeIsBoundary(mesh, halfedge)) { halfedge = HalfedgeOppositeHalfedgeHandle(mesh, halfedge); }
                    const auto a = HalfedgeVertexHandleAtTail(mesh, halfedge);
                    const auto b = HalfedgeVertexHandleAtTip(mesh, halfedge);
                    AZ::Vector3 normal;
                    if (!TryCalculateTriangleNormal(VertexPosition(mesh, b) - VertexPosition(mesh, a), offset, normal))
                    {
                        return fail("Drag across the edge, not along it.");
                    }
                    const auto polygon = AddQuadWithoutFallback(*candidate, {{
                        a, b, top.find(b.Index())->second, top.find(a.Index())->second}});
                    if (polygon.m_faceHandles.size() != 2) { return fail("These edges cannot extrude together without invalid connections."); }
                    const auto source = EdgeFaceHandles(mesh, edge).front();
                    for (const auto face : polygon.m_faceHandles)
                    {
                        CopyFaceAttributes(*candidate, face, mesh, source);
                    }
                    const auto outer = candidate->mesh.find_halfedge(
                        om_vh(top.find(b.Index())->second), om_vh(top.find(a.Index())->second));
                    if (!outer.is_valid()) { return fail("Could not create the extruded edge."); }
                    result.push_back(EdgeHandle{candidate->mesh.edge_handle(outer).idx()});
                    preview.push_back(polygon);
                    CalculateNormals(*candidate);
                    CalculatePlanarUVs(*candidate, polygon.m_faceHandles);
                }
            }
            AssignMesh(mesh, *candidate);
            return true;
        }

        // Even-width inset for regions proportional scaling cannot take: concave, holed or folded ones.
        static bool OffsetInsetRegion(
            const WhiteBoxMesh& whiteBox, const FaceHandles& faces, const HalfedgeHandles& border, const VertexHandles& vertices,
            const float amount, AZStd::map<int, AZ::Vector3>& moved, AZStd::string& error)
        {
            const auto fail = [&error](const char* message)
            {
                error = message;
                return false;
            };
            // Border halfedges run with the region on their left, so every border vertex has one in and one out.
            AZStd::map<int, HalfedgeHandle> incoming;
            AZStd::map<int, HalfedgeHandle> outgoing;
            for (const auto halfedge : border)
            {
                if (!outgoing.emplace(HalfedgeVertexHandleAtTail(whiteBox, halfedge).Index(), halfedge).second ||
                    !incoming.emplace(HalfedgeVertexHandleAtTip(whiteBox, halfedge).Index(), halfedge).second)
                {
                    return fail("Inset needs a region whose border does not touch itself at a vertex.");
                }
            }
            const auto faceNormal = [&whiteBox](const FaceHandle face)
            {
                const auto p = FaceVertexPositions(whiteBox, face);
                return (p[1] - p[0]).Cross(p[2] - p[0]).GetNormalizedSafe();
            };
            // The in-surface direction into the region, square to one border edge.
            const auto inward = [&whiteBox, &faceNormal](const HalfedgeHandle halfedge)
            {
                const auto along = HalfedgeVertexPositionAtTip(whiteBox, halfedge) - HalfedgeVertexPositionAtTail(whiteBox, halfedge);
                return faceNormal(HalfedgeFaceHandle(whiteBox, halfedge)).Cross(along).GetNormalizedSafe();
            };

            // How far each border vertex travels per unit of inset depth.
            AZStd::map<int, AZ::Vector3> directions;
            for (const auto& entry : outgoing)
            {
                const auto in = incoming.find(entry.first);
                if (in == incoming.end())
                {
                    return fail("Inset needs a closed region border.");
                }
                const AZ::Vector3 before = inward(in->second);
                const AZ::Vector3 after = inward(entry.second);
                if (before.IsZero() || after.IsZero())
                {
                    return fail("The region has a collapsed border edge. Weld it away first.");
                }
                AZ::Vector3 direction = AZ::Vector3::CreateZero();
                // Across a crease the corner slides along the fold line, so it stays on both faces.
                AZ::Vector3 fold = faceNormal(HalfedgeFaceHandle(whiteBox, in->second))
                                       .Cross(faceNormal(HalfedgeFaceHandle(whiteBox, entry.second)));
                if (fold.GetLengthSq() > 4e-4f)
                {
                    fold.Normalize();
                    if (fold.Dot(before + after) < 0.0f)
                    {
                        fold = -fold;
                    }
                    if (const float reach = 0.5f * (fold.Dot(before) + fold.Dot(after)); reach > 0.3f)
                    {
                        direction = fold / reach;
                    }
                }
                if (direction.IsZero())
                {
                    const AZ::Vector3 bisector = before + after;
                    if (bisector.GetLengthSq() < 1e-6f)
                    {
                        return fail("The region's border turns straight back on itself, which cannot be inset evenly.");
                    }
                    // A sharp corner moves further so both of its edges end up the same distance in.
                    const AZ::Vector3 unit = bisector.GetNormalized();
                    direction = unit / AZStd::max(unit.Dot(before), 0.2f);
                }
                directions.emplace(entry.first, direction);
            }

            AZ::Vector3 centre = AZ::Vector3::CreateZero();
            for (const auto vertex : vertices)
            {
                centre += VertexPosition(whiteBox, vertex);
            }
            centre /= static_cast<float>(vertices.size());
            float extent = 0.0f;
            for (const auto vertex : vertices)
            {
                extent = AZ::GetMax(extent, (VertexPosition(whiteBox, vertex) - centre).GetLength());
            }
            AZ::Vector3 normal = AZ::Vector3::CreateZero();
            for (const auto face : faces)
            {
                const auto p = FaceVertexPositions(whiteBox, face);
                normal += (p[1] - p[0]).Cross(p[2] - p[0]);
            }
            normal = normal.GetNormalizedSafe();
            if (extent <= AZ::Constants::FloatEpsilon || normal.IsZero())
            {
                return fail("The region is collapsed and has no area to inset.");
            }
            // A flat outline can cross itself without any one face folding, so it also gets a 2d crossing test.
            bool planar = true;
            for (const auto vertex : vertices)
            {
                planar = planar && AZStd::abs((VertexPosition(whiteBox, vertex) - centre).Dot(normal)) <= extent * 1e-4f;
            }
            const AZ::Vector3 reference = AZStd::abs(normal.GetZ()) < 0.9f ? AZ::Vector3::CreateAxisZ() : AZ::Vector3::CreateAxisX();
            const AZ::Vector3 uAxis = normal.Cross(reference).GetNormalized();
            const AZ::Vector3 vAxis = normal.Cross(uAxis);
            const auto flatten = [&centre, extent, &uAxis, &vAxis](const AZ::Vector3& position)
            {
                const auto offset = (position - centre) / extent;
                return AZ::Vector2(offset.Dot(uAxis), offset.Dot(vAxis));
            };
            const auto positionAt = [&whiteBox, &directions](const VertexHandle vertex, const float depth)
            {
                const auto position = VertexPosition(whiteBox, vertex);
                const auto found = directions.find(vertex.Index());
                return found == directions.end() ? position : position + found->second * depth;
            };

            // A depth is usable while no face folds or collapses, no border edge reverses and a flat outline stays simple.
            const auto usable = [&](const float depth)
            {
                for (const auto face : faces)
                {
                    const auto corners = FaceVertexHandles(whiteBox, face);
                    const bool touched = AZStd::any_of(
                        corners.begin(), corners.end(),
                        [&directions](const VertexHandle vertex)
                        {
                            return directions.find(vertex.Index()) != directions.end();
                        });
                    if (!touched)
                    {
                        continue;
                    }
                    const auto p0 = VertexPosition(whiteBox, corners[0]);
                    const AZ::Vector3 was = (VertexPosition(whiteBox, corners[1]) - p0).Cross(VertexPosition(whiteBox, corners[2]) - p0);
                    const auto q0 = positionAt(corners[0], depth);
                    const AZ::Vector3 now = (positionAt(corners[1], depth) - q0).Cross(positionAt(corners[2], depth) - q0);
                    if (now.Dot(was) <= was.GetLengthSq() * 1e-3f)
                    {
                        return false;
                    }
                }
                struct Segment
                {
                    AZ::Vector2 m_from;
                    AZ::Vector2 m_to;
                    int m_tail;
                    int m_tip;
                };
                AZStd::vector<Segment> segments;
                for (const auto halfedge : border)
                {
                    const auto tail = HalfedgeVertexHandleAtTail(whiteBox, halfedge);
                    const auto tip = HalfedgeVertexHandleAtTip(whiteBox, halfedge);
                    const AZ::Vector3 was = VertexPosition(whiteBox, tip) - VertexPosition(whiteBox, tail);
                    const AZ::Vector3 from = positionAt(tail, depth);
                    const AZ::Vector3 to = positionAt(tip, depth);
                    if ((to - from).Dot(was) <= was.GetLengthSq() * 1e-3f)
                    {
                        return false;
                    }
                    if (planar)
                    {
                        segments.push_back({ flatten(from), flatten(to), tail.Index(), tip.Index() });
                    }
                }
                for (size_t i = 0; i < segments.size(); ++i)
                {
                    for (size_t j = i + 1; j < segments.size(); ++j)
                    {
                        const auto& a = segments[i];
                        const auto& b = segments[j];
                        // Neighbouring edges share a corner and always touch there.
                        if (a.m_tail == b.m_tail || a.m_tail == b.m_tip || a.m_tip == b.m_tail || a.m_tip == b.m_tip)
                        {
                            continue;
                        }
                        if (SegmentsTouch(a.m_from, a.m_to, b.m_from, b.m_to))
                        {
                            return false;
                        }
                    }
                }
                return true;
            };

            const float shallowest = extent * 1e-4f;
            if (!usable(shallowest))
            {
                return fail("This region folds over at even the thinnest inset. Select fewer or flatter faces.");
            }
            // 100 percent is the deepest usable depth, found by bisection because the outline decides it.
            float low = shallowest;
            float high = extent * 2.0f;
            if (usable(high))
            {
                low = high;
            }
            else
            {
                for (int step = 0; step < 32; ++step)
                {
                    const float middle = 0.5f * (low + high);
                    if (usable(middle))
                    {
                        low = middle;
                    }
                    else
                    {
                        high = middle;
                    }
                }
            }
            for (const auto& entry : directions)
            {
                moved.emplace(entry.first, VertexPosition(whiteBox, VertexHandle{ entry.first }) + entry.second * (low * amount));
            }
            return true;
        }

        bool ExtrudeInsetRegions(
            WhiteBoxMesh& whiteBox, const PolygonHandles& selection, const float amount, const bool inset,
            PolygonHandles& result, AZStd::string& error, PolygonHandles* preview)
        {
            result.clear();
            if (preview) { preview->clear(); }
            const auto fail = [&error](const char* message) { error = message; return false; };
            if (!AZ::IsFiniteFloat(amount) || (inset ? amount <= 0.0f || amount >= 1.0f : AZStd::abs(amount) < 1e-5f))
            {
                return fail(inset ? "Inset must be between 0 and 100 percent." : "Enter a nonzero extrusion distance.");
            }
            const auto polygons = MeshPolygonHandles(whiteBox);
            AZStd::vector<int> sourceGroup(whiteBox.mesh.n_faces(), -1);
            AZStd::vector<bool> selected(whiteBox.mesh.n_faces(), false);
            for (size_t i = 0; i < polygons.size(); ++i)
            {
                for (const auto face : polygons[i].m_faceHandles) { sourceGroup[face.Index()] = static_cast<int>(i); }
            }
            for (const auto& polygon : selection)
            {
                if (AZStd::find(polygons.begin(), polygons.end(), polygon) == polygons.end())
                {
                    return fail("The polygon selection is stale. Select the faces again.");
                }
                for (const auto face : polygon.m_faceHandles) { selected[face.Index()] = true; }
            }
            if (selection.empty()) { return fail("Select one or more polygons."); }

            struct Region
            {
                FaceHandles m_faces;
                HalfedgeHandles m_border;
                VertexHandles m_vertices;
                AZ::Vector3 m_normal = AZ::Vector3::CreateZero();
                AZ::Vector3 m_center = AZ::Vector3::CreateZero();
                AZStd::map<int, VertexHandle> m_top;
                bool m_offsetInset = false; //!< Even-width inset: only border vertices move, to these positions.
                AZStd::map<int, AZ::Vector3> m_offset;
            };
            AZStd::vector<Region> regions;
            AZStd::vector<int> faceRegion(whiteBox.mesh.n_faces(), -1);
            for (const auto seed : MeshFaceHandles(whiteBox))
            {
                if (!selected[seed.Index()] || faceRegion[seed.Index()] >= 0) { continue; }
                const int regionIndex = static_cast<int>(regions.size());
                regions.emplace_back();
                auto& region = regions.back();
                FaceHandles pending{seed};
                while (!pending.empty())
                {
                    const auto face = pending.back();
                    pending.pop_back();
                    if (faceRegion[face.Index()] >= 0) { continue; }
                    faceRegion[face.Index()] = regionIndex;
                    region.m_faces.push_back(face);
                    const auto vertices = FaceVertexHandles(whiteBox, face);
                    if (vertices.size() != 3) { return fail("The selection contains invalid triangles."); }
                    const auto points = FaceVertexPositions(whiteBox, face);
                    region.m_normal += (points[1] - points[0]).Cross(points[2] - points[0]);
                    for (const auto vertex : vertices)
                    {
                        if (AZStd::find(region.m_vertices.begin(), region.m_vertices.end(), vertex) == region.m_vertices.end())
                        {
                            region.m_vertices.push_back(vertex);
                            region.m_center += VertexPosition(whiteBox, vertex);
                        }
                    }
                    for (const auto halfedge : FaceHalfedgeHandles(whiteBox, face))
                    {
                        const auto other = HalfedgeFaceHandle(whiteBox, HalfedgeOppositeHalfedgeHandle(whiteBox, halfedge));
                        if (other.IsValid() && selected[other.Index()]) { pending.push_back(other); }
                        else { region.m_border.push_back(halfedge); }
                    }
                }
                if (region.m_border.empty() || region.m_normal.IsZero(1e-6f))
                {
                    return fail("Select a surface region with an open perimeter and a clear extrusion direction.");
                }
                region.m_normal.Normalize();
                region.m_center /= static_cast<float>(region.m_vertices.size());
                if (inset)
                {
                    // Proportional scaling suits one planar convex perimeter; anything else gets an even-width border.
                    const auto proportional = [&whiteBox, &region]()
                    {
                        AZStd::map<int, int> next;
                        for (const auto edge : region.m_border)
                        {
                            const auto a = HalfedgeVertexHandleAtTail(whiteBox, edge);
                            const auto b = HalfedgeVertexHandleAtTip(whiteBox, edge);
                            if (!next.emplace(a.Index(), b.Index()).second) { return false; }
                        }
                        int current = next.begin()->first;
                        const int first = current;
                        size_t visited = 0;
                        do
                        {
                            const auto it = next.find(current);
                            if (it == next.end() || visited++ >= next.size()) { return false; }
                            const auto after = next.find(it->second);
                            if (after == next.end()) { return false; }
                            const auto a = VertexPosition(whiteBox, VertexHandle{current});
                            const auto b = VertexPosition(whiteBox, VertexHandle{it->second});
                            const auto c = VertexPosition(whiteBox, VertexHandle{after->second});
                            if ((b - a).Cross(c - b).Dot(region.m_normal) < -1e-6f) { return false; }
                            current = it->second;
                        } while (current != first);
                        if (visited != next.size()) { return false; }
                        for (const auto vertex : region.m_vertices)
                        {
                            if (AZStd::abs((VertexPosition(whiteBox, vertex) - region.m_center).Dot(region.m_normal)) > 1e-4f)
                            {
                                return false;
                            }
                        }
                        return true;
                    };
                    if (!proportional())
                    {
                        region.m_offsetInset = true;
                        if (!OffsetInsetRegion(whiteBox, region.m_faces, region.m_border, region.m_vertices, amount, region.m_offset, error))
                        {
                            return false;
                        }
                    }
                }
            }

            auto candidate = CreateWhiteBoxMesh();
            AZStd::vector<VertexHandle> original(whiteBox.mesh.n_vertices());
            for (const auto vertex : MeshVertexHandles(whiteBox))
            {
                original[vertex.Index()] = AddVertex(*candidate, VertexPosition(whiteBox, vertex));
                if (VertexIsHidden(whiteBox, vertex)) { HideVertex(*candidate, original[vertex.Index()]); }
                CopyVertexBlend(*candidate, original[vertex.Index()], whiteBox, vertex);
            }
            for (auto& region : regions)
            {
                for (const auto vertex : region.m_vertices)
                {
                    const auto position = VertexPosition(whiteBox, vertex);
                    AZ::Vector3 moved = position + region.m_normal * amount;
                    if (inset && region.m_offsetInset)
                    {
                        const auto offset = region.m_offset.find(vertex.Index());
                        moved = offset != region.m_offset.end() ? offset->second : position;
                    }
                    else if (inset)
                    {
                        moved = region.m_center + (position - region.m_center) * (1.0f - amount);
                    }
                    const auto added = AddVertex(*candidate, moved);
                    region.m_top.emplace(vertex.Index(), added);
                    if (VertexIsHidden(whiteBox, vertex)) { HideVertex(*candidate, added); }
                    CopyVertexBlend(*candidate, added, whiteBox, vertex);
                }
            }
            struct RegionFace
            {
                AZStd::array<VertexHandle, 3> m_vertices;
                AZStd::array<AZ::Vector2, 3> m_uvs;
                FaceHandle m_source;
                FaceHandle m_result;
                size_t m_group = 0;
                bool m_keepUv = true;
                bool m_created = false; //!< Invented here, rather than carried over or translated.
            };
            AZStd::vector<RegionFace> faces;
            for (const auto face : MeshFaceHandles(whiteBox))
            {
                if (sourceGroup[face.Index()] < 0) { return fail("Repair the mesh's polygon groups first."); }
                RegionFace added{};
                added.m_source = face;
                added.m_group = static_cast<size_t>(sourceGroup[face.Index()]);
                const auto halfedges = FaceHalfedgeHandles(whiteBox, face);
                if (halfedges.size() != 3) { return fail("The mesh contains invalid triangles."); }
                for (size_t i = 0; i < 3; ++i)
                {
                    const auto vertex = HalfedgeVertexHandleAtTip(whiteBox, halfedges[i]);
                    added.m_vertices[i] = selected[face.Index()]
                        ? regions[faceRegion[face.Index()]].m_top.find(vertex.Index())->second : original[vertex.Index()];
                    added.m_uvs[i] = HalfedgeUV(whiteBox, halfedges[i]);
                }
                faces.push_back(added);
            }
            size_t groupCount = polygons.size();
            for (const auto& region : regions)
            {
                for (const auto halfedge : region.m_border)
                {
                    const auto a = HalfedgeVertexHandleAtTail(whiteBox, halfedge);
                    const auto b = HalfedgeVertexHandleAtTip(whiteBox, halfedge);
                    const auto source = HalfedgeFaceHandle(whiteBox, halfedge);
                    const AZStd::array<VertexHandle, 4> quad{
                        original[a.Index()], original[b.Index()],
                        region.m_top.find(b.Index())->second, region.m_top.find(a.Index())->second};
                    for (size_t i = 1; i < 3; ++i)
                    {
                        RegionFace side{};
                        side.m_vertices = {quad[0], quad[i], quad[i + 1]};
                        side.m_source = source;
                        side.m_group = groupCount;
                        side.m_keepUv = false;
                        side.m_created = true;
                        faces.push_back(side);
                    }
                    ++groupCount;
                }
            }
            // Validate only the side walls this operation creates; pre-existing slivers must not block the extrude.
            for (const auto& face : faces)
            {
                if (!face.m_created)
                {
                    continue;
                }
                const auto& v = face.m_vertices;
                const auto a = VertexPosition(*candidate, v[0]);
                AZ::Vector3 normal;
                if (!TryCalculateTriangleNormal(
                        VertexPosition(*candidate, v[1]) - a, VertexPosition(*candidate, v[2]) - a, normal))
                {
                    return fail("The new side faces collapse at this amount. Move a little further.");
                }
            }
            // Adjacent source triangles may not be listed in insertion order. Retry uninserted
            // triangles until the connected fans are assembled; never duplicate vertices on failure.
            size_t remaining = faces.size();
            while (remaining)
            {
                const size_t before = remaining;
                for (auto& face : faces)
                {
                    if (face.m_result.IsValid()) { continue; }
                    const auto& v = face.m_vertices;
                    const auto added = candidate->mesh.add_face(om_vh(v[0]), om_vh(v[1]), om_vh(v[2]));
                    if (!added.is_valid()) { continue; }
                    face.m_result = wb_fh(added);
                    --remaining;
                }
                if (remaining == before) { return fail("This selection would create invalid face connections. The mesh was left unchanged."); }
            }
            PolygonPropertyHandle property;
            candidate->mesh.get_property_handle(property, PolygonProps);
            auto& mapping = candidate->mesh.property(property);
            AZStd::vector<FaceHandlesInternal> groups(groupCount);
            CalculateNormals(*candidate);
            FaceHandles projectedFaces;
            for (const auto& face : faces)
            {
                groups[face.m_group].push_back(om_fh(face.m_result));
                CopyFaceAttributes(*candidate, face.m_result, whiteBox, face.m_source);
                if (face.m_keepUv)
                {
                    for (size_t i = 0; i < 3; ++i)
                    {
                        const auto& v = face.m_vertices;
                        const auto edge = candidate->mesh.find_halfedge(om_vh(v[(i + 2) % 3]), om_vh(v[i]));
                        candidate->mesh.set_texcoord2D(edge, face.m_uvs[i]);
                    }
                }
                else
                {
                    projectedFaces.push_back(face.m_result);
                }
            }
            // After the copy, so new walls take the projection of the face they grew from.
            CalculatePlanarUVs(*candidate, projectedFaces);
            for (const auto& group : groups)
            {
                for (const auto face : group) { mapping[face] = group; }
            }
            for (size_t i = 0; i < polygons.size(); ++i)
            {
                if (selected[polygons[i].m_faceHandles.front().Index()]) { result.push_back(PolygonHandleFromInternal(groups[i])); }
            }
            if (preview)
            {
                *preview = result;
                for (size_t i = polygons.size(); i < groups.size(); ++i)
                {
                    preview->push_back(PolygonHandleFromInternal(groups[i]));
                }
            }
            RemoveIsolatedVertices(*candidate);
            whiteBox.mesh = candidate->mesh;
            return true;
        }

        bool BevelEdges(
            WhiteBoxMesh& whiteBox, const EdgeHandles& edges, float width, int segments, AZStd::string& error, float profile)
        {
            error.clear();
            const auto fail = [&error](const char* message) { error = message; return false; };
            if (!(width > 0.0f && width < AZ::Constants::FloatMax) || segments < 1 || segments > 32 || !(profile >= 0.05f && profile <= 0.95f))
            {
                return fail("Use positive width, 1-32 segments, and a profile between 0.05 and 0.95.");
            }
            if (edges.empty()) { return fail("Select sharp edges in Transform mode."); }
            const auto meshEdges = MeshEdgeHandles(whiteBox);
            AZStd::unordered_set<int> selected;
            AZStd::unordered_set<int> endpointsUsed;
            AZStd::map<int, int> selectedAtCorner;
            EdgeHandles orderedEdges;
            for (const auto edge : edges)
            {
                if (AZStd::find(meshEdges.begin(), meshEdges.end(), edge) == meshEdges.end())
                {
                    return fail("The edge selection is stale. Select the edges again.");
                }
                if (!selected.insert(edge.Index()).second) { continue; }
                orderedEdges.push_back(edge);
                for (const auto vertex : EdgeVertexHandles(whiteBox, edge))
                {
                    endpointsUsed.insert(vertex.Index());
                    ++selectedAtCorner[vertex.Index()];
                }
            }
            struct BevelPolygon
            {
                PolygonHandle m_polygon;
                VertexHandles m_border;
                AZ::Vector3 m_normal;
                AZ::Data::AssetId m_material;
                AZ::u32 m_paint;
                UvProjection m_uvProjection;
                AZ::u32 m_smoothing;
            };
            AZStd::map<int, BevelPolygon> polygons;
            AZStd::map<int, PolygonHandles> incident;
            for (const auto& polygon : MeshPolygonHandles(whiteBox))
            {
                for (const auto& border : PolygonBorderVertexHandles(whiteBox, polygon))
                {
                    for (const auto vertex : border) { incident[vertex.Index()].push_back(polygon); }
                }
            }
            const auto loadPolygon = [&](const PolygonHandle& polygon) -> BevelPolygon*
            {
                const int key = polygon.m_faceHandles.front().Index();
                if (const auto found = polygons.find(key); found != polygons.end()) { return &found->second; }
                const auto borders = PolygonBorderVertexHandles(whiteBox, polygon);
                if (borders.size() != 1 || borders[0].size() < 3)
                {
                    error = "Bevel requires faces without holes.";
                    return nullptr;
                }
                BevelPolygon data{polygon, borders[0], PolygonNormal(whiteBox, polygon),
                    FaceMaterial(whiteBox, polygon.m_faceHandles.front()), FacePaintColor(whiteBox, polygon.m_faceHandles.front()),
                    FaceUvProjection(whiteBox, polygon.m_faceHandles.front()), FaceSmoothingGroups(whiteBox, polygon.m_faceHandles.front())};
                for (const auto face : polygon.m_faceHandles)
                {
                    if (FaceMaterial(whiteBox, face) != data.m_material || FacePaintColor(whiteBox, face) != data.m_paint)
                    {
                        error = "A face has different materials or paint on its triangles. Make them match before beveling.";
                        return nullptr;
                    }
                }
                const auto origin = VertexPosition(whiteBox, data.m_border.front());
                float scale = 0.0f;
                for (const auto vertex : data.m_border)
                {
                    scale = AZStd::max(scale, (VertexPosition(whiteBox, vertex) - origin).GetLength());
                }
                for (size_t i = 0; i < data.m_border.size(); ++i)
                {
                    const auto p = VertexPosition(whiteBox, data.m_border[i]);
                    if (!p.IsFinite() || AZStd::abs((p - origin).Dot(data.m_normal)) > scale * 1e-5f)
                    {
                        error = "Bevel requires flat neighboring faces.";
                        return nullptr;
                    }
                }
                return &polygons.emplace(key, AZStd::move(data)).first->second;
            };
            auto candidate = CloneMesh(whiteBox);
            AZStd::map<int, AZStd::map<int, VertexHandles>> replacements;
            struct NewPolygon
            {
                VertexHandles m_vertices;
                AZ::Vector3 m_normal;
                AZ::Data::AssetId m_material;
                AZ::u32 m_paint;
                UvProjection m_uvProjection;
                AZ::u32 m_smoothing;
            };
            AZStd::vector<NewPolygon> strips;
            // One shared vertex pool per original corner joins the ends of
            // adjacent bevel strips instead of generating overlapping caps.
            AZStd::map<int, VertexHandles> cornerVertices;
            const auto cornerVertex = [&](VertexHandle original, const AZ::Vector3& point)
            {
                auto& pool = cornerVertices[original.Index()];
                for (const auto vertex : pool)
                {
                    if (VertexPosition(*candidate, vertex).IsClose(point, 1e-6f)) { return vertex; }
                }
                const auto vertex = AddVertex(*candidate, point);
                pool.push_back(vertex);
                return vertex;
            };
            const auto edgeSelected = [&](VertexHandle a, VertexHandle b)
            {
                for (const auto edge : orderedEdges)
                {
                    const auto pair = EdgeVertexHandles(whiteBox, edge);
                    if ((pair[0] == a && pair[1] == b) || (pair[1] == a && pair[0] == b)) { return true; }
                }
                return false;
            };
            for (const auto index : endpointsUsed)
            {
                const VertexHandle vertex{index};
                const auto origin = VertexPosition(whiteBox, vertex);
                const auto& around = incident[index];
                if (around.size() < 2)
                {
                    return fail("Bevel needs an edge between two faces, not an open boundary.");
                }
                for (const auto& polygon : around)
                {
                    auto* data = loadPolygon(polygon);
                    if (!data) { return false; }
                    const auto& border = data->m_border;
                    const auto found = AZStd::find(border.begin(), border.end(), vertex);
                    if (found == border.end()) { return fail("Invalid bevel corner."); }
                    const size_t i = static_cast<size_t>(found - border.begin());
                    const auto prev = border[(i + border.size() - 1) % border.size()];
                    const auto next = border[(i + 1) % border.size()];
                    const bool before = edgeSelected(vertex, prev);
                    const bool after = edgeSelected(vertex, next);
                    if (!before && !after)
                    {
                        if (around.size() > 3)
                        {
                            return fail("At a corner with more than three faces, select edges touching every incident face.");
                        }
                        continue;
                    }
                    const auto u = VertexPosition(whiteBox, prev) - origin;
                    const auto v = VertexPosition(whiteBox, next) - origin;
                    const auto un = u.GetNormalizedSafe();
                    const auto vn = v.GetNormalizedSafe();
                    // Signed offsets follow the face interior even at reflex corners.
                    const float sine = vn.Cross(un).Dot(data->m_normal);
                    if (AZStd::abs(sine) < 1e-5f)
                    {
                        // A split straight edge has a well-defined offset when both halves
                        // are selected. Keep its vertex to join the two strips exactly.
                        if (before && after && un.Dot(vn) < -0.9999f)
                        {
                            const auto point = origin + data->m_normal.Cross(vn) * width;
                            replacements[polygon.m_faceHandles.front().Index()][index] = {cornerVertex(vertex, point)};
                            continue;
                        }
                        return fail("Select both edge segments at a straight corner, or remove the collapsed corner.");
                    }
                    const float distance = width / sine;
                    if ((after && distance >= u.GetLength() - 1e-6f) ||
                        (before && distance >= v.GetLength() - 1e-6f))
                    {
                        return fail("Bevel width reaches a neighboring vertex. Reduce the width.");
                    }
                    const auto point = origin + (after ? un * distance : AZ::Vector3::CreateZero()) +
                        (before ? vn * distance : AZ::Vector3::CreateZero());
                    replacements[polygon.m_faceHandles.front().Index()][index] = {cornerVertex(vertex, point)};
                }
            }
            // At a three-edge junction all strips share one profile centre, or the corner grooves inward.
            AZStd::map<int, AZ::Vector3> cornerCenters;
            for (const auto& item : selectedAtCorner)
            {
                if (item.second != 3) { continue; }
                const auto& around = incident[item.first];
                if (around.size() != 3) { continue; }
                AZ::Vector3 sum = AZ::Vector3::CreateZero();
                for (const auto& polygon : around)
                {
                    const auto& point = replacements[polygon.m_faceHandles.front().Index()][item.first];
                    if (point.size() != 1) { return fail("Cannot construct the bevel corner profile."); }
                    sum += VertexPosition(*candidate, point.front());
                }
                cornerCenters[item.first] = (sum - VertexPosition(whiteBox, VertexHandle{item.first})) * 0.5f;
            }
            // End arcs not used by a surviving original face become corner patches.
            AZStd::map<int, AZStd::vector<VertexHandles>> cornerArcs;
            for (const auto edge : orderedEdges)
            {
                const auto faces = EdgeFaceHandles(whiteBox, edge);
                if (faces.size() != 2) { return fail("Bevel needs an edge between two faces, not an open boundary."); }
                const auto side0 = FacePolygonHandle(whiteBox, faces[0]);
                const auto side1 = FacePolygonHandle(whiteBox, faces[1]);
                if (side0 == side1) { return fail("Select polygon edges, not internal triangle diagonals."); }
                auto* first = loadPolygon(side0);
                auto* second = loadPolygon(side1);
                if (!first || !second) { return false; }
                VertexHandle a;
                VertexHandle b;
                for (const auto& border : PolygonBorderHalfedgeHandles(whiteBox, side0))
                {
                    for (const auto halfedge : border)
                    {
                        if (HalfedgeEdgeHandle(whiteBox, halfedge) == edge)
                        {
                            a = HalfedgeVertexHandleAtTail(whiteBox, halfedge);
                            b = HalfedgeVertexHandleAtTip(whiteBox, halfedge);
                        }
                    }
                }
                if (!a.IsValid() || !b.IsValid()) { return fail("Invalid polygon edge."); }
                AZStd::array<VertexHandles, 2> arcs;
                const AZStd::array<VertexHandle, 2> ends{{a, b}};
                for (size_t end = 0; end < 2; ++end)
                {
                    const auto vertex = ends[end];
                    const auto origin = VertexPosition(whiteBox, vertex);
                    const auto p0 = replacements[side0.m_faceHandles.front().Index()][vertex.Index()];
                    const auto p1 = replacements[side1.m_faceHandles.front().Index()][vertex.Index()];
                    if (p0.empty() || p1.empty()) { return fail("Invalid bevel boundary."); }
                    const auto v0 = VertexPosition(*candidate, p0.front()) - origin;
                    const auto v1 = VertexPosition(*candidate, p1.front()) - origin;
                    const float sideDistance0 = first->m_normal.Dot(v1);
                    const float sideDistance1 = second->m_normal.Dot(v0);
                    if (AZStd::abs(sideDistance0) <= width * 1e-5f || AZStd::abs(sideDistance1) <= width * 1e-5f ||
                        (sideDistance0 < 0.0f) != (sideDistance1 < 0.0f))
                    {
                        return fail("Select sharp edges between consistently oriented faces; flat edges cannot be beveled.");
                    }
                    for (int segment = 0; segment <= segments; ++segment)
                    {
                        const float angle = AZ::Constants::HalfPi * static_cast<float>(segment) / segments;
                        const float exponent = 2.0f * (1.0f - profile);
                        const float x = segment == segments ? 0.0f : 1.0f - powf(sinf(angle), exponent);
                        const float y = segment == 0 ? 0.0f : 1.0f - powf(AZStd::max(cosf(angle), 0.0f), exponent);
                        auto point = origin + v0 * x + v1 * y;
                        if (const auto center = cornerCenters.find(vertex.Index()); center != cornerCenters.end())
                        {
                            // Preserve the same superellipse used by the corner
                            // grid, including its exact shared boundary vertices.
                            point = center->second +
                                (origin + v0 - center->second) * (1.0f - y) +
                                (origin + v1 - center->second) * (1.0f - x);
                        }
                        if (segment == 0) { arcs[end].push_back(p0.front()); }
                        else if (segment == segments) { arcs[end].push_back(p1.front()); }
                        else { arcs[end].push_back(cornerVertex(vertex, point)); }
                    }
                    bool capped = false;
                    for (const auto& cap : incident[vertex.Index()])
                    {
                        if (cap == side0 || cap == side1) { continue; }
                        const int key = cap.m_faceHandles.front().Index();
                        if (replacements[key].count(vertex.Index())) { continue; }
                        auto* data = loadPolygon(cap);
                        if (!data) { return false; }
                        const auto& border = data->m_border;
                        const auto found = AZStd::find(border.begin(), border.end(), vertex);
                        const size_t i = static_cast<size_t>(found - border.begin());
                        const auto prev = border[(i + border.size() - 1) % border.size()];
                        const auto towardPrev = VertexPosition(whiteBox, prev) - origin;
                        auto arc = arcs[end];
                        if (towardPrev.Cross(v1).GetLengthSq() < towardPrev.Cross(v0).GetLengthSq())
                        {
                            AZStd::reverse(arc.begin(), arc.end());
                        }
                        replacements[key][vertex.Index()] = AZStd::move(arc);
                        capped = true;
                    }
                    if (!capped && incident[vertex.Index()].size() >= 3)
                    {
                        auto arc = arcs[end];
                        // Reverse the strip end boundary so the patch closes it.
                        if (end == 0) { AZStd::reverse(arc.begin(), arc.end()); }
                        cornerArcs[vertex.Index()].push_back(AZStd::move(arc));
                    }
                }
                for (int segment = 0; segment < segments; ++segment)
                {
                    strips.push_back({{arcs[1][segment], arcs[0][segment], arcs[0][segment + 1], arcs[1][segment + 1]},
                        (first->m_normal + second->m_normal).GetNormalizedSafe(), first->m_material, first->m_paint,
                        first->m_uvProjection, first->m_smoothing | second->m_smoothing});
                }
            }
            AZStd::vector<NewPolygon> patches;
            for (const auto& item : cornerArcs)
            {
                const auto& arcs = item.second;
                if (arcs.size() == 2)
                {
                    // Two selected edges form a miter. Their end arcs must coincide.
                    if (arcs[0].size() != arcs[1].size()) { return fail("Incompatible bevel miter."); }
                    for (size_t i = 0; i < arcs[0].size(); ++i)
                    {
                        if (arcs[0][i] != arcs[1][arcs[1].size() - 1 - i])
                        {
                            return fail("This asymmetric corner needs a smaller selection or a different width.");
                        }
                    }
                    continue;
                }
                if (arcs.size() < 3) { return fail("Unsupported bevel corner junction."); }
                VertexHandles ring = arcs.front();
                AZStd::unordered_set<size_t> used{0};
                while (used.size() < arcs.size())
                {
                    bool joined = false;
                    for (size_t i = 1; i < arcs.size(); ++i)
                    {
                        if (!used.count(i) && arcs[i].front() == ring.back())
                        {
                            ring.insert(ring.end(), arcs[i].begin() + 1, arcs[i].end());
                            used.insert(i);
                            joined = true;
                            break;
                        }
                    }
                    if (!joined) { return fail("Cannot join the bevel corner patch."); }
                }
                if (ring.front() != ring.back()) { return fail("The bevel corner patch is not closed."); }
                ring.pop_back();
                AZ::Vector3 normal = AZ::Vector3::CreateZero();
                for (const auto& polygon : incident[item.first]) { normal += PolygonNormal(whiteBox, polygon); }
                normal = normal.GetNormalizedSafe();
                const auto source = incident[item.first].front().m_faceHandles.front();
                const auto material = FaceMaterial(whiteBox, source);
                const auto paint = FacePaintColor(whiteBox, source);
                const auto uvProjection = FaceUvProjection(whiteBox, source);
                AZ::u32 smoothing = 0;
                for (const auto& polygon : incident[item.first]) { smoothing |= FaceSmoothingGroups(whiteBox, polygon.m_faceHandles.front()); }
                const auto centerIt = cornerCenters.find(item.first);
                if (centerIt == cornerCenters.end())
                {
                    // General junctions share a single interior vertex and the exact strip
                    // boundary. The specialized trihedral grid below keeps its rounded profile.
                    AZ::Vector3 center = AZ::Vector3::CreateZero();
                    for (const auto vertex : ring) { center += VertexPosition(*candidate, vertex); }
                    center /= static_cast<float>(ring.size());
                    const auto centerVertex = cornerVertex(VertexHandle{item.first}, center);
                    for (size_t i = 0; i < ring.size(); ++i)
                    {
                        patches.push_back({{ring[i], ring[(i + 1) % ring.size()], centerVertex},
                            normal, material, paint, uvProjection, smoothing});
                    }
                    continue;
                }
                const auto center = centerIt->second;
                const AZStd::array<AZ::Vector3, 3> axes{{
                    VertexPosition(*candidate, ring[0]) - center,
                    VertexPosition(*candidate, ring[segments]) - center,
                    VertexPosition(*candidate, ring[2 * segments]) - center}};
                const float exponent = 2.0f * (1.0f - profile);
                const auto gridVertex = [&](const AZ::Vector3& barycentric)
                {
                    const float a = barycentric.GetX();
                    const float b = barycentric.GetY();
                    const float c = barycentric.GetZ();
                    // Reuse strip boundary handles exactly: no cracks, duplicate
                    // vertices, or T-junctions at the grid perimeter.
                    const auto sample = [&](float value)
                    {
                        return AZStd::clamp(static_cast<int>(value * segments + 0.5f), 0, segments);
                    };
                    if (c < 1e-6f) { return ring[sample(b)]; }
                    if (a < 1e-6f) { return ring[segments + sample(c)]; }
                    if (b < 1e-6f) { return ring[(3 * segments - sample(c)) % (3 * segments)]; }
                    const auto direction = AZ::Vector3(
                        sinf(AZ::Constants::HalfPi * a), sinf(AZ::Constants::HalfPi * b),
                        sinf(AZ::Constants::HalfPi * c)).GetNormalizedSafe();
                    const auto point = center + axes[0] * powf(direction.GetX(), exponent) +
                        axes[1] * powf(direction.GetY(), exponent) + axes[2] * powf(direction.GetZ(), exponent);
                    return cornerVertex(VertexHandle{item.first}, point);
                };
                if (segments % 2 == 0)
                {
                    // Three quad grids meet at one valence-three center. Each
                    // outer side uses half of an existing strip end arc.
                    const int size = segments / 2;
                    const AZStd::array<AZ::Vector3, 3> corners{{
                        AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ()}};
                    for (int sector = 0; sector < 3; ++sector)
                    {
                        const auto a = corners[sector];
                        const auto b = (a + corners[(sector + 1) % 3]) * 0.5f;
                        const auto c = AZ::Vector3(1.0f / 3.0f);
                        const auto d = (a + corners[(sector + 2) % 3]) * 0.5f;
                        AZStd::vector<VertexHandles> grid(size + 1);
                        for (int row = 0; row <= size; ++row)
                        {
                            const float v = static_cast<float>(row) / size;
                            for (int column = 0; column <= size; ++column)
                            {
                                const float u = static_cast<float>(column) / size;
                                grid[row].push_back(gridVertex(
                                    a * ((1.0f - u) * (1.0f - v)) + b * (u * (1.0f - v)) +
                                    c * (u * v) + d * ((1.0f - u) * v)));
                            }
                        }
                        for (int row = 0; row < size; ++row)
                        {
                            for (int column = 0; column < size; ++column)
                            {
                                patches.push_back({{
                                    grid[row][column], grid[row][column + 1],
                                    grid[row + 1][column + 1], grid[row + 1][column]}, normal, material, paint, uvProjection, smoothing});
                            }
                        }
                    }
                }
                else
                {
                    // Odd boundaries: two equal runs and a middle edge, so the corner stays symmetric
                    // with a single central triangle.
                    const int size = segments / 2;
                    const AZStd::array<AZ::Vector3, 3> corners{{
                        AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ()}};
                    AZStd::array<AZ::Vector3, 3> inner;
                    for (int sector = 0; sector < 3; ++sector)
                    {
                        inner[sector] = (AZ::Vector3(static_cast<float>(size)) + corners[sector]) /
                            static_cast<float>(3 * size + 1);
                    }
                    if (size > 0)
                    {
                        const float fraction = static_cast<float>(size) / segments;
                        for (int sector = 0; sector < 3; ++sector)
                        {
                            const int next = (sector + 1) % 3;
                            const auto a = corners[sector];
                            const auto b = a.Lerp(corners[next], fraction);
                            const auto c = inner[sector];
                            const auto d = a.Lerp(corners[(sector + 2) % 3], fraction);
                            AZStd::vector<VertexHandles> grid(size + 1);
                            for (int row = 0; row <= size; ++row)
                            {
                                const float v = static_cast<float>(row) / size;
                                for (int column = 0; column <= size; ++column)
                                {
                                    const float u = static_cast<float>(column) / size;
                                    grid[row].push_back(gridVertex(
                                        a * ((1.0f - u) * (1.0f - v)) + b * (u * (1.0f - v)) +
                                        c * (u * v) + d * ((1.0f - u) * v)));
                                }
                            }
                            for (int row = 0; row < size; ++row)
                            {
                                for (int column = 0; column < size; ++column)
                                {
                                    patches.push_back({{
                                        grid[row][column], grid[row][column + 1],
                                        grid[row + 1][column + 1], grid[row + 1][column]}, normal, material, paint, uvProjection, smoothing});
                                }
                            }
                            const auto opposite = corners[next].Lerp(a, fraction);
                            auto left = gridVertex(b);
                            auto right = gridVertex(opposite);
                            for (int row = 1; row <= size; ++row)
                            {
                                const float v = static_cast<float>(row) / size;
                                const auto nextLeft = gridVertex(b.Lerp(inner[sector], v));
                                const auto nextRight = gridVertex(opposite.Lerp(inner[next], v));
                                patches.push_back({{left, right, nextRight, nextLeft}, normal, material, paint, uvProjection, smoothing});
                                left = nextLeft;
                                right = nextRight;
                            }
                        }
                    }
                    patches.push_back({{
                        gridVertex(inner[0]), gridVertex(inner[1]), gridVertex(inner[2])}, normal, material, paint, uvProjection, smoothing});
                }
            }
            FaceHandles removed;
            AZStd::vector<NewPolygon> rebuilt;
            for (const auto& item : polygons)
            {
                const auto& data = item.second;
                VertexHandles border;
                const auto& changed = replacements[item.first];
                for (const auto vertex : data.m_border)
                {
                    const auto replacement = changed.find(vertex.Index());
                    if (replacement == changed.end()) { border.push_back(vertex); }
                    else { border.insert(border.end(), replacement->second.begin(), replacement->second.end()); }
                }
                // Even a simple outline can hide an over-wide offset that reverses an
                // original edge. Test the surviving span before triangulating concave faces.
                for (size_t i = 0; i < data.m_border.size(); ++i)
                {
                    const auto a = data.m_border[i];
                    const auto b = data.m_border[(i + 1) % data.m_border.size()];
                    const auto aIt = changed.find(a.Index());
                    const auto bIt = changed.find(b.Index());
                    const auto newA = aIt == changed.end() ? a : aIt->second.back();
                    const auto newB = bIt == changed.end() ? b : bIt->second.front();
                    const auto original = VertexPosition(whiteBox, b) - VertexPosition(whiteBox, a);
                    if ((VertexPosition(*candidate, newB) - VertexPosition(*candidate, newA)).Dot(original) <= 0.0f)
                    {
                        return fail("Bevel width collapses a neighboring edge. Reduce the width.");
                    }
                }
                rebuilt.push_back({AZStd::move(border), data.m_normal, data.m_material, data.m_paint, data.m_uvProjection, data.m_smoothing});
                removed.insert(removed.end(), data.m_polygon.m_faceHandles.begin(), data.m_polygon.m_faceHandles.end());
            }
            rebuilt.insert(rebuilt.end(), strips.begin(), strips.end());
            rebuilt.insert(rebuilt.end(), patches.begin(), patches.end());
            // Project each boundary in its own orientation and reject crossings explicitly.
            // Ear clipping permits concave outlines and retained collinear border vertices.
            using Triangles = AZStd::vector<AZStd::array<size_t, 3>>;
            AZStd::vector<Triangles> triangulations;
            for (const auto& polygon : rebuilt)
            {
                const auto& vertices = polygon.m_vertices;
                if (vertices.size() < 3 || polygon.m_normal.GetLengthSq() < 0.5f)
                {
                    return fail("Bevel would create a collapsed face. Reduce the width.");
                }
                const auto base = VertexPosition(*candidate, vertices.front());
                float extent = 0.0f;
                for (const auto vertex : vertices)
                {
                    const auto point = VertexPosition(*candidate, vertex);
                    if (!point.IsFinite()) { return fail("Bevel produced an invalid vertex."); }
                    extent = AZStd::max(extent, (point - base).GetLength());
                }
                if (!(extent > 0.0f)) { return fail("Bevel would create a collapsed face."); }
                const auto reference = AZStd::abs(polygon.m_normal.GetZ()) < 0.9f
                    ? AZ::Vector3::CreateAxisZ() : AZ::Vector3::CreateAxisX();
                const auto u = polygon.m_normal.Cross(reference).GetNormalizedSafe();
                const auto v = polygon.m_normal.Cross(u);
                AZStd::vector<AZ::Vector2> points;
                for (const auto vertex : vertices)
                {
                    const auto offset = (VertexPosition(*candidate, vertex) - base) / extent;
                    points.emplace_back(offset.Dot(u), offset.Dot(v));
                }
                const auto cross = [](const AZ::Vector2& a, const AZ::Vector2& b)
                {
                    return double(a.GetX()) * b.GetY() - double(a.GetY()) * b.GetX();
                };
                const auto onSegment = [](const AZ::Vector2& a, const AZ::Vector2& b, const AZ::Vector2& p)
                {
                    return p.GetX() >= AZStd::min(a.GetX(), b.GetX()) && p.GetX() <= AZStd::max(a.GetX(), b.GetX()) &&
                        p.GetY() >= AZStd::min(a.GetY(), b.GetY()) && p.GetY() <= AZStd::max(a.GetY(), b.GetY());
                };
                bool convex = true;
                for (size_t i = 0; i < points.size(); ++i)
                {
                    const auto& a = points[i];
                    const auto& b = points[(i + 1) % points.size()];
                    if ((b - a).GetLengthSq() <= 1e-20f) { return fail("Bevel would collapse an edge. Reduce the width."); }
                    convex = convex && cross(b - a, points[(i + 2) % points.size()] - b) > EarMinimumTurn;
                    for (size_t j = i + 1; j < points.size(); ++j)
                    {
                        if (j == i + 1 || (i == 0 && j + 1 == points.size())) { continue; }
                        const auto& c = points[j];
                        const auto& d = points[(j + 1) % points.size()];
                        const auto abC = cross(b - a, c - a);
                        const auto abD = cross(b - a, d - a);
                        const auto cdA = cross(d - c, a - c);
                        const auto cdB = cross(d - c, b - c);
                        if (((abC < 0.0 && abD > 0.0) || (abC > 0.0 && abD < 0.0)) &&
                            ((cdA < 0.0 && cdB > 0.0) || (cdA > 0.0 && cdB < 0.0)))
                        {
                            return fail("Bevel would overlap a face. Reduce the width.");
                        }
                        if ((abC == 0.0 && onSegment(a, b, c)) || (abD == 0.0 && onSegment(a, b, d)) ||
                            (cdA == 0.0 && onSegment(c, d, a)) || (cdB == 0.0 && onSegment(c, d, b)))
                        {
                            return fail("Bevel would pinch a face. Reduce the width.");
                        }
                    }
                }
                Triangles triangles;
                if (convex)
                {
                    // Preserve the existing diagonals on convex strips and corner grids.
                    for (size_t i = 1; i + 1 < vertices.size(); ++i) { triangles.push_back({{0, i, i + 1}}); }
                }
                else if (!TriangulateLoop(points, triangles))
                {
                    return fail("Bevel cannot triangulate this face safely. Reduce the width.");
                }
                for (const auto& triangle : triangles)
                {
                    if (cross(points[triangle[1]] - points[triangle[0]], points[triangle[2]] - points[triangle[0]]) <= EarMinimumTurn)
                    {
                        return fail("Bevel would collapse or invert a face. Reduce the width.");
                    }
                }
                triangulations.push_back(AZStd::move(triangles));
            }
            RemoveFaces(*candidate, removed);
            FaceHandles added;
            PolygonPropertyHandle property;
            candidate->mesh.get_property_handle(property, PolygonProps);
            auto& groups = candidate->mesh.property(property);
            for (size_t polygonIndex = 0; polygonIndex < rebuilt.size(); ++polygonIndex)
            {
                const auto& polygon = rebuilt[polygonIndex];
                FaceHandlesInternal group;
                const auto& vertices = polygon.m_vertices;
                for (const auto& triangle : triangulations[polygonIndex])
                {
                    const auto face = candidate->mesh.add_face(
                        om_vh(vertices[triangle[0]]), om_vh(vertices[triangle[1]]), om_vh(vertices[triangle[2]]));
                    if (!face.is_valid()) { return fail("Bevel would create invalid face connections. The mesh was left unchanged."); }
                    group.push_back(face);
                    const auto handle = wb_fh(face);
                    SetFaceMaterial(*candidate, handle, polygon.m_material);
                    SetFacePaintColor(*candidate, handle, polygon.m_paint);
                    SetFaceUvProjection(*candidate, handle, polygon.m_uvProjection);
                    SetFaceSmoothingGroups(*candidate, handle, polygon.m_smoothing);
                    added.push_back(handle);
                }
                for (const auto face : group) { groups[face] = group; }
            }
            CalculateNormals(*candidate);
            CalculatePlanarUVs(*candidate, added);
            RemoveIsolatedVertices(*candidate);
            whiteBox.mesh = candidate->mesh;
            return true;
        }

        bool KnifeCut(
            WhiteBoxMesh& whiteBox, const KnifePoint& start, KnifePoint& end, const AZ::Vector3& viewDirection,
            AZStd::vector<AZ::Vector3>& lines, AZStd::string& error)
        {
            lines.clear();
            error.clear();
            const auto fail = [&error, &lines](const char* message)
            {
                error = message;
                lines.clear();
                return false;
            };
            const auto faces = MeshFaceHandles(whiteBox);
            if (AZStd::find(faces.begin(), faces.end(), start.m_face) == faces.end() ||
                AZStd::find(faces.begin(), faces.end(), end.m_face) == faces.end() ||
                !start.m_position.IsFinite() || !end.m_position.IsFinite() || !viewDirection.IsFinite())
            {
                return fail("Choose two points on the active mesh.");
            }
            const auto delta = end.m_position - start.m_position;
            const auto view = viewDirection.GetNormalizedSafe();
            const auto axis = (delta - view * delta.Dot(view)).GetNormalizedSafe();
            const float span = delta.Dot(axis);
            const float epsilon = AZStd::max(delta.GetLength() * 1e-5f, 1e-7f);
            if (view.IsZero() || span <= epsilon)
            {
                return fail("Move the next point farther away in the viewport.");
            }
            const auto planeNormal = axis.Cross(view).GetNormalizedSafe();
            auto candidate = CloneMesh(whiteBox);
            auto& mesh = candidate->mesh;
            // Track each triangle's original polygon and source face while OpenMesh appends faces.
            // PolygonProps is rebuilt only after the full path succeeds.
            AZStd::map<int, int> labels;
            AZStd::map<int, int> origins;
            for (const auto& polygon : MeshPolygonHandles(whiteBox))
            {
                for (const auto face : polygon.m_faceHandles)
                {
                    labels[face.Index()] = polygon.m_faceHandles.front().Index();
                    origins[face.Index()] = face.Index();
                }
            }
            struct SourceTriangle
            {
                AZStd::array<AZ::Vector3, 3> m_points;
                AZStd::array<AZ::Vector2, 3> m_uvs;
                AZ::Data::AssetId m_material;
                AZ::u32 m_paint;
                UvProjection m_uvProjection;
                AZ::u32 m_smoothing;
                int m_label;
                int m_origin;
            };
            const auto barycentric = [epsilon](
                const AZStd::array<AZ::Vector3, 3>& points, const AZ::Vector3& p, AZ::Vector3& weights)
            {
                const auto n = (points[1] - points[0]).Cross(points[2] - points[0]);
                const float areaSq = n.GetLengthSq();
                if (areaSq <= 0.0f || AZStd::abs((p - points[0]).Dot(n)) > epsilon * n.GetLength())
                {
                    return false;
                }
                weights.SetX((points[1] - p).Cross(points[2] - p).Dot(n) / areaSq);
                weights.SetY((points[2] - p).Cross(points[0] - p).Dot(n) / areaSq);
                weights.SetZ(1.0f - weights.GetX() - weights.GetY());
                return weights.GetMinElement() >= -1e-4f;
            };
            const auto snapshot = [&](const FaceHandle face)
            {
                SourceTriangle source{};
                const auto halfedges = FaceHalfedgeHandles(*candidate, face);
                for (size_t i = 0; i < 3; ++i)
                {
                    source.m_points[i] = VertexPosition(*candidate, HalfedgeVertexHandleAtTip(*candidate, halfedges[i]));
                    source.m_uvs[i] = HalfedgeUV(*candidate, halfedges[i]);
                }
                source.m_material = FaceMaterial(*candidate, face);
                source.m_paint = FacePaintColor(*candidate, face);
                source.m_uvProjection = FaceUvProjection(*candidate, face);
                source.m_smoothing = FaceSmoothingGroups(*candidate, face);
                source.m_label = labels.find(face.Index())->second;
                source.m_origin = origins.find(face.Index())->second;
                return source;
            };
            // Split one triangle or edge. Splitting an edge updates BOTH incident faces, so the
            // cut never leaves T-junctions. Restore attributes using each source triangle's UV chart.
            const auto insert = [&](const FaceHandle face, const AZ::Vector3& position)
            {
                const auto vertices = FaceVertexHandles(*candidate, face);
                for (const auto vertex : vertices)
                {
                    if ((VertexPosition(*candidate, vertex) - position).GetLength() <= epsilon) { return vertex; }
                }
                EdgeHandle splitEdge;
                AZ::Vector3 splitPosition = position;
                for (const auto edge : FaceEdgeHandles(*candidate, face))
                {
                    const auto points = EdgeVertexPositions(*candidate, edge);
                    const auto d = points[1] - points[0];
                    const float lengthSq = d.GetLengthSq();
                    if (lengthSq == 0.0f) { continue; }
                    const float t = AZStd::clamp((position - points[0]).Dot(d) / lengthSq, 0.0f, 1.0f);
                    const auto projected = points[0] + d * t;
                    if ((projected - position).GetLength() <= epsilon)
                    {
                        splitEdge = edge;
                        splitPosition = projected;
                        break;
                    }
                }
                AZStd::vector<SourceTriangle> sources;
                if (splitEdge.IsValid())
                {
                    for (const auto adjacent : EdgeFaceHandles(*candidate, splitEdge)) { sources.push_back(snapshot(adjacent)); }
                }
                else { sources.push_back(snapshot(face)); }
                const auto vertex = mesh.add_vertex(splitPosition);
                if (splitEdge.IsValid()) { mesh.split_copy(om_eh(splitEdge), vertex); }
                else { mesh.split_copy(om_fh(face), vertex); }
                for (auto it = mesh.vf_ccwbegin(vertex); it.is_valid(); ++it)
                {
                    const auto resultFace = wb_fh(*it);
                    const auto positions = FaceVertexPositions(*candidate, resultFace);
                    const auto center = (positions[0] + positions[1] + positions[2]) / 3.0f;
                    bool assigned = false;
                    for (const auto& source : sources)
                    {
                        AZ::Vector3 weights;
                        if (!barycentric(source.m_points, center, weights)) { continue; }
                        const auto sourceNormal =
                            (source.m_points[1] - source.m_points[0]).Cross(source.m_points[2] - source.m_points[0]);
                        const auto resultNormal = (positions[1] - positions[0]).Cross(positions[2] - positions[0]);
                        if (sourceNormal.Dot(resultNormal) <= 0.0f) { return VertexHandle{}; }
                        labels[resultFace.Index()] = source.m_label;
                        origins[resultFace.Index()] = source.m_origin;
                        SetFaceMaterial(*candidate, resultFace, source.m_material);
                        SetFacePaintColor(*candidate, resultFace, source.m_paint);
                        SetFaceUvProjection(*candidate, resultFace, source.m_uvProjection);
                        SetFaceSmoothingGroups(*candidate, resultFace, source.m_smoothing);
                        for (const auto halfedge : FaceHalfedgeHandles(*candidate, resultFace))
                        {
                            const auto p = VertexPosition(*candidate, HalfedgeVertexHandleAtTip(*candidate, halfedge));
                            barycentric(source.m_points, p, weights);
                            mesh.set_texcoord2D(om_heh(halfedge),
                                source.m_uvs[0] * weights.GetX() + source.m_uvs[1] * weights.GetY() +
                                source.m_uvs[2] * weights.GetZ());
                        }
                        assigned = true;
                        break;
                    }
                    if (!assigned) { return VertexHandle{}; }
                }
                return wb_vh(vertex);
            };
            const auto insertAnchor = [&](const KnifePoint& point)
            {
                for (const auto face : MeshFaceHandles(*candidate))
                {
                    if (origins.find(face.Index())->second != point.m_face.Index()) { continue; }
                    const auto source = snapshot(face);
                    AZ::Vector3 weights;
                    if (barycentric(source.m_points, point.m_position, weights)) { return insert(face, point.m_position); }
                }
                return VertexHandle{};
            };
            const auto first = insertAnchor(start);
            if (!first.IsValid()) { return fail("The first point is no longer on its face."); }
            const auto last = insertAnchor(end);
            if (!last.IsValid() || first == last) { return fail("The next point must lie on a different part of the surface."); }
            AZStd::vector<AZStd::pair<VertexHandle, VertexHandle>> cutVertices;
            VertexHandle current = first;
            const size_t limit = faces.size() * 4 + 16;
            for (size_t step = 0; current != last && step < limit; ++step)
            {
                const auto currentPosition = VertexPosition(*candidate, current);
                const float progress = (currentPosition - start.m_position).Dot(axis);
                FaceHandle nextFace;
                AZ::Vector3 nextPosition = AZ::Vector3::CreateZero();
                // Accept one forward route through a vertex; reject folds and branches.
                for (auto it = mesh.vf_ccwbegin(om_vh(current)); it.is_valid(); ++it)
                {
                    const auto face = wb_fh(*it);
                    const auto verts = FaceVertexHandles(*candidate, face);
                    VertexHandles opposite;
                    for (const auto vertex : verts) { if (vertex != current) { opposite.push_back(vertex); } }
                    if (opposite.size() != 2) { continue; }
                    const auto a = VertexPosition(*candidate, opposite[0]);
                    const auto b = VertexPosition(*candidate, opposite[1]);
                    const auto facePoints = FaceVertexPositions(*candidate, face);
                    const auto normal = (facePoints[1] - facePoints[0]).Cross(facePoints[2] - facePoints[0]).GetNormalizedSafe();
                    // Follow the visible side of the surface. At a silhouette vertex the same
                    // plane may also meet rear faces, which must not become an alternative route.
                    if (normal.Dot(view) >= -1e-5f) { continue; }
                    const float da = (a - start.m_position).Dot(planeNormal);
                    const float db = (b - start.m_position).Dot(planeNormal);
                    if ((da > epsilon && db > epsilon) || (da < -epsilon && db < -epsilon) ||
                        AZStd::abs(da - db) <= epsilon) { continue; }
                    const float t = AZStd::clamp(da / (da - db), 0.0f, 1.0f);
                    const auto hit = a.Lerp(b, t);
                    const float along = (hit - start.m_position).Dot(axis);
                    if (along <= progress + epsilon || along > span + epsilon) { continue; }
                    if (nextFace.IsValid() && (nextPosition - hit).GetLength() > epsilon * 2.0f)
                    {
                        return fail("The cut has more than one route. Add a point at the corner or change the view.");
                    }
                    nextFace = face;
                    nextPosition = hit;
                }
                if (!nextFace.IsValid())
                {
                    return fail("The cut cannot reach that point on a connected surface. Add a point at the boundary.");
                }
                const auto next = insert(nextFace, nextPosition);
                if (!next.IsValid()) { return fail("Could not split this face safely."); }
                const auto halfedge = mesh.find_halfedge(om_vh(current), om_vh(next));
                if (!halfedge.is_valid()) { return fail("The cut did not form a valid edge."); }
                cutVertices.push_back({current, next});
                lines.push_back(currentPosition);
                lines.push_back(VertexPosition(*candidate, next));
                current = next;
            }
            if (current != last) { return fail("The cut could not reach the endpoint."); }
            AZStd::unordered_set<int> cutEdges;
            for (const auto& cut : cutVertices)
            {
                const auto h = mesh.find_halfedge(om_vh(cut.first), om_vh(cut.second));
                if (!h.is_valid()) { return fail("A cut edge was lost while splitting."); }
                cutEdges.insert(mesh.edge_handle(h).idx());
                RestoreVertex(*candidate, cut.first);
                RestoreVertex(*candidate, cut.second);
            }
            PolygonPropertyHandle property;
            mesh.get_property_handle(property, PolygonProps);
            auto& groups = mesh.property(property);
            const auto regroup = [&]()
            {
                groups.clear();
                AZStd::unordered_set<int> visited;
                for (const auto face : MeshFaceHandles(*candidate))
                {
                    if (!visited.insert(face.Index()).second) { continue; }
                    FaceHandlesInternal group{om_fh(face)};
                    for (size_t i = 0; i < group.size(); ++i)
                    {
                        for (auto h = mesh.cfh_iter(group[i]); h.is_valid(); ++h)
                        {
                            if (cutEdges.count(mesh.edge_handle(*h).idx())) { continue; }
                            const auto neighbor = mesh.face_handle(mesh.opposite_halfedge_handle(*h));
                            if (neighbor.is_valid() && labels[neighbor.idx()] == labels[face.Index()] &&
                                visited.insert(neighbor.idx()).second) { group.push_back(neighbor); }
                        }
                    }
                    for (const auto member : group) { groups[member] = group; }
                }
            };
            regroup();
            // A dangling edge cannot bound a WhiteBox polygon on its own. Keep its neighboring
            // triangles separate so the new cut remains selectable even at interior endpoints.
            int supportLabel = -1;
            bool needsSupports = false;
            for (const int edge : cutEdges)
            {
                const auto adjacent = EdgeFaceHandles(*candidate, EdgeHandle{edge});
                if (adjacent.size() != 2 ||
                    groups[om_fh(adjacent[0])].front() != groups[om_fh(adjacent[1])].front()) { continue; }
                for (const auto face : adjacent)
                {
                    labels[face.Index()] = supportLabel--;
                }
                needsSupports = true;
            }
            if (needsSupports) { regroup(); }
            CalculateNormals(*candidate);
            KnifePoint nextAnchor = end;
            nextAnchor.m_position = VertexPosition(*candidate, last);
            nextAnchor.m_face = wb_fh(*mesh.vf_ccwbegin(om_vh(last)));
            whiteBox.mesh = mesh;
            end = nextAnchor;
            return true;
        }

        static bool BuildEdgeLoops(
            WhiteBoxMesh& whiteBox, const EdgeHandle seed, const float fraction, const int count, const float spacing,
            AZStd::vector<AZ::Vector3>* preview, AZStd::string& error)
        {
            error.clear();
            const auto fail = [&error](const char* message) { error = message; return false; };
            const auto allEdges = MeshEdgeHandles(whiteBox);
            if (AZStd::find(allEdges.begin(), allEdges.end(), seed) == allEdges.end())
            {
                return fail("Select one polygon boundary edge in Transform mode.");
            }
            if (!(fraction > 0.0f && fraction + spacing * static_cast<float>(count - 1) < 1.0f))
            {
                return fail("The loop position must be between 0% and 100%, excluding the endpoints.");
            }
            struct LoopQuad
            {
                PolygonHandle m_polygon;
                AZStd::array<VertexHandle, 4> m_vertices;
                EdgeHandle m_entry;
                EdgeHandle m_exit;
                AZ::Data::AssetId m_material;
                AZ::u32 m_paint;
                UvProjection m_uvProjection;
                AZ::u32 m_smoothing = 0;
            };
            AZStd::map<int, LoopQuad> quads;
            AZStd::map<int, float> crossings;
            // Track orientation independently of cut position, including when a slid
            // first cut lies exactly at the midpoint.
            crossings[seed.Index()] = 0.25f;
            EdgeHandles pending{seed};
            while (!pending.empty())
            {
                const auto edge = pending.back();
                pending.pop_back();
                const auto adjacentFaces = EdgeFaceHandles(whiteBox, edge);
                if (adjacentFaces.empty()) { return fail("The selected strip contains an edge without a face."); }
                for (const auto face : adjacentFaces)
                {
                    const auto polygon = FacePolygonHandle(whiteBox, face);
                    if (polygon.m_faceHandles.size() != 2)
                    {
                        return fail("Loop insertion needs a continuous quad strip. This strip reaches a triangle or n-gon.");
                    }
                    const int key = polygon.m_faceHandles.front().Index();
                    if (const auto found = quads.find(key); found != quads.end())
                    {
                        if (edge != found->second.m_entry && edge != found->second.m_exit)
                        {
                            return fail("The quad strip crosses itself. Choose another edge.");
                        }
                        continue;
                    }
                    const auto borders = PolygonBorderHalfedgeHandles(whiteBox, polygon);
                    if (borders.size() != 1 || borders[0].size() != 4)
                    {
                        return fail("Loop insertion requires four-corner polygons without holes.");
                    }
                    const auto& border = borders[0];
                    size_t entry = 4;
                    for (size_t i = 0; i < 4; ++i)
                    {
                        if (HalfedgeEdgeHandle(whiteBox, border[i]) == edge) { entry = i; break; }
                    }
                    if (entry == 4) { return fail("Select a polygon boundary edge, not an internal triangle diagonal."); }
                    LoopQuad quad{};
                    quad.m_polygon = polygon;
                    quad.m_entry = edge;
                    quad.m_exit = HalfedgeEdgeHandle(whiteBox, border[(entry + 2) % 4]);
                    quad.m_material = FaceMaterial(whiteBox, polygon.m_faceHandles.front());
                    quad.m_paint = FacePaintColor(whiteBox, polygon.m_faceHandles.front());
                    quad.m_uvProjection = FaceUvProjection(whiteBox, polygon.m_faceHandles.front());
                    quad.m_smoothing = FaceSmoothingGroups(whiteBox, polygon.m_faceHandles.front());
                    for (const auto source : polygon.m_faceHandles)
                    {
                        if (FaceMaterial(whiteBox, source) != quad.m_material || FacePaintColor(whiteBox, source) != quad.m_paint)
                        {
                            return fail("A quad has different material or paint on its triangles. Make them match before inserting a loop.");
                        }
                    }
                    AZStd::array<AZ::Vector3, 4> points;
                    for (size_t i = 0; i < 4; ++i)
                    {
                        quad.m_vertices[i] = HalfedgeVertexHandleAtTail(whiteBox, border[(entry + i) % 4]);
                        points[i] = VertexPosition(whiteBox, quad.m_vertices[i]);
                    }
                    const auto normal = (points[1] - points[0]).Cross(points[2] - points[0]).GetNormalizedSafe();
                    float scale = 0.0f;
                    for (size_t i = 0; i < 4; ++i)
                    {
                        scale = AZStd::max(scale, (points[(i + 1) % 4] - points[i]).GetLength());
                        if ((points[(i + 1) % 4] - points[i]).Cross(points[(i + 2) % 4] - points[(i + 1) % 4]).Dot(normal) <= 1e-10f)
                        {
                            return fail("This strip contains a collapsed or concave quad.");
                        }
                    }
                    if (AZStd::abs((points[3] - points[0]).Dot(normal)) > scale * 1e-5f)
                    {
                        return fail("This strip contains a twisted quad. Flatten it before inserting a loop.");
                    }
                    const auto entryVertices = EdgeVertexHandles(whiteBox, edge);
                    const float localFraction = entryVertices[0] == quad.m_vertices[0]
                        ? crossings[edge.Index()] : 1.0f - crossings[edge.Index()];
                    const auto exitVertices = EdgeVertexHandles(whiteBox, quad.m_exit);
                    const float nextFraction = exitVertices[0] == quad.m_vertices[2] ? 1.0f - localFraction : localFraction;
                    const auto crossing = crossings.find(quad.m_exit.Index());
                    if (crossing != crossings.end())
                    {
                        if (AZStd::abs(crossing->second - nextFraction) > 1e-5f && !(count == 1 && AZStd::abs(fraction - 0.5f) < 1e-5f))
                        {
                            return fail("The loop returns with reversed orientation. Try the midpoint (50%).");
                        }
                    }
                    else
                    {
                        crossings[quad.m_exit.Index()] = nextFraction;
                        pending.push_back(quad.m_exit);
                    }
                    quads.emplace(key, AZStd::move(quad));
                }
            }
            if (quads.empty()) { return fail("No quad strip was found."); }
            const auto crossingPoint = [&](const EdgeHandle edge, const int cut)
            {
                const auto endpoints = EdgeVertexPositions(whiteBox, edge);
                const float first = crossings.find(edge.Index())->second;
                const float position = fraction + spacing * static_cast<float>(cut);
                const float t = first < 0.5f ? position : 1.0f - position;
                return endpoints[0].Lerp(endpoints[1], t);
            };
            if (preview)
            {
                for (const auto& item : quads)
                {
                    for (int cut = 0; cut < count; ++cut)
                    {
                        preview->push_back(crossingPoint(item.second.m_entry, cut));
                        preview->push_back(crossingPoint(item.second.m_exit, cut));
                    }
                }
                return true;
            }
            auto candidate = CloneMesh(whiteBox);
            AZStd::map<int, VertexHandles> inserted;
            for (const auto& crossing : crossings)
            {
                auto& vertices = inserted[crossing.first];
                for (int cut = 0; cut < count; ++cut)
                {
                    vertices.push_back(AddVertex(*candidate, crossingPoint(EdgeHandle{crossing.first}, cut)));
                }
            }
            FaceHandles removed;
            for (const auto& item : quads)
            {
                removed.insert(removed.end(), item.second.m_polygon.m_faceHandles.begin(), item.second.m_polygon.m_faceHandles.end());
            }
            RemoveFaces(*candidate, removed);
            FaceHandles added;
            for (const auto& item : quads)
            {
                const auto& quad = item.second;
                const auto& v = quad.m_vertices;
                auto entryCuts = inserted.find(quad.m_entry.Index())->second;
                auto exitCuts = inserted.find(quad.m_exit.Index())->second;
                const auto entryVertices = EdgeVertexHandles(whiteBox, quad.m_entry);
                const float localFraction = entryVertices[0] == v[0]
                    ? crossings.find(quad.m_entry.Index())->second : 1.0f - crossings.find(quad.m_entry.Index())->second;
                if (localFraction > 0.5f)
                {
                    AZStd::reverse(entryCuts.begin(), entryCuts.end());
                    AZStd::reverse(exitCuts.begin(), exitCuts.end());
                }
                VertexHandle a = v[0];
                VertexHandle b = v[3];
                for (int strip = 0; strip <= count; ++strip)
                {
                    const auto nextA = strip == count ? v[1] : entryCuts[strip];
                    const auto nextB = strip == count ? v[2] : exitCuts[strip];
                    const auto polygon = AddQuadWithoutFallback(*candidate, {{a, nextA, nextB, b}});
                    a = nextA;
                    b = nextB;
                    if (polygon.m_faceHandles.size() != 2)
                    {
                        return fail("The loop could not be connected cleanly. The mesh was left unchanged.");
                    }
                    for (const auto face : polygon.m_faceHandles)
                    {
                        SetFaceMaterial(*candidate, face, quad.m_material);
                        SetFacePaintColor(*candidate, face, quad.m_paint);
                        SetFaceUvProjection(*candidate, face, quad.m_uvProjection);
                        SetFaceSmoothingGroups(*candidate, face, quad.m_smoothing);
                        added.push_back(face);
                    }
                }
            }
            CalculateNormals(*candidate);
            CalculatePlanarUVs(*candidate, added);
            whiteBox.mesh = candidate->mesh;
            return true;
        }

        bool InsertEdgeLoop(WhiteBoxMesh& whiteBox, EdgeHandle edge, float fraction, AZStd::string& error)
        {
            return BuildEdgeLoops(whiteBox, edge, fraction, 1, 0.0f, nullptr, error);
        }

        bool InsertEdgeLoops(WhiteBoxMesh& whiteBox, EdgeHandle edge, int count, AZStd::string& error, float slide)
        {
            if (!(slide >= -1.0f && slide <= 1.0f)) { error = "Slide must be between -1 and 1."; return false; }
            if (count < 1 || count > 64) { error = "Choose between 1 and 64 cuts."; return false; }
            const float spacing = 1.0f / static_cast<float>(count + 1);
            return BuildEdgeLoops(whiteBox, edge, spacing * (1.0f + slide * 0.98f), count, spacing, nullptr, error);
        }

        bool PreviewEdgeLoops(
            WhiteBoxMesh& whiteBox, EdgeHandle edge, int count, AZStd::vector<AZ::Vector3>& lines, AZStd::string& error, float slide)
        {
            lines.clear();
            if (!(slide >= -1.0f && slide <= 1.0f)) { error = "Slide must be between -1 and 1."; return false; }
            if (count < 1 || count > 64) { error = "Choose between 1 and 64 cuts."; return false; }
            const float spacing = 1.0f / static_cast<float>(count + 1);
            return BuildEdgeLoops(whiteBox, edge, spacing * (1.0f + slide * 0.98f), count, spacing, &lines, error);
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
