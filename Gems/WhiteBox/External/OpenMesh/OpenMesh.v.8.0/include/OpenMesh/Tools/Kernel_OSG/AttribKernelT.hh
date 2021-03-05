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



#ifndef OPENMESH_KERNEL_OSG_ATTRIBKERNEL_HH
#define OPENMESH_KENREL_OSG_ATTRIBKERNEL_HH


//== INCLUDES =================================================================

#include <OpenMesh/Core/Utils/GenProg.hh>
#include <OpenMesh/Core/Mesh/Attributes.hh>
// --------------------
#include <OpenMesh/Tools/Kernel_OSG/PropertyT.hh>
#include <OpenMesh/Tools/Kernel_OSG/PropertyKernel.hh>
// --------------------
#include <osg/Geometry>


//== NAMESPACES ===============================================================

namespace OpenMesh {
namespace Kernel_OSG {

//== CLASS DEFINITION =========================================================



/// This class adds the standard properties to the mesh type.
template <class MeshItems>
class AttribKernelT 
  : public PropertyKernel< typename MeshItems::Face::IsTriangle >
{
protected:

  typedef typename MeshItems::Face::IsTriangle IsTriMesh;
  typedef PropertyKernel< IsTriMesh > Base;

  typedef typename Base::FPTypesHandle         FPTypesHandle;
  typedef typename Base::FPLengthsHandle       FPLengthsHandle;
  typedef typename Base::FIndicesHandle        FIndicesHandle;

public:

  //---------------------------------------------------------------- item types

  typedef typename MeshItems::Vertex             Vertex;
  typedef typename MeshItems::Halfedge           Halfedge;
  typedef typename MeshItems::Edge               Edge;
  typedef typename MeshItems::Face               Face;

  typedef typename MeshItems::Point              Point;
  typedef typename MeshItems::Normal             Normal;
  typedef typename MeshItems::Color              Color;
  typedef typename MeshItems::TexCoord           TexCoord;

  typedef typename MeshItems::Scalar             Scalar;

  typedef Attributes::StatusInfo                 StatusInfo;


  enum Attribs {
    VAttribs = MeshItems::VAttribs,
    HAttribs = MeshItems::HAttribs,
    EAttribs = MeshItems::EAttribs,
    FAttribs = MeshItems::FAttribs,
  };

  typedef GenProg::Bool2Type<(bool)(HAttribs & Attributes::PrevHalfedge)> 
    HasPrevHalfedge;

  //

  typedef typename _t2vp< Point >::prop                 GeoPositions;
  typedef typename _t2vn< Normal >::prop                GeoNormals;
  typedef typename _t2vc< Color  >::prop                GeoColors;
  typedef typename _t2vtc< TexCoord >::prop             GeoTexCoords;

//   typedef typename Base::GeoPTypes                      GeoPTypes;
//   typedef typename Base::GeoPLengths                    GeoPLengths;
//   typedef typename Base::GeoIndices                     GeoIndices;
  
  //-------------------------------------------------- constructor / destructor

  AttribKernelT() :

    refcount_vnormals_(0),
    refcount_vcolors_(0),
    refcount_vtexcoords_(0),
    refcount_vstatus_(0),
    refcount_estatus_(0),
    refcount_ecolors_(0),
    refcount_hstatus_(0),
    refcount_fnormals_(0),
    refcount_fcolors_(0),
    refcount_fstatus_(0)

  {
    points_       = add_vpositions( Point(), "v:points" );

    face_types_   = add_fptypes();
    face_lengths_ = add_fplengths();
    face_indices_ = add_findices( face_types_, face_lengths_);

    if (VAttribs & Attributes::Normal)
      request_vertex_normals();

    if (VAttribs & Attributes::Color)
      request_vertex_colors();

    if (VAttribs & Attributes::TexCoord)
      request_vertex_texcoords();

    if (VAttribs & Attributes::Status)
      request_vertex_status();

    if (EAttribs & Attributes::Status)
      request_edge_status();
    
    if (EAttribs & Attributes::Color)
      request_edge_colors();

    if (FAttribs & Attributes::Normal)
      request_face_normals();

    if (FAttribs & Attributes::Color)
      request_face_colors();

    if (FAttribs & Attributes::Status)
      request_face_status();
  }

  ~AttribKernelT() 
  {
    // should remove properties, but this will be done in
    // BaseKernel's destructor anyway...
  }


  // ------------------------------------------------------- copy & assignement

  AttribKernelT( const AttribKernelT& _rhs )
    : Base( _rhs )
  {
    operator=(_rhs);
  }

  AttribKernelT& operator = ( const AttribKernelT& _rhs )
  {
    // remove old properties
    remove_property(points_);
    remove_property(vertex_normals_);
    remove_property(vertex_colors_);
    remove_property(vertex_texcoords_);
    remove_property(vertex_status_);
    remove_property(halfedge_status_);
    remove_property(edge_status_);
    remove_property(edge_colors_);
    remove_property(face_normals_);
    remove_property(face_colors_);
    remove_property(face_status_);

    // parent deep-copies properties
    BaseKernel::operator=(_rhs);

    // copy property handles
    points_            = _rhs.points_;
    vertex_normals_    = _rhs.vertex_normals_;
    vertex_colors_     = _rhs.vertex_colors_;
    vertex_texcoords_  = _rhs.vertex_texcoords_;
    vertex_status_     = _rhs.vertex_status_;
    halfedge_status_   = _rhs.halfedge_status_;
    edge_status_       = _rhs.edge_status_;
    edge_colors_       = _rhs.edge_colors_;
    face_normals_      = _rhs.face_normals_;
    face_colors_       = _rhs.face_colors_;
    face_status_       = _rhs.face_status_;

    // copy ref-counts
    refcount_vnormals_   = _rhs.refcount_vnormals_;
    refcount_vcolors_    = _rhs.refcount_vcolors_;
    refcount_vtexcoords_ = _rhs.refcount_vtexcoords_;
    refcount_vstatus_    = _rhs.refcount_vstatus_;
    refcount_hstatus_    = _rhs.refcount_hstatus_;
    refcount_estatus_    = _rhs.refcount_estatus_;
    refcount_ecolors_    = _rhs.refcount_ecolors_;
    refcount_fnormals_   = _rhs.refcount_fnormals_;
    refcount_fcolors_    = _rhs.refcount_fcolors_;
    refcount_fstatus_    = _rhs.refcount_fstatus_;

    return *this;
  }

  //------------------------------------------------------------ osg properties

  //------------------------------ vertex property

  typename GeoPositions::property_ptr_t osg_vpositions()
  { return vpositions(points_).osg_ptr(); }

  typename GeoNormals::property_ptr_t osg_vnormals() 
  { return vnormals(vertex_normals_).osg_ptr(); }

  typename GeoColors::property_ptr_t osg_vcolors() 
  { return vcolors(vertex_colors_).osg_ptr(); }

  typename GeoTexCoords::property_ptr_t osg_vtexcoords() 
  { return vtexcoords(vertex_texcoords_).osg_ptr(); }
  
  //------------------------------ edge property
  
  typename GeoColors::property_ptr_t osg_ecolors() 
  { return ecolors(edge_colors_).osg_ptr(); }

  //------------------------------ face property

  GeoPTypes::property_ptr_t osg_ptypes() 
  { return fptypes( face_types_ ).osg_ptr(); }

  GeoPLengths::property_ptr_t osg_plengths() 
  { return fplengths( face_lengths_ ).osg_ptr(); }

  typename GeoIndices::property_ptr_t osg_indices() 
  { return findices( face_indices_ ).osg_ptr(); }


  //---------------------------------------- set osg geo property
  
  //------------------------------ face property

  void set_face_types( FaceHandle _fh, GeoPTypes::value_type _t)
  { fptypes( face_types_, _fh ) = _t; }

  void set_face_lengths( FaceHandle _fh, GeoPLengths::value_type _l) 
  { fplengths( face_lengths_, _fh ) = _l; }

  void set_face_indices( FaceHandle _fh, 
			 typename GeoIndices::value_type _i) 
  { findices( face_indices_, _fh ) = _i; }

  //--------------------------------------------------------- set/get properties

  //---------------------------------------- points

  const Point* points() const 
  { return vpositions( points_ ).data(); }

  const Point& point(VertexHandle _vh) const 
  { return vpositions( points_, _vh); }

  void set_point(VertexHandle _vh, const Point& _p) 
  { vpositions( points_, _vh ) = _p; }


  //---------------------------------------- vertex normals

  const Normal* vertex_normals() const {
    return vnormals(vertex_normals_).data(); 
  }

  const Normal& normal(VertexHandle _vh) const {
    return vnormals(vertex_normals_, _vh);
  }

  void set_normal(VertexHandle _vh, const Normal& _n) {
    vnormals(vertex_normals_, _vh) = _n;
  }


  //---------------------------------------- vertex colors

  const Color* vertex_colors() const {
    return vcolors(vertex_colors_).data(); 
  }

  const Color& color(VertexHandle _vh) const {
    return vcolors(vertex_colors_, _vh);
  }

  void set_color(VertexHandle _vh, const Color& _c) {
    vcolors(vertex_colors_, _vh) = _c;
  }


  //---------------------------------------- vertex texcoords

  const TexCoord* texcoords() const {
    return vtexcoords(vertex_texcoords_).data(); 
  }

  const TexCoord& texcoord(VertexHandle _vh) const {
    return vtexcoords(vertex_texcoords_, _vh);
  }

  void set_texcoord(VertexHandle _vh, const TexCoord& _t) {
    vtexcoords(vertex_texcoords_, _vh) = _t;
  }


  //---------------------------------------- vertex status

  const StatusInfo& status(VertexHandle _vh) const {
    return property(vertex_status_, _vh);
  }

  StatusInfo& status(VertexHandle _vh) {
    return property(vertex_status_, _vh);
  }


  //---------------------------------------- halfedge status

  const StatusInfo& status(HalfedgeHandle _eh) const {
    return property(halfedge_status_, _eh);
  }

  StatusInfo& status(HalfedgeHandle _eh) {
    return property(halfedge_status_, _eh);
  }


  //---------------------------------------- edge status

  const StatusInfo& status(EdgeHandle _eh) const {
    return property(edge_status_, _eh);
  }

  StatusInfo& status(EdgeHandle _eh) {
    return property(edge_status_, _eh);
  }
  
  //---------------------------------------- edge colors
  
  const Color* edge_colors() const {
      return ecolors(edge_colors_).data(); 
  }
  
  const Color& color(EdgeHandle _eh) const {
      return ecolors(edge_colors_, _eh);
  }
  
  void set_color(EdgeHandle _eh, const Color& _c) {
      ecolors(edge_colors_, _eh) = _c;
  }


  //---------------------------------------- face status

  const StatusInfo& status(FaceHandle _fh) const {
    return property(face_status_, _fh);
  }

  StatusInfo& status(FaceHandle _fh) {
    return property(face_status_, _fh);
  }


  //---------------------------------------- face normals

  const Normal& normal(FaceHandle _fh) const {
    return property(face_normals_, _fh);
  }

  void set_normal(FaceHandle _fh, const Normal& _n) {
    property(face_normals_, _fh) = _n;
  }


  //---------------------------------------- face colors

  const Color& color(FaceHandle _fh) const {
    return property(face_colors_, _fh);
  }

  void set_color(FaceHandle _fh, const Color& _c) {
    property(face_colors_, _fh) = _c;
  }



  //------------------------------------------------ request / alloc properties

  void request_vertex_normals() {
    if (!refcount_vnormals_++)  
      vertex_normals_ = add_vnormals( Normal(), "v:normals" );
  }

  void request_vertex_colors() {
    if (!refcount_vcolors_++)
      vertex_colors_ = add_vcolors( Color(), "v:colors" );
  }

  void request_vertex_texcoords() {
    if (!refcount_vtexcoords_++)
      vertex_texcoords_ = add_vtexcoords( TexCoord(), "v:texcoords" );
  }

  void request_vertex_status() {
    if (!refcount_vstatus_++)
      add_property( vertex_status_, "v:status" );
  }

  void request_halfedge_status() {
    if (!refcount_hstatus_++)
      add_property( halfedge_status_, "h:status" );
  }

  void request_edge_status() {
    if (!refcount_estatus_++)
      add_property( edge_status_, "e:status" );
  }
  
  void request_edge_colors() {
    if (!refcount_ecolors_++)
      edge_colors_ = add_ecolors( Color(), "e:colors" );
  }

  void request_face_normals() {
    if (!refcount_fnormals_++)
      add_property( face_normals_, "f:normals" );
  }

  void request_face_colors() {
    if (!refcount_fcolors_++)
      add_property( face_colors_, "f:colors" );
  }

  void request_face_status() {
    if (!refcount_fstatus_++)
      add_property( face_status_, "f:status" );
  }



  //------------------------------------------------- release / free properties

  void release_vertex_normals() {
    if ((refcount_vnormals_ > 0) && (! --refcount_vnormals_))
      remove_property(vertex_normals_);
  }

  void release_vertex_colors() {
    if ((refcount_vcolors_ > 0) && (! --refcount_vcolors_))
      remove_property(vertex_colors_);
  }

  void release_vertex_texcoords() {
    if ((refcount_vtexcoords_ > 0) && (! --refcount_vtexcoords_))
      remove_property(vertex_texcoords_);
  }

  void release_vertex_status() {
    if ((refcount_vstatus_ > 0) && (! --refcount_vstatus_))
      remove_property(vertex_status_);
  }

  void release_halfedge_status() {
    if ((refcount_hstatus_ > 0) && (! --refcount_hstatus_))
      remove_property(halfedge_status_);
  }

  void release_edge_status() {
    if ((refcount_estatus_ > 0) && (! --refcount_estatus_))
      remove_property(edge_status_);
  }
  
  void release_edge_colors() {
    if ((refcount_ecolors_ > 0) && (! --refcount_ecolors_))
      remove_property(edge_colors_);
  }

  void release_face_normals() {
    if ((refcount_fnormals_ > 0) && (! --refcount_fnormals_))
      remove_property(face_normals_);
  }

  void release_face_colors() {
    if ((refcount_fcolors_ > 0) && (! --refcount_fcolors_))
      remove_property(face_colors_);
  }

  void release_face_status() {
    if ((refcount_fstatus_ > 0) && (! --refcount_fstatus_))
      remove_property(face_status_);
  }


  //----------------------------------------------- static check for properties

  typedef 
  GenProg::Bool2Type<(bool)(VAttribs & Attributes::Normal)> 
  HasVertexNormals;

  typedef 
  GenProg::Bool2Type<(bool)(VAttribs & Attributes::Color)> 
  HasVertexColors;

  typedef
  GenProg::Bool2Type<(bool)(VAttribs & Attributes::TexCoord)> 
  HasVertexTexCoords;

  typedef
  GenProg::Bool2Type<(bool)(VAttribs & Attributes::Status)> 
  HasVertexStatus;


  typedef
  GenProg::Bool2Type<(bool)(HAttribs & Attributes::PrevHalfedge)> 
  HasPrevHalfedge;

  typedef
  GenProg::Bool2Type<(bool)(HAttribs & Attributes::Status)> 
  HasHalfedgeStatus;


  typedef
  GenProg::Bool2Type<(bool)(EAttribs & Attributes::Status)> 
  HasEdgeStatus;

  typedef 
  GenProg::Bool2Type<(bool)(EAttribs & Attributes::Color)> 
  HasEdgeColors;
  

  typedef 
  GenProg::Bool2Type<(bool)(FAttribs & Attributes::Normal)> 
  HasFaceNormals;

  typedef 
  GenProg::Bool2Type<(bool)(FAttribs & Attributes::Color)> 
  HasFaceColors;

  typedef
  GenProg::Bool2Type<(bool)(FAttribs & Attributes::Status)> 
  HasFaceStatus;


  //---------------------------------------------- dynamic check for properties

  bool has_vertex_normals()   const { return vertex_normals_.is_valid();   }
  bool has_vertex_colors()    const { return vertex_colors_.is_valid();    }
  bool has_vertex_texcoords() const { return vertex_texcoords_.is_valid(); }
  bool has_vertex_status()    const { return vertex_status_.is_valid();    }
  bool has_edge_status()      const { return edge_status_.is_valid();      }
  bool has_halfedge_status()  const { return halfedge_status_.is_valid();  }
  bool has_edge_colors()      const { return edge_colors_.is_valid();      }
  bool has_face_normals()     const { return face_normals_.is_valid();     }
  bool has_face_colors()      const { return face_colors_.is_valid();      }
  bool has_face_status()      const { return face_status_.is_valid();      }

  static bool has_prev_halfedge() {
    return (HAttribs & Attributes::PrevHalfedge); 
  }


public:

  osg::GeometryPtr createGeometryPtr()
  {
    using namespace osg;
    GeometryPtr geo=Geometry::create();
    return bind(geo) ? geo : NullFC;    
  }

  // create new geometry core from mesh
  bool bind( osg::GeometryPtr& _geo )
  {
    using namespace osg;

    int Mask = 
      Geometry::TypesFieldMask     |
      Geometry::LengthsFieldMask   |
      Geometry::IndicesFieldMask   |
      Geometry::PositionsFieldMask;

    if ( has_vertex_colors() )
      Mask |= Geometry::ColorsFieldMask;
    if ( has_vertex_normals() )
      Mask |= Geometry::NormalsFieldMask;
    if ( has_vertex_texcoords() )
      Mask |= Geometry::TexCoordsFieldMask;

//     std::clog << "#ptypes   : " << osg_ptypes()->getSize()     << std::endl;
//     std::clog << "#plengths : " << osg_plengths()->getSize()   << std::endl;
//     std::clog << "#indices  : " << osg_indices()->getSize()    << std::endl;
//     std::clog << "#points   : " << osg_vpositions()->getSize() << std::endl;

    beginEditCP( _geo, Mask );
    {
      addRefCP( osg_ptypes() );
      _geo->setTypes    ( osg_ptypes() );
      addRefCP( osg_plengths() );
      _geo->setLengths  ( osg_plengths() );
      addRefCP( osg_indices() );
      _geo->setIndices  ( osg_indices() );
      addRefCP( osg_vpositions() );
      _geo->setPositions( osg_vpositions() );

      if ( has_vertex_colors() )
      {
	addRefCP( osg_vcolors() );
	_geo->setColors   ( osg_vcolors() );
      }
      if ( has_vertex_normals() )
      {
	addRefCP( osg_vnormals() );
	_geo->setNormals  ( osg_vnormals() );
      }
      if ( has_vertex_texcoords() )
      {
	addRefCP( osg_vtexcoords() );
	_geo->setTexCoords( osg_vtexcoords() );
      }
    }
    endEditCP  (_geo, Mask);

    return true;      
  }

private:
  
  VPropHandleT<Point>         points_;
  VPropHandleT<Normal>        vertex_normals_;
  VPropHandleT<Color>         vertex_colors_;
  VPropHandleT<TexCoord>      vertex_texcoords_;
  VPropHandleT<StatusInfo>    vertex_status_;

  FPTypesHandle               face_types_;
  FPLengthsHandle             face_lengths_;
  FIndicesHandle              face_indices_;

  EPropHandleT<StatusInfo>    edge_status_;
  EPropHandleT<StatusInfo>    edge_colors_;
  HPropHandleT<StatusInfo>    halfedge_status_;

  FPropHandleT<Normal>        face_normals_;
  FPropHandleT<Color>         face_colors_;
  FPropHandleT<StatusInfo>    face_status_;

  unsigned int  refcount_vnormals_;
  unsigned int  refcount_vcolors_;
  unsigned int  refcount_vtexcoords_;
  unsigned int  refcount_vstatus_;
  unsigned int  refcount_estatus_;
  unsigned int  refcount_ecolors_;
  unsigned int  refcount_hstatus_;
  unsigned int  refcount_fnormals_;
  unsigned int  refcount_fcolors_;
  unsigned int  refcount_fstatus_;

};


//=============================================================================
} // namespace Kernel_OSG
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_KERNEL_OSG_ATTRIBKERNEL_HH defined
//=============================================================================

