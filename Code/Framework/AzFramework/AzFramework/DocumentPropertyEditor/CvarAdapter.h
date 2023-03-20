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
    //! An adapter for displaying an editable list of CVars registered to the console instance.
    //! Supports editing CVars with primitive types, string types, and numeric vector types
    //! (VectorX, Quaternion, and Color).
    class CvarAdapter : public DocumentAdapter
    {
    public:
        CvarAdapter();
        ~CvarAdapter();

        void OnContentsChanged(const Dom::Path& path, const Dom::Value& value);

    protected:
        Dom::Value GenerateContents() override;

    private:
        struct Impl;
        AZStd::unique_ptr<Impl> m_impl;
    };
}
