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

#include <SceneAPI/SceneCore/Events/ImportEventContext.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Events
        {
            /////////////
            // PreImportEventContext
            /////////////

            PreImportEventContext::PreImportEventContext(const char* inputDirectory)
                : m_inputDirectory(inputDirectory)
            {
            }

            PreImportEventContext::PreImportEventContext(const AZStd::string& inputDirectory)
                : m_inputDirectory(inputDirectory)
            {
            }

            PreImportEventContext::PreImportEventContext(AZStd::string&& inputDirectory)
                : m_inputDirectory(AZStd::move(inputDirectory))
            {
            }

            const AZStd::string& PreImportEventContext::GetInputDirectory() const
            {
                return m_inputDirectory;
            }

            /////////////
            // ImportEventContext
            /////////////

            ImportEventContext::ImportEventContext(const char* inputDirectory, Containers::Scene& scene)
                : m_inputDirectory(inputDirectory)
                , m_scene(scene)
            {
            }

            ImportEventContext::ImportEventContext(const AZStd::string& inputDirectory, Containers::Scene& scene)
                : m_inputDirectory(inputDirectory)
                , m_scene(scene)
            {
            }

            ImportEventContext::ImportEventContext(AZStd::string&& inputDirectory, Containers::Scene& scene)
                : m_inputDirectory(AZStd::move(inputDirectory))
                , m_scene(scene)
            {
            }

            const AZStd::string& ImportEventContext::GetInputDirectory() const
            {
                return m_inputDirectory;
            }

            Containers::Scene& ImportEventContext::GetScene()
            {
                return m_scene;
            }

            /////////////
            // PostImportEventContext
            /////////////

            PostImportEventContext::PostImportEventContext(const Containers::Scene& scene)
                : m_scene(scene)
            {
            }

            const Containers::Scene& PostImportEventContext::GetScene() const
            {
                return m_scene;
            }
            
        } // Events
    } // SceneAPI
} // AZ