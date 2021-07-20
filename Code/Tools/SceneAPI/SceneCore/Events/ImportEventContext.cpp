/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
