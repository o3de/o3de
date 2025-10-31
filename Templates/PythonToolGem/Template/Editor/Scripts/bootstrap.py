"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------
"""${SanitizedCppName}\\editor\\scripts\\boostrap.py
Generated from O3DE PythonToolGem Template"""

import az_qt_helpers
from ${SanitizedNameLower}_dialog import ${SanitizedCppName}Dialog

if __name__ == "__main__":
    print("${SanitizedCppName}.boostrap, Generated from O3DE PythonToolGem Template")

    try:
        import azlmbr.editor as editor
        # Register our custom widget as a dockable tool with the Editor under an Examples sub-menu
        options = editor.ViewPaneOptions()
        options.showOnToolsToolbar = True
        options.toolbarIcon = ":/${Name}/toolbar_icon.svg"
        az_qt_helpers.register_view_pane('${SanitizedCppName}', ${SanitizedCppName}Dialog, category="Examples", options=options)
    except:
        # If the editor is not available (in the cases where this gem is activated outside of the Editor), then just 
        # report it and continue.
        print(f'Skipping registering view pane ${SanitizedCppName}, Editor is not available.')
