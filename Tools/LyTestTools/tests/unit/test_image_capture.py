"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Unit tests for image_capture.py
"""
import unittest.mock as mock
import unittest

import pytest

import ly_test_tools.image.image_capture

pytestmark = pytest.mark.SUITE_smoke


class TestScreenCap(unittest.TestCase):

    @mock.patch('pyscreenshot.grab')
    def test_ScreenCap_CoordsAndNameGiven_Used(self, mock_grab):
        mock_grab.return_value = mock.MagicMock()
        mock_grab.return_value.save = mock.MagicMock()
        x1 = 10
        x2 = 20
        y1 = 30
        y2 = 40
        image_name = 'test_capture.png'

        ly_test_tools.image.image_capture.screencap(x1, y1, x2, y2, filename=image_name)

        mock_grab.assert_called_once_with(bbox=(x1, y1, x2, y2), childprocess=False)
        mock_grab.return_value.save.assert_called_once_with(image_name)
