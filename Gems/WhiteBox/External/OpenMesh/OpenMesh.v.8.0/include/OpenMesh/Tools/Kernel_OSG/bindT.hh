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




/** \file bindT.hh

    Bind an OpenMesh to a OpenSG geometry node. Be aware that due to
    this link the geometry node maybe modified. For instance triangle
    strips are converted to regular triangles.
*/


//=============================================================================
//
//  CLASS Traits
//
//=============================================================================

#ifndef OPENMESH_KERNEL_OSG_BINDT_HH
#define OPENMESH_KERNEL_OSG_BINDT_HH


//== INCLUDES =================================================================


#include <functional>
#include <algorithm>
//
#include <OpenMesh/Core/Mesh/TriMeshT.hh>
#include <OpenMesh/Core/Utils/color_cast.hh>
#include <OpenMesh/Tools/Utils/GLConstAsString.hh>
#include <OpenSG/OSGGeometry.h>
//
#include "color_cast.hh"

//== NAMESPACES ===============================================================

namespace OpenMesh  {
namespace Kernel_OSG {


//== CLASS DEFINITION =========================================================

inline
bool type_is_valid( unsigned char _t )
{
  return _t == GL_TRIANGLES
    ||   _t == GL_TRIANGLE_STRIP
    ||   _t == GL_QUADS
    ||   _t == GL_POLYGON;
}


/** Bind a OpenSG geometry to a mesh.
 *
 *  \param _mesh The mesh object to bind the geometry to.
 *  \param _geo  The geometry object to bind.
 *  \return      true if the connection has been established else false.
 */
template < typename Mesh > inline
bool bind( osg::GeometryPtr& _geo, Mesh& _mesh )
{
  _geo = _mesh.createGeometryPtr();
}

/** Bind a mesh object to geometry. The binder is able to handle
 *  non-indexed and indexed geometry. Multi-indexed geometry is not
 *  supported.
 *
 *  \param _mesh The mesh object to bind.
 *  \param _geo  The geometry object to bind to.
 *  \return      true if the connection has been established else false.
 */
template < typename Mesh > inline
bool bind( Mesh& _mesh, osg::GeometryPtr& _geo )
{
  using namespace OpenMesh;
  using namespace osg;
  using namespace std;

  bool ok = true;

  // pre-check if types are supported

  GeoPTypesPtr types = _geo->getTypes();

  if ( (size_t)count_if( types->getData(), types->getData()+types->size(),
                         ptr_fun(type_is_valid) ) != (size_t)types->size() )
    return false;

  // pre-check if it is a multi-indexed geometry, which is not supported!

  if ( _geo->getIndexMapping().getSize() > 1 )
  {
    omerr << "OpenMesh::Kernel_OSG::bind(): Multi-indexed geometry is not supported!\n";
    return false;
  }


  // create shortcuts

  GeoPLengthsPtr  lengths = _geo->getLengths();
  GeoIndicesPtr   indices = _geo->getIndices();
  GeoPositionsPtr pos     = _geo->getPositions();
  GeoNormalsPtr   normals = _geo->getNormals();
  GeoColorsPtr    colors  = _geo->getColors();
 
  
  // -------------------- now convert everything to polygon/triangles

  size_t tidx, bidx; // types; base index into indices
  vector< VertexHandle > vhandles;

  // ---------- initialize geometry

  {
    VertexHandle vh;
    typedef typename Mesh::Color color_t;

    bool bind_normal = (normals!=NullFC) && _mesh.has_vertex_normals();
    bool bind_color  = (colors !=NullFC) && _mesh.has_vertex_colors();

    for (bidx=0; bidx < pos->size(); ++bidx)
    {
      vh = _mesh.add_vertex( pos->getValue(bidx) );
      if ( bind_normal )
        _mesh.set_normal(vh, normals->getValue(bidx));
      if ( bind_color )
        _mesh.set_color(vh, color_cast<color_t>(colors->getValue(bidx)));
    }
  }

  // ---------- create topology

  FaceHandle   fh;

  size_t max_bidx = indices != NullFC ? indices->size() : pos->size();

  for (bidx=tidx=0; ok && tidx<types->size() && bidx < max_bidx; ++tidx)
  {
    switch( types->getValue(tidx) )
    {
      case GL_TRIANGLES:
        vhandles.resize(3);
        for(size_t lidx=0; lidx < lengths->getValue(tidx)-2; lidx+=3)
        {
          if (indices == NullFC ) {
            vhandles[0] = VertexHandle(bidx+lidx);
            vhandles[1] = VertexHandle(bidx+lidx+1);
            vhandles[2] = VertexHandle(bidx+lidx+2);
          }
          else {
            vhandles[0] = VertexHandle(indices->getValue(bidx+lidx  ) );
            vhandles[1] = VertexHandle(indices->getValue(bidx+lidx+1) );
            vhandles[2] = VertexHandle(indices->getValue(bidx+lidx+2) );
          }

          if ( !(fh = _mesh.add_face( vhandles )).is_valid() )
          {
            // if fh is complex try swapped order
            swap(vhandles[2], vhandles[1]);
            fh = _mesh.add_face( vhandles );
          }
          ok = fh.is_valid();
        }
        break;

      case GL_TRIANGLE_STRIP:
        vhandles.resize(3);
        for (size_t lidx=0; lidx < lengths->getValue(tidx)-2; ++lidx)
        {
          if (indices == NullFC ) {
            vhandles[0] = VertexHandle(bidx+lidx);
            vhandles[1] = VertexHandle(bidx+lidx+1);
            vhandles[2] = VertexHandle(bidx+lidx+2);
          }
          else {
            vhandles[0] = VertexHandle(indices->getValue(bidx+lidx  ) );
            vhandles[1] = VertexHandle(indices->getValue(bidx+lidx+1) );
            vhandles[2] = VertexHandle(indices->getValue(bidx+lidx+2) );
          }

          if (vhandles[0]!=vhandles[2] &&
              vhandles[0]!=vhandles[1] &&
              vhandles[1]!=vhandles[2])
          {
            // if fh is complex try swapped order
            bool swapped(false);

            if (lidx % 2) // odd numbered triplet must be reordered
              swap(vhandles[2], vhandles[1]);
              
            if ( !(fh = _mesh.add_face( vhandles )).is_valid() )
            {
              omlog << "OpenMesh::Kernel_OSG::bind(): complex entity!\n";

              swap(vhandles[2], vhandles[1]);
              fh = _mesh.add_face( vhandles );
              swapped = true;
            }
            ok = fh.is_valid();
          }
        }
        break;

      case GL_QUADS:
        vhandles.resize(4);
        for(size_t nf=_mesh.n_faces(), lidx=0; 
            lidx < lengths->getValue(tidx)-3; lidx+=4)
        {
          if (indices == NullFC ) {
            vhandles[0] = VertexHandle(bidx+lidx);
            vhandles[1] = VertexHandle(bidx+lidx+1);
            vhandles[2] = VertexHandle(bidx+lidx+2);
            vhandles[3] = VertexHandle(bidx+lidx+3);
          }
          else {
            vhandles[0] = VertexHandle(indices->getValue(bidx+lidx  ) );
            vhandles[1] = VertexHandle(indices->getValue(bidx+lidx+1) );
            vhandles[2] = VertexHandle(indices->getValue(bidx+lidx+2) );
            vhandles[3] = VertexHandle(indices->getValue(bidx+lidx+3) );
          }

          fh = _mesh.add_face( vhandles );
          ok = ( Mesh::Face::is_triangle() && (_mesh.n_faces()==(nf+2)))
            || fh.is_valid();
          nf = _mesh.n_faces();
        }
        break;

      case GL_POLYGON:
      {
        size_t ne = lengths->getValue(tidx);
        size_t nf = _mesh.n_faces();

        vhandles.resize(ne);

        for(size_t lidx=0; lidx < ne; ++lidx)
          vhandles[lidx] = (indices == NullFC)
            ? VertexHandle(bidx+lidx)
            : VertexHandle(indices->getValue(bidx+lidx) );

        fh = _mesh.add_face( vhandles );
        ok = ( Mesh::Face::is_triangle() && (_mesh.n_faces()==nf+ne-2) )
          || fh.is_valid();
        
        break;
      }
      default:
        cerr << "Warning! Skipping unsupported type " 
             << types->getValue(tidx) << " '"
             << Utils::GLenum_as_string( types->getValue(tidx) ) << "'\n";
    }

    // update base index into indices for next face type
    bidx += lengths->getValue(tidx);
  }

  if (ok)
    ok=_mesh.bind(_geo);
  else
    _mesh.clear();

  return ok;
}


//=============================================================================
} // namespace Kernel_OSG
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_KERNEL_OSG_BINDT_HH defined
//=============================================================================

