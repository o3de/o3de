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


# --------------------------------------------------------------------------
import os
import sys
import logging as _logging
# --------------------------------------------------------------------------


# -------------------------------------------------------------------------
#  global space debug flag, no fancy stuff here we use in bootstrap
_DCCSI_GDEBUG = False  # manually enable to debug this file

_PACKAGENAME = __name__
if _PACKAGENAME is '__main__':
    _PACKAGENAME = 'azpy.return_stub'

_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))

__all__ = ['return_stub']
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def return_stub(stub):
    _dir_to_last_file = None
    '''Take a file name (stub) and returns the directory of the file (stub)'''
    if _dir_to_last_file is None:
        path = os.path.abspath(__file__)
        while 1:
            path, tail = os.path.split(path)
            if (os.path.isfile(os.path.join(path, stub))):
                break
            if (len(tail) == 0):
                path = ""
                if _DCCSI_GDEBUG:
                    _LOGGER.debug('~ Debug Message:  I was not able to find the '
                                  'path to that file (stub) in a walk-up '
                                  'from currnet path')
                break
        _dir_to_last_file = path

    return _dir_to_last_file
# --------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    # there are not really tests to run here due to this being a list of
    # constants for shared use.

    # happy print
    _LOGGER.info("# {0} #".format('-' * 72))
    _LOGGER.info('~ find_stub.py ... Running script as __main__')
    _LOGGER.info("# {0} #\r".format('-' * 72))

    _LOGGER.info('~   Current Work dir: {0}'.format(os.getcwd()))

    _LOGGER.info('~   Dev\: {0}'.format(return_stub('engineroot.txt')))

    # custom prompt
    sys.ps1 = "[azpy]>>"
