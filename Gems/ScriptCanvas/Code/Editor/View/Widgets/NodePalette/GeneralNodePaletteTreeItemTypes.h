/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Widgets/NodePalette/TreeItems/DraggableNodePaletteTreeItem.h>

#include "CreateNodeMimeEvent.h"
#include "NodePaletteModel.h"
#include <Source/Translation/TranslationBus.h>
#include "TranslationGeneration.h"

namespace ScriptCanvasEditor
{
    struct GlobalMethodNodeModelInformation;

    // <ClassMethod>
    class CreateClassMethodMimeEvent
        : public CreateNodeMimeEvent
    {
    public:
        AZ_RTTI(CreateClassMethodMimeEvent, "{20641353-0513-4399-97D4-5509377BF0C8}", CreateNodeMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateClassMethodMimeEvent, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* reflectContext);

        CreateClassMethodMimeEvent() = default;
        CreateClassMethodMimeEvent(const QString& className, const QString& methodName, bool isOverload, ScriptCanvas::PropertyStatus);
        ~CreateClassMethodMimeEvent() = default;

    protected:
        ScriptCanvasEditor::NodeIdPair CreateNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId) const override;

    private:
        bool m_isOverload = false;
        AZStd::string m_className;
        AZStd::string m_methodName;
        ScriptCanvas::PropertyStatus m_propertyStatus = ScriptCanvas::PropertyStatus::None;
    };

    class ClassMethodEventPaletteTreeItem
        : public GraphCanvas::DraggableNodePaletteTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(ClassMethodEventPaletteTreeItem, AZ::SystemAllocator);
        AZ_RTTI(ClassMethodEventPaletteTreeItem, "{96F93970-F38A-4F08-8DC5-D52FCCE34E25}", GraphCanvas::DraggableNodePaletteTreeItem);

        ClassMethodEventPaletteTreeItem(AZStd::string_view className, AZStd::string_view methodName, bool isOverload, ScriptCanvas::PropertyStatus propertyStatus);
        ~ClassMethodEventPaletteTreeItem() = default;

        GraphCanvas::GraphCanvasMimeEvent* CreateMimeEvent() const override;

        AZStd::string GetClassMethodName() const;
        AZStd::string GetMethodName() const;
        bool IsOverload() const;
        ScriptCanvas::PropertyStatus GetPropertyStatus() const;

        AZ::IO::Path GetTranslationDataPath() const override;
        void GenerateTranslationData() override;

    private:
        bool m_isOverload = false;
        QString m_className;
        QString m_methodName;
        ScriptCanvas::PropertyStatus m_propertyStatus = ScriptCanvas::PropertyStatus::None;
    };
    // </ClassMethod>

    //! <GlobalMethod>
    //! Mime Event associated with Behavior Context Method nodes
    class CreateGlobalMethodMimeEvent
        : public CreateNodeMimeEvent
    {
    public:
        AZ_RTTI(CreateGlobalMethodMimeEvent, "{5225DC7E-FF12-4D17-8B72-D772D44DFFA0}", CreateNodeMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateGlobalMethodMimeEvent, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* reflectContext);

        CreateGlobalMethodMimeEvent() = default;
        CreateGlobalMethodMimeEvent(AZStd::string methodName, bool isProperty);

    protected:
        ScriptCanvasEditor::NodeIdPair CreateNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId) const override;

    private:
        AZStd::string m_methodName;
        bool m_isProperty;
    };

    //! GlobalMethod Node Palette Tree Item
    //! This represents a tree item that can be used to create a ScriptCanvas node
    //! out of a method reflected BehaviorContext instance(i.e a Global Method)
    class GlobalMethodEventPaletteTreeItem
        : public GraphCanvas::DraggableNodePaletteTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(GlobalMethodEventPaletteTreeItem, AZ::SystemAllocator);
        AZ_RTTI(GlobalMethodEventPaletteTreeItem, "{C6C633FC-605B-4161-8FAE-65E4807D1147}", GraphCanvas::DraggableNodePaletteTreeItem);

        GlobalMethodEventPaletteTreeItem(const GlobalMethodNodeModelInformation& nodeModelInformation);

        GraphCanvas::GraphCanvasMimeEvent* CreateMimeEvent() const override;

        const AZStd::string& GetMethodName() const;

        AZ::IO::Path GetTranslationDataPath() const override;
        void GenerateTranslationData() override;


    private:
        AZStd::string m_methodName;
        bool m_isProperty;
    };
    //! <GlobalMethod>

    // <CustomNode>
    class CreateCustomNodeMimeEvent
        : public CreateNodeMimeEvent
    {
    public:
        AZ_RTTI(CreateCustomNodeMimeEvent, "{7130C89B-2F2D-493F-AA5C-8B72968D4200}", CreateNodeMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateCustomNodeMimeEvent, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* reflectContext);

        CreateCustomNodeMimeEvent() = default;
        CreateCustomNodeMimeEvent(const AZ::Uuid& typeId);
        CreateCustomNodeMimeEvent(const AZ::Uuid& typeId, const AZStd::string& styleOverride, const AZStd::string& titlePalette);
        ~CreateCustomNodeMimeEvent() = default;

    protected:
        ScriptCanvasEditor::NodeIdPair CreateNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId) const override;

    private:
        AZ::Uuid m_typeId = AZ::Uuid::CreateNull();
        AZStd::string m_styleOverride;
        AZStd::string m_titlePalette;
    };

    class CustomNodePaletteTreeItem
        : public GraphCanvas::DraggableNodePaletteTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(CustomNodePaletteTreeItem, AZ::SystemAllocator);
        AZ_RTTI(CustomNodePaletteTreeItem, "{50E75C4D-F59C-4AF6-A6A3-5BAD557E335C}", GraphCanvas::DraggableNodePaletteTreeItem);

        explicit CustomNodePaletteTreeItem(const ScriptCanvasEditor::CustomNodeModelInformation&);
        ~CustomNodePaletteTreeItem() = default;

        GraphCanvas::GraphCanvasMimeEvent* CreateMimeEvent() const override;

        AZ::Uuid GetTypeId() const;
        const ScriptCanvasEditor::CustomNodeModelInformation& GetInfo() const { return m_info; }

        AZ::IO::Path GetTranslationDataPath() const override;
        void GenerateTranslationData() override;

    private:
        AZ::Uuid m_typeId;
        ScriptCanvasEditor::CustomNodeModelInformation m_info;
    };

    // </CustomNode>
}
