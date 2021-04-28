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
#include "ImGui_precompiled.h"

#include "ImGuiViewport.h"
#include <IRenderer.h>
#include <IEditor.h>
#include <QResizeEvent>
#include <imgui/imgui.h>
#include "ImGuiBus.h"
#include "ImGuiManager.h"
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

using namespace ImGui;

ImGuiViewportWidget::ImGuiViewportWidget(QWidget* parent)
    : QWidget(parent)
    , m_width(0)
    , m_height(0)
    , m_renderContextCreated(false)
    , m_creatingRenderContext(false)
    , m_lastTime(0)
    , m_lastFrameTime(0.f)
    , m_averageFrameTime(0.f)
{
    // Setup a timer for the maximum refresh rate we want.
    // Refresh is actually triggered by interaction events and by the IdleUpdate. This avoids the UI
    // Editor slowing down the main editor when no UI interaction is occurring.
    QObject::connect(&m_updateTimer, SIGNAL(timeout()), this, SLOT(RefreshTick()));
    const int kUpdateIntervalInMillseconds = 1000 / 60; // 60 Hz
    m_updateTimer.start(kUpdateIntervalInMillseconds);

    CreateRenderContext();
}

ImGuiViewportWidget::~ImGuiViewportWidget()
{
    DestroyRenderContext();
    ImGuiManagerBus::Broadcast(&IImGuiManager::SetEditorWindowState,
                                       DisplayState::Hidden);
}

bool ImGuiViewportWidget::CreateRenderContext()
{
    if (m_creatingRenderContext)
    {
        return false;
    }
    m_creatingRenderContext = true;
    DestroyRenderContext();
    HWND window = (HWND)QWidget::winId();
    IEditor* editor = nullptr;
    EBUS_EVENT_RESULT(editor, AzToolsFramework::EditorRequests::Bus, GetEditor);
    if (window && editor->GetEnv()->pRenderer && !m_renderContextCreated)
    {
        m_renderContextCreated = true;

        StorePreviousContext();
        editor->GetEnv()->pRenderer->CreateContext(window);
        RestorePreviousContext();

        ImGuiManagerBus::Broadcast(&IImGuiManager::SetEditorWindowState,
                                           DisplayState::Visible);

        m_creatingRenderContext = false;
        return true;
    }
    m_creatingRenderContext = false;
    return false;
}

void ImGuiViewportWidget::DestroyRenderContext()
{
    IEditor* editor = nullptr;
    EBUS_EVENT_RESULT(editor, AzToolsFramework::EditorRequests::Bus, GetEditor);
    if (editor->GetEnv()->pRenderer && m_renderContextCreated)
    {
        HWND window = (HWND)QWidget::winId();
        if (window != editor->GetEnv()->pRenderer->GetHWND())
        {
            editor = nullptr;
            EBUS_EVENT_RESULT(editor, AzToolsFramework::EditorRequests::Bus, GetEditor);
            editor->GetEnv()->pRenderer->DeleteContext(window);
        }
        m_renderContextCreated = false;
    }
}

void ImGuiViewportWidget::StorePreviousContext()
{
    SPreviousContext previous;
    IEditor* editor = nullptr;
    EBUS_EVENT_RESULT(editor, AzToolsFramework::EditorRequests::Bus, GetEditor);
    previous.width = editor->GetEnv()->pRenderer->GetWidth();
    previous.height = editor->GetEnv()->pRenderer->GetHeight();
    previous.window = (HWND)editor->GetEnv()->pRenderer->GetCurrentContextHWND();
    previous.renderCamera = editor->GetEnv()->pRenderer->GetCamera();
    previous.systemCamera = gEnv->pSystem->GetViewCamera();
    previous.isMainViewport = editor->GetEnv()->pRenderer->IsCurrentContextMainVP();
    m_previousContexts.push(previous);
}

void ImGuiViewportWidget::SetCurrentContext()
{
    StorePreviousContext();

    IEditor* editor = nullptr;
    EBUS_EVENT_RESULT(editor, AzToolsFramework::EditorRequests::Bus, GetEditor);

    HWND window = (HWND)QWidget::winId();
    editor->GetEnv()->pRenderer->SetCurrentContext(window);
    editor->GetEnv()->pRenderer->ChangeViewport(0, 0, m_width, m_height);
}

