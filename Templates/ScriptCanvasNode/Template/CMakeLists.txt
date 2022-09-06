ly_add_target(
    NAME ${Name}.Static STATIC
    NAMESPACE ScriptCanvasNodes
    FILES_CMAKE
        ${Name}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            .
        PUBLIC
            .
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            Gem::ScriptCanvas
            Gem::ScriptCanvas.Extensions
    AUTOGEN_RULES
        *.ScriptCanvasNodeable.xml,ScriptCanvasNodeable_Header.jinja,$path/$fileprefix.generated.h
        *.ScriptCanvasNodeable.xml,ScriptCanvasNodeable_Source.jinja,$path/$fileprefix.generated.cpp
        *.ScriptCanvasNodeable.xml,ScriptCanvasNodeableRegistry_Header.jinja,$path/$fileprefix_Nodeables.generated.h
        *.ScriptCanvasNodeable.xml,ScriptCanvasNodeableRegistry_Source.jinja,$path/$fileprefix_Nodeables.generated.cpp
)