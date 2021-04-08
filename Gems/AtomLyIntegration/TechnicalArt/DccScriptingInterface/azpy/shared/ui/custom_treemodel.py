# coding:utf-8
#!/usr/bin/python
#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# -------------------------------------------------------------------------

from __future__ import unicode_literals
# from builtins import str

# built in's
import os
# from io import StringIO  # for handling unicode strings

#  azpy
from azpy import initialize_logger

# 3rd Party
from unipath import Path
import PySide2.QtCore as QtCore
import PySide2.QtWidgets as QtWidgets
import PySide2.QtGui as QtGui
# -------------------------------------------------------------------------
#  global space debug flag
_G_DEBUG = os.getenv('DCCSI_GDEBUG', False)

#  global space debug flag
_DCCSI_DEV_MODE = os.getenv('DCCSI_DEV_MODE', False)

_MODULE_PATH = Path(__file__)

_ORG_TAG = 'Amazon_Lumberyard'
_APP_TAG = 'DCCsi'
_TOOL_TAG = 'azpy.shared.ui.custom_treemodel'
_TYPE_TAG = 'test'

_MODULENAME = __name__
if _MODULENAME is '__main__':
    _MODULENAME = _TOOL_TAG

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
        self._root_node = self.buildRootNode('root', None, self._rootFilePath)

        # master selection
        self._masterSelection = masterSelection

        # build the master selection node
        self._masterNode = self.buildMasterNode('master', None, self._masterSelection, self._rootNode.path())

        # want to store off a couple lists in the model, for retreival later
        self._nodeList = None
        self._depNodesList = None

