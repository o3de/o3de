/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <ScriptCanvasGem.h>

#include <SystemComponent.h>

#include <Asset/RuntimeAssetSystemComponent.h>
#include <ScriptCanvas/AutoGen/ScriptCanvasAutoGenRegistry.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Connection.h>

#include <ScriptCanvas/Data/DataRegistry.h>
#include <ScriptCanvas/Libraries/Libraries.h>

#include <ScriptCanvas/Debugger/Debugger.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <ScriptCanvas/Libraries/Libraries.h>
#include <ScriptCanvas/Libraries/Math/MathNodeUtilities.h>
#include <ScriptCanvas/Variable/GraphVariableManagerComponent.h>

namespace ScriptCanvas
{
    /////////////////////////////
    // ScriptCanvasModuleCommon
    /////////////////////////////

    ScriptCanvasModuleCommon::ScriptCanvasModuleCommon()
        : AZ::Module()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        m_descriptors.insert(m_descriptors.end(), {
            // System Component
            ScriptCanvas::SystemComponent::CreateDescriptor(),

            // Components
            ScriptCanvas::Connection::CreateDescriptor(),
            ScriptCanvas::Node::CreateDescriptor(),
            ScriptCanvas::Debugger::ServiceComponent::CreateDescriptor(),
            ScriptCanvas::Graph::CreateDescriptor(),
            ScriptCanvas::GraphVariableManagerComponent::CreateDescriptor(),
            ScriptCanvas::RuntimeComponent::CreateDescriptor(),

            // ScriptCanvasBuilder
            ScriptCanvas::RuntimeAssetSystemComponent::CreateDescriptor(),
        });

        ScriptCanvas::InitLibraries();
        AZStd::vector<AZ::ComponentDescriptor*> libraryDescriptors = ScriptCanvas::GetLibraryDescriptors();
        m_descriptors.insert(m_descriptors.end(), libraryDescriptors.begin(), libraryDescriptors.end());

        MathNodeUtilities::RandomEngineInit();
        InitDataRegistry();

        ScriptCanvasModel::Instance().Init();
    }

    ScriptCanvasModuleCommon::~ScriptCanvasModuleCommon()
    {
        MathNodeUtilities::RandomEngineReset();
        ScriptCanvas::ResetLibraries();
        ResetDataRegistry();
    }

    AZ::ComponentTypeList ScriptCanvasModuleCommon::GetCommonSystemComponents() const
    {
        return std::initializer_list<AZ::Uuid> {
            azrtti_typeid<ScriptCanvas::SystemComponent>(),
            azrtti_typeid<ScriptCanvas::RuntimeAssetSystemComponent>(),
            azrtti_typeid<ScriptCanvas::Debugger::ServiceComponent>(),
        };
    }
}
