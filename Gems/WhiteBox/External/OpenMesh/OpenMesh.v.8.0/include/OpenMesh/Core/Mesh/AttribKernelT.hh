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



#ifndef OPENMESH_ATTRIBKERNEL_HH
#define OPENMESH_ATTRIBKERNEL_HH


//== INCLUDES =================================================================

#include <OpenMesh/Core/Mesh/Attributes.hh>
#include <OpenMesh/Core/Utils/GenProg.hh>
#include <OpenMesh/Core/Utils/vector_traits.hh>
#include <vector>
#include <algorithm>

//== NAMESPACES ===============================================================

namespace OpenMesh {


//== CLASS DEFINITION =========================================================

/** \class AttribKernelT AttribKernelT.hh <OpenMesh/Mesh/AttribKernelT.hh>

 The attribute kernel adds all standard properties to the kernel. Therefore
 the functions/types defined here provide a subset of the kernel
 interface as described in Concepts::KernelT.

 \see Concepts::KernelT
*/
template <class MeshItems, class Connectivity>
class AttribKernelT : public Connectivity
{
public:

  //---------------------------------------------------------------- item types

  typedef MeshItems MeshItemsT;
  typedef Connectivity ConnectivityT;
  typedef typename Connectivity::Vertex     Vertex;
  typedef typename Connectivity::Halfedge   Halfedge;
  typedef typename Connectivity::Edge       Edge;
  typedef typename Connectivity::Face       Face;

  typedef typename MeshItems::Point         Point;
  typedef typename MeshItems::Normal        Normal;
  typedef typename MeshItems::Color         Color;
  typedef typename MeshItems::TexCoord1D    TexCoord1D;
  typedef typename MeshItems::TexCoord2D    TexCoord2D;
  typedef typename MeshItems::TexCoord3D    TexCoord3D;
  typedef typename MeshItems::Scalar        Scalar;
  typedef typename MeshItems::TextureIndex  TextureIndex;

  typedef typename MeshItems::VertexData    VertexData;
  typedef typename MeshItems::HalfedgeData  HalfedgeData;
  typedef typename MeshItems::EdgeData      EdgeData;
  typedef typename MeshItems::FaceData      FaceData;

  typedef AttribKernelT<MeshItems,Connectivity>  AttribKernel;

  enum Attribs  {
    VAttribs = MeshItems::VAttribs,
    HAttribs = MeshItems::HAttribs,
    EAttribs = MeshItems::EAttribs,
    FAttribs = MeshItems::FAttribs
  };

  typedef VPropHandleT<VertexData>              DataVPropHandle;
  typedef HPropHandleT<HalfedgeData>            DataHPropHandle;
  typedef EPropHandleT<EdgeData>                DataEPropHandle;
  typedef FPropHandleT<FaceData>                DataFPropHandle;

public:

  //-------------------------------------------------- constructor / destructor

  AttribKernelT()
  : refcount_vnormals_(0),
    refcount_vcolors_(0),
    refcount_vtexcoords1D_(0),
    refcount_vtexcoords2D_(0),
    refcount_vtexcoords3D_(0),
    refcount_htexcoords1D_(0),
    refcount_htexcoords2D_(0),
    refcount_htexcoords3D_(0),
    refcount_henormals_(0),
    refcount_hecolors_(0),
    refcount_ecolors_(0),
    refcount_fnormals_(0),
    refcount_fcolors_(0),
    refcount_ftextureIndex_(0)
  {
    this->add_property( points_, "v:points" );

    if (VAttribs & Attributes::Normal)
      request_vertex_normals();

    if (VAttribs & Attributes::Color)
      request_vertex_colors();

    if (VAttribs & Attributes::TexCoord1D)
      request_vertex_texcoords1D();

    if (VAttribs & Attributes::TexCoord2D)
      request_vertex_texcoords2D();

    if (VAttribs & Attributes::TexCoord3D)
      request_vertex_texcoords3D();

    if (HAttribs & Attributes::TexCoord1D)
      request_halfedge_texcoords1D();

    if (HAttribs & Attributes::TexCoord2D)
      request_halfedge_texcoords2D();

    if (HAttribs & Attributes::TexCoord3D)
      request_halfedge_texcoords3D();

    if (HAttribs & Attributes::Color)
      request_halfedge_colors();

    if (VAttribs & Attributes::Status)
      Connectivity::request_vertex_status();

    if (HAttribs & Attributes::Status)
      Connectivity::request_halfedge_status();

    if (HAttribs & Attributes::Normal)
      request_halfedge_normals();

    if (EAttribs & Attributes::Status)
      Connectivity::request_edge_status();
    
    if (EAttribs & Attributes::Color)
      request_edge_colors();

    if (FAttribs & Attributes::Normal)
      request_face_normals();

    if (FAttribs & Attributes::Color)
      request_face_colors();

    if (FAttribs & Attributes::Status)
      Connectivity::request_face_status();

    if (FAttribs & Attributes::TextureIndex)
      request_face_texture_index();

    //FIXME: data properties might actually cost storage even
    //if there are no data traits??
    this->add_property(data_vpph_);
    this->add_property(data_fpph_);
    this->add_property(data_hpph_);
    this->add_property(data_epph_);
  }

