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
Module Documentation:
    DccScriptingInterface:: SDK//maya//scripts//set_shelf.py

This module manages a custom shelf in maya for the DCCsi
Reference: https://gist.github.com/vshotarov/1c3176fe9e38dcaadd1e56c2f15c95d9
"""
# -------------------------------------------------------------------------
# -- Standard Python modules
# none

# -- External Python modules
# none

# -- DCCsi Extension Modules
# none

# -- Maya Extension Modules
import maya.cmds as mc
# -------------------------------------------------------------------------

def _null(*args):
    pass

# -------------------------------------------------------------------------
class customShelf(_Custom_Shelf):
    '''This is an example shelf.'''
    
    def build(self):
        self.add_button(label="button1")
        self.add_button("button2")
        self.add_button("popup")
        p = mc.popupMenu(b=1)
        self.add_menu_item(p, "popupMenuItem1")
        self.add_menu_item(p, "popupMenuItem2")
        sub = self.add_submenu(p, "subMenuLevel1")
        self.add_menu_item(sub, "subMenuLevel1Item1")
        sub2 = self.add_submenu(sub, "subMenuLevel2")
        self.add_menu_item(sub2, "subMenuLevel2Item1")
        self.add_menu_item(sub2, "subMenuLevel2Item2")
        self.add_menu_item(sub, "subMenuLevel1Item2")
        self.add_menu_item(p, "popupMenuItem3")
        self.add_button("button3")
# -------------------------------------------------------------------------



class _Custom_Shelf():
    '''A simple class to build custom shelves in maya.
     The build method is empty and an inheriting class should override'''

    def __init__(self, name="DCCsi", icon_path=""):
        self._name = name

        self._icon_path = icon_path

        self._label_background_color = (0, 0, 0, 0)
        self._label_colour = (.9, .9, .9)

        self._clean_old_shlef()
        
        mc.setParent(self._name)
        
        self.build()

    def build(self):
        '''Override this method in custom class. 
        Otherwise, nothing is added to the shelf.'''
        pass

    def add_button(self,
                   label='<NotSet>',
                   icon="commandButton.png",
                   command=_null,
                   doubleCommand=_null):
        '''Adds a shelf button with the specified label,
         command, double click command and image.'''
        mc.setParent(self._name)
        if icon:
            icon = self._icon_path + icon
        mc.shelfButton(width=37, height=37,
                       image=icon,
                       label=label,
                       command=command,
                       doubleClickCommand=doubleCommand,
                       imageOverlayLabel=label,
                       overlayLabelBackColor=self._label_background_color,
                       overlayLabelColor=self._label_colour)

    def add_menu_item(self, parent, label, command=_null, icon=""):
        '''Adds a shelf button with the specified label,
         command, double click command and image.'''
        if icon:
            icon = self._icon_path + icon
        return mc.menuItem(p=parent, l=label, c=command, i="")

    def add_submenu(self, parent, label, icon=None):
        '''Adds a sub menu item with the specified label and icon
         to the specified parent popup menu.'''
        if icon:
            icon = self._icon_path + icon
        return mc.menuItem(p=parent, l=label, i=icon, subMenu=1)

    def _clean_old_shlef(self):
        '''Checks if the shelf exists and empties it if it does or
         creates it if it does not.'''
        if mc.shelfLayout(self._name, ex=1):
            if mc.shelfLayout(self._name, q=1, ca=1):
                for each in mc.shelfLayout(self._name, q=1, ca=1):
                    mc.deleteUI(each)
        else:
            mc.shelfLayout(self._name, p="ShelfLayout")


# ==========================================================================
# Module Tests
# ==========================================================================
if __name__ == '__main__':
    customShelf()
    pass
