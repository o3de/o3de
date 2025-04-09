/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/DocumentPropertyEditor/Reflection/Visitor.h>

namespace AZ::Reflection
{
    void IRead::Visit(bool, const IAttributes&)
    {
    }

    void IRead::Visit(char, const IAttributes&)
    {
    }

    void IRead::Visit(AZ::s8, const IAttributes&)
    {
    }

    void IRead::Visit(AZ::s16, const IAttributes&)
    {
    }

    void IRead::Visit(AZ::s32, const IAttributes&)
    {
    }

    void IRead::Visit(AZ::s64, const IAttributes&)
    {
    }

    void IRead::Visit(AZ::u8, const IAttributes&)
    {
    }

    void IRead::Visit(AZ::u16, const IAttributes&)
    {
    }

    void IRead::Visit(AZ::u32, const IAttributes&)
    {
    }

    void IRead::Visit(AZ::u64, const IAttributes&)
    {
    }

    void IRead::Visit(float, const IAttributes&)
    {
    }

    void IRead::Visit(double, const IAttributes&)
    {
    }

    void IRead::VisitObjectBegin(const IObjectAccess&, const IAttributes&)
    {
    }

    void IRead::VisitObjectEnd(const IObjectAccess&, const IAttributes&)
    {
    }

    void IRead::Visit(const AZStd::string_view, const IStringAccess&, const IAttributes&)
    {
    }

    void IRead::Visit(const IArrayAccess&, const IAttributes&)
    {
    }

    void IRead::Visit(const IMapAccess&, const IAttributes&)
    {
    }

    void IRead::Visit(const IDictionaryAccess&, const IAttributes&)
    {
    }

    void IRead::Visit(const IPointerAccess&, const IAttributes&)
    {
    }

    void IRead::Visit(const IBufferAccess&, const IAttributes&)
    {
    }

    void IRead::Visit(const AZ::Data::Asset<AZ::Data::AssetData>&, const IAssetAccess&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(bool&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(char&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(AZ::s8&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(AZ::s16&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(AZ::s32&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(AZ::s64&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(AZ::u8&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(AZ::u16&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(AZ::u32&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(AZ::u64&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(float&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(double&, const IAttributes&)
    {
    }

    void IReadWrite::VisitObjectBegin(IObjectAccess&, const IAttributes&)
    {
    }

    void IReadWrite::VisitObjectEnd(IObjectAccess&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(const AZStd::string_view, IStringAccess&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(IArrayAccess&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(IMapAccess&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(IDictionaryAccess&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(AZ::s64, const IEnumAccess&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(IPointerAccess&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(IBufferAccess&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(const AZ::Data::Asset<AZ::Data::AssetData>&, IAssetAccess&, const IAttributes&)
    {
    }
} // namespace AZ::Reflection