  virtual ~AttribKernelT()
  {
    // should remove properties, but this will be done in
    // BaseKernel's destructor anyway...
  }

  /** Assignment from another mesh of \em another type.
      \note All that's copied is connectivity and vertex positions.
      All other information (like e.g. attributes or additional
      elements from traits classes) is not copied.
      \note If you want to copy all information, including *custom* properties,
      use PolyMeshT::operator=() instead.
  */
  template <class _AttribKernel>
  void assign(const _AttribKernel& _other, bool copyStandardProperties = false)
  {
    //copy standard properties if necessary
    if(copyStandardProperties)
      this->copy_all_kernel_properties(_other);

    this->assign_connectivity(_other);
    for (typename Connectivity::VertexIter v_it = Connectivity::vertices_begin();
         v_it != Connectivity::vertices_end(); ++v_it)
    {//assumes Point constructor supports cast from _AttribKernel::Point
      set_point(*v_it, (Point)_other.point(*v_it));
    }

    //initialize standard properties if necessary
    if(copyStandardProperties)
        initializeStandardProperties();
  }

  //-------------------------------------------------------------------- points

  const Point* points() const
  { return this->property(points_).data(); }

  const Point& point(VertexHandle _vh) const
  { return this->property(points_, _vh); }

  Point& point(VertexHandle _vh)
  { return this->property(points_, _vh); }

  void set_point(VertexHandle _vh, const Point& _p)
  { this->property(points_, _vh) = _p; }


  //------------------------------------------------------------ vertex normals

  const Normal* vertex_normals() const
  { return this->property(vertex_normals_).data(); }

  const Normal& normal(VertexHandle _vh) const
  { return this->property(vertex_normals_, _vh); }

  void set_normal(VertexHandle _vh, const Normal& _n)
  { this->property(vertex_normals_, _vh) = _n; }


  //------------------------------------------------------------- vertex colors

  const Color* vertex_colors() const
  { return this->property(vertex_colors_).data(); }

  const Color& color(VertexHandle _vh) const
  { return this->property(vertex_colors_, _vh); }

  void set_color(VertexHandle _vh, const Color& _c)
  { this->property(vertex_colors_, _vh) = _c; }


  //------------------------------------------------------- vertex 1D texcoords

  const TexCoord1D* texcoords1D() const {
    return this->property(vertex_texcoords1D_).data();
  }

  const TexCoord1D& texcoord1D(VertexHandle _vh) const {
    return this->property(vertex_texcoords1D_, _vh);
  }

  void set_texcoord1D(VertexHandle _vh, const TexCoord1D& _t) {
    this->property(vertex_texcoords1D_, _vh) = _t;
  }


  //------------------------------------------------------- vertex 2D texcoords

  const TexCoord2D* texcoords2D() const {
    return this->property(vertex_texcoords2D_).data();
  }

  const TexCoord2D& texcoord2D(VertexHandle _vh) const {
    return this->property(vertex_texcoords2D_, _vh);
  }

  void set_texcoord2D(VertexHandle _vh, const TexCoord2D& _t) {
    this->property(vertex_texcoords2D_, _vh) = _t;
  }


  //------------------------------------------------------- vertex 3D texcoords

  const TexCoord3D* texcoords3D() const {
    return this->property(vertex_texcoords3D_).data();
  }

  const TexCoord3D& texcoord3D(VertexHandle _vh) const {
    return this->property(vertex_texcoords3D_, _vh);
  }

  void set_texcoord3D(VertexHandle _vh, const TexCoord3D& _t) {
    this->property(vertex_texcoords3D_, _vh) = _t;
  }

  //.------------------------------------------------------ halfedge 1D texcoords

  const TexCoord1D* htexcoords1D() const {
    return this->property(halfedge_texcoords1D_).data();
  }

