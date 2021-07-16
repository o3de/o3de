/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "GraphCanvas/Components/NodePropertyDisplay/NodePropertyDisplay.h"

namespace GraphCanvas
{
    class DataInterface
    {
    public:
    
        DataInterface()
            : m_display(nullptr)
        {
        }

        virtual ~DataInterface() = default;

        void RegisterDisplay(NodePropertyDisplay* display)
        {
            if (m_display == nullptr)
            {
                m_display = display;
            }
        }

        virtual void SignalValueChanged()
        {
            if (m_display != nullptr)
            {
                m_display->UpdateDisplay();
            }
        }
        
        virtual bool EnableDropHandling() const
        {
            return false;
        }
        
        // Outcome signifies whether or not the data is recognized and could be handled.
        // The DragDropState determines how the recognized data is handled for visual feedback.
        virtual AZ::Outcome<DragDropState> ShouldAcceptMimeData(const QMimeData* mimeData)
        {
            AZ_UNUSED(mimeData);
            return AZ::Failure();
        }

        virtual bool HandleMimeData(const QMimeData* mimeData)
        {
            AZ_UNUSED(mimeData);
            return false;
        }

    protected:

        const NodePropertyDisplay* GetDisplay() const
        {
            return m_display;
        }
        
    private:
    
        NodePropertyDisplay*  m_display;
    };
}
