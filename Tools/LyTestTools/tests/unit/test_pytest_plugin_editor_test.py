"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import pytest
import os
import unittest.mock as mock
import unittest

import ly_test_tools._internal.pytest_plugin.editor_test as editor_test

pytestmark = pytest.mark.SUITE_smoke

class TestEditorTest(unittest.TestCase):

    @mock.patch('inspect.isclass', mock.MagicMock(return_value=True))
    def test_PytestPycollectMakeitem_ValidArgs_CallsCorrectly(self):
        mock_collector = mock.MagicMock()
        mock_name = mock.MagicMock()
        mock_obj = mock.MagicMock()
        mock_base = mock.MagicMock()
        mock_obj.__bases__ = [mock_base]

        editor_test.pytest_pycollect_makeitem(mock_collector, mock_name, mock_obj)
        mock_base.pytest_custom_makeitem.assert_called_once_with(mock_collector, mock_name, mock_obj)

    def test_PytestCollectionModifyitem_OneValidClass_CallsOnce(self):
        mock_item = mock.MagicMock()
        mock_class = mock.MagicMock()
        mock_class.pytest_custom_modify_items = mock.MagicMock()
        mock_item.instance.__class__ = mock_class
        mock_session = mock.MagicMock()
        mock_items = [mock_item, mock.MagicMock()]
        mock_config = mock.MagicMock()

        generator = editor_test.pytest_collection_modifyitems(mock_session, mock_items, mock_config)
        for x in generator:
            pass
        assert mock_class.pytest_custom_modify_items.call_count == 1
