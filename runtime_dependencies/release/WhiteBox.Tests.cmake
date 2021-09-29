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

ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/boost_filesystem-vc141-mt-x64-1_73.dll" "C:/Users/lesaelr/o3de/bin/release")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/boost_thread-vc141-mt-x64-1_73.dll" "C:/Users/lesaelr/o3de/bin/release")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/Half-2_5.dll" "C:/Users/lesaelr/o3de/bin/release")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/heif.dll" "C:/Users/lesaelr/o3de/bin/release")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/Iex-2_5.dll" "C:/Users/lesaelr/o3de/bin/release")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/IlmImf-2_5.dll" "C:/Users/lesaelr/o3de/bin/release")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/IlmThread-2_5.dll" "C:/Users/lesaelr/o3de/bin/release")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/Imath-2_5.dll" "C:/Users/lesaelr/o3de/bin/release")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/jpeg62.dll" "C:/Users/lesaelr/o3de/bin/release")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/lzma.dll" "C:/Users/lesaelr/o3de/bin/release")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/libpng16.dll" "C:/Users/lesaelr/o3de/bin/release")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/tiff.dll" "C:/Users/lesaelr/o3de/bin/release")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/zlib1.dll" "C:/Users/lesaelr/o3de/bin/release")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/yaml-cpp.dll" "C:/Users/lesaelr/o3de/bin/release")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/OpenColorIO.dll" "C:/Users/lesaelr/o3de/bin/release")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/OpenImageIO.dll" "C:/Users/lesaelr/o3de/bin/release")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/OpenImageIO.pyd" "C:/Users/lesaelr/o3de/bin/release")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/OpenImageIO_Util.dll" "C:/Users/lesaelr/o3de/bin/release")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/iconvert.exe" "C:/Users/lesaelr/o3de/bin/release")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/idiff.exe" "C:/Users/lesaelr/o3de/bin/release")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/igrep.exe" "C:/Users/lesaelr/o3de/bin/release")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/iinfo.exe" "C:/Users/lesaelr/o3de/bin/release")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/maketx.exe" "C:/Users/lesaelr/o3de/bin/release")
ly_copy("C:/Users/lesaelr/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/oiiotool.exe" "C:/Users/lesaelr/o3de/bin/release")
ly_copy("C:/Users/lesaelr/o3de/bin/release/ImGui.imguilib.dll" "C:/Users/lesaelr/o3de/bin/release")
ly_copy("C:/Users/lesaelr/o3de/bin/release/AzTestRunner.exe" "C:/Users/lesaelr/o3de/bin/release")


file(TOUCH C:/Users/lesaelr/o3de/runtime_dependencies/release/WhiteBox.Tests_release.stamp)
