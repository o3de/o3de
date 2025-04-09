## This is a list of functionality that was removed at some point.

The main goal of this list, is to enable future developers to find old code that might be useful when implementing new,or re-implementing missing features.

|Date  | PR | Short description of removed functionality|
|--|--|--|
| 2021-11-25 |  | ObjectManager - parameter begin/end editing callbacks|
| 2021-11-25 |  | ObjectManager - `InvertSelection()`|
| 2021-11-25 |  | ObjectManager - unused named selection group support|
| 2021-11-25 |  | ObjectManager - unused xml export functionality|
| 2021-11-25 |  | ObjectManager - unused object hiding and freezing functionality|
| 2021-11-25 |  | ObjectManager - unused object renaming and duplicate name detection|
| 2021-11-25 |  | ObjectManager - unused selection callbacks and serialization|
| 2021-11-26 |  | ObjectManager - unused selection support, MoveObjects/HitTestObject/EndEditParams|
| 2021-11-26 |  | Unused code - CloneObject,SelectEntities,EnableUniqueObjectNames,NotifyObjectListeners,CloneChildren, PostClone|
| 2021-11-26 |  | Unused code - GetClassCategories/SetCreateGameObject/FindAndRenameProperty2/IsExporting/IsReloading/StartObjectLoading|
| 2021-11-26 |  | ObjectManager - unused code - ForceID/ConvertToType/SetSkipUpdate/LoadRegistry/UpdateRegisterObjectName/HitTestObjectAgainstRect/SelectObjectInRect|
| 2021-11-26 |  | ObjectManager - unused GetObjects/SelectObjects, ObjectLoader - LoadObjects|
| 2021-11-26 |  | CBaseObject - unused material layers mask support|
| 2021-11-26 |  | CBaseObject - GetWarningsText/HideOrder, unused scaling functions,IsChildOf and GetLinkParent|
| 2021-11-26 |  | CBaseObject - SubObj selection, IsParentAttachmentValid, IMouseCreateCallback,GetWorldAngles|
| 2021-11-26 |  | CBaseObject - procedural floor management, EditTags, HelperScale|
| 2021-11-26 |  | CBaseObject - PropertyChanged,SetNameInternal,OnMenuShowInAssetBrowser|
| 2021-11-26 |  | CObjectArchive - unused sequence remapping, loaded object access |
| 2021-11-26 |  | IDisplayViewport - unused interface `HitTestLine/GetGridStep/setHitcontext`|
| 2021-11-27 |  | CBaseObject and children - unused HitHelperTest/MouseCreateCallback|
| 2021-11-28 |  | CBaseObject - unused OnContextMenu and unused undo description strings. |
| 2021-11-28 |  | Editor KDTree implementation |
| 2021-11-28 |  | CExportManager - unused AddStatObj/AddMeshes/AddMesh|
| 2021-11-28 |  | unused CUndoBaseLibraryManager and CUndoBaseLibrary|
| 2021-11-28 |  | IXmlNode - unused shareChildren/deleteChildAt/clone/insertChild/replaceChild functionality|
| 2021-11-28 |  | CXmlArchive - unused Load/Save methods|
| 2021-12-03 | #6086 | MemoryDriller, PlatformMemoryInstrumentation - Driller no longer functional (Profiler was deleted months ago)|
| 2021-12-03 | #6086 | TraceMessageDrillerBus, TraceMessagesDriller - Driller no longer functional (Profiler was deleted months ago)|
| 2021-12-03 | #6086 | ThreadDrillerEvents - Driller no longer functional (Profiler was deleted months ago)|
| 2021-12-03 | #6086 | CarrierDrillerBus, CarrierDriller, ReplicaDriller, SessionDriller, SessionDrillerBus (from GridMate) - Driller no longer functional (Profiler was deleted months ago)|
| 2021-12-03 | #6086 | AssetTracking, Gems/AssetMemoryAnalyzer - Relies on driller that is no longer functional (Profiler was deleted months ago)|
| 2021-12-03 | #6086 | DrillerBus, DrillerManager, DrillerEvents, EventTraceDrillerBus, Driller, DrillerDefaultStringPool - Driller no longer functional (Profiler was deleted months ago)|
| 2021-12-03 | #6086 | FileIOEvents - Prone to false-positives (errors from other threads), not really useful|
| 2021-12-03 | #6086 | FileIOBus - Unused and not the best way to implement a filesystem replacement|
| 2022-04-20 | #9026 | GridMate - Unused and unmaintained networking solution. Replaced by AzNetworking and MultiplayerGem|
| 2022-04-20 | #9026 | GridHub - Unused and unmaintained discovery service. Usages in TargetManagement replaced by AzNetworking. Other usages have been long since removed|
