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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Interface to the Performance HUD


#ifndef CRYINCLUDE_CRYCOMMON_IPERFHUD_H
#define CRYINCLUDE_CRYCOMMON_IPERFHUD_H
#pragma once


#include <CryExtension/ICryUnknown.h>
#include <CryExtension/Impl/ClassWeaver.h>
#include <ICryMiniGUI.h>
#include <IXml.h>

struct ICryPerfHUDWidget
    : public _reference_target_t
{
    //
    enum EWidgetID
    {
        eWidget_Warnings = 0,
        eWidget_RenderStats,
        eWidget_StreamingStats,
        eWidget_RenderBatchStats,
        eWidget_FpsBuckets,
        eWidget_Particles,
        eWidget_PakFile,
        eWidget_Num, //number of widgets
    };

    ICryPerfHUDWidget(int id = -1)
        : m_id(id)
    {}

    // <interfuscator:shuffle>
    virtual ~ICryPerfHUDWidget() {}

    virtual void Reset() = 0;
    virtual void Update() = 0;
    virtual bool ShouldUpdate() = 0;
    virtual void LoadBudgets(XmlNodeRef perfXML) = 0;
    virtual void SaveStats(XmlNodeRef statsXML) = 0;
    virtual void Enable(int mode) = 0;
    virtual void Disable() = 0;
    // </interfuscator:shuffle>

    int m_id;
};

// Base Interface for all engine module extensions
struct ICryPerfHUD
    : public ICryUnknown
{
    CRYINTERFACE_DECLARE(ICryPerfHUD, 0x268d142e043d464c, 0xa0776580f81b988a);

    struct PerfBucket
    {
        ILINE PerfBucket(float _target)
        {
            target = _target;
            timeAtTarget = 0.f;
        }

        float target;
        float timeAtTarget;
    };

    enum EHudState
    {
        eHudOff = 0,
        eHudInFocus,
        eHudOutOfFocus,
        eHudNumStates,
    };

    // <interfuscator:shuffle>
    // Called once to initialize HUD.
    virtual void Init() = 0;
    virtual void Done() = 0;
    virtual void Draw() = 0;
    virtual void LoadBudgets() = 0;
    virtual void SaveStats(const char* filename = NULL) = 0;
    virtual void ResetWidgets() = 0;
    virtual void SetState(EHudState state) = 0;
    virtual void Reset() = 0;
    virtual void Destroy() = 0;

    // Retrieve name of the extension module.
    virtual void Show(bool bRestoreState) = 0;

    virtual void AddWidget(ICryPerfHUDWidget* pWidget) = 0;
    virtual void RemoveWidget(ICryPerfHUDWidget* pWidget) = 0;

    virtual minigui::IMiniCtrl*         CreateMenu(const char* name, minigui::IMiniCtrl* pParent = NULL) = 0;
    virtual bool                                        CreateCVarMenuItem(minigui::IMiniCtrl* pMenu, const char* name, const char* controlVar, float controlVarOn, float controlVarOff) = 0;
    virtual bool                                        CreateCallbackMenuItem(minigui::IMiniCtrl* pMenu, const char* name, minigui::ClickCallback clickCallback, void* pCallbackData) = 0;
    virtual minigui::IMiniInfoBox*  CreateInfoMenuItem(minigui::IMiniCtrl* pMenu, const char* name, minigui::RenderCallback renderCallback, const minigui::Rect& rect, bool onAtStart = false) = 0;
    virtual minigui::IMiniTable*        CreateTableMenuItem(minigui::IMiniCtrl* pMenu, const char* name) = 0;

    virtual minigui::IMiniCtrl* GetMenu(const char* name) = 0;

    virtual void EnableWidget(ICryPerfHUDWidget::EWidgetID id, int mode) = 0;
    virtual void DisableWidget(ICryPerfHUDWidget::EWidgetID id) = 0;

    //Warnings - Widget Specific interface
    virtual void AddWarning(float duration, const char* fmt, va_list argList) = 0;
    virtual bool WarningsWindowEnabled() const = 0;

    //FPS - Widget Specific interface
    virtual const std::vector<PerfBucket>* GetFpsBuckets(float& totalTime) const = 0;
    // </interfuscator:shuffle>
};

DECLARE_SMART_POINTERS(ICryPerfHUD);

void CryPerfHUDWarning(float duration, const char*, ...) PRINTF_PARAMS(2, 3);
inline void CryPerfHUDWarning(float duration, const char* format, ...)
{
    if (gEnv && gEnv->pSystem)
    {
        ICryPerfHUD* pPerfHud = gEnv->pSystem->GetPerfHUD();

        if (pPerfHud)
        {
            va_list args;
            va_start(args, format);
            pPerfHud->AddWarning(duration, format, args);
            va_end(args);
        }
    }
}

#endif // CRYINCLUDE_CRYCOMMON_IPERFHUD_H
