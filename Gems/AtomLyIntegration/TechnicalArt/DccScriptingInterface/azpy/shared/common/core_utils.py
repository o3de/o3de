# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -- This line is 75 characters -------------------------------------------
from __future__ import unicode_literals
# from builtins import str

# A patch to make this module work in Python3 (hopefully still works in Py27)
try:
    unicode = unicode
except NameError:
    # 'unicode' is undefined, must be Python3
    str = str
    unicode = str
    bytes = bytes
    basestring = (str, bytes)
else:
    # 'unicode' exists, must be Python2
    str = str
    unicode = unicode
    bytes = str
    basestring = basestring

# -------------------------------------------------------------------------
'''
Module Documentation:
    DccScriptingInterface\\azpy\\shared\\common\\core_utils.py

    A set of utility functions

    <to do: further document this module>

    To Do:
        ATOM-5859
'''
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# built in's
import os
import sys
import site
import fnmatch
import logging as _logging

# 3rd Party
from pathlib import Path
# from progress.spinner import Spinner # deprecate use (or refactor)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
#  global space
from DccScriptingInterface.azpy.shared.common import _PACKAGENAME
_MODULENAME = f'{_PACKAGENAME}.core_utils'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))

from DccScriptingInterface.azpy.constants import *
# -------------------------------------------------------------------------


# --------------------------------------------------------------------------
def gather_paths_of_type_from_dir(in_path=str('c:\\'),
                                  extension=str('*.py'),
                                  return_path_list=list()):
    '''Walks from in_path and returns list of directories that contain the
    file type matching the extension'''

    # recursive function for finding paths
    dir_contents = os.listdir(in_path)
    complete = False
    while complete != True:
        found = None
        for item in dir_contents:
            # found a dir to search
            if os.path.isdir((in_path + "/" + item)):
                return_path_list = gather_paths_of_type_from_dir((in_path + "/" + item),
                                                                 extension,
                                                                 return_path_list)
            # found a path
            elif os.path.isfile:
                if fnmatch.fnmatch(item, extension):
                    found = True

        if found:
            return_path_list.append(dir_trim_following_slash(to_unix_path(os.path.abspath(in_path))))

        complete = True

    return return_path_list
# --------------------------------------------------------------------------


# ------------------------------------------------------------------------
def dir_trim_following_slash(current_path_str):
    '''removes the trailing slash from a path str'''
    safe_path = current_path_str
    if current_path_str.endswith('/'):
        safe_path = current_path_str[0:-1]
    if current_path_str.endswith('\\'):
        safe_path = current_path_str[0:-1]
    return safe_path
# --------------------------------------------------------------------------


# ------------------------------------------------------------------------
def to_unix_path(current_path_str):
    '''converts path string to use unix slashes'''
    _LOGGER.debug('to_unix_path({0})'.format(current_path_str))
    safe_path = str(current_path_str).replace('\\', '/')
    return safe_path
# --------------------------------------------------------------------------


# --------------------------------------------------------------------------
def we_are_frozen():
    # All of the modules are built-in to the interpreter, e.g., by py2exe
    return hasattr(sys, "frozen")
# --------------------------------------------------------------------------


# --------------------------------------------------------------------------
def module_path():
    encoding = sys.getfilesystemencoding()
    if we_are_frozen():
        return os.path.dirname(unicode(sys.executable, encoding))
    return os.path.dirname(__file__)
# --------------------------------------------------------------------------


# --------------------------------------------------------------------------
def get_stub_check_path(in_path, check_stub='engineroot.txt'):
    '''
    Returns the branch root directory of the dev\'engineroot.txt'
    (... or you can pass it another known stub)

    so we can safely build relative filepaths within that branch.

    If the stub is not found, it returns None
    '''
    path = Path(in_path).absolute()

    while 1:
        test_path = Path(path, check_stub)

        if test_path.is_file():
            return Path(test_path)

        else:
            path, tail = (path.parent, path.name)

            if (len(tail) == 0):
                return None
# --------------------------------------------------------------------------


# -------------------------------------------------------------------------
def reorder_sys_paths(known_sys_paths):
    """Reorders new directories to the front"""
    sys_paths = list(sys.path)
    new_sys_paths = []

    for item in sys_paths:
        item = Path(item)
        if str(item).lower() not in known_sys_paths:
            new_sys_paths.append(item)
        sys_paths.remove(str(item))

    sys.path[:0] = new_sys_paths

    known_sys_paths = site._init_pathinfo()
    return known_sys_paths
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def check_path_exists(in_path):
    """Will bark if path does not exist"""

    check = os.path.exists(in_path)

    if check:
        _LOGGER.info('~ Path EXISTS: {0}\r'.format(in_path))

    else:
        _LOGGER.info('~ Path does not exist: {0}\r'.format(in_path))

    return check
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def path_split_all(in_path):
    '''Splits the in_path to all of it's parts'''
    all_path_parts = []
    while 1:
        parts = os.path.split(path)
        if parts[0] == path:  # sentinel for absolute paths
            all_path_parts.insert(0, parts[0])
            break
        elif parts[1] == path:  # sentinel for relative paths
            all_path_parts.insert(0, parts[1])
            break
        else:
            path = parts[0]
            all_path_parts.insert(0, parts[1])
    return all_path_parts
