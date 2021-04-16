#
# Copies a set of files to a directory (only if they already don't exist or are different) 
# Usage example:
#     set(media_src ${CMAKE_CURRENT_SOURCE_DIR}/../../media/brdfLut.dds )    
#     copyCommand("${media_src}" ${CMAKE_HOME_DIRECTORY}/bin)
#
function(copyCommand list dest)
    foreach(fullFileName ${list})    
        get_filename_component(file ${fullFileName} NAME)
        message("Generating custom command for ${fullFileName}")
        add_custom_command(
            OUTPUT   ${dest}/${file}
            PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${dest}
            COMMAND ${CMAKE_COMMAND} -E copy ${fullFileName}  ${dest}
            MAIN_DEPENDENCY  ${fullFileName}
            COMMENT "Updating ${file} into ${dest}" 
        )
    endforeach()    
endfunction()
