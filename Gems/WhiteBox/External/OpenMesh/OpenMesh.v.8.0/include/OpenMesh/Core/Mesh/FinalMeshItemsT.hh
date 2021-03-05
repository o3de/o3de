/* ========================================================================= *
 *                                                                           *
 *                               OpenMesh                                    *
 *           Copyright (c) 2001-2015, RWTH-Aachen University                 *
 *           Department of Computer Graphics and Multimedia                  *
 *                          All rights reserved.                             *
 *                            www.openmesh.org                               *
 *                                                                           *
 *---------------------------------------------------------------------------*
 * This file is part of OpenMesh.                                            *
 *---------------------------------------------------------------------------*
 *                                                                           *
 * Redistribution and use in source and binary forms, with or without        *
 * modification, are permitted provided that the following conditions        *
 * are met:                                                                  *
 *                                                                           *
 * 1. Redistributions of source code must retain the above copyright notice, *
 *    this list of conditions and the following disclaimer.                  *
 *                                                                           *
 * 2. Redistributions in binary form must reproduce the above copyright      *
 *    notice, this list of conditions and the following disclaimer in the    *
 *    documentation and/or other materials provided with the distribution.   *
 *                                                                           *
 * 3. Neither the name of the copyright holder nor the names of its          *
 *    contributors may be used to endorse or promote products derived from   *
 *    this software without specific prior written permission.               *
 *                                                                           *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       *
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED *
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A           *
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER *
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,  *
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,       *
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR        *
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    *
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING      *
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS        *
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              *
 *                                                                           *
 * ========================================================================= */



#ifndef OPENMESH_MESH_ITEMS_HH
#define OPENMESH_MESH_ITEMS_HH


//== INCLUDES =================================================================


#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/Utils/GenProg.hh>
#include <OpenMesh/Core/Utils/vector_traits.hh>
#include <OpenMesh/Core/Mesh/Handles.hh>


//== NAMESPACES ===============================================================


namespace OpenMesh {


//== CLASS DEFINITION =========================================================

/// Definition of the mesh entities (items).
template <class Traits, bool IsTriMesh>
struct FinalMeshItemsT
{
  //--- build Refs structure ---
#ifndef DOXY_IGNORE_THIS
  struct Refs
  {
    typedef typename Traits::Point            Point;
    typedef typename vector_traits<Point>::value_type Scalar;

    typedef typename Traits::Normal           Normal;
    typedef typename Traits::Color            Color;
    typedef typename Traits::TexCoord1D       TexCoord1D;
    typedef typename Traits::TexCoord2D       TexCoord2D;
    typedef typename Traits::TexCoord3D       TexCoord3D;
    typedef typename Traits::TextureIndex     TextureIndex;
    typedef OpenMesh::VertexHandle            VertexHandle;
    typedef OpenMesh::FaceHandle              FaceHandle;
    typedef OpenMesh::EdgeHandle              EdgeHandle;
    typedef OpenMesh::HalfedgeHandle          HalfedgeHandle;
  };
#endif
  //--- export Refs types ---
  typedef typename Refs::Point           Point;
  typedef typename Refs::Scalar          Scalar;
  typedef typename Refs::Normal          Normal;
  typedef typename Refs::Color           Color;
  typedef typename Refs::TexCoord1D      TexCoord1D;
  typedef typename Refs::TexCoord2D      TexCoord2D;
  typedef typename Refs::TexCoord3D      TexCoord3D;
  typedef typename Refs::TextureIndex    TextureIndex;

  //--- get attribute bits from Traits ---
  enum Attribs
  {
    VAttribs = Traits::VertexAttributes,
    HAttribs = Traits::HalfedgeAttributes,
    EAttribs = Traits::EdgeAttributes,
    FAttribs = Traits::FaceAttributes
  };
  //--- merge internal items with traits items ---


/*
  typedef typename GenProg::IF<
    (bool)(HAttribs & Attributes::PrevHalfedge),
    typename InternalItems::Halfedge_with_prev,
    typename InternalItems::Halfedge_without_prev
  >::Result   InternalHalfedge;
*/
  //typedef typename InternalItems::Vertex                     InternalVertex;
  //typedef typename InternalItems::template Edge<Halfedge>      InternalEdge;
  //typedef typename InternalItems::template Face<IsTriMesh>     InternalFace;
  class ITraits
  {};