void ImGuiViewportWidget::RestorePreviousContext()
{
    if (m_previousContexts.empty())
    {
        return;
    }

    SPreviousContext x = m_previousContexts.top();
    m_previousContexts.pop();
    IEditor* editor = nullptr;
    EBUS_EVENT_RESULT(editor, AzToolsFramework::EditorRequests::Bus, GetEditor);
    editor->GetEnv()->pRenderer->SetCurrentContext(x.window);
    editor->GetEnv()->pRenderer->ChangeViewport(0, 0, x.width, x.height, x.isMainViewport);

    editor->GetEnv()->pRenderer->SetCamera(x.renderCamera);
    gEnv->pSystem->SetViewCamera(x.systemCamera);
}

void ImGuiViewportWidget::Render()
{
    SetCurrentContext();

    IEditor* editor = nullptr;
    EBUS_EVENT_RESULT(editor, AzToolsFramework::EditorRequests::Bus, GetEditor);

    editor->GetEnv()->pSystem->RenderBegin();

    IRenderer* renderer = editor->GetEnv()->pRenderer;

    ColorF viewportBackgroundColor(Col_Gray);
    ColorF stateMessageColor(Col_Gray);
    AZStd::string stateMessage = "No State";
    DisplayState visibilityState = DisplayState::Hidden;
    ImGuiManagerBus::BroadcastResult(visibilityState,
                                             &IImGuiManager::GetEditorWindowState);
    switch (visibilityState)
    {
    case ImGui::DisplayState::Hidden:
        stateMessage = "Invisible";
        break;
    case ImGui::DisplayState::Visible:
        stateMessage = "ImGui Window";
        stateMessageColor = Col_SteelBlue;
        viewportBackgroundColor = Col_SkyBlue;
        break;
    case ImGui::DisplayState::VisibleNoMouse:
        stateMessage = "Game Focus";
        stateMessageColor = Col_Gray;
        viewportBackgroundColor = Col_LightGray;
        break;
    }

    renderer->ClearTargetsImmediately(FRT_CLEAR | FRT_CLEAR_IMMEDIATE, viewportBackgroundColor);
    renderer->ResetToDefault();

    renderer->Draw2dLabel(
        12.0f, m_height - 50.f, 1.25f, Col_White, false, "FPS: %.2f", 1.0f / m_averageFrameTime);
    renderer->Draw2dLabel(
        12.0f, m_height - 30.f, 2.f, stateMessageColor, false, "State: %s", stateMessage.c_str());

    if (ImDrawData* drawData = ImGui::GetDrawData())
    {
        // Configure renderer for 2d imgui rendering
        renderer->SetCullMode(R_CULL_DISABLE);
        TransformationMatrices backupSceneMatrices;
        renderer->Set2DMode(renderer->GetWidth(), renderer->GetHeight(), backupSceneMatrices);
        renderer->SetColorOp(eCO_REPLACE, eCO_MODULATE, eCA_Diffuse, DEF_TEXARG0);
        renderer->SetSrgbWrite(false);
        renderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_NODEPTHTEST);

        // Expand vertex buffer if necessary
        m_vertBuffer.reserve(drawData->TotalVtxCount);
        if (m_vertBuffer.size() < drawData->TotalVtxCount)
        {
            m_vertBuffer.insert(m_vertBuffer.end(),
                                drawData->TotalVtxCount - m_vertBuffer.size(),
                                SVF_P3F_C4B_T2F());
        }

        // Expand index buffer if necessary
        m_idxBuffer.reserve(drawData->TotalIdxCount);
        if (m_idxBuffer.size() < drawData->TotalIdxCount)
        {
            m_idxBuffer.insert(m_idxBuffer.end(), drawData->TotalIdxCount - m_idxBuffer.size(), 0);
        }

        // Process each draw command list individually
        for (int n = 0; n < drawData->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = drawData->CmdLists[n];

            // Cache max vert count for easy access
            int numVerts = cmd_list->VtxBuffer.Size;

            // Copy command list verts into buffer
            for (int i = 0; i < numVerts; ++i)
            {
                const ImDrawVert& imguiVert = cmd_list->VtxBuffer[i];
                SVF_P3F_C4B_T2F& vert = m_vertBuffer[i];

                vert.xyz = Vec3(imguiVert.pos.x, imguiVert.pos.y, 0.0f);
                // Convert color from RGBA to ARGB
                vert.color.dcolor = (imguiVert.col & 0xFF00FF00)
                                    | ((imguiVert.col & 0xFF0000) >> 16)
                                    | ((imguiVert.col & 0xFF) << 16);
                vert.st = Vec2(imguiVert.uv.x, imguiVert.uv.y);
            }

            // Copy command list indices into buffer
            for (int i = 0; i < cmd_list->IdxBuffer.Size; ++i)
            {
                m_idxBuffer[i] = uint16(cmd_list->IdxBuffer[i]);
            }

            // Use offset pointer to step along rendering operation
            uint16* idxBufferDataOffset = m_idxBuffer.data();

            // Process each draw command individually
            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); cmd_i++)
            {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];

                // Defer to user rendering callback, if appropriate
                if (pcmd->UserCallback)
                {
                    pcmd->UserCallback(cmd_list, pcmd);
                }
                // Otherwise render our buffers
                else
                {
                    int textureId = ((ITexture*)pcmd->TextureId)->GetTextureID();
                    renderer->SetTexture(textureId);
                    renderer->SetScissor((int)pcmd->ClipRect.x,
                                         (int)(pcmd->ClipRect.y),
                                         (int)(pcmd->ClipRect.z - pcmd->ClipRect.x),
                                         (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                    renderer->DrawDynVB(m_vertBuffer.data(),
                                        idxBufferDataOffset,
                                        numVerts,
                                        pcmd->ElemCount,
                                        prtTriangleList);
                }

                // Update offset pointer into command list's index buffer
                idxBufferDataOffset += pcmd->ElemCount;
            }
        }

        // Reset scissor usage on renderer
        renderer->SetScissor();

        // Clean up renderer settings
        renderer->Unset2DMode(backupSceneMatrices);
    }

    bool renderStats = false;
    editor->GetEnv()->pSystem->RenderEnd(renderStats, false);
    RestorePreviousContext();
}

