/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class Entity;
}

namespace LYShineToShine
{
    //! Upgrades old LyShine v1/v2 .uicanvas files to Shine v4 format.
    //!
    //! The v2 format stores entities inside a RootSliceEntity → SliceComponent hierarchy:
    //!   UiCanvasFileObject (version 2)
    //!     CanvasEntity (with UiCanvasComponent)
    //!     RootSliceEntity (AZ::Entity with SliceComponent)
    //!       SliceComponent
    //!         Entities[] — loose entities
    //!         Prefabs[] — SliceReference[] → SliceInstance[] (with EntityIdMap + DataPatch)
    //!
    //! The v4 format stores local entities and prefab references:
    //!   UiCanvasFileObject (version 4)
    //!     CanvasEntity (with UiCanvasComponent)
    //!     ChildEntities[] — entities local to this canvas (not from any prefab)
    //!     PrefabInstances[] — references to .uiprefab files with JSON Patch overrides
    //!
    //! Conversion strategy:
    //!   Phase 1 (Simple): Files with no SliceReferences — extract entities from
    //!     SliceComponent::Entities and move to ChildEntities. Pure XML manipulation.
    //!
    //!   Phase 2 (Complex): Files with SliceReferences — extract direct entities to
    //!     ChildEntities, convert each SliceReference to a UiPrefabInstance that
    //!     references the corresponding .uiprefab file (created by convert_slices).
    //!
    class CanvasUpgrader
    {
    public:
        struct UpgradeReport
        {
            size_t m_filesScanned = 0;
            size_t m_alreadyV3 = 0;
            size_t m_upgradedSimple = 0;
            size_t m_upgradedWithSliceRefs = 0;
            size_t m_failed = 0;
            AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>> m_failureDetails; // path, reason
        };

        struct SliceConvertReport
        {
            size_t m_filesScanned = 0;
            size_t m_converted = 0;
            size_t m_skippedNonUi = 0;
            size_t m_failed = 0;
            AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>> m_failureDetails; // path, reason
        };

        //! Recursively upgrades all .uicanvas files in the given directory.
        UpgradeReport UpgradeDirectory(const AZ::IO::Path& directoryPath);

        //! Upgrades a single .uicanvas file. Returns true on success.
        bool UpgradeFile(const AZ::IO::Path& filePath, AZStd::string& outError);

        //! Detects the canvas format version from the raw XML content.
        //! Returns 1, 2, or 3 (or 0 if unrecognized).
        int DetectVersion(const AZStd::string& xmlContent) const;

        //! Recursively converts all .slice files in the given directory to .uiprefab format.
        SliceConvertReport ConvertSlicesInDirectory(const AZ::IO::Path& directoryPath);

        //! Converts a single .slice file to .uiprefab format.
        //! The .uiprefab file is written alongside the .slice with the same base name.
        bool ConvertSliceToUiPrefab(const AZ::IO::Path& slicePath, AZStd::string& outError);

    private:

        //! Returns true if the XML content contains SliceReference elements in the Prefabs list.
        bool HasSliceReferences(const AZStd::string& xmlContent) const;

        //! Phase 1: Upgrade a simple v2 canvas (no SliceReferences) via XML manipulation.
        //! Extracts entities from SliceComponent::Entities, removes the RootSliceEntity wrapper,
        //! and writes ChildEntities directly in v4 format.
        bool UpgradeSimple(const AZStd::string& xmlContent, const AZ::IO::Path& filePath, AZStd::string& outError);

        //! Phase 2: Upgrade a complex v2 canvas (with SliceReferences).
        //! Extracts direct entities to ChildEntities and converts SliceReferences
        //! to UiPrefabInstance entries referencing .uiprefab files.
        bool UpgradeWithSliceRefs(const AZStd::string& xmlContent, const AZ::IO::Path& filePath, AZStd::string& outError);

        //! Extract a named XML block: finds <Class ... field="fieldName" ...>...</Class> at the
        //! expected nesting depth. Returns the full block including open/close tags.
        //! Returns empty string if not found.
        AZStd::string ExtractXmlBlock(const AZStd::string& xml, const char* fieldName, size_t startSearchPos = 0) const;

        //! Extract all entity blocks from inside a SliceComponent's Entities vector.
        //! Returns the individual <Class name="AZ::Entity" ...>...</Class> blocks.
        AZStd::vector<AZStd::string> ExtractEntitiesFromSliceComponent(const AZStd::string& sliceComponentXml) const;

        //! Build v4 format XML from a CanvasEntity block, entity blocks, and optional prefab instances.
        AZStd::string BuildV4Canvas(
            const AZStd::string& objectStreamVersion,
            const AZStd::string& canvasEntityXml,
            const AZStd::vector<AZStd::string>& entityXmlBlocks,
            const AZStd::vector<AZStd::string>& prefabInstanceXmlBlocks = {}) const;

        //! Serialize an AZ::Entity to XML and inject a field attribute.
        //! Returns the entity's XML with the given field name, stripped of ObjectStream wrapper.
        //! Returns empty string on failure.
        AZStd::string SerializeEntityToXml(
            AZ::Entity* entity,
            AZ::SerializeContext* serializeContext,
            const char* fieldName) const;
    };
} // namespace LYShineToShine
