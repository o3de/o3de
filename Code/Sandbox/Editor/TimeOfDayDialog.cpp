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

#include "EditorDefs.h"

#include "TimeOfDayDialog.h"

// Qt
#include <QMessageBox>

// AzToolsFramework
#include <AzToolsFramework/API/ViewPaneOptions.h>

// CryCommon
#include <CryCommon/ITimeOfDay.h>

// Editor
#include "Settings.h"
#include "Controls/CurveEditorCtrl.h"
#include "Mission.h"
#include "CryEditDoc.h"
#include "Clipboard.h"
#include "QtViewPaneManager.h"
#include "Undo/Undo.h"
#include "LyViewPaneNames.h"

// Cry3DEngine
#include <Cry3DEngine/Environment/OceanEnvironmentBus.h>

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_TimeOfDayDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

#define PANE_LAYOUT_SECTION _T("DockingPaneLayouts\\TimeOfDay")
#define PANE_LAYOUT_VERSION_ENTRY _T("PaneLayoutVersion")

namespace TimeOfDayDetails
{
    const float kEpsilon = 0.00001f;

    const int kTimeOfDayDialogLayoutVersion = 0x0002; // bump this up on every substantial pane layout change

    static void SetKeyTangentType(ISplineInterpolator* pSpline, int key, ESplineKeyTangentType type)
    {
        int flags = (pSpline->GetKeyFlags(key) & ~SPLINE_KEY_TANGENT_IN_MASK) & ~SPLINE_KEY_TANGENT_OUT_MASK;
        pSpline->SetKeyFlags(key, flags | (type << SPLINE_KEY_TANGENT_IN_SHIFT) | (type << SPLINE_KEY_TANGENT_OUT_SHIFT));
    }

    static QTime qTimeFromFloat(float time)
    {
        // The float time goes from 0.0 - 23.98 (since max time is 23:59), so
        // convert this to seconds so we can construct a QTime object from that
        int seconds = (time * 60.0f) * 60;
        return QTime(0, 0).addSecs(seconds);
    }

    static float floatFromQTime(const QTime& time)
    {
        return static_cast<float>(time.msecsSinceStartOfDay() / 60000) / 60.0;
    }

    // Is the ocean component feature toggle enabled?
    AZ_INLINE static bool HasOceanFeatureToggle()
    {
        bool bHasOceanFeature = false;
        AZ::OceanFeatureToggleBus::BroadcastResult(bHasOceanFeature, &AZ::OceanFeatureToggleBus::Events::OceanComponentEnabled);
        return bHasOceanFeature;
    }

    AZ_INLINE static bool SkipUserInterface(int value)
    {
        // Check for obsolete parameters that we still want to keep around to migrate legacy data to new data
        // but don't want to display in the UI
        const ITimeOfDay::ETimeOfDayParamID enumValue = static_cast<ITimeOfDay::ETimeOfDayParamID>(value);

        // Split the check into two parts to try to keep the logic clear.
        // The First set of parameters are always checked...
        bool skipParameter = (enumValue == ITimeOfDay::PARAM_HDR_DYNAMIC_POWER_FACTOR) ||
            (enumValue == ITimeOfDay::PARAM_TERRAIN_OCCL_MULTIPLIER) ||
            (enumValue == ITimeOfDay::PARAM_SUN_COLOR_MULTIPLIER);

        // Only check the ocean parameters if the ocean feature (aka the Infinite Ocean Component)
        // has been enabled
        skipParameter |= HasOceanFeatureToggle() &&
               ((enumValue == ITimeOfDay::PARAM_OCEANFOG_COLOR) ||
               (enumValue == ITimeOfDay::PARAM_OCEANFOG_COLOR_MULTIPLIER) ||
               (enumValue == ITimeOfDay::PARAM_OCEANFOG_DENSITY));

        return skipParameter;
    }
}

CHDRPane::CHDRPane(CTimeOfDayDialog* pTODDlg)
    : QWidget(pTODDlg)
    , m_pTODDlg(pTODDlg)
    , m_pVars(new CVarBlock)
{
    assert(m_pTODDlg);
#if !defined(NDEBUG)
    bool ok =
#endif
        Init();
    assert(ok);
}

bool CHDRPane::Init()
{
    m_filmCurveCtrl = new CCurveEditorCtrl(this);
    m_filmCurveCtrl->SetControlPointCount(21);
    m_filmCurveCtrl->SetMouseEnable(false);
    m_filmCurveCtrl->SetPadding(16);
    m_filmCurveCtrl->SetFlags(
        CCurveEditorCtrl::eFlag_ShowVerticalRuler |
        CCurveEditorCtrl::eFlag_ShowHorizontalRuler |
        CCurveEditorCtrl::eFlag_ShowCursorAlways |
        CCurveEditorCtrl::eFlag_ShowVerticalRulerText |
        CCurveEditorCtrl::eFlag_ShowHorizontalRulerText |
        CCurveEditorCtrl::eFlag_ShowPaddingBorder);
    m_filmCurveCtrl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_propsCtrl = new ReflectedPropertyControl(this);
    m_propsCtrl->Setup();

    QVBoxLayout* l = new QVBoxLayout;
    l->setMargin(0);
    l->addWidget(m_filmCurveCtrl);
    l->addWidget(m_propsCtrl);
    setLayout(l);

    m_propsCtrl->SetSelChangeCallback(AZStd::bind(&CHDRPane::OnPropertySelected, this, AZStd::placeholders::_1));

    return true;
}

void CHDRPane::OnPropertySelected(IVariable *pVar)
{
    if (pVar && pVar->GetType() == IVariable::ARRAY)
    {
        pVar = nullptr;
    }

    emit propertySelected(pVar);
}

bool CHDRPane::GetFilmCurveParams(float& shoulderScale, float& midScale, float& toeScale, float& whitePoint) const
{
    int checked = 0;
    ITimeOfDay* pTimeOfDay = gEnv->p3DEngine->GetTimeOfDay();

    for (int i = 0; pTimeOfDay->GetVariableCount(); ++i)
    {
        ITimeOfDay::SVariableInfo varInfo;
        bool ok = pTimeOfDay->GetVariableInfo(i, varInfo);
        if (ok == false)
        {
            continue;
        }
        if (TimeOfDayDetails::SkipUserInterface(varInfo.nParamId))
        {
            continue;
        }

        switch (varInfo.nParamId)
        {
        case ITimeOfDay::PARAM_HDR_FILMCURVE_SHOULDER_SCALE:
            shoulderScale = varInfo.fValue[0];
            ++checked;
            break;
        case ITimeOfDay::PARAM_HDR_FILMCURVE_LINEAR_SCALE:
            midScale = varInfo.fValue[0];
            ++checked;
            break;
        case ITimeOfDay::PARAM_HDR_FILMCURVE_TOE_SCALE:
            toeScale = varInfo.fValue[0];
            ++checked;
            break;
        case ITimeOfDay::PARAM_HDR_FILMCURVE_WHITEPOINT:
            whitePoint = varInfo.fValue[0];
            ++checked;
            break;
        }

        if (checked == 4)
        {
            return true;
        }
    }

    return false;
}

static float EvalFilmCurve(float x, float ss, float ms, float ts)
{
    return (x * (ss * 6.2f * x + 0.5f * ms)) / max<float>((x * (ss * 6.2f * x + 1.7f) + ts * 0.06f), TimeOfDayDetails::kEpsilon);
}

