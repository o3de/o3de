/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_INCLUDE_IANIMATIONCOMPRESSIONMANAGER_H
#define CRYINCLUDE_EDITOR_INCLUDE_IANIMATIONCOMPRESSIONMANAGER_H
#pragma once

struct IAnimationCompressionManager
{
    virtual bool IsEnabled() const = 0;
    virtual void UpdateLocalAnimations() = 0;
};

#endif // CRYINCLUDE_EDITOR_INCLUDE_IANIMATIONCOMPRESSIONMANAGER_H
