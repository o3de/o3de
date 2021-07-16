/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

class IEntityObjectListener
{
public:
    virtual ~IEntityObjectListener() = default;

    virtual void OnNameChanged(const char* pName) = 0;
    virtual void OnSelectionChanged(const bool bSelected) = 0;
    virtual void OnDone() = 0;
};

