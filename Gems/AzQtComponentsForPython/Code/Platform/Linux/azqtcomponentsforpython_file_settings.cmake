#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Find the paths for the pyside2/shiboken modules.
ly_get_package_association("pyside2" PYSIDE_PACKAGE_NAME)
ly_package_get_target_folder(${PYSIDE_PACKAGE_NAME} PYSIDE2_DOWNLOAD_LOCATION)
set(PYSIDE2_MODULE_DIR ${PYSIDE2_DOWNLOAD_LOCATION}/${PYSIDE_PACKAGE_NAME})

# Find the qt package location.
ly_get_package_association("Qt" QT_PACKAGE_NAME)
ly_package_get_target_folder(${QT_PACKAGE_NAME} QT_DOWNLOAD_LOCATION)
set(QT_MODULE_DIR ${QT_DOWNLOAD_LOCATION}/${QT_PACKAGE_NAME}/qt)

set(PYSIDE2_PATH "${PYSIDE2_MODULE_DIR}/pyside2")
set(INCLUDE_DIR "${PYSIDE2_PATH}/include")
set(PYSIDE2_SHARE_PATH "${PYSIDE2_PATH}/share/PySide2")
set(PYSIDE2_TYPESYSTEM_PATH "${PYSIDE2_SHARE_PATH}/typesystems")
set(PYSIDE2_BIN_PATH "${PYSIDE2_PATH}/bin")
set(PYSIDE2_INCLUDE_DIR ${INCLUDE_DIR}/PySide2)
set(SHIBOKEN_GENERATOR_EXE_PATH "${PYSIDE2_BIN_PATH}/shiboken2${CMAKE_EXECUTABLE_SUFFIX}") 
 
set(SHIBOKEN_MODULE_PATH "${PYSIDE2_MODULE_DIR}/pyside2/lib")
set(PYSIDE2_LIB_DIR ${PYSIDE2_PATH}/lib)  

set(SHIBOKEN_INCLUDE_DIR "${INCLUDE_DIR}/shiboken2")

set(SHIBOKEN_SHARED_LIBRARIES "${SHIBOKEN_MODULE_PATH}/libshiboken2.abi3.so")
set(SHIBOKEN_SHARED_LIBRARIES_D "${SHIBOKEN_MODULE_PATH}/libshiboken2.abi3.so")
set(PYSIDE2_SHARED_LIBRARIES "${PYSIDE2_LIB_DIR}/libpyside2.abi3.so")
set(PYSIDE2_SHARED_LIBRARIES_D "${PYSIDE2_LIB_DIR}/libpyside2.abi3.so")
