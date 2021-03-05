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

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

#include <GraphCanvas/Widgets/GraphCanvasTreeItem.h>
#include <GraphCanvas/Editor/EditorTypes.h>

namespace GraphCanvas
{
    class NodePaletteTreeItem
        : public GraphCanvasTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(NodePaletteTreeItem, AZ::SystemAllocator, 0);
        AZ_RTTI(NodePaletteTreeItem, "{D1BAAF63-F823-4D2A-8F55-01AC2A659FF5}", GraphCanvas::GraphCanvasTreeItem);

        static constexpr const char* DefaultNodeTitlePalette = "DefaultNodeTitlePalette";

        enum Column
        {
            IndexForce = -1,
            Name,
            Customization,

            Count
        };

        static const int k_defaultItemOrdering;

        NodePaletteTreeItem(AZStd::string_view name, EditorId editorId);
        ~NodePaletteTreeItem() = default;
        
        const QString& GetName() const;
        int GetColumnCount() const override final;
        
        QVariant Data(const QModelIndex& index, int role) const override final;
        Qt::ItemFlags Flags(const QModelIndex& index) const override final;

        void SetToolTip(const QString& toolTip);

        void SetItemOrdering(int ordering);

        void SetStyleOverride(const AZStd::string& styleOverride);
        const AZStd::string& GetStyleOverride() const;

        void SetTitlePalette(const AZStd::string& palette, bool force = false);
        const AZStd::string& GetTitlePalette() const;

        // General Purpose flags for passing along state from the TreeView into the items
        void SetHovered(bool hovered);
        bool IsHovered() const;

        void SetSelected(bool selected);
        bool IsSelected() const;

        void SetEnabled(bool enabled);
        bool IsEnabled() const;

        void SetHighlight(const AZStd::pair<int, int>& highlight);
        bool HasHighlight() const;

        const AZStd::pair<int, int>& GetHighlight() const;
        void ClearHighlight();

        void SignalClicked(int row);
        bool SignalDoubleClicked(int row);

    protected:

        void PreOnChildAdded(GraphCanvasTreeItem* item) override;
        
        void SetName(const QString& name);

        const EditorId& GetEditorId() const;

        // Child Overrides
        virtual bool LessThan(const GraphCanvasTreeItem* graphItem) const;
        virtual QVariant OnData(const QModelIndex& index, int role) const;
        virtual Qt::ItemFlags OnFlags() const;

        virtual void OnStyleOverrideChange();
        virtual void OnTitlePaletteChanged();

        virtual void OnHoverStateChanged();
        virtual void OnSelectionStateChanged();
        virtual void OnEnabledStateChanged();

        virtual void OnClicked(int row);
        virtual bool OnDoubleClicked(int row);
        ////

    private:

        AZStd::string m_styleOverride;
        AZStd::string m_palette;

        EditorId m_editorId;

        QString m_name;
        QString m_toolTip;

        bool m_selected;
        bool m_hovered;
        bool m_enabled;

        AZStd::pair<int, int> m_highlight;

        int m_ordering;
    };
    
    
}
