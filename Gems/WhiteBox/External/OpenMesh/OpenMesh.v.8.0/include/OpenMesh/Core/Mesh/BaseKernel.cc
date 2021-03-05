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



#include <OpenMesh/Core/Mesh/BaseKernel.hh>
#include <iostream>

namespace OpenMesh
{

void BaseKernel::property_stats() const
{
  property_stats(std::clog);
}
void BaseKernel::property_stats(std::ostream& _ostr) const
{
  const PropertyContainer::Properties& vps = vprops_.properties();
  const PropertyContainer::Properties& hps = hprops_.properties();
  const PropertyContainer::Properties& eps = eprops_.properties();
  const PropertyContainer::Properties& fps = fprops_.properties();
  const PropertyContainer::Properties& mps = mprops_.properties();

  PropertyContainer::Properties::const_iterator it;

  _ostr << vprops_.size() << " vprops:\n";
  for (it=vps.begin(); it!=vps.end(); ++it)
  {
    *it == NULL ? (void)(_ostr << "[deleted]" << "\n") : (*it)->stats(_ostr);
  }
  _ostr << hprops_.size() << " hprops:\n";
  for (it=hps.begin(); it!=hps.end(); ++it)
  {
    *it == NULL ? (void)(_ostr << "[deleted]" << "\n") : (*it)->stats(_ostr);
  }
  _ostr << eprops_.size() << " eprops:\n";
  for (it=eps.begin(); it!=eps.end(); ++it)
  {
    *it == NULL ? (void)(_ostr << "[deleted]" << "\n") : (*it)->stats(_ostr);
  }
  _ostr << fprops_.size() << " fprops:\n";
  for (it=fps.begin(); it!=fps.end(); ++it)
  {
    *it == NULL ? (void)(_ostr << "[deleted]" << "\n") : (*it)->stats(_ostr);
  }
  _ostr << mprops_.size() << " mprops:\n";
  for (it=mps.begin(); it!=mps.end(); ++it)
  {
    *it == NULL ? (void)(_ostr << "[deleted]" << "\n") : (*it)->stats(_ostr);
  }
}



void BaseKernel::vprop_stats( std::string& _string ) const
{
  _string.clear();

  PropertyContainer::Properties::const_iterator it;
  const PropertyContainer::Properties& vps = vprops_.properties();
  for (it=vps.begin(); it!=vps.end(); ++it)
    if ( *it == NULL )
      _string += "[deleted] \n";
    else {
      _string += (*it)->name();
      _string += "\n";
    }

}

void BaseKernel::hprop_stats( std::string& _string ) const
{
  _string.clear();

  PropertyContainer::Properties::const_iterator it;
  const PropertyContainer::Properties& hps = hprops_.properties();
  for (it=hps.begin(); it!=hps.end(); ++it)
    if ( *it == NULL )
      _string += "[deleted] \n";
    else {
      _string += (*it)->name();
      _string += "\n";
    }

}

void BaseKernel::eprop_stats( std::string& _string ) const
{
  _string.clear();

  PropertyContainer::Properties::const_iterator it;
  const PropertyContainer::Properties& eps = eprops_.properties();
  for (it=eps.begin(); it!=eps.end(); ++it)
    if ( *it == NULL )
      _string += "[deleted] \n";
    else {
      _string += (*it)->name();
      _string += "\n";
    }

}
void BaseKernel::fprop_stats( std::string& _string ) const
{
  _string.clear();

  PropertyContainer::Properties::const_iterator it;
  const PropertyContainer::Properties& fps = fprops_.properties();
  for (it=fps.begin(); it!=fps.end(); ++it)
    if ( *it == NULL )
      _string += "[deleted] \n";
    else {
      _string += (*it)->name();
      _string += "\n";
    }

}

void BaseKernel::mprop_stats( std::string& _string ) const
{
  _string.clear();

  PropertyContainer::Properties::const_iterator it;
  const PropertyContainer::Properties& mps = mprops_.properties();
  for (it=mps.begin(); it!=mps.end(); ++it)
    if ( *it == NULL )
      _string += "[deleted] \n";
    else {
      _string += (*it)->name();
      _string += "\n";
    }

}

void BaseKernel::vprop_stats() const
{
  vprop_stats(std::clog);
}
void BaseKernel::vprop_stats(std::ostream& _ostr ) const
{
  PropertyContainer::Properties::const_iterator it;
  const PropertyContainer::Properties& vps = vprops_.properties();
  for (it=vps.begin(); it!=vps.end(); ++it)
    *it == NULL ? (void)(_ostr << "[deleted]" << "\n") : (*it)->stats(_ostr);

}
void BaseKernel::hprop_stats() const
{
  hprop_stats(std::clog);
}
void BaseKernel::hprop_stats(std::ostream& _ostr ) const
{
  PropertyContainer::Properties::const_iterator it;
  const PropertyContainer::Properties& hps = hprops_.properties();
  for (it=hps.begin(); it!=hps.end(); ++it)
    *it == NULL ? (void)(_ostr << "[deleted]" << "\n") : (*it)->stats(_ostr);

}
void BaseKernel::eprop_stats() const
{
  eprop_stats(std::clog);
}
void BaseKernel::eprop_stats(std::ostream& _ostr ) const
{
  PropertyContainer::Properties::const_iterator it;
  const PropertyContainer::Properties& eps = eprops_.properties();
  for (it=eps.begin(); it!=eps.end(); ++it)
    *it == NULL ? (void)(_ostr << "[deleted]" << "\n") : (*it)->stats(_ostr);

}
void BaseKernel::fprop_stats() const
{
  fprop_stats(std::clog);
}
void BaseKernel::fprop_stats(std::ostream& _ostr ) const
{
  PropertyContainer::Properties::const_iterator it;
  const PropertyContainer::Properties& fps = fprops_.properties();
  for (it=fps.begin(); it!=fps.end(); ++it)
    *it == NULL ? (void)(_ostr << "[deleted]" << "\n") : (*it)->stats(_ostr);

}
void BaseKernel::mprop_stats() const
{
  mprop_stats(std::clog);
}
void BaseKernel::mprop_stats(std::ostream& _ostr ) const
{
  PropertyContainer::Properties::const_iterator it;
  const PropertyContainer::Properties& mps = mprops_.properties();
  for (it=mps.begin(); it!=mps.end(); ++it)
    *it == NULL ? (void)(_ostr << "[deleted]" << "\n") : (*it)->stats(_ostr);

}


}
