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
