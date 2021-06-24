/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <LocalUserSystemComponent.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace LocalUser
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    LocalUserSystemComponent::Implementation* LocalUserSystemComponent::Implementation::Create(LocalUserSystemComponent&)
    {
        return nullptr;
    }
} // namespace LocalUser
