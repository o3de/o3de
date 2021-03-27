"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Utility package for LyTestTools.

Provides image capturing functionality.
"""
import pyscreenshot


def screencap(x1=None, y1=None, x2=None, y2=None, filename='screenshot.png'):
    """
    Capture an arbitrary portion (in absolute coordinates) of the screen to a file.  Note currently accepts coords
    for the leftmost screen only.  Any span beyond/below will be blank.

    If any of the coordinate parameters is None, the entire screen will be captured.

    Note this is a brittle way to verify a portion of an application.

    :param x1: Top left X
    :param y1: Top left Y
    :param x2: Bottom right X
    :param y2: Bottom right Y
    :param filename: Filename to save to.
    :return: Image that was saved in PIL "Image" class.
    """

    if x1 is not None and x2 is not None and y1 is not None and y2 is not None:
        im = pyscreenshot.grab(bbox=(x1, y1, x2, y2), childprocess=False)
    else:
        im = pyscreenshot.grab(childprocess=False)

    im.save(filename)

    return im
