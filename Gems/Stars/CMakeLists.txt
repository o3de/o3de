
set(gem_path ${CMAKE_CURRENT_LIST_DIR})
set(gem_json ${gem_path}/gem.json)
o3de_restricted_path(${gem_json} gem_restricted_path gem_parent_relative_path)

ly_add_external_target_path(${CMAKE_CURRENT_LIST_DIR}/3rdParty)

add_subdirectory(Code)
