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

#include <ScriptCanvas/Core/ModifiableDatumView.h>

#include <ScriptCanvas/Variable/VariableBus.h>
#include <ScriptCanvas/Variable/GraphVariable.h>

namespace ScriptCanvas
{
    ////////////////////////
    // ModifiableDatumView
    ////////////////////////

    ModifiableDatumView::ModifiableDatumView()
        : m_datum(nullptr)
    {
    }

    ModifiableDatumView::ModifiableDatumView(const AZ::EntityId& uniqueId, const VariableId& variableId)
        : ModifiableDatumView()
    {
        ConfigureView(uniqueId, variableId);
    }

    ModifiableDatumView::~ModifiableDatumView()
    {
    }

    bool ModifiableDatumView::IsValid() const
    {
        return m_datum != nullptr;
    }

    bool ModifiableDatumView::IsType(const ScriptCanvas::Data::Type& dataType) const
    {
        return m_datum ? (m_datum->GetType() == dataType) : false;
    }

    ScriptCanvas::Data::Type ModifiableDatumView::GetDataType() const
    {
        return m_datum ? m_datum->GetType() : ScriptCanvas::Data::Type::Invalid();
    }

    void ModifiableDatumView::SetDataType(const ScriptCanvas::Data::Type& dataType)
    {
        if (m_datum)
        {
            m_datum->SetType(dataType);
        }
    }

    const Datum* ModifiableDatumView::GetDatum() const
    {
        return m_datum;
    }

    Datum ModifiableDatumView::CloneDatum()
    {
        return m_datum ? Datum((*m_datum)) : Datum();
    }

    void ModifiableDatumView::SetLabel(const AZStd::string& label)
    {
        m_datum->SetLabel(label);
    }

    void ModifiableDatumView::SetToDefaultValueOfType()
    {
        if (m_datum)
        {
            m_datum->SetToDefaultValueOfType();
        }
    }

    void ModifiableDatumView::AssignToDatum(Datum&& datum)
    {
        if (m_datum)
        {
            (*m_datum) = AZStd::move(datum);
            SignalModification();
        }
    }

    void ModifiableDatumView::AssignToDatum(const Datum& datum)
    {
        if (m_datum)
        {
            ComparisonOutcome comparisonResult = (*m_datum) != datum;
            
            if (!comparisonResult || comparisonResult.GetValue())
            {
                (*m_datum) = datum;
                SignalModification();
            }
        }
    }

    void ModifiableDatumView::ReconfigureDatumTo(Datum&& datum)
    {
        if (m_datum)
        {
            m_datum->ReconfigureDatumTo(AZStd::move(datum));
            SignalModification();
        }
    }

    void ModifiableDatumView::HardCopyDatum(const Datum& datum)
    {
        if (m_datum)
        {
            m_datum->DeepCopyDatum(datum);
            SignalModification();
        }
    }

    void ModifiableDatumView::RelabelDatum(const AZStd::string& datumName)
    {
        // If it's a variable datum. We want to just leave it alone.
        if (m_scopedVariableId.IsValid() || m_datum == nullptr)
        {
            return;
        }

        m_datum->SetLabel(datumName);
    }

    void ModifiableDatumView::SetVisibility(AZ::Crc32 visibility)
    {
        if (m_scopedVariableId.IsValid() || m_datum == nullptr)
        {
            return;
        }

        m_datum->SetVisibility(visibility);
    }

    AZ::Crc32 ModifiableDatumView::GetVisibility() const
    {
        return m_datum ? m_datum->GetVisibility() : AZ::Crc32();
    }

    Datum* ModifiableDatumView::ModifyDatum()
    {
        return m_datum;
    }

    void ModifiableDatumView::ConfigureView(GraphVariable& graphVariable)
    {
        SignalModification();

        m_datum = &graphVariable.m_datum;

        m_scopedVariableId = graphVariable.GetGraphScopedId();
    }

    void ModifiableDatumView::ConfigureView(Datum& datum)
    {
        SignalModification();

        m_datum = &datum;
        m_scopedVariableId.Clear();
    }

    void ModifiableDatumView::ConfigureView(const ScriptCanvasId& scriptCanvasId, const VariableId& variableId)
    {
        GraphVariable* variable = nullptr;

        VariableRequestBus::EventResult(variable, GraphScopedVariableId(scriptCanvasId, variableId), &VariableRequests::GetVariable);

        if (variable)
        {
            ConfigureView((*variable));
        }
    }

    void ModifiableDatumView::SignalModification()
    {
        if (m_scopedVariableId.IsValid())
        {
            VariableNotificationBus::Event(m_scopedVariableId, &VariableNotifications::OnVariableValueChanged);
        }
    }
}
