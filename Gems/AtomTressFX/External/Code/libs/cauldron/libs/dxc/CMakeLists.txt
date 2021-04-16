project (DXC)

add_library(${PROJECT_NAME} SHARED IMPORTED GLOBAL)

set_property(TARGET ${PROJECT_NAME} PROPERTY IMPORTED_IMPLIB dxcompiler.lib)

set(DXC_dlls bin/dxil.dll bin/dxcompiler.dll)
file(COPY ${DXC_dlls} DESTINATION ${CMAKE_HOME_DIRECTORY}/bin)
