# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# inspiration: http://www.emeraldartist.com/blog/2012/10/11/remotely-sending-code-to-maya-from-wing

from __future__ import unicode_literals

""" Module to remotely send code to maya. Inside of WingIDE prefs, you will
need to add the parent dire of this module to 'IDE Extension Scripting>Search Path'
Additionally, you will need set up custom key bindings 'User Interface>Keyboard'

"""
# -- This line is 75 characters -------------------------------------------

# standard imports
import socket
import random
import sys
import os
import time
import logging as _logging

# wing ide
import wingapi
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
MODULENAME = 'azpy.dev.ide.wing.hot_keys'
_LOGGER = _logging.getLogger(MODULENAME)

## extend logger
#_handler = _logging.StreamHandler(sys.stdout)
#_handler.setLevel(_logging.DEBUG)
#FRMT_LOG_LONG = "[%(name)s][%(levelname)s] >> %(message)s (%(asctime)s; %(filename)s:%(lineno)d)"
#_formatter = _logging.Formatter(FRMT_LOG_LONG)
#_handler.setFormatter(_formatter)
#_LOGGER.addHandler(_handler)
#_LOGGER.debug('Loading: {0}.'.format({MODULENAME}))

_LOCAL_HOST = socket.gethostbyname(socket.gethostname())
_LOGGER.info('local_host: {}'.format(_LOCAL_HOST))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def display_text(test_str):
    """Displays text in a WingIDE pop-up dialog"""
    app = wingapi.gApplication
    v = "Product info is: " + str(app.GetProductInfo())
    v += "\nAnd you typed: %s" % test_str
    wingapi.gApplication.ShowMessageDialog("Test Message", v)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def get_wing_text():  # no hotkey
    """
    Return the text currently selected in wing
    """
    editor = wingapi.gApplication.GetActiveEditor()
    if editor is None:
        return None
    else:
        current_doc = editor.GetDocument()
        start, end = editor.GetSelection()
        text_block = current_doc.GetCharRange(start, end)
        _LOGGER.debug('selected text is: {}'.format(text_block))
        return text_block
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def display_wing_text():  # Ctrl+Shift+D
    text_block = get_wing_text()
    display_text(text_block)
    return text_block

display_wing_text.contexts = [
    wingapi.kContextNewMenu("DCCsi Scripts"),
    wingapi.kContextEditor(),
]
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
# globals

_O3DE_DEV = get_stub_check_path()
_LOGGER.info('_O3DE_DEV: {}'.format(_O3DE_DEV))

_PROJ_CACHE = os.path.join(_O3DE_DEV, 'cache', 'DCCsi', 'wing')
_LOGGER.info('_PROJ_CACHE: {}'.format(_PROJ_CACHE))

if not os.path.exists(_PROJ_CACHE):
    os.makedirs(_PROJ_CACHE)
    _LOGGER.info('SUCCESS creating: {}'.format(_PROJ_CACHE))
else:
    _LOGGER.info('_PROJ_CACHE already exists: {}'.format(_PROJ_CACHE))

