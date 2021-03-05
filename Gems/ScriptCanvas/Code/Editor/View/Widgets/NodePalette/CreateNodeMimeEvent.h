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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/ReflectContext.h>

#include <GraphCanvas/Widgets/MimeEvents/CreateSplicingNodeMimeEvent.h>

#include "ScriptCanvas/Bus/NodeIdPair.h"
#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvasEditor
{
    class CreateNodeMimeEvent
        : public GraphCanvas::CreateSplicingNodeMimeEvent
    {
    public:
        AZ_RTTI(CreateNodeMimeEvent, "{95C84213-1FF8-42FF-96F3-37B80B7E2C20}", GraphCanvas::CreateSplicingNodeMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateNodeMimeEvent, AZ::SystemAllocator, 0);
            
        static void Reflect(AZ::ReflectContext* reflectContext);
            
        CreateNodeMimeEvent() = default;
        ~CreateNodeMimeEvent() = default;

        const ScriptCanvasEditor::NodeIdPair& GetCreatedPair() const;

        bool ExecuteEvent(const AZ::Vector2& mouseDropPosition, AZ::Vector2& dropPosition, const AZ::EntityId& graphCanvasGraphId) override final;
        AZ::EntityId CreateSplicingNode(const AZ::EntityId& graphCanvasGraphId) override;
        
    protected:
        virtual ScriptCanvasEditor::NodeIdPair CreateNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId) const = 0;

        NodeIdPair m_nodeIdPair;
    };

    // There are a couple of cases where we have some weird construction steps that aren't captured in the CreateNodeMimeEvent
    // To deal with those cases, we want to make a specialized mime event so we can catch these cases from the context menu
    // and execute the right functions.
    class SpecializedCreateNodeMimeEvent
        : public GraphCanvas::GraphCanvasMimeEvent
    {
    public:
        AZ_RTTI(SpecializedCreateNodeMimeEvent, "{7909C855-B6DA-47E4-97DB-BBC8315C30B1}", GraphCanvas::GraphCanvasMimeEvent);
        AZ_CLASS_ALLOCATOR(SpecializedCreateNodeMimeEvent, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflectContext);

        SpecializedCreateNodeMimeEvent() = default;
        ~SpecializedCreateNodeMimeEvent() = default;

        virtual NodeIdPair ConstructNode(const AZ::EntityId& scriptCanvasGraphId, const AZ::Vector2& scenePosition) = 0;
    };

    // Special case specialization here for some automation procedures.
    // Want to be able to generate all of the possible events from a MultiCreationNode and handle
    // them all in an automated way
    class MultiCreateNodeMimeEvent
        : public SpecializedCreateNodeMimeEvent
    {
    public:
        AZ_RTTI(MultiCreateNodeMimeEvent, "{44A3F43F-E6D3-4EC7-8E80-82981661603E}", SpecializedCreateNodeMimeEvent);
        AZ_CLASS_ALLOCATOR(MultiCreateNodeMimeEvent, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflectContext);

        virtual AZStd::vector< GraphCanvas::GraphCanvasMimeEvent* > CreateMimeEvents() const = 0;
    };
}