#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Explicitly setting CMAKE_SYSTEM_NAME forces CMake to use cross compiling mode
# Even if it is set to the current platform. See the CMAKE_CROSSCOMPILING var documentation
# https://cmake.org/cmake/help/latest/variable/CMAKE_CROSSCOMPILING.html#cmake-crosscompiling
set(CMAKE_SYSTEM_NAME ${CMAKE_HOST_SYSTEM_NAME})
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)

# Make sure that the try compile step doesn't try to run the application it builds
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

list(APPEND CMAKE_C_COMPILE_FEATURES
    c_std_90
    c_std_99
    c_std_11
    c_std_17
    c_function_prototypes
    c_restrict
    c_static_assert
    c_variadic_macros
)

list(APPEND CMAKE_CXX_COMPILE_FEATURES
    cxx_std_98
    cxx_std_11
    cxx_std_14
    cxx_std_17
    #C++ 98: https://cmake.org/cmake/help/latest/prop_gbl/CMAKE_CXX_KNOWN_FEATURES.html#individual-features-from-c-98
    cxx_template_template_parameters
    #C++ 11: https://cmake.org/cmake/help/latest/prop_gbl/CMAKE_CXX_KNOWN_FEATURES.html#individual-features-from-c-11
    cxx_alias_templates
    cxx_alignas
    cxx_alignof
    cxx_attributes
    cxx_auto_type
    cxx_constexpr
    cxx_decltype_incomplete_return_types
    cxx_decltype
    cxx_default_function_template_args
    cxx_defaulted_functions
    cxx_defaulted_move_initializers
    cxx_delegating_constructors
    cxx_deleted_functions
    cxx_enum_forward_declarations
    cxx_explicit_conversions
    cxx_extended_friend_declarations
    cxx_extern_templates
    cxx_final
    cxx_func_identifier
    cxx_generalized_initializers
    cxx_inheriting_constructors
    cxx_inline_namespaces
    cxx_lambdas
    cxx_local_type_template_args
    cxx_long_long_type
    cxx_noexcept
    cxx_nonstatic_member_init
    cxx_nullptr
    cxx_override
    cxx_range_for
    cxx_raw_string_literals
    cxx_reference_qualified_functions
    cxx_right_angle_brackets
    cxx_rvalue_references
    cxx_sizeof_member
    cxx_static_assert
    cxx_strong_enums
    cxx_thread_local
    cxx_trailing_return_types
    cxx_unicode_literals
    cxx_uniform_initialization
    cxx_unrestricted_unions
    cxx_user_literals
    cxx_variadic_macros
    cxx_variadic_templates
    # C++ 14: https://cmake.org/cmake/help/latest/prop_gbl/CMAKE_CXX_KNOWN_FEATURES.html#individual-features-from-c-14
    cxx_aggregate_default_initializers
    cxx_attribute_deprecated
    cxx_binary_literals
    cxx_contextual_conversions
    cxx_decltype_auto
    cxx_digit_separators
    cxx_generic_lambdas
    cxx_lambda_init_captures
    cxx_relaxed_constexpr
    cxx_return_type_deduction
    cxx_variable_templates
)