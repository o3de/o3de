/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
#endif

#include <QWidget>
#include <QMenu>
#include <QPoint>
#include <QVBoxLayout>
#include <QLineEdit>

namespace OpenParticleSystemEditor
{
    class ExploreInspector
        : public QWidget
    {
        Q_OBJECT;

    public:
        AZ_CLASS_ALLOCATOR(ExploreInspector, AZ::SystemAllocator, 0);

        explicit ExploreInspector(QWidget* parent = nullptr);
        ~ExploreInspector();

    Q_SIGNALS:

    private:
        QVBoxLayout* m_layout = nullptr;
    };
} // namespace OpenParticleSystemEditor
