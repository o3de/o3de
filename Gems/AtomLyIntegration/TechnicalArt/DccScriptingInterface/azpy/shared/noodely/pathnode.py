"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------

# this has to be at the beginning
from __future__ import division

# -------------------------------------------------------------------------
# -------------------------------------------------------------------------
# pathnode.py
# simple path objecy based Node Class, for tool creation.
# version: 0.1
# author: Gallowj
# -------------------------------------------------------------------------
# -------------------------------------------------------------------------
"""
Module docstring:
A simple path objecy based Node Class, for creating path hierarchies.
"""
__author__ = 'HogJonny'

# -------------------------------------------------------------------------
# built-ins
import os
import copy
import subprocess
import traceback
import string
import logging
from unipath import Path, AbstractPath

# local ly imports
from azpy.shared.noodely.helpers import istext
from azpy.shared.noodely.find_arg import find_arg
from azpy.shared.noodely.synth import synthesize
from azpy.shared.noodely.node import Node

import azpy
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE

# -------------------------------------------------------------------------
#  global space
# To Do: update to dynaconf dynamic env and settings?
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)

_MODULENAME = 'azpy.shared.noodely.pathnode'

_log_level = int(20)
if _DCCSI_GDEBUG:
    _log_level = int(10)
_LOGGER = azpy.initialize_logger(_MODULENAME,
                                 log_to_file=False,
                                 default_log_level=_log_level)
