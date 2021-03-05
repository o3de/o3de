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




#ifndef LOOPSCHEMEMASKT_HH
#define LOOPSCHEMEMASKT_HH

#include <cmath>
#include <vector>

#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/Utils/SingletonT.hh>

namespace OpenMesh
{

/** implements cache for the weights of the original Loop scheme
    supported:
      - vertex projection rule on the next level
      - vertex projection rule on the limit surface
      - vertex projection rule on the k-th (level) step (Barthe, Kobbelt'2003)
      - vertex tangents on the limit surface
*/

template <class T_, unsigned int cache_size_ = 100>
class LoopSchemeMaskT
{
public:
  enum { cache_size = cache_size_ };
  typedef T_                                Scalar;

protected:

  Scalar                                    proj_weights_[cache_size];
  Scalar                                    limit_weights_[cache_size];
  Scalar                                    step_weights_[cache_size];
  std::vector<Scalar>                       tang0_weights_[cache_size];
  std::vector<Scalar>                       tang1_weights_[cache_size];

protected:

  inline static Scalar                      compute_proj_weight(uint _valence)
  {
    //return pow(3.0 / 2.0 + cos(2.0 * M_PI / _valence), 2) / 2.0 - 1.0;
    double denom = (3.0 + 2.0*cos(2.0*M_PI/(double)_valence));
    double weight = (64.0*_valence)/(40.0 - denom*denom) - _valence;
    return (Scalar) weight;
  }

  inline static Scalar                      compute_limit_weight(uint _valence)
  {
    double proj_weight = compute_proj_weight(_valence);
    proj_weight = proj_weight/(proj_weight + _valence);//normalize the proj_weight
    double weight = (3.0/8.0)/(1.0 - proj_weight + (3.0/8.0));
    return (Scalar)weight;
  }

  inline static Scalar                      compute_step_weight(uint _valence)
  {
    double proj_weight = compute_proj_weight(_valence);
    proj_weight = proj_weight/(proj_weight + _valence);//normalize the proj_weight
    double weight = proj_weight - (3.0/8.0);
    return (Scalar)weight;
  }

  inline static Scalar                      compute_tang0_weight(uint _valence, uint _ver_id)
  {
    return (Scalar)cos(2.0*M_PI*(double)_ver_id/(double)_valence);
  }

  inline static Scalar                      compute_tang1_weight(uint _valence, uint _ver_id)
  {
    return (Scalar)sin(2.0*M_PI*(double)_ver_id/(double)_valence);
  }

  void                                      cache_weights()
  {
    proj_weights_[0] = 1;
    for (uint k = 1; k < cache_size; ++k)
    {
      proj_weights_[k] = compute_proj_weight(k);
      limit_weights_[k] = compute_limit_weight(k);
      step_weights_[k] = compute_step_weight(k);
      tang0_weights_[k].resize(k);
      tang1_weights_[k].resize(k);
      for (uint i = 0; i < k; ++i)
      {
        tang0_weights_[k][i] = compute_tang0_weight(k,i);
        tang1_weights_[k][i] = compute_tang1_weight(k,i);
      }
    }
  }

public:

  LoopSchemeMaskT()
  {
    cache_weights();
  }

  inline Scalar                             proj_weight(uint _valence) const
  {
    assert(_valence < cache_size );
    return proj_weights_[_valence];
  }

  inline Scalar                             limit_weight(uint _valence) const
  {
    assert(_valence < cache_size );
    return limit_weights_[_valence];
  }

  inline Scalar                             step_weight(uint _valence, uint _step) const
  {
    assert(_valence < cache_size);
    return pow(step_weights_[_valence], (int)_step);//can be precomputed
  }

  inline Scalar                             tang0_weight(uint _valence, uint _ver_id) const
  {
    assert(_valence < cache_size );
    assert(_ver_id < _valence);
    return tang0_weights_[_valence][_ver_id];
  }

  inline Scalar                             tang1_weight(uint _valence, uint _ver_id) const
  {
    assert(_valence < cache_size );
    assert(_ver_id < _valence);
    return tang1_weights_[_valence][_ver_id];
  }

  void                                      dump(uint _max_valency = cache_size - 1) const
  {
    assert(_max_valency <= cache_size - 1);
    //CConsole::printf("(k : pw_k, lw_k): ");
    for (uint i = 0;  i <= _max_valency; ++i)
    {
      //CConsole::stream() << "(" << i << " : " << proj_weight(i) << ", " << limit_weight(i) << ", " << step_weight(i,1) << "), ";
    }
    //CConsole::printf("\n");
  }
};

typedef LoopSchemeMaskT<double, 100>        LoopSchemeMaskDouble;
typedef SingletonT<LoopSchemeMaskDouble>    LoopSchemeMaskDoubleSingleton;

}//namespace OpenMesh

#endif//LOOPSCHEMEMASKT_HH

