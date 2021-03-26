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
#An example implementation for communicating with MotionBuilder
#through the Python Remote Server
#
#
#Christopher Evans - Apr 2008

import wx
import telnetlib
import win32gui
import win32com.client
import os
import sys
import time
import traceback

def process_is_running(proc_name):
    #From Adam Pletcher's "Python for Tech Artsts" (http://www.volition-inc.com/gdc/)
    import win32pdh

    proc_name = os.path.splitext(proc_name)[0].lower()

    junk, instances = win32pdh.EnumObjectItems(None,None,'Process', win32pdh.PERF_DETAIL_WIZARD)

    for proc_inst in instances:
        if (proc_inst.lower() == proc_name):
            return True

    return False

class MB_Sub_Frame(wx.Frame):
        #This comes from Adam Pletcher's "Python for Tech Artsts" free GDC course materials

    _top_mb_window_handle = None
    
    def __init__(self, parent, app, id=-1, title="Motion Builder Tool", pos=(60, 120), size=(350, 200), name='frame', resize=True, style=wx.DEFAULT_FRAME_STYLE|wx.FRAME_FLOAT_ON_PARENT|wx.FRAME_NO_TASKBAR):

        wx.Frame.__init__(self, parent, id, title, pos, size, style)

        self.app = app
        self.panel = None

        self.initialPosOffset = pos
        self._runningModal = False

        # resizable windows is the default in wxPython, but we allow caller to override that.
        if (not resize):
            wx.Frame.ToggleWindowStyle(self, wx.RESIZE_BORDER)
        
        # Tool window's background color.  This should be made to match the user's BG color in Max.
        bg_color = (197, 197, 197)

        self.SetBackgroundStyle(wx.BG_STYLE_COLOUR)
        self.SetBackgroundColour(wx.Color(bg_color[0], bg_color[1], bg_color[2]))

        # Event Bindings
        self.Bind(wx.EVT_CLOSE, self.on_close)

    @classmethod
    def create(cls, *args, **kw):
        """
        Call this static class method to create a new instance of your subframe.
        This will use the 3dsmax window as a parent for the current frame.
        Limitation: If > 1 copy of 3dsmax is open, uses the first one it finds.
        Could probably be fixed, however.
        """
        if (not process_is_running('motionbuilder.exe')):
            raise WindowsError('*** ERROR: MotionBuilder is not running')
        else:
            app = wx.GetApp()
    
            if (app == None):
                app = wx.App(redirect=False)
    
            val = MB_Sub_Frame._get_top_mb_window()
            topHandle = val[0]
            
            if (topHandle == None):
                raise WindowsError('*** ERROR: Motion Builder window handle not found.')
            else:
                windowPos = val[1]
                
                toolFrame = wx.PreFrame()
                toolFrame.AssociateHandle(topHandle)
                toolFrame.PostCreate(toolFrame)
                app.SetTopWindow(toolFrame)

                try:
                    frame = cls(toolFrame, app, *args, **kw)
                    frame.Show(True)
                    
                except:
                    print cls.__name__
                    print traceback.format_exc()
                    frame = None
                
                toolFrame.DissociateHandle()
        
                return frame

    @staticmethod
    def _get_top_mb_window():
        if MB_Sub_Frame._top_mb_window_handle is not None:
            return MB_Sub_Frame._top_mb_window_handle

        # EnumWindows requires a callback function
        def callback(handle, winList):
            winList.append(handle)
            return True

        # Get list of open windows
        windows = []
        win32gui.EnumWindows(callback, windows)

        # Find window belonging to 3ds Max
        for handle in windows:
            title = win32gui.GetWindowText(handle)
            if ('MotionBuilder 7.5' in title):
                # Maximize the window if it's minimized
                import win32con
                window = win32gui.ShowWindow(handle, win32con.SW_MAXIMIZE)
                
                # Get upper left desktop coords of MB window
                pos = win32gui.GetWindowRect(handle)[:2]

                # Set handle id in our instance                
                MB_Sub_Frame._top_mb_window_handle = handle
                
                return (handle, pos)
        # MB window not found
        return None
        
    def on_close(self, evt):
        """
        Event handler for EVT_CLOSE event.
        """
        self.Show(False)
        self.Destroy()
        win32gui.DestroyWindow(self.GetHandle())

        # Regular wx Destroy waits for the application to destroy the window.
        # Since there is no wxApp running, we do it ourselves.
        win32gui.SetFocus(MB_Sub_Frame._top_mb_window_handle)

    def show_modal_dialog(self, dlg):
        """
        Method to display a modal dialog that also disables the 3dsmax window.
        """
        mb_disabled = self._runningModal                    # Save enabled state, so we can restore it when this modal dialog is done
        
        if (not mb_disabled):
            self._runningModal = True
            top = MB_Sub_Frame._top_mb_window_handle    # get MB window handle
            win32gui.EnableWindow(top, False)                # disable it
        
        ret_val = dlg.ShowModal()                                # show our dialog - won't continue until dialog is closed

        if (not mb_disabled):                                    # enables 3dsmax window again, if it was enabled when this dialog started
            win32gui.EnableWindow(top, True)
            self._runningModal = False

        return ret_val