  const TexCoord1D& texcoord1D(HalfedgeHandle _heh) const {
    return this->property(halfedge_texcoords1D_, _heh);
  }

  void set_texcoord1D(HalfedgeHandle _heh, const TexCoord1D& _t) {
    this->property(halfedge_texcoords1D_, _heh) = _t;
  }


  //------------------------------------------------------- halfedge 2D texcoords

  const TexCoord2D* htexcoords2D() const {
    return this->property(halfedge_texcoords2D_).data();
  }

  const TexCoord2D& texcoord2D(HalfedgeHandle _heh) const {
    return this->property(halfedge_texcoords2D_, _heh);
  }

  void set_texcoord2D(HalfedgeHandle _heh, const TexCoord2D& _t) {
    this->property(halfedge_texcoords2D_, _heh) = _t;
  }


  //------------------------------------------------------- halfedge 3D texcoords

  const TexCoord3D* htexcoords3D() const {
    return this->property(halfedge_texcoords3D_).data();
  }

  const TexCoord3D& texcoord3D(HalfedgeHandle _heh) const {
    return this->property(halfedge_texcoords3D_, _heh);
  }

  void set_texcoord3D(HalfedgeHandle _heh, const TexCoord3D& _t) {
    this->property(halfedge_texcoords3D_, _heh) = _t;
  }
  
  //------------------------------------------------------------- edge colors
  
  const Color* edge_colors() const
  { return this->property(edge_colors_).data(); }
  
  const Color& color(EdgeHandle _eh) const
  { return this->property(edge_colors_, _eh); }
  
  void set_color(EdgeHandle _eh, const Color& _c)
  { this->property(edge_colors_, _eh) = _c; }


  //------------------------------------------------------------- halfedge normals

  const Normal& normal(HalfedgeHandle _heh) const
  { return this->property(halfedge_normals_, _heh); }

  void set_normal(HalfedgeHandle _heh, const Normal& _n)
  { this->property(halfedge_normals_, _heh) = _n; }


  //------------------------------------------------------------- halfedge colors
  
  const Color* halfedge_colors() const
  { return this->property(halfedge_colors_).data(); }
  
  const Color& color(HalfedgeHandle _heh) const
  { return this->property(halfedge_colors_, _heh); }
  
  void set_color(HalfedgeHandle _heh, const Color& _c)
  { this->property(halfedge_colors_, _heh) = _c; }

  //-------------------------------------------------------------- face normals

  const Normal& normal(FaceHandle _fh) const
  { return this->property(face_normals_, _fh); }

  void set_normal(FaceHandle _fh, const Normal& _n)
  { this->property(face_normals_, _fh) = _n; }

  //-------------------------------------------------------------- per Face Texture index

  const TextureIndex& texture_index(FaceHandle _fh) const
  { return this->property(face_texture_index_, _fh); }

  void set_texture_index(FaceHandle _fh, const TextureIndex& _t)
  { this->property(face_texture_index_, _fh) = _t; }

  //--------------------------------------------------------------- face colors

  const Color& color(FaceHandle _fh) const
  { return this->property(face_colors_, _fh); }

  void set_color(FaceHandle _fh, const Color& _c)
  { this->property(face_colors_, _fh) = _c; }

  //------------------------------------------------ request / alloc properties

  void request_vertex_normals()
  {
    if (!refcount_vnormals_++)
      this->add_property( vertex_normals_, "v:normals" );
  }

  void request_vertex_colors()
  {
    if (!refcount_vcolors_++)
      this->add_property( vertex_colors_, "v:colors" );
  }

  void request_vertex_texcoords1D()
  {
    if (!refcount_vtexcoords1D_++)
      this->add_property( vertex_texcoords1D_, "v:texcoords1D" );
  }

  void request_vertex_texcoords2D()
  {
    if (!refcount_vtexcoords2D_++)
      this->add_property( vertex_texcoords2D_, "v:texcoords2D" );
  }

  void request_vertex_texcoords3D()
  {
    if (!refcount_vtexcoords3D_++)
      this->add_property( vertex_texcoords3D_, "v:texcoords3D" );
  }

  void request_halfedge_texcoords1D()
  {
    if (!refcount_htexcoords1D_++)
      this->add_property( halfedge_texcoords1D_, "h:texcoords1D" );
  }

  void request_halfedge_texcoords2D()
  {
    if (!refcount_htexcoords2D_++)
      this->add_property( halfedge_texcoords2D_, "h:texcoords2D" );
  }

