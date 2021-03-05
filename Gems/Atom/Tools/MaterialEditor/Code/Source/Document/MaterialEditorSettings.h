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
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/any.h>

#include <Atom/Document/MaterialEditorSettingsBus.h>

namespace MaterialEditor
{
    class MaterialEditorSettings
        : public MaterialEditorSettingsRequestBus::Handler
    {
    public:
        AZ_RTTI(MaterialEditorSettings, "{9C6B6E20-A28E-45DD-85BE-68CA35E9305E}");
        AZ_CLASS_ALLOCATOR(MaterialEditorSettings, AZ::SystemAllocator, 0);

        MaterialEditorSettings();
        ~MaterialEditorSettings();

        AZ::Outcome<AZStd::any> GetProperty(AZStd::string_view name) const override;
        AZ::Outcome<AZStd::string> GetStringProperty(AZStd::string_view name) const override;
        AZ::Outcome<bool> GetBoolProperty(AZStd::string_view name) const override;

        void SetProperty(AZStd::string_view name, const AZStd::any& value) override;
        void SetStringProperty(AZStd::string_view name, AZStd::string_view stringValue) override;
        void SetBoolProperty(AZStd::string_view name, bool boolValue) override;

    private:
        AZStd::unordered_map<AZStd::string, AZStd::any> m_propertyMap;
    };

} // namespace MaterialEditor
