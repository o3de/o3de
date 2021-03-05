////////////////////////////////////////////
//
// A C++ interface to gnuplot. 
//
// This is a direct translation from the C interface
// written by N. Devillard (which is available from
// http://ndevilla.free.fr/gnuplot/).
//
// As in the C interface this uses pipes and so wont
// run on a system that doesn't have POSIX pipe 
// support
//
// Rajarshi Guha
// <rajarshi@presidency.com>
//
// 07/03/03
//
////////////////////////////////////////////
//
// A little correction for Win32 compatibility
// and MS VC 6.0 done by V.Chyzhdzenka 
//
// Notes:
// 1. Added private method Gnuplot::init().
// 2. Temporary file is created in th current
//    folder but not in /tmp.
// 3. Added #indef WIN32 e.t.c. where is needed.
// 4. Added private member m_sGNUPlotFileName is
//    a name of executed GNUPlot file.
//
// Viktor Chyzhdzenka
// e-mail: chyzhdzenka@mail.ru
//
// 20/05/03
//
////////////////////////////////////////////

#ifndef _GNUPLOT_HH
#define _GNUPLOT_HH

#include <OpenMesh/Core/System/config.hh>
// #ifndef WIN32
// #  include <unistd.h>
// #else
// #  pragma warning (disable : 4786) // Disable 4786 warning for MS VC 6.0
// #endif
#if defined(OM_CC_MIPS)
#  include <stdio.h>
#else
#  include <cstdio>
#endif
#include <string>
#include <vector>
#include <stdexcept>

// ----------------------------------------------------------------------------

#ifdef WIN32
#  define GP_MAX_TMP_FILES    27 //27 temporary files it's Microsoft restriction
#else
#  define GP_MAX_TMP_FILES    64
#  define GP_TMP_NAME_SIZE    512
#  define GP_TITLE_SIZE       80
#endif
#define GP_CMD_SIZE         1024

// ----------------------------------------------------------------------------

using namespace std;

// ----------------------------------------------------------------------------

/// Exception thrown by class Gnuplot
class GnuplotException : public runtime_error
{
public:
  explicit GnuplotException(const string &msg) : runtime_error(msg){}
};

// ----------------------------------------------------------------------------

/** Utility class interfacing with Gnuplot.
 * 
 *  \note The plot will be visible as long as the object is not destructed.
 *
 *  \author Rajarshi Guha (C++ API based on the C API by Nicolas Devillard)
 *
 *  \see <a
 *  href="http://ndevilla.free.fr/gnuplot/">http://ndevilla.free.fr/gnuplot/</a>
 *  more information.
 */
class Gnuplot
{
private:

  FILE            *gnucmd;
  string           pstyle;
  vector<string>   to_delete;
  int              nplots;
  bool             get_program_path(const string& );
  bool             valid;

  // Name of executed GNUPlot file
  static string    gnuplot_executable_;

  void init();

public:

  /// \name Constructors
  //@{
  /// Default constructor.
  Gnuplot();
  
  /// Set a style during construction.
  explicit Gnuplot(const string & _style);

  /// Constructor calling plot_xy().
  Gnuplot(const string & _title,
          const string & _style,
          const string & _xlabel,
          const string & _ylabel,
          vector<double> _x, vector<double> _y);
  
  /// Constructor calling plot_x().
  Gnuplot(const string &_title,
          const string &_style,
          const string &_xlabel,
          const string &_ylabel,
          vector<double> _x);
  //@}

  ~Gnuplot();
  
  /// Send a command to gnuplot (low-level function use by all plot functions.)
  void cmd(const char *_cmd, ...);
  
  /// \name Gnuplot settings
  //@{
  void set_style(const string & _style);   ///< set line style
  void set_ylabel(const string & _ylabel); ///< set x axis label
  void set_xlabel(const string & _xlabel); ///< set x axis label
  //@}

  /// \name plot functions
  //@{

  /// Plot a single vector
  void plot_x(vector<double> _x, const string &_title);
  
  /// Plot x,y pairs
  void plot_xy(vector<double> _x, vector<double> _y, const string  &_title);
  
  /// Plot an equation of the form: y = ax + b
  /// You supply a and b
  void plot_slope(
                  double _a,
                  double _b,
                  const string & _title
                  );
  
  /// Plot an equation supplied as a string
  void plot_equation(
                     const string & _equation,
                     const string & _title
                     );
  
  /// If multiple plots are present it will clear the plot area
  void reset_plot(void);

  //@}

  /// Is \c Self valid?
  bool is_valid(void) const { return valid; }

  /// Is \c Self active, i.e. does it have an active plot?
  bool is_active(void) const { return this->nplots > 0; }
};


// ----------------------------------------------------------------------------
#endif // _GNUPLOT_HH
// ============================================================================