  void request_halfedge_texcoords3D()
  {
    if (!refcount_htexcoords3D_++)
      this->add_property( halfedge_texcoords3D_, "h:texcoords3D" );
  }
  
  void request_edge_colors()
  {
    if (!refcount_ecolors_++)
      this->add_property( edge_colors_, "e:colors" );
  }

  void request_halfedge_normals()
  {
    if (!refcount_henormals_++)
      this->add_property( halfedge_normals_, "h:normals" );
  }

  void request_halfedge_colors()
  {
    if (!refcount_hecolors_++)
      this->add_property( halfedge_colors_, "h:colors" );
  }

  void request_face_normals()
  {
    if (!refcount_fnormals_++)
      this->add_property( face_normals_, "f:normals" );
  }

  void request_face_colors()
  {
    if (!refcount_fcolors_++)
      this->add_property( face_colors_, "f:colors" );
  }

  void request_face_texture_index()
  {
    if (!refcount_ftextureIndex_++)
      this->add_property( face_texture_index_, "f:textureindex" );
  }

  //------------------------------------------------- release / free properties

  void release_vertex_normals()
  {
    if ((refcount_vnormals_ > 0) && (! --refcount_vnormals_))
      this->remove_property(vertex_normals_);
  }

  void release_vertex_colors()
  {
    if ((refcount_vcolors_ > 0) && (! --refcount_vcolors_))
      this->remove_property(vertex_colors_);
  }

  void release_vertex_texcoords1D() {
    if ((refcount_vtexcoords1D_ > 0) && (! --refcount_vtexcoords1D_))
      this->remove_property(vertex_texcoords1D_);
  }

  void release_vertex_texcoords2D() {
    if ((refcount_vtexcoords2D_ > 0) && (! --refcount_vtexcoords2D_))
      this->remove_property(vertex_texcoords2D_);
  }

  void release_vertex_texcoords3D() {
    if ((refcount_vtexcoords3D_ > 0) && (! --refcount_vtexcoords3D_))
      this->remove_property(vertex_texcoords3D_);
  }

  void release_halfedge_texcoords1D() {
    if ((refcount_htexcoords1D_ > 0) && (! --refcount_htexcoords1D_))
      this->remove_property(halfedge_texcoords1D_);
  }

  void release_halfedge_texcoords2D() {
    if ((refcount_htexcoords2D_ > 0) && (! --refcount_htexcoords2D_))
      this->remove_property(halfedge_texcoords2D_);
  }

  void release_halfedge_texcoords3D() {
    if ((refcount_htexcoords3D_ > 0) && (! --refcount_htexcoords3D_))
      this->remove_property(halfedge_texcoords3D_);
  }
  
  void release_edge_colors()
  {
      if ((refcount_ecolors_ > 0) && (! --refcount_ecolors_))
          this->remove_property(edge_colors_);
  }

  void release_halfedge_normals()
  {
      if ((refcount_henormals_ > 0) && (! --refcount_henormals_))
          this->remove_property(halfedge_normals_);
  }

  void release_halfedge_colors()
  {
      if ((refcount_hecolors_ > 0) && (! --refcount_hecolors_))
          this->remove_property(halfedge_colors_);
  }

  void release_face_normals()
  {
    if ((refcount_fnormals_ > 0) && (! --refcount_fnormals_))
      this->remove_property(face_normals_);
  }

  void release_face_colors()
  {
    if ((refcount_fcolors_ > 0) && (! --refcount_fcolors_))
      this->remove_property(face_colors_);
  }

  void release_face_texture_index()
  {
    if ((refcount_ftextureIndex_ > 0) && (! --refcount_ftextureIndex_))
      this->remove_property(face_texture_index_);
  }

  //---------------------------------------------- dynamic check for properties

  bool has_vertex_normals()       const { return vertex_normals_.is_valid();      }
  bool has_vertex_colors()        const { return vertex_colors_.is_valid();       }
  bool has_vertex_texcoords1D()   const { return vertex_texcoords1D_.is_valid();  }
  bool has_vertex_texcoords2D()   const { return vertex_texcoords2D_.is_valid();  }
  bool has_vertex_texcoords3D()   const { return vertex_texcoords3D_.is_valid();  }
  bool has_halfedge_texcoords1D() const { return halfedge_texcoords1D_.is_valid();}
  bool has_halfedge_texcoords2D() const { return halfedge_texcoords2D_.is_valid();}
  bool has_halfedge_texcoords3D() const { return halfedge_texcoords3D_.is_valid();}
  bool has_edge_colors()          const { return edge_colors_.is_valid();         }
  bool has_halfedge_normals()     const { return halfedge_normals_.is_valid();    }
  bool has_halfedge_colors()      const { return halfedge_colors_.is_valid();     }
  bool has_face_normals()         const { return face_normals_.is_valid();        }
  bool has_face_colors()          const { return face_colors_.is_valid();         }
  bool has_face_texture_index()   const { return face_texture_index_.is_valid();  }

public:

