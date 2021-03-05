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



#include <OpenMesh/Tools/Utils/conio.hh>

// ----------------------------------------------------------------- MSVC Compiler ----
#ifdef _MSC_VER

#include <conio.h>

namespace OpenMesh {
namespace Utils {

int kbhit()  { return ::_kbhit();  }
int getch()  { return ::_getch();  }
int getche() { return ::_getche(); }

} // Tools
} // AS

// ----------------------------------------------------------------- Win32 ----
#elif defined(WIN32) || defined(__MINGW32__)

#include <conio.h>

namespace OpenMesh {
namespace Utils {

int kbhit()  { return ::kbhit();  }
int getch()  { return ::getch();  }
int getche() { return ::getche(); }

} // Tools
} // AS
// ----------------------------------------------------------------- Other ----
#else

// Based on code published by Floyd Davidson in a newsgroup.

#include <stdio.h>     /* stdout, fflush() */
#if !defined(POSIX_1003_1_2001)
#  include <fcntl.h>
#  include <unistd.h>  
#else
#  include <select.h>  /* select()       */
#endif
#include <termios.h>   /* tcsetattr()    */
#include <sys/ioctl.h> /* ioctl()        */
#include <sys/time.h>  /* timeval struct */ 

namespace OpenMesh {
namespace Utils {

#ifdef CTIME
#  undef CTIME
#endif
#define CTIME 1
#define CMIN  1


int kbhit(void)
{
  int cnt = 0;
  int error;
  static struct termios Otty, Ntty;

  tcgetattr(0, &Otty);
  Ntty = Otty;

  Ntty.c_iflag      = 0; /* input mode */
  Ntty.c_oflag      = 0; /* output mode */
  Ntty.c_lflag     &= ~ICANON; /* raw mode */
  Ntty.c_cc[VMIN]   = CMIN; /* minimum chars to wait for */
  Ntty.c_cc[VTIME]  = CTIME; /* minimum wait time */

  if (0 == (error = tcsetattr(0, TCSANOW, &Ntty))) 
  {
    struct timeval tv;
    error += ioctl(0, FIONREAD, &cnt);
    error += tcsetattr(0, TCSANOW, &Otty);
    tv.tv_sec = 0;
    tv.tv_usec = 100; /* insert at least a minimal delay */
    select(1, NULL, NULL, NULL, &tv);
  }
  return (error == 0 ? cnt : -1 );
}


int getch(void)
{
  char ch = ' ';
  int error;
  static struct termios Otty, Ntty;
  
  fflush(stdout);
  tcgetattr(0, &Otty);
  Ntty = Otty;
  
  Ntty.c_iflag     = 0;        // input mode
  Ntty.c_oflag     = 0;        // output mode
  Ntty.c_lflag    &= ~ICANON;  // line settings  
  Ntty.c_lflag    &= ~ECHO;    // enable echo
  Ntty.c_cc[VMIN]  = CMIN;     // minimum chars to wait for
  Ntty.c_cc[VTIME] = CTIME;    // minimum wait time

  // Conditionals allow compiling with or without flushing pre-existing
  // existing buffered input before blocking.
#if 1
  // use this to flush the input buffer before blocking for new input
#  define FLAG TCSAFLUSH
#else
  // use this to return a char from the current input buffer, or block if
  // no input is waiting.
#  define FLAG TCSANOW
#endif

  if (0 == (error = tcsetattr(0, FLAG, &Ntty))) 
  {
    error = read(0, &ch, 1 );           // get char from stdin
    error += tcsetattr(0, FLAG, &Otty); // restore old settings
  }
  return (error == 1 ? (int) ch : -1 );
}


int getche(void)
{
  char ch = ' ';
  int error;
  static struct termios Otty, Ntty;
  
  fflush(stdout);
  tcgetattr(0, &Otty);
  Ntty = Otty;
  
  Ntty.c_iflag     = 0;        // input mode
  Ntty.c_oflag     = 0;        // output mode
  Ntty.c_lflag    &= ~ICANON;  // line settings  
  Ntty.c_lflag    |= ECHO;     // enable echo
  Ntty.c_cc[VMIN]  = CMIN;     // minimum chars to wait for
  Ntty.c_cc[VTIME] = CTIME;    // minimum wait time

  // Conditionals allow compiling with or without flushing pre-existing
  // existing buffered input before blocking.
#if 1
  // use this to flush the input buffer before blocking for new input
#  define FLAG TCSAFLUSH
#else
  // use this to return a char from the current input buffer, or block if
  // no input is waiting.
#  define FLAG TCSANOW
#endif

  if (0 == (error = tcsetattr(0, FLAG, &Ntty))) {
    error = read(0, &ch, 1 );           // get char from stdin
    error += tcsetattr(0, FLAG, &Otty); // restore old settings
  }

  return (error == 1 ? (int) ch : -1 );
}

} // namespace Tools
} // namespace AS
// ----------------------------------------------------------------------------
#endif // System dependent parts
// ============================================================================

//#define Test
#if defined(Test)

#include <ctype.h>

int main (void) 
{ 
   char  msg[] = "press key to continue...";
   char *ptr   = msg;

  while ( !OpenMesh::Utils::kbhit() )
  {
    char* tmp = *ptr;
    *ptr = islower(tmp) ? toupper(tmp) : tolower(tmp);
    printf("\r%s", msg); fflush(stdout);
    *ptr = (char)tmp;
    if (!*(++ptr)) 
      ptr = msg; 
    usleep(20000);
  }

  printf("\r%s.", msg); fflush(stdout);
  OpenMesh::Utils::getch();
  printf("\r%s..", msg); fflush(stdout);
  OpenMesh::Utils::getche();
  return 0;
}

#endif // Test

// ============================================================================
