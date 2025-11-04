/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace Compression
{
    // System Component TypeIds
    inline constexpr const char* CompressionSystemComponentTypeId = "{7E220BA3-B665-4225-9023-F0520E4B436E}";
    inline constexpr const char* CompressionEditorSystemComponentTypeId = "{82C6760D-5C0C-435F-BC1B-F7B9632DE7C5}";

    // Module derived classes TypeIds
    inline constexpr const char* CompressionModuleInterfaceTypeId = "{89158BD2-EAC1-4CBF-9F05-9237034FF28E}";
    inline constexpr const char* CompressionModuleTypeId = "{6D256D91-6F1F-4132-B78E-6C24BA9D688C}";
    // The Editor Module by default is mutually exclusive with the Client Module
    // so they use the Same TypeId
    inline constexpr const char* CompressionEditorModuleTypeId = CompressionModuleTypeId;

    // Interface TypeIds
    inline constexpr const char* CompressionRequestsTypeId = "{40076ED8-6AC5-498B-AF61-C5CD7136750E}";

    inline constexpr const char* CompressionRegistrarInterfaceTypeId = "{92251FE8-9D19-4A23-9A2B-F91D99D9491B}";
    inline constexpr const char* CompressionRegistrarImplTypeId ="{9F3B8418-4BEB-4249-BAAF-6653A8F511A3}";

    inline constexpr const char* DecompressionRegistrarInterfaceTypeId = "{DB1ACA55-B36F-469B-9704-EC486D9FC810}";
    inline constexpr const char* DecompressionRegistrarImplTypeId = "{2353362A-A059-4681-ADF0-5ABE41E85A6B}";

    // etc... TypeIds
    inline constexpr const char* CompressionOptionsTypeId = "{037B2A25-E195-4C5D-B402-6108CE978280}";

    inline constexpr const char* DecompressionOptionsTypeId = "{EA85CCE4-B630-47B8-892F-3A5B1C9ECD99}";
} // namespace Compression
