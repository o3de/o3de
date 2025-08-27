#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

include(cmake/Platform/Common/Configurations_common.cmake)

# Exceptions are disabled by default.  Use this to turn them on just for a specific target.
# MSVC-Clang uses MSVC compiler option syntax (so /EHsc instead of -fexceptions)
set(O3DE_COMPILE_OPTION_ENABLE_EXCEPTIONS PUBLIC /EHsc)

# O3DE Sets visibility to hidden by default, requiring explicit export on non-windows platforms
# But on MSVC or MS-Clang, these compilers use MSVC compiler options and behavior, which means
# it is not necessary to set visibility to hidden as on MSVC, things behave similar to if
# hidden by default.  As such, there is no need to change compile options for 3rd Party Libraries
# to cause them to export symbols.  This is thus blank
set(O3DE_COMPILE_OPTION_EXPORT_SYMBOLS "")

# By default, O3DE sets warning level 4 and sets warnings as errors.  If you're pulling in
# external code (from 3rd Party libraries) you can't really control whether they generate
# warnings or not, and its usually out of scope to fix them.  Add this compile option to 
# those 3rd Party targets ONLY.
set(O3DE_COMPILE_OPTION_DISABLE_WARNINGS PRIVATE /W0)

# C++20 no longer allows to implicitly convert between enum values of different types or enum values and integral types.
# This is problematic if 3rd-party libraries use such operations in header files.
set(O3DE_COMPILE_OPTION_DISABLE_DEPRECATED_ENUM_ENUM_CONVERSION PRIVATE /Wv:18)

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

