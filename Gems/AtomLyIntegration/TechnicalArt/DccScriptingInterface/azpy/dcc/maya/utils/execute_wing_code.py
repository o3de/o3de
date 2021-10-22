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
import os
import socket
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def get_stub_check_path(in_path=__file__, check_stub='engineroot.txt'):
    '''
    Returns the branch root directory of the dev\\'engineroot.txt'
    (... or you can pass it another known stub)

    so we can safely build relative filepaths within that branch.

    If the stub is not found, it returns None
    '''
    path = os.path.abspath(os.path.join(os.path.dirname(in_path), ".."))
    _LOGGER.info('parent dir: {}'.format(path))

    while 1:
        test_path = os.path.join(path, check_stub)

        if os.path.isfile(test_path):
            return os.path.abspath(os.path.join(os.path.dirname(test_path)))

        else:
            path, tail = (os.path.abspath(os.path.join(os.path.dirname(test_path), "..")),
                          os.path.basename(test_path))

            if (len(tail) == 0):
                return None
# -------------------------------------------------------------------------

# -------------------------------------------------------------------------
# -- Global Definitions --
_MODULENAME = 'azpy.dcc.maya.utils.execute_wing_code'
_LOGGER = _logging.getLogger(_MODULENAME)

_O3DE_DEV = get_stub_check_path()
_LOGGER.info('_O3DE_DEV: {}'.format(_O3DE_DEV))

_PROJ_CACHE = os.path.join(_O3DE_DEV, 'cache', 'DCCsi', 'wing')
_LOGGER.info('_PROJ_CACHE: {}'.format(_PROJ_CACHE))

_LOCAL_HOST = socket.gethostbyname(socket.gethostname())
_LOGGER.info('local_host: {}'.format(_LOCAL_HOST))
# -------------------------------------------------------------------------


###########################################################################
# --main code block--------------------------------------------------------
def main(code_type='python'):
    """
    Evaluate the temp file on disk, made by Wing, in Maya.

    code_type : string : Supports either 'python' or 'mel'

    """
    temp_file_name = 'tmp_wing_data.txt'

    temp_file_path = os.path.join(_PROJ_CACHE, temp_file_name)
    temp_file_path = os.path.abspath(temp_file_path)
    temp_file = temp_file_path.replace("\\", "/")  # maya is linux paths?
    _LOGGER.debug('temp_file_path is: {}'.format(temp_file_path))
    
    if os.access(temp_file, os.F_OK):
        # open and print the file in Maya:
        f = open(temp_file, "r")
        lines = f.readlines()
        for line in lines:
            print(line.rstrip())
        f.close()

        if code_type == "python":
            # execute the file contents in Maya:
            f = open(temp_file, "r")
            # (1) doesn't work?
            #exec(f, __main__.__dict__, __main__.__dict__)
            # (2) works is series of single expressions
            #for line in lines:
                #exec(line.rstrip())
                # f.close()
            # (3) this seems to work much better
            temp_code_file_name = 'temp_code.py'
            temp_code_file = os.path.join(_PROJ_CACHE, temp_code_file_name)
            temp_code_file = os.path.abspath(temp_code_file)
            temp_code = temp_file_path.replace("\\", "/")  # maya is linux paths?
            code = compile(f.read(), temp_code, 'exec')
            _LOGGER.debug(type(code))
            exec(code)                

        elif code_type == "mel":
            mel_cmd = "source '{}'".format(temp_file)
            # This causes the "// Result: " line to show up in the Script Editor:
            om.MGlobal.executeCommand(mel_cmd, True, True)
    else:
        _LOGGER.warning("No temp file exists: {}".format(temp_file))
        file=open(temp_file, "w")
        file.write("test file write")
        if os.path.isfile(temp_file):
            _LOGGER.info('Created the temp file, please try again!')
        else:
            _LOGGER.error('File not created: {}'.format(temp_file))

    return
# -------------------------------------------------------------------------
