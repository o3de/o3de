"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""
# -------------------------------------------------------------------------

# -------------------------------------------------------------------------
# -------------------------------------------------------------------------
# master.py
# Allows for project based setup to be used with noodly
# version: 0.1
# author: Gallowj
# -------------------------------------------------------------------------
# -------------------------------------------------------------------------
import os

from unipath import Path

_G_DEFAULT_PROJECT_DIR = os.getcwd()
_G_MASTER_ROOT_NODE = None


@property
def _G_DEFAULT_PROJECT_DIR(value):
    _G_DEFAULT_PROJECT_DIR = value
    return _G_DEFAULT_PROJECT_DIR


def set_PROJECT_DIR(value):
    """Sets and returns _G_DEFAULT_PROJECT_DIR"""
    global _G_DEFAULT_PROJECT_DIR
    _G_DEFAULT_PROJECT_DIR = Path(value).expand()
    return Path(_G_DEFAULT_PROJECT_DIR)


@property
def _G_MASTER_ROOT_NODE(value):
    _G_MASTER_ROOT_NODE = value
    return _G_MASTER_ROOT_NODE


def set_MASTER_ROOT_NODE(value):
    """Sets and returns _G_MASTER_ROOT_NODE"""
    global _G_MASTER_ROOT_NODE
    if not isinstance(inNode, ProjectRootNode):
        raise TypeError('self._projectRootNode is: {0}\r'
                        'A _projectRootNode, needs to be properly set\r'
                        'So that we can acces:\r'
                        '\tself._projectRootNode._sourceRoot\r'
                        '\tself._projectRootNode._overrideRoot\r'
                        'Use self.assignProjectRootNode(<projectRootNode>)\r'
                        ''.format(type(inNode)))

    _G_MASTER_ROOT_NODE = inNode
    return _G_MASTER_ROOT_NODE


###########################################################################
# tests(), code block for testing module
# -------------------------------------------------------------------------
def tests():
    set_PROJECT_DIR(os.getcwd())
    print(_G_DEFAULT_PROJECT_DIR)
    print(_G_DEFAULT_PROJECT_DIR.parent)
    print(_G_DEFAULT_PROJECT_DIR.components())

    # NOT implemented yet
    # set_PROJECT_DIR

    return


###########################################################################
# --call block-------------------------------------------------------------
if __name__ == "__main__":
    print ("# ----------------------------------------------------------------------- #")
    print ('~ noodly.master ... Running script as __main__')
    print ("# ----------------------------------------------------------------------- #\r")

    # run simple tests
    tests()

    #_G_DEFAULT_PROJECT_DIR = Path(os.getcwd())
    print(_G_DEFAULT_PROJECT_DIR.components())
