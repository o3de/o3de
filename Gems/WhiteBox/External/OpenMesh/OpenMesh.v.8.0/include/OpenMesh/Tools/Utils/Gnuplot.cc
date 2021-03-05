////////////////////////////////////////////
//
// A C++ interface to gnuplot. 
//
// This is a direct translation from the C interface
// written by N. Devillard (which is available from
// http://ndevilla.free.fr/gnuplot/).
//
// As in the C interface this uses pipes and so wont
// run on a system that does'nt have POSIX pipe 
// support
//
// Rajarshi Guha
// <rajarshi@presidency.com>
//
// 07/03/03
//
////////////////////////////////////////////

#include "Gnuplot.hh"
#include <stdarg.h>
#ifdef WIN32
#  include <io.h>
#else
#  include <fcntl.h>  // X_OK
#  include <unistd.h> // access
#  define PATH_MAXNAMESZ       4096
#endif
#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <algorithm>

#if defined(WIN32)
#  define pclose _pclose
#  define popen  _popen
#  define access _access
#  define ACCESS_OK 0
#  define PATH_SEP ";"
#  define MKTEMP_AND_CHECK_FAILED(name) (_mktemp(name) == NULL)
#else
#  define ACCESS_OK X_OK
#  define PATH_SEP ":"
#  define MKTEMP_AND_CHECK_FAILED(name) (mkstemp(name) == -1)
#endif

#ifndef WIN32

#include <string.h>
#else
  #ifdef __MINGW32__
    #include <stdlib.h>
    #include <string.h>
  #endif
#endif

using namespace std;

/////////////////////////////
//
// A string tokenizer taken from
// http://www.sunsite.ualberta.ca/
//        Documentation/Gnu/libstdc++-2.90.8/html/21_strings/stringtok_std_h.txt
//
/////////////////////////////

template <typename Container>
void
stringtok (Container &container, string const &in,
           const char * const delimiters = " \t\n")
{
  const string::size_type len = in.length();

  string::size_type i = 0;

  while ( i < len )
  {
    // eat leading whitespace
    i = in.find_first_not_of (delimiters, i);
    if (i == string::npos)
      return;   // nothing left but white space
    
    // find the end of the token
    string::size_type j = in.find_first_of (delimiters, i);
    
    // push token
    if (j == string::npos) 
    {
      container.push_back (in.substr(i));
      return;
    } else
      container.push_back (in.substr(i, j-i));
    
    // set up for next loop
    i = j + 1;
  }
}

// ----------------------------------------------------------------------------

#ifdef WIN32
  std::string Gnuplot::gnuplot_executable_ = "pgnuplot.exe";
#else
  std::string Gnuplot::gnuplot_executable_ = "gnuplot";
#endif

// ----------------------------------------------------------------------------

Gnuplot::Gnuplot(void)
{
  init();
  set_style("points");
}

// ----------------------------------------------------------------------------

Gnuplot::Gnuplot(const string &style)
{
  init();
  set_style(style);
}

// ----------------------------------------------------------------------------

Gnuplot::Gnuplot(const string &title,
                 const string &style,
                 const string &labelx,  const string &labely,
                 vector<double> x, vector<double> y)
{
  init();
		
  if (x.empty() || y.empty() )
    throw GnuplotException("vectors too small");
  
  if (style == "")
    this->set_style("lines");
  else
    this->set_style(style);
  
  if (labelx == "")
    this->set_xlabel("X");
  else
    this->set_xlabel(labelx);
  if (labely == "")
    this->set_ylabel("Y");
  else
    this->set_ylabel(labely);
  
  this->plot_xy(x,y,title);
  
  cout << "Press enter to continue" << endl;
  while (getchar() != '\n'){}
}

// ----------------------------------------------------------------------------

Gnuplot::Gnuplot(const string &title,  const string &style,
                 const string &labelx, const string &labely,
                 vector<double> x)
{
  init();
  
  if (x.empty() )
    throw GnuplotException("vector too small");
  if (!this->gnucmd)
    throw GnuplotException("Could'nt open connection to gnuplot");
  
  if (style == "")
    this->set_style("lines");
    else
      this->set_style(style);
  
  if (labelx == "")
    this->set_xlabel("X");
  else
    this->set_xlabel(labelx);
  if (labely == "")
    this->set_ylabel("Y");
  else
    this->set_ylabel(labely);
  
  this->plot_x(x,title);
  
  cout << "Press enter to continue" << endl;
  while (getchar() != '\n'){}
}