# makedirs(_PROJ_CACHE)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def create_client_socket(local_host=_LOCAL_HOST,command_port=6000):
    """create a client (wing) socket connection to maya (server, commandPort)"""
    for res in socket.getaddrinfo(local_host, command_port,
                                  socket.AF_UNSPEC, socket.SOCK_STREAM,0, socket.AI_PASSIVE):
        af, socktype, proto, canonname, sa = res
        try:
            mSocket = socket.socket(af, socktype, proto)
        except socket.error as e:
            mSocket = None
            continue
        try:
            # Make our socket --> Maya connection:
            mSocket.connect(sa)
        except socket.error as e:
            mSocket.close()
            mSocket = None
            continue 
        break

    if not mSocket:
        raise RuntimeError("Unable to initialise client socket.")
    
    return mSocket
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def send_selection_to_maya(language='python',
                           local_host=_LOCAL_HOST,
                           command_port=6000):
    """Basic method to connect to Maya and send selected code over.
    chunks can be large, which makes socket programming cumbersome.
    This module stashes the selection in a temp .txt file in the cache.
    Then send maya a command for a specific module, which will then
    read that file and execute the code line-by-line, allowing for
    arbirtarily large selections that might otherwise overrun buffer"""

    port_name = str('{0}:{1}'.format(local_host, command_port))
    _LOGGER.info('port_name: {}'.format(port_name))

    if language != "mel" and language != "python":
        raise ValueError("Expecting either 'mel' or 'python'")

    # Save the text to a temp file.
    # If  mel, make sure it end with a semicolon
    selected_text = get_wing_text()
    if language == 'mel':
        if not selected_text.endswith(';'):
            selected_text += ';'

    # This saves a temp file on Windows
    # Mac\Linux support may need updating
    temp_file_name = 'tmp_wing_data.txt'

    temp_file_path = os.path.join(_PROJ_CACHE, temp_file_name)
    temp_file_path = os.path.abspath(temp_file_path)
    temp_file = temp_file_path.replace("\\", "/")  # maya is linux paths?
    _LOGGER.debug('temp_file_path is: {}'.format(temp_file_path))

    if os.access(temp_file, os.F_OK):
        # open and print the file in Maya:
        f=open(temp_file_path, "w")
        f.write(selected_text)
        f.close()
    else:
        _LOGGER.info("No temp file exists: {}".format(temp_file))
        file=open(temp_file, "w")
        if os.path.isfile(temp_file):
            _LOGGER.info('Created the file, please try again')
        else:
            _LOGGER.info('File not created')

    # Create the socket that will connect to Maya,  Opening a socket can vary from
    mSocket = create_client_socket(local_host, command_port)
    
    if mSocket:
        # Now ping Maya over the command-port
        message = ("import azpy.maya.utils.execute_wing_code;"
                   "azpy.maya.utils.execute_wing_code.main('{}')".format(language))

        if language == 'mel':
            message = 'python({})'.format(message)  # wrap in in mel python cmd
        try:
            # Send our code to Maya:
            mSocket.send(message.encode('ascii'))
            time.sleep(1)
            client_response = str(mSocket.recv(4096)).encode("utf-8")  # receive the result info
            # time.sleep(0.25)
            # next command
        except Exception as e:
            _LOGGER.error("Sending command to Maya failed: {}".format(e))
    
        _LOGGER.info("salt:{0}:: sent: {1}".format(str(random.randint(1, 9999)), message))
        _LOGGER.info("The result is: {}".format(client_response))
    
        mSocket.close()
    else:
        _LOGGER.error('No client socket, mSocket is: {}'.format(mSocket))

    return
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def send_command_to_maya(language='python',
                         local_host=_LOCAL_HOST,
                         command_port=6000):
    """Basic method to connect to Maya and send a single smaller command directly"""

    port_name = str('{0}:{1}'.format(local_host, command_port))
    _LOGGER.info('port_name: {}'.format(port_name))

    if language != "mel" and language != "python":
        raise ValueError("Expecting either 'mel' or 'python'")

    # Save the text to a temp file.
    # If  mel, make sure it end with a semicolon
    selected_text = get_wing_text()
    if language == 'mel':
        if not selected_text.endswith(';'):
            selected_text += ';'

    # Create the socket that will connect to Maya,  Opening a socket can vary from
    mSocket = create_client_socket(local_host, command_port)
    _LOGGER.info('mSocket is: {}'.format(mSocket))
    
    if mSocket:
        # Now ping Maya over the command-port
        message = str(selected_text)
        if language == 'mel':
            message = 'python({})'.format(message)  # wrap in in mel python cmd
        # Now ping Maya over the command-port
        try:
            if language == 'mel':
                message = 'python({})'.format(message)

            # to do: the buffer default I think is 4096
            # long selections are going to fail (not sure how)
            mSocket.send(message.encode('ascii'))
            time.sleep(1)
            client_response = str(mSocket.recv(4096)).encode("utf-8")  # receive the result info
            # time.sleep(0.25)
            # next command
        except Exception as e:
            _LOGGER.error("Sending command to Maya failed: {}".format(e))
    
        _LOGGER.info("salt:{0}:: sent: {1}".format(str(random.randint(1, 9999)), message))
        _LOGGER.info("The result is: {}".format(client_response))
    
        mSocket.close()
    else:
        _LOGGER.error('No client socket, mSocket is: {}'.format(mSocket))

    return
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def send_py_cmd_to_maya():
    """Send the selected Python command to Maya"""
    send_command_to_maya()  # default language is 'python'

send_py_cmd_to_maya.contexts = [
    wingapi.kContextNewMenu("DCCsi Scripts"),
    wingapi.kContextEditor(),
]

def send_mel_cmd_to_maya():
    """Send the selected code to Maya as mel"""
    send_command_to_maya('mel')

send_mel_cmd_to_maya.contexts = [
    wingapi.kContextNewMenu("DCCsi Scripts"),
    wingapi.kContextEditor(),
]

def python_selection_to_maya():
    """Send the selected Python code to Maya"""
    send_selection_to_maya()  # default language is 'python'

python_selection_to_maya.contexts = [
    wingapi.kContextNewMenu("DCCsi Scripts"),
    wingapi.kContextEditor(),
]

def mel_selection_to_maya():
    """Send the selected code to Maya as mel"""
    send_selection_to_maya('mel')

mel_selection_to_maya.contexts = [
    wingapi.kContextNewMenu("DCCsi Scripts"),
    wingapi.kContextEditor(),
]

# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    # there are not really tests to run here due to this being a list of
    # constants for shared use.
    _DCCSI_GDEBUG = True
    _DCCSI_DEV_MODE = True
    _LOGGER.setLevel(_logging.DEBUG)  # force debugging

    foo = get_wing_text()

    # send each line to maya with > send_py_cmd_to_maya
    print('Hello World: Command received from WingIDE')
    #import maya.cmds as cmds
    #foo = cmds.polySphere()

