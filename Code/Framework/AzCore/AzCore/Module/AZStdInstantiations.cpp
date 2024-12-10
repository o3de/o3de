/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformDef.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>

namespace AZStd
{
    template class AZ_DLL_EXPORT intrusive_refcount<atomic_uint>;
}