  typedef VPropHandleT<Point>               PointsPropertyHandle;
  typedef VPropHandleT<Normal>              VertexNormalsPropertyHandle;
  typedef VPropHandleT<Color>               VertexColorsPropertyHandle;
  typedef VPropHandleT<TexCoord1D>          VertexTexCoords1DPropertyHandle;
  typedef VPropHandleT<TexCoord2D>          VertexTexCoords2DPropertyHandle;
  typedef VPropHandleT<TexCoord3D>          VertexTexCoords3DPropertyHandle;
  typedef HPropHandleT<TexCoord1D>          HalfedgeTexCoords1DPropertyHandle;
  typedef HPropHandleT<TexCoord2D>          HalfedgeTexCoords2DPropertyHandle;
  typedef HPropHandleT<TexCoord3D>          HalfedgeTexCoords3DPropertyHandle;
  typedef EPropHandleT<Color>               EdgeColorsPropertyHandle;
  typedef HPropHandleT<Normal>              HalfedgeNormalsPropertyHandle;
  typedef HPropHandleT<Color>               HalfedgeColorsPropertyHandle;
  typedef FPropHandleT<Normal>              FaceNormalsPropertyHandle;
  typedef FPropHandleT<Color>               FaceColorsPropertyHandle;
  typedef FPropHandleT<TextureIndex>        FaceTextureIndexPropertyHandle;

public:
  //standard vertex properties
  PointsPropertyHandle                      points_pph() const
  { return points_; }

  VertexNormalsPropertyHandle               vertex_normals_pph() const
  { return vertex_normals_; }

  VertexColorsPropertyHandle                vertex_colors_pph() const
  { return vertex_colors_; }

  VertexTexCoords1DPropertyHandle           vertex_texcoords1D_pph() const
  { return vertex_texcoords1D_; }

  VertexTexCoords2DPropertyHandle           vertex_texcoords2D_pph() const
  { return vertex_texcoords2D_; }

  VertexTexCoords3DPropertyHandle           vertex_texcoords3D_pph() const
  { return vertex_texcoords3D_; }

  //standard halfedge properties
  HalfedgeTexCoords1DPropertyHandle           halfedge_texcoords1D_pph() const
  { return halfedge_texcoords1D_; }

  HalfedgeTexCoords2DPropertyHandle           halfedge_texcoords2D_pph() const
  { return halfedge_texcoords2D_; }

  HalfedgeTexCoords3DPropertyHandle           halfedge_texcoords3D_pph() const
  { return halfedge_texcoords3D_; }

  // standard edge properties
  HalfedgeNormalsPropertyHandle              halfedge_normals_pph() const
  { return halfedge_normals_; }


  // standard edge properties
  HalfedgeColorsPropertyHandle              halfedge_colors_pph() const
  { return halfedge_colors_; }
  
  // standard edge properties
  EdgeColorsPropertyHandle                  edge_colors_pph() const
  { return edge_colors_; }

  //standard face properties
  FaceNormalsPropertyHandle                 face_normals_pph() const
  { return face_normals_; }

  FaceColorsPropertyHandle                  face_colors_pph() const
  { return face_colors_; }

  FaceTextureIndexPropertyHandle            face_texture_index_pph() const
  { return face_texture_index_; }

  VertexData&                               data(VertexHandle _vh)
  { return this->property(data_vpph_, _vh); }

  const VertexData&                         data(VertexHandle _vh) const
  { return this->property(data_vpph_, _vh); }

  FaceData&                                 data(FaceHandle _fh)
  { return this->property(data_fpph_, _fh); }

  const FaceData&                           data(FaceHandle _fh) const
  { return this->property(data_fpph_, _fh); }

  EdgeData&                                 data(EdgeHandle _eh)
  { return this->property(data_epph_, _eh); }

  const EdgeData&                           data(EdgeHandle _eh) const
  { return this->property(data_epph_, _eh); }

  HalfedgeData&                             data(HalfedgeHandle _heh)
  { return this->property(data_hpph_, _heh); }

