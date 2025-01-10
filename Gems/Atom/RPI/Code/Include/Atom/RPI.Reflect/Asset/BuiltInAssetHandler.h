/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Configuration.h>
#include <AzCore/Asset/AssetManager.h>

namespace AZ
{
    namespace RPI
    {
        //! This AssetHandler provides a simple way to create and register hard-coded assets. Normally,
        //! assets are a read from files on disk in the asset cache, but these assets are built-in to
        //! the application. As such, they will immediately be in the "Ready" state after created via CreateAsset().
        //! 
        //! An example use for this class is to allow custom gems to provide implementations of standard interfaces.
        //! 
        //! Example:
        //! 
        //!     // Somewhere in Atom code...
        //! 
        //!     class SomeAbstractAtomClass : Data::AssetData
        //!     {
        //!     ...
        //!     };
        //!   
        //!     // Somewhere in game code...
        //! 
        //!     class Foo : public SomeAbstractAtomClass
        //!     {
        //!         ...
        //!         Foo()
        //!         {
        //!            ...
        //!            m_status = static_cast<int>(AssetStatus::Ready); // Since this is a hard-coded asset, not a loaded asset, it should be ready immediately
        //!         }
        //!         ...
        //!     };
        //!   
        //!     class Bar : public SomeAbstractAtomClass
        //!     {
        //!     ...
        //!     };
        //! 
        //!     class FooBarAssetCollection final
        //!     {
        //!     public:
        //!         ~FooBarAssetCollection()
        //!         {
        //!             Shutdown();
        //!         }
        //! 
        //!         void Init()
        //!         {
        //!             m_fooAssetHandler.Init(azrtti_typeid<Foo>(), { []() { return new Foo(); } });
        //!             m_barAssetHandler.Init(azrtti_typeid<Bar>(), { []() { return new Bar(); } });
        //!             m_fooAssetHandler.Register();
        //!             m_barAssetHandler.Register();
        //! 
        //!             m_foo1 = Data::AssetManager::Instance().CreateAsset<Foo>(Data::AssetId("{AE302643-B77C-43C9-A932-F8E7FA39FF5C}"));
        //!             // Configure m_foo1 here
        //! 
        //!             m_foo2 = Data::AssetManager::Instance().CreateAsset<Foo>(Data::AssetId("{D9990E51-E20F-4F93-BB87-A63672C7F7E2}"));
        //!             // Configure m_foo2 here
        //! 
        //!             m_bar1 = Data::AssetManager::Instance().CreateAsset<Bar>(Data::AssetId("{43C59425-8236-49DE-9E58-317158BF12C4}"));
        //!             // Configure m_bar1 here
        //! 
        //!             m_bar2 = Data::AssetManager::Instance().CreateAsset<Bar>(Data::AssetId("{DB61552B-3A70-45C8-9C9D-39B75E43E51C}"));
        //!             // Configure m_bar2 here
        //!         }
        //! 
        //!         void Shutdown()
        //!         {
        //!             m_foo1.Release();
        //!             m_foo2.Release();
        //!             m_bar1.Release();
        //!             m_bar2.Release();
        //!             m_fooAssetHandler.Unregister();
        //!             m_barAssetHandler.Unregister();
        //!         }
        //! 
        //!     private:
        //!         BuiltInAssetHandler m_fooAssetHandler;
        //!         BuiltInAssetHandler m_barAssetHandler;
        //!         Data::Asset<Foo> m_foo1;
        //!         Data::Asset<Foo> m_foo2;
        //!         Data::Asset<Bar> m_bar1;
        //!         Data::Asset<Bar> m_bar2;
        //!     };
        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_REFLECT_API BuiltInAssetHandler
            : public Data::AssetHandler
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING

        public:
            AZ_CLASS_ALLOCATOR(BuiltInAssetHandler, AZ::SystemAllocator);
            AZ_RTTI(BuiltInAssetHandler, "{C6615D6C-72AF-4444-8C27-8B88D89074E8}", Data::AssetHandler);

            static void StandardDestroyFunction(AZ::Data::AssetPtr ptr);

            using CreateFunction = AZStd::function<Data::AssetPtr()>;
            using DeleteFunction = AZStd::function<void(AZ::Data::AssetPtr ptr)>;

            struct AssetHandlerFunctions
            {
                AssetHandlerFunctions(CreateFunction createFunction, DeleteFunction deleteFunction = &StandardDestroyFunction);

                CreateFunction m_create = nullptr;
                DeleteFunction m_destroy = &StandardDestroyFunction;
            };

            BuiltInAssetHandler(const Data::AssetType& assetType, const AssetHandlerFunctions& handlerFunctions);
            BuiltInAssetHandler(const Data::AssetType& assetType, CreateFunction createFunction);
            ~BuiltInAssetHandler();

            void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override;

            void Register();
            void Unregister();

            Data::AssetPtr CreateAsset(const Data::AssetId& id, const Data::AssetType& type) override;
            void DestroyAsset(AZ::Data::AssetPtr ptr) override;

            Data::AssetHandler::LoadResult LoadAssetData(
                const Data::Asset<Data::AssetData>& asset,
                AZStd::shared_ptr<Data::AssetDataStream> stream,
                const Data::AssetFilterCB& assetLoadFilterCB) override;

        private:

            Data::AssetType m_assetType;
            AssetHandlerFunctions m_handlerFunctions;
            bool m_registered = false;
        };

    } // namespace RPI
} // namespace AZ
