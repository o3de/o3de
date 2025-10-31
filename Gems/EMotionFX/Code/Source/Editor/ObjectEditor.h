/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

        //! Filter Reflected Properties
        void SetFilterString(QString filterString);
        //! Returns true if any Node in the Reflected Property Editor is displayed
        bool HasDisplayedNodes() const;

        void* GetObject() { return m_object; }

    public slots:
        void InvalidateAll();
        void InvalidateValues();

    private:
        void*                                       m_object;
        AzToolsFramework::ReflectedPropertyEditor*  m_propertyEditor;
        static const int                            s_propertyLabelWidth;
    };
} // namespace EMotionFX
