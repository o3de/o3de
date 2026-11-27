#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

set(FILES
    "${openxr-sdk_SOURCE_DIR}/src/common/filesystem_utils.cpp"
    "${openxr-sdk_SOURCE_DIR}/src/common/object_info.cpp"
    "${openxr-sdk_SOURCE_DIR}/src/loader/api_layer_interface.cpp"
    "${openxr-sdk_SOURCE_DIR}/src/loader/loader_core.cpp"
    "${openxr-sdk_SOURCE_DIR}/src/loader/loader_init_data.cpp"
    "${openxr-sdk_SOURCE_DIR}/src/loader/loader_instance.cpp"
    "${openxr-sdk_SOURCE_DIR}/src/loader/loader_logger.cpp"
    "${openxr-sdk_SOURCE_DIR}/src/loader/loader_logger_recorders.cpp"
    "${openxr-sdk_SOURCE_DIR}/src/loader/loader_properties.cpp"
    "${openxr-sdk_SOURCE_DIR}/src/loader/manifest_file.cpp"
    "${openxr-sdk_SOURCE_DIR}/src/loader/runtime_interface.cpp"
    "${openxr-sdk_SOURCE_DIR}/src/loader/xr_generated_loader.cpp"
    "${openxr-sdk_SOURCE_DIR}/src/xr_generated_dispatch_table_core.c"
)

if(ANDROID)
    list(APPEND FILES
        "${openxr-sdk_SOURCE_DIR}/src/external/android-jni-wrappers/wrap/android.content.cpp"
        "${openxr-sdk_SOURCE_DIR}/src/external/android-jni-wrappers/wrap/android.database.cpp"
        "${openxr-sdk_SOURCE_DIR}/src/external/android-jni-wrappers/wrap/android.net.cpp"
        "${openxr-sdk_SOURCE_DIR}/src/external/jnipp/jnipp.cpp"
        "${openxr-sdk_SOURCE_DIR}/src/loader/android_utilities.cpp"
    )
endif()
