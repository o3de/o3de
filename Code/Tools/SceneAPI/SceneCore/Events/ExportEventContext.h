#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>
#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }

        namespace DataTypes
        {
            class IGroup;
        }

        namespace Events
        {
            class ExportProductList;

            // Signals an export of the contained scene is about to happen.
            class PreExportEventContext
                : public ICallContext
            {
            public:
                AZ_RTTI(PreExportEventContext, "{6B303E35-8BF0-43DD-9AD7-7D7F24F18F37}", ICallContext);
                ~PreExportEventContext() override = default;
                SCENE_CORE_API PreExportEventContext(ExportProductList& productList, const AZStd::string& outputDirectory, const Containers::Scene& scene, const char* platformIdentifier);
                SCENE_CORE_API PreExportEventContext(ExportProductList& productList, AZStd::string&& outputDirectory, const Containers::Scene& scene, const char* platformIdentifier);

                SCENE_CORE_API const AZStd::string& GetOutputDirectory() const;
                SCENE_CORE_API ExportProductList& GetProductList();
                SCENE_CORE_API const ExportProductList& GetProductList() const;
                SCENE_CORE_API const Containers::Scene& GetScene() const;
                SCENE_CORE_API const char* GetPlatformIdentifier() const;

            private:
                AZStd::string m_outputDirectory;
                ExportProductList& m_productList;
                const Containers::Scene& m_scene;
                
                /**
                * The platform identifier is configured in the AssetProcessorPlatformConfig.ini and is data driven
                * it is generally a value like "pc" or "ios" or such.
                * this const char* points at memory owned by the caller but it will always survive for the duration of the call.
                */
                const char* m_platformIdentifier = nullptr;
            };

            // Signals the scene that the contained scene needs to be exported to the specified directory.
            class ExportEventContext
                : public ICallContext
            {
            public:
                AZ_RTTI(ExportEventContext, "{ECE4A3BD-CE48-4B17-9609-6D97F8A887D3}", ICallContext);
                ~ExportEventContext() override = default;

                SCENE_CORE_API ExportEventContext(ExportProductList& productList, const AZStd::string& outputDirectory, const Containers::Scene& scene, const char* platformIdentifier);
                SCENE_CORE_API ExportEventContext(ExportProductList& productList, AZStd::string&& outputDirectory, const Containers::Scene& scene, const char* platformIdentifier);

                SCENE_CORE_API const AZStd::string& GetOutputDirectory() const;
                SCENE_CORE_API ExportProductList& GetProductList();
                SCENE_CORE_API const ExportProductList& GetProductList() const;
                SCENE_CORE_API const Containers::Scene& GetScene() const;
                SCENE_CORE_API const char* GetPlatformIdentifier() const;

            private:
                AZStd::string m_outputDirectory;
                ExportProductList& m_productList;
                const Containers::Scene& m_scene;
                
                /** 
                * The platform identifier is configured in the AssetProcessorPlatformConfig.ini and is data driven
                * it is generally a value like "pc" or "ios" or such.
                * this const char* points at memory owned by the caller but it will always survive for the duration of the call.
                */
                const char* m_platformIdentifier = nullptr;
            };

            // Signals that an export has completed and written (if successful) to the specified directory.
            class PostExportEventContext
                : public ICallContext
            {
            public:
                AZ_RTTI(PostExportEventContext, "{92E0AD59-62CA-45E3-BB73-5659D10FF0DE}", ICallContext);
                ~PostExportEventContext() override = default;

                SCENE_CORE_API PostExportEventContext(ExportProductList& productList, const AZStd::string& outputDirectory, const char* platformIdentifier);
                SCENE_CORE_API PostExportEventContext(ExportProductList& productList, AZStd::string&& outputDirectory, const char* platformIdentifier);

                SCENE_CORE_API const AZStd::string GetOutputDirectory() const;
                SCENE_CORE_API ExportProductList& GetProductList();
                SCENE_CORE_API const ExportProductList& GetProductList() const;
                SCENE_CORE_API const char* GetPlatformIdentifier() const;

            private:
                AZStd::string m_outputDirectory;
                
                /**
                * The platform identifier is configured in the AssetProcessorPlatformConfig.ini and is data driven
                * it is generally a value like "pc" or "ios" or such.
                * this const char* points at memory owned by the caller but it will always survive for the duration of the call.
                */
                const char* m_platformIdentifier = nullptr;
                ExportProductList& m_productList;
            };
        } // namespace Events
    } // namespace SceneAPI
} // namespace AZ
