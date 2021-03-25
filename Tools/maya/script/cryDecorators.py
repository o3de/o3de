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
from maya import cmds
from functools import wraps
import inspect

import logging
logging.basicConfig()
log = logging.getLogger(__name__)
log.setLevel(logging.DEBUG)

def pluginDependency(sPlugin, bEnableAutoLoad=False, bErrorOnFailure=False):
    '''
    This is a decorator, to be used with functions that expect a certain plugin to be loaded.
    If the required plugin is not already loaded, it will try to load it, before calling the decorated function.
    If the plugin cannot be loaded, it will raise an exception and return None without calling the decorated function.
    
    The plugin's name needs to be supplied as a string, like so:
    @pluginDepdendency("myPlugin.mll)
    def myFunction():
        # do things...
    '''
    print('sPlugin: %s' % sPlugin)
    
    def decorator(fFunction):
        @wraps(fFunction)
        def wrapper(*args, **kwargs):
            # check for plugin and try to load it if necessary
            bPluginLoaded = False
            
            if not cmds.pluginInfo(sPlugin, query=True, loaded=True):
                log.debug("Plugin not loaded. Attempting to load: " + sPlugin)
                
                try:
                    cmds.loadPlugin(sPlugin)
                    if bEnableAutoLoad:
                        cmds.pluginInfo(sPlugin, edit=True, autoload=True)
                    bPluginLoaded = True
                except Exception as eException:
                    sModule = (inspect.getmodule(fFunction)).__name__
                    #sFile = inspect.getsourcefile(fFunction)
                    #sLine = (inspect.getsourcelines(fFunction))[-1]
                    sMessage = str(eException) + "Aborting function call: " + sModule + "." + fFunction.__name__
                    
                    if bErrorOnFailure:
                        cmds.error(sMessage)
                    else:
                        cmds.warning(sMessage)
            else:
                bPluginLoaded = True
            
            if bPluginLoaded:
                log.debug("Plugin loaded. Continuing function: " + fFunction.__name__)
                
                return fFunction(*args, **kwargs)
            else:
                return None
        
        return wrapper
    
    return decorator