/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>

namespace AZ::DocumentPropertyEditor
{
    //! BasicAdapter is a DocumentAdapter that simply provides a fixed DOM provided
    //! by a call to SetContents. The consumer is responsible for listening to change
    //! and reset events and handling them accordingly to provide interactivity.
    class BasicAdapter : public DocumentAdapter
    {
    public:
        //! Resets the contents of this adapter with a new DOM.
        void SetContents(Dom::Value contents);
        Dom::PatchOutcome RequestContentChange(const Dom::Patch& patch);

    private:
        Dom::Value GenerateContents() override;
        Dom::Value m_value;
    };
} // namespace AZ::DocumentPropertyEditor
