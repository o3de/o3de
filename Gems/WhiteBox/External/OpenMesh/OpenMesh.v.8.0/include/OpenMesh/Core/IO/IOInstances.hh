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




//=============================================================================
//
//  Helper file for static builds
//
//  In opposite to dynamic builds where the instance of every reader module
//  is generated within the OpenMesh library, static builds only instanciate
//  objects that are at least referenced once. As all reader modules are
//  never used directly, they will not be part of a static build, hence
//  this file.
//
//=============================================================================


#ifndef __IOINSTANCES_HH__
#define __IOINSTANCES_HH__

#if defined(OM_STATIC_BUILD) || defined(ARCH_DARWIN)

//=============================================================================

#include <OpenMesh/Core/System/config.h>

#include <OpenMesh/Core/IO/reader/BaseReader.hh>
#include <OpenMesh/Core/IO/reader/OBJReader.hh>
#include <OpenMesh/Core/IO/reader/OFFReader.hh>
#include <OpenMesh/Core/IO/reader/PLYReader.hh>
#include <OpenMesh/Core/IO/reader/STLReader.hh>
#include <OpenMesh/Core/IO/reader/OMReader.hh>

#include <OpenMesh/Core/IO/writer/BaseWriter.hh>
#include <OpenMesh/Core/IO/writer/OBJWriter.hh>
#include <OpenMesh/Core/IO/writer/OFFWriter.hh>
#include <OpenMesh/Core/IO/writer/STLWriter.hh>
#include <OpenMesh/Core/IO/writer/OMWriter.hh>
#include <OpenMesh/Core/IO/writer/PLYWriter.hh>
#include <OpenMesh/Core/IO/writer/VTKWriter.hh>

//=== NAMESPACES ==============================================================

namespace OpenMesh {
namespace IO {

//=============================================================================


// Instanciate every Reader module
static BaseReader* OFFReaderInstance = &OFFReader();
static BaseReader* OBJReaderInstance = &OBJReader();
static BaseReader* PLYReaderInstance = &PLYReader();
static BaseReader* STLReaderInstance = &STLReader();
static BaseReader* OMReaderInstance  = &OMReader();

// Instanciate every writer module
static BaseWriter* OBJWriterInstance = &OBJWriter();
static BaseWriter* OFFWriterInstance = &OFFWriter();
static BaseWriter* STLWriterInstance = &STLWriter();
static BaseWriter* OMWriterInstance  = &OMWriter();
static BaseWriter* PLYWriterInstance = &PLYWriter();
static BaseWriter* VTKWriterInstance = &VTKWriter();


//=============================================================================
} // namespace IO
} // namespace OpenMesh
//=============================================================================
#endif // static ?
#endif //__IOINSTANCES_HH__
//=============================================================================
