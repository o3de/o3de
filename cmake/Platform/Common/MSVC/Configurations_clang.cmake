#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

include(cmake/Platform/Common/Configurations_common.cmake)

ly_append_configurations_options(
    DEFINES_PROFILE
        _FORTIFY_SOURCE=2
    DEFINES_RELEASE
        _FORTIFY_SOURCE=2
    COMPILATION
        -Wno-reorder-ctor
        -Wno-logical-not-parentheses
        -Wno-logical-op-parentheses
        -Wno-switch
        -Wno-undefined-var-template
        -Wno-inconsistent-missing-override
        -Wno-parentheses
        -Wno-unused-parameter
        -Wno-sign-compare
        -Wno-ignored-qualifiers
        -Wno-missing-field-initializers

        # disable warning introduced by -fsized-deallocation, pybind11 used.
        -Wno-unknown-argument

        
        /fp:fast        # allows the compiler to reorder, combine, or simplify floating-point operations to optimize floating-point code for speed and space
        /Gd             # Use _cdecl calling convention for all functions
        /MP             # Multicore compilation in Visual Studio
        /nologo         # Suppress Copyright and version number message
        /W4             # Warning level 4
        /WX             # Warnings as errors
        /permissive-    # Conformance with standard
        /Zc:preprocessor # Forces preprocessor into conformance mode:  https://docs.microsoft.com/en-us/cpp/preprocessor/preprocessor-experimental-overview?view=msvc-170
        /Zc:forScope    # Force Conformance in for Loop Scope
        /diagnostics:caret # Compiler diagnostic options: includes the column where the issue was found and places a caret (^) under the location in the line of code where the issue was detected.
        /Zc:__cplusplus       
        /bigobj         # Increase number of sections in obj files. Profiling has shown no meaningful impact in memory nore build times
        /GS             # Enable Buffer security check
        /sdl
    COMPILATION_DEBUG
        /MDd            # defines _DEBUG, _MT, and _DLL and causes the application to use the debug multithread-specific and DLL-specific version of the run-time library.
                        # It also causes the compiler to place the library name MSVCRTD.lib into the .obj file.
        /Ob0            # Disables inline expansions
        /Od             # Disables optimization
    COMPILATION_PROFILE
        /GF             # Enable string pooling
        /Gy             # Function level linking
        /MD             # Causes the application to use the multithread-specific and DLL-specific version of the run-time library. Defines _MT and _DLL and causes the compiler
                        # to place the library name MSVCRT.lib into the .obj file.
        /O2             # Maximinize speed, equivalent to /Og /Oi /Ot /Oy /Ob2 /GF /Gy
        /Zc:inline      # Removes unreferenced functions or data that are COMDATs or only have internal linkage
        /Zc:wchar_t     # Use compiler native wchar_t
        /Zi             # Generate debugging information (no Edit/Continue)
    COMPILATION_RELEASE
        /Ox             # Full optimization
        /Ob2            # Inline any suitable function
        /Ot             # Favor fast code over small code
        /Oi             # Use Intrinsic Functions
        /Oy             # Omit the frame pointer
    LINK
        /NOLOGO             # Suppress Copyright and version number message
        /IGNORE:4099        # 3rdParty linking produces noise with LNK4099
    LINK_NON_STATIC_PROFILE
        /OPT:REF            # Eliminates functions and data that are never referenced
        /OPT:ICF            # Perform identical COMDAT folding. Redundant COMDATs can be removed from the linker output
        /INCREMENTAL:NO
        /DEBUG              # Generate pdbs
    LINK_NON_STATIC_RELEASE
        /OPT:REF # Eliminates functions and data that are never referenced
        /OPT:ICF # Perform identical COMDAT folding. Redundant COMDATs can be removed from the linker output
        /INCREMENTAL:NO
)

if(LY_BUILD_WITH_ADDRESS_SANITIZER)
    ly_append_configurations_options(
        COMPILATION_DEBUG
            -fsanitize=address
            -fno-omit-frame-pointer
        LINK_NON_STATIC_DEBUG
            -shared-libsan
            -fsanitize=address
    )
endif()
include(cmake/Platform/Common/TargetIncludeSystemDirectories_supported.cmake)

