#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

set(LY_RUNTIME_DEPENDENCIES_HEADER
"
set(anything_new FALSE)
set(plugin_libs)
set(plugin_dirs)

function(ly_copy source_file target_directory)
    get_filename_component(target_filename \"\${source_file}\" NAME)
    get_filename_component(source_file_dir \"\${source_file}\" DIRECTORY)
    # If source_file is a Framework and target_directory is a bundle
    if(\"\${source_file}\" MATCHES \".[Ff]ramework\" AND \"\${target_directory}\" MATCHES \".app/Contents/MacOS\")
        set(plugin_dirs \"\${plugin_dirs};\${source_file_dir}\" PARENT_SCOPE)
        return() # skip, it will be fixed with fixup_bundle
    elseif(\"\${source_file}\" MATCHES \"qt/plugins\" AND \"\${target_directory}\" MATCHES \".app/Contents/MacOS\")
        # fixup the destination so it ends up in Contents/Plugins
        set(plugin_dirs \"\${plugin_dirs};\${source_file_dir}\" PARENT_SCOPE)
        set(plugin_libs \"\${plugin_libs};\${target_directory}/\${target_filename}\" PARENT_SCOPE)
    elseif(\"\${source_file}\" MATCHES \"qt/translations\" AND \"\${target_directory}\" MATCHES \".app/Contents/MacOS\")
        return() # skip
    elseif(\"\${source_file}\" MATCHES \".dylib\")
        set(plugin_dirs \"\${plugin_dirs};\${source_file_dir}\" PARENT_SCOPE)
    endif()
    if(NOT \"\${source_file}\" STREQUAL \"\${target_directory}/\${target_filename}\")
        if(NOT EXISTS \"\${target_directory}\")
            file(MAKE_DIRECTORY \"\${target_directory}\")
        endif()
        if(\"\${source_file}\" IS_NEWER_THAN \"\${target_directory}/\${target_filename}\")
            file(LOCK \"\${CMAKE_BINARY_DIR}/runtimedependencies.lock\" GUARD FUNCTION TIMEOUT 30)
            file(COPY \"\${source_file}\" DESTINATION \"\${target_directory}\" FILE_PERMISSIONS ${LY_COPY_PERMISSIONS} FOLLOW_SYMLINK_CHAIN)
            set(anything_new TRUE)
        endif()
    endif()    
endfunction()
\n")

set(LY_RUNTIME_DEPENDENCIES_FOOTER
"
if(@target_file_dir@ MATCHES \".app/Contents/MacOS\")
    if(NOT anything_new)
        string(REGEX REPLACE \"(.*.app)/Contents/MacOS.*\" \"\\\\1\" bundle_path \"@target_file_dir@\")
        set(timestamp_file \"\${bundle_path}.fixup.stamp\")
        if(NOT EXISTS \"\${timestamp_file}\")
            set(anything_new TRUE)
        else()
            file(GLOB_RECURSE files_in_bundle FOLLOW_SYMLINKS \"\${bundle_path}\")
            foreach(file \${files_in_bundle})
                if(\${file} IS_NEWER_THAN \"\${timestamp_file}\")
                    set(anything_new TRUE)
                    break()
                endif()
            endforeach()
        endif()
    endif()
    if(anything_new)
        include(BundleUtilities)
        list(REMOVE_DUPLICATES plugin_libs)
        list(REMOVE_DUPLICATES plugin_dirs)
        fixup_bundle(\"\${bundle_path}\" \"\${plugin_libs}\" \"\${plugin_dirs}\")
        file(TOUCH \"\${timestamp_file}\")
    endif()
endif()
")




include(cmake/Platform/Common/RuntimeDependencies_common.cmake)