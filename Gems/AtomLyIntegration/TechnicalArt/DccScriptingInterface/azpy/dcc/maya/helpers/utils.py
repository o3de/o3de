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

"""
azpy.dcc.maya utility module 
"""
# -------------------------------------------------------------------------
# built in's
# none

# 3rd Party
# none

# Lumberyard extensions
from azpy import initialize_logger
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE

# maya imports
import maya.cmds as cmds
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# -- Misc Global Space Definitions
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)

_PACKAGENAME = __name__
if _PACKAGENAME is '__main__':
    _PACKAGENAME = 'azpy.dcc.maya.helpers.undo_context'

_LOGGER = initialize_logger(_PACKAGENAME, default_log_level=int(20))
_LOGGER.debug('Invoking:: {0}.'.format({_PACKAGENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# Initiate the Wing IDE debug connection.
if _DCCSI_GDEBUG:
    #import azpy.dev.connectDebugger as lyDevConnnect
    # lyDevConnnect()
    pass
# -------------------------------------------------------------------------


# =========================================================================
# First Class
# =========================================================================
class Selection(object):
    '''
    Custom Class to handle selection data as well as helper commands
    to use/parse the selection data
    '''

    component_prefixes = {0: 'vtx', 1: 'e', 2: 'f', 3: 'map', 4: 'vtxFace'}

    #----------------------------------------------------------------------
    def __init__(self):
        self.selection = dict()
        self._populate_selection_data()
    #----------------------------------------------------------------------


    #----------------------------------------------------------------------
    def _populate_cpv_data(self):
        '''
        Color per-vertex data
        Populates a dictionary:
            color/alpha key -> vtxFace list
        '''

        for obj in self.selection.keys():
            rgba_dict = dict()

            if not cmds.listRelatives( obj, shapes = True ) :
                self.selection[obj].append( rgba_dict )
                continue

            selection_list = []
            selection_list = self.get_vtx_face_list( obj )

            if len(selection_list) == 0:
                # build attribute fetch
                attrTag = '{0}.{1}'.format( obj, "vtxFace[*][*]")
                # objects vtxFaceList
                obj_vtx_faces = cmds.polyListComponentConversion(attrTag,
                                                                toVertexFace=True)

                selection_list = cmds.ls( obj_vtx_faces,
                                         long = True,
                                         flatten = True )

            for vtx_face in selection_list:
                # get color RGB values
                try:
                    # query the color
                    color_list = cmds.polyColorPerVertex( vtx_face,
                                                         query = True,
                                                         colorRGB = True)
                except:
                    # if no color, assign balck
                    cmds.polyColorPerVertex( obj,
                                             colorRGB = [0,0,0],
                                             alpha = True)

                    color_list = cmds.polyColorPerVertex(vtx_face,
                                                         query=True,
                                                         colorRGB=True)

                # and get alpha values
                alpha = cmds.polyColorPerVertex(vtx_face,
                                                query = True,
                                                alpha = True)

                # tuple up color and alpha
                rgba = ( color_list[0], color_list[1], color_list[2], alpha[0] )

                if rgba not in rgba_dict :
                    rgba_dict[rgba] = []

                rgba_dict[rgba].append( vtx_face )

            self.selection[obj].append( rgba_dict )
    #----------------------------------------------------------------------


    #----------------------------------------------------------------------
    def _make_component_dict(self, component_list):
        '''Populates a dictionary of selected components of the given type'''

        component = dict()

        if not component_list:
            return component

        for comp in component_list :
            # parent of this dag node
            par = cmds.listRelatives( comp, parent = True, fullPath = True)[0]

            # get transform
            transform = cmds.listRelatives( par, parent = True, fullPath = True)[0]

            # To Do: explain what this does
            comp_num = comp.split('[')[-1].split(']')[0]

            if transform not in component:
                component[transform] = []

            component[transform].append(comp)

        return component
    #----------------------------------------------------------------------


    #----------------------------------------------------------------------
    def _fill_with_component_dict(self, component_type):
        '''
        Wrapper class for making a component dictionary

        Takes in a component type:
        http://download.autodesk.com/us/maya/2011help/CommandsPython/filterExpand.html

        '''
        sel_objs = []
        component_dict = dict()
        components_from_type = cmds.filterExpand(expand=True,
                                                fullPath=True,
                                                selectionMask=component_type)

        # pack component dict
        component_dict = self._make_component_dict( components_from_type )

        try:
            sel_objs += self.selection.keys()
        except:
            pass

        try:
            sel_objs += component_dict.keys()
        except:
            pass

        sel_objs = set(sel_objs)

        for obj in sel_objs:
            if obj not in self.selection.keys() :
                self.selection[obj] = []

            if obj in component_dict:
                self.selection[obj].append( component_dict[obj] )

            else:
                self.selection[obj].append( None )
    #----------------------------------------------------------------------


    #----------------------------------------------------------------------
    def _populate_selection_data(self):
        '''Main population method for filling the class with all the selection data'''
        #Vertices - Edges - Faces - UVs - VtxFace - Color To VtxFace
        #Final Selection Dictionary Map
        sel_objs = []

        try:
            sel_objs += cmds.ls( selection = True, transforms = True, long = True )
        except:
            pass

        try:
            sel_objs += cmds.ls( hilite = True, long = True )
        except:
            pass

        sel_objs = set( sel_objs )

        for obj in sel_objs :
            self.selection[obj] = []

        self._fill_with_component_dict(31)  # Polygon Vertices
        self._fill_with_component_dict(32)  # Polygon Edges
        self._fill_with_component_dict(34)  # Polygon Face
        self._fill_with_component_dict(35)  # Polygon UVs
        self._fill_with_component_dict(70)  # Polygon Vertex Face

        self._populate_cpv_data()
    #----------------------------------------------------------------------


    #----------------------------------------------------------------------
    def store_selection(self):
        '''Takes current selection and populates the class with the selection data'''
        self.selection = dict()
        self._populate_selection_data()
    #----------------------------------------------------------------------


    #----------------------------------------------------------------------
    def prettyprint(self):
        '''Pretty Print method to inspect the selection data'''

        crossbar_str = '{0}'.format('*' * 75)

        print ( crossbar_str )
        print ( '~ Begin Selection Data Output...' )

        for key, value in self.selection.items() :
            print key
            #This is less explict but allows for expansion easier, lets test it out for a while
            for itemSet in value:
                print '  ', itemSet
            #print '  vtx       - ', value[0]
            #print '  edg       - ', value[1]
            #print '  face      - ', value[2]
            #print '  UV        - ', value[3]
            #print '  vtx-face  - ', value[4]
            #print '  colorDict - ', value[5]

        print ( crossbar_str )
    #----------------------------------------------------------------------


    #----------------------------------------------------------------------
    def select(self, obj, component_type, clear_selection=True, add_value=False):
        '''Allow easy reselection of specific selection data'''

        cmds.select( clear = clear_selection )

        if self.selection[obj][component_type] != None :
            cmds.hilite( obj )
            cmds.select(self.selection[obj][component_type], add=add_value)
    #----------------------------------------------------------------------


    #----------------------------------------------------------------------
    def restore_selection(self):
        '''Restores the selection back to how it was when the class populated its selection data'''

        cmds.select( clear = True)

        for obj in self.selection.keys() :
            cmds.select( obj, add = True )
            self.select( obj, 0, 0, add_value = True )
            self.select( obj, 1, 0, add_value = True )
            self.select( obj, 2, 0, add_value = True )
            self.select( obj, 3, 0, add_value = True )
    #----------------------------------------------------------------------


    #----------------------------------------------------------------------
    def get_vtx_face_list(self, obj):
        '''Turns all current selection data into a vtxFace selection list'''

        selection_list = set()

        # To Do: explain what this does
        for index in xrange(0, 2) :

            sel_part = self.selection[obj][index]
            try:
                sel = cmds.polyListComponentConversion( sel_part, toVertexFace = True)
                selection_list.update( cmds.ls( sel, long = True, flatten = True) )
            except:
                pass

        # To Do: explain what this does
        try:
            selection_list.update( self.selection[obj][4] )
        except:
            pass

        return list(selection_list)
    #----------------------------------------------------------------------


    #----------------------------------------------------------------------
    def get_first_mesh(self):
        '''Returns the first mesh it finds in the selection list'''

        for item in self.selection.keys() :
            if cmds.listRelatives( item, shanpes = True ) :
                return item
    #----------------------------------------------------------------------


    #----------------------------------------------------------------------
    def get_component_list(self, obj, component_index=0):

        if self.selection[obj][component_index] == None : # Return all
            component_tag = '{0}.{1}[*]'.format(obj, self.component_prefixes[component_index])
            return cmds.ls( component_tag, long = True, flatten = True )
        else:
            return self.selection[obj][component_index]
    #----------------------------------------------------------------------


    #----------------------------------------------------------------------
    def get_component_index(self, obj, component_index=0):

        index_list = []

        if self.selection[obj][component_index] :
            for comp in self.selection[obj][component_index] :
                component_number = comp.split('[')[-1].split(']')[0]
                index_list.append(component_number)

        return index_list
    #----------------------------------------------------------------------


    #----------------------------------------------------------------------
    def get_inverse_component_index(self, obj, component_index=0):
        ''''''

        index_list = []
        full_component_list = []

        if self.selection[obj][component_index] :
            working = self.selection[obj][component_index]
            sel_component_list = cmds.ls(working, long=True, flatten=True)

            component_tag = '{0}.{1}[*]'.format(obj, self.component_prefixes[component_index])
            full_component_list = cmds.ls( component_tag, long = True, flatten = True)

            for comp in [comp for comp in full_component_list if comp not in sel_component_list ] :
                component_number = comp.split('[')[-1].split(']')[0]
                index_list.append(component_number)

        return index_list
    #----------------------------------------------------------------------
# -------------------------------------------------------------------------


#==========================================================================
# Class Test
#==========================================================================
if __name__ == '__main__':

    # get a selection object
    sel = Selection()
    
    # to do: this needs some tests?
