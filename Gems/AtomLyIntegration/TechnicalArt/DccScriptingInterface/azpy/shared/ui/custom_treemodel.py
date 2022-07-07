# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------

from __future__ import unicode_literals
# from builtins import str

# built in's
import os
# from io import StringIO  # for handling unicode strings

#  azpy
import azpy
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE

# 3rd Party
from unipath import Path
import PySide2.QtCore as QtCore
import PySide2.QtWidgets as QtWidgets
import PySide2.QtGui as QtGui
# -------------------------------------------------------------------------
#  global space
# To Do: update to dynaconf dynamic env and settings?
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)

_MODULE_PATH = Path(__file__)

_MODULENAME = 'azpy.shared.ui.custom_treemodel'

_log_level = int(20)
if _DCCSI_GDEBUG:
    _log_level = int(10)
_LOGGER = azpy.initialize_logger(_MODULENAME,
                                 log_to_file=False,
                                 default_log_level=_log_level)
_LOGGER.debug('Starting:: {}.'.format({_MODULENAME}))

_UI_FILE = Path(_MODULE_PATH.parent, 'resources', 'example.ui')
# -------------------------------------------------------------------------

###########################################################################
## CustomTreeModel, Class
# -------------------------------------------------------------------------


class CustomFileTreeModel(QtCore.QAbstractItemModel):
    """
    Creates a customized model subclassed from, QAbstractItemModel
    Compatible with a TreeView
    """

    # --constructor--------------------------------------------------------
    def __init__(self, parent=None, *args, **kwargs):
        '''
        Constructor, INPUTS: Node, QObject
        '''
        super(CustomFileTreeModel, self).__init__(parent=parent, *args, **kwargs)

        # can use these later (maybe?)
        #  easily extended later, TODO implement as property, add append
        self._file_ext_list = ['.sbs', '.sbsar']

        # we assume this is a file tree, we need a root path (lmbr project?)
        self._root_filepath = root_filepath
        # TODO: need to make sure we are getting path objects

        # build the root node
        self._root_node = self.build_rootnode('root', None, self._root_filepath)

        # prime selection
        self._prime_selection = prime_selection

        # build the prime selection node
        self._primenode = self.build_primenode('prime', None, self._prime_selection, self._rootnode.path())

        # want to store off a couple lists in the model, for retreival later
        self._nodelist = None
        self._dep_nodelist = None

