/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <QtForPython/QtForPythonBus.h>
#include <EditorPythonBindings/EditorPythonBindingsBus.h>

namespace QtForPython
{
    class QtForPythonEventHandler;

    class QtForPythonSystemComponent
        : public AZ::Component
        , protected QtForPythonRequestBus::Handler
        , protected EditorPythonBindings::EditorPythonBindingsNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(QtForPythonSystemComponent, "{0C939FBF-8BC9-4CB0-93B8-04140155AA8C}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // QtForPythonRequestBus interface implementation
        bool IsActive() const override;
        QtBootstrapParameters GetQtBootstrapParameters() const override;

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        ////////////////////////////////////////////////////////////////////////
        // EditorPythonBindings::EditorPythonBindingsNotificationBus interface implementation
        void OnImportModule(PyObject* module) override;

    private:
        QtForPythonEventHandler* m_eventHandler = nullptr;
    };
}
