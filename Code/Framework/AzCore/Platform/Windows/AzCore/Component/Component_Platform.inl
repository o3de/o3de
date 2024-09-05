/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Clang on Windows uses the preprocessor in MSVC compatibility mode, which cannot parse the AZ_VA_OPT macro in some instances, but
// since the comma after __VA_ARGS__ is removed automatically if __VA_ARGS__ is empty, it does not have to be used.
#if defined(AZ_COMPILER_CLANG)
#define AZ_RTTI_NO_TYPE_INFO_IMPL_VA_OPT_HELPER(_ComponentClassOrTemplate, _BaseClass, ...) \
    AZ_RTTI_NO_TYPE_INFO_IMPL(_ComponentClassOrTemplate, __VA_ARGS__, _BaseClass)
#endif
