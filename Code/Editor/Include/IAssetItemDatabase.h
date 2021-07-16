/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Standard interface for asset database creators used to
//               create an asset plugin for the asset browser
//               The category of the plugin must be  Asset Item DB


#ifndef CRYINCLUDE_EDITOR_INCLUDE_IASSETITEMDATABASE_H
#define CRYINCLUDE_EDITOR_INCLUDE_IASSETITEMDATABASE_H
#pragma once
struct IAssetItem;
struct IAssetViewer;

class QString;
class QStringList;

// Description:
//  This struct keeps the info, filter and sorting settings for an asset field
struct SAssetField
{
    // the condition for the current filter on the field
    enum EAssetFilterCondition
    {
        eCondition_Any = 0,
        // string conditions
        // this also supports '*' and '?' as wildcards inside text
        eCondition_Contains,
        // this filter will search the target for at least one of the words specified
        // ( ex: filter: "water car moon" , field value : "the_great_moon.dds", this will pass the test
        // it also supports '*' and '?' as wildcards inside words text
        eCondition_ContainsOneOfTheWords,
        eCondition_StartsWith,
        eCondition_EndsWith,
        // string & numerical conditions
        eCondition_Equal,
        eCondition_Greater,
        eCondition_Less,
        eCondition_GreaterOrEqual,
        eCondition_LessOrEqual,
        eCondition_Not,
        eCondition_InsideRange
    };

    // the asset field type
    enum EAssetFieldType
    {
        eType_None = 0,
        eType_Bool,
        eType_Int8,
        eType_Int16,
        eType_Int32,
        eType_Int64,
        eType_Float,
        eType_Double,
        eType_String
    };

    // used when a field can have different specific values
    typedef QStringList TFieldEnumValues;

    SAssetField(
        const char* pFieldName = "",
        const char* pDisplayName = "Unnamed field",
        EAssetFieldType aFieldType = eType_None,
        UINT aColumnWidth = 50,
        bool bVisibleInUI = true,
        bool bReadOnly = true)
    {
        m_fieldName = pFieldName;
        m_displayName = pDisplayName;
        m_fieldType = aFieldType;
        m_filterCondition = eCondition_Equal;
        m_bUseEnumValues = false;
        m_bReadOnly = bReadOnly;
        m_listColumnWidth = aColumnWidth;
        m_bFieldVisibleInUI = bVisibleInUI;
        m_bPostFilter = false;

        SetupEnumValues();
    }

    void SetupEnumValues()
    {
        m_bUseEnumValues = true;

        if (m_fieldType == eType_Bool)
        {
            m_enumValues.clear();
            m_enumValues.push_back("Yes");
            m_enumValues.push_back("No");
        }
    }

    // the field's display name, used in UI
    QString m_displayName,
    // the field internal name, used in C++ code
            m_fieldName,
    // the current filter value, if its empty "" then no filter is applied
            m_filterValue,
    // the field's max value, valid when the field's filter condition is eAssertFilterCondition_InsideRange
            m_maxFilterValue,
    // the name of the database holding this field, used in Asset Browser preset editor, if its "" then the field
    // is common to all current databases
            m_parentDatabaseName;
    // is this field visible in the UI ?
    bool m_bFieldVisibleInUI,
    // if true, then you cannot modify this field of an asset item, only use it
         m_bReadOnly,
    // this field filter is applied after the other filters
         m_bPostFilter;
    // the field data type
    EAssetFieldType m_fieldType;
    // the filter's condition
    EAssetFilterCondition m_filterCondition;
    // use the enum list values to choose a value for the field ?
    bool m_bUseEnumValues;
    // this map is used when asset field has m_bUseEnumValues on true,
    // choose a value for the field from this list in the UI
    TFieldEnumValues m_enumValues;
    // recommended list column width
    unsigned int m_listColumnWidth;
};

struct SFieldFiltersPreset
{
    QString presetName2;
    QStringList checkedDatabaseNames;
    bool bUsedInLevel;
    std::vector<SAssetField> fields;
};

