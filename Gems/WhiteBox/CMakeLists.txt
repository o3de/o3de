o3de_gem_setup("WhiteBox_test")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})

# Explicitly include our FindOpenMesh to ensure FetchContent runs even in
# installed engine builds where the engine only provides stub targets.
include(${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/FindOpenMesh.cmake)

# Manifold provides the CSG boolean operations (Api::MeshBoolean).
include(${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/FindManifold.cmake)

add_subdirectory(Code)