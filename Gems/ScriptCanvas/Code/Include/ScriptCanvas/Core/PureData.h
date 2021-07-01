/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Datum.h>
#include <ScriptCanvas/Data/PropertyTraits.h>

namespace AZ
{
    class BehaviorClass;
    struct BehaviorValueParameter;
    class ReflectContext;
}

namespace ScriptCanvas
{
    struct PropertyAccount
    {
        AZStd::unordered_map<SlotId, Data::GetterWrapper> m_gettersByInputSlot;
        AZStd::unordered_map<SlotId, Data::SetterWrapper> m_settersByInputSlot;
        // The first slot id of the pair is the Getter SlotId, the second slot id of the pair is the Setter SlotID
        AZStd::unordered_map<AZStd::string, AZStd::pair<SlotId, SlotId>> m_getterSetterIdPairs;
    };

    class PureData
        : public Node
    {
    public:
        AZ_COMPONENT(PureData, "{8B80FF54-0786-4FEE-B4A3-12907EBF8B75}", Node);

        static void Reflect(AZ::ReflectContext* reflectContext);
        static const char* k_getThis;
        static const char* k_setThis;

        const AZStd::unordered_map<AZStd::string, AZStd::pair<SlotId, SlotId>>& GetPropertyNameSlotMap() const;

        AZ_INLINE AZ::Outcome<DependencyReport, void> GetDependencies() const override { return AZ::Success(DependencyReport{}); }

        ~PureData() override;

    protected:
        void AddInputAndOutputTypeSlot(const Data::Type& type, const void* defaultValue = nullptr);
        template<typename DatumType>
        void AddDefaultInputAndOutputTypeSlot(DatumType&& defaultValue);
        void AddInputTypeAndOutputTypeSlot(const Data::Type& type);

        bool IsDeprecated() const override { return true; }

        void OnActivate() override;
        void OnInputChanged(const Datum& input, const SlotId& id) override;
        void MarkDefaultableInput() override {}

        AZ_INLINE void OnOutputChanged(const Datum& output) const
        { 
            Slot* slot = GetSlotByName(GetOutputDataName());
            if (slot)
            {
                OnOutputChanged(output, (*slot));
            }
        }
        
        AZ_INLINE void OnOutputChanged(const Datum& output, const Slot& outputSlot) const
        {
            PushOutput(output, outputSlot);
        }

        // push data out
        AZ_INLINE void PushThis()
        {
            auto slotId = GetSlotId(GetInputDataName());

            if (auto setDatum = FindDatum(slotId))
            {
                OnInputChanged(*setDatum, slotId);
            }
            else
            {
                SCRIPTCANVAS_REPORT_ERROR((*this), "No input datum in a PureData class %s. You must push your data manually in OnActivate() if no input is connected!");
            }
        }

        AZStd::string_view GetInputDataName() const;
        AZStd::string_view GetOutputDataName() const;

        void SetInput(const Datum& input, const SlotId& id) override;
        void SetInput(Datum&& input, const SlotId& id) override;
        void SetProperty(const Datum& input, const SlotId& id);
        void CallGetter(const SlotId& getterSlotId);
        bool IsConfigured() { return m_configured; }

        PropertyAccount m_propertyAccount;
        bool m_configured = false;
    };

    template<typename DatumType>
    void PureData::AddDefaultInputAndOutputTypeSlot(DatumType&& defaultValue)
    {
        AddInputDatumSlot(GetInputDataName(), "", Datum::eOriginality::Original, AZStd::forward<DatumType>(defaultValue));
        AddOutputTypeSlot(GetOutputDataName(), "", Data::FromAZType(azrtti_typeid<AZStd::decay_t<DatumType>>()), OutputStorage::Optional);
    }

    template<>
    void PureData::AddDefaultInputAndOutputTypeSlot<Data::Type>(Data::Type&&) = delete;
}
