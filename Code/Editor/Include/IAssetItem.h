/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Standard interface for asset display in the asset browser,
//               this header should be used to create plugins.
//               The method Release of this interface should NOT be called.
//               Instead, the FreeData from the database (from IAssetItemDatabase) should
//               be used as it will safely release all the items from the database.
//               It is still possible to call the release method, but this is not the
//               recomended method, specially for usage outside of the plugins because there
//               is no guarantee that a the asset will be properly removed from the database
//               manager.

#ifndef CRYINCLUDE_EDITOR_INCLUDE_IASSETITEM_H
#define CRYINCLUDE_EDITOR_INCLUDE_IASSETITEM_H
#pragma once

struct IAssetItemDatabase;

namespace AssetViewer
{
    // Used in GetAssetFieldValue for each asset type to check if field name is the right one
    inline bool IsFieldName(const char* pIncomingFieldName, const char* pFieldName)
    {
        return !strncmp(pIncomingFieldName, pFieldName, strlen(pIncomingFieldName));
    }
}

// Description:
//      This interface allows the programmer to extend asset display types visible in the asset browser.
struct IAssetItem
    : public IUnknown
{
    DEFINE_UUID(0x04F20346, 0x2EC3, 0x43f2, 0xBD, 0xA1, 0x2C, 0x0B, 0x97, 0x76, 0xF3, 0x84);

    // The supported asset flags
    enum EAssetFlags
    {
        // asset is visible in the database for filtering and sorting (not asset view control related)
        eFlag_Visible = BIT(0),
        // the asset is loaded
        eFlag_Loaded = BIT(1),
        // the asset is loaded
        eFlag_Cached = BIT(2),
        // the asset is selected in a selection set
        eFlag_Selected = BIT(3),
        // this asset is invalid, no thumb is shown/available
        eFlag_Invalid = BIT(4),
        // this asset has some errors/warnings, in the asset browser it will show some blinking/red elements
        // and the user can check out the errors. Error text will be fetched using GetAssetFieldValue( "errors", &someStringVar )
        eFlag_HasErrors = BIT(5),
        // this flag is set when the asset is rendering its contents using GDI, and not the engine's rendering capabilities
        // (this flags is used as hint for the preview tool, which will use a double-buffer canvas if this flag is set,
        // and send a memory HDC to the OnBeginPreview method, for drawing of the asset)
        eFlag_UseGdiRendering = BIT(6),
        // set if this asset is draggable into the render viewports, and can be created there
        eFlag_CanBeDraggedInViewports = BIT(7),
        // set if this asset can be moved after creation, otherwise the asset instance will just be created where user clicked
        eFlag_CanBeMovedAfterDroppedIntoViewport = BIT(8),
        // the asset thumbnail image is loaded
        eFlag_ThumbnailLoaded = BIT(9),
        // the asset thumbnail image is loaded
        eFlag_UsedInLevel = BIT(10)
    };

    // Asset field name and field values map
    typedef std::map < QString/*fieldName*/, QString/*value*/ > TAssetFieldValuesMap;
    // Dependency category names and corresponding files map, example: "Textures"=>{ "foam.dds","water.dds","normal.dds" }
    typedef std::map < QString/*dependencyCategory*/, std::set<QString>/*dependency filenames*/ > TAssetDependenciesMap;

    virtual ~IAssetItem() {
    }

    // Description:
    //      Get the hash number/key used for database thumbnail and info records management
    virtual uint32 GetHash() const = 0;
    // Description:
    //      Set the hash number/key used for database thumbnail and info records management
    virtual void SetHash(uint32 hash) = 0;
    // Description:
    //      Get the owner database for this asset
    // Return Value:
    //      The owner database for this asset
    // See Also:
    //      SetOwnerDatabase()
    virtual IAssetItemDatabase* GetOwnerDatabase() const = 0;
    // Description:
    //      Set the owner database for this asset
    // Arguments:
    //      piOwnerDisplayDatabase - the owner database
    // See Also:
    //      GetOwnerDatabase()
    virtual void SetOwnerDatabase(IAssetItemDatabase* pOwnerDisplayDatabase) = 0;
    // Description:
    //      Get the asset's dependency files / objects
    // Return Value:
    //      The vector with filenames which this asset is dependent upon, ex.: ["Textures"].(vector of textures)
    virtual const TAssetDependenciesMap& GetDependencies() const = 0;
    // Description:
    //      Set the file size of this asset in bytes
    // Arguments:
    //      aSize - size of the file in bytes
    // See Also:
    //      GetFileSize()
    virtual void SetFileSize(quint64 aSize) = 0;
    // Description:
    //      Get the file size of this asset in bytes
    // Return Value:
    //      The file size of this asset in bytes
    // See Also:
    //      SetFileSize()
    virtual quint64 GetFileSize() const = 0;
    // Description:
    //      Set asset filename (extension included and no path)
    // Arguments:
    //      pName - the asset filename (extension included and no path)
    // See Also:
    //      GetFilename()
    virtual void SetFilename(const char* pName) = 0;
    // Description:
    //      Get asset filename (extension included and no path)
    // Return Value:
    //      The asset filename (extension included and no path)
    // See Also:
    //      SetFilename()
    virtual QString GetFilename() const = 0;
    // Description:
    //      Set the asset's relative path
    // Arguments:
    //      pName - file's relative path
    // See Also:
    //      GetRelativePath()
    virtual void SetRelativePath(const char* pName) = 0;
    // Description:
    //      Get the asset's relative path
    // Return Value:
    //      The asset's relative path
    // See Also:
    //      SetRelativePath()
    virtual QString GetRelativePath()  const = 0;
    // Description:
    //      Set the file extension ( dot(s) must be included )
    // Arguments:
    //      pExt - the file's extension
    // See Also:
    //      GetFileExtension()
    virtual void SetFileExtension(const char* pExt) = 0;
    // Description:
    //      Get the file extension ( dot(s) included )
    // Return Value:
    //      The file extension ( dot(s) included )
    // See Also:
    //      SetFileExtension()
    virtual QString GetFileExtension()  const = 0;
    // Description:
    //      Get the asset flags, with values from IAssetItem::EAssetFlags
    // Return Value:
    //      The asset flags, with values from IAssetItem::EAssetFlags
    // See Also:
    //      SetFlags(), SetFlag(), IsFlagSet()
    virtual UINT GetFlags() const = 0;
    // Description:
    //      Set the asset flags
    // Arguments:
    //      aFlags - flags, OR-ed values from IAssetItem::EAssetFlags
    // See Also:
    //      GetFlags(), SetFlag(), IsFlagSet()
    virtual void SetFlags(UINT aFlags) = 0;
    // Description:
    //      Set/clear a single flag bit for the asset
    // Arguments:
    //      aFlag - the flag to set/clear, with values from IAssetItem::EAssetFlags
    // See Also:
    //      GetFlags(), SetFlags(), IsFlagSet()
    virtual void SetFlag(EAssetFlags aFlag, bool bSet = true) = 0;
    // Description:
    //      Check if a specified flag is set
    // Arguments:
    //      aFlag - the flag to check, with values from IAssetItem::EAssetFlags
    // Return Value:
    //      True if the flag is set
    // See Also:
    //      GetFlags(), SetFlags(), SetFlag()
    virtual bool IsFlagSet(EAssetFlags aFlag) const = 0;
    // Description:
    //      Set this asset's index; used in sorting, selections, and to know where an asset is in the current list
    // Arguments:
    //      aIndex - the asset's index
    // See Also:
    //      GetIndex()
    virtual void SetIndex(UINT aIndex) = 0;
    // Description:
    //      Get the asset's index in the current list
    // Return Value:
    //      The asset's index in the current list
    // See Also:
    //      SetIndex()
    virtual UINT GetIndex() const = 0;
    // Description:
    //      Get the asset's field raw data value into a user location, you must check the field's type ( from asset item's owner database )
    //      before using this function and send the correct pointer to destination according to the type ( int8, float32, string, etc. )
    // Arguments:
    //      pFieldName - the asset field name to query the value for
    //      pDest - the destination variable address, must be the same type as the field type
    // Return Value:
    //      True if the asset field name is found and the value is returned correctly
    // See Also:
    //      SetAssetFieldValue()
    virtual QVariant GetAssetFieldValue(const char* pFieldName) const = 0;
    // Description:
    //      Set the asset's field raw data value from a user location, you must check the field's type ( from asset item's owner database )
    //      before using this function and send the correct pointer to source according to the type ( int8, float32, string, etc. )
    // Arguments:
    //      pFieldName - the asset field name to set the value for
    //      pSrc - the source variable address, must be the same type as the field type
    // Return Value:
    //      True if the asset field name is found and the value is set correctly
    // See Also:
    //      GetAssetFieldValue()
    virtual bool SetAssetFieldValue(const char* pFieldName, void* pSrc) = 0;
    // Description:
    //      Get the drawing rectangle for the asset's thumb ( absolute viewer canvas location )
    // Arguments:
    //      rstDrawingRectangle - destination location to set with the asset's thumbnail rectangle location
    // See Also:
    //      SetDrawingRectangle()
    virtual void GetDrawingRectangle(QRect& rstDrawingRectangle) const = 0;
    // Description:
    //      Set the drawing rectangle for the asset's thumb ( absolute viewer canvas location )
    // Arguments:
    //      crstDrawingRectangle - source to set the asset's thumbnail rectangle
    // See Also:
    //      GetDrawingRectangle()
    virtual void SetDrawingRectangle(const QRect& crstDrawingRectangle) = 0;
    // Description:
    //      Checks if the given 2D point is inside the asset's thumb rectangle
    // Arguments:
    //      nX - mouse pointer position on X axis, relative to the asset viewer control
    //      nY - mouse pointer position on Y axis, relative to the asset viewer control
    // Return Value:
    //      True if the given 2D point is inside the asset's thumb rectangle
    // See Also:
    //      HitTest(CRect)
    virtual bool HitTest(int nX, int nY) const = 0;
    // Description:
    //      Checks if the given rectangle intersects the asset thumb's rectangle
    // Arguments:
    //      nX - mouse pointer position on X axis, relative to the asset viewer control
    //      nY - mouse pointer position on Y axis, relative to the asset viewer control
    // Return Value:
    //      True if the given rectangle intersects the asset thumb's rectangle
    // See Also:
    //      HitTest(int nX,int nY)
    virtual bool HitTest(const QRect& roTestRect) const = 0;
    // Description:
    //      When user drags this asset item into a viewport, this method is called when the dragging operation ends
    //      and the mouse button is released,   for the asset to return an instance of the asset object to be placed in the level
    // Arguments:
    //      aX - instance's X position component in world coordinates
    //      aY - instance's Y position component in world coordinates
    //      aZ - instance's Z position component in world coordinates
    // Return Value:
    //      The newly created asset instance (Example: BrushObject*)
    // See Also:
    //      MoveInstanceInViewport()
    virtual void* CreateInstanceInViewport(float aX, float aY, float aZ) = 0;
    // Description:
    //      When the mouse button is released after level object creation, the user now can move the mouse
    //      and move the asset instance in the 3D world
    // Arguments:
    //      pDraggedObject -    the actual entity or brush object (CBaseObject* usually) to be moved around with the mouse
    //                                          returned by the CreateInstanceInViewport()
    //      aNewX - the new X world coordinates of the asset instance
    //      aNewY - the new Y world coordinates of the asset instance
    //      aNewZ - the new Z world coordinates of the asset instance
    // Return Value:
    //      True if asset instance was moved properly
    // See Also:
    //      CreateInstanceInViewport()
    virtual bool MoveInstanceInViewport(const void* pDraggedObject, float aNewX, float aNewY, float aNewZ) = 0;
    // Description:
    //      This will be called when the user presses ESCAPE key when dragging the asset in the viewport, you must delete the given object
    //      because the creation was aborted
    // Arguments:
    //      pDraggedObject - the asset instance to be deleted ( you must cast to the needed type, and delete it properly )
    // See Also:
    //      CreateInstanceInViewport()
    virtual void AbortCreateInstanceInViewport(const void* pDraggedObject) = 0;
    // Description:
    //      This method is used to cache/load asset's data, so it can be previewed/rendered
    // Return Value:
    //      True if the asset was successfully cached
    // See Also:
    //      UnCache()
    virtual bool Cache() = 0;
    // Description:
    //      This method is used to force cache/load asset's data, so it can be previewed/rendered
    // Return Value:
    //      True if the asset was successfully forced cached
    // See Also:
    //      UnCache(), Cache()
    virtual bool ForceCache() = 0;
    // Description:
    //      This method is used to load the thumbnail image of the asset
    // Return Value:
    //      True if thumb loaded ok
    // See Also:
    //      UnloadThumbnail()
    virtual bool LoadThumbnail() = 0;
    // Description:
    //      This method is used to unload the thumbnail image of the asset
    // See Also:
    //      LoadThumbnail()
    virtual void UnloadThumbnail() = 0;
    // Description:
    //      This is called when the asset starts to be previewed in full detail, so here you can load the whole asset, in fine detail
    //      ( textures are fully loaded, models etc. ). It is called once, when the Preview dialog is shown
    // Arguments:
    //      hPreviewWnd - the window handle of the quick preview dialog
    //      hMemDC - the memory DC used to render assets that can render themselves in the DC, otherwise they will render in the dialog's HWND
    // See Also:
    //      OnEndPreview(), GetCustomPreviewPanelHeader()
    virtual void OnBeginPreview(QWidget* hPreviewWnd) = 0;
    // Description:
    //      Called when the Preview dialog is closed, you may release the detail asset data here
    // See Also:
    //      OnBeginPreview(), GetCustomPreviewPanelHeader()
    virtual void OnEndPreview() = 0;
    // Description:
    //      If the asset has a special preview panel with utility controls, to be placed at the top of the Preview window, it can return an child dialog window
    //      otherwise it can return NULL, if no panel is available
    // Arguments:
    //      pParentWnd - a valid CDialog*, or NULL
    // Return Value:
    //      A valid child dialog window handle, if this asset wants to have a custom panel in the top side of the Asset Preview window,
    //      otherwise it can return NULL, if no panel is available
    // See Also:
    //      OnBeginPreview(), OnEndPreview()
    virtual QWidget* GetCustomPreviewPanelHeader(QWidget* pParentWnd) = 0;
    virtual QWidget* GetCustomPreviewPanelFooter(QWidget* pParentWnd) = 0;
    // Description:
    //      Used when dragging/rotate/zoom a model, or other asset that can support preview
    // Arguments:
    //      hRenderWindow - the rendering window handle
    //      rstViewport - the viewport rectangle
    //      aMouseX - the render window relative mouse pointer X coordinate
    //      aMouseY - the render window relative mouse pointer Y coordinate
    //      aMouseDeltaX - the X coordinate delta between two mouse movements
    //      aMouseDeltaY - the Y coordinate delta between two mouse movements
    //      aMouseWheelDelta - the mouse wheel scroll delta/step
    //      aKeyFlags - the key flags, see WM_LBUTTONUP
    // See Also:
    //      OnPreviewRenderKeyEvent()
    virtual void PreviewRender(
        QWidget* hRenderWindow,
        const QRect& rstViewport,
        int aMouseX = 0, int aMouseY = 0,
        int aMouseDeltaX = 0, int aMouseDeltaY = 0,
        int aMouseWheelDelta = 0, UINT aKeyFlags = 0) = 0;
    // Description:
    //      This is called when the user manipulates the assets in interactive render and a key is pressed ( with down or up state )
    // Arguments:
    //      bKeyDown - true if this is a WM_KEYDOWN event, else it is a WM_KEYUP event
    //      aChar - the char/key code pressed/released
    //      aKeyFlags - the key flags, compatible with WM_KEYDOWN/UP events
    // See Also:
    //      InteractiveRender()
    virtual void OnPreviewRenderKeyEvent(bool bKeyDown, UINT aChar, UINT aKeyFlags) = 0;
    // Description:
    //      Called when user clicked once on the thumb image
    // Arguments:
    //      point - mouse coordinates relative to the thumbnail rectangle
    //      aKeyFlags - the key flags, see WM_LBUTTONDOWN
    // See Also:
    //      OnThumbDblClick()
    virtual void OnThumbClick(const QPoint& point, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers) = 0;
    // Description:
    //      Called when user double clicked on the thumb image
    // Arguments:
    //      point - mouse coordinates relative to the thumbnail rectangle
    //      aKeyFlags - the key flags, see WM_LBUTTONDOWN
    // See Also:
    //      OnThumbClick()
    //! called when user clicked twice on the thumb image
    virtual void OnThumbDblClick(const QPoint& point, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers) = 0;
    // Description:
    //      Draw the cached thumb bitmap only, if any, no other kind of rendering
    // Arguments:
    //      hDC - the destination DC, where to draw the thumb
    //      rRect - the destination rectangle
    // Return Value:
    //      True if drawing of the thumbnail was done OK
    // See Also:
    //      Render()
    virtual bool DrawThumbImage(QPainter* painter, const QRect& rRect) = 0;
    // Description:
    //     Writes asset info to a XML node.
    //     This is needed to save cached info as a persistent XML file for the next run of the editor.
    // Arguments:
    //     node - An XML node to contain the info
    // See Also:
    //     FromXML()
    virtual void ToXML(XmlNodeRef& node) const = 0;
    // Description:
    //     Gets asset info from a XML node.
    //       This is needed to get the asset info from previous runs of the editor without re-caching it.
    // Arguments:
    //     node - An XML node that contains info for this asset
    // See Also:
    //     ToXML()
    virtual void FromXML(const XmlNodeRef& node) = 0;

    // From IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface([[maybe_unused]] const IID& riid, [[maybe_unused]] void** ppvObject)
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
#endif // CRYINCLUDE_EDITOR_INCLUDE_IASSETITEM_H
