/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/typetraits/config.h>

/**
 * \page TypeTraits Type Traits
 *
 * One of our goals is to be compatible with \ref C++14 (which by the time you read it it may be in the standard).
 */

/**
 * \defgroup TypeTraitsDefines Type Traits Defines
 * @{
 *
 */
#include <AzCore/std/typetraits/add_const.h>
#include <AzCore/std/typetraits/add_cv.h>
#include <AzCore/std/typetraits/add_pointer.h>
#include <AzCore/std/typetraits/add_reference.h>
#include <AzCore/std/typetraits/add_volatile.h>
#include <AzCore/std/typetraits/alignment_of.h>
#include <AzCore/std/typetraits/aligned_storage.h>
#include <AzCore/std/typetraits/conditional.h>
#include <AzCore/std/typetraits/conjunction.h>
#include <AzCore/std/typetraits/decay.h>
#include <AzCore/std/typetraits/disjunction.h>
#include <AzCore/std/typetraits/function_traits.h>
#include <AzCore/std/typetraits/has_member_function.h>
#include <AzCore/std/typetraits/has_virtual_destructor.h>
#include <AzCore/std/typetraits/is_signed.h>
#include <AzCore/std/typetraits/is_unsigned.h>
#include <AzCore/std/typetraits/is_abstract.h>
#include <AzCore/std/typetraits/is_arithmetic.h>
#include <AzCore/std/typetraits/is_assignable.h>
#include <AzCore/std/typetraits/is_array.h>
#include <AzCore/std/typetraits/is_base_of.h>
#include <AzCore/std/typetraits/is_class.h>
#include <AzCore/std/typetraits/is_compound.h>
#include <AzCore/std/typetraits/is_const.h>
#include <AzCore/std/typetraits/is_convertible.h>
#include <AzCore/std/typetraits/is_constructible.h>
#include <AzCore/std/typetraits/is_destructible.h>
#include <AzCore/std/typetraits/is_empty.h>
#include <AzCore/std/typetraits/is_enum.h>
#include <AzCore/std/typetraits/is_floating_point.h>
#include <AzCore/std/typetraits/is_function.h>
#include <AzCore/std/typetraits/is_fundamental.h>
#include <AzCore/std/typetraits/is_integral.h>
#include <AzCore/std/typetraits/is_member_function_pointer.h>
#include <AzCore/std/typetraits/is_member_object_pointer.h>
#include <AzCore/std/typetraits/is_member_pointer.h>
#include <AzCore/std/typetraits/is_object.h>
#include <AzCore/std/typetraits/is_pod.h>
#include <AzCore/std/typetraits/is_polymorphic.h>
#include <AzCore/std/typetraits/is_pointer.h>
#include <AzCore/std/typetraits/is_reference.h>
#include <AzCore/std/typetraits/is_lvalue_reference.h>
#include <AzCore/std/typetraits/is_rvalue_reference.h>
#include <AzCore/std/typetraits/is_same.h>
#include <AzCore/std/typetraits/is_scalar.h>
#include <AzCore/std/typetraits/is_swappable.h>
#include <AzCore/std/typetraits/is_trivial.h>
#include <AzCore/std/typetraits/is_union.h>
#include <AzCore/std/typetraits/is_void.h>
#include <AzCore/std/typetraits/is_volatile.h>
#include <AzCore/std/typetraits/negation.h>
#include <AzCore/std/typetraits/rank.h>
#include <AzCore/std/typetraits/extent.h>
#include <AzCore/std/typetraits/remove_extent.h>
#include <AzCore/std/typetraits/remove_all_extents.h>
#include <AzCore/std/typetraits/remove_const.h>
#include <AzCore/std/typetraits/remove_cvref.h>
#include <AzCore/std/typetraits/remove_pointer.h>
#include <AzCore/std/typetraits/remove_reference.h>
#include <AzCore/std/typetraits/remove_volatile.h>
#include <AzCore/std/typetraits/type_identity.h>
#include <AzCore/std/typetraits/underlying_type.h>
#include <AzCore/std/typetraits/void_t.h>

