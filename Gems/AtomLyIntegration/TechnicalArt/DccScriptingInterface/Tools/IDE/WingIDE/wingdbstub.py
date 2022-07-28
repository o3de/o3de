#########################################################################
""" wingdbstub.py -- Start debugging Python programs from outside of Wing

Copyright (c) 1999-2018, Archaeopteryx Software, Inc.  All rights reserved.

Written by Stephan R.A. Deibel and John P. Ehresman

Usage:
-----

This file is used to initiate debug in Python code without launching
that code from Wing.  To use it, edit the configuration values below,
start Wing and configure it to listen for outside debug connections,
and then add the following line to your code:

  import wingdbstub

Debugging will start immediately after this import statement is reached.

For details, see Debugging Externally Launched Code in the manual.

"""
#########################################################################

import sys
orig_sys_modules = list(sys.modules.keys())
orig_sys_path = list(sys.path)
orig_meta_path = list(sys.meta_path)

import os
if sys.version_info >= (3, 7):
  import importlib
else:
  import imp

#------------------------------------------------------------------------
# Required configuration values (some installers set this automatically)

# This should be the full path to your Wing installation.  On macOS, use 
# the full path of the Wing application bundle, for example 
# /Applications/WingPro.app.  When set to None, the environment variable 
# WINGHOME is used instead.  
WINGHOME = r"C:\Program Files (x86)\Wing Pro 8"

#------------------------------------------------------------------------
# Optional configuration values:  The named environment variables, if set, 
# will override these settings.

# Set this to 1 to disable all debugging; 0 to enable debugging
# (WINGDB_DISABLED environment variable)
kWingDebugDisabled = 0

# Host:port of the IDE within which to debug: As configured in the IDE
# with the Server Port preference
# (WINGDB_HOSTPORT environment variable)
kWingHostPort = 'localhost:50005'

# Port on which to listen for connection requests, so that the
# IDE can attach or reattach to the debug process after it has started.
# Set this to '-1' to disable listening for connection requests.
# This is only used when the debug process is not attached to
# an IDE or the IDE has dropped its connection. The configured
# port can optionally be added to the IDE's Common Attach Hosts
# preference. Note that a random port is used instead if the given 
# port is already in use.
# (WINGDB_ATTACHPORT environment variable)
kAttachPort = '50015'

# Set this to a filename to log verbose information about the debugger
# internals to a file.  If the file does not exist, it will be created
# as long as its enclosing directory exists and is writeable.  Use 
# "<stderr>" or "<stdout>" to write to stderr or stdout.  Note that 
# "<stderr>" may cause problems on Windows if the debug process is not 
# running in a console.
# (WINGDB_LOGFILE environment variable)
kLogFile = None

# Produce a tremendous amount of logging from the debugger internals.
# Do not set this unless instructed to do so by Wingware support.  It
# will slow the debugger to a crawl.
# (WINGDB_LOGVERYVERBOSE environment variable)
kLogVeryVerbose = 0

# Set this to 1 when debugging embedded scripts in an environment that
# creates and reuses a Python instance across multiple script invocations.  
# It turns off automatic detection of program quit so that the debug session
# can span multiple script invocations.  When this is turned on, you may
# need to call ProgramQuit() on the debugger object to shut down the
# debugger cleanly when your application exits or discards the Python
# instance.  If multiple Python instances are created in the same run,
# only the first one will be able to debug unless it terminates debug
# and the environment variable WINGDB_ACTIVE is unset before importing
# this module in the second or later Python instance.  See the Wing
# manual for details.
# (WINGDB_EMBEDDED environment variable)
kEmbedded = 1

# Security Token configuration

# Wing uses a security token to control access from the debug process.
# In cases where the debugger is running as the same user and on the same
# host as the IDE, no further configuration is needed because the token
# stored in wingdebugpw in the Settings Directory shown in Wing's About
# box is used by both ends of the connection.  In other cases, Wing will
# wrote a new security token to the Settings Directory for the user running
# the debug process.  An unknown security token will be rejected, but Wing
# will offer to accept future connections using an that token.  To avoid
# this, copy the file wingdebugpw from the Settings Directory shown in Wing's
# About box to the same directory as this copy of wingdbstub.py, or set
# kSecurityToken below to the contents of that file.  This is also needed
# when Wing cannot write to the Settings Directory, or the contents of that
# directory are not persisted between debug runs, as is the case with some
# types of containers.