  const HalfedgeData&                       data(HalfedgeHandle _heh) const
  { return this->property(data_hpph_, _heh); }

private:
  //standard vertex properties
  PointsPropertyHandle                      points_;
  VertexNormalsPropertyHandle               vertex_normals_;
  VertexColorsPropertyHandle                vertex_colors_;
  VertexTexCoords1DPropertyHandle           vertex_texcoords1D_;
  VertexTexCoords2DPropertyHandle           vertex_texcoords2D_;
  VertexTexCoords3DPropertyHandle           vertex_texcoords3D_;
  //standard halfedge properties
  HalfedgeTexCoords1DPropertyHandle         halfedge_texcoords1D_;
  HalfedgeTexCoords2DPropertyHandle         halfedge_texcoords2D_;
  HalfedgeTexCoords3DPropertyHandle         halfedge_texcoords3D_;
  HalfedgeNormalsPropertyHandle             halfedge_normals_;
  HalfedgeColorsPropertyHandle              halfedge_colors_;
  // standard edge properties
  EdgeColorsPropertyHandle                  edge_colors_;
  //standard face properties
  FaceNormalsPropertyHandle                 face_normals_;
  FaceColorsPropertyHandle                  face_colors_;
  FaceTextureIndexPropertyHandle            face_texture_index_;
  //data properties handles
  DataVPropHandle                           data_vpph_;
  DataHPropHandle                           data_hpph_;
  DataEPropHandle                           data_epph_;
  DataFPropHandle                           data_fpph_;

  unsigned int                              refcount_vnormals_;
  unsigned int                              refcount_vcolors_;
  unsigned int                              refcount_vtexcoords1D_;
  unsigned int                              refcount_vtexcoords2D_;
  unsigned int                              refcount_vtexcoords3D_;
  unsigned int                              refcount_htexcoords1D_;
  unsigned int                              refcount_htexcoords2D_;
  unsigned int                              refcount_htexcoords3D_;
  unsigned int                              refcount_henormals_;
  unsigned int                              refcount_hecolors_;
  unsigned int                              refcount_ecolors_;
  unsigned int                              refcount_fnormals_;
  unsigned int                              refcount_fcolors_;
  unsigned int                              refcount_ftextureIndex_;

  /**
   * @brief initializeStandardProperties Initializes the standard properties
   * and sets refcount to 1 if found. (e.g. when the copy constructor was used)
   */
  void initializeStandardProperties()
  {
      if(!this->get_property_handle(points_,
               "v:points"))
      {
          //mesh has no points?
      }
      refcount_vnormals_ = this->get_property_handle(vertex_normals_,
          "v:normals") ? 1 : 0 ;
      refcount_vcolors_ = this->get_property_handle(vertex_colors_,
          "v:colors") ? 1 : 0 ;
      refcount_vtexcoords1D_ = this->get_property_handle(vertex_texcoords1D_,
          "v:texcoords1D") ? 1 : 0 ;
      refcount_vtexcoords2D_ = this->get_property_handle(vertex_texcoords2D_,
          "v:texcoords2D") ? 1 : 0 ;
      refcount_vtexcoords3D_ = this->get_property_handle(vertex_texcoords3D_,
          "v:texcoords3D") ? 1 : 0 ;
      refcount_htexcoords1D_ = this->get_property_handle(halfedge_texcoords1D_,
          "h:texcoords1D") ? 1 : 0 ;
      refcount_htexcoords2D_ = this->get_property_handle(halfedge_texcoords2D_,
          "h:texcoords2D") ? 1 : 0 ;
      refcount_htexcoords3D_ = this->get_property_handle(halfedge_texcoords3D_,
          "h:texcoords3D") ? 1 : 0 ;
      refcount_henormals_ = this->get_property_handle(halfedge_normals_,
          "h:normals") ? 1 : 0 ;
      refcount_hecolors_ = this->get_property_handle(halfedge_colors_,
          "h:colors") ? 1 : 0 ;
      refcount_ecolors_ = this->get_property_handle(edge_colors_,
          "e:colors") ? 1 : 0 ;
      refcount_fnormals_ = this->get_property_handle(face_normals_,
          "f:normals") ? 1 : 0 ;
      refcount_fcolors_ = this->get_property_handle(face_colors_,
          "f:colors") ? 1 : 0 ;
      refcount_ftextureIndex_ = this->get_property_handle(face_texture_index_,
          "f:textureindex") ? 1 : 0 ;
  }
};

//=============================================================================
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_ATTRIBKERNEL_HH defined
//=============================================================================
