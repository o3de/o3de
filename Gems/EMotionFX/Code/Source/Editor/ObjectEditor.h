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


#if !defined(Q_MOC_RUN)
#include <AzCore/RTTI/TypeInfo.h>
#include <QFrame>
#include <QWidget>
#endif


// Forward declarations
namespace AZ { class SerializeContext; }
namespace AzToolsFramework
{
    class ReflectedPropertyEditor;
    class IPropertyEditorNotify;
} // namespace AzToolsFramework

namespace EMotionFX
{
    class ObjectEditor
        : public QFrame
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit ObjectEditor(AZ::SerializeContext* serializeContext, QWidget* parent = nullptr);
        ObjectEditor(AZ::SerializeContext* serializeContext, AzToolsFramework::IPropertyEditorNotify* notify, QWidget* parent = nullptr);

        void AddInstance(void* object, const AZ::TypeId& objectTypeId, void* aggregateInstance = nullptr, void* compareInstance = nullptr);
        void ClearInstances(bool invalidateImmediately);

        void* GetObject() { return m_object; }

    public slots:
        void InvalidateAll();
        void InvalidateValues();

    private:
        void*                                       m_object;
        AzToolsFramework::ReflectedPropertyEditor*  m_propertyEditor;
        static const int                            m_propertyLabelWidth;
    };
} // namespace EMotionFX