// ----------------------------------------------------------------------------

Gnuplot::~Gnuplot()
{
  if ( !((this->to_delete).empty()) )
  {
    for (size_t i = 0; i < this->to_delete.size(); i++)
      remove(this->to_delete[i].c_str());
  }
  
  if (pclose(this->gnucmd) == -1)
    cerr << "Problem closing communication to gnuplot" << endl;
  
  return;
}

// ----------------------------------------------------------------------------

bool Gnuplot::get_program_path(const string& pname)
{
  list<string> ls;
  char *path;
  
  path = getenv("PATH");
  if (!path)
    return false;

  stringtok(ls, path, PATH_SEP);

  for (list<string>::const_iterator i = ls.begin(); i != ls.end(); ++i)
  {    
    string tmp = (*i) + "/" + pname;
    if ( access(tmp.c_str(), ACCESS_OK) == 0 )
      return true;
  }

  return false;
}

// ----------------------------------------------------------------------------

void Gnuplot::reset_plot(void)
{       
  if ( !(this->to_delete.empty()) )
  {
    for (size_t i = 0; i < this->to_delete.size(); i++)
      remove(this->to_delete[i].c_str());
  }
  this->nplots = 0;
  return;
}

// ----------------------------------------------------------------------------

void Gnuplot::set_style(const string &stylestr)
{
  if (stylestr != "lines" &&
      stylestr != "points" &&
      stylestr != "linespoints" &&
      stylestr != "impulses" &&
      stylestr != "dots" &&
      stylestr != "steps" &&
      stylestr != "errorbars" &&
      stylestr != "boxes" &&
      stylestr != "boxerrorbars")
    this->pstyle = string("points");
  else
    this->pstyle = stylestr;
}

// ----------------------------------------------------------------------------

void Gnuplot::cmd(const char *_cmd, ...)
{
    va_list ap;
    char local_cmd[GP_CMD_SIZE];

    va_start(ap, _cmd);
    vsprintf(local_cmd, _cmd, ap);
    va_end(ap);

    strcat(local_cmd,"\n");
    fputs(local_cmd,this->gnucmd);
    fflush(this->gnucmd);
    return;
}

// ----------------------------------------------------------------------------

void Gnuplot::set_ylabel(const string &label)
{
    ostringstream cmdstr;

    cmdstr << "set xlabel \"" << label << "\"";
    this->cmd(cmdstr.str().c_str());

    return;
}

// ----------------------------------------------------------------------------

void Gnuplot::set_xlabel(const string &label)
{
    ostringstream cmdstr;

    cmdstr << "set xlabel \"" << label << "\"";
    this->cmd(cmdstr.str().c_str());

    return;
}

// ----------------------------------------------------------------------------

// Plots a linear equation (where you supply the
// slope and intercept)
//
void Gnuplot::plot_slope(double a, double b, const string &title)
{
    ostringstream stitle;
    ostringstream cmdstr;

    if (title == "")
        stitle << "no title";
    else
        stitle << title;

    if (this->nplots > 0)
        cmdstr << "replot " << a << " * x + " << b << " title \"" << stitle.str() << "\" with " << pstyle;
    else
        cmdstr << "plot " << a << " * x + " << b << " title \"" << stitle.str() << "\" with " << pstyle;
    this->cmd(cmdstr.str().c_str());
    this->nplots++;
    return;
}

// ----------------------------------------------------------------------------

// Plot an equation which is supplied as a string
// 
void Gnuplot::plot_equation(const string &equation, const string &title)
{
    string titlestr, plotstr;
    ostringstream cmdstr;

    if (title == "")
        titlestr = "no title";
    else
        titlestr = title;

    if (this->nplots > 0)
        plotstr = "replot";
    else
        plotstr = "plot";

    cmdstr << plotstr << " " << equation << " " << "title \"" << titlestr << "\" with " << this->pstyle;
    this->cmd(cmdstr.str().c_str());
    this->nplots++;

    return;
}

