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

#include <EditorPythonBindings/EditorPythonBindingsSymbols.h>

#include <AzCore/Component/Component.h>
#include <EditorPythonBindings/EditorPythonBindingsBus.h>
#include <Source/PythonCommon.h>
#include <pybind11/pybind11.h>

namespace EditorPythonBindings
{
    namespace Internal
    {
        struct StaticPropertyHolderMap;
    }

    //! Inspects the Behavior Context for methods to expose as Python bindings
    class PythonReflectionComponent
        : public AZ::Component
        , private EditorPythonBindings::EditorPythonBindingsNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(PythonReflectionComponent, PythonReflectionComponentTypeId, AZ::Component);

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // EditorPythonBindings::EditorPythonBindingsNotificationBus interface implementation
        void OnPreFinalize() override;
        void OnImportModule(PyObject* module) override;
        ////////////////////////////////////////////////////////////////////////

    private:
        void ExportGlobalsFromBehaviorContext(pybind11::module parentModule);
        AZStd::shared_ptr<Internal::StaticPropertyHolderMap> m_staticPropertyHolderMap;
    };
}
