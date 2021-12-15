import logging as _logging

FRMT_LOG_LONG = "[%(name)s][%(levelname)s] >> %(message)s (%(asctime)s; %(filename)s:%(lineno)d)"
_logging.basicConfig(level=_logging.INFO,
                    format=FRMT_LOG_LONG,
                    datefmt='%m-%d %H:%M')


_LOGGER = _logging.getLogger('Tools.Python.SubstanceTools')
_LOGGER.propagate = False
_LOGGER.info('\n\n Tool Start ----------------->>>>')
from utils import settings
from PySide2 import QtWidgets, QtCore, QtGui
from azpy.dcc.maya.utils import maya_materials
from azpy.dcc.substance import sbs_export


class SubstanceTools(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super(SubstanceTools, self).__init__(parent)

        self.setWindowTitle("Substance Tools")
        self.setObjectName('SubstanceTools')
        self.setGeometry(50, 50, 500, 606)





"""
********************************
*** Create PySide2 interface ***
********************************

-> Set Substance Automation Toolkit Location (if not found, but hardcode it for tool)
-> File Browser
-> Baking options (Bake all possible maps)
---- AO
---- Normal
---- Direction
---- Curvature
---- Position

-> 

-> Export Atom Material checkbox
-> Process File button

**********************
*** DCC conversion ***
**********************

***  (Start with Maya and expand if there's time)
-> Determine file type, connect to DCC api
-> Parse file objects and materials
-> Output list of scene objects that contains mesh name, material name(s), and connected textures
--- Put this information into a scrollable table with individual components for each found object
--- Include a button for each object found that will process texture baking and sbs creation
--- Include a button that activates after bake and launches 
++++++ (Radio Buttons- After process complete:) -Launch Substance  -Do Nothing

"""