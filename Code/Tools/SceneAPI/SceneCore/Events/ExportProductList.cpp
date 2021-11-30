/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneCore/Events/ExportProductList.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/limits.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Events
        {
            ExportProduct::ExportProduct(const AZStd::string& filename, Uuid id, Data::AssetType assetType, AZStd::optional<u8> lod, AZStd::optional<u32> subId,
                Data::ProductDependencyInfo::ProductDependencyFlags dependencyFlags)
                :ExportProduct(AZStd::string(filename), id, assetType, lod, subId, dependencyFlags)
            {
            }

            ExportProduct::ExportProduct(AZStd::string&& filename, Uuid id, Data::AssetType assetType, AZStd::optional<u8> lod, AZStd::optional<u32> subId,
                Data::ProductDependencyInfo::ProductDependencyFlags dependencyFlags)
                : m_filename(AZStd::move(filename))
                , m_id(id)
                , m_assetType(assetType)
                , m_lod(lod)
                , m_subId(subId)
                , m_dependencyFlags(dependencyFlags)
            {
            }

            ExportProduct::ExportProduct(ExportProduct&& rhs)
            {
                *this = AZStd::move(rhs);
            }

            ExportProduct& ExportProduct::operator=(ExportProduct&& rhs)
            {
                m_legacyFileNames = AZStd::move(rhs.m_legacyFileNames);
                m_filename = AZStd::move(rhs.m_filename);
                m_id = rhs.m_id;
                m_assetType = rhs.m_assetType;
                m_lod = rhs.m_lod;
                m_subId = rhs.m_subId;
                m_dependencyFlags = rhs.m_dependencyFlags;
                m_legacyPathDependencies = rhs.m_legacyPathDependencies;
                m_productDependencies = rhs.m_productDependencies;
                return *this;
            }

            void ExportProductList::Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<ExportProduct>()->Version(1);
                    serializeContext->Class<ExportProductList>()->Version(1);
                }

                if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
                {
                    behaviorContext->Class<ExportProduct>("ExportProduct")
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                        ->Attribute(AZ::Script::Attributes::Module, "scene")
                        ->Property("filename", BehaviorValueProperty(&ExportProduct::m_filename))
                        ->Property("sourceId", BehaviorValueProperty(&ExportProduct::m_id))
                        ->Property("assetType", BehaviorValueProperty(&ExportProduct::m_assetType))
                        ->Property("productDependencies", BehaviorValueProperty(&ExportProduct::m_productDependencies))
                        ->Property("subId",
                            [](ExportProduct* self) { return self->m_subId.has_value() ? self->m_subId.value() : 0; },
                            [](ExportProduct* self, u32 subId) { self->m_subId = AZStd::optional<u32>(subId); });

                    behaviorContext->Class<ExportProductList>("ExportProductList")
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                        ->Attribute(AZ::Script::Attributes::Module, "scene")
                        ->Method("AddProduct", [](ExportProductList& self, ExportProduct& product)
                        {
                            self.AddProduct(
                                product.m_filename,
                                product.m_id,
                                product.m_assetType,
                                product.m_lod,
                                product.m_subId,
                                product.m_dependencyFlags);
                        })
                        ->Method("GetProducts", &ExportProductList::GetProducts)
                        ->Method("AddDependencyToProduct", &ExportProductList::AddDependencyToProduct);
                }
            }

            ExportProduct& ExportProductList::AddProduct(const AZStd::string& filename, Uuid id, Data::AssetType assetType, AZStd::optional<u8> lod, AZStd::optional<u32> subId,
                Data::ProductDependencyInfo::ProductDependencyFlags dependencyFlags)
            {
                return AddProduct(AZStd::string(filename), id, assetType, lod, subId, dependencyFlags);
            }

            ExportProduct& ExportProductList::AddProduct(AZStd::string&& filename, Uuid id, Data::AssetType assetType, AZStd::optional<u8> lod, AZStd::optional<u32> subId,
                Data::ProductDependencyInfo::ProductDependencyFlags dependencyFlags)
            {
                AZ_Assert(!filename.empty(), "A filename is required to register a product.");
                AZ_Assert(!id.IsNull(), "Provided guid is not valid");
                AZ_Assert(!lod.has_value() || lod < 16, "Lod value has to be between 0 and 15 or disabled.");

                size_t index = m_products.size();
                m_products.emplace_back(AZStd::move(filename), id, assetType, lod, subId, dependencyFlags);
                return m_products[index];
            }

            const AZStd::vector<ExportProduct>& ExportProductList::GetProducts() const
            {
                return m_products;
            }

            void ExportProductList::AddDependencyToProduct(const AZStd::string& productName, ExportProduct& dependency)
            {
                for (ExportProduct& product : m_products)
                {
                    if (product.m_filename == productName)
                    {
                        product.m_productDependencies.push_back(dependency);
                        break;
                    }
                }
            }

        } // namespace Events
    } // namespace SceneAPI
} // namespace AZ