# -------------------------------------------------------------------------


# --------------------------------------------------------------------------
def synthetic_property(inst, name, value, read_only=False):
    '''
    This is a convenience method for OOP
    synthesizes the creation of property attr with convenience methods:
        x.attrbute          # the @property (and setter)
        x._attribute        # attribute storage (private)
        x.getAttribute()    # retreive attribute
        x.setAttribute()    # set attribute (only created if not 'read only')
        x.delAttribute()    # delete the attribute from object
    '''
    cls = type(inst)
    storage_name = '_{0}'.format(name)
    getter_name = 'get{0}{1}'.format(name[0].capitalize(), name[1:])
    setter_name = 'set{0}{1}'.format(name[0].capitalize(), name[1:])
    deleter_name = 'del{0}{1}'.format(name[0].capitalize(), name[1:])

    setattr(inst, storage_name, value)

    # We always define the getter
    def custom_getter(self):
        return getattr(self, storage_name)

    # Add the Getter
    if not hasattr(inst, getter_name):
        setattr(cls, getter_name, custom_getter)

    # Handle Read Only
    if read_only:
        if not hasattr(inst, name):
            setattr(cls, name, property(fget=getattr(cls, getter_name, None)
                                        or custom_getter,
                                        fdel=getattr(cls, getter_name, None)))
    else:
        # We only define the setter if we aren't read only
        def custom_setter(self, state):
            setattr(self, storage_name, state)
        if not hasattr(inst, setter_name):
            setattr(cls, setter_name, custom_setter)
        member = None
        if hasattr(cls, name):
            # we need to try to update the property fget, fset,
            # fdel incase the class has defined its own custom functions
            member = getattr(cls, name)
            if not isinstance(member, property):
                raise ValueError('Member "{0}" for class "{1}" exists and is not a property.'
                                 ''.format(name, cls.__name__))
        # Regardless if the class has the property or not we still try to set it with
        setattr(cls, name, property(fget=getattr(member, 'fget', None)
                                    or getattr(cls, getter_name, None)
                                    or custom_getter,
                                    fset=getattr(member, 'fset', None)
                                    or getattr(cls, setter_name, None)
                                    or custom_setter,
                                    fdel=getattr(member, 'fdel', None)
                                    or getattr(cls, getter_name, None)))
# --------------------------------------------------------------------------


# --------------------------------------------------------------------------
def find_arg(arg_pos_index=None, arg_tag=None, remove_kwarg=None,
             in_args=None, in_kwargs=None, default_value=None):
    """
    # finds and returns an arg...
    # if a positional index is given arg_pos_index=0, it checks args first
    # if a arg_tag is given, it checks kwargs
    #    If remove_kwarg=True, it will remove the found arg from kwargs
    #    * I actually want/need to do this often
    #
    # a set kwarg will ALWAYS take precident over positional arg!!!
    #
    # return outArg, args, kwargs   <-- get back modified kwargs!
    #
    # proper usage:
    #
    #    found_arg, args, kwargs = find_arg(0, 'name',)
    """
    if arg_pos_index != None:
        if not isinstance(arg_pos_index, int):
            raise TypeError('remove_kwarg: accepts a index integer!\r'
                            'got: {0}'.format(remove_kwarg))

        # positional args ... check the position
        if len(in_args) > 0:
            try:
                found_arg = in_args[arg_pos_index]
            except:
                pass

    # check kwargs ... a set kwarg will ALWAYS take precident over
    # positional arg!!!
    try:
        found_arg
    except:
        found_arg = in_kwargs.get(arg_tag, default_value)  # defaults to None

    if remove_kwarg:
        if arg_tag in in_kwargs:
            del in_kwargs[arg_tag]

    # if we didn't find the arg/kwarg, the defualt return will be None
    return found_arg, in_kwargs
# -------------------------------------------------------------------------


# --------------------------------------------------------------------------
def set_synth_arg_kwarg(inst, arg_pos_index, arg_tag, in_args, in_kwargs,
                        remove_kwarg=True, default_value=None, set_anyway=True):
    """
    Uses find_arg and sets a property on a object.
    Special args:
        set_anyway  <-- if the object has the property already, set it
    """

    # find the argument, or set to default value
    found_arg, in_kwargs = find_arg(arg_pos_index, arg_tag, remove_kwarg,
                                    in_args, in_kwargs,
                                    default_value)

    # make sure the object doesn't arealdy have this property
    try:
        hasattr(inst, arg_tag)  # <-- check if property exists
        if set_anyway:
            try:
                setattr(inst, arg_tag, found_arg)  # <-- try to set
            except Exception as e:
                raise e
    except:
        try:
            found_arg = synthetic_property(inst, arg_tag, found_arg)
        except Exception as e:
            raise e

    return found_arg, in_kwargs
# --------------------------------------------------------------------------


