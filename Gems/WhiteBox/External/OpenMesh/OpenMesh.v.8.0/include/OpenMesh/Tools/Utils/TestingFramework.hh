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



#ifndef TESTINGFRAMEWORK_HH
#define TESTINGFRAMEWORK_HH
// ----------------------------------------------------------------------------

/** \file TestingFramework.hh
    This file contains a little framework for test programms
*/


// ----------------------------------------------------------------------------

#include "Config.hh"
#include <iosfwd>
#include <sstream>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <OpenMesh/Core/Utils/Noncopyable.hh>

// ------------------------------------------------------------- namespace ----

namespace OpenMesh { // BEGIN_NS_OPENMESH
namespace Utils { // BEGIN_NS_UTILS


// ----------------------------------------------------------------- class ----
//
// Usage Example
//
// #include <iostream>
// #include <.../TestingFramework.hh>
//
// struct test_func : public TestingFramework::TestFunc
// {
//    typedef test_func Self;
//
//    // define ctor and copy-ctor
//    test_func( TestingFramework& _th, std::string _n ) : TestingFramework::TestFunc( _th, _n ) { }
//    test_func( Self& _cpy ) : TestingFramework::TestFunc(_cpy) { }
//
//    // overload body()
//    void body()
//    {
//
//       // Do the tests
//       // direct call to verify
//       verify( testResult, expectedResult, "additional information" );
//
//       // or use the define TH_VERIFY. The test-expression will be used as the message string
//       TH_VERIFY( testResult, expectedResult );
//
//       ...
//    }
// };
//
// int main(...)
// {
//    TestingFramework testSuite(std::cout); // send output to stdout
//
//    new test_func(testSuite);        // create new test instance. It registers with testSuite.
//    return testSuite.run();
// }
//

// 
#define TH_VERIFY( expr, expt ) \
     verify( expr, expt, #expr )

//
#define TH_VERIFY_X( expr, expt ) \
     verify_x( expr, expt, #expr )

/** Helper class for test programms.
    \internal
 */
class TestingFramework : Noncopyable
{
public:

  typedef TestingFramework Self;
  typedef std::logic_error verify_error;
   
#ifndef DOXY_IGNORE_THIS
  class TestFunc
  {
  public:
    TestFunc( TestingFramework& _th, const std::string& _n ) 
      : th_(_th), name_(_n)
    {
      th_.reg(this);
    }
    
    virtual ~TestFunc()
    { }
    
    void operator() ( void )
    {
      prolog();
      try
      {
        body();
      }
      catch( std::exception& x )
      {
        std::cerr << "<<Error>>: Cannot proceed test due to failure of last"
                  << "  test: " << x.what() << std::endl;
      }
      catch(...)
      {
        std::cerr << "Fatal: cannot proceed test due to unknown error!" 
                  << std::endl;
      }
      epilog();
    }
    
    const TestingFramework& testHelper() const { return th_; }
    
  protected:
    
    virtual void prolog(void)
    {
      begin(name_);
    }
    
    virtual void body(void) = 0;
    
    virtual void epilog(void)
    {
      end();
    }
    
  protected:
    
    TestingFramework& testHelper() { return th_; }
    
    TestFunc( const TestFunc& _cpy ) : th_(_cpy.th_), name_(_cpy.name_) { }
    
    
    // Use the following method in prolog()
    TestFunc& begin(std::string _title, const std::string& _info = "")
    { th_.begin(_title,_info); return *this; }
    
    
    // Use the following method in epilog()
    TestFunc& end(void)
    { th_.end(); return *this; }
    
    
    // Use the followin methods in body()
    
    template <typename ValueType>
    bool
    verify( const ValueType& _rc, const ValueType& _expected, 
            std::string _info )
    { return th_.verify( _rc, _expected, _info ); }
    
    template <typename ValueType>
    void
    verify_x( const ValueType& _rc, const ValueType& _expected, 
              std::string _info )
    {
      if ( !verify(_rc, _expected, _info) )
        throw verify_error(_info);
    }
    
    TestFunc& info(const std::string& _info) 
    { th_.info(_info); return *this; }

    TestFunc& info(const std::ostringstream& _ostr) 
    { th_.info(_ostr); return *this; }
    
  private:      
    TestFunc();
    
  protected:
    TestingFramework& th_;
    std::string name_;      
  };
#endif

  typedef TestFunc*                TestFuncPtr;
  typedef std::vector<TestFuncPtr> TestSet;
  
public:
  
  TestingFramework(std::ostream& _os) 
    : errTotal_(0),    errCount_(0),
      verifyTotal_(0), verifyCount_(0),
      testTotal_(0),   testCount_(0),
      os_(_os)
  { }
  
protected:
  
#ifndef DOXY_IGNORE_THIS
  struct TestDeleter
  {
    void operator() (TestFuncPtr _tfptr) { delete _tfptr; }
  };
#endif

public:   

   virtual ~TestingFramework()
   {
      std::for_each(tests_.begin(), tests_.end(), TestDeleter() );
   }

public:
   
   template <typename ValueType>
   bool verify(const ValueType& _rc, 
	       const ValueType& _expected, 
	       const std::string& _info)
   {
      ++verifyTotal_;
      if ( _rc == _expected )
      {
         os_ << "    " << _info << ", result: " << _rc << ", OK!" << std::endl;
         return true;
      }
      ++errTotal_;
      os_ << "    " << _info << ", result: " << _rc << " != " << _expected
          << " <<ERROR>>" << std::endl;
      return false;
   }
   
   Self& begin(std::string _title, const std::string& _info = "")
   {
      std::ostringstream ostr;
      
      testTitle_ = _title;
      errCount_  = errTotal_;
      ++testTotal_;
      
      ostr << _title;
      if ( !_info.empty() )
         ostr << " ["<< _info << "]";
      testTitle_ = ostr.str();
      os_ << "Begin " << testTitle_ << std::endl;
      return *this;
   }
   
   Self& end()
   {
      if (errorCount()==0)
         ++testCount_;
      
      os_ << "End " << testTitle_ << ": " << errorCount() << " Error(s)." << std::endl;
      return *this;
   }

   Self& info(const std::string& _info)
   {
      os_ << "  + " << _info << std::endl;
      return *this;
   }
   
   Self& info(const std::ostringstream& _ostr)
   {
      os_ << "  + " << _ostr.str() << std::endl;
      return *this;
   }
   
   size_t errorTotal()  const { return errTotal_; }
   size_t errorCount()  const { return errTotal_ - errCount_; }
   size_t verifyTotal() const { return verifyTotal_; }
   size_t verifyCount() const { return verifyTotal_ - verifyCount_; }
   size_t goodTotal()   const { return verifyTotal() - errorTotal(); }
   size_t goodCount()   const { return verifyCount() - errorCount(); }
   
   size_t testTotal()   const { return testTotal_; }
   size_t testCount()   const { return testCount_; }

public:

   int run(void)
   {
      os_ << "Test started\n";
      TestRunner executer;
      std::for_each(tests_.begin(), tests_.end(), executer );
      os_ << std::endl;
      os_ << "All tests completed" << std::endl
          << "   #Tests: " << testCount() << "/" << testTotal() << std::endl
          << "  #Errors: " << errorTotal() << "/" << verifyTotal() << std::endl;
      return errorTotal();
   }

protected:

#ifndef DOXY_IGNORE_THIS
  struct TestRunner
  {
    void operator() (TestFuncPtr _tfptr) { (*_tfptr)(); }
  };   
#endif

  int reg(TestFuncPtr _tfptr)
  {
    tests_.push_back(_tfptr);
    return true;
  }

  friend class TestFunc;
   
private:

  size_t errTotal_;    
  size_t errCount_;    
  size_t verifyTotal_;
  size_t verifyCount_;   
  size_t testTotal_;   // #Tests
  size_t testCount_;   // #Tests ohne Fehler
  
  std::string testTitle_;
  std::ostream& os_;
  
  TestSet tests_;
  
};

// ============================================================================
} // END_NS_UTILS
} // END_NS_OPENMESH
// ============================================================================
#endif // TESTINGFRMEWORK_HH
// ============================================================================
