/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "Atom/RHI/FrameGraph.h"
#include "Atom/RHI/Device.h"
#include <Atom/RHI/FrameGraphAttachmentDatabase.h>
#include <Atom/RHI/ImageScopeAttachment.h>
#include <Atom/RHI/BufferScopeAttachment.h>
namespace ImGui
{
    #ifndef SAFE_DELETE
    #define SAFE_DELETE(p){if(p){delete p;p=nullptr;}}
    #endif
    #ifndef ImGuiMouseButton_Left
        #define ImGuiMouseButton_Left 0
    #endif
    #ifndef ImGuiMouseButton_Right
        #define ImGuiMouseButton_Right 1
    #endif
    #ifndef ImGuiMouseButton_Middle
        #define ImGuiMouseButton_Middle 2
    #endif
    inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
    inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }
    enum class ImGuiFrameVisualizerFieldType
    {
        Invalid = -1,
        Text,
    };
    
    //!Visible fields withing an node.
    class ImGuiFrameVisualizerField
    {
    public:
        ImGuiFrameVisualizerField() = default;
        virtual ~ImGuiFrameVisualizerField() = default;
        virtual ImGuiFrameVisualizerFieldType GetFieldType()
        {
            return ImGuiFrameVisualizerFieldType::Invalid;
        }
    protected:
        ImGuiFrameVisualizerFieldType m_fieldType = ImGuiFrameVisualizerFieldType::Invalid;
    };

    //!Node text visible field.
    class ImGuiFrameVisualizerTextField :public ImGuiFrameVisualizerField
    {
    public:
        ImGuiFrameVisualizerTextField() = default;
        ~ImGuiFrameVisualizerTextField() override = default;
        ImGuiFrameVisualizerTextField(const AZStd::string& text):
        m_text(text)
        {
        }
        ImGuiFrameVisualizerFieldType GetFieldType() override
        {
            return ImGuiFrameVisualizerFieldType::Text;
        }
        const  AZStd::string& GetString()
        {
            return m_text;
        }
    private:
        AZStd::string m_text;
    };


    class ImGuiFrameVisualizerWindow;


    //!Node definition.
    class ImGuiFrameVisualizerNode
    {
    public:
        friend class ImGuiFrameVisualizerWindow;
        ImGuiFrameVisualizerNode(
            ImGuiFrameVisualizerNode* parent,
            AZStd::string name,
            ImVec2 position,
            int inputCounts,
            int outputCounts):
            m_parent(parent),
            m_name(name),
            m_position(position),
            m_inputCounts(inputCounts),
            m_outputCount(outputCounts)
        {
            AutoSize();
        }

        //!Add an new child node.
        ImGuiFrameVisualizerNode* AddChild(const AZStd::string& name, const int inputCount, const int outputCount)
        {
            const size_t childIndex = m_childrens.size();
            const float nodeYOffset = 8.0f;
            const float nodeXOffset = 10.0f;
            float nodeYPosition = (m_size.y + nodeYOffset) * childIndex;
            if (childIndex > 0)
            {
                ImVec2 mMin;
                ImVec2 mMax;
                ImGuiFrameVisualizerNode* previousChild = m_childrens[childIndex - 1];
                previousChild->GetAABBHierachy(mMin, mMax);
                nodeYPosition = (mMax.y + nodeYOffset) - m_position.y;
            }
            ImGuiFrameVisualizerNode* node = new ImGuiFrameVisualizerNode(this, name, m_position+ ImVec2(m_size.x + nodeXOffset, nodeYPosition), inputCount, outputCount);
            return AddChild(node);
        }

        //!Remove an child node.
        void RemoveChild(ImGuiFrameVisualizerNode* node)
        {
            const size_t memoryAddress = reinterpret_cast<size_t>(node);
            auto ittFind = m_removableNodes.find(memoryAddress);
            if (ittFind != m_removableNodes.end())
            {
                const size_t arrayOffset = ittFind->second;
                m_childrens.erase(m_childrens.begin() + arrayOffset);
                m_removableNodes.erase(ittFind);
                return;
            }
        }

        //!Clear all the child nodes.
        void ClearChildren()
        {
            const size_t numChildrens = m_childrens.size();
            for (size_t index = 0; index < numChildrens; ++index)
            {
                ImGuiFrameVisualizerNode* currentNode = m_childrens[index];
                currentNode->ClearChildren();
                const size_t numFields = currentNode->m_fields.size();
                for (size_t i = 0; i < numFields; ++i)
                {
                    ImGuiFrameVisualizerField* currentField = currentNode->m_fields[i];
                    SAFE_DELETE(currentField);
                }
                currentNode->m_fields.clear();
                currentNode->m_childrens.clear();
                currentNode->m_removableNodes.clear();
                SAFE_DELETE(currentNode);
            }
        }

        //!Add an text field to the node.
        void AddTextField(const AZStd::string& text)
        {
            ImGuiFrameVisualizerField* field = new ImGuiFrameVisualizerTextField(text);
            m_fields.push_back(field);
            AutoSize();
        }

        //!Get input slot position based on slot number.
        ImVec2 GetInputSlotPosition(const unsigned int slotNo)
        {
            const float slotLevel = static_cast<float>(slotNo + 1) / static_cast<float>(m_inputCounts + 1);
            return ImVec2(m_position.x, m_position.y + m_size.y * slotLevel);
        }

        //!Get output slot position based on slot number.
        ImVec2 GetOutputSlotPosition(const unsigned int slotNo)
        {
            const float slotLevel = static_cast<float>(slotNo + 1) / static_cast<float>(m_outputCount + 1);
            return ImVec2(m_position.x + m_size.x, m_position.y + m_size.y * slotLevel);
        }
    private:

        //!Add an new child node.
        ImGuiFrameVisualizerNode* AddChild(ImGuiFrameVisualizerNode* node)
        {
            const size_t memoryAddress = (size_t)node;
            auto ittFind = m_removableNodes.find(memoryAddress);
            if (ittFind == m_removableNodes.end())
            {
                const size_t arrayOffset = m_childrens.size();
                m_childrens.push_back(node);
                m_removableNodes[(size_t)node] = arrayOffset;
            }
            return node;
        }

        //!Paint the link bezier curve.
        void PaintLink(ImGuiFrameVisualizerNode* node, const ImVec2& offset)
        {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            const size_t numChildrens = node->m_childrens.size();
            for (size_t index = 0; index < numChildrens; ++index)
            {
                ImGuiFrameVisualizerNode* childNode = node->m_childrens[index];
                PaintLink(childNode, offset);
            }
            draw_list->ChannelsSetCurrent(0);
            const ImVec2 p1 = offset + node->GetOutputSlotPosition(0);
            for (size_t index = 0; index < numChildrens; ++index)
            {
                const ImVec2 p2 = offset+node->m_childrens[index]->GetInputSlotPosition(0);
                draw_list->AddBezierCurve(p1, p1 + ImVec2(50, 0), p2 + ImVec2(-50, 0), p2, IM_COL32(200, 200, 100, 255), 3.0f);
            }
        }

        //!Compute the size of node based on how many
        //!items are in it.
        void AutoSize()
        {
            AutoSize(this);
        }

        //!Compute the size of node based on how many
        //!items are in it.
        void AutoSize(ImGuiFrameVisualizerNode* node)
        {
            ImGui::BeginGroup();
            ImGui::Text("%s", node->m_name.c_str());
            const size_t numFields = node->m_fields.size();
            for (size_t index = 0; index < numFields; ++index)
            {
                ImGuiFrameVisualizerField* currentField = node->m_fields[index];
                ImGuiFrameVisualizerFieldType fieldType = currentField->GetFieldType();
                switch (fieldType)
                {
                case ImGuiFrameVisualizerFieldType::Text:
                {
                    ImGuiFrameVisualizerTextField* textField = (ImGuiFrameVisualizerTextField*)currentField;
                    ImGui::Text("%s", textField->GetString().c_str());
                }
                break;
                }
            }
            ImGui::EndGroup();
            node->m_size = ImGui::GetItemRectSize();
            const size_t numChildrens = node->m_childrens.size();
            for (size_t index = 0; index < numChildrens; ++index)
            {
                ImGuiFrameVisualizerNode* childNode = node->m_childrens[index];
                AutoSize(childNode);
            }
        }

        //!Get the aabb of all subTreeNodes.
        void GetAABBHierachy(ImVec2& mMin, ImVec2& mMax)
        {
            mMin.x = 1e6f;
            mMin.y = 1e6f;
            mMax.x = -1e6f;
            mMax.y = -1e6f;
            GetAABBHierachy(this, mMin, mMax);
        }

        //!Get the aabb of all subTreeNodes helper function.
        void GetAABBHierachy(ImGuiFrameVisualizerNode* node ,ImVec2& mMin, ImVec2& mMax)
        {
            mMin.x = ImMin(mMin.x, node->m_position.x);
            mMin.y = ImMin(mMin.y, node->m_position.y);
            mMax.x = ImMax(mMax.x, node->m_position.x + node->m_size.x);
            mMax.y = ImMax(mMax.y, node->m_position.y + node->m_size.y);
            const size_t numChildrens = node->m_childrens.size();
            for (size_t index = 0; index < numChildrens; ++index)
            {
                ImGuiFrameVisualizerNode* childNode = node->m_childrens[index];
                GetAABBHierachy(childNode, mMin, mMax);
            }
        }

        //!Compute the bounding box of the node
        void GetAABB(ImVec2& mMin,ImVec2& mMax)
        {
            mMin.x = 1e6f;
            mMin.y = 1e6f;
            mMax.x = -1e6f;
            mMax.y = -1e6f;
            ImGuiFrameVisualizerNode* node = this;

            mMin.x = ImMin(mMin.x, node->m_position.x);
            mMin.y = ImMin(mMin.y, node->m_position.y);
            mMax.x = ImMax(mMax.x, node->m_position.x + node->m_size.x);
            mMax.y = ImMax(mMax.y, node->m_position.y + node->m_size.y);

            const size_t numChildrens = node->m_childrens.size();
            for (size_t index = 0; index < numChildrens; ++index)
            {
                ImGuiFrameVisualizerNode* childNode = node->m_childrens[index];
                mMin.x = ImMin(mMin.x, childNode->m_position.x);
                mMin.y = ImMin(mMin.y, childNode->m_position.y);
                mMax.x = ImMax(mMax.x, childNode->m_position.x + childNode->m_size.x);
                mMax.y = ImMax(mMax.y, childNode->m_position.y + childNode->m_size.y);
            }
        }

        //!Draw the current node on the active window.
        void Paint(ImGuiFrameVisualizerNode* node, const ImVec2& offset,unsigned int& nodeID)
        {
            ImGuiIO& io = ImGui::GetIO();

            const float nodeSlotRadius = 4.0f;
            const ImVec2 nodeWindowPadding(0.0f, 0.0f);

            ImDrawList* drawList = ImGui::GetWindowDrawList();

            ImGui::PushID(nodeID);
            ImVec2 nodeRectMin = offset + node->m_position;

            drawList->ChannelsSetCurrent(1);
            ImGui::SetCursorScreenPos(nodeRectMin);
            ImGui::BeginGroup();
            ImGui::Text("%s", node->m_name.c_str());
            const size_t numFields = node->m_fields.size();
            for (size_t index = 0; index < numFields; ++index)
            {
                ImGuiFrameVisualizerField* currentField = node->m_fields[index];
                ImGuiFrameVisualizerFieldType fieldType = currentField->GetFieldType();
                switch (fieldType)
                {
                case ImGuiFrameVisualizerFieldType::Text:
                {
                    ImGuiFrameVisualizerTextField* textField = static_cast<ImGuiFrameVisualizerTextField*>(currentField);
                    ImGui::Text("%s", textField->GetString().c_str());
                }
                break;
                }
            }
            ImGui::EndGroup();

            node->m_size = ImGui::GetItemRectSize();
            ImVec2 node_rect_max = nodeRectMin + node->m_size;


            drawList->ChannelsSetCurrent(0);
            ImGui::SetCursorScreenPos(nodeRectMin);
            ImGui::InvisibleButton("node", node->m_size);

            bool nodeActive = ImGui::IsItemActive();
            if(nodeActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            {
                node->m_position = node->m_position+ io.MouseDelta;
            }
            ImU32 nodeBackgroundColor = IM_COL32(127, 127, 127, 255);
            drawList->AddRectFilled(nodeRectMin, node_rect_max, nodeBackgroundColor, 4.0f);
            drawList->AddRect(nodeRectMin, node_rect_max, IM_COL32(255, 0, 0, 255), 4.0f);

            for (int slotIDIndex = 0; slotIDIndex < node->m_inputCounts; slotIDIndex++)
            {
                drawList->AddCircleFilled(offset + node->GetInputSlotPosition(slotIDIndex), nodeSlotRadius, IM_COL32(150, 150, 150, 150));
            }
            for (int slotIDIndex = 0; slotIDIndex < node->m_outputCount; slotIDIndex++)
            {
                drawList->AddCircleFilled(offset + node->GetOutputSlotPosition(slotIDIndex), nodeSlotRadius, IM_COL32(150, 150, 150, 150));
            }
            ImGui::PopID();

            const size_t numChildrens = node->m_childrens.size();
            for (size_t index = 0; index < numChildrens; ++index)
            {
                ImGuiFrameVisualizerNode* childNode = node->m_childrens[index];
                Paint(childNode, offset, ++nodeID);
            }
        }

        //!Resolved overlapping nodes.
        void ResolvedOverlappingNodes()
        {
            ResolvedOverlappingNodes(this);
        }
    protected:
 
        //!Draw the current control.
        void Paint(const ImVec2& scrolling)
        {
            const ImVec2 offset = ImGui::GetCursorScreenPos() + scrolling;
            unsigned int nodeID = 0;
            Paint(this,offset,nodeID);
            PaintLink(this, offset);
        }
        
        //!Resolved overlapping for the current node.
        void ResolvedOverlappingNodes(ImGuiFrameVisualizerNode* node)
        {
            ImVec2 mMin, mMax;
            if (node->m_parent && node->m_parent->m_parent)
            {
                const float xOffsetPosition = 10.0f;
                node->m_parent->m_parent->GetAABB(mMin, mMax);
                node->m_position.x = mMax.x + xOffsetPosition;
                const size_t numChildrens = node->m_childrens.size();
                for (size_t index = 0; index < numChildrens; ++index)
                {
                    ImGuiFrameVisualizerNode* childNode = node->m_childrens[index];
                    childNode->m_position.x = mMax.x + 10.0f;
                }
            }
            const size_t numChildrens = node->m_childrens.size();
            for (size_t index = 0; index < numChildrens; ++index)
            {
                ImGuiFrameVisualizerNode* childNode = node->m_childrens[index];
                ResolvedOverlappingNodes(childNode);
            }
        }
        AZStd::string m_name;
        int m_inputCounts = 0;
        int m_outputCount = 0;
        ImVec2 m_position;
        ImVec2 m_size = ImVec2(1.0f, 1.0f);
        ImGuiFrameVisualizerNode* m_parent = nullptr;
        AZStd::vector<ImGuiFrameVisualizerNode*> m_childrens;
        AZStd::unordered_map<size_t,size_t> m_removableNodes;
        AZStd::vector<ImGuiFrameVisualizerField*> m_fields;
    };
    
    //!Window class for the frame visualizer.
    class ImGuiFrameVisualizerWindow final
    {
    public:

        ImGuiFrameVisualizerWindow(const AZStd::string& name,const unsigned int width,const int unsigned height) :
        m_rootNode(nullptr),
        m_windowWidth(width),
        m_windowHeight(height),
        m_windowName(name),
        m_showGrid(true),
        m_frameCapture(false)
        {

        }

        ~ImGuiFrameVisualizerWindow()
        {
            if(m_rootNode)
            {
                m_rootNode->ClearChildren();
                SAFE_DELETE(m_rootNode);
            }
        }
        
        //!Add an node to the window.
        ImGuiFrameVisualizerNode* AddNode(const AZStd::string& name,const unsigned int numInputs,const unsigned int numOutputs)
        {
            if (m_rootNode == nullptr)
            {
                m_rootNode = new  ImGuiFrameVisualizerNode(nullptr, name, ImVec2(10.0f, 10.0f), numInputs, numOutputs);
                return m_rootNode;
            }
            else
            {
                return m_rootNode->AddChild(name, numInputs, numOutputs);
            }
        }
        
        //!Resolve all the overlapping nodes.
        void ResolvedOverlappingNodes()
        {
            if(m_rootNode)
            {
                m_rootNode->ResolvedOverlappingNodes();
            }
        }

        //!Draw the UI and all the nodes.
        void Paint(bool& draw)
        {
            ImGui::SetNextWindowSize(ImVec2((float)m_windowWidth, (float)m_windowHeight), ImGuiCond_FirstUseEver);
            if (!ImGui::Begin(m_windowName.c_str(), &draw)) 
            {
                ImGui::End();
                return;
            }
            ImGuiIO& io = ImGui::GetIO();

            ImGui::SameLine();
            ImGui::BeginGroup();

            ImGui::Text("Hold middle mouse button to scroll (%.2f,%.2f)", m_scrolling.x, m_scrolling.y);
            ImGui::SameLine(ImGui::GetWindowWidth() - 150);
            ImGui::Checkbox("Show grid", &m_showGrid);
            ImGui::SameLine(ImGui::GetWindowWidth() - 280);
            if(ImGui::Button("Capture Frame", ImVec2(95, 20)))
            {
                m_frameCapture = true;
            }
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(60, 60, 70, 200));
            ImGui::BeginChild("scrolling_region", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
            ImGui::PopStyleVar();
            ImGui::PushItemWidth(120.0f);


            ImDrawList* drawList = ImGui::GetWindowDrawList();

            if (m_showGrid)
            {
                const ImU32 gridColor = IM_COL32(200, 200, 200, 40);
                const float gridSize = 64.0f;
                ImVec2 windowPosition = ImGui::GetCursorScreenPos();
                ImVec2 canvasSize = ImGui::GetWindowSize();
                for (float x = fmodf(m_scrolling.x, gridSize); x < canvasSize.x; x += gridSize)
                {
                    drawList->AddLine(ImVec2(x, 0.0f) + windowPosition, ImVec2(x, canvasSize.y) + windowPosition, gridColor);
                }
                for (float y = fmodf(m_scrolling.y, gridSize); y < canvasSize.y; y += gridSize)
                {
                    drawList->AddLine(ImVec2(0.0f, y) + windowPosition, ImVec2(canvasSize.x, y) + windowPosition, gridColor);
                }
            }

            if (m_rootNode)
            {
                drawList->ChannelsSplit(2);
                m_rootNode->Paint(m_scrolling);
                drawList->ChannelsMerge();
            }
            if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
            {
                m_scrolling = m_scrolling + io.MouseDelta;
            }
            ImGui::PopItemWidth();
            ImGui::EndChild();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
            ImGui::EndGroup();
            ImGui::End();
        }

        //!Capture an single frame to display.
        void CaptureFrame(AZ::Render::ImGuiFrameVisualizer* frameVisualizer)
        {
            if(frameVisualizer)
            {
                if(m_rootNode)
                {
                    m_rootNode->ClearChildren();
                    SAFE_DELETE(m_rootNode);
                }
                AZStd::vector<AZ::Render::ImGuiFrameVisualizer::FrameAttachmentVisualizeInfo>& frameAttachments = frameVisualizer->GetFrameAttachments();
                for (size_t index = 0; index < frameAttachments.size(); ++index)
                {
                    ImGuiFrameVisualizerNode* node = AddNode(frameAttachments[index].m_pFirstScopeVisual[0].m_scopeId.GetCStr(), 1, 1);
                    for (int nextIndex = 1; nextIndex < int(frameAttachments[index].m_pFirstScopeVisual.size()); ++nextIndex)
                    {
                        node->AddChild(frameAttachments[index].m_pFirstScopeVisual[nextIndex].m_scopeId.GetCStr(), 1, 1);
                    }
                }
                ResolvedOverlappingNodes();
            }
        }

        //!Returns true if frame need to be captured.
        bool IsFrameNeedCaptured()const
        {
            return m_frameCapture;
        }

        //!Disable capture state.
        void DisableCaptureFrame()
        {
            m_frameCapture = false;
        }
    private:
        //[GFX TODO][ATOM-5510] Switch to smart pointer for auto ref counting
        ImGuiFrameVisualizerNode* m_rootNode = nullptr;
        ImVec2 m_scrolling;
        AZStd::string m_windowName;
        unsigned int m_windowWidth = 1;
        unsigned int m_windowHeight = 1;
        bool m_frameCapture = false;
        bool m_showGrid = true;
    };
}
static ImGui::ImGuiFrameVisualizerWindow* visualizerWindow = nullptr;
namespace AZ::Render
{
    //! Draw the frame graph.
    inline void ImGuiFrameVisualizer::Draw(bool& draw)
    {
        if (!visualizerWindow)
        {
            visualizerWindow = new ImGui::ImGuiFrameVisualizerWindow("Frame Visualizer", 1920, 1080);
        }
        if (visualizerWindow)
        {
            if(visualizerWindow->IsFrameNeedCaptured() && m_framesAttachments.size())
            {

                visualizerWindow->CaptureFrame(this);
                visualizerWindow->DisableCaptureFrame();
            }
            visualizerWindow->Paint(draw);
        }
    }