# -------------------------------------------------------------------------
def walk_up_dir(in_path, dir_tag='foo'):
    '''
    Mimic something like os.walk, but walks up the directory tree
    Walks Up from the in_path looking for a dir with the name dir_tag

        in_path: the path to start in
        dir_tag: the name of diretory above us we are looking for

    returns None if the directory named dir_tag is not found
    '''
    path = Path(__file__).absolute()
    while 1:
        # hmmm, will this break on unix paths?
        # what about case sensitivity?
        dir_base_name = path.norm_case().name()
        if (dir_base_name == dir_tag):
            break
        path, tail = (path.parent(), path.name())
        if (len(tail) == 0):
            return None

    return path
# --------------------------------------------------------------------------


# --------------------------------------------------------------------------
def return_stub(stub):
    '''Take a file name (stub) and returns the directory of the file (stub)'''
    # to do: refactor to pathlib.Path
    from unipath import Path

    dir_last_file = None
    if dir_last_file is None:
        path = Path(__file__).absolute()
        while 1:
            path, tail = (path.parent, path.name)
            newpath = Path(path, stub)
            if newpath.isfile():
                break
            if (len(tail) == 0):
                path = ""
                _LOGGER.debug('~ Debug Message:  I was not able to find the '
                              'path to that file (stub) in a walk-up from currnet path')
                break
        dir_last_file = path

    return dir_last_file
# --------------------------------------------------------------------------


# --------------------------------------------------------------------------
# direct call for testing methods functions
if __name__ == "__main__":
    '''To Do: Document'''
    # constants for shared use.
    DCCSI_GDEBUG = True

    # happy _LOGGER.info
    _LOGGER.info("# {0} #".format('-' * 72))
    _LOGGER.info('~ constants.py ... Running script as __main__')
    _LOGGER.info("# {0} #\r".format('-' * 72))

    cwd = Path(os.getcwd())

    # This grabs pythons known paths
    _KNOWN_SITEDIR_PATHS = list(sys.path)  # this appears to give me a somehow malformed syspath?
    _KNOWN_SITEDIR_PATHS = site._init_pathinfo()

    #  this is just a debug developer convenience _LOGGER.info (for testing acess)
    if DCCSI_GDEBUG:
        import pkgutil
        _LOGGER.info('Current working dir: {0}'.format(cwd))
        search_path = ['.']  # set to None to see all modules importable from sys.path
        all_modules = [x[1] for x in pkgutil.iter_modules(path=search_path)]
        _LOGGER.info('All Available Modules in working dir: {0}\r'.format(all_modules))

    # test toUnixPath
    #  assumes the current working directory (cwd) is <ly>\\dev\\Gems\\DccScriptingInterface
    test_path = Path(cwd, 'LyPy', 'si_shared', 'common', 'core_utils.py')
    safeTest = to_unix_path(test_path)
    _LOGGER.info("Unix format: '{0}'".format(safeTest))
    _LOGGER.info('')

    # test dirTrimFollowingSlash
    short_path = Path(cwd)
    _LOGGER.info("Original: '{0}'".format(short_path))
    short_path = to_unix_path(short_path)
    _LOGGER.info("Unix: '{0}'".format(short_path))
    trimTest = dir_trim_following_slash(short_path)
    _LOGGER.info("Trimmed: '{0}'".format(trimTest))
    _LOGGER.info('')

    # test gather_paths_of_type_from_dir
    extTest = '*.py'
    fileList = gather_paths_of_type_from_dir(in_path=os.getcwd(), extension=extTest)
    _LOGGER.info('Found {0}: {1}'.format(extTest, len(fileList)))
    _LOGGER.info('')

    # test weAreFrozen
    # none

    # test modulePath
    modulePathTest = module_path()
    _LOGGER.info("This Module: '{0}'".format(modulePathTest))
    modulePathTest = to_unix_path(modulePathTest)
    _LOGGER.info("This module unix: '{0}'".format(modulePathTest))
    _LOGGER.info('')

    # test checkstub_getpath
    stubTest = get_stub_check_path(__file__)
    _LOGGER.info("Stub Path: '{0}'".format(stubTest))

    # reorderSysPaths test
    pkgTestPath = Path(cwd, 'LyPy', 'si_shared', 'packagetest')
    site.addsitedir(pkgTestPath)
    anotherTestPath = Path(cwd, 'LyPy', 'si_shared', 'dev')
    site.addsitedir(anotherTestPath)
    #  pass in the previous list we retreived earlier
    _KNOWN_SITEDIR_PATHS = reorder_sys_paths(_KNOWN_SITEDIR_PATHS)  # I think this is broken,
    # I get back this as one of the paths:
    #    G:\depot\gallowj_PC1_lrgWrlds\dev\Gems\DccScriptingInterface\Shared\Python\LyPyCommon\PYTHONPATH

    _LOGGER.info('done')
    pass

    # checkPathExists test

    # pathSplitAll test

    # walkUp test

    # test synthesize
    # to do: write test

    # test findArg
    # to do: write test

    # test setSynthArgKwarg
    # to do: write test
