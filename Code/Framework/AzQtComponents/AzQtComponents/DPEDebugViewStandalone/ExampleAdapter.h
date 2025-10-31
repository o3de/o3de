/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>
#include <AzCore/Math/Color.h>

namespace AZ::DocumentPropertyEditor
{
    class ExampleAdapter : public DocumentAdapter
    {
    public:
        ExampleAdapter();

    protected:
        Dom::Value GenerateContents() override;

    private:
        int GetMatrixValue(int i, int j) const;
        void SetMatrixValue(int i, int j, int value);
        void ResizeMatrix();

        int m_rowCount = 0;
        int m_columnCount = 0;
        AZStd::vector<AZStd::vector<int>> m_matrix;

        struct ColorTreeNode
        {
            ColorTreeNode* m_parent = nullptr;
            AZ::Color m_color;
            AZStd::list<ColorTreeNode> m_children;
        };
        ColorTreeNode m_colorTreeRoot;
    };
} // namespace AZ::DocumentPropertyEditor