_LOGGER.debug('Starting:: {}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# Use unicode strings
_base = str               # Python 3 str (=unicode), or Python 2 bytes.
if os.path.supports_unicode_filenames:
    try:
        _base = unicode   # Python 2 unicode.
    except NameError:
        pass
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
class PathNode(Node):
    """doc string"""

    # share the debug state
    _DEBUG = _DCCSI_GDEBUG

    # class header
    _message_header = 'noodly, PathNode(): Message'

    # App Launcher paths...
    try:
        _maya_exe_path = Path(os.environ['MAYAPY'])
    except:
        _maya_exe_path = Path(r"C:\Program Files\Autodesk\Maya2019\bin\maya.exe")
    try:
        _notepad_exe_path = Path(os.environ['DEFAULT_TXT_EXE'])
    except:
        _notepad_exe_path = Path(r"C:\Program Files (x86)\Notepad++\notepad++.exe")

    # --BASE-METHODS-------------------------------------------------------
    def __new__(cls, path="", root_path=None, *args, **kwargs):
        '''docstring'''
#         if not isinstance(path, str) and not isinstance(path, Path):
#             raise TypeError("{0}, {1}: Accepts paths as str or Path() types!\r"
#                             "Input data is:{2}\r"
#                             "".format('noodly, PathNode',
#                                       'PathNode(filename)', type(path)))
#
        self = super(PathNode, cls).__new__(cls)
        return self

    # --constructor--------------------------------------------------------
    def __init__(self, path="", root_path=None, parent_is_root=None,
                 name_is_path=None, *args, **kwargs):

        self._logger = Node._LOGGER

        self._node_type = self.__class__.__name__

        # a dict to store properties/attrs
        # in the event an object is re-built / re-init
        # it is important to store anything here that needs retention
        self._kwargs_dict = {}

        # -- secret keyword -----------------------------------------------
        self._temp_node = False
        temp_node, kwargs = find_arg(arg_pos_index=None, arg_tag='temp_node',
                                     remove_kwarg=True, in_args=args,
                                     in_kwargs=kwargs)  # <-- kwarg only

        self._temp_node = temp_node
        if self._temp_node:
            self.k_wargs_dict['temp_node'] = self._temp_node

        # -- Node class args/kwargs ---------------------------------------
        node_name, kwargs = find_arg(arg_pos_index=2, arg_tag='node_name',
                                     remove_kwarg=True, in_args=args,
                                     in_kwargs=kwargs)  # <-- third arg, kwarg

        parent_node, kwargs = find_arg(arg_pos_index=3, arg_tag='parent_node',
                                       remove_kwarg=True, in_args=args,
                                       in_kwargs=kwargs)  # <-- fourth arg, kwarg

        self._root_path = root_path

        self._parent_is_root = parent_is_root
        if self._parent_is_root != None:
            self._kwargs_dict['parent_is_root'] = self.parent_is_root

        if parent_is_root:  # <-- do it
            self._root_path = parent_node

        # make sure the path is a Path
        self._path = path
        if not isinstance(self._path, Path):
            try:
                self._path = Path(path)
            except:
                self._path = Path()  # empty path object fallback

        self._name_is_path = name_is_path
        if self._name_is_path:
            self._kwargs_dict['name_is_path'] = self._name_is_path

        # this might only work if the file actually exists
        self._node_name = node_name
        if self._name_is_path:
            if self._path.name != None or self._path.name != '':
                self._node_name = str(self._path.name)

        # Path.__init__(self)
        super(PathNode, self).__init__(self._node_name, parent_node,
                                       temp_node=temp_node,
                                       *args, **kwargs)

    # -- properties -------------------------------------------------------

    @property
    def path(self):
        return self._path

    @path.setter
    def path(self, path):
        self._path = path
        return self._path

    @path.getter
    def path(self):
        return self._path

    @property
    def root_path(self):
        return self._root_path

    @root_path.setter
    def root_path(self, root_path):
        self._root_path = root_path
        return self._root_path

    @root_path.getter
    def root_path(self):
        return self._root_path

    @property
    def parent_is_root(self):
        return self._parent_is_root

    @parent_is_root.setter
    def parent_is_root(self, parent_is_root):
        self._parent_is_root = parent_is_root
        return self._parent_is_root

    @parent_is_root.getter
    def parent_is_root(self):
        return self._parent_is_root

    @property
    def name_is_path(self):
        return self._name_is_path

#     @name_is_path.setter
#     def name_is_path(self, name_is_path):
#         self._name_is_path = name_is_path
#         return self._name_is_path

    @name_is_path.getter
    def name_is_path(self):
        return self._name_is_path

    # --method-------------------------------------------------------------
    def set_file_path(self, path):
        if not isinstance(path, Path):
            try:
                path = Path(path)
            except:
                raise TypeError("must be Path compatible")

        # retreive a copy of the old _kwargs dict
        _kwargs_dict_copy = copy.copy(self._kwargs_dict)
        _name_is_uni_hashid = copy.copy(self.name_is_uni_hashid)

        # create a new me (self), with new value
        # attempt to keep existing attrs/settings
        self = PathNode(path=path,
                        root_path=self.root_path,
                        parent_is_root=self.parent_is_root,
                        name_is_path=self.name_is_path,
                        temp_node=self.temp_node,
                        node_name=self.node_name,
                        parent_node=self.parent_node,
                        name_is_uni_hashid=self.name_is_uni_hashid)

        # now we need to restore any custom properties on the replacement object
        for key, value in _kwargs_dict_copy.items():
            self._kwargs_dict[key] = value
            try:
                synthesize(self, '{0}'.format(key), value)
            except:
                code = compile('self._{0} = {1}'.format(key, value), 'synthProp', 'exec')
                if Node._DEBUG:
                    self.logger.error('can not set: self._{0} = {1}'.format(key, value))

        # replace myself in the class nodeDict, based on my unihashid
        self.cls_node_dict[self.uni_hashid] = self

        # return the new version of myself
        return self.cls_node_dict[self.uni_hashid]
    # ---------------------------------------------------------------------

    # --method-------------------------------------------------------------
    def start_file(self, filepath=None):
        '''opens the file in the prefered os editor for the filetype'''
        if filepath == None:
            filepath = self.path

        if not isinstance(filepath, Path):  # <-any subclass of Path works?
            filepath = Path(filepath)

        self.logger.debug('starting file: {0}'.format(filepath))
        try:
            os.startfile(filepath)
        except IOError as e:
            self.logger.error(e)

        return filepath
    # ---------------------------------------------------------------------

    # --method-------------------------------------------------------------
    def explore_file(self, filepath=None):
        if filepath == None:
            filepath = self.path

        if not isinstance(filepath, Path):
            filepath = Path(filepath)

        self.logger.debug('exploring file: {0}'.format(filepath))
        if filepath.exists():
            try:
                subprocess.Popen(r'explorer /select,"{0}"'.format(filepath))
            except IOError as e:
                self.logger.error(e)
        else:
            self.logger.error('file does not exist: {0}'.format(filepath))

        return filepath
    # ---------------------------------------------------------------------

    # --method-------------------------------------------------------------
    def hierarchy(self, tabLevel=-1):

        output = ''
        if isinstance(self, RootNode):
            if gDebug:
                func = inspect.currentframe().f_back.f_code
                output += ('{0}Called from:\n'
                           '{0}{1}\n'.format('\t' * (tabLevel + 1), func))

        tabLevel += 1

        for i in range(tabLevel):
            output += '\t'

        output += ('{tab}/------ nodeName:: "{0}"\n'
                   '{1}       |typeInfo:: {2}\n'
                   '{1}       |_uniHashid:: "{3}"\r'
                   '{1}       |path:: "{4}"\n'
                   '{1}       |get_root():: "{5}"\n'
                   '{1}       |getPathFromRoot():: "{6}"\n'
                   ''.format(self.getNodeName(),
                             '\t' * tabLevel,
                             self.get_typeInfo(),
                             self.get_uniHashid(),
                             self,
                             self.get_root(),
                             self.getPathFromRoot(),
                             tab=tabLevel))

        for child in self._children:
            output += child.hierarchy(tabLevel)

        tabLevel -= 1

        return output
    # ---------------------------------------------------------------------

# --Class End--------------------------------------------------------------


###########################################################################
# tests(), code block for testing module
# -------------------------------------------------------------------------
def tests():
    from node import Node
    default_node = Node()  # result: Node(node_name='PRIME')
    print(default_node)

    first_child = PathNode(path=None, node_name='first_child', parent_node=default_node)
    print(first_child)
    # result: PathNode(temp_node=True, parent_node=Node(node_name='PRIME')).siblingNodeFromHashid('WNPZoKBVpXV16QLz')
    # first_child.nodeType
    # first_child.parent_node
    # first_child.node_name

    try:
        # PathNode requires a arg 'path' input (should be a path str)
        fubar_path_node = PathNode()  # <-- this should fail
        print (fubar_path_node)
    except Exception as err:
        print ('\r{0}'.format(err))
        print (traceback.format_exc())

    foo = PathNode(r'\foo\fooey\kablooey', node_name='foo',
                   parent_node=default_node)
    print(foo)

    testes = Path(r'/foo/fooey/kablooey')

    print(foo.path.exists())
    print(foo.path.parent)
    print(foo.path.norm_case())
    print(foo.path.absolute())

    fooey = PathNode(None, parent_node=foo)
    print(fooey)

    kablooey = PathNode(r'\foo\fooey\kablooey',
                        parent_node=default_node,
                        name_is_path=True)
    print(kablooey)

    kablooey = kablooey.set_file_path(r'c:\mytemp\fubar.txt')
    print(kablooey)
    kablooey.start_file()
    kablooey.explore_file()

    return
# - END, tests() ----------------------------------------------------------


def main():
    pass
    return
# - END, main() -----------------------------------------------------------


###########################################################################
# --call block-------------------------------------------------------------
if __name__ == "__main__":
    print ("# ----------------------------------------------------------------------- #")
    print ('~ noodly.PathNode ... Running script as __main__')
    print ("# ----------------------------------------------------------------------- #\r")

    # run simple tests
    tests()
