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
# node.py
# simple base Node Class, useful in tool creation.
# version: 0.1
# author: Gallowj
# -------------------------------------------------------------------------
# -------------------------------------------------------------------------
"""
Module docstring:
A Simple Node Base Class Module, for creating basic nodes within a hierarchy.
"""
__author__ = 'HogJonny'

# built-ins
import os
import copy
import traceback
import string
import logging

# using hashids to generate unique name identifiers for nodes
import hashids
import cachetools
from sched import scheduler

# local imports
from azpy.shared.noodely.helpers import display_cached_value
from azpy.shared.noodely.find_arg import find_arg
from azpy.shared.noodely.synth import synthesize

import azpy
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE

#  global space
# To Do: update to dynaconf dynamic env and settings?
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)

_MODULENAME = 'azpy.shared.noodely.node'

_log_level = int(20)
if _DCCSI_GDEBUG:
    _log_level = int(10)
_LOGGER = azpy.initialize_logger(_MODULENAME,
                                 log_to_file=False,
                                 default_log_level=_log_level)
_LOGGER.debug('Starting:: {}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# quick test code (remove later)
from hashids import Hashids
hashids = Hashids(min_length=16, salt='DCCsi')
if _DCCSI_GDEBUG:
    print (hashids.encrypt(193487))  # test hash
# -------------------------------------------------------------------------

# -------------------------------------------------------------------------
#  set up logger
_G_LOGGER = logging.getLogger(__name__)
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

###########################################################################
# HELPER method functions
# -------------------------------------------------------------------------
def return_node_from_hashid(hashid):
    if not isinstance(hashid, str):
        raise TypeError("{0},{1}: Accepts hashids as str types!\r"
                        "Input hashid:{2}\r"
                        "".format('noodly',
                                  'return_node_from_hashid(hashid)',
                                  type(hashid)))

    temp_node = Node(temp_node=True).get_sibling_node_from_hashid('{0}'.format(hashid))

    return temp_node
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
class Node(object):
    """Class constructor: makes a node."""

    # share the debug state
    _DEBUG = _DCCSI_GDEBUG

    # logger
    _LOGGER = _G_LOGGER

    # class header
    message_header = 'noodly, Node(): Message'

    # class variable
    _cls_node_count = 0
    _cls_node_list = []
    _cls_node_dict = {}

    # --BASE-METHODS-------------------------------------------------------
    # --constructor-
    def __init__(self, node_name=None, parent_node=None, *args, **kwargs):

        self._logger = Node._LOGGER

        self._node_type = self.__class__.__name__

        # a dict to store properties/attrs
        # in the event an object is re-built / re-init
        # it is important to store anything here that needs retention
        self._kwargs_dict = {}
        self._children = []

        #  private local access to the cls_node_list
        self._cls_node_list = Node._cls_node_list

        # -- secret keyword -----------------------------------------------
        self._temp_node = False
        temp_node, kwargs = find_arg(arg_pos_index=None, arg_tag='temp_node',
                                     remove_kwarg=True, in_args=args,
                                     in_kwargs=kwargs)  # <-- kwarg only
        self._temp_node = temp_node
        if self._temp_node:
            self._kwargs_dict['temp_node'] = self._temp_node
        # -----------------------------------------------------------------

        # -- store message header -----------------------------------------
        # setup the .message_header <-- kwarg only
        message_header, kwargs = find_arg(arg_pos_index=None, arg_tag='message_header',
                                          remove_kwarg=True, in_args=args, in_kwargs=kwargs,
                                          default_value=('{0}(), Message'
                                                        .format(self._node_type)))
        self._message_header = message_header
        # -----------------------------------------------------------------

        # -- hashid -------------------------------------------------------
        self._name_is_uni_hashid = False
        self._node_class_index = len(Node._cls_node_list)
        if Node._DEBUG:
            print ('__init__.node_class_index: {0}'.format(self._node_class_index))
        self._uni_hashid = hashids.encrypt(self._node_class_index)
        if Node._DEBUG:
            print ('__init__.uni_hashid: {0}'.format(self._uni_hashid))

        # update class dict
        if not self._temp_node:
            Node._cls_node_dict[self._uni_hashid] = self
        # -----------------------------------------------------------------

        # -- store the node name ------------------------------------------
        self._node_name = node_name
        if (self._node_class_index == 0 and self._node_name == None):
            self._node_name = 'PRIME'
        elif (self._node_class_index > 0 and self._node_name == None):
            # set a default node_name if none, based on the unihashid
            if not self._name_is_uni_hashid:
                self._node_name = self._uni_hashid
                self._name_is_uni_hashid = True
        if Node._DEBUG:
            print ('__init__.node_name: {0}'.format(self._node_name))
        # -----------------------------------------------------------------

        # -- node parent_node --------------------------------------------------
        # set up the parent_node property
        self._parent_node = parent_node

        if self._parent_node != None:
            # add this node, to the parent_nodes list of children
            try:
                self._parent_node.add_child(self)
            except:
                pass  # <-- parent_node object passed is NOT a noodly.node?

        # Update class variables
        Node.cls_node_count_up(self)
        Node.cls_node_list_append(self)
        # -----------------------------------------------------------------

        # -----------------------------------------------------------------
        # arbitrary argument properties
        # check postions *args and **kwargs
        # any kwargs left will be used to synthesize a property
        try:
            # checking import due to the way the code is structured
            # the code was passing even if module was not imported
            synthesize
            synthExists = True
        except Exception as e:
            print(e)
            raise e

        for key, value in kwargs.items():
            self._kwargs_dict[key] = value
            try:
                synthesize(self, key, value)
            except Exception as e:  # <-- maybe it can't synthesize?
                # in which case fall back to setting the property
                print(e)
                code = compile(r'self._{0}={1}'.format(key, value), 'synthProp', 'exec')
                pass
            if Node._DEBUG:
                print("{0}:{1}".format(key, value))
        # -----------------------------------------------------------------

        # if temp node, adjust the class counter
        if temp_node:
            self.cls_node_count_down()
            self.cls_node_list_remove()

    # -- properties ------------------------------------------------------------
    @property
    def logger(self):
        return self._logger

    @logger.setter
    def logger(self, logger):
        self._logger = logger
        return self._logger

    @logger.getter
    def logger(self):
        return self._logger

    @property
    def kwargs_dict(self):
        return self._kwargs_dict

    @kwargs_dict.getter
    def kwargs_dict(self):
        return self._kwargs_dict

    @property
    def message_header(self):
        return self._message_header

    @message_header.setter
    def message_header(self, message_header):
        self._message_header = message_header
        return self._message_header

    @property
    def node_type(self):
        return self._node_type

    @node_type.setter
    def node_type(self, node_type):
        self._node_type = node_type
        return self._node_type

    @node_type.getter
    def node_type(self):
        return self._node_type

    @property
    def temp_node(self):
        return self._temp_node

    @temp_node.setter
    def temp_node(self, temp_node):
        self._temp_node = temp_node
        return self._temp_node

    @temp_node.getter
    def temp_node(self):
        return self._temp_node

    @property
    def node_name(self):
        return self._node_name

    @node_name.setter
    def node_name(self, nameStr):
        if nameStr != None:
            if not isinstance(nameStr, str):
                raise TypeError("{0}, {1}: Accepts str types!"
                                "".format(self.__class__.__name__,
                                          self._node_name.__name__))
            try:
                self._node_name = nameStr
            except:
                synthesize(self, '_node_name', None)

        return self._node_name

    @node_name.getter
    def node_name(self):
        return self._node_name

    @property
    def name_is_uni_hashid(self):
        return self._name_is_uni_hashid

    @node_type.setter
    def name_is_uni_hashid(self, value):
        self._name_is_uni_hashid = value
        return self._name_is_uni_hashid

    @node_type.getter
    def name_is_uni_hashid(self):
        return self._name_is_uni_hashid

    @property
    def node_class_index(self):
        return self._node_class_index

    @property
    def uni_hashid(self):
        return self._uni_hashid

    @property
    def cls_node_dict(self):
        return Node._cls_node_dict
    # ---------------------------------------------------------------------

    # --method-set---------------------------------------------------------
    def cls_node_count_up(self):
        Node._cls_node_count += 1
        return Node._cls_node_count

    def cls_node_count_down(self):
        Node._cls_node_count -= 1
        return Node._cls_node_count

    def cls_node_list_append(self):
        Node._cls_node_list.append(self)
        return Node._cls_node_list

    def cls_node_list_remove(self):
        Node._cls_node_list.remove(self)
        return Node._cls_node_list
    # ---------------------------------------------------------------------

    # --method-set---------------------------------------------------------
    @property
    def parent_node(self):
        return self._parent_node

    @parent_node.setter
    def parent_node(self, parent_node):
        self._parent_node = parent_node
        return self._parent_node

    @parent_node.getter
    def parent_node(self):
        return self._parent_node

    def add_child(self, child):
        self._children.append(child)

    # --method--
    def remove_child(self, child):
        self._children.remove(child)

    @property
    def children(self):
        return self._children

    def child(self, row):
        return self._children[row]

    def child_count(self):
        return len(self._children)

    def row(self):
        if self._parent_node != None:
            return self._parent_node._children.index(self)
    # ---------------------------------------------------------------------

    # --method-------------------------------------------------------------
    def get_sibling_node_from_hashid(self, hashid):
        if not isinstance(hashid, str):
            raise TypeError("{0}.{1}: Accepts hashids as str types!"
                            "".format(self.__class__.__name__,
                                      'get_sibling_node_from_hashid(hashid)'))

        if hashid in self.cls_node_dict.keys():
            return self.cls_node_dict[hashid]
        else:
            return None

    # --method-------------------------------------------------------------
    def clear_node_dep(self):
        self.clear_children
        self.clear_node_list()
        self.clear_node_count()
        return self

    def clear_node_list(self):
        Node._cls_node_list = []
        return Node._cls_node_list

    def clear_node_count(self):
        Node._cls_node_count = 0
        return Node._cls_node_count

    def clear_children(self):
        self._children = []
        return self._children
    # ---------------------------------------------------------------------

    # ---------------------------------------------------------------------
    @cachetools.cached(cachetools.LFUCache(maxsize=2048))
    def cache_node(self):
        return self
    # ---------------------------------------------------------------------

    # --method-------------------------------------------------------------
    def hierarchy(self, tab_level=-1):

        output = ''
        tab_level += 1

        for i in range(tab_level):
            output += '\t'

        output += ('{tab}/------node_name:: "{0}"\n'
                   '{1}       |type:: {2}\n'
                   '{1}       |_uni_hashid:: "{3}"\r'
                   ''.format(self._node_name,
                             '\t' * tab_level,
                             self._node_type,
                             self._uni_hashid,
                             tab=tab_level))

        # TO DO:: object hierarchy  "'hips'|'rightLeg'|'etc'"

        for child in self._children:
            output += child.hierarchy(tab_level)

        tab_level -= 1
        # output += '\n'

        return output

    def log_hierarchy(self):
        # Not implemented
        self._logger.autolog(self.hierarchy(),
                             "return_node_from_hashid('{0}').log_hierarchy()"
                             "".format(self._uni_hashid()))
        return
    # ---------------------------------------------------------------------

    # --method-------------------------------------------------------------
    # representation
    def __str__(self):
        '''Returns a nice string representation of the object.'''

        # TO DO:  need to improve this
        if self.node_name == None or self.name_is_uni_hashid == True:
            #  open parenthesis only
            output = ("{0}(node_name='{1}'"
                      "".format(self.__class__.__name__,
                                self.uni_hashid))
        else:
            output = ("{0}(node_name='{1}'"
                      "".format(self.__class__.__name__,
                                self.node_name))

        if self.parent_node != None:
            output += (", parent_node=return_node_from_hashid('{0}')"
                       "".format(self.parent_node.uni_hashid))

        if len(self.kwargs_dict) > 0:
            for key, value in self.kwargs_dict.items():
                if not isinstance(value, str):
                    output += (", {0}={1}".format(key, value))
                else:  # if a str add the extra quotes
                    output += (", {0}='{1}'".format(key, value))

        # add the close parenthesis
        output += ')'

        if self.name_is_uni_hashid == True or self.temp_node:
            output = ("return_node_from_hashid('{0}')"
                      "".format(self.node_name))

        # Node(self, node_name, parent_node=None, path='')
        return output

    # representation
    def __repr__(self):
        return '{0}({1})\r'.format(self.__class__.__name__, self.__dict__)
# --Class End--------------------------------------------------------------


class ClassProperty(property):
    """Decorator"""

    def __get__(self, cls, owner):
        return self.fget.__get__(None, owner)()
# --Class End--------------------------------------------------------------


###########################################################################
# tests(), code block for testing module
# -------------------------------------------------------------------------
def tests():
    default_node = Node()
    print(str(default_node))
    print(repr(default_node))
    print(default_node.uni_hashid)
    print(default_node.kwargs_dict)
    print(default_node.message_header)
    print(default_node.node_name)
    print(default_node.parent_node)
    print(default_node.node_class_index)
    # print(default_node.cls_node_dict)

    # temp_node = Node()
    temp_node = Node(temp_node=True)
    print(temp_node)

    test_default_node = Node(temp_node=True).get_sibling_node_from_hashid('kxYLm0XQeXJ7jWaP')
    print(test_default_node)

    another_test = return_node_from_hashid('kxYLm0XQeXJ7jWaP')
    print(another_test)

    second_node = Node(node_name='foo', parent_node=default_node)
    print(str(second_node))
    print(second_node.uni_hashid)
    print(second_node.kwargs_dict)
    print(second_node.message_header)
    print(second_node.node_name)
    print(second_node.parent_node.node_name)
    print(second_node.node_class_index)
    # print(second_node.cls_node_dict)

    second_child = Node(node_name='fooey', parent_node=another_test)
    print(str(second_child))
    print(second_child.uni_hashid)
    print(second_child.kwargs_dict)
    print(second_child.message_header)
    print(second_child.node_name)
    print(second_child.parent_node.node_name)
    print(second_child.node_class_index)
    # print(second_child.cls_node_dict)

    third_node = Node(node_name='kablooey', parent_node=second_node,
                      message_header='Node(): CUSTOM Message')
    print(str(third_node))
    print(third_node.uni_hashid)
    print(third_node.kwargs_dict)
    print(third_node.message_header)
    print(third_node.node_name)
    print(third_node.parent_node.node_name)
    print(third_node.node_class_index)

    fourth_node = Node(parent_node=second_node,
                       message_header='Node(): CUSTOM Message')
    print(str(fourth_node))
    print(fourth_node.uni_hashid)
    print(fourth_node.kwargs_dict)
    print(fourth_node.message_header)
    print(fourth_node.node_name)
    print(fourth_node.parent_node.node_name)
    print(fourth_node.node_class_index)

    kwarg_test_child = Node(node_name='kwargChild', parent_node=default_node, garble=1001001)  # custom kwarg
    print(str(kwarg_test_child))
    print(kwarg_test_child.uni_hashid)
    print(kwarg_test_child.node_name)
    print(kwarg_test_child.parent_node.node_name)
    print(kwarg_test_child.kwargs_dict)
    # check the custom arg/property garble
    print(kwarg_test_child.garble)

    # check the node hierarchy
    print(default_node.hierarchy())

    # retreive a node from it's known hashid
    prime_node = return_node_from_hashid('kxYLm0XQeXJ7jWaP')
    print(prime_node.uni_hashid)  # verify hasid
    # should return the same node as default_node
    print(prime_node.node_name)  # should be 'PRIME'
    return
# -------------------------------------------------------------------------


def cache_tests():
    cache = cachetools.LFUCache(maxsize=128)
    runner = scheduler()
    cache["HogJonny"] = 1001001
    runner.enter(2, 1, display_cached_value,
                 kwargs={'cache': cache, 'cache_key': 'HogJonny'})
    runner.enter(6, 1, display_cached_value,
                 kwargs={'cache': cache, 'cache_key': 'HogJonny'})
    runner.run()
    return
# -------------------------------------------------------------------------


def main():
    return
# - END, main() --


###########################################################################
# --call block-------------------------------------------------------------
if __name__ == "__main__":
    print ("# ----------------------------------------------------------------------- #")
    print ('~ noodly.Node ... Running script as __main__')
    print ("# ----------------------------------------------------------------------- #\r")

    # run simple tests
    tests()

    cache_tests()
