/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Deprecated/VariableDatumBase.h>

namespace ScriptCanvas
{
    namespace Deprecated
    {
        class VariableDatum
            : public VariableDatumBase
        {
        public:
            AZ_TYPE_INFO(VariableDatum, "{E0315386-069A-4061-AD68-733DCBE393DD}");
            AZ_CLASS_ALLOCATOR(VariableDatum, AZ::SystemAllocator);
            static void Reflect(AZ::ReflectContext* context);

            VariableDatum();
            explicit VariableDatum(const Datum& variableData);
            explicit VariableDatum(Datum&& variableData);

            bool operator==(const VariableDatum& rhs) const;
            bool operator!=(const VariableDatum& rhs) const;

            AZ::Crc32 GetInputControlVisibility() const;
            void SetInputControlVisibility(const AZ::Crc32& inputControlVisibility);
            
            AZ::Crc32 GetVisibility() const;
            void SetVisibility(AZ::Crc32 visibility);

            // Temporary work around. Eventually we're going to want a bitmask so we can have multiple options here.
            // But the editor functionality isn't quite ready for this. So going to bias this towards maintaining a
            // consistent editor rather then consistent data.
            bool ExposeAsComponentInput() const { return m_exposeAsInput; }
            void SetExposeAsComponentInput(bool exposeAsInput) { m_exposeAsInput = exposeAsInput;  }

            void SetExposureCategory(AZStd::string_view exposureCategory) { m_exposureCategory = exposureCategory; }
            AZStd::string_view GetExposureCategory() const { return m_exposureCategory; }

            void GenerateNewId();

        private:
            friend bool VariableDatumVersionConverter(AZ::SerializeContext&, AZ::SerializeContext::DataElementNode&);
            void OnExposureChanged();
            void OnExposureGroupChanged();

            // Still need to make this a proper bitmask, once we have support for multiple
            // input/output attributes. For now, just going to assume it's only the single flag(which is is).
            bool m_exposeAsInput;
            AZ::Crc32 m_inputControlVisibility;
            AZ::Crc32 m_visibility;

            AZStd::string m_exposureCategory;
        };

        struct VariableNameValuePair
        {
            AZ_TYPE_INFO(VariableNameValuePair, "{C1732C54-5E61-4D00-9A39-5B919CF2F8E7}");
            AZ_CLASS_ALLOCATOR(VariableNameValuePair, AZ::SystemAllocator);
            static void Reflect(AZ::ReflectContext* context);

            VariableNameValuePair() = default;
            VariableNameValuePair(AZStd::string_view variableName, const VariableDatum& variableDatum);

            VariableDatum m_varDatum;

            void SetVariableName(AZStd::string_view displayName);
            AZStd::string_view GetVariableName() const;

        private:
            AZStd::string GetDescriptionOverride();

            AZStd::string m_varName;
        };
    }
    
    using VariableDatum = Deprecated::VariableDatum;
}
