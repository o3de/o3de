"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from aws_cdk import (
    core,
    aws_cloudwatch as cloudwatch
)

from typing import List

from . import aws_metrics_constants


class LayoutWidget(cloudwatch.Column):
    """
    Construct for creating a custom layout with a maximum width.
    The layout has a text widget containing a short description on the first row and
    all other widgets will be presented on the following rows based on their width values.
    """
    def __init__(
            self,
            widgets: List[cloudwatch.IWidget],
            layout_description: str,
            max_width: int = aws_metrics_constants.DASHBOARD_MAX_WIDGET_WIDTH) -> None:
        self._max_width = max_width
        self._Rows = []
        self._currentRowWidth = 0
        self._currentRowWidgets = []

        widgets.insert(
            0,
            cloudwatch.TextWidget(
                markdown=layout_description,
                width=max_width
            )
        )

        # Add all the widgets to the layout.
        self.__add_widgets(widgets)

        super().__init__(*self._Rows)

    def __add_widgets(
            self,
            widgets: [cloudwatch.IWidget]
            ):
        for widget in widgets:
            if (self._currentRowWidth + widget.width) > self._max_width:
                # The current row is full. Add the new widget to the next row.
                self._Rows.append(cloudwatch.Row(*self._currentRowWidgets))
                self._currentRowWidgets.clear()
                self._currentRowWidth = 0

            self._currentRowWidgets.append(widget)
            self._currentRowWidth = self._currentRowWidth + widget.width

        if self._currentRowWidth > 0:
            # Add the rest widgets to the last row even if the row is not full.
            self._Rows.append(cloudwatch.Row(*self._currentRowWidgets))
