/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef ASSETPROCESSOR_ASSETDATA_H
#define ASSETPROCESSOR_ASSETDATA_H

#include <QMetaType>
#include <QString>
#include <QSet>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>

namespace AssetProcessor
{
    using namespace AzToolsFramework::AssetDatabase;

    //Check the extension of all the products
    //return true if any one of the product extension matches the input extension, else return false
    bool CheckProductsExtension( const ProductDatabseEntryContainer& products, const char* ext );

    //! this is the interface which we use to speak to the legacy database tables.
    // its known as the legacy database interface because the forthcoming tables will completely replace these
    // but this layer exits for compatibility with the previous version and allows us to upgrade in place.
    class AssetDatabaseInterface
    {
    public:

        AssetDatabaseInterface()
        {
            qRegisterMetaType<SourceDatabaseEntry>( "SourceEntry" );
            qRegisterMetaType<ProductDatabaseEntry>( "ProductEntry" );
            qRegisterMetaType<SourceDatabaseEntryContainer>( "SourceEntryContainer" );
            qRegisterMetaType<ProductDatabseEntryContainer>( "ProductEntryContainer" );
        }

        virtual ~AssetDatabaseInterface() 
        {
        }

        //! Returns true if the database or file exists already
        virtual bool DataExists() = 0;

        //! Actually connects to the database, loads it, or creates empty database depending on above.
        virtual void LoadData() = 0;

        //! Use with care.  Resets all data!  This causes an immediate commit and save!
        virtual void ClearData() = 0;

        //! Retrieve the scan folders
        virtual void GetScanFolders(QStringList& scanFolderList) = 0;

        //! Retrieves a specific scan folder by id, return false if not found
        virtual bool GetScanFolder(AZ::s64 scanFolderID, QString& scanFolder) = 0;

        //! Adds a scan folder
        virtual AZ::s64 AddScanFolder(QString scanFolder) = 0;

        // ! remove a scanfolder
        virtual void RemoveScanFolder(AZ::s64 scanFolderID) = 0;
        virtual void RemoveScanFolder(QString scanFolder) = 0;

        //! query the scanFolder ID for a given folder, return false if not found
        virtual bool GetScanFolderID(QString scanfolder, AZ::s64& scanFolderID) = 0;

        //! query the sourceID of a source
        virtual bool GetSourceID(QString sourceName, QString jobDescription, AZ::s64& sourceID) = 0;

        //! Retrieve the fingerprint for a given source name on a given platform for a particular jobDescription
        //! This could return zero if its never seen this file before.
        virtual bool GetFingerprintForSource(QString sourceName, QString jobDescription, AZ::u32& fingerprint) = 0;

        //! Set the fingerprint for the given source name, platform and jobDescription to the value provided
        //! If updating a existing fingerprint you do not have to supply guid or scanfolderid
        virtual void SetSource(QString sourceName, QString jobDescription, AZ::u32 fingerprint, AZ::Uuid guid = AZ::Uuid::CreateNull(), AZ::s64 scanFolderID = 0) = 0;

        //! Removing a fingerprint will destroy its entry in the database
        //! and any entries that refer to it (products, etc).  if you want to merely set it dirty
        //! Then instead call SetSource to zero
        virtual void RemoveSource(QString sourceName, QString jobDescription) = 0;
        virtual void RemoveSource(AZ::s64 sourceID) = 0;

        //! Given a source name, jobDescription, and platform return the list of products by the last compile of that file
        //! returns false if it doesn't know about source or if the source did not emitted any products.
        virtual bool GetProductsForSource(QString sourceName, QString jobDescription, ProductDatabseEntryContainer& products, QString platform = QString()) = 0;

        //! Given a source name and platform return the list of all jobDescriptions associated with them from the last compile of that file
        //! returns false if it doesn't know about any job description for that source and platform.
        virtual bool GetJobDescriptionsForSource(QString sourceName, QStringList& jobDescription) = 0;

        //! Given an product file name, compute source file name
        //! False is returned if its never heard of that product.
        virtual bool GetSourceFromProductName(QString productName, SourceDatabaseEntry& source) = 0;

        //! For a given source, set the list of products for that source.
        //! Removes any data that's present and overwrites it with the new list
        //! Note that an empty list is acceptable data, it means the source emitted no products
        virtual void SetProductsForSource(QString sourceName, QString jobDescription, const ProductDatabseEntryContainer& productList = ProductDatabseEntryContainer(), QString platform = QString()) = 0;

        //! Clear the products for a given source.  This removes the entry entirely, not just sets it to empty.
        virtual void RemoveProducts(QString sourceName, QString jobDescription, QString platform = QString()) = 0;
        virtual void RemoveProduct(AZ::s64 productID) = 0;

        //! GetMatchingProductFiles - checks the database for all products that begin with the given match check
        //! Note that the input string is expected to not include the cache folder
        //! so it probably starts with platform name.
        virtual void GetMatchingProducts(QString matchCheck, ProductDatabseEntryContainer& products, QString platform = QString()) = 0;

        //! GetMatchingSourceFiles - checks the database for all source files that begin with the given match  check
        //! note that the input string is expected to be the relative path name
        //! and the output is the relative name (so to convert it to a full path, you will need to call the appropriate function)
        virtual void GetMatchingSources(QString matchCheck, SourceDatabaseEntryContainer& sources) = 0;

        //! Get a giant list of ALL known source files in the database.
        virtual void GetSources(SourceDatabaseEntryContainer& sources) = 0;

        //! Get a giant list of ALL known products in the database.
        virtual void GetProducts(ProductDatabseEntryContainer& products, QString platform = QString()) = 0;

        //! finds all elements in the database that ends with the given input.  (Used to look things up by extensions, in general)
        virtual void GetSourcesByExtension(QString extension, SourceDatabaseEntryContainer& sources) = 0;

        //! SetJobLogForSource updates the Job Log table to record the status of a particular job.
        //! It also sets all prior jobs that match that job exactly to not be the "latest one" but keeps them in the database.
        virtual void SetJobLogForSource(AZ::s64 jobId, const AZStd::string& sourceName, const AZStd::string& platform, const AZ::Uuid& builderUuid, const AZStd::string& jobKey, AzToolsFramework::AssetProcessor::JobStatus status) = 0;
    };
} // namespace AssetProcessor

#endif // ASSETPROCESSOR_ASSETDATA_H