void CHDRPane::UpdateFilmCurve()
{
    float shoulderScale = 0, midScale = 0, toeScale = 0, whitePoint = 0;
#if !defined(NDEBUG)
    bool ok =
#endif
        GetFilmCurveParams(shoulderScale, midScale, toeScale, whitePoint);
    assert(ok);
    const float minX = -4.0f, minY = 0.0f, maxX = 4.0f;
    float maxY = 1.0f, stepY = 0.1f;

    int numSamplePoints = m_filmCurveCtrl->GetControlPointCount();

    for (int i = 0; i < numSamplePoints; ++i)
    {
        float t = static_cast<float>(i) / (numSamplePoints - 1);
        float logX = minX + ((maxX - minX) * t);
        float x = powf(10.0f, logX); // Conventionally, the x domain is logarithmic.

        float v = EvalFilmCurve(x, shoulderScale, midScale, toeScale) /
            max<float>(EvalFilmCurve(whitePoint, shoulderScale, midScale, toeScale), TimeOfDayDetails::kEpsilon);
        v = powf(v, 2.2f); // converting to a linear space

        // Update the maximum Y so that a proper domain can be set later.
        if (v > maxY)
        {
            maxY = ceil(v / stepY) * stepY;
        }

        m_filmCurveCtrl->SetControlPoint(i, Vec2(logX, v));
    }

    // maxY is not fixed, so adjust Y grid count properly according to it.
    const UINT gridX = 4; // X grid is fixed.
    UINT gridY = static_cast<UINT>(maxY / stepY);
    while (gridY > 20) // > 20 means too many, so reduce the count properly.
    {
        gridY = (gridY + 9) / 10;
        stepY *= 10.0f;
    }
    maxY = stepY * gridY;
    // Also prepare labels for the grid since the default labeling is improper
    // especially for the X axis due to its log scale.
    QStringList labelsX, labelsY;
    for (int i = 0; i <= gridX; ++i)
    {
        QString label;
        label.asprintf("%.4g", powf(10.0f, minX + (maxX - minX) * static_cast<float>(i) / gridX));
        labelsX.push_back(label);
    }
    for (int i = 0; i <= gridY; ++i)
    {
        QString label;
        label.asprintf("%.1f", i * stepY);
        labelsY.push_back(label);
    }
    m_filmCurveCtrl->SetGrid(gridX, gridY, labelsX, labelsY);
    m_filmCurveCtrl->MarkY(1.0f); // Mark the output of 1 so that users can quickly recognize where the clamping happens.
    m_filmCurveCtrl->SetDomainBounds(minX, minY, maxX, maxY);

    m_filmCurveCtrl->update();
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Adapter for Multi element interpolators that allow to split it to several
// different interpolators for each element separately.
//////////////////////////////////////////////////////////////////////////
class CMultiElementSplineInterpolatorAdapter
    : public ISplineInterpolator
{
public:
    CMultiElementSplineInterpolatorAdapter()
        : m_pInterpolator(0)
        , m_element(0) {}
    CMultiElementSplineInterpolatorAdapter(ISplineInterpolator* pSpline, int element)
    {
        m_pInterpolator = pSpline;
        m_element = element;
    }
    // Dimension of the spline from 0 to 3, number of parameters used in ValueType.
    virtual int GetNumDimensions() { return m_pInterpolator->GetNumDimensions(); }
    virtual int  InsertKey(float time, ValueType value)
    {
        ValueType v = {0, 0, 0, 0};
        v[m_element] = value[0];
        return m_pInterpolator->InsertKey(time, v);
    };
    virtual void RemoveKey(int key) { m_pInterpolator->RemoveKey(key); };

    virtual void FindKeysInRange(float startTime, float endTime, int& firstFoundKey, int& numFoundKeys)
    { m_pInterpolator->FindKeysInRange(startTime, endTime, firstFoundKey, numFoundKeys); }
    virtual void RemoveKeysInRange(float startTime, float endTime) { m_pInterpolator->RemoveKeysInRange(startTime, endTime); }

    virtual int   GetKeyCount() { return m_pInterpolator->GetKeyCount(); }
    virtual void  SetKeyTime(int key, float time) { return m_pInterpolator->SetKeyTime(key, time); };
    virtual float GetKeyTime(int key) { return m_pInterpolator->GetKeyTime(key); }

    virtual void  SetKeyValue(int key, ValueType value)
    {
        ValueType v = {0, 0, 0, 0};
        m_pInterpolator->GetKeyValue(key, v);
        v[m_element] = value[0];
        m_pInterpolator->SetKeyValue(key, v);
    }
    virtual bool  GetKeyValue(int key, ValueType& value)
    {
        ValueType v = {0, 0, 0, 0};
        v[m_element] = value[0];
        return m_pInterpolator->GetKeyValue(key, value);
        value[0] = v[m_element];
    }

    virtual void  SetKeyInTangent([[maybe_unused]] int key, [[maybe_unused]] ValueType tin) {};
    virtual void  SetKeyOutTangent([[maybe_unused]] int key, [[maybe_unused]] ValueType tout) {};
    virtual void  SetKeyTangents([[maybe_unused]] int key, [[maybe_unused]] ValueType tin, [[maybe_unused]] ValueType tout) {};
    virtual bool  GetKeyTangents([[maybe_unused]] int key, [[maybe_unused]] ValueType& tin, [[maybe_unused]] ValueType& tout) { return false; };

    // Changes key flags, @see ESplineKeyFlags
    virtual void  SetKeyFlags(int key, int flags) { return m_pInterpolator->SetKeyFlags(key, flags); };
    // Retrieve key flags, @see ESplineKeyFlags
    virtual int   GetKeyFlags(int key) { return m_pInterpolator->GetKeyFlags(key); }

    virtual void Interpolate(float time, ValueType& value)
    {
        m_pInterpolator->Interpolate(time, value);
        value[0] = value[m_element];
    };

    virtual void SerializeSpline(XmlNodeRef& node, bool bLoading) { return m_pInterpolator->SerializeSpline(node, bLoading); };

    virtual ISplineBackup* Backup() { return m_pInterpolator->Backup(); };
    virtual void Restore(ISplineBackup* pBackup) { return m_pInterpolator->Restore(pBackup); };

public:
    ISplineInterpolator* m_pInterpolator;
    int m_element;
};

//////////////////////////////////////////////////////////////////////////
class CTimeOfDaySplineSet
    : public ISplineSet
{
public:
    void AddSpline(ISplineInterpolator* pSpline) { m_splines.push_back(pSpline); };
    void RemoveAllSplines() { m_splines.clear(); }

    virtual ISplineInterpolator* GetSplineFromID(const string& id)
    {
        int i = atoi(id.c_str());
        if (i >= 0 && i < (int)m_splines.size())
        {
            return m_splines[i];
        }
        return 0;
    };
    virtual string GetIDFromSpline(ISplineInterpolator* pSpline)
    {
        for (int i = 0; i < (int)m_splines.size(); i++)
        {
            if (m_splines[i] == pSpline)
            {
                string s;
                s.Format("%d", i);
                return s;
            }
        }
        return "";
    }
    virtual int GetSplineCount() const { return (int)m_splines.size(); }
    virtual int GetKeyCountAtTime(float time, float threshold) const
    {
        int count = 0;
        for (int i = 0; i < (int)m_splines.size(); i++)
        {
            if (m_splines[i]->FindKey(time, threshold) > 0)
            {
                count++;
            }
        }
        return count;
    }

public:
    std::vector<ISplineInterpolator*> m_splines;
};

//////////////////////////////////////////////////////////////////////////
CTimeOfDayDialog::CTimeOfDayDialog(QWidget* parent /* = nullptr */)
    : QMainWindow(parent)
    , m_ui(new Ui::TimeOfDayDialog)
    , m_timelineCtrl(new TimelineWidget(this))
    , m_pHDRPane(new CHDRPane(this))
{
    gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this);
    GetIEditor()->RegisterNotifyListener(this);

    m_ui->setupUi(this);
    m_ui->parameters->Setup();
    splitDockWidget(m_ui->hdrPaneDock, m_ui->tasksDock, Qt::Horizontal);

    // Calculate our maximum time from the slider (23:59)
    m_maxTime = m_ui->timelineSlider->maximum() / 60.0f;

    Init();
}

