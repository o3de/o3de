/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <ITexture.h>
#include <AzTest/AzTest.h>

class ITextureMock
    : public ITexture
{
public:
    MOCK_METHOD0(AddRef,
        int());
    MOCK_METHOD0(Release,
        int());
    MOCK_METHOD0(ReleaseForce,
        int());
    MOCK_CONST_METHOD0(GetName,
        const char*());
};
