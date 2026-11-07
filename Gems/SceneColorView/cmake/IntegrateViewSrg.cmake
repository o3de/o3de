# Material shaders and SceneAndViewSrgs must see the same per-view declarations.
# Run at configure time, before Asset Processor consumes the project ShaderLib.
set(_scv_projects ${LY_PROJECTS})
if(NOT _scv_projects AND EXISTS "${CMAKE_SOURCE_DIR}/project.json")
    list(APPEND _scv_projects "${CMAKE_SOURCE_DIR}")
endif()
foreach(_scv_project IN LISTS _scv_projects)
    file(REAL_PATH "${_scv_project}" _scv_project_path BASE_DIRECTORY "${CMAKE_SOURCE_DIR}")
    if(NOT EXISTS "${_scv_project_path}/project.json")
        message(FATAL_ERROR "SceneColorView: project.json missing in ${_scv_project_path}")
    endif()
    if(NOT LY_PYTHON_CMD)
        message(FATAL_ERROR "SceneColorView: O3DE LY_PYTHON_CMD is unavailable")
    endif()
    execute_process(
        COMMAND ${LY_PYTHON_CMD} "${CMAKE_CURRENT_LIST_DIR}/../Tools/integrate_viewsrg.py"
            --project "${_scv_project_path}"
        RESULT_VARIABLE _scv_result
        OUTPUT_VARIABLE _scv_output
        ERROR_VARIABLE _scv_error
    )
    if(NOT _scv_result EQUAL 0)
        message(FATAL_ERROR "SceneColorView: shared View SRG setup failed.\n${_scv_output}\n${_scv_error}")
    endif()
    message(STATUS "SceneColorView: ${_scv_output}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_LIST_DIR}/../Tools/integrate_viewsrg.py"
        "${CMAKE_CURRENT_LIST_DIR}/../Assets/ShaderLib/SceneColorView/ViewSrg.azsli"
        "${_scv_project_path}/ShaderLib/viewsrg.srgi"
    )
endforeach()
unset(_scv_projects)
unset(_scv_project)
unset(_scv_project_path)
unset(_scv_result)
unset(_scv_output)
unset(_scv_error)
