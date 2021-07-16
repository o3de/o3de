/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/ReflectContext.h>
#include <MCore/Source/Attribute.h>

class QWidget;

namespace EMotionFX
{
    class AnimGraph;
    class ValueParameter;
}

namespace EMStudio
{
    class ValueParameterEditor
    {
    public:
        AZ_RTTI(ValueParameterEditor, "{9A86E080-A759-4906-BB5A-83C43DAFECFC}")
        AZ_CLASS_ALLOCATOR_DECL

        // Required for serialization
        ValueParameterEditor()
            : m_animGraph(nullptr)
            , m_valueParameter(nullptr)
            , m_isReadOnly(false)
        {}

        ValueParameterEditor(EMotionFX::AnimGraph* animGraph,
            const EMotionFX::ValueParameter* valueParameter,
            const AZStd::vector<MCore::Attribute*>& attributes)
            : m_animGraph(animGraph)
            , m_valueParameter(valueParameter)
            , m_attributes(attributes)
        {}

        virtual ~ValueParameterEditor() = default;

        static void Reflect(AZ::ReflectContext* context);

        virtual void setIsReadOnly(bool isReadOnly) { m_isReadOnly = isReadOnly; }
        bool IsReadOnly() const { return m_isReadOnly; }

        void SetAttributes(const AZStd::vector<MCore::Attribute*>& attributes);

        // When the attribute changed without going through this editor, this editor needs
        // to update the current value. If there is a RPE hooked the client is responsible
        // for invalidating the values
        virtual void UpdateValue() = 0;

        virtual QWidget* CreateGizmoWidget(const AZStd::function<void()>&) { return nullptr; }

        AZStd::string GetDescription() const;

    protected:
        EMotionFX::AnimGraph* m_animGraph;
        const EMotionFX::ValueParameter* m_valueParameter;
        AZStd::vector<MCore::Attribute*> m_attributes;
        bool m_isReadOnly;
    };
}
