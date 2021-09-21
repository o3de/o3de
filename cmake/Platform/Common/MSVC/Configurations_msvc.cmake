#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(minimum_supported_toolset 142)
if(MSVC_TOOLSET_VERSION VERSION_LESS ${minimum_supported_toolset})
    message(FATAL_ERROR "MSVC toolset ${MSVC_TOOLSET_VERSION} is too old, minimum supported toolset is ${minimum_supported_toolset}")
endif()
unset(minimum_supported_toolset)

include(cmake/Platform/Common/Configurations_common.cmake)
include(cmake/Platform/Common/MSVC/VisualStudio_common.cmake)

# Verify that it wasn't invoked with an unsupported target/host architecture. Currently only supports x64/x64
if(CMAKE_VS_PLATFORM_NAME AND NOT CMAKE_VS_PLATFORM_NAME STREQUAL "x64")
    message(FATAL_ERROR "${CMAKE_VS_PLATFORM_NAME} target architecture is not supported, it must be 'x64'")
endif()
if(CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE AND NOT CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE STREQUAL "x64")
    message(FATAL_ERROR "${CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE} host toolset is not supported, it must be 'x64'")
endif()

ly_append_configurations_options(
    DEFINES
        _ENABLE_EXTENDED_ALIGNED_STORAGE # Enables support for extended alignment for the MSVC std::aligned_storage class
    COMPILATION
        /fp:fast        # allows the compiler to reorder, combine, or simplify floating-point operations to optimize floating-point code for speed and space
        /Gd             # Use _cdecl calling convention for all functions
        /MP             # Multicore compilation in Visual Studio
        /nologo         # Suppress Copyright and version number message
        /W4             # Warning level 4
        /WX             # Warnings as errors
        /permissive-    # Conformance with standard
        
        ###################
        # Disabled warnings (please do not disable any others without first consulting sig-build)
        ###################
        /wd4201 # nonstandard extension used: nameless struct/union. This actually became part of the C++11 std, MS has an open issue: https://developercommunity.visualstudio.com/t/warning-level-4-generates-a-bogus-warning-c4201-no/103064

        ###################
        # Enabled warnings (that are disabled by default from /W4)
        ###################
        # https://docs.microsoft.com/en-us/cpp/preprocessor/compiler-warnings-that-are-off-by-default?view=vs-2019
        /we4263 # 'function': member function does not override any base class virtual member function
        /we4264 # 'virtual_function': no override available for virtual member function from base 'class'; function is hidden
        /we4265 # 'class': class has virtual functions, but destructor is not virtual
        /we4266 # 'function': no override available for virtual member function from base 'type'; function is hidden
        /we4296 # 'operator': expression is always false
        /we4426 # optimization flags changed after including header, may be due to #pragma optimize()
        /we4437 # dynamic_cast from virtual base 'class1' to 'class2' could fail in some contexts
        #/we4619 # #pragma warning: there is no warning number 'number'. Unfortunately some versions of MSVC 16.X dont filter this warning coming from external headers and Qt has a bad warning in QtCore/qvector.h(340,12)
        /we4774 # 'string' : format string expected in argument number is not a string literal
        /we4777 # 'function' : format string 'string' requires an argument of type 'type1', but variadic argument number has type 'type2
        /we5031 # #pragma warning(pop): likely mismatch, popping warning state pushed in different file
        /we5032 # detected #pragma warning(push) with no corresponding #pragma warning(pop)
        /we5233 # explicit lambda capture 'identifier' is not used

        /Zc:forScope    # Force Conformance in for Loop Scope
        /diagnostics:caret # Compiler diagnostic options: includes the column where the issue was found and places a caret (^) under the location in the line of code where the issue was detected.
        /Zc:__cplusplus
        /Zc:lambda      # Use the new lambda processor (See https://developercommunity.visualstudio.com/t/A-lambda-that-binds-the-this-pointer-w/1467873 for more details)
        /favor:AMD64    # Create Code optimized for 64 bit
        /bigobj         # Increase number of sections in obj files. Profiling has shown no meaningful impact in memory nore build times
    COMPILATION_DEBUG
        /GS             # Enable Buffer security check
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

set(LY_BUILD_WITH_ADDRESS_SANITIZER FALSE CACHE BOOL "Builds using AddressSanitizer (ASan). Will disable Edit/Continue, Incremental building and Run-Time checks (default = FALSE)")
if(LY_BUILD_WITH_ADDRESS_SANITIZER)
    set(LY_BUILD_WITH_INCREMENTAL_LINKING_DEBUG FALSE) 
    ly_append_configurations_options(
        COMPILATION_DEBUG
            /fsanitize=address
    )
    get_filename_component(link_tools_dir ${CMAKE_LINKER} DIRECTORY)
    file(COPY
        ${link_tools_dir}/clang_rt.asan_dbg_dynamic-x86_64.dll
        DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG})
else()
    ly_append_configurations_options(
        COMPILATION_DEBUG
            /RTCsu  # Run-Time Error Checks: c Reports when a value is assigned to a smaller data type and results in a data loss (Not supoported by the STL)
                    #                        s Enables stack frame run-time error checking
                    #                        u Reports when a variable is used without having been initialized
    )
endif()

set(LY_BUILD_WITH_INCREMENTAL_LINKING_DEBUG FALSE CACHE BOOL "Indicates if incremental linking is used in debug configurations (default = FALSE)")
if(LY_BUILD_WITH_INCREMENTAL_LINKING_DEBUG)
    ly_append_configurations_options(
        COMPILATION_DEBUG
            /ZI         # Enable Edit/Continue
    )
else()
    # Disable incremental linking
    ly_append_configurations_options(
        COMPILATION_DEBUG
            /Zi         # Generate debugging information (no Edit/Continue). Edit/Continue requires incremental linking
        LINK_NON_STATIC_DEBUG
            /DEBUG      # Despite the documentation states /Zi implies /DEBUG, without it, stack traces are not expanded
            /INCREMENTAL:NO

    )    
endif()

# Configure system includes
ly_set(LY_CXX_SYSTEM_INCLUDE_CONFIGURATION_FLAG 
    /experimental:external # Turns on "external" headers feature for MSVC compilers
    /external:W0 # Set warning level in external headers to 0. This is used to suppress warnings 3rdParty libraries which uses the "system_includes" option in their json configuration
)
if(NOT CMAKE_INCLUDE_SYSTEM_FLAG_CXX)
    ly_set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX /external:I)
endif()

include(cmake/Platform/Common/TargetIncludeSystemDirectories_unsupported.cmake)
