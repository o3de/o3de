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
//  Helper Functions for binary reading / writing
//
//=============================================================================


//== INCLUDES =================================================================


// -------------------- STL
#include <algorithm>
#include <iostream>
// -------------------- OpenMesh
#include <OpenMesh/Core/IO/BinaryHelper.hh>


//== NAMESPACES ===============================================================


namespace OpenMesh {
namespace IO {

#ifndef DOXY_IGNORE_THIS

//== IMPLEMENTATION ===========================================================

//-----------------------------------------------------------------------------

short int read_short(FILE* _in, bool _swap)
{
  union u1 { short int s; unsigned char c[2]; }  sc;
  fread((char*)sc.c, 1, 2, _in);
  if (_swap) std::swap(sc.c[0], sc.c[1]);
  return sc.s;
}


//-----------------------------------------------------------------------------


int read_int(FILE* _in, bool _swap)
{
  union u2 { int i; unsigned char c[4]; } ic;
  fread((char*)ic.c, 1, 4, _in);
  if (_swap) {
    std::swap(ic.c[0], ic.c[3]);
    std::swap(ic.c[1], ic.c[2]);
  }
  return ic.i;
}


//-----------------------------------------------------------------------------


float read_float(FILE* _in, bool _swap)
{
  union u3 { float f; unsigned char c[4]; } fc;
  fread((char*)fc.c, 1, 4, _in);
  if (_swap) {
    std::swap(fc.c[0], fc.c[3]);
    std::swap(fc.c[1], fc.c[2]);
  }
  return fc.f;
}


//-----------------------------------------------------------------------------


double read_double(FILE* _in, bool _swap)
{
  union u4 { double d; unsigned char c[8]; } dc;
  fread((char*)dc.c, 1, 8, _in);
  if (_swap) {
    std::swap(dc.c[0], dc.c[7]);
    std::swap(dc.c[1], dc.c[6]);
    std::swap(dc.c[2], dc.c[5]);
    std::swap(dc.c[3], dc.c[4]);
  }
  return dc.d;
}

//-----------------------------------------------------------------------------

short int read_short(std::istream& _in, bool _swap)
{
  union u1 { short int s; unsigned char c[2]; }  sc;
  _in.read((char*)sc.c, 2);
  if (_swap) std::swap(sc.c[0], sc.c[1]);
  return sc.s;
}


//-----------------------------------------------------------------------------


int read_int(std::istream& _in, bool _swap)
{
  union u2 { int i; unsigned char c[4]; } ic;
  _in.read((char*)ic.c, 4);
  if (_swap) {
    std::swap(ic.c[0], ic.c[3]);
    std::swap(ic.c[1], ic.c[2]);
  }
  return ic.i;
}


//-----------------------------------------------------------------------------


float read_float(std::istream& _in, bool _swap)
{
  union u3 { float f; unsigned char c[4]; } fc;
  _in.read((char*)fc.c, 4);
  if (_swap) {
    std::swap(fc.c[0], fc.c[3]);
    std::swap(fc.c[1], fc.c[2]);
  }
  return fc.f;
}


//-----------------------------------------------------------------------------


double read_double(std::istream& _in, bool _swap)
{
  union u4 { double d; unsigned char c[8]; } dc;
  _in.read((char*)dc.c, 8);
  if (_swap) {
    std::swap(dc.c[0], dc.c[7]);
    std::swap(dc.c[1], dc.c[6]);
    std::swap(dc.c[2], dc.c[5]);
    std::swap(dc.c[3], dc.c[4]);
  }
  return dc.d;
}


//-----------------------------------------------------------------------------


void write_short(short int _i, FILE* _out, bool _swap)
{
  union u1 { short int s; unsigned char c[2]; } sc;
  sc.s = _i;
  if (_swap) std::swap(sc.c[0], sc.c[1]);
  fwrite((char*)sc.c, 1, 2, _out);
}


//-----------------------------------------------------------------------------


void write_int(int _i, FILE* _out, bool _swap)
{
  union u2 { int i; unsigned char c[4]; } ic;
  ic.i = _i;
  if (_swap) {
    std::swap(ic.c[0], ic.c[3]);
    std::swap(ic.c[1], ic.c[2]);
  }
  fwrite((char*)ic.c, 1, 4, _out);
}


//-----------------------------------------------------------------------------


void write_float(float _f, FILE* _out, bool _swap)
{
  union u3 { float f; unsigned char c[4]; } fc;
  fc.f = _f;
  if (_swap) {
    std::swap(fc.c[0], fc.c[3]);
    std::swap(fc.c[1], fc.c[2]);
  }
  fwrite((char*)fc.c, 1, 4, _out);
}


//-----------------------------------------------------------------------------


void write_double(double _d, FILE* _out, bool _swap)
{
  union u4 { double d; unsigned char c[8]; } dc;
  dc.d = _d;
  if (_swap) {
    std::swap(dc.c[0], dc.c[7]);
    std::swap(dc.c[1], dc.c[6]);
    std::swap(dc.c[2], dc.c[5]);
    std::swap(dc.c[3], dc.c[4]);
  }
  fwrite((char*)dc.c, 1, 8, _out);
}


//-----------------------------------------------------------------------------


void write_short(short int _i, std::ostream& _out, bool _swap)
{
  union u1 { short int s; unsigned char c[2]; } sc;
  sc.s = _i;
  if (_swap) std::swap(sc.c[0], sc.c[1]);
  _out.write((char*)sc.c, 2);
}


//-----------------------------------------------------------------------------


void write_int(int _i, std::ostream& _out, bool _swap)
{
  union u2 { int i; unsigned char c[4]; } ic;
  ic.i = _i;
  if (_swap) {
    std::swap(ic.c[0], ic.c[3]);
    std::swap(ic.c[1], ic.c[2]);
  }
  _out.write((char*)ic.c, 4);
}


//-----------------------------------------------------------------------------


void write_float(float _f, std::ostream& _out, bool _swap)
{
  union u3 { float f; unsigned char c[4]; } fc;
  fc.f = _f;
  if (_swap) {
    std::swap(fc.c[0], fc.c[3]);
    std::swap(fc.c[1], fc.c[2]);
  }
  _out.write((char*)fc.c, 4);
}


//-----------------------------------------------------------------------------


void write_double(double _d, std::ostream& _out, bool _swap)
{
  union u4 { double d; unsigned char c[8]; } dc;
  dc.d = _d;
  if (_swap) {
    std::swap(dc.c[0], dc.c[7]);
    std::swap(dc.c[1], dc.c[6]);
    std::swap(dc.c[2], dc.c[5]);
    std::swap(dc.c[3], dc.c[4]);
  }
  _out.write((char*)dc.c, 8);
}


#endif

//=============================================================================
} // namespace IO
} // namespace OpenMesh
//=============================================================================