#------------------------------------------------------------------------------------------------------------------------

host = telnetlib.Telnet("127.0.0.1", 4242)

def mbPipe(command):
    host.read_until('>>>', .01)
    #write the command
    host.write(command + '\n')
    print ('Sending>>> '+ command)
    #read all data returned
    raw = str(host.read_until('>>>', .1))
    #removing garbage i don't want
    raw = raw.replace('\n\r>>>','')
    raw = raw.replace('\r','')
    rawArr = raw.split('\n')
    #cleanArr = [item.replace('\r', '') for item in rawArr]
    return rawArr

def getSelection():
    selectedItems = []
    mbPipe("selectedModels = FBModelList()")
    mbPipe("FBGetSelectedModels(selectedModels,None,True)")
    for item in (mbPipe("for item in selectedModels: print item.Name")):
        selectedItems.append(item)
    return selectedItems

class MyFrame(MB_Sub_Frame):
    rStabMarkers = []
    lStabMarkers = []
    cStabMarkers = []
    mSetMarkers = []
    
    def __init__(self,parent,app):
        # create a frame, no parent, default to wxID_ANY
        MB_Sub_Frame.__init__(self, parent, app, id=-1, title='Face Stabilization',pos=(300, 150), size=(200, 190))

        self.rStab = wx.Button(self, id=-1, label='Right STAB Markers',pos=(8, 8), size=(175, 28))
        self.rStab.Bind(wx.EVT_BUTTON,self.rStabClick)
        self.rStab.SetToolTip(wx.ToolTip("Select 1+ Right Stabilization Markers and Press"))

        self.lStab = wx.Button(self, id=-1, label='Left STAB Markers',pos=(8, 38), size=(175, 28))
        self.lStab.Bind(wx.EVT_BUTTON, self.lStabClick)
        self.lStab.SetToolTip(wx.ToolTip("Select 1+ Left Stabilization Markers and Press"))
        
        self.cStab = wx.Button(self, id=-1, label='Center STAB Markers',pos=(8, 68), size=(175, 28))
        self.cStab.Bind( wx.EVT_BUTTON, self.cStabClick )
        self.cStab.SetToolTip(wx.ToolTip("Select 1+ Center Stabilization Markers"))

        self.markerSet = wx.Button(self, id=-1, label='Markers to Stabilize',pos=(8, 98), size=(175, 28))
        self.markerSet.Bind(wx.EVT_BUTTON, self.markerSetClick)
        self.markerSet.SetToolTip(wx.ToolTip("Select All Markers to Stabilize"))

        self.stabilize = wx.Button(self, id=-1, label='Stabilize Markerset',pos=(8, 128), size=(175, 28))
        self.stabilize.Bind(wx.EVT_BUTTON, self.stabilizeClick)
        self.stabilize.SetToolTip(wx.ToolTip("Press to Stabilize the Markers"))

    def rStabClick(self,event):
        self.rStabMarkers = getSelection()
        print str(self.rStabMarkers)
        self.rStab.Label = (str(len(self.rStabMarkers)) + " Right Markers")
        
    def lStabClick(self,event):
        self.lStabMarkers = getSelection()
        print str(self.lStabMarkers)
        self.lStab.Label = (str(len(self.lStabMarkers)) + " Light Markers")

    def cStabClick(self,event):
        self.cStabMarkers = getSelection()
        print str(self.cStabMarkers)
        self.cStab.Label = (str(len(self.cStabMarkers)) + " Center Markers")

    def markerSetClick(self,event):
        self.mSetMarkers = getSelection()
        print str(self.mSetMarkers)
        self.markerSet.Label = (str(len(self.mSetMarkers)) + " Markers to Stabilize")

    def stabilizeClick(self,event):
        mbPipe('from euclid import *')
        mbPipe('from cryLib import *')
        mbPipe('rStab = modelsFromStrings(' + str(self.rStabMarkers) + ')')
        mbPipe('lStab = modelsFromStrings(' + str(self.lStabMarkers) + ')')
        mbPipe('cStab = modelsFromStrings(' + str(self.cStabMarkers) + ')')
        mbPipe('markerset = modelsFromStrings(' + str(self.mSetMarkers) + ')')
        mbPipe('stab(rStab,lStab,cStab,markerset,False)')

    def on_close(self, event):
        #This method is bound by MB_Sub_Frame to the EVT_CLOSE event.
        print 'Shutting down...'
        host.close()
        self.user_exit = True
        self.Destroy()

application = wx.PySimpleApp()
window = MyFrame.create()
application.MainLoop()