void ImGuiViewportWidget::resizeEvent(QResizeEvent* ev)
{
    QWidget::resizeEvent(ev);

    int cx = ev->size().width();
    int cy = ev->size().height();
    if (cx == 0 || cy == 0)
    {
        return;
    }

    m_width = cx;
    m_height = cy;

    IEditor* editor = nullptr;
    EBUS_EVENT_RESULT(editor, AzToolsFramework::EditorRequests::Bus, GetEditor);
    editor->GetEnv()->pSystem->GetISystemEventDispatcher()->OnSystemEvent(
        ESYSTEM_EVENT_RESIZE, cx, cy);

    RefreshTick();
}

void ImGuiViewportWidget::RefreshTick()
{
    IEditor* editor = nullptr;
    EBUS_EVENT_RESULT(editor, AzToolsFramework::EditorRequests::Bus, GetEditor);
    int64 time = editor->GetEnv()->pSystem->GetITimer()->GetAsyncTime().GetMilliSecondsAsInt64();
    if (m_lastTime == 0)
    {
        m_lastTime = time;
    }
    m_lastFrameTime = (time - m_lastTime) * 0.001f;
    m_lastTime = time;
    if (m_averageFrameTime == 0.0f)
    {
        m_averageFrameTime = m_lastFrameTime;
    }
    else
    {
        m_averageFrameTime = 0.01f * m_lastFrameTime + 0.99f * m_averageFrameTime;
    }

    Render();

    m_updateTimer.start();
}

bool ImGuiViewportWidget::event(QEvent* ev)
{
    bool result = QWidget::event(ev);

    if (ev->type() == QEvent::WinIdChange)
    {
        CreateRenderContext();
    }

    return result;
}

#include <Editor/moc_ImGuiViewport.cpp>
