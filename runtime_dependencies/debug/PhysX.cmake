#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

cmake_policy(SET CMP0012 NEW) # new policy for the if that evaluates a boolean out of "if(NOT ${same_location})"

function(ly_copy source_file target_directory)
    get_filename_component(target_filename "${source_file}" NAME)
    cmake_path(COMPARE "${source_file}" EQUAL "${target_directory}/${target_filename}" same_location)
    if(NOT ${same_location})
        file(LOCK ${target_directory}/${target_filename}.lock GUARD FUNCTION TIMEOUT 300)
        if("${source_file}" IS_NEWER_THAN "${target_directory}/${target_filename}")
            message(STATUS "Copying \"${source_file}\" to \"${target_directory}\"...")
            file(COPY "${source_file}" DESTINATION "${target_directory}" FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE FOLLOW_SYMLINK_CHAIN)
            file(TOUCH_NOCREATE ${target_directory}/${target_filename})
        endif()
    endif()    
endfunction()

ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/PhysX-4.1.2.29882248-rev3-windows/PhysX/pxshared//debug/bin/PhysX_64.dll" "C:/Users/lesaelr/o3de/bin/debug")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/PhysX-4.1.2.29882248-rev3-windows/PhysX/pxshared//debug/bin/PhysXCooking_64.dll" "C:/Users/lesaelr/o3de/bin/debug")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/PhysX-4.1.2.29882248-rev3-windows/PhysX/pxshared//debug/bin/PhysXFoundation_64.dll" "C:/Users/lesaelr/o3de/bin/debug")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/PhysX-4.1.2.29882248-rev3-windows/PhysX/pxshared//debug/bin/PhysXCommon_64.dll" "C:/Users/lesaelr/o3de/bin/debug")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/PhysX-4.1.2.29882248-rev3-windows/PhysX/pxshared/debug/bin/PhysXDevice64.dll" "C:/Users/lesaelr/o3de/bin/debug")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/PhysX-4.1.2.29882248-rev3-windows/PhysX/pxshared/debug/bin/PhysXGpu_64.dll" "C:/Users/lesaelr/o3de/bin/debug")
ly_copy("C:/Users/lesaelr/o3de/bin/debug/LmbrCentral.dll" "C:/Users/lesaelr/o3de/bin/debug")


file(TOUCH C:/Users/lesaelr/o3de/runtime_dependencies/debug/PhysX_debug.stamp)