// Description:
//  This interface allows the programmer to extend asset display types
//  visible in the asset browser.
struct IAssetItemDatabase
    : public IUnknown
{
    DEFINE_UUID(0xFB09B039, 0x1D9D, 0x4057, 0xA5, 0xF0, 0xAA, 0x3C, 0x7B, 0x97, 0xAE, 0xA8)

    typedef std::vector<SAssetField> TAssetFields;
    typedef std::map < QString/*field name*/, SAssetField > TAssetFieldFiltersMap;
    typedef std::map < QString/*asset filename*/, IAssetItem* > TFilenameAssetMap;
    typedef AZStd::function<bool(const IAssetItem*)> MetaDataChangeListener;

    // Description:
    //      Refresh the database by scanning the folders/paks for files, does not load the files, only filename and filesize are fetched
    virtual void Refresh() = 0;
    // Description:
    //      Fills the asset meta data from the loaded xml meta data DB.
    // Arguments:
    //      db - the database XML node from where to cache the info
    virtual void PrecacheFieldsInfoFromFileDB(const XmlNodeRef& db) = 0;
    // Description:
    //      Return all assets loaded/scanned by this database
    // Return Value:
    //      The assets map reference (filename-asset)
    virtual TFilenameAssetMap& GetAssets() = 0;
    // Description:
    //      Get an asset item by its filename
    // Return Value:
    //      A single asset from the database given the filename
    virtual IAssetItem* GetAsset(const char* pAssetFilename) = 0;
    // Description:
    //      Return the asset fields this database's items support
    // Return Value:
    //      The asset fields vector reference
    virtual TAssetFields& GetAssetFields() = 0;
    // Description:
    //      Return an asset field object pointer by the field internal name
    // Arguments:
    //      pFieldName - the internal field's name (ex: "filename", "relativepath")
    // Return Value:
    //      The asset field object pointer
    virtual SAssetField* GetAssetFieldByName(const char* pFieldName) = 0;
    // Description:
    //      Get the database name
    // Return Value:
    //      Returns the database name, ex: "Textures"
    virtual const char* GetDatabaseName() const = 0;
    // Description:
    //      Get the database supported file name extension(s)
    // Return Value:
    //      Returns the supported extensions, separated by comma, ex: "tga,bmp,dds"
    virtual const char* GetSupportedExtensions() const = 0;
    // Description:
    //      Free the database internal data structures
    virtual void FreeData() = 0;
    // Description:
    //      Apply filters to this database which will set/unset the IAssetItem::eAssetFlag_Visible of each asset, based
    //      on the given field filters
    // Arguments:
    //      rFieldFilters - a reference to the field filters map (fieldname-field)
    // See Also:
    //      ClearFilters()
    virtual void ApplyFilters(const TAssetFieldFiltersMap& rFieldFilters) = 0;
    // Description:
    //      Clear the current filters, by setting the IAssetItem::eAssetFlag_Visible of each asset to true
    // See Also:
    //      ApplyFilters()
    virtual void ClearFilters() = 0;
    virtual QWidget* CreateDbFilterDialog(QWidget* pParent, IAssetViewer* pViewerCtrl) = 0;
    virtual void UpdateDbFilterDialogUI(QWidget* pDlg) = 0;
    virtual void OnAssetBrowserOpen() = 0;
    virtual void OnAssetBrowserClose() = 0;
    // Description:
    //       Gets the filename for saving new cached asset info.
    // Return Value:
    //     A file name to save new transactions to the persistent asset info DB
    // See Also:
    //     CAssetInfoFileDB, IAssetItem::ToXML(), IAssetItem::FromXML()
    virtual const char* GetTransactionFilename() const = 0;
    // Description:
    //     Adds a callback to be called when the meta data of this asset changed.
    // Arguments:
    //     callBack - A functor to be added
    // Return Value:
    //     True if successful, false otherwise.
    // See Also:
    //     RemoveMetaDataChangeListener()
    virtual bool AddMetaDataChangeListener(MetaDataChangeListener callBack) = 0;
    // Description:
    //     Removes a callback from the list of meta data change listeners.
    // Arguments:
    //     callBack - A functor to be removed
    // Return Value:
    //     True if successful, false otherwise.
    // See Also:
    //     AddMetaDataCHangeListener()
    virtual bool RemoveMetaDataChangeListener(MetaDataChangeListener callBack) = 0;
    // Description:
    //          The method that should be called when the meta data of an asset item changes to notify all listeners
    // Arguments:
    //     pAssetItem - An asset item whose meta data have changed
    // See Also:
    //      AddMetaDataCHangeListener(), RemoveMetaDataChangeListener()
    virtual void OnMetaDataChange(const IAssetItem* pAssetItem) = 0;

    //! from IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface([[maybe_unused]] REFIID riid, [[maybe_unused]] void** ppvObject)
    {
        return E_NOINTERFACE;
    };
    virtual ULONG STDMETHODCALLTYPE AddRef()
    {
        return 0;
    };
    virtual ULONG STDMETHODCALLTYPE Release()
    {
        return 0;
    };
};
#endif // CRYINCLUDE_EDITOR_INCLUDE_IASSETITEMDATABASE_H
