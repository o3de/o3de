/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzFramework/FileTag/FileTag.h>
#include <AzFramework/FileTag/FileTagBus.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

namespace AzFramework
{
    namespace FileTag
    {
        class FileTagComponent
            : public AZ::Component
        {
        public:

            AZ_COMPONENT(FileTagComponent, "{43D5CE4F-C687-4CAA-9CED-54FBBC928876}");

            void Activate() override;
            void Deactivate() override;

            static void Reflect(AZ::ReflectContext* context);

        private:
            AZStd::unique_ptr<FileTagManager> m_fileTagManager;
        };

        //! This component can be used to query the file tagging system 
        class FileTagQueryComponent
            : public AZ::Component
        {
        public:

            AZ_COMPONENT(FileTagQueryComponent, "{72DE1060-28C9-442F-9927-540E830663F6}");
            explicit FileTagQueryComponent() = default;
            void Activate() override;
            void Deactivate() override;

            static void Reflect(AZ::ReflectContext* context);

        private:
            AZStd::unique_ptr<FileTagQueryManager> m_excludeFileQueryManager;
            AZStd::unique_ptr<FileTagQueryManager> m_includeFileQueryManager;
        };

        //! This component can be used to query whether we need to exclude files based on file tags.
        //! This component loads the default excluded filetags file automatically. 
        class ExcludeFileComponent
            : public AZ::Component
            , public AzFramework::AssetCatalogEventBus::Handler
        {
        public:
            AZ_COMPONENT(ExcludeFileComponent, "{40CF5F1D-BE1F-46AA-9D35-11FC173DCDBC}");
            explicit ExcludeFileComponent() = default;
            void Activate() override;
            void Deactivate() override;

            void OnCatalogLoaded(const char* catalogFile) override;

            static void Reflect(AZ::ReflectContext* context);

        protected:
            AZStd::unique_ptr<FileTagQueryManager> m_excludeFileQueryManager;
        };
    }
}