// ----------------------------------------------------------------------------

void Gnuplot::plot_x(vector<double> d, const string &title)
{
  ofstream tmp;
  ostringstream cmdstr;
#ifdef WIN32
  char name[] = "gnuplotiXXXXXX";
#else
  char name[] = "/tmp/gnuplotiXXXXXX";
#endif

  if (this->to_delete.size() == GP_MAX_TMP_FILES - 1)
  {
    cerr << "Maximum number of temporary files reached (" << GP_MAX_TMP_FILES << "): cannot open more files" << endl;
    return;
  }
  
  //
  //open temporary files for output
#ifdef WIN32
  if ( _mktemp(name) == NULL)
#else
  if ( mkstemp(name) == -1 )
#endif
  {
    cerr << "Cannot create temporary file: exiting plot" << endl;
    return;
  }
  tmp.open(name);
  if (tmp.bad())
  {
    cerr << "Cannot create temorary file: exiting plot" << endl;
    return;
  }

  //
  // Save the temporary filename
  // 
  this->to_delete.push_back(name);

  //
  // write the data to file
  //
  for (size_t i = 0; i < d.size(); i++)
    tmp << d[i] << endl;
  tmp.flush();    
  tmp.close();
  
  //
  // command to be sent to gnuplot
  //
  cmdstr << ( (this->nplots > 0) ? "replot " : "plot ");

  if (title.empty())
    cmdstr << "\"" << name << "\" with " << this->pstyle;
  else
    cmdstr << "\"" << name << "\" title \"" << title << "\" with " 
           << this->pstyle;
  
  //
  // Do the actual plot
  //
  this->cmd(cmdstr.str().c_str());
  this->nplots++;
  
  return;
}

// ----------------------------------------------------------------------------
    
void Gnuplot::plot_xy(vector<double> x, vector<double> y, const string &title)
{
  ofstream tmp;
  ostringstream cmdstr;
#ifdef WIN32
  char name[] = "gnuplotiXXXXXX";
#else
  char name[] = "/tmp/gnuplotiXXXXXX";
#endif
    
  // should raise an exception
  if (x.size() != y.size())
    return;
  
  if ((this->to_delete).size() == GP_MAX_TMP_FILES - 1)
  {
    std::stringstream s;
    s << "Maximum number of temporary files reached (" 
      << GP_MAX_TMP_FILES << "): cannot open more files" << endl;
    throw GnuplotException( s.str() );
  }

  //open temporary files for output
  //
  if (MKTEMP_AND_CHECK_FAILED(name))
    throw GnuplotException("Cannot create temporary file: exiting plot");

  tmp.open(name);
  if (tmp.bad())
    throw GnuplotException("Cannot create temorary file: exiting plot");
  
  // Save the temporary filename
  // 
  this->to_delete.push_back(name);
  
  // Write the data to file
  //
  size_t N = std::min(x.size(), y.size());
  for (size_t i = 0; i < N; i++)
    tmp << x[i] << " " << y[i] << endl;
  tmp.flush();    
  tmp.close();
  
  //
  // command to be sent to gnuplot
  //
  if (this->nplots > 0)
    cmdstr << "replot ";
  else cmdstr << "plot ";
  if (title == "")
    cmdstr << "\"" << name << "\" with " << this->pstyle;
  else
    cmdstr << "\"" << name << "\" title \"" << title << "\" with " << this->pstyle;
  
  //
  // Do the actual plot
  //
  this->cmd(cmdstr.str().c_str());
  this->nplots++;
  
  return;
}

// ----------------------------------------------------------------------------

void Gnuplot::init()
{
  if (!this->get_program_path(gnuplot_executable_))
  {
    this->valid = false;
    throw GnuplotException("Can't find gnuplot in your PATH");
  }
  
  this->gnucmd = popen(gnuplot_executable_.c_str(),"w");
  if (!this->gnucmd)
  {
    this->valid = false;
    throw GnuplotException("Couldn't open connection to gnuplot");
  }
  this->nplots = 0;
  this->valid = true;
}

// ============================================================================
