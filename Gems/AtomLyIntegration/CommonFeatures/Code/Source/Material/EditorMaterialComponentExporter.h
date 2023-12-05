/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QProgressDialog>
AZ_POP_DISABLE_WARNING

namespace AZ
{
    namespace Render
    {
        namespace EditorMaterialComponentExporter
        {
            //! Generates a destination file path for exporting material source data
            AZStd::string GetExportPathByAssetId(const AZ::Data::AssetId& assetId, const AZStd::string& materialSlotName);

            class ExportItem
            {
            public:
                //! @param originalAssetId   AssetId of the original built-in material, which will be exported.
                //! @param materialSlotName  The name of the material slot will be used as part of the exported file name.
                ExportItem(AZ::Data::AssetId originalAssetId, const AZStd::string& materialSlotName, const AZStd::string& exportPath = {})
                    : m_originalAssetId(originalAssetId)
                    , m_materialSlotName(materialSlotName)
                    , m_exportPath(!exportPath.empty() ? exportPath : GetExportPathByAssetId(originalAssetId, materialSlotName))
                {
                }

                void SetEnabled(bool enabled) { m_enabled = enabled; }
                void SetExists(bool exists) { m_exists = exists; }
                void SetOverwrite(bool overwrite) { m_overwrite = overwrite; }
                void SetExportPath(const AZStd::string& exportPath) { m_exportPath = exportPath; }
                
                bool GetEnabled() const   { return m_enabled; }
                bool GetExists() const    { return m_exists; }
                bool GetOverwrite() const { return m_overwrite; }
                const AZStd::string& GetExportPath() const { return m_exportPath; }

                AZ::Data::AssetId GetOriginalAssetId() const { return m_originalAssetId; }
                const AZStd::string& GetMaterialSlotName() const { return m_materialSlotName; }

            private:
                bool m_enabled = true;
                bool m_exists = false;
                bool m_overwrite = false;
                AZStd::string m_exportPath;
                AZ::Data::AssetId m_originalAssetId; //!< AssetId of the original built-in material, which will be exported.
                AZStd::string m_materialSlotName;
            };

            using ExportItemsContainer = AZStd::vector<ExportItem>;

            //! Generates and opens a dialog for configuring material data export paths and actions.
            //! Note this will not modify the m_originalAssetId field in each ExportItem.
            bool OpenExportDialog(ExportItemsContainer& exportItems);

            //! Attemts to construct and save material source data from a product asset
            bool ExportMaterialSourceData(const ExportItem& exportItem);

            //! Create a progress dialog for displaying the status of generated material assets.
            class ProgressDialog
            {
            public:
                ProgressDialog(const AZStd::string& title, const AZStd::string& label, const int itemCount);
                ~ProgressDialog() = default;

                //! Blocking call that polls for asset info until valid or the user cancels the operation.
                AZ::Data::AssetInfo ProcessItem(const ExportItem& exportItem);

                //! Increment the progress bar in the dialog.
                void CompleteItem();

            private:
                AZStd::unique_ptr<QProgressDialog> m_progressDialog;
            };
        } // namespace EditorMaterialComponentExporter
    } // namespace Render
} // namespace AZ
