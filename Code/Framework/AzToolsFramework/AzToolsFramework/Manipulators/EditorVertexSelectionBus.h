/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace AzToolsFramework
{
    //! Bus to request operations to the EditorVertexSelection classes.
    class EditorVertexSelectionVariableRequests : public AZ::EntityComponentBus
    {
    public:
        virtual void DuplicateSelectedVertices() = 0;
        virtual void DeleteSelectedVertices() = 0;
        virtual void ClearVertexSelection() = 0;
        virtual int GetSelectedVerticesCount() = 0;

    protected:
        ~EditorVertexSelectionVariableRequests() = default;
    };
    using EditorVertexSelectionVariableRequestBus = AZ::EBus<EditorVertexSelectionVariableRequests>;
}
