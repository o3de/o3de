/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace LmbrCentral
{
    /// Editor specific TubeShapeComponent requests.
    class EditorTubeShapeComponentRequests
        : public AZ::ComponentBus
    {
    public:
         /// Request the EditorTubeShapeComponent to regenerate its vertices.
         /// Recreate vertex and index buffers to update the visual representation
         /// of the tube to match the internals state.
        virtual void GenerateVertices() = 0;

    protected:
        ~EditorTubeShapeComponentRequests() = default;
    };

    /// Type to inherit to provide EditorTubeShapeComponentRequests
    using EditorTubeShapeComponentRequestBus = AZ::EBus<EditorTubeShapeComponentRequests>;
} // namespace LmbrCentral