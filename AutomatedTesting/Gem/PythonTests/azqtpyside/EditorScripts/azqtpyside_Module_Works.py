"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.
SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class AzQtPysideTests:
    found_outliner_titlebar_correct = (
        "Successfully found the Outliner Titlebar.",
        "Failed to find the Outliner Titlebar."
    )
    found_eliding_label_correct = (
        "Successfully found ElidingLabel.",
        "Failed to fine the ElidingLabel."
    )
    titlebar_text_correct = (
        "Successfully retrieved Titlebar text.",
        "Failed to retrieve Titlebar text."
    )


def AzQtPyside_AccessUIElementWorks():
    """
    Summary:
    Test retrieving data from UI elements to verify the azqtpyside module works.
    :return: None
    """

    import editor_python_test_tools.hydra_editor_utils as hydra
    import editor_python_test_tools.pyside_utils as pyside_utils

    from PySide2 import QtWidgets, QtCore
    from azqtpyside import AzQtComponents as az
    import azlmbr.legacy.general as general

    EntityOutlinerName = "Entity Outliner"

    hydra.open_base_level()

    # Ensure the outliner is open
    general.open_pane("Entity Outliner")

    editor_window = pyside_utils.get_editor_main_window()

    # Find the Outliner widget.
    entity_outliner = editor_window.findChild(QtWidgets.QDockWidget, EntityOutlinerName)

    # Look for the Titlebar of the widget using AzQtComponent types.
    titlebars = entity_outliner.findChildren(az.TitleBar)

    # Check one was found
    Report.result(AzQtPysideTests.found_outliner_titlebar_correct, len(titlebars) == 1)

    # To get the text content of the Titlebar, find the AzQtComponents::ElidingLabel child.
    labels = titlebars[0].findChildren(az.ElidingLabel)

    # Check one was found
    Report.result(AzQtPysideTests.found_eliding_label_correct, len(labels) == 1)

    # Check the text is as we expected, using the AzQtComponents::ElidingLabel::Text() method.
    label_text = labels[0].Text()
    res = label_text == EntityOutlinerName
    Report.result(AzQtPysideTests.titlebar_text_correct, label_text == EntityOutlinerName)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(AzQtPyside_AccessUIElementWorks)
