"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit test for ly_test_tools.image.screenshot_compare_qssim
"""

import unittest.mock as mock

import numpy as np
import pytest

import ly_test_tools.image.screenshot_compare_qssim as screenshot_compare

pytestmark = pytest.mark.SUITE_smoke


class TestScreenshotCompare(object):

    def test_QuaternionMatrixConj_4x3Matrix_ValidConjugate(self):
        given_matrix = np.array([[[1, 2, 3, 4], [5, 6, 7, 8], [9, 10 , 11, 12]]])
        expected_conjugateMatrix = np.array([[[1, -2, -3, -4], [5, -6, -7, -8], [9, -10, -11, -12]]])
        result_conjugateMatrix = screenshot_compare._quaternion_matrix_conj(given_matrix)
        assert np.array_equal(result_conjugateMatrix,expected_conjugateMatrix)

    def test_QuaternionMatrixDot_4x3matrix_ValidDotProduct(self):
        given_matrix = np.array([[[0, 0, 0, 2], [0, 0, 4, 0], [0, 0, 8, 0]]])
        expected_answer = np.array([[2, 4, 8]])
        result_matrix = screenshot_compare._quaternion_matrix_dot(given_matrix, given_matrix)
        assert np.array_equal(expected_answer,result_matrix)

    @mock.patch('ly_test_tools.image.screenshot_compare_qssim._quaternion_matrix_dot')
    def test_QuaternionMatrixNorm_DotProductUsed_AssertDotProductCalled(self, mock_matrixDotProduct):
        given_matrix = np.array([[[1, 2, 3, 4], [5, 6, 7, 8], [9, 10, 11, 12]]])
        screenshot_compare._quaternion_matrix_norm(given_matrix)
        mock_matrixDotProduct.assert_called_once()
        mock_matrixDotProduct.assert_called_with(given_matrix,given_matrix)

    @mock.patch('ly_test_tools.image.screenshot_compare_qssim._quaternion_matrix_norm')
    def test_QuaternionMatrixDivide_NormCalledForSecondMatrix_AssertNormCalled(self, mock_matrixNorm):
        matrix_a = np.array([[[1, 2, 3, 4], [5, 6, 7, 8], [9, 10, 11, 12]]])
        matrix_b = np.array([[[1, 2, 3, 4], [5, 6, 7, 8], [9, 10, 11, 12]]])
        mock_matrixNorm.return_value = np.array([[1,2,3]])
        screenshot_compare._quaternion_matrix_div(matrix_a, matrix_b)
        mock_matrixNorm.assert_called_with(matrix_b)

    @mock.patch('numpy.divide',mock.MagicMock())
    @mock.patch('ly_test_tools.image.screenshot_compare_qssim._quaternion_matrix_mult',mock.MagicMock())
    @mock.patch('ly_test_tools.image.screenshot_compare_qssim._quaternion_matrix_conj')
    def test_QuaternionMatrixDivide_ConjugateCalledForSecondMatrix_AssertConjugateCalled(self, mock_matrixConjugate):
        matrix_a = np.array([[[1, 2, 3, 4], [5, 6, 7, 8], [9, 10, 11, 12]]])
        matrix_b = np.array([[[1, 2, 3, 4], [5, 6, 7, 8], [9, 10, 11, 12]]])
        screenshot_compare._quaternion_matrix_div(matrix_a, matrix_b)
        mock_matrixConjugate.assert_called_with(matrix_b)

    @mock.patch('numpy.divide',mock.MagicMock())
    @mock.patch('ly_test_tools.image.screenshot_compare_qssim._quaternion_matrix_conj')
    @mock.patch('ly_test_tools.image.screenshot_compare_qssim._quaternion_matrix_mult')
    def test_QuaternionMatrixDivide_MultiplyCalledForMatAConjB_AssertMultiplyCalled(self, mock_matrixMultiply,mock_matrixConjugate):
        matrix_a = np.array([[[1, 2, 3, 4], [5, 6, 7, 8], [9, 10, 11, 12]]])
        matrix_b = np.array([[[1, 2, 3, 4], [5, 6, 7, 8], [9, 10, 11, 12]]])
        mock_conjugate_return = np.array([[[1, -2, -3, -4], [5, -6, -7, -8], [9, -10, -11, -12]]])
        mock_matrixConjugate.return_value = mock_conjugate_return
        screenshot_compare._quaternion_matrix_div(matrix_a, matrix_b)
        mock_matrixMultiply.assert_called_with(matrix_a,mock_conjugate_return)

    @mock.patch('imageio.imread')
    @mock.patch('imageio.imwrite',mock.MagicMock())
    def test_qssim_CheckSameImage_ShouldReturnOne(self, mock_imageRead):
        matrix_a = np.array([[[1, 2, 3], [4, 5, 6], [7, 8, 9]]])
        matrix_b = np.array([[[1, 2, 3], [4, 5, 6], [7, 8, 9]]])
        mock_imageRead.side_effect = [matrix_a,matrix_b]
        assert screenshot_compare.qssim('test1.jpg', 'test2.jpg') == 1

    @mock.patch('imageio.imread')
    @mock.patch('imageio.imwrite',mock.MagicMock())
    def test_qssim_CheckAlmostSameImage_GreaterThanHalf(self, mock_imageRead):
        matrix_a = np.array([[[1, 2, 3], [4, 5, 6], [7, 8, 9]]])
        matrix_b = np.array([[[1, 2, 3], [4, 5, 6], [7, 8, 19]]])
        mock_imageRead.side_effect = [matrix_a,matrix_b]
        assert screenshot_compare.qssim('test1.jpg', 'test2.jpg') > 0.5

    @mock.patch('imageio.imread')
    @mock.patch('imageio.imwrite',mock.MagicMock())
    def test_qssim_CheckDifferentImage_ShouldNotReturnOne(self, mock_imageRead):
        matrix_a = np.array([[[1, 2, 3], [4, 5, 6], [7, 8, 9]]])
        matrix_b = np.array([[[11, 12, 13], [14, 15, 16], [17, 18, 19]]])
        mock_imageRead.side_effect = [matrix_a,matrix_b]
        assert screenshot_compare.qssim('test1.jpg', 'test2.jpg') != 1

    @mock.patch('imageio.imread')
    @mock.patch('imageio.imwrite')
    def test_qssim_CheckDiffImageSaved_AssertImSave(self, mock_imageSave, mock_imageRead):
        matrix_a = np.array([[[1, 2, 3], [4, 5, 6], [7, 8, 9]]])
        matrix_b = np.array([[[1, 2, 3], [4, 5, 6], [7, 8, 9]]])
        mock_imageRead.side_effect = [matrix_a,matrix_b]
        screenshot_compare.qssim('test1.jpg', 'test2.jpg')
        mock_imageSave.assert_called()
