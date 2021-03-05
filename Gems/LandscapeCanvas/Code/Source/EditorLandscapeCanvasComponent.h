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

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <GraphModel/Model/Graph.h>

namespace LandscapeCanvas
{
    static const char* EditorLandscapeCanvasComponentTypeId = "{A3E4EBB8-DAC1-4D59-A9CD-64D6DA2F79F7}";

    class EditorLandscapeCanvasComponent
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(EditorLandscapeCanvasComponent, EditorLandscapeCanvasComponentTypeId, AzToolsFramework::Components::EditorComponentBase);
        virtual ~EditorLandscapeCanvasComponent() = default;

        static void Reflect(AZ::ReflectContext* reflection);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::Components::EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        //////////////////////////////////////////////////////////////////////////

        GraphModel::Graph m_graph;

    protected:
        AZ::Crc32 OnOpenGraphButtonClicked();
        AZStd::string GetOpenGraphButtonText() const;
    };
}