    //! Get the frame attachments.
    inline AZStd::vector<ImGuiFrameVisualizer::FrameAttachmentVisualizeInfo>& ImGuiFrameVisualizer::GetFrameAttachments()
    {
        return m_framesAttachments;
    }

    //! Init The visualizer.
    inline void ImGuiFrameVisualizer::Init(RHI::Device* device)
    {
        if (!m_deviceInit)
        {
            m_deviceInit = true;
            m_device = device;
            if (m_device)
            {
                RHI::FrameEventBus::Handler::BusConnect(m_device);
            }
        }
    }

    //! Reset the frame graph.
    inline void ImGuiFrameVisualizer::Reset()
    {
        if (m_device)
        {
            RHI::FrameEventBus::Handler::BusDisconnect();
        }
        m_device = nullptr;
        m_deviceInit = false;
        //[GFX TODO][ATOM-5510] Switch to smart pointer for auto ref counting
        SAFE_DELETE(visualizerWindow);
    }

    //! Called when the frame graph has completed.
    inline void ImGuiFrameVisualizer::OnFrameCompileEnd(RHI::FrameGraph& frameGraph)
    {
        const AZ::RHI::FrameGraphAttachmentDatabase& attachmentDatabase = frameGraph.GetAttachmentDatabase();
        const AZStd::vector<AZ::RHI::FrameAttachment*>& attachments = attachmentDatabase.GetAttachments();
        m_framesAttachments.clear();
        m_framesAttachments.resize(attachments.size());
        for (size_t i = 0; i < attachments.size(); ++i)
        {
            AZ::RHI::FrameAttachment& attachment = *attachments[i];
            FrameAttachmentVisualizeInfo& attachmentVisualInfo = m_framesAttachments[i];
            attachmentVisualInfo.m_pFirstScopeVisual.clear();
            attachmentVisualInfo.m_pLastScopeVisual.clear();
            for (int deviceIndex{ 0 }; deviceIndex < RHI::RHISystemInterface::Get()->GetDeviceCount(); ++deviceIndex)
            {
                const AZ::RHI::ScopeAttachment* scopeAttachment = attachment.GetFirstScopeAttachment(deviceIndex);
                while (scopeAttachment)
                {
                    attachmentVisualInfo.m_pFirstScopeVisual.push_back({ scopeAttachment->GetScope().GetId() });
                    scopeAttachment = scopeAttachment->GetNext();
                }
            }
        }
    }
}