# The security token to use, overriding any other value.  If None, the token is 
# instead read from the file configured with kFWFilePath and kPWFileName below.  
# Disk permissions of any copy of wingdbstub.py with kSecurityToken set should 
# be adjusted to avoid unauthorized access by other users.
# (WINGDB_SECURITYTOKEN environment variable)
kSecurityToken = None

# Path to search for the debugger security token and the name of the file
# to use.  These values are for advanced use cases and rarely need to be changed.  
# In kPWFilePath, the value '$<winguserprofile>' is replaced by the User Settings 
# Directory for the user that is running the debug process.
# (WINGDB_PWFILEPATH and WINGDB_PWFILENAME environment variables)
kPWFilePath = [os.path.dirname(__file__), '$<winguserprofile>']
kPWFileName = 'wingdebugpw'

# Whether to exit when the debugger fails to run or to connect with the IDE
# By default, execution continues without debug or without connecting to
# the IDE.
# (WINGDB_EXITONFAILURE environment variable)
kExitOnFailure = 0

#------------------------------------------------------------------------
# Find Wing debugger installation location

# Check environment:  Must have WINGHOME defined if still == None
if WINGHOME == None:
  if 'WINGHOME' in os.environ:
    WINGHOME=os.environ['WINGHOME']
  else:
    msg = '\n'.join(["*******************************************************************", 
                     "Error: Could not find Wing installation!  You must set WINGHOME or edit", 
                     "wingdbstub.py where indicated to point it to the location where", 
                     "Wing is installed.\n"])
    sys.stderr.write(msg)
    raise ImportError(msg)
  
WINGHOME = os.path.expanduser(WINGHOME)

def NP_FindActualWingHome(winghome):
  """ Find the actual directory to use for winghome.  Needed on macOS
  where the .app directory is the preferred dir to use for WINGHOME and
  .app/Contents/MacOS is accepted for backward compatibility. """
  
  if sys.platform != 'darwin':
    return winghome
  
  app_dir = None
  if os.path.isdir(winghome):
    if winghome.endswith('/'):
      wo_slash = winghome[:-1]
    else:
      wo_slash = winghome
      
    if wo_slash.endswith('.app'):
      app_dir = wo_slash
    elif wo_slash.endswith('.app/Contents/MacOS'):
      app_dir = wo_slash[:-len('/Contents/MacOS')]
    
  if app_dir and os.path.isdir(os.path.join(app_dir, 'Contents', 'Resources')):
    return os.path.join(app_dir, 'Contents', 'Resources')
  
  return winghome
  
WINGHOME = NP_FindActualWingHome(WINGHOME)
os.environ['WINGHOME'] = WINGHOME

#-----------------------------------------------------------------------
def NP_LoadModuleFromBootstrap(winghome, modname):
  """Load a module from the installation bootstrap directory.  Assumes that
  imports don't change sys.path.  The modules are unloaded from sys.modules
  so they can be loaded again later from an update."""
  
  # Limited to simple module loads
  assert '.' not in modname
  
  orig_sys_path = sys.path[:]
  orig_modules = set(sys.modules)
  
  dirname = winghome + '/bootstrap'
  sys.path.insert(0, dirname)
  
  code = 'import %s' % modname
  exec(code)
  
  new_modules = set(sys.modules)
  new_modules.difference_update(orig_modules)
  for mod in new_modules:
    del sys.modules[mod]
    
  sys.path = orig_sys_path
  
  return locals()[modname]

#------------------------------------------------------------------------
# Set debugger if it hasn't been set -- this is to handle module reloading
# In the reload case, the debugger variable will be set to something
try:
  debugger
except NameError:
  debugger = None
  
#-----------------------------------------------------------------------
# Unset WINGDB_ACTIVE env if it was inherited from another process
# XXX Would be better to be able to call getpid() on dbgtracer but can't access it yet
if 'WINGDB_ACTIVE' in os.environ and os.environ['WINGDB_ACTIVE'] != str(os.getpid()):
  del os.environ['WINGDB_ACTIVE']