  typedef typename Traits::template VertexT<ITraits, Refs>      VertexData;
  typedef typename Traits::template HalfedgeT<ITraits, Refs>    HalfedgeData;
  typedef typename Traits::template EdgeT<ITraits, Refs>        EdgeData;
  typedef typename Traits::template FaceT<ITraits, Refs>        FaceData;
};


#ifndef DOXY_IGNORE_THIS
namespace {
namespace TM {
template<typename Lhs, typename Rhs> struct TypeEquality;
template<typename Lhs> struct TypeEquality<Lhs, Lhs> {};

template<typename LhsTraits, typename RhsTraits> struct ItemsEquality {
    TypeEquality<typename LhsTraits::Point, typename RhsTraits::Point> te1;
    TypeEquality<typename LhsTraits::Scalar, typename RhsTraits::Scalar> te2;
    TypeEquality<typename LhsTraits::Normal, typename RhsTraits::Normal> te3;
    TypeEquality<typename LhsTraits::Color, typename RhsTraits::Color> te4;
    TypeEquality<typename LhsTraits::TexCoord1D, typename RhsTraits::TexCoord1D> te5;
    TypeEquality<typename LhsTraits::TexCoord2D, typename RhsTraits::TexCoord2D> te6;
    TypeEquality<typename LhsTraits::TexCoord3D, typename RhsTraits::TexCoord3D> te7;
    TypeEquality<typename LhsTraits::TextureIndex, typename RhsTraits::TextureIndex> te8;
};

} /* namespace TM */
} /* anonymous namespace */
#endif

/**
 * @brief Cast a mesh with different but identical traits into each other.
 *
 * Note that there exists a syntactically more convenient global method
 * mesh_cast().
 *
 * Example:
 * @code{.cpp}
 * struct TriTraits1 : public OpenMesh::DefaultTraits {
 *   typedef Vec3d Point;
 * };
 * struct TriTraits2 : public OpenMesh::DefaultTraits {
 *   typedef Vec3d Point;
 * };
 * struct TriTraits3 : public OpenMesh::DefaultTraits {
 *   typedef Vec3f Point;
 * };
 *
 * TriMesh_ArrayKernelT<TriTraits1> a;
 * TriMesh_ArrayKernelT<TriTraits2> &b = MeshCast<TriMesh_ArrayKernelT<TriTraits2>&, TriMesh_ArrayKernelT<TriTraits1>&>::cast(a); // OK
 * TriMesh_ArrayKernelT<TriTraits3> &c = MeshCast<TriMesh_ArrayKernelT<TriTraits3>&, TriMesh_ArrayKernelT<TriTraits1>&>::cast(a); // ERROR
 * @endcode
 *
 * @see mesh_cast()
 *
 * @param rhs
 * @return
 */
template<typename LhsMeshT, typename RhsMeshT> struct MeshCast;

template<typename LhsMeshT, typename RhsMeshT>
struct MeshCast<LhsMeshT&, RhsMeshT&> {
    static LhsMeshT &cast(RhsMeshT &rhs) {
        (void)sizeof(TM::ItemsEquality<typename LhsMeshT::MeshItemsT, typename RhsMeshT::MeshItemsT>);
        (void)sizeof(TM::TypeEquality<typename LhsMeshT::ConnectivityT, typename RhsMeshT::ConnectivityT>);
        return reinterpret_cast<LhsMeshT&>(rhs);
    }
};

template<typename LhsMeshT, typename RhsMeshT>
struct MeshCast<const LhsMeshT&, const RhsMeshT&> {
    static const LhsMeshT &cast(const RhsMeshT &rhs) {
        (void)sizeof(TM::ItemsEquality<typename LhsMeshT::MeshItemsT, typename RhsMeshT::MeshItemsT>);
        (void)sizeof(TM::TypeEquality<typename LhsMeshT::ConnectivityT, typename RhsMeshT::ConnectivityT>);
        return reinterpret_cast<const LhsMeshT&>(rhs);
    }
};

template<typename LhsMeshT, typename RhsMeshT>
struct MeshCast<LhsMeshT*, RhsMeshT*> {
    static LhsMeshT *cast(RhsMeshT *rhs) {
        (void)sizeof(TM::ItemsEquality<typename LhsMeshT::MeshItemsT, typename RhsMeshT::MeshItemsT>);
        (void)sizeof(TM::TypeEquality<typename LhsMeshT::ConnectivityT, typename RhsMeshT::ConnectivityT>);
        return reinterpret_cast<LhsMeshT*>(rhs);
    }
};

template<typename LhsMeshT, typename RhsMeshT>
struct MeshCast<const LhsMeshT*, const RhsMeshT*> {
    static const LhsMeshT *cast(const RhsMeshT *rhs) {
        (void)sizeof(TM::ItemsEquality<typename LhsMeshT::MeshItemsT, typename RhsMeshT::MeshItemsT>);
        (void)sizeof(TM::TypeEquality<typename LhsMeshT::ConnectivityT, typename RhsMeshT::ConnectivityT>);
        return reinterpret_cast<const LhsMeshT*>(rhs);
    }
};


//=============================================================================
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_MESH_ITEMS_HH defined
//=============================================================================

