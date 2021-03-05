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

#include "precompiled.h"
#include "VersionedProperty.h"

#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Serialization/DynamicSerializableField.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Script/ScriptProperty.h>

namespace ScriptEventData
{
    namespace Internal
    {
        void VersionedPropertyConstructor(VersionedProperty* self, AZ::ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 0)
            {
                AZ_Warning("VersionedProperty", false, "Not enough arguments specified to construct VersionedProperty");
                return;
            }

            if (dc.IsString(0))
            {
                AZStd::string data;
                if (dc.ReadArg(0, data))
                {
                    *self = VersionedProperty();
                    self->Set(data.c_str());
                }
            }
            else
            if (dc.IsRegisteredClass(0))
            {
                AZStd::any data;
                if (dc.ReadArg(0, data))
                {
                    *self = VersionedProperty();
                    self->Set(data);
                }
            }
            else
            if (dc.IsNumber(0))
            {
                double value;
                if (dc.ReadArg(0, value))
                {
                    *self = VersionedProperty();
                    self->Set(value);
                }
            }
        }

        void Set(VersionedProperty* self, AZ::ScriptDataContext& dc)
        {
            self->Set(dc);
        }

        void Get(VersionedProperty* self, AZ::ScriptDataContext& dc)
        {
            self->Get(dc);
        }
    }

    VersionedProperty::VersionedProperty(AZ::ScriptDataContext& dc)
    {
        Internal::VersionedPropertyConstructor(this, dc);
    }

    void VersionedProperty::Reflect(AZ::ReflectContext* context)
    {
        VoidType::Reflect(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<VersionedProperty>()
                ->Version(4)
                ->Field("m_id", &VersionedProperty::m_id)
                ->Field("m_label", &VersionedProperty::m_label)
                ->Field("m_version", &VersionedProperty::m_version)
                ->Field("m_versions", &VersionedProperty::m_versions)
                ->Field("m_data", &VersionedProperty::m_data)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<VersionedProperty>("VersionedProperty", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::ChildNameLabelOverride, &VersionedProperty::GetLabel)
                        //->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(0, &VersionedProperty::m_data, "", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
         
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<ScriptEventData::VersionedProperty>()
                ->Constructor<AZ::ScriptDataContext&>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("Set", &Internal::Set)
                ->Method("Get", &Internal::Get)
              ;
        }
    }

    void VersionedProperty::PreSave()
    {
        NewVersion();
    }

    void VersionedProperty::Set(AZ::ScriptDataContext& dc)
    {
        VersionedProperty& newVersion = NewVersion();
        Internal::VersionedPropertyConstructor(&newVersion, dc);
    }

    void VersionedProperty::Get(AZ::ScriptDataContext& dc)
    {
        AZ::ScriptValue<AZStd::any>::StackPush(dc.GetScriptContext()->NativeContext(), m_data);
        dc.PushResult(m_data);
    }
}