//////////////////////////////////////////////////////////////////////////
CTimeOfDayDialog::~CTimeOfDayDialog()
{
    m_alive = false;
    GetIEditor()->UnregisterNotifyListener(this);
    GetIEditor()->GetUndoManager()->RemoveListener(this);

    gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);

    QSettings settings;
    settings.beginGroup(QStringLiteral("EnvironmentEditor"));
    settings.setValue(QStringLiteral("state"), saveState());
}

const GUID& CTimeOfDayDialog::GetClassID()
{
    // {85FB1272-D858-4ca5-ABB4-04D484ABF51E}
    static const GUID guid = {
        0x85fb1272, 0xd858, 0x4ca5, { 0xab, 0xb4, 0x4, 0xd4, 0x84, 0xab, 0xf5, 0x1e }
    };
    return guid;
}

void CTimeOfDayDialog::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions options;
    options.paneRect = QRect(100, 100, 1500, 800);
    options.canHaveMultipleInstances = true;
    options.isDockable = true;

    AzToolsFramework::RegisterViewPane<CTimeOfDayDialog>(LyViewPane::TimeOfDayEditor, LyViewPane::CategoryOther, options);
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDayDialog::UpdateValues()
{
    if (!m_alive)
    {
        return;
    }

    ITimeOfDay* pTimeOfDay = gEnv->p3DEngine->GetTimeOfDay();
    ITimeOfDay::SAdvancedInfo advInfo;
    pTimeOfDay->GetAdvancedInfo(advInfo);

    RefreshPropertiesValues();

    SetTimeRange(advInfo.fStartTime, advInfo.fEndTime, advInfo.fAnimSpeed);
    UpdateUI(false);
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDayDialog::OnSystemEvent(ESystemEvent event, [[maybe_unused]] UINT_PTR wparam, [[maybe_unused]] UINT_PTR lparam)
{


    switch (event)
    {
    case ESYSTEM_EVENT_TIME_OF_DAY_SET:
        // We update the UI in response to a system event (instead of with direct callbacks in the dialog) because time could be set by any of the dialog,
        // e_TimeOfDay cvar, a Flow Graph Node or Track View.
        UpdateUI();
        break;
    default:
        // do nothing
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDayDialog::Init()
{
    // Load toolbar images for the main toolbar
    m_ui->actionUndo->setIcon(QIcon(":/TimeOfDay/main-00.png"));
    m_ui->actionRedo->setIcon(QIcon(":/TimeOfDay/main-01.png"));
    m_ui->actionImportFile->setIcon(QIcon(":/TimeOfDay/main-02.png"));
    m_ui->actionExportFile->setIcon(QIcon(":/TimeOfDay/main-03.png"));
    m_ui->actionPlayPause->setIcon(QIcon(":/TimeOfDay/main-04.png"));
    m_ui->actionSetTimeTo0000->setIcon(QIcon(":/TimeOfDay/main-05.png"));
    m_ui->actionSetTimeTo0600->setIcon(QIcon(":/TimeOfDay/main-06.png"));
    m_ui->actionSetTimeTo1200->setIcon(QIcon(":/TimeOfDay/main-07.png"));
    m_ui->actionSetTimeTo1800->setIcon(QIcon(":/TimeOfDay/main-08.png"));
    m_ui->actionSetTimeTo2400->setIcon(QIcon(":/TimeOfDay/main-09.png"));
    m_ui->actionStartStopRecording->setIcon(QIcon(":/TimeOfDay/main-10.png"));
    m_ui->actionHold->setIcon(QIcon(":/TimeOfDay/main-11.png"));
    m_ui->actionFetch->setIcon(QIcon(":/TimeOfDay/main-12.png"));

    // Load the images for the spline edit toolbar
    m_ui->tangentsToAutoButton->setIcon(QIcon(":/Common/spline_edit-00.png"));
    m_ui->inTangentToZeroButton->setIcon(QIcon(":/Common/spline_edit-01.png"));
    m_ui->inTangentToStepButton->setIcon(QIcon(":/Common/spline_edit-02.png"));
    m_ui->inTangentToLinearButton->setIcon(QIcon(":/Common/spline_edit-03.png"));
    m_ui->outTangentToZerobutton->setIcon(QIcon(":/Common/spline_edit-04.png"));
    m_ui->outTangentToStepButton->setIcon(QIcon(":/Common/spline_edit-05.png"));
    m_ui->outTangentToLinearButton->setIcon(QIcon(":/Common/spline_edit-06.png"));
    m_ui->fitSplinesHorizontalButton->setIcon(QIcon(":/Common/spline_edit-07.png"));
    m_ui->fitSplinesVerticalButton->setIcon(QIcon(":/Common/spline_edit-08.png"));
    m_ui->splineSnapGridX->setIcon(QIcon(":/Common/spline_edit-09.png"));
    m_ui->splineSnapGridY->setIcon(QIcon(":/Common/spline_edit-10.png"));
    m_ui->previousKeyButton->setIcon(QIcon(":/Common/spline_edit-14.png"));
    m_ui->nextKeyButton->setIcon(QIcon(":/Common/spline_edit-15.png"));
    m_ui->removeAllExceptSelectedButton->setIcon(QIcon(":/Common/spline_edit-16.png"));

    m_timelineCtrl->SetTicksTextScale(24.0f);
    m_timelineCtrl->SetTimeRange(Range(0, 1));

    m_ui->spline->SetDefaultKeyTangentType(SPLINE_KEY_TANGENT_LINEAR);
    m_ui->spline->SetTimelineCtrl(m_timelineCtrl);
    m_ui->spline->SetTimeRange(Range(0, 1.0f));
    m_ui->spline->SetValueRange(Range(-1.0f, 1.0f));
    m_ui->spline->SetMinTimeEpsilon(0.00001f);
    m_ui->spline->SetTooltipValueScale(24, 1);

    m_ui->colorGradient->SetNoZoom(false);
    m_ui->colorGradient->SetTimeRange(0, 1.0f);
    m_ui->colorGradient->LockFirstAndLastKeys(true);
    m_ui->colorGradient->SetTooltipValueScale(24, 1);

    CreateProperties();
    UpdateValues();
    UpdateUI();

    QSettings settings;
    settings.beginGroup(QStringLiteral("EnvironmentEditor"));
    QByteArray state = settings.value(QStringLiteral("state")).toByteArray();
    if (!state.isEmpty())
    {
        restoreState(state);
    }

    ResetSpline(0);

    m_ui->hdrPaneDock->setWidget(m_pHDRPane);
    m_pHDRPane->UpdateFilmCurve();

    QString copyAllLabel(tr("Copy All Parameters"));
    QString pasteAllLabel(tr("Paste All Parameters"));
    m_ui->parameters->AddCustomPopupMenuItem(copyAllLabel, AZStd::bind(&CTimeOfDayDialog::CopyAllProperties, this));
    m_ui->parameters->AddCustomPopupMenuItem(pasteAllLabel, AZStd::bind(&CTimeOfDayDialog::PasteAllProperties, this));
    m_pHDRPane->properties().AddCustomPopupMenuItem(copyAllLabel, AZStd::bind(&CTimeOfDayDialog::CopyAllProperties, this));
    m_pHDRPane->properties().AddCustomPopupMenuItem(pasteAllLabel, AZStd::bind(&CTimeOfDayDialog::PasteAllProperties, this));

    m_ui->parameters->SetSelChangeCallback(AZStd::bind(&CTimeOfDayDialog::OnPropertySelected, this, AZStd::placeholders::_1));
    connect(m_pHDRPane, &CHDRPane::propertySelected, this, &CTimeOfDayDialog::HdrPropertySelected);
    m_pHDRPane->properties().SetUpdateCallback(AZStd::bind(&CTimeOfDayDialog::OnUpdateProperties, this, AZStd::placeholders::_1));
    m_ui->parameters->SetUpdateCallback(AZStd::bind(&CTimeOfDayDialog::OnUpdateProperties, this, AZStd::placeholders::_1));

    connect(m_ui->importFromFileClickable, &QLabel::linkActivated, this, &CTimeOfDayDialog::OnImport);
    connect(m_ui->exportToFileClickable, &QLabel::linkActivated, this, &CTimeOfDayDialog::OnExport);
    connect(m_ui->resetValuesClickable, &QLabel::linkActivated, this, &CTimeOfDayDialog::OnResetToDefaultValues);
    connect(m_ui->expandAllClickable, &QLabel::linkActivated, this, &CTimeOfDayDialog::OnExpandAll);
    connect(m_ui->collapseAllClickable, &QLabel::linkActivated, this, &CTimeOfDayDialog::OnCollapseAll);

    connect(m_ui->currentTimeEdit, &QTimeEdit::timeChanged, this, [=](const QTime& time) { SetTime(TimeOfDayDetails::floatFromQTime(time)); });
    connect(m_ui->startTimeEdit, &QTimeEdit::timeChanged, this, &CTimeOfDayDialog::StartTimeChanged);
    connect(m_ui->endTimeEdit, &QTimeEdit::timeChanged, this, &CTimeOfDayDialog::EndTimeChanged);
    auto doubleValueChanged = static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);
    connect(m_ui->playSpeedDoubleSpinBox, doubleValueChanged, this, &CTimeOfDayDialog::OnChangeTimeAnimSpeed);

    connect(m_ui->playClickable, &QLabel::linkActivated, this, [=]() { m_ui->actionPlayPause->setChecked(true); });
    connect(m_ui->stopClickable, &QLabel::linkActivated, this, [=]() { m_ui->actionPlayPause->setChecked(false); });

    connect(m_ui->forceSkyUpdateCheckBox, &QCheckBox::stateChanged, this, [=](int state) { gSettings.bForceSkyUpdate = state == Qt::Checked; });

    connect(m_ui->actionUndo, &QAction::triggered, this, &CTimeOfDayDialog::OnUndo);
    connect(m_ui->actionRedo, &QAction::triggered, this, &CTimeOfDayDialog::OnRedo);
    connect(m_ui->actionImportFile, &QAction::triggered, this, &CTimeOfDayDialog::OnImport);
    connect(m_ui->actionExportFile, &QAction::triggered, this, &CTimeOfDayDialog::OnExport);

    connect(m_ui->actionSetTimeTo0000, &QAction::triggered, this, [=]() { SetTime(0); });
    connect(m_ui->actionSetTimeTo0600, &QAction::triggered, this, [=]() { SetTime(6); });
    connect(m_ui->actionSetTimeTo1200, &QAction::triggered, this, [=]() { SetTime(12); });
    connect(m_ui->actionSetTimeTo1800, &QAction::triggered, this, [=]() { SetTime(18); });
    connect(m_ui->actionSetTimeTo2400, &QAction::triggered, this, [=]() { SetTime(m_maxTime); });

    connect(m_ui->actionHold, &QAction::triggered, this, &CTimeOfDayDialog::OnHold);
    connect(m_ui->actionFetch, &QAction::triggered, this, &CTimeOfDayDialog::OnFetch);

    connect(m_ui->tangentsToAutoButton, &QAbstractButton::clicked, this, [=]() { m_ui->spline->OnUserCommand(ID_TANGENT_AUTO); });
    connect(m_ui->inTangentToZeroButton, &QAbstractButton::clicked, this, [=]() { m_ui->spline->OnUserCommand(ID_TANGENT_IN_ZERO); });
    connect(m_ui->inTangentToStepButton, &QAbstractButton::clicked, this, [=]() { m_ui->spline->OnUserCommand(ID_TANGENT_IN_STEP); });
    connect(m_ui->inTangentToLinearButton, &QAbstractButton::clicked, this, [=]() { m_ui->spline->OnUserCommand(ID_TANGENT_IN_LINEAR); });
    connect(m_ui->outTangentToZerobutton, &QAbstractButton::clicked, this, [=]() { m_ui->spline->OnUserCommand(ID_TANGENT_OUT_ZERO); });
    connect(m_ui->outTangentToStepButton, &QAbstractButton::clicked, this, [=]() { m_ui->spline->OnUserCommand(ID_TANGENT_OUT_STEP); });
    connect(m_ui->outTangentToLinearButton, &QAbstractButton::clicked, this, [=]() { m_ui->spline->OnUserCommand(ID_TANGENT_OUT_LINEAR); });
    connect(m_ui->fitSplinesHorizontalButton, &QAbstractButton::clicked, this, [=]() { m_ui->spline->OnUserCommand(ID_SPLINE_FIT_X); });
    connect(m_ui->fitSplinesVerticalButton, &QAbstractButton::clicked, this, [=]() { m_ui->spline->OnUserCommand(ID_SPLINE_FIT_Y); });
    connect(m_ui->splineSnapGridX, &QAbstractButton::clicked, this, [=]() { m_ui->spline->OnUserCommand(ID_SPLINE_SNAP_GRID_X); });
    connect(m_ui->splineSnapGridY, &QAbstractButton::clicked, this, [=]() { m_ui->spline->OnUserCommand(ID_SPLINE_SNAP_GRID_Y); });
    connect(m_ui->previousKeyButton, &QAbstractButton::clicked, this, [=]() { m_ui->spline->OnUserCommand(ID_SPLINE_PREVIOUS_KEY); });
    connect(m_ui->nextKeyButton, &QAbstractButton::clicked, this, [=]() { m_ui->spline->OnUserCommand(ID_SPLINE_NEXT_KEY); });
    connect(m_ui->removeAllExceptSelectedButton, &QAbstractButton::clicked, this, [=]() { m_ui->spline->OnUserCommand(ID_SPLINE_FLATTEN_ALL); });

    connect(m_ui->timelineSlider, &AzQtComponents::SliderInt::valueChanged, this, [=](int value) { SetTime(value / 60.0f); });

    connect(m_ui->spline, &SplineWidget::beforeChange, this, &CTimeOfDayDialog::OnBeforeSplineChange);
    connect(m_ui->spline, &SplineWidget::change, this, [=]() { OnSplineChange(m_ui->spline); });
    connect(m_ui->spline, &SplineWidget::scrollZoomRequested, this, &CTimeOfDayDialog::OnSplineCtrlScrollZoom);
    connect(m_ui->spline, &SplineWidget::timeChange, this, &CTimeOfDayDialog::OnTimelineCtrlChange);
    connect(m_ui->spline, &SplineWidget::keySelectionChange, this, [=]() { SetTimeFromActiveKey(); });

    connect(m_ui->colorGradient, &CColorGradientCtrl::beforeChange, this, &CTimeOfDayDialog::OnBeforeSplineChange);
    connect(m_ui->colorGradient, &CColorGradientCtrl::change, this, [=]() { OnSplineChange(m_ui->colorGradient); });
    connect(m_ui->colorGradient, &CColorGradientCtrl::activeKeyChange, this, [=]() { SetTimeFromActiveKey(true); });
    //connect(m_timelineCtrl.data(), &TimelineWidget::startChange, this, &CTimeOfDayDialog::OnBeforeSplineChange);
    connect(m_timelineCtrl, &TimelineWidget::change, this, &CTimeOfDayDialog::OnTimelineCtrlChange);

    //ITimeOfDay *pTimeOfDay = gEnv->p3DEngine->GetTimeOfDay();
    //m_ui->currentTimeEdit->setTime(qTimeFromFloat(pTimeOfDay->GetTime()));

    GetIEditor()->GetUndoManager()->AddListener(this);
}

void CTimeOfDayDialog::HdrPropertySelected(IVariable* v)
{
    if (v)
    {
        m_ui->parameters->ClearSelection();
    }
    ResetSpline(v);
}

void CTimeOfDayDialog::StartTimeChanged([[maybe_unused]] const QTime& time)
{
    float converted = TimeOfDayDetails::floatFromQTime(m_ui->startTimeEdit->time());
    ITimeOfDay* pTimeOfDay = gEnv->p3DEngine->GetTimeOfDay();
    ITimeOfDay::SAdvancedInfo advInfo;
    pTimeOfDay->GetAdvancedInfo(advInfo);
    advInfo.fStartTime = converted;
    pTimeOfDay->SetAdvancedInfo(advInfo);
}

void CTimeOfDayDialog::EndTimeChanged([[maybe_unused]] const QTime& time)
{
    float converted = TimeOfDayDetails::floatFromQTime(m_ui->endTimeEdit->time());
    ITimeOfDay* pTimeOfDay = gEnv->p3DEngine->GetTimeOfDay();
    ITimeOfDay::SAdvancedInfo advInfo;
    pTimeOfDay->GetAdvancedInfo(advInfo);
    advInfo.fEndTime = converted;
    pTimeOfDay->SetAdvancedInfo(advInfo);
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDayDialog::CreateProperties()
{
    m_pVars = new CVarBlock;

    std::map<QString, IVariable*> groups;

    ITimeOfDay* pTimeOfDay = gEnv->p3DEngine->GetTimeOfDay();
    for (int i = 0; i < pTimeOfDay->GetVariableCount(); i++)
    {
        ITimeOfDay::SVariableInfo varInfo;
        if (!pTimeOfDay->GetVariableInfo(i, varInfo))
        {
            continue;
        }

        if (TimeOfDayDetails::SkipUserInterface(varInfo.nParamId))
        {
            continue;
        }

        if (!varInfo.pInterpolator)
        {
            continue;
        }

        IVariable* pGroupVar = stl::find_in_map(groups, varInfo.group, 0);
        if (!pGroupVar)
        {
            // Create new group
            pGroupVar = new CVariableArray();
            pGroupVar->SetName(varInfo.group);
            pGroupVar->SetUserData(-1);
            if (strcmp(varInfo.group, "HDR") == 0) // HDR parameters should go into the separate HDR pane.
            {
                m_pHDRPane->variables()->AddVariable(pGroupVar);
            }
            else
            {
                m_pVars->AddVariable(pGroupVar);
            }
            groups[varInfo.group] = pGroupVar;
        }

        IVariable* pVar = 0;
        if (varInfo.type == ITimeOfDay::TYPE_COLOR)
        {
            //////////////////////////////////////////////////////////////////////////
            // Add Var.
            pVar = new CVariable<Vec3>();
            pVar->SetDataType(IVariable::DT_COLOR);
            pVar->Set(Vec3(varInfo.fValue[0], varInfo.fValue[1], varInfo.fValue[2]));
        }
        else if (varInfo.type == ITimeOfDay::TYPE_FLOAT)
        {
            // Add Var.
            pVar = new CVariable<float>();
            pVar->Set(varInfo.fValue[0]);
            pVar->SetLimits(varInfo.fValue[1], varInfo.fValue[2]);
        }

        if (pVar)
        {
            pVar->SetName(varInfo.name);
            pVar->SetHumanName(varInfo.displayName);
            pGroupVar->AddVariable(pVar);
            pVar->SetUserData(i);
        }
    }

    m_ui->parameters->AddVarBlock(m_pVars);
    m_ui->parameters->ExpandAll();
    m_ui->parameters->EnableNotifyWithoutValueChange(true);

    m_pHDRPane->properties().AddVarBlock(m_pHDRPane->variables());
    m_pHDRPane->properties().ExpandAll();
    m_pHDRPane->properties().EnableNotifyWithoutValueChange(true);
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDayDialog::OnBeforeSplineChange()
{
    if (CUndo::IsRecording())
    {
        CUndo::Record(new CUndoTimeOfDayObject());
    }
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDayDialog::OnSplineChange(const QWidget* source)
{
    RefreshPropertiesValues();

    if (source == m_ui->spline)
    {
        // Update the time of day settings on spline changes (e.g. keys being moved)
        ITimeOfDay* pTimeOfDay = gEnv->p3DEngine->GetTimeOfDay();
        bool bForceUpdate = m_ui->forceSkyUpdateCheckBox->isChecked();
        pTimeOfDay->Update(true, bForceUpdate);

        m_timelineCtrl->update();
        m_ui->colorGradient->update();
    }
    else if (source == m_timelineCtrl)
    {
        m_ui->colorGradient->update();
        m_ui->spline->update();
    }
    else
    {
        m_ui->spline->SplinesChanged();
        m_ui->spline->update();
        m_timelineCtrl->update();
    }


    if (m_ui->spline->GetSplineCount() > 0)
    {
        ISplineInterpolator* pSpline = m_ui->spline->GetSpline(0);
        if (NULL != pSpline)
        {
            ITimeOfDay* pTimeOfDay = gEnv->p3DEngine->GetTimeOfDay();
            const uint numVars = pTimeOfDay->GetVariableCount();
            for (uint i = 0; i < numVars; ++i)
            {
                ITimeOfDay::SVariableInfo varInfo;
                pTimeOfDay->GetVariableInfo(i, varInfo);
            }
        }
    }
}

/**
 * Update our time based on the currently active key
 */
void CTimeOfDayDialog::SetTimeFromActiveKey(bool useColorGradient)
{
    if (m_ui->spline->GetSplineCount() < 1)
    {
        return;
    }

    ISplineInterpolator* pSpline = m_ui->spline->GetSpline(0);
    if (!pSpline)
    {
        return;
    }

    int activeKey = -1;
    if (useColorGradient)
    {
        // If this method was triggered from our color gradient control, then
        // retrieve its active key
        activeKey = m_ui->colorGradient->GetActiveKey();
    }
    else
    {
        // Otherwise this method was triggered from our main spline control, so
        // we need to find the selected key by cycling through its keys
        int numKeys = pSpline->GetKeyCount();
        for (int i = 0; i < numKeys; ++i)
        {
            if (pSpline->IsKeySelectedAtAnyDimension(i))
            {
                activeKey = i;
                break;
            }
        }
    }

    if (activeKey == -1)
    {
        return;
    }

    SetTime(pSpline->GetKeyTime(activeKey) * m_maxTime);
}

//////////////////////////////////////////////////////////////////////////
float CTimeOfDayDialog::GetTime() const
{
    // This used to get the time from GetIEditor()->GetDocument()->GetCurrentMission()->GetTime(); but that seems like legacy CryEngine, so we're
    // switching to grabbing it from the 3DEngine if possible
    float time = .0f;
    if (gEnv->p3DEngine->GetTimeOfDay())
    {
        time = gEnv->p3DEngine->GetTimeOfDay()->GetTime();
    }
    return time;
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDayDialog::SetTimeRange(float fTimeStart, float fTimeEnd, float fSpeed)
{
    m_ui->startTimeEdit->setTime(TimeOfDayDetails::qTimeFromFloat(fTimeStart));
    m_ui->endTimeEdit->setTime(TimeOfDayDetails::qTimeFromFloat(fTimeEnd));

    m_ui->playSpeedDoubleSpinBox->setValue(fSpeed);

    ITimeOfDay* pTimeOfDay = gEnv->p3DEngine->GetTimeOfDay();
    ITimeOfDay::SAdvancedInfo advInfo;
    pTimeOfDay->GetAdvancedInfo(advInfo);
    advInfo.fStartTime = fTimeStart;
    advInfo.fEndTime = fTimeEnd;
    advInfo.fAnimSpeed = fSpeed;
    pTimeOfDay->SetAdvancedInfo(advInfo);
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDayDialog::RefreshPropertiesValues()
{
    m_ui->parameters->EnableUpdateCallback(false);
    m_pHDRPane->properties().EnableUpdateCallback(false);

    // Interpolate internal values
    ITimeOfDay* pTimeOfDay = gEnv->p3DEngine->GetTimeOfDay();
    for (int i = 0, numVars = pTimeOfDay->GetVariableCount(); i < numVars; i++)
    {
        ITimeOfDay::SVariableInfo varInfo;
        if (!pTimeOfDay->GetVariableInfo(i, varInfo))
        {
            continue;
        }
        if (TimeOfDayDetails::SkipUserInterface(varInfo.nParamId))
        {
            continue;
        }

        IVariable* pVar = FindVariable(varInfo.name);

        if (!pVar)
        {
            continue;
        }

        switch (varInfo.type)
        {
        case ITimeOfDay::TYPE_FLOAT:
            pVar->Set(varInfo.fValue[0]);
            break;
        case ITimeOfDay::TYPE_COLOR:
            pVar->Set(Vec3(varInfo.fValue[0], varInfo.fValue[1], varInfo.fValue[2]));
            break;
        }
    }
    m_ui->parameters->EnableUpdateCallback(true);
    m_pHDRPane->properties().EnableUpdateCallback(true);

    m_pHDRPane->UpdateFilmCurve();

    // Notify that time of day values changed.
    GetIEditor()->Notify(eNotify_OnTimeOfDayChange);
}

void CTimeOfDayDialog::UpdateUI(bool updateProperties)
{
    float timeOfDayInHours = GetTime();

    // update the Current Time edit field and Time Of Day Time Slider
    QTime qTime = TimeOfDayDetails::qTimeFromFloat(timeOfDayInHours);

    QSignalBlocker sliderBlocker(m_ui->timelineSlider);
    QSignalBlocker editBlocker(m_ui->currentTimeEdit);

    int v = qTime.msecsSinceStartOfDay();
    v /= 60000;
    m_ui->timelineSlider->setValue(v);
    m_ui->currentTimeEdit->setTime(qTime);

    m_ui->spline->SetTimeMarker(timeOfDayInHours / m_maxTime);
    m_ui->colorGradient->SetTimeMarker(timeOfDayInHours / m_maxTime);

    if (updateProperties)
    {
        RefreshPropertiesValues();
    }
    else
    {
        GetIEditor()->Notify(eNotify_OnTimeOfDayChange);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDayDialog::SetTime(float time)
{
    const bool bForceUpdate = m_ui->forceSkyUpdateCheckBox->isChecked();
    ITimeOfDay* pTimeOfDay = gEnv->p3DEngine->GetTimeOfDay();

    // This is probably legacy and deprecated, but leaving it here just in case it's needed by some legacy game.
    if (GetIEditor()->GetDocument()->GetCurrentMission())
    {
        GetIEditor()->GetDocument()->GetCurrentMission()->SetTime(time);
    }

    // pTimeOfDay->SetTime will trigger a ESYSTEM_EVENT_TIME_OF_DAY_SET, which in turn will result in UpdateUI() being called
    pTimeOfDay->SetTime(time, bForceUpdate);
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDayDialog::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnCloseScene:
    case eNotify_OnBeginNewScene:
    case eNotify_OnBeginSceneOpen:
    {
        // prevent crash during redraw which can happen before eNotify_OnEndSceneOpen
        m_ui->spline->RemoveAllSplines();
        m_ui->colorGradient->SetSpline(0);
    }
    break;
    case eNotify_OnEndSceneOpen:
    case eNotify_OnEndNewScene:
    {
        UpdateValues();

        m_pHDRPane->properties().ClearSelection();
        if (ReflectedPropertyItem* pSelectedItem = m_ui->parameters->GetSelectedItem())
        {
            if (IVariable* pVar = pSelectedItem->GetVariable())
            {
                ResetSpline(pVar);
            }
        }
    }
    break;

    case eNotify_OnIdleUpdate:
        if (m_ui->actionPlayPause->isChecked())
        {
            ITimeOfDay* pTimeOfDay = gEnv->p3DEngine->GetTimeOfDay();
            float fHour = pTimeOfDay->GetTime();

            ITimeOfDay::SAdvancedInfo advInfo;
            pTimeOfDay->GetAdvancedInfo(advInfo);
            // get the TOD cycle speed from UI
            advInfo.fAnimSpeed = m_ui->playSpeedDoubleSpinBox->value();
            float dt = gEnv->pTimer->GetFrameTime();
            float fTime = fHour + dt * advInfo.fAnimSpeed;
            if (fTime > advInfo.fEndTime)
            {
                fTime = advInfo.fStartTime;
            }
            if (fTime < advInfo.fStartTime)
            {
                fTime = advInfo.fEndTime;
            }
            SetTime(fTime);
        }
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDayDialog::OnTimelineCtrlChange()
{
    float fTime = m_timelineCtrl->GetTimeMarker();
    SetTime(fTime * m_maxTime);
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDayDialog::OnChangeTimeAnimSpeed(double value)
{
    ITimeOfDay* pTimeOfDay = gEnv->p3DEngine->GetTimeOfDay();
    ITimeOfDay::SAdvancedInfo advInfo;
    pTimeOfDay->GetAdvancedInfo(advInfo);
    // set current speed based on if we animating it currently or not
    advInfo.fAnimSpeed = value;
    pTimeOfDay->SetAdvancedInfo(advInfo);
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDayDialog::OnSplineCtrlScrollZoom()
{
    if (m_ui->spline && m_ui->colorGradient)
    {
        m_ui->colorGradient->SetZoom(m_ui->spline->GetZoom().x);
        m_ui->colorGradient->SetOrigin(m_ui->spline->GetScrollOffset().x);
        m_ui->colorGradient->update();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDayDialog::OnImport()
{
    char szFilters[] = "Time Of Day Settings (*.xml);;Time Of Day Settings Old (*.tod)";
    QString fileName;

    if (CFileUtil::SelectFile(szFilters, GetIEditor()->GetLevelFolder(), fileName))
    {
        XmlNodeRef root = GetISystem()->LoadXmlFromFile(fileName.toStdString().c_str());
        if (root)
        {
            ITimeOfDay* pTimeOfDay = gEnv->p3DEngine->GetTimeOfDay();
            float fTime = GetTime();
            pTimeOfDay->Serialize(root, true);
            pTimeOfDay->SetTime(fTime, true);

            UpdateValues();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDayDialog::OnExport()
{
    char szFilters[] = "Time Of Day Settings (*.xml)";
    QString fileName;
    if (CFileUtil::SelectSaveFile(szFilters, "xml", GetIEditor()->GetLevelFolder(), fileName))
    {
        // Write the light settings into the archive
        XmlNodeRef node = XmlHelpers::CreateXmlNode("TimeOfDay");
        ITimeOfDay* pTimeOfDay = gEnv->p3DEngine->GetTimeOfDay();
        pTimeOfDay->Serialize(node, false);
        XmlHelpers::SaveXmlNode(GetIEditor()->GetFileUtil(), node, fileName.toStdString().c_str());
    }
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDayDialog::OnExpandAll()
{
    m_ui->parameters->ExpandAll();
    m_pHDRPane->properties().ExpandAll();
}
//////////////////////////////////////////////////////////////////////////
void CTimeOfDayDialog::OnResetToDefaultValues()
{
    auto answer = QMessageBox::question(QApplication::activeWindow(), "Reset Values", "Are you sure you want to reset all values to their default values?");

    if (answer == QMessageBox::Yes)
    {
        ITimeOfDay* pTimeOfDay = gEnv->p3DEngine->GetTimeOfDay();

        // Load the default time of day settings and use those to reset the time of day.
        XmlNodeRef root = GetISystem()->LoadXmlFromFile("default_time_of_day.xml");
        if (root)
        {
            pTimeOfDay->Serialize(root, true);
        }
        else
        {
            QMessageBox::warning(QApplication::activeWindow(), "Reset Values", "Unable to read default time of day file (Editor/default_time_of_day.xml), initializing variables to default values.", QMessageBox::Ok);

            // If for some reason the file is  missing or corrupted, recreate the variables with their default states.
            // Note that these variables may be out of sync with the default_time_of_day.xml file.
            pTimeOfDay->ResetVariables();
        }

        ITimeOfDay::SAdvancedInfo advInfo;
        pTimeOfDay->GetAdvancedInfo(advInfo);
        SetTimeRange(advInfo.fStartTime, advInfo.fEndTime, advInfo.fAnimSpeed);
        RefreshPropertiesValues();

        m_pHDRPane->properties().ClearSelection();
        ReflectedPropertyItem* selectedItem = m_ui->parameters->GetSelectedItem();
        if (selectedItem)
        {
            IVariable* var = selectedItem->GetVariable();
            if (var)
            {
                ResetSpline(var);
            }
        }
    }
}
//////////////////////////////////////////////////////////////////////////
void CTimeOfDayDialog::OnCollapseAll()
{
    m_ui->parameters->CollapseAll();
    m_pHDRPane->properties().CollapseAll();
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDayDialog::OnUpdateProperties(IVariable* pVar)
{
    ITimeOfDay* pTimeOfDay = gEnv->p3DEngine->GetTimeOfDay();

    int nIndex = -1;
    if (pVar)
    {
        nIndex = pVar->GetUserData().toInt();
    }

    if (nIndex != -1)
    {
        ITimeOfDay::SVariableInfo varInfo;
        if (pTimeOfDay->GetVariableInfo(nIndex, varInfo))
        {
            float fTime = GetTime();
            float fSplineTime = fTime / m_maxTime;

            const float cNearestKeySearchEpsilon = 0.00001f;
            int nKey = varInfo.pInterpolator->FindKey(fSplineTime, cNearestKeySearchEpsilon);
            int nLastKey = varInfo.pInterpolator->GetKeyCount() - 1;

            if (CUndo::IsRecording())
            {
                CUndo::Record(new CUndoTimeOfDayObject());
            }

            switch (varInfo.type)
            {
            case ITimeOfDay::TYPE_FLOAT:
            {
                float fVal = 0;
                pVar->Get(fVal);
                if (m_ui->actionStartStopRecording->isChecked())
                {
                    if (nKey < 0)
                    {
                        nKey = varInfo.pInterpolator->InsertKeyFloat(fSplineTime, fVal);
                    }
                    else
                    {
                        varInfo.pInterpolator->SetKeyValueFloat(nKey, fVal);
                        if (nKey == 0)
                        {
                            varInfo.pInterpolator->SetKeyValueFloat(nLastKey, fVal);
                        }
                        else if (nKey == nLastKey)
                        {
                            varInfo.pInterpolator->SetKeyValueFloat(0, fVal);
                        }
                    }
                    if (m_ui->spline && m_ui->spline->GetDefaultKeyTangentType() != SPLINE_KEY_TANGENT_NONE)
                    {
                        TimeOfDayDetails::SetKeyTangentType(varInfo.pInterpolator, nKey, m_ui->spline->GetDefaultKeyTangentType());
                    }
                }

                float v3[3] = {fVal, varInfo.fValue[1], varInfo.fValue[2]};
                pTimeOfDay->SetVariableValue(nIndex, v3);
            }
            break;
            case ITimeOfDay::TYPE_COLOR:
            {
                Vec3 vVal;
                pVar->Get(vVal);
                float v3[3] = {vVal.x, vVal.y, vVal.z};
                if (m_ui->actionStartStopRecording->isChecked())
                {
                    if (nKey < 0)
                    {
                        nKey = varInfo.pInterpolator->InsertKeyFloat3(fSplineTime, v3);
                    }
                    else
                    {
                        varInfo.pInterpolator->SetKeyValueFloat3(nKey, v3);
                        if (nKey == 0)
                        {
                            varInfo.pInterpolator->SetKeyValueFloat3(nLastKey, v3);
                        }
                        else if (nKey == nLastKey)
                        {
                            varInfo.pInterpolator->SetKeyValueFloat3(0, v3);
                        }
                    }
                    if (m_ui->spline && m_ui->spline->GetDefaultKeyTangentType() != SPLINE_KEY_TANGENT_NONE)
                    {
                        TimeOfDayDetails::SetKeyTangentType(varInfo.pInterpolator, nKey, m_ui->spline->GetDefaultKeyTangentType());
                    }
                }
                pTimeOfDay->SetVariableValue(nIndex, v3);

                m_ui->colorGradient->update();
            }
            break;
            }

            if (m_ui->spline)
            {
                m_ui->spline->update();
            }

            if (varInfo.nParamId == ITimeOfDay::PARAM_HDR_FILMCURVE_SHOULDER_SCALE
                || varInfo.nParamId == ITimeOfDay::PARAM_HDR_FILMCURVE_LINEAR_SCALE
                || varInfo.nParamId == ITimeOfDay::PARAM_HDR_FILMCURVE_TOE_SCALE
                || varInfo.nParamId == ITimeOfDay::PARAM_HDR_FILMCURVE_WHITEPOINT)
            {
                m_pHDRPane->UpdateFilmCurve();
            }

            bool bForceUpdate = m_ui->forceSkyUpdateCheckBox->isChecked();
            pTimeOfDay->Update(false, bForceUpdate);

            GetIEditor()->Notify(eNotify_OnTimeOfDayChange);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDayDialog::OnPropertySelected(IVariable *pVar)
{
    if (pVar)
    {
        m_pHDRPane->properties().ClearSelection();
    }

    if (pVar && pVar->GetType() == IVariable::ARRAY)
    {
        pVar = nullptr;
    }

    ResetSpline(pVar);
}


void CTimeOfDayDialog::ResetSpline(IVariable* pVar)
{
    if (pVar)
    {
        ITimeOfDay* pTimeOfDay = gEnv->p3DEngine->GetTimeOfDay();
        ITimeOfDay::SVariableInfo varInfo;
        int index = pVar->GetUserData().toInt();
        if (!pTimeOfDay->GetVariableInfo(index, varInfo))
        {
            return;
        }

        m_ui->spline->SetTimeRange(Range(0, 1.0f));
        m_ui->spline->RemoveAllSplines();

        if (varInfo.type == ITimeOfDay::TYPE_COLOR)
        {
            QColor    afColorArray[4];
            afColorArray[0] = QColor(255, 0, 0);
            afColorArray[1] = QColor(0, 255, 0);
            afColorArray[2] = QColor(0, 0, 255);
            afColorArray[3] = QColor(255, 0, 255); //Pink... so you know it's wrong if you see it.
            m_ui->spline->AddSpline(varInfo.pInterpolator, 0, afColorArray);
            m_ui->spline->SetValueRange(Range(0, 1));

            m_ui->colorGradient->SetSpline(varInfo.pInterpolator, TRUE);
            m_ui->colorGradient->setEnabled(true);
            m_ui->colorGradient->update();
        }
        else
        {
            m_ui->colorGradient->setEnabled(false);
            m_ui->colorGradient->SetSpline(0);
            m_ui->colorGradient->update();

            m_ui->spline->SetValueRange(Range(varInfo.fValue[1], varInfo.fValue[2]));
            m_ui->spline->AddSpline(varInfo.pInterpolator, 0, QColor(0, 255, 0));
        }
        m_ui->spline->SetSplineSet(0);
        m_ui->spline->FitSplineToViewWidth();
        m_ui->spline->FitSplineHeightToValueRange();
    }
    else
    {
        m_ui->spline->RemoveAllSplines();
        m_ui->colorGradient->setEnabled(false);
        m_ui->colorGradient->SetSpline(0);
    }
    m_ui->spline->update();
}


//////////////////////////////////////////////////////////////////////////
void CTimeOfDayDialog::OnHold()
{
    XmlNodeRef node = XmlHelpers::CreateXmlNode("TimeOfDay");
    GetIEditor()->Get3DEngine()->GetTimeOfDay()->Serialize(node, false);
    node->saveToFile((Path::GetUserSandboxFolder() + "TimeOfDayHold.xml").toUtf8().data());
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDayDialog::OnFetch()
{
    XmlNodeRef node = XmlHelpers::LoadXmlFromFile((Path::GetUserSandboxFolder() + "TimeOfDayHold.xml").toUtf8().data());
    if (node)
    {
        GetIEditor()->Get3DEngine()->GetTimeOfDay()->Serialize(node, true);
        UpdateValues();
        UpdateUI();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDayDialog::OnUndo()
{
    GetIEditor()->Undo();
    m_ui->spline->update();
    m_ui->colorGradient->update();
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDayDialog::OnRedo()
{
    GetIEditor()->Redo();
    m_ui->spline->update();
    m_ui->colorGradient->update();
}

//////////////////////////////////////////////////////////////////////////
IVariable* CTimeOfDayDialog::FindVariable(const char* name) const
{
    IVariable* pVar = m_pVars->FindVariable(name);
    if (!pVar)
    {
        pVar = m_pHDRPane->variables()->FindVariable(name);
    }

    return pVar;
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDayDialog::CopyAllProperties()
{
    CClipboard clipboard(this);
    XmlNodeRef collectionNode = XmlHelpers::CreateXmlNode("PropertyCtrls");

    ReflectedPropertyItem* pRoot = m_ui->parameters->GetRootItem();
    if (pRoot) // Main properties
    {
        XmlNodeRef rootNode = collectionNode->newChild("PropertyCtrl");
        for (int i = 0; i < pRoot->GetChildCount(); ++i)
        {
            m_ui->parameters->CopyItem(rootNode, pRoot->GetChild(i), true);
        }
    }

    pRoot = m_pHDRPane->properties().GetRootItem();
    if (pRoot) // HDR properties
    {
        XmlNodeRef rootNode = collectionNode->newChild("PropertyCtrl");
        for (int i = 0; i < pRoot->GetChildCount(); ++i)
        {
            m_pHDRPane->properties().CopyItem(rootNode, pRoot->GetChild(i), true);
        }
    }

    clipboard.Put(collectionNode);
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDayDialog::PasteAllProperties()
{
    CClipboard clipboard(this);
    CUndo undo("Paste Properties");

    XmlNodeRef collectionNode = clipboard.Get();
    if (collectionNode != NULL && collectionNode->isTag("PropertyCtrls")
        && collectionNode->getChildCount() == 2)
    {
        // Main properties
        XmlNodeRef rootNode = collectionNode->getChild(0);
        m_ui->parameters->SetValuesFromNode(rootNode);

        // HDR properties
        rootNode = collectionNode->getChild(1);
        m_pHDRPane->properties().SetValuesFromNode(rootNode);
    }
}

void CTimeOfDayDialog::SignalNumUndoRedo(const unsigned int& numUndo, const unsigned int& numRedo)
{
    m_ui->actionUndo->setEnabled(numUndo > 0);
    m_ui->actionRedo->setEnabled(numRedo > 0);
}

void CTimeOfDayDialog::resizeEvent([[maybe_unused]] QResizeEvent* event)
{
    if (m_ui->spline->GetSplineCount() == 0)
    {
        m_ui->spline->FitSplineToViewWidth();
        m_ui->spline->FitSplineToViewHeight();
    }
}

CUndoTimeOfDayObject::CUndoTimeOfDayObject()
{
    m_undo = XmlHelpers::CreateXmlNode("Undo");
    GetIEditor()->Get3DEngine()->GetTimeOfDay()->Serialize(m_undo, false);
}

void CUndoTimeOfDayObject::Undo(bool bUndo)
{
    if (bUndo)
    {
        m_redo = XmlHelpers::CreateXmlNode("Redo");
        GetIEditor()->Get3DEngine()->GetTimeOfDay()->Serialize(m_redo, false);
    }

    GetIEditor()->Get3DEngine()->GetTimeOfDay()->Serialize(m_undo, true);
    UpdateTimeOfDayDialog();
}

void CUndoTimeOfDayObject::Redo()
{
    GetIEditor()->Get3DEngine()->GetTimeOfDay()->Serialize(m_redo, true);
    UpdateTimeOfDayDialog();
}

void CUndoTimeOfDayObject::UpdateTimeOfDayDialog()
{
    CTimeOfDayDialog* targetDialog = FindViewPane<CTimeOfDayDialog>(CTimeOfDayDialog::ClassName());
    if (targetDialog != nullptr)
    {
        targetDialog->UpdateValues();
    }
}

#include <moc_TimeOfDayDialog.cpp>
