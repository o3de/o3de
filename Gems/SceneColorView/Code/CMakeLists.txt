ly_add_target(
    NAME SceneColorView.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE scenecolorview_files.cmake
    INCLUDE_DIRECTORIES PUBLIC Include PRIVATE Source
    BUILD_DEPENDENCIES PUBLIC AZ::AzCore AZ::AzFramework Gem::Atom_RPI.Public
)

ly_add_target(
    NAME SceneColorView ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE scenecolorview_module_files.cmake
    INCLUDE_DIRECTORIES PRIVATE Source
    BUILD_DEPENDENCIES PRIVATE Gem::SceneColorView.Static
)

ly_create_alias(NAME SceneColorView.Clients NAMESPACE Gem TARGETS Gem::SceneColorView)
ly_create_alias(NAME SceneColorView.Unified NAMESPACE Gem TARGETS Gem::SceneColorView)
o3de_add_variant_dependencies_for_gem_dependencies(GEM_NAME SceneColorView VARIANTS Clients Unified)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME SceneColorView.Editor GEM_MODULE
        NAMESPACE Gem
        FILES_CMAKE scenecolorview_editor_files.cmake
        INCLUDE_DIRECTORIES PRIVATE Source
        BUILD_DEPENDENCIES PRIVATE Gem::SceneColorView.Static AZ::AzToolsFramework
    )
    ly_create_alias(NAME SceneColorView.Tools NAMESPACE Gem TARGETS Gem::SceneColorView.Editor)
    ly_create_alias(NAME SceneColorView.Builders NAMESPACE Gem TARGETS Gem::SceneColorView.Editor)
    o3de_add_variant_dependencies_for_gem_dependencies(GEM_NAME SceneColorView VARIANTS Tools Builders)
endif()
