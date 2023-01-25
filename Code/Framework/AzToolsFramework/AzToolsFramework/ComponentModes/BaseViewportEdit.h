/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzToolsFramework
{
    //! Base class for objects used in sub component modes which use viewport editing.
    class BaseViewportEdit
    {
    public:
        virtual ~BaseViewportEdit() = default;

        virtual void Setup(const AZ::EntityComponentIdPair& entityComponentIdPair) = 0;
        virtual void Teardown() = 0;
        virtual void UpdateManipulators() = 0;
        virtual void ResetValues() = 0;
    };
} // namespace AzToolsFramework