#-----------------------------------------------------------------------
# Start debugging if not disabled and this module has never been imported
# before
if (not kWingDebugDisabled and debugger is None and
    'WINGDB_DISABLED' not in os.environ and 
    'WINGDB_ACTIVE' not in os.environ):

  exit_on_fail = 0
  
  try:
    # Obtain exit if fails value
    exit_on_fail = os.environ.get('WINGDB_EXITONFAILURE', kExitOnFailure)
    
    # Obtain configuration for log file to use, if any
    logfile = os.environ.get('WINGDB_LOGFILE', kLogFile)
    if logfile == '-' or logfile == None or len(logfile.strip()) == 0:
      logfile = None

    very_verbose_log = os.environ.get('WINGDB_LOGVERYVERBOSE', kLogVeryVerbose)
    if type(very_verbose_log) == type('') and very_verbose_log.strip() == '':
      very_verbose_log = 0
      
    # Determine remote host/port where the IDE is running
    hostport = os.environ.get('WINGDB_HOSTPORT', kWingHostPort)
    colonpos = hostport.find(':')
    host = hostport[:colonpos]
    port = int(hostport[colonpos+1:])
  
    # Determine port to listen on locally for attach requests
    attachport = int(os.environ.get('WINGDB_ATTACHPORT', kAttachPort))
  
    # Check if running embedded script
    embedded = int(os.environ.get('WINGDB_EMBEDDED', kEmbedded))
  
    # Obtain security token
    security_token = os.environ.get('WINGDB_SECURITYTOKEN', kSecurityToken)
    
    # Obtain debug password file search path
    if 'WINGDB_PWFILEPATH' in os.environ:
      pwfile_path = os.environ['WINGDB_PWFILEPATH'].split(os.pathsep)
    else:
      pwfile_path = kPWFilePath
    
    # Obtain debug password file name
    if 'WINGDB_PWFILENAME' in os.environ:
      pwfile_name = os.environ['WINGDB_PWFILENAME']
    else:
      pwfile_name = kPWFileName
    
    # Set up temporary log for errors from merge importer Setup
    class CTmpLog:
      def __init__(self):
        self.fErrors = []
      def write(self, s):
        self.fErrors.append(s)
    tmp_log = CTmpLog()
    
    # Set up the meta importer; everything after this point can be updated
    bootstrap_utils = NP_LoadModuleFromBootstrap(WINGHOME, 'bootstrap_utils')
    winghome, user_settings = bootstrap_utils.NP_SetupWingHomeModule(WINGHOME)
    meta = bootstrap_utils.NP_CreateMetaImporter(WINGHOME, user_settings, 'dbg',
                                                 tmp_log)
    import _winghome
    _winghome.kWingHome = winghome
    _winghome.kUserSettings = user_settings
    
    # Find the netserver module and create an error stream
    from debug.tserver import startdebug
    netserver = startdebug.FindNetServerModule(WINGHOME, user_settings)
    err = startdebug.CreateErrStream(netserver, logfile, very_verbose_log)
    
    # Write any temporary log entries
    for s in tmp_log.fErrors:
      err.write(s)
      
    # Create debug server
    debugger = netserver.CNetworkServer(host, port, attachport, err,
                                        security_token=security_token, 
                                        pwfile_path=pwfile_path,
                                        pwfile_name=pwfile_name,
                                        autoquit=not embedded)
    
    # Restore module and path environment
    from debug.tserver import startdebug
    startdebug.RestoreEnvironment(orig_sys_modules, orig_sys_path, orig_meta_path)
    
    # Start debugging
    debugger.StartDebug(stophere=0)
    os.environ['WINGDB_ACTIVE'] = str(os.getpid())
    if debugger.ChannelClosed():
      raise ValueError('Not connected')
    
  except:
    if exit_on_fail:
      raise
    else:
      pass

#-----------------------------------------------------------------------
def Ensure(require_connection=1, require_debugger=1):
  """ Ensure the debugger is started and attempt to connect to the IDE if
  not already connected.  Will raise a ValueError if:
  
  * the require_connection arg is true and the debugger is unable to connect
  * the require_debugger arg is true and the debugger cannot be loaded
  
  If SuspendDebug() has been called through the low-level API, calling
  Ensure() resets the suspend count to zero and additional calls to
  ResumeDebug() will be ignored until SuspendDebug() is called again.
  
  Note that a change to the host & port to connect to will only
  be use if a new connection is made.
  
  """
  
  if debugger is None:
    if require_debugger:
      raise ValueError("No debugger")
    return

  hostport = os.environ.get('WINGDB_HOSTPORT', kWingHostPort)
  colonpos = hostport.find(':')
  host = hostport[:colonpos]
  port = int(hostport[colonpos+1:])
  
  resumed = debugger.ResumeDebug()
  while resumed > 0:
    resumed = debugger.ResumeDebug()
  
  debugger.SetClientAddress((host, port))  
  
  if not debugger.DebugActive():
    debugger.StartDebug()
  elif debugger.ChannelClosed():
    debugger.ConnectToClient()
    
  if require_connection and debugger.ChannelClosed():
    raise ValueError('Not connected')
    