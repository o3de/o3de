-------------------------------------------------------------------------------
-- load.ms
-- Version 2.8
-- Loader script to define structures and loading and managing functions
-- By: Mathias Lindner
--   eMail: devsupport@crytek.com
-------------------------------------------------------------------------------


try
(
	if cryTools.showSplash == true then
	(
		if crySplash.open == false then
		(
			bm = (openBitmap (crytools.BuildPathFull + "Tools\\maxscript\\icons\\splash.png"))
			createDialog crysplash style:#(#style_border) modal:false bitmap:bm width:bm.width height:bm.height fgcolor:white
		)

		try crysplash.build.text = "LOADING CryANIM" catch()
	)
)catch()


--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:logOutput
----------------------------------------------------------------------------------------------------------------------------------
--			creates the log and fills it with function definitions, errors
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

function logOutput outputText init:undefined =
(
	logFilepath = (cryTools.cryTemp + "cryAnim_log.txt")
	
	if init != undefined then
		try deleteFile logFilepath catch()
	
	try
	(
		if doesfileexist logFilepath == false then
			logStream = createFile logFilepath
		else
			logStream = openFile logFilepath mode:"a"
		
		if logStream != undefined then
		(
			if findString "!!>" outputText != 0 and getCurrentException() != undefined then
			(
				
				additionString = "-----------------------------------------------------------------------------------\n"
				additionString += (getCurrentException() as String) + "\n" + additionString
				
				if cryTools.suppresswarnings == false then
					break()
				else
					print (outputText + "\n" + additionString)
			)
			else
				additionString = ""
			
			
			
			if trackTime != undefined then
			(
				trackTimeDiv = timeStamp() - trackTimeStop
				if additionString == "" then
					outputText += " [" + (trackTimeDiv ) as String + " ms]"
				
				trackTimeStop = timeStamp()
			)
			
			format (outputText + "\n" + additionString) to:logStream
			close logStream
			
		)
	)
	catch ( print "Error in Log-Output" )
)
logOutput ( "CryAnim Log   " + localTime + "\n===============================================\n" ) init:true


trackTime = timeStamp()
trackTimeStop = timeStamp()







--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:Structures
----------------------------------------------------------------------------------------------------------------------------------
--			structure initialisation
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////




	--// cryTools.cryAnim.base structure
--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
struct aToolsBaseStruct
(
	setVars,
	getFilePaths,
	lowercase,
	readTempFile,
	perforce,
	loadFiles,
	iniFile,
	getPerforcePath,
	setMacros,
	setCallbacks,
	setMenu,
	callStartUp,
	reloadScript,
	killCryAnim
)
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////










	--// cryTools.cryAnim._f structure
--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
struct bipPartSt ( object, part, name, pivotSel )
struct objectSt ( selected, parent, child, isPivot )
struct pivotSelSt ( index, pivotName, position, pivPoint, pivObject )
struct partNameSt ( name, leftRight, bodyPart )
struct pivPointSt ( name, position )
struct pivotDistanceStruct ( distance, name )

struct aToolsFunctionStruct
(
	getBipNames,
	getBaseBip,
	getSelectedBipPart,
	getPivotPointPos,
	getNearestSnapshot,
	moveBipPartPivPoint,
	moveRotBipPartPivPoint,
	moveBipPart,
	moveRotBipPart,
	moveToFloor,
	resetRotation,
	rotateAnim,
	moveToBodyMass,
	bipToZero2,
	bipToZero3,
	setTrajectories,
	resetLocator,
	getChildren,
	getChildExtent,
	createSnapshot,
	lockRotation,
	pasteTransform
)
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////









	
	--// cryTools.cryAnim.align structure
--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
struct selectionSt (	pivot = #(), selection = #() )
struct pivotLastSt (	index = 6, pivotName, position, pivObject, pivPoint = pivPointSt() )
struct partSt ( LHand = pivotLastSt(), RHand = pivotLastSt(), LFoot = pivotLastSt(), RFoot = pivotLastSt(), tempSelection )
struct pivotListSt ( pointList = selectionSt(), edgeList = selectionSt(), pivotLast = partSt() )
struct pointEdgeSt ( pivot, selection )
struct activePivotSt ( interPivot, interPos )
	
struct aToolsAlignFunctionStruct
(
	createPivotEdge,
	createPivotPoint,
	createPointEdge,
	initVars,
	interactPivotPoint,
	setPivotPoint,
	clickAction,
	deletePivotPoints,
	switchViewPort,
	callPivotSelect
)	

struct aToolsAlignStruct
(
	pivotSelect,
	_f = aToolsAlignFunctionStruct(),
	_v = pivotListSt()
)
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////













	--// cryTools.cryAnim.UI.main.operation structure
--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
struct operationFunctionStruct
(
	updateDialog,
	getSelectedAxis,
	align,
	alignLine,
	applyOperation
)

struct operationStruct
(
	_f = operationFunctionStruct(),
	_v = #()
)
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////










	
	--// cryTools.cryAnim.UI.main.pivot structure
--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
struct pivotFunctionStruct
(
	pivotPoint
)

struct pivotStruct
(
	_f = pivotFunctionStruct(),
	_v = #()
)
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////











	--// cryTools.cryAnim.UI.main.poseManager structure
--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
struct charStruct ( name, nodes = #(), collections = #() )
struct collectionStruct ( name, poses = #() )
struct poseStruct ( name, nodes = #() )
struct nodeStruct ( name, transform )

struct poseManagerFunctionStruct
(
	outputINI,
	inputINI,
	createPoseBitmap,
	updateList,
	addEntry,
	createEntry,
	deleteEntry,
	renameEntry,
	applyPose
)

struct poseManagerVarsStruct
(
	poses = #()
)

struct poseManagerStruct
(
	createDialog,
	renameDialog,
	_f = poseManagerFunctionStruct(),
	_v = poseManagerVarsStruct()
)
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////












	--// cryTools.cryAnim.UI.main.loadSave structure
--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
struct loadSaveFunctionStruct
(
	progressDialog,
	checkRange,
	progressDialog,
	updateProgressBar,
	bakeMuscleBones,
	getStatusExtents,
	getProgressInfo
)

struct loadSaveVarsStruct
(
	bipOpenPath = "",
	bipSavePath = "",
	cafSavePath = ""
)

struct loadSaveStruct
(
	testBar,
	showSelectedFile,
	_f = loadSaveFunctionStruct(),
	_v = loadSaveVarsStruct()
)
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////









	--// cryTools.cryAnim.UI.main.models structure
--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
struct modelPathStruct ( name, path )
struct itemStruct ( name, external, model, parent, reference, rotation, position )

struct modelsFunctionStruct
(
	selectItem,
	sortList,
	setVisible,
	getEntries,
	LoadModel,
	updateExtent,
	updateExtentModel,
	selectByNameFilterNode,
	selectByNameFilterParent
)

struct modelsVarsStruct
(
	itemList = #()
)

struct modelsStruct
(
	editItemList,
	entryDetails,
	editModelList,
	loadModelRC,
	_f = modelsFunctionStruct(),
	_v = modelsVarsStruct()
)
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////










	--// cryTools.cryAnim.UI.main.target structure
--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
struct targetFunctionStruct
(
	updateDDSelection,
	updateOffset,
	setManualOffset
)

struct targetStruct
(
	_f = targetFunctionStruct(),
	_v = #()
)
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////





struct settingsFunctionStruct
(
	callCustomizeDialog
)

struct settingsStruct
(
	customizeRollouts,
	_f = settingsFunctionStruct(),
	_v = #()
)










--################################################################
struct mainDialogVarsStruct
(
	animToolsINIPath,
	tempINIPath,
	bipWorkingDir = "",
	cafWorkingDir = "",
	dialogMaxHeight
)

struct controlStruct ( name, rolloutName, tooltip, pos, width, height )
struct mainDialogFunctionStruct
(
	getUI,
	checkExport,
	updateDialog,
	callDialog
)

struct mainDialogStruct
(
	dialog,
	rollouts = #(),
	loadSave = loadSaveStruct(),
	models = modelsStruct(),
	operation = operationStruct(),
	pivot = pivotStruct(),
	poseManager = poseManagerStruct(),
	target = targetStruct(),
	settings = settingsStruct(),
	_f = mainDialogFunctionStruct(),
	_v = mainDialogVarsStruct()
)
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////











	--// cryTools.cryAnim.UI.batchProcess structure
--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
struct boneStruct ( active, name, external, bones )
struct resultStruct ( index, name )
struct maskStruct (	normal = #(),	add = #(), remove = #(), extension = #() )
struct subFolderStruct ( path, node )

struct subRangeStruct ( objects, range, export )

struct batchFileStruct
(
	filename,
	path,
	details,
	process,
	used,
	extension,
	anm,
	node,
	subRanges = #(),
	subRangePrefix = ""
)

struct batchProcessVarsStruct
(
	sourcePath = "",
	exportPath = "",
	customScripts = #(),
	customScriptsSave = #(),
	customScriptStatus = "",
	customScriptSelected = 0,
	flags = #{},
	subFolders = #(),
	exportFiles = #(),
	selectedFile,
	fileQue,
	exportBones,
	boneList = #()
)

struct batchProcessFunctionStruct
(
	subRangeCallDialog,
	subRangeUpdateList,
	subRangeGetRange,
	subRangeUpdateRange,
	subRangeUpdateValues,
	subRangeSetList,
	subRangeLoadFromDummy,
	subRangeUpdateDialog,
	subRangeCreateUDPString,
	
	addNode,
	callDialog,
	updateSubFolderSelection,
	updateSubFolders,
	getCheckedNodes,
	updateCounter,
	getScriptList,
	updateScriptLists,
	createScriptDialog,
	getAllFolders,
	getAllFiles,
	getBoneDetection,
	getFilename,
	getExtension,
	findMaskEntry,
	getFileFound,
	refreshFiles,
	updateDialog,
	generateOutput,
	executeCustomScripts,
	loadBatchFile,
	processFiles,
	updateExtent
)

struct batchProcessDialogStruct
(
	dialog,
	customizeScript,
	subRangeDialog,
	progressDialog,
	editBoneList,
	entryDetails,
	_f = batchProcessFunctionStruct(),
	_v = batchProcessVarsStruct()
)
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////














	--// cryTools.cryAnim.UI structure
--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
struct aToolsUIStruct
(
	main = mainDialogStruct(),
	batchProcess = batchProcessDialogStruct()
)
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////












	--// cryTools.cryAnim._v structure
--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
struct rotPosStruct ( rotation = (eulerangles 0 0 0), position = [0,0,0] )

struct aToolsGlobalVarsStruct
(
	various = #(),
	tempATPath,
	perforceDir = "",
	lockRotation,
	transformList = #()
)
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////









	--// cryTools.cryAnim structure
--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
struct aToolsStruct
(
	_v = ( aToolsGlobalVarsStruct() ),
	_f = ( aToolsFunctionStruct() ),
	base = ( aToolsBaseStruct() ),
	align = ( aToolsAlignStruct() ),
	UI = ( aToolsUIStruct() )
)
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

logOutput "> Initialized main structures"



if cryTools.cryAnim != undefined then
	reloadCryAnim = true

try cryTools.cryAnim.base.killCryAnim() catch()


cryTools.cryAnim = aToolsStruct()



logOutput "> Created CryAnim structure interface"

































	





--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.base.getFilePaths
----------------------------------------------------------------------------------------------------------------------------------
--			gets the paths of all used functions when loading and return the perforce path
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.base.getFilePaths = function getFilePaths op =
(
	try
	(
		struct ATFileStruct ( aToolsPaths = #(), perforcePaths = #() )
		local JPath = cryTools.BuildPathFull + "Tools\\maxscript\\cryAnim\\"
		local ATFilePath = ATFileStruct()
		
		append ATFilePath.aToolsPaths (JPath + "load.ms")
		append ATFilePath.aToolsPaths (JPath + "functions.ms")
		
		append ATFilePath.perforcePaths (JPath + "load.ms")
		append ATFilePath.perforcePaths (JPath + "functions.ms")
		append ATFilePath.perforcePaths (JPath + "ui\\main\\operation.ms")
		append ATFilePath.perforcePaths (JPath + "ui\\main\\perforce.ms")
		append ATFilePath.perforcePaths (JPath + "ui\\main\\pivot.ms")
		append ATFilePath.perforcePaths (JPath + "ui\\main\\pose.ms")
		append ATFilePath.perforcePaths (JPath + "ui\\main\\save.ms")
		append ATFilePath.perforcePaths (JPath + "ui\\main\\target.ms")
		append ATFilePath.perforcePaths (JPath + "ui\\main\\tools.ms")
		append ATFilePath.perforcePaths (JPath + "ui\\main\\locator.ms")
		append ATFilePath.perforcePaths (JPath + "ui\\main\\muscle.ms")
		append ATFilePath.perforcePaths (JPath + "ui\\main\\models\\models8.ms")
		append ATFilePath.perforcePaths (JPath + "ui\\main\\models\\models9.ms")
		append ATFilePath.perforcePaths (JPath + "ui\\batch\\batch8.ms")
		append ATFilePath.perforcePaths (JPath + "ui\\batch\\batch9.ms")
		append ATFilePath.perforcePaths (JPath + "ui\\batch\\Scripts\\AutoLoc.ms")
		append ATFilePath.perforcePaths (JPath + "ui\\batch\\Scripts\\Rotate180.ms")
		append ATFilePath.perforcePaths (JPath + "ui\\batch\\Scripts\\ConvertTCB.ms")
		append ATFilePath.perforcePaths (JPath + "ui\\batch\\Scripts\\bipExport.ms")
		append ATFilePath.perforcePaths (JPath + "ui\\batch\\Scripts\\bipSave.ms")
		append ATFilePath.perforcePaths (JPath + "ui\\batch\\Scripts\\bipSaveExport.ms")
		
		case op of
		(
			#perforcePath:
			(
				newString = substring JPath cryTools.buildPathFull.count JPath.count
				
				
				local perforcePath = cryTools.cryAnim._v.perforceDir
				if perforcePath != undefined then
				(
					perforcePath += "/"
					local tempArray = filterString newString "\\"
					for i = 1 to tempArray.count do
						perforcePath += tempArray[i] + (if i < tempArray.count then "/" else "")
				)
				return perforcePath
			)
			
			#localPath: return JPath
			
			#aToolsPaths: return ATFilePath
		)
	)
	catch ( logOutput "!!> Error in cryAnim.base.getFilePaths" )
)
getFilePaths = undefined
logOutput "> Created cryAnim.base.getFilePath function"













--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.base.lowercase
----------------------------------------------------------------------------------------------------------------------------------
--			converts string to lower case
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.base.lowercase = function lowercase input =
(
	try
	(
		local tempString = ""
		
		for i = 1 to input.count do
		(
			x = input[i]
			n = bit.charAsInt x
			if (n > 64) and (n < 90) then
				x = bit.intAsChar (n + 32)
			tempString += x
		)
		return tempString
	)
	catch ( logOutput "!!> Error in cryAnim.base.lowercase"; return "" )
)
lowercase = undefined
logOutput "> Created cryAnim.base.lowercase function"










--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.base.readTempFile
----------------------------------------------------------------------------------------------------------------------------------
--			reads the temp file generated by perforce
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.base.readTempFile = function readTempFile =
(
	try
	(
		local tempSize = 0
		local tempString = ""
		
			--// if file temp file exists
		if doesFileExist cryTools.cryAnim._v.tempATPath == true then
		(
		
				--// wait, until maxscript can access the file
			while tempStream == undefined do --tempSize < 1 do
			(
				escapeEnable = true
				tempStream = openFile cryTools.cryAnim._v.tempATPath mode:"r"
			)
				
			
			--// open the temp file
			if getFileSize cryTools.cryAnim._v.tempATPath == 0 then
			(
				print "Perforce not found..."			
				return undefined			
			)
				--// read all lines
			while eof tempStream != true do
				tempString += (readLine tempStream) + "\n"

				--// close the file
			close tempStream
				--// delete file
			deleteFile cryTools.cryAnim._v.tempATPath
			
				--// returns the lines of the file
			return tempString
		)
		else
			return undefined
	)
	catch ( logOutput "!!> Error in cryAnim.base.readTempFile" )
)
readTempFile = undefined
logOutput "> Created cryAnim.base.readTempFile function"











--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.base.perforce
----------------------------------------------------------------------------------------------------------------------------------
--			connection between perforce and max
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.base.perforce = function perforce filename op =
(
	try
	(
		case op of
		(
			#localToDepot:
			(
				try
				(
					buildFilter = filterString cryTools.buildPathFull "\\"
					buildString = ""
					for i = 1 to (buildFilter.count - 1) do
						buildString += buildFilter[i] + "\\"
						
					perforceFilter = filterString cryTools.cryAnim._v.perforceDir "/"
					perforceString = "//"
					for i = 1 to (perforceFilter.count - 1) do
						perforceString += perforceFilter[i] + (if i < (perforceFilter.count - 1) then "/" else "")
					
					pathString = substring filename buildString.count filename.count

					
					tempFilter = filterString pathString "\\"
					if tempFilter.count > 1 then
					(
						tempString = perforceString
						for i = 1 to tempFilter.count do
							tempString += "/" + tempFilter[i]
						
						return tempString
					)
					else
						return false
						
				)catch (return undefined)
			)
			#depotToLocal:
			(
				try
				(
					tempFilter = filterString filename "/"
					if tempFilter.count > 1 then
					(
						tempString = (filterString crytools.BUIldPathFull "\\")[1]
						
						for i = 2 to tempFilter.count do
							tempString += "\\" + tempFilter[i]
					
						
						return tempString
					)
					else
						return false
				)
				catch(return undefined)
			)
			
			#getFilename:
			(
				tempFilter = filterString filename "\\"
				return tempFilter[tempFilter.count]
			)
			
			#getDirectory:
			(
				tempFilter = filterString filename "\\"
				tempString = ""
				for i = 1 to (tempFilter.count - 1) do
					tempString += tempFilter[i] + "\\"
				
				return tempString
			)
			
			#getFileStats:
			(
				struct tempATFileStruct
				(
					depotFile,
					clientFile,
					headRev,
					haveRev,
					otherOpen,
					userOpen,
					exist
				)
				
				tempStruct = tempATFileStruct exist:true
				
				tempPath = cryTools.cryAnim.base.perforce filename #localToDepot
				
				if (tempPath != false) and (tempPath != undefined) then
				(					
					cryTools.scmd ("p4 -s fstat " + tempPath + " > \"" + cryTools.cryAnim._v.tempATPath + "\"") false
				
					tempString = ""
					while tempString == "" do
					(
						escapeEnable = true
						tempString = cryTools.cryAnim.base.readTempFile()
					)
					if tempString != undefined then
					(
						tempFilter = filterString tempString "\n"
						
						
						
						for i = 1 to tempFilter.count do
						(
							tempSecFilter = filterString tempFilter[i] " "
							if (findString tempFilter[i] "depotFile") != undefined then
								tempStruct.depotFile = tempSecFilter[tempSecFilter.count]
							if (findString tempFilter[i] "clientFile") != undefined then
								tempStruct.clientFile = tempSecFilter[tempSecFilter.count]
							if (findString tempFilter[i] "headRev") != undefined then
								tempStruct.headRev = tempSecFilter[tempSecFilter.count]
							if (findString tempFilter[i] "haveRev") != undefined then
								tempStruct.haveRev = tempSecFilter[tempSecFilter.count]
							if (findString tempFilter[i] "otherOpen0") != undefined then
								tempStruct.otherOpen = tempSecFilter[tempSecFilter.count]
							if (findString tempFilter[i] "actionOwner") != undefined then
								tempStruct.userOpen = true
							if (findString tempFilter[i] "no such file") != undefined then
								tempStruct.exist = false
						)
						
						return tempStruct
					)
				)
				--else
				return undefined
			)
				
			#getLatestRev:
			(
				cryTools.scmd ("p4 sync -f " + (cryTools.cryAnim.base.perforce filename #localToDepot) ) false
				return true				
			)
				
			#open:
			(
				cryTools.cryAnim.base.perforce filename #getLatestRev
				cryTools.scmd ("p4 edit " + (cryTools.cryAnim.base.perforce filename #localToDepot) ) false
				return true
			)
			#add:
			(
				tempBinary == ""
				fileType = cryTools.cryAnim.base.perforce filename #getFilename
				tempString = cryTools.cryAnim.base.lowercase fileType
				
				if (findString tempString ".caf") != undefined then tempBinary = "-t binary+l "
				if (findString tempString ".bip") != undefined then tempBinary = "-t binary+l "
				
				cryTools.scmd ("p4 add " + tempBinary + (cryTools.cryAnim.base.perforce filename #localToDepot) ) false
				return true
			)
			
			#getPath:
			(
				if cryTools.scmd != undefined then
				(
					cryTools.scmd ("p4 where " + filename + " > \"" + cryTools.cryAnim._v.tempATPath + "\"") false
					
					tempString = ""
					while tempString == "" do
					(
						escapeEnable = true
						tempString = cryTools.cryAnim.base.readTempFile()
					)
					
					try ( return (filterString tempString " ")[1] ) catch (return undefined)
				)
				else
					return undefined
			)
			
			
			#checkForLoad:
			(
				tempStruct = cryTools.cryAnim.base.perforce filename #getFileStats
				if tempStruct != undefined then
				(
					if tempStruct.exist == true then
					(
						if tempStruct.otherOpen != undefined then
						(
							messageBox ("The file is currently opened by:\n\n" + tempStruct.otherOpen) title:("Perforce Check:    " + (cryTools.cryAnim.base.perforce filename #getFilename))
						)
						else
						(
							if tempStruct.userOpen == undefined then
							(
								if (queryBox "Do you want to 'Open for Edit' ?" title:("Perforce Check:   " + (cryTools.cryAnim.base.perforce filename #getFilename))) == true then
								(
									if tempStruct.headRev != tempStruct.haveRev then
									(
										cryTools.cryAnim.base.perforce filename #getLatestRev										
									)
									
									cryTools.cryAnim.base.perforce filename #open																		
								)
								else
									return false
							)							
							return true
						)
					)
					return false
				)
				else
					return undefined
			)
			
			#checkForSave:
			(
				tempCBAStruct = cryTools.cryAnim.base.perforce (cryTools.buildPathFull + "Game\\Animations\\Animations.cba") #getFileStats
				if tempCBAStruct != undefined then
				(
					if tempCBAStruct.headRev != tempCBAStruct.haveRev then
						if (cryTools.cryAnim.base.perforce (cryTools.buildPathFull + "Game\\Animations\\Animations.cba") #getLatestRev) == true then
							print "Synched - Animations.cba"
				)
				
				tempStruct = cryTools.cryAnim.base.perforce filename #getFileStats
				if tempStruct != undefined then
				(
					if tempStruct.exist == true then
					(
						if tempStruct.headRev != tempStruct.haveRev then
						(
							cryTools.cryAnim.base.perforce filename #getLatestRev
						)
						
						if tempStruct.userOpen == undefined then
						(
							if tempStruct.otherOpen != undefined then
							(
								return (queryBox ("The file is currently opened by:\n\n" + tempStruct.otherOpen + "\n\nOverwrite File?") title:("Perforce Check:    " + (cryTools.cryAnim.base.perforce filename #getFilename)) beep:true)
							)
							if (queryBox "Do you want to 'Open for Edit' ?" title:("Perforce Check:   " + (cryTools.cryAnim.base.perforce filename #getFilename))) == true then
							(
								cryTools.cryAnim.base.perforce filename #open
								return true
							)
							else
							(
								cryTools.cryAnim.base.perforce filename #setReadMessage
								return false
							)
																
						)
						else
							return true						
					)
					else
					(
						cryTools.cryAnim.base.perforce filename #setReadMessage
						return false
					)
				)
				else
					return undefined
			)
			
			
			#setRead:
			(
					--// set attribute to read --
				if ((getFiles filename)[1]) != undefined then
				(
					if (getFileAttribute filename #readOnly) == true then
					(
						setFileAttribute filename #readOnly false						
						return true
					)
					else
						return false
				)
				else
					return undefined
			)
			#setReadMessage:
			(
				if (cryTools.cryAnim.base.perforce filename #setRead) == true then
				(
					try
					(
						if (cryTools.cryAnim.getUI "Perforce" "chkReadOnly").checked == true then
							messageBox ("Set File Attribute to Read") title:("File Check:    " + (cryTools.cryAnim.base.perforce filename #getFilename))
					)catch()
					return true
				)
				else
					return false
			)			
			
			#checkForAdd:
			(				
				tempStruct = cryTools.cryAnim.base.perforce filename #getFileStats
				if tempStruct != undefined then
				(
					if tempStruct.exist == false then
					(
						if (queryBox "The file is not in the database.\nAdd to Source Control?" title:("Perforce Check:   " + (cryTools.cryAnim.base.perforce filename #getFilename))) == true then
						(
							cryTools.cryAnim.base.perforce filename #add
						)
					)
				)
			)
			
			#checkLoading:
			(
				
				local fileArray = #()
				local perforcePath = cryTools.cryAnim.base.getFilePaths #perforcePath			
				if perforcePath != undefined then
				(
					local tempFileString = ""
					local tempFileList = #()		
					local ATFileListPath = (cryTools.cryAnim.base.getFilePaths #aToolsPaths).perforcePaths
					local ATFileListHeadRev = #()
					local ATFileListHaveRev = #()
					local ATUpdatedFiles = #()
					local tempFileString = ""
					
					while tempFileString == "" do
					(
						escapeEnable = true
						tempFileString = cryTools.cryAnim.base.readTempFile()
					)
					
					for i = 1 to tempFileList.count do
					(
						if (findString tempFileList[i] "clientFile") != undefined then
						(
							append ATFileListPath (substring tempFileList[i] 19 tempFileList[i].count)
						)
					)
					
					tempBool = false
					
					for i = 1 to ATFileListPath.count do
					(
						local tempFileStat = cryTools.cryAnim.base.perforce ATFileListPath[i] #getFileStats
						
						if (tempFileStat.depotFile != undefined) and (tempFileStat.haveRev != undefined) then
						(
							if (tempFileStat.haveRev != tempFileStat.headRev) then
							(
								cryTools.cryAnim.base.perforce ATFileListPath[i] #getLatestRev
								append ATUpdatedFiles (cryTools.cryAnim.base.perforce ATFileListPath[i] #getFilename)
								print (" Synched - " + (cryTools.cryAnim.base.perforce ATFileListPath[i] #getFilename) )						
								tempBool = true
							)
						)
					)
					
					
					
					if tempBool == false then
						return false
					else
					(						
						tempMessageFilesString = ""
						local tempMessageBottomString = ""
						local tempMessageString = ""
						local compareFileSize = undefined
						
						for i = 1 to ATUpdatedFiles.count do
							tempMessageFilesString += "\n" + ATUpdatedFiles[i]
						
							
						return ATUpdatedFiles
						
						
					)
						
					return true
				)
				return undefined
							
			)
			
			#messageOnLoading:
			(
				local tempMessageString = ""
				local tempMessageBottomString = ""
				local tempMessageFilesString = ""
			
				if filename.count > 1 then
					tempMessageString = "These Files were Updated:\n"
				else
					tempMessageString = "This File was Updated:\n"
					for i = 1 to filename.count do
						tempMessageFilesString += "\n" + filename[i]
					
				if cryTools.suppressWarnings == false then
					messageBox (tempMessageString + tempMessageFilesString + tempMessageBottomString) title:"CryAnim Perforce Check"
			)
			
			#messageOnUpdate:
			(
				local tempMessageString = ""
				local tempMessageBottomString = ""
				local tempMessageFilesString = ""
			
				if filename.count > 1 then
					tempMessageString = "These Files were Updated:\n"
				else
					tempMessageString = "This File was Updated:\n"
				
				for i = 1 to filename.count do
					tempMessageFilesString += "\n   " + filename[i]
				
				tempMessageFilesString += "\n\n Do you want to reload   cryAnim?"
				
				if cryTools.suppressWarnings == false then
					if (queryBox (tempMessageString + tempMessageFilesString + tempMessageBottomString) title:"CryAnim Perforce Check") == true then
						cryTools.cryAnim.base.reloadScript()
			)

			
			#autoUpdate:
			(
				local tempVar = cryTools.cryAnim.base.perforce "" #checkLoading
				if tempVar != false then
					cryTools.cryAnim.base.perforce tempVar #messageOnUpdate
			)		
		)
	)
	catch ( logOutput "!!> Error in cryAnim.base.perforce" )
)
perforce = undefined
logOutput "> Created cryAnim.base.perforce function"













--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.base.loadFiles
----------------------------------------------------------------------------------------------------------------------------------
--			loads used files
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.base.loadFiles = function loadFiles op =
(
	try
	(
		local ATFilePath = cryTools.cryAnim.base.getFilePaths #aToolsPaths
		
		case op of
		(
			#load:(	fileIn ATFilePath.aToolsPaths[1] ;  return true )
			#all:
			(				
				for i = 2 to ATFilePath.aToolsPaths.count do
				(
					try	( fileIn ATFilePath.aToolsPaths[i] )
					catch ( print ("Can't load: " + ATFilePath.aToolsPaths[i] ) )
				)
				return true
			)			
			#functions:( fileIn ATFilePath.aToolsPaths[2] ; return true )
			#pivot:( fileIn ATFilePath.aToolsPath[3] ; return true	)
			#dialog:( fileIn ATFilePath.aToolsPath[4] ;  return true )
		)
	)
	catch ( logOutput "!!> Error in cryAnim.base.loadFiles" )
)
loadFiles = undefined
logOutput "> Created cryAnim.base.loadFiles function"











--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.base.iniFile
----------------------------------------------------------------------------------------------------------------------------------
--			input and output of the cryAnim ini file
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.base.iniFile = function iniFile op item value:undefined =
(
	try
	(
		try 
		(
			csexport.set_value
		
			case op of
			(
				#set:
				(
					case item of
					(
						-- Default Settings --
						#dialogPos: ( try ( csexport.set_value "cryAnimDlgPos" ( if value == undefined then ( (getDialogPos cryTools.cryAnim.UI.main.dialog) as String) else value) ) catch() )
						#visible: ( try (csexport.set_value "cryAnimDlgVisible" value ) catch() )
						#autoUpdate: ( try ( csexport.set_value "cryAnimDlgAutoUpdate" (if value == undefined then (if (cryTools.cryAnim.UI.main._f.getUI "Perforce" "chkAutoUpdate").checked == true then "1" else "0") else value ) ) catch() )
						#autoUpdateTime: ( try ( csexport.set_value "cryAnimDlgAutoUpdateTime" (if value == undefined then ( (cryTools.cryAnim.UI.main._f.getUI "Perforce" "spnAutoUpdateTime").value as String )  else value ) ) catch() )
						#multiRow: ( try ( csexport.set_value "cryAnimDlgMultiRow" (if value == undefined then (if (cryTools.cryAnim.UI.main._f.getUI "Settings" "chkMultiRow").checked == true then "1" else "0")  else value ) ) catch() )
						#rolloutStates: ( try ( csexport.set_value "cryAnimDlgRolloutStates" (if value == undefined then (if (cryTools.cryAnim.UI.main._f.getUI "Settings" "chkRolloutStates").checked == true then "1" else "0")  else value ) ) catch() )
						#readOnly: ( try ( csexport.set_value "cryAnimDlgReadOnly" (if value == undefined then ( if (cryTools.cryAnim.UI.main._f.getUI "Settings" "chkReadOnly").checked == true then "1" else "0" ) else value ) ) catch () )
						#workingDir: ( try ( csexport.set_value "cryAnimDlgWorkingDir" (if value == undefined then ( (cryTools.cryAnim.UI.main._f.getUI "Settings" "edWorkingDir").text ) else value ) ) catch () )
						#loadBiped: ( try ( csexport.set_value "cryAnimDlgLoadBiped" (if value == undefined then cryTools.cryAnim.UI.main.loadSave._v.bipOpenPath else value ) ) catch () )



						-- Open For Edit --
						#loadOpen: ( try ( csexport.set_value "cryAnimDlgOFELoad" (if value == undefined then (case (cryTools.cryAnim.UI.main._f.getUI "Perforce" "radLoadOpen").state of ( 1: "1"; 2: "2"; 3: "3" ); )  else value ) ) catch() )
						#saveOpen: ( try ( csexport.set_value "cryAnimDlgOFESave" (if value == undefined then (case (cryTools.cryAnim.UI.main._f.getUI "Perforce" "radSaveOpen").state of ( 1: "1"; 2: "2"; 3: "3" ); )  else value ) ) catch() )
						#exportOpen: ( try ( csexport.set_value "cryAnimDlgOFEExport" (if value == undefined then (case (cryTools.cryAnim.UI.main._f.getUI "Perforce" "radExportOpen").state of ( 1: "1"; 2: "2"; 3: "3" ); )  else value ) ) catch() )
						#saveExportOpen: ( try ( csexport.set_value "cryAnimDlgOFESaveExport" (if value == undefined then (case (cryTools.cryAnim.UI.main._f.getUI "Perforce" "radSaveExportOpen").state of ( 1: "1"; 2: "2"; 3: "3" ); )  else value ) ) catch()  )

						-- Add To Source Control --
						#saveAdd: ( try ( csexport.set_value "cryAnimDlgASCSave" (if value == undefined then (case (cryTools.cryAnim.UI.main._f.getUI "Perforce" "radSaveAdd").state of ( 1: "1"; 2: "2"; 3: "3" ); )  else value ) ) catch() )
						#exportAdd: ( try ( csexport.set_value "cryAnimDlgASCExport" (if value == undefined then (case (cryTools.cryAnim.UI.main._f.getUI "Perforce" "radExportAdd").state of ( 1: "1"; 2: "2"; 3: "3" ); )  else value ) ) catch() )
						#saveExportAdd: ( try ( csexport.set_value "cryAnimDlgASCSaveExport" (if value == undefined then (case (cryTools.cryAnim.UI.main._f.getUI "Perforce" "radSaveExportAdd").state of ( 1: "1"; 2: "2"; 3: "3" ); )  else value ) ) catch()  )
						
						-- Prompt for File --
						#savePrompt: ( try ( csexport.set_value "cryAnimDlgPFFSave" (if value == undefined then (case (cryTools.cryAnim.UI.main._f.getUI "Settings" "radSavePrompt").state of ( 1: "1"; 2: "2" ); )  else value ) ) catch() )
						#exportPrompt: ( try ( csexport.set_value "cryAnimDlgPFFExport" (if value == undefined then (case (cryTools.cryAnim.UI.main._f.getUI "Settings" "radExportPrompt").state of ( 1: "1"; 2: "2" ); )  else value ) ) catch() )
						#saveExportPrompt: ( try ( csexport.set_value "cryAnimDlgPFFSaveExport" (if value == undefined then (case (cryTools.cryAnim.UI.main._f.getUI "Settings" "radSaveExportPrompt").state of ( 1: "1"; 2: "2" ); )  else value ) ) catch()  )

						
						
						-- Rollout Settings --
						#targetRO: ( try ( csexport.set_value "cryAnimDlgRolloutTarget" (if value == undefined then ( if (cryTools.cryAnim.UI.main._f.getUI "Target" "").open == true then "1" else "0" ) else value ) ) catch () )
						#operationRO: ( try ( csexport.set_value "cryAnimDlgRolloutOperation" (if value == undefined then ( if (cryTools.cryAnim.UI.main._f.getUI "Operation" "").open == true then "1" else "0" ) else value ) ) catch () )
						#pivotRO: ( try ( csexport.set_value "cryAnimDlgRolloutPivot" (if value == undefined then ( if (cryTools.cryAnim.UI.main._f.getUI "Pivot" "").open == true then "1" else "0" ) else value ) ) catch () )
						#modelsRO: ( try ( csexport.set_value "cryAnimDlgRolloutModels" (if value == undefined then ( if (cryTools.cryAnim.UI.main._f.getUI "Models" "").open == true then "1" else "0" ) else value ) ) catch () )
						#locatorRO: ( try ( csexport.set_value "cryAnimDlgRolloutLocator" (if value == undefined then ( if (cryTools.cryAnim.UI.main._f.getUI "Locator" "").open == true then "1" else "0" ) else value ) ) catch () )
						#poseRO: ( try ( csexport.set_value "cryAnimDlgRolloutPose" (if value == undefined then ( if (cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "").open == true then "1" else "0" ) else value ) ) catch () )
						#musclesRO: ( try ( csexport.set_value "cryAnimDlgRolloutMuscles" (if value == undefined then ( if (cryTools.cryAnim.UI.main._f.getUI "Muscles" "").open == true then "1" else "0" ) else value ) ) catch () )
						#loadSaveRO: ( try ( csexport.set_value "cryAnimDlgRolloutLoadSave" (if value == undefined then ( if (cryTools.cryAnim.UI.main._f.getUI "Load / Save / Export" "").open == true then "1" else "0" ) else value ) ) catch () )
						#perforceRO: ( try ( csexport.set_value "cryAnimDlgRolloutPerforce" (if value == undefined then ( if (cryTools.cryAnim.UI.main._f.getUI "Perforce" "").open == true then "1" else "0" ) else value ) ) catch () )
						#toolsRO: ( try ( csexport.set_value "cryAnimDlgRolloutTools" (if value == undefined then ( if (cryTools.cryAnim.UI.main._f.getUI "Settings" "").open == true then "1" else "0" ) else value ) ) catch () )
						
						#batchProcessExt: (	try ( csexport.set_value "cryAnimDlgBatchExtension" value ) catch() )
						#batchProcessSourcePath: (	try ( csexport.set_value "cryAnimDlgBatchOpenPath" value ) catch() )
						#batchProcessExportPath: (	try ( csexport.set_value "cryAnimDlgbatchProcessPath" value ) catch() )
						#batchProcessSubFolderSelection: (	try ( csexport.set_value "cryAnimDlgBatchSubFolderSelection" value ) catch() )
						#batchProcessSubFolders: (	try ( csexport.set_value "cryAnimDlgBatchSubFolders" (if value != undefined then (if value == true then "1" else "0") ) ) catch() )


						#models:
						(
							if value != undefined then
							(
								for i = 1 to 20 do
								(
									if value[i] != undefined then
									(
										tempString = value[i].name + "#" + value[i].path
										try ( csexport.set_value ("cryAnimDlgModel" + i as String) tempString ) catch()
									)
								
									else
										try ( csexport.set_value ("cryAnimDlgModel" + i as String) "" ) catch()
										
								)
							)
							else
								print "No value defined."
						)
						
						
						#items:
						(
							if value != undefined then
							(
								for i = 1 to 20 do
								(
									if value[i] != undefined then
									(
										local tempStringArray = (itemStruct name:"" external:"" model:"" reference:"" parent:"" rotation:"" position:"")
										local maxCount = value[i].model.count
										
										for d = 1 to maxCount do
										(
											tempStringArray.model += value[i].model[d] + (if i != maxCount then ";" else "")
											tempStringArray.reference += value[i].reference[d] as String + (if i != maxCount then ";" else "")
											tempStringArray.parent += value[i].parent[d] as String + (if i != maxCount then ";" else "")
											tempStringArray.rotation += value[i].rotation[d] as String + (if i != maxCount then ";" else "")
											tempStringArray.position += value[i].position[d] as String + (if i != maxCount then ";" else "")
										)
										
										tempString = value[i].name + "#" + value[i].external + "#" + tempStringArray.model + "#" + tempStringArray.reference + "#" + tempStringArray.parent + "#" + tempStringArray.rotation + "#" + tempStringArray.position
										try ( csexport.set_value ("cryAnimDlgWeapon" + i as String) tempString ) catch()
									)
									else
										try ( csexport.set_value ("cryAnimDlgWeapon" + i as String) "" ) catch()
								)
							)
							else
								print "No value defined."
						)


						#bones:
						(
							if value != undefined then
							(
								for i = 1 to 20 do
								(
									if value[i] != undefined then
									(
										tempString = value[i].active + "#" + value[i].name + "#" + value[i].external + "#" + value[i].bones
										try ( csexport.set_value ("cryAnimDlgBone" + i as String) tempString ) catch()
									)
									else
										try ( csexport.set_value ("cryAnimDlgBone" + i as String) "" ) catch()
								)
							)
							else
								print "No value defined."
						)
					)
				)
				#get:
				(
					case item of
					(
							--// Default Settings --
						#dialogPos: ( try ( execute ("return " + (csexport.get_value "cryAnimDlgPos" ) ) ) catch(return undefined) )
						#visible: ( try ( return (if (csexport.get_value "cryAnimDlgVisible") == "1" then true else false) ) catch() )
						#autoUpdate: ( try ( return (if (csexport.get_value "cryAnimDlgAutoUpdate") == "1" then true else false) ) catch() )
						#autoUpdateTime: ( try ( return ( (csexport.get_value "cryAnimDlgAutoUpdateTime") as Integer) ) catch() )
						#multiRow: ( try ( return (if (csexport.get_value "cryAnimDlgMultiRow") == "1" then true else false) ) catch() )
						#rolloutStates: ( try ( return (if (csexport.get_value "cryAnimDlgRolloutStates") == "1" then true else false) ) catch() )
						#readOnly: ( try ( return (if (csexport.get_value "cryAnimDlgReadOnly") == "1" then true else false) ) catch() )
						#workingDir: ( try ( return (csexport.get_value "cryAnimDlgWorkingDir") ) catch() )
						#loadBiped: ( try ( return (csexport.get_value "cryAnimDlgLoadBiped") ) catch() )


						
							--// Open For Edit --
						#loadOpen: ( try ( return (case (csexport.get_value "cryAnimDlgOFELoad") of ("1": 1; "2": 2; "3": 3);) ) catch() )
						#saveOpen: ( try ( return (case (csexport.get_value "cryAnimDlgOFESave") of ("1": 1; "2": 2; "3": 3);) ) catch() )
						#exportOpen: ( try ( return (case (csexport.get_value "cryAnimDlgOFEExport") of ("1": 1; "2": 2; "3": 3);) ) catch() )
						#saveExportOpen: ( try ( return (case (csexport.get_value "cryAnimDlgOFESaveExport") of ("1": 1; "2": 2; "3": 3);) ) catch() )
						
							--// Add To Source Control --
						#saveAdd: ( try ( return (case (csexport.get_value "cryAnimDlgASCSave") of ("1": 1; "2": 2; "3": 3);) ) catch() )
						#exportAdd: ( try ( return (case (csexport.get_value "cryAnimDlgASCExport") of ("1": 1; "2": 2; "3": 3);) ) catch() )
						#saveExportAdd: ( try ( return (case (csexport.get_value "cryAnimDlgASCSaveExport") of ("1": 1; "2": 2; "3": 3);) ) catch() )
						
							--// Prompt for File --
						#savePrompt: ( try ( return (case (csexport.get_value "cryAnimDlgPFFSave") of ("1": 1; "2": 2; "3": 3);) ) catch() )
						#exportPrompt: ( try ( return (case (csexport.get_value "cryAnimDlgPFFExport") of ("1": 1; "2": 2; "3": 3);) ) catch() )
						#saveExportPrompt: ( try ( return (case (csexport.get_value "cryAnimDlgPFFSaveExport") of ("1": 1; "2": 2; "3": 3);) ) catch() )

						
							--// Rollout Settings --
						#targetRO: ( try ( return (if (csexport.get_value "cryAnimDlgRolloutTarget") == "1" then true else false) ) catch() )
						#operationRO: ( try ( return (if (csexport.get_value "cryAnimDlgRolloutOperation") == "1" then true else false) ) catch() )
						#pivotRO: ( try ( return (if (csexport.get_value "cryAnimDlgRolloutPivot") == "1" then true else false) ) catch() )
						#modelsRO: ( try ( return (if (csexport.get_value "cryAnimDlgRolloutModels") == "1" then true else false) ) catch() )
						#locatorRO: ( try ( return (if (csexport.get_value "cryAnimDlgRolloutLocator") == "1" then true else false) ) catch() )
						#poseRO: ( try ( return (if (csexport.get_value "cryAnimDlgRolloutPose") == "1" then true else false) ) catch() )
						#musclesRO: ( try ( return (if (csexport.get_value "cryAnimDlgRolloutMuscles") == "1" then true else false) ) catch() )
						#loadSaveRO: ( try ( return (if (csexport.get_value "cryAnimDlgRolloutLoadSave") == "1" then true else false) ) catch() )
						#perforceRO: ( try ( return (if (csexport.get_value "cryAnimDlgRolloutPerforce") == "1" then true else false) ) catch() )
						#toolsRO: ( try ( return (if (csexport.get_value "cryAnimDlgRolloutTools") == "1" then true else false) ) catch() )
						
							--// Batch Process Settings --
						#batchProcessExt: (		try ( return (csexport.get_value "cryAnimDlgBatchExtension") ) catch() )
						#batchProcessSourcePath: (	try ( return (csexport.get_value "cryAnimDlgBatchOpenPath") ) catch() )
						#batchProcessExportPath: (	try ( return (csexport.get_value "cryAnimDlgbatchProcessPath") ) catch() )	
						#batchProcessSubFolderSelection: (	try ( return (csexport.get_value "cryAnimDlgBatchSubFolderSelection") ) catch() )	
						#batchProcessSubFolders: (	try ( return (if (csexport.get_value "cryAnimDlgBatchSubFolders") == "1" then true else false) ) catch() )
						
						#models:
						(
							tempArray = #()
							
							for i = 1 to 20 do
							(
								tempString = csexport.get_value ("cryAnimDlgModel" + i as String)
								if tempString != "" then
								(
									tempInt = findString tempString "#"
									tempFilter = filterString tempString "#"
									if tempInt == 1 then
									(
										tempFilter[2] = tempFilter[1]
										tempFilter[1] = ""
									)
									
									if tempIn == tempString.count then
									(
										tempFilter[2] = ""
									)

									append tempArray (modelPathStruct name:tempFilter[1] path:tempFilter[2])
								)
							)
							return tempArray
						)
						
						
						#items:
						(
							tempArray = #()				
							
							for i = 1 to 20 do
							(
								tempString = csexport.get_value ("cryAnimDlgWeapon" + i as String)
								if tempString != "" then
								(							
									tempFilter = filterString tempString "#"

									append tempArray (itemStruct name:(tempFilter[1] as String) external:(tempFilter[2] as String) model:(tempFilter[3] as String) reference:(tempFilter[4] as String) parent:(tempFilter[5] as String) rotation:(tempFilter[6] as String) position:(tempFilter[7] as String))
								)
							)
							return tempArray
						)
						
						
						#bones:
						(
							local tempArray = #()				
							
							for i = 1 to 20 do
							(
								local tempString = csexport.get_value ("cryAnimDlgBone" + i as String)
								if tempString != "" then
								(							
									local tempFilter = filterString tempString "#"

									append tempArray (boneStruct active:(tempFilter[1] as String) name:(tempFilter[2] as String) external:(tempFilter[3] as String) bones:(tempFilter[4] as String) )
								)
							)
							return tempArray
						)
					)
				)
			)
		)

		catch
		(
			case op of
			(
				#set:
				(
					case item of
					(
						-- Default Settings --
						#dialogPos:  (try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "cryAnimDlg" "pos" ( if value == undefined then (cryTools.cryAnim.UI.dialog.pos as String) else value) ) catch() )
						#visible: (try (setINISetting cryTools.cryAnim._v.animToolsINIPath "cryAnimDlg" "visible" value ) catch() )
						#autoUpdate: ( try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "cryAnimDlg" "autoUpdate" (if value == undefined then (if (cryTools.cryAnim.UI.main._f.getUI "Perforce" "chkAutoUpdate").checked == true then "1" else "0") else value ) ) catch() )
						#autoUpdateTime: (try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "cryAnimDlg" "autoUpdateTime" (if value == undefined then (if (cryTools.cryAnim.UI.main._f.getUI "Perforce" "spnAutoUpdateTime").value == true then "1" else "0")  else value ) ) catch() )
						#rolloutStates: (try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "cryAnimDlg" "rolloutStates" (if value == undefined then (if (cryTools.cryAnim.UI.main._f.getUI "Settings" "chkRolloutStates").checked == true then "1" else "0")  else value ) ) catch() )
						#readOnly: (try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "cryAnimDlg" "readOnly" (if value == undefined then ( if (cryTools.cryAnim.UI.main._f.getUI "Settings" "chkReadOnly").checked == true then "1" else "0" ) else value ) ) catch () )
						#workingDir: (try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "cryAnimDlg" "workingDir" (if value == undefined then ( (cryTools.cryAnim.UI.main._f.getUI "Settings" "edWorkingDir").text ) else value ) ) catch () )
						#loadBiped: (try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "cryAnimDlg" "loadBiped" (if value == undefined then cryTools.cryAnim.UI.main.loadSave._v.bipOpenPath else value ) ) catch () )



						-- Open For Edit --
						#loadOpen: (try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "OpenForEdit" "Load" (if value == undefined then (case (cryTools.cryAnim.UI.main._f.getUI "Perforce" "radLoadOpen").state of ( 1: "1"; 2: "2"; 3: "3" ); )  else value ) ) catch() )
						#saveOpen: (try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "OpenForEdit" "Save" (if value == undefined then (case (cryTools.cryAnim.UI.main._f.getUI "Perforce" "radSaveOpen").state of ( 1: "1"; 2: "2"; 3: "3" ); )  else value ) ) catch() )
						#exportOpen: (try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "OpenForEdit" "Export" (if value == undefined then (case (cryTools.cryAnim.UI.main._f.getUI "Perforce" "radExportOpen").state of ( 1: "1"; 2: "2"; 3: "3" ); )  else value ) ) catch() )
						#saveExportOpen: (try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "OpenForEdit" "SaveExport" (if value == undefined then (case (cryTools.cryAnim.UI.main._f.getUI "Perforce" "radSaveExportOpen").state of ( 1: "1"; 2: "2"; 3: "3" ); )  else value ) ) catch()  )

						-- Add To Source Control --
						#saveAdd: (try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "AddToSourceControl" "Save" (if value == undefined then (case (cryTools.cryAnim.UI.main._f.getUI "Perforce" "radSaveAdd").state of ( 1: "1"; 2: "2"; 3: "3" ); )  else value ) ) catch() )
						#exportAdd: (try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "AddToSourceControl" "Export" (if value == undefined then (case (cryTools.cryAnim.UI.main._f.getUI "Perforce" "radExportAdd").state of ( 1: "1"; 2: "2"; 3: "3" ); )  else value ) ) catch() )
						#saveExportAdd: (try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "AddToSourceControl" "SaveExport" (if value == undefined then (case (cryTools.cryAnim.UI.main._f.getUI "Perforce" "radSaveExportAdd").state of ( 1: "1"; 2: "2"; 3: "3" ); )  else value ) ) catch()  )
						
						-- Prompt for File --
						#savePrompt: (try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "PromptForFile" "Save" (if value == undefined then (case (cryTools.cryAnim.UI.main._f.getUI "Settings" "radSavePrompt").state of ( 1: "1"; 2: "2" ); )  else value ) ) catch() )
						#exportPrompt: (try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "PromptForFile" "Export" (if value == undefined then (case (cryTools.cryAnim.UI.main._f.getUI "Settings" "radExportPrompt").state of ( 1: "1"; 2: "2" ); )  else value ) ) catch() )
						#saveExportPrompt: (try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "PromptForFile" "SaveExport" (if value == undefined then (case (cryTools.cryAnim.UI.main._f.getUI "Settings" "radSaveExportPrompt").state of ( 1: "1"; 2: "2" ); )  else value ) ) catch()  )

						
						
						-- Rollout Settings --
						#target: (try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "Rollouts" "Target" (if value == undefined then ( if (cryTools.cryAnim.UI.main._f.getUI "Target" "").open == true then "1" else "0" ) else value ) ) catch () )
						#operation: (try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "Rollouts" "Operation" (if value == undefined then ( if (cryTools.cryAnim.UI.main._f.getUI "Operation" "").open == true then "1" else "0" ) else value ) ) catch () )
						#pivot: (try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "Rollouts" "Pivot" (if value == undefined then ( if (cryTools.cryAnim.UI.main._f.getUI "Pivot" "").open == true then "1" else "0" ) else value ) ) catch () )
						#base: (try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "Rollouts" "Misc" (if value == undefined then ( if (cryTools.cryAnim.UI.main._f.getUI "Misc" "").open == true then "1" else "0" ) else value ) ) catch () )
						#muscles: (try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "Rollouts" "Muscles" (if value == undefined then ( if (cryTools.cryAnim.UI.main._f.getUI "Muscles" "").open == true then "1" else "0" ) else value ) ) catch () )
						#loadSave: (try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "Rollouts" "LoadSave" (if value == undefined then ( if (cryTools.cryAnim.UI.main._f.getUI "Load / Save / Export" "").open == true then "1" else "0" ) else value ) ) catch () )
						#perforce: (try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "Rollouts" "Perforce" (if value == undefined then ( if (cryTools.cryAnim.UI.main._f.getUI "Perforce" "").open == true then "1" else "0" ) else value ) ) catch () )
						#tools: (try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "Rollouts" "Settings" (if value == undefined then ( if (cryTools.cryAnim.UI.main._f.getUI "Settings" "").open == true then "1" else "0" ) else value ) ) catch () )
						
						#batchProcessExt: (	try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "batchProcess" "Extension" value ) catch() )
						#batchProcessSourcePath: (	try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "batchProcess" "OpenPath" value ) catch() )
						#batchProcessExportPath: (	try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "batchProcess" "ExportPath" value ) catch() )
						#batchProcessSubFolderSelection: (	try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "batchProcess" "SubFolderSelection" value ) catch() )
						#batchProcessSubFolders: (	try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "batchProcess" "SubFolders" (if value != undefined then (if value == true then "1" else "0") ) ) catch() )


						#models:
						(
							if value != undefined then
							(
								for i = 1 to 20 do
								(
									if value[i] != undefined then
									(
										tempString = value[i].name + "#" + value[i].path
										try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "Models" ("path" + i as String) tempString ) catch()
									)
								
									else
										try ( delINISetting cryTools.cryAnim._v.animToolsINIPath "Models" ("path" + i as String) ) catch()
										
								)
							)
							else
								print "No value defined."
						)
						
						
						#items:
						(
							if value != undefined then
							(
								for i = 1 to 20 do
								(
									if value[i] != undefined then
									(
										local tempStringArray = (itemStruct name:"" external:"" model:"" reference:"" parent:"" rotation:"" position:"")
										local maxCount = value[i].model.count
										
										for d = 1 to maxCount do
										(
											tempStringArray.model += value[i].model[d] + (if i != maxCount then ";" else "")
											tempStringArray.reference += value[i].reference[d] as String + (if i != maxCount then ";" else "")
											tempStringArray.parent += value[i].parent[d] as String + (if i != maxCount then ";" else "")
											tempStringArray.rotation += value[i].rotation[d] as String + (if i != maxCount then ";" else "")
											tempStringArray.position += value[i].position[d] as String + (if i != maxCount then ";" else "")
										)
										
										tempString = value[i].name + "#" + value[i].external + "#" + tempStringArray.model + "#" + tempStringArray.reference + "#" + tempStringArray.parent + "#" + tempStringArray.rotation + "#" + tempStringArray.position
										try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "Weapons" ("wp" + i as String) tempString ) catch()
									)
									else
										try ( delINISetting cryTools.cryAnim._v.animToolsINIPath "Weapons" ("wp" + i as String) ) catch()
								)
							)
							else
								print "No value defined."
						)


						#bones:
						(
							if value != undefined then
							(
								for i = 1 to 20 do
								(
									if value[i] != undefined then
									(
										tempString = value[i].active + "#" + value[i].name + "#" + value[i].external + "#" + value[i].bones
										try ( setINISetting cryTools.cryAnim._v.animToolsINIPath "Bones" ("bone" + i as String) tempString ) catch()
									)
									else
										try ( delINISetting cryTools.cryAnim._v.animToolsINIPath "Bones" ("bone" + i as String) ) catch()
								)
							)
							else
								print "No value defined."
						)

					)
				)
				#get:
				(
					case item of
					(
						-- Default Settings --
						#dialogPos: ( try ( execute ("return " + (getINISetting cryTools.cryAnim._v.animToolsINIPath "cryAnimDlg" "pos" ) ) ) catch(return undefined) )
						#visible: (try ( return (if (getINISetting cryTools.cryAnim._v.animToolsINIPath "cryAnimDlg" "visible") == "1" then true else false) ) catch() )
						#autoUpdate: ( try ( return (if (getINISetting cryTools.cryAnim._v.animToolsINIPath "cryAnimDlg" "autoUpdate") == "1" then true else false) ) catch() )
						#autoUpdateTime: ( try ( return (getINISetting cryTools.cryAnim._v.animToolsINIPath "cryAnimDlg" "spnAutoUpdateTime") ) catch() )
						#rolloutStates: ( try ( return (if (getINISetting cryTools.cryAnim._v.animToolsINIPath "cryAnimDlg" "rolloutStates") == "1" then true else false) ) catch() )
						#readOnly: ( try ( return (if (getINISetting cryTools.cryAnim._v.animToolsINIPath "cryAnimDlg" "readOnly") == "1" then true else false) ) catch() )
						#workingDir: ( try ( return (getINISetting cryTools.cryAnim._v.animToolsINIPath "cryAnimDlg" "workingDir") ) catch() )
						#loadBiped: ( try ( return (getINISetting cryTools.cryAnim._v.animToolsINIPath "cryAnimDlg" "loadBiped") ) catch() )


						
						-- Open For Edit --
						#loadOpen: (try ( return (case (getINISetting cryTools.cryAnim._v.animToolsINIPath "OpenForEdit" "Load") of ("1": 1; "2": 2; "3": 3);) ) catch() )
						#saveOpen: (try ( return (case (getINISetting cryTools.cryAnim._v.animToolsINIPath "OpenForEdit" "Save") of ("1": 1; "2": 2; "3": 3);) ) catch() )
						#exportOpen: (try ( return (case (getINISetting cryTools.cryAnim._v.animToolsINIPath "OpenForEdit" "Export") of ("1": 1; "2": 2; "3": 3);) ) catch() )
						#saveExportOpen: ( try ( return (case (getINISetting cryTools.cryAnim._v.animToolsINIPath "OpenForEdit" "SaveExport") of ("1": 1; "2": 2; "3": 3);) ) catch() )
						
						-- Add To Source Control --
						#saveAdd: (try ( return (case (getINISetting cryTools.cryAnim._v.animToolsINIPath "AddToSourceControl" "Save") of ("1": 1; "2": 2; "3": 3);) ) catch() )
						#exportAdd: (try ( return (case (getINISetting cryTools.cryAnim._v.animToolsINIPath "AddToSourceControl" "Export") of ("1": 1; "2": 2; "3": 3);) ) catch() )
						#saveExportAdd: (try ( return (case (getINISetting cryTools.cryAnim._v.animToolsINIPath "AddToSourceControl" "SaveExport") of ("1": 1; "2": 2; "3": 3);) ) catch() )
						
						-- Prompt for File --
						#savePrompt: (try ( return (case (getINISetting cryTools.cryAnim._v.animToolsINIPath "PromptForFile" "Save") of ("1": 1; "2": 2; "3": 3);) ) catch() )
						#exportPrompt: (try ( return (case (getINISetting cryTools.cryAnim._v.animToolsINIPath "PromptForFile" "Export") of ("1": 1; "2": 2; "3": 3);) ) catch() )
						#saveExportPrompt: (try ( return (case (getINISetting cryTools.cryAnim._v.animToolsINIPath "PromptForFile" "SaveExport") of ("1": 1; "2": 2; "3": 3);) ) catch() )

						
						-- Rollout Settings --
						#target: (try ( return (if (getINISetting cryTools.cryAnim._v.animToolsINIPath "Rollouts" "Target") == "1" then true else false) ) catch() )
						#operation: ( try ( return (if (getINISetting cryTools.cryAnim._v.animToolsINIPath "Rollouts" "Operation") == "1" then true else false) ) catch() )
						#pivot: (try ( return (if (getINISetting cryTools.cryAnim._v.animToolsINIPath "Rollouts" "Pivot") == "1" then true else false) ) catch() )
						#base: ( try ( return (if (getINISetting cryTools.cryAnim._v.animToolsINIPath "Rollouts" "Misc") == "1" then true else false) ) catch() )
						#muscles: ( try ( return (if (getINISetting cryTools.cryAnim._v.animToolsINIPath "Rollouts" "Muscles") == "1" then true else false) ) catch() )
						#loadSave: ( try ( return (if (getINISetting cryTools.cryAnim._v.animToolsINIPath "Rollouts" "LoadSave") == "1" then true else false) ) catch() )
						#perforce: ( try ( return (if (getINISetting cryTools.cryAnim._v.animToolsINIPath "Rollouts" "Perforce") == "1" then true else false) ) catch() )
						#tools: ( try ( return (if (getINISetting cryTools.cryAnim._v.animToolsINIPath "Rollouts" "Settings") == "1" then true else false) ) catch() )
						
						#batchProcessExt: (		try ( return (getINISetting cryTools.cryAnim._v.animToolsINIPath "batchProcess" "Extension") ) catch() )
						#batchProcessSourcePath: (	try ( return (getINISetting cryTools.cryAnim._v.animToolsINIPath "batchProcess" "OpenPath") ) catch() )
						#batchProcessExportPath: (	try ( return (getINISetting cryTools.cryAnim._v.animToolsINIPath "batchProcess" "ExportPath") ) catch() )	
						#batchProcessSubFolderSelection: (	try ( return (getINISetting cryTools.cryAnim._v.animToolsINIPath "batchProcess" "SubFolderSelection") ) catch() )	
						#batchProcessSubFolders: (	try ( return (if (getINISetting cryTools.cryAnim._v.animToolsINIPath "batchProcess" "SubFolders") == "1" then true else false) ) catch() )
						
						#models:
						(
							tempArray = #()
							
							for i = 1 to 20 do
							(
								tempString = getINISetting cryTools.cryAnim._v.animToolsINIPath "Models" ("path" + i as String)
								if tempString != "" then
								(
									tempInt = findString tempString "#"
									tempFilter = filterString tempString "#"
									if tempInt == 1 then
									(
										tempFilter[2] = tempFilter[1]
										tempFilter[1] = ""
									)
									
									if tempIn == tempString.count then
									(
										tempFilter[2] = ""
									)

									append tempArray (modelPathStruct name:tempFilter[1] path:tempFilter[2])
								)
							)
							return tempArray
						)
						
						
						#items:
						(
							tempArray = #()				
							
							for i = 1 to 20 do
							(
								tempString = getINISetting cryTools.cryAnim._v.animToolsINIPath "Weapons" ("wp" + i as String)
								if tempString != "" then
								(							
									tempFilter = filterString tempString "#"

									append tempArray (itemStruct name:(tempFilter[1] as String) external:(tempFilter[2] as String) model:(tempFilter[3] as String) reference:(tempFilter[4] as String) parent:(tempFilter[5] as String) rotation:(tempFilter[6] as String) position:(tempFilter[7] as String))
								)
							)
							return tempArray
						)
						
						
						#bones:
						(
							tempArray = #()				
							
							for i = 1 to 20 do
							(
								tempString = getINISetting cryTools.cryAnim._v.animToolsINIPath "Bones" ("bone" + i as String)
								if tempString != "" then
								(							
									tempFilter = filterString tempString "#"

									append tempArray (boneStruct active:(tempFilter[1] as String) name:(tempFilter[2] as String) external:(tempFilter[3] as String) bones:(tempFilter[4] as String) )
								)
							)
							return tempArray
						)
					)
				)
			)
		)
	)
	catch ( logOutput "!!> Error in cryAnim.base.iniFile" )
)
iniFile = undefined
logOutput "> Created cryAnim.base.iniFile function"












--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.base.getPerforcePath
----------------------------------------------------------------------------------------------------------------------------------
--			checks, if the dialog is visible or not
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.base.getPerforcePath = function getPerforcePath =
(
	try
	(
		local perforceString = ""
		local buildFilter = filterString cryTools.buildPathFull "\\"
		for i = 1 to buildFilter.count do
			perforceString += buildFilter[i] + (if i < buildFilter.count then "\\" else "")
							
		local perforceString = cryTools.cryAnim.base.perforce perforceString #getPath
		
		return perforceString
	)
	catch ( logOutput "!!> Error in cryAnim.base.getPerforcePath" )
)
getPerforcePath = undefined
logOutput "> Created cryAnim.base.getPerforcePath function"











--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.base.setMacros
----------------------------------------------------------------------------------------------------------------------------------
--			creates macros and menu/quad menu entries
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.base.setMacros = function setMacros op =
(
	try
	(
		case op of
		(
			#delete:
			(
				local animToolsMacros = getFiles (getDir #UI + "MacroScripts\\CryAnim*.mcr")
		
				for i = 1 to animToolsMacros.count do				
					deleteFile animToolsMacros[i]
		
				menuMan.updateMenubar()				
			)
			
			#update:
			(			
				ATFilesArray = # \
				( \
					"ATAnimToolsDlg", \
					"ATBipToZero2", \
					"ATBipToZero3", \
					"ATCreateSingleSnapshot", \
					"ATCreateChildrenSnapshot", \
					"ATHideAllTrajectories", \
					"ATMoveRot", \
					"ATMoveToFloor", \
					"ATMoveToPivPoint", \
					"ATMoveRotToPivPoint", \
					"ATMoveToSnapshot", \
					"ATPivotSelect", \
					"ATResetRotation", \
					"ATShowTrajectory", \
					"ATLockRotation", \
					"ATUnlockRotation", \
					"ATCopyTransform", \
					"ATPasteTransform", \
					"ATPastePosition", \
					"ATPasteRotation" \
				)
					
				localMacros = getFiles ((getDir #UI) + "\\MacroScripts\\CryAnim*")
				for i = 1 to ATFilesArray.count do
				(					
					for f = 1 to 40 do
					(
						if localMacros.count == 0 then
							exit
						else
						(							
							try
							(
								if (findString localMacros[f] ATFilesArray[i]) != undefined then
									deleteItem localMacros f
							)
							catch()								
						)
					)
				)
				
				if localMacros.count != 0 then
				(
					for i = 1 to localMacros.count do
					(
						deleteFile localMacros[i]
						print ("Deleted unused MacroScript:  " + localMacros[i])
					)
				)
			)
		)
		
			
		

		macroScript ATCreateSingleSnapshot category:"CryAnim" toolTip:"create single snapshot"
		(
			on isVisible do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						if selection.count > 0 then
							return true
						else
							return false
					)
				)catch( return false )
			)
			
			on execute do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						with animate off
							cryTools.cryAnim._f.createSnapshot()
					)
				)catch()
			)
		)	
			
			
			
			
		macroScript ATCreateChildrenSnapshot category:"CryAnim" toolTip:"create children snapshot"
		(
			on isVisible do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						if selection.count > 0 then
							return true
						else
							return false
					)
				)catch( return false )
			)
			
			on execute do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						with animate off
						(
							if $selection.count > 0 then
							(
								local baseArray = cryTools.cryAnim._f.createSnapshot children:true
								if baseArray != undefined then
									group baseArray prefix:"Snapshot_"
							)
							else
								print "Nothing selected"
						)
					)
				)catch()
			)
		)
					
					
		macroScript ATPivotSelect category:"CryAnim" toolTip:"select pivot"
		(					
			on isVisible do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()
						if selectedBipPart != undefined then
							return true
						else
							return false
					)
				)catch( return false )
			)
			on isEnabled do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()
						if selectedBipPart != undefined then
							if selectedBipPart.object.isPivot == true then
								return true
							else
								return false
					)
				)catch( return false )
			)
			on execute do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						undo off
							cryTools.cryAnim.align._f.callPivotSelect()
					)
				)catch()
			)
		)
					
						
		macroScript ATMoveToSnapshot category:"CryAnim" toolTip:"move to snapshot"
		(				
			on isVisible  do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()
						if selectedBipPart != undefined then
							return true					
					)
					return false
				)catch( return false )
			)
			on isEnabled do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						nearestSnap = cryTools.cryAnim._f.getNearestSnapshot()
						if nearestSnap != undefined then
							return true
						else
							return false
					)
				)catch( return false )
			)
			on execute do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(					
						undo "move to snap" on
							cryTools.cryAnim._f.moveBipPart()
					)
				)catch()
			)
		)
					
					
					
		macroScript ATMoveToPivPoint category:"CryAnim" toolTip:"move to pivot point"
		(				
			on isVisible  do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()
						if selectedBipPart != undefined then
						(
							if selectedBipPart.pivotSel.pivPoint.name != undefined then
								return true
							else
								return false
						)
					)
					return false
				)catch( return false )
			)
			
			on isEnabled do
			(
				try
				(
					selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()
					if selectedBipPart != undefined then
						if selectedBipPart.object.isPivot == true then
							return true
							
					return false
				)catch( return false )
			)
			
			on execute do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(							
						undo "move to pivot point" on
							cryTools.cryAnim._f.moveBipPartPivPoint()							
					)
				)catch()
			)
		)
		
		
		
		
		macroScript ATMoveRotToPivPoint category:"CryAnim" toolTip:"move/rotate to pivot point"
		(	
			on isVisible  do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()
						if selectedBipPart != undefined then
						(
							if selectedBipPart.pivotSel.pivPoint.name != undefined then
								return true
							else
								return false
						)
					)
					return false
				)catch( return false )
			)
			on isEnabled do
			(
				try
				(
					selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()
					if selectedBipPart != undefined then
						if selectedBipPart.object.isPivot == true then
							return true
							
					return false
				)catch( return false )
			)
			on execute do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						undo "move/rot to pivot point" on
							cryTools.cryAnim._f.moveRotBipPartPivPoint()
					)
				)catch()
			)
		)
		
		
			
					
		macroScript ATMoveRot category:"CryAnim" toolTip:"move/rotate to snapshot"
		(	
			on isVisible  do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()
						if selectedBipPart != undefined then
							return true
						else
							return false
					)
				)catch( return false )
			)
			on isEnabled do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						nearestSnap = cryTools.cryAnim._f.getNearestSnapshot()
						if nearestSnap != undefined then
							return true
						else
							return false
					)
				)catch( return false )
			)
			on execute do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						undo "move/rot to snap" on
							cryTools.cryAnim._f.moveRotBipPart()
					)
				)catch()
			)
		)
					
					
		macroScript ATMoveToFloor category:"CryAnim" toolTip:"move to floor"
		(	
			on isVisible  do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()
						if selectedBipPart != undefined then
							return true
						else
							return false
					)
				)catch( return false )
			)
			on isEnabled do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()
						if selectedBipPart != undefined then
							if (selectedBipPart.part.bodyPart == "Foot") and (selectedBipPart.object.child == undefined)then
								return true
							else
								return false
					)
					else
						print ("No aTools Structure loaded...")
				)catch( return false )
			)
			on execute do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						undo "move to floor" on
							cryTools.cryAnim._f.moveToFloor()
					)
				)catch()
			)
		)
					
					
		macroScript ATResetRotation category:"CryAnim" toolTip:"reset rotation"
		(	
			on isVisible  do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()
						if selectedBipPart != undefined then
							return true
						else
							return false
					)
				)catch( return false )
			)
			on execute do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						undo "reset rotation" on
							cryTools.cryAnim._f.resetRotation()
					)
				)catch()
			)
		)
					
					
		macroScript ATCryAnimDlg category:"CryAnim" toolTip:"cryAnim dialog" icon:#("Reactor",5)
		(
			try
			(
				if cryTools.cryAnim != undefined then
				(
					undo off
						cryTools.cryAnim.UI.main._f.callDialog()
				)
			)catch()
		)
			
					
		macroScript ATBipToZero2 category:"CryAnim" toolTip:"bip to [0,0]"
		(
			on isVisible do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						if $Bip01 != undefined then
							return true
						else
							return false
					)
				)catch( return false )
			)
			on execute do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						cryTools.cryAnim._f.bipToZero2()
					)
				)catch()
			)
		)
		
						
		macroScript ATBipToZero3 category:"CryAnim" toolTip:"bip to [0,0,0]"
		(	
			on isVisible do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						if $Bip01 != undefined then
							return true
						else
							return false
					)
				)catch( return false )
			)
			on execute do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						cryTools.cryAnim._f.bipToZero3()
					)
				)catch()
			)
		)
		
				
		macroScript ATShowTrajectory category:"CryAnim" toolTip:"show trajectory"
		(	
			on isVisible do
			(
				try
				(
					if (cryTools.cryAnim != undefined) and ((maxVersion())[1] < 9000) then
					(				
						for obj in $selection do
							if getTrajectoryOn obj == false then
								return true
						
						return false
					)
				)catch( return false )
			)
			on execute do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						cryTools.cryAnim._f.setTrajectories #show
					)
				)catch()
			)
		)


		macroScript ATHideAllTrajectories category:"CryAnim" toolTip:"hide all trajectories"
		(	
			on isVisible do
			(
				try
				(
					if (cryTools.cryAnim != undefined) and ((maxVersion())[1] < 9000) then
					(
						return cryTools.cryAnim._f.setTrajectories #checkHide		
					)
				)catch( return false )
			)
			on execute do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						cryTools.cryAnim._f.setTrajectories #hide
					)
				)catch()
			)
		)
		
			
		macroScript ATLockRotation category:"CryAnim" toolTip:"lock rotation"
		(	
			on isVisible do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						local selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()					
						if selectedBipPart != undefined then
						(
							if cryTools.cryAnim._v.lockRotation == undefined then
							(
								/*
								
								*/
							
								return true
							)
						)
						
						return false
					)
				)catch( return false )
			)
			on execute do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()					
						if selectedBipPart != undefined then
						(
							cryTools.cryAnim._f.lockRotation()
							
							/*
							when transform $selection[1] change id:#lockRotation do
							(
								if cryTools != undefined then
								(
									if cryTools.cryAnim._v.lockRotation != undefined then
									(
										local selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()
										if selectedBipPart != undefined then
											if selectedBipPart.part.bodyPart == "Hand" then
												biped.setTransform $selection[1] #rotation cryTools.cryAnim._v.lockRotation true
									)
								)
							)
							*/
						)
					)
				)catch()
			)
		)
		
		
		macroScript ATUnlockRotation category:"CryAnim" toolTip:"unlock rotation"
		(	
			on isVisible do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()					
						if selectedBipPart != undefined then
						(
							if cryTools.cryAnim._v.lockRotation != undefined then						
								return true
						)
						
						return false
					)
				)catch( return false )
			)
			on execute do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()					
						if selectedBipPart != undefined then
						(
							cryTools.cryAnim._v.lockRotation = undefined
						)					
					)
				)catch()
			)
		)
		
		
		macroScript ATCopyTransform category:"CryAnim" toolTip:"copy transform"
		(	
			on isVisible do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						if $selection.count > 0 then
							return true
					)
					return false
				)catch( return false )
			)
			on execute do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						try deleteFile cryTools.cryAnim.UI.main._v.tempINIPath catch()
						for d = 1 to $selection.count do
						(								
							setINISetting cryTools.cryAnim.UI.main._v.tempINIPath ("Object" + d as String) "1" ($selection[d].transform as String)
						)
					)
				)catch()
			)
		)


		macroScript ATPasteTransform category:"CryAnim" toolTip:"paste transform"
		(	
			on isVisible do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(				
						if $selection.count > 0 then
							return true
					)
					return false
				)catch( return false )
			)
			on isEnabled do
			(
				try
				(
					if (getFiles cryTools.cryAnim.UI.main._v.tempINIPath).count > 0 then
						return true
					else
						return false
				)catch( return false )
			)
			on execute do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						if (getFiles cryTools.cryAnim.UI.main._v.tempINIPath).count > 0 then
						(
							if $selection.count > 0 then
							(
								for d = 1 to $selection.count do
								(
									local newTran = getINISetting cryTools.cryAnim.UI.main._v.tempINIPath ("Object" + d as String) "1"
									if newTran != "" then
									(
										newTran = execute ( newTran )
										cryTools.cryAnim._f.pasteTransform $selection[d] newTran #transform
									)
								)
							)
						)			
					)
				)catch()
			)
		)
		
		
		macroScript ATPastePosition category:"CryAnim" toolTip:"paste position"
		(	
			on isVisible do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(				
						if $selection.count > 0 then
							return true
					)
					return false	
				)catch( return false )
			)
			on isEnabled do
			(
				try
				(
					if (getFiles cryTools.cryAnim.UI.main._v.tempINIPath).count > 0 then
						return true
					else
						return false
				)catch( return false )
			)
			on execute do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						if (getFiles cryTools.cryAnim.UI.main._v.tempINIPath).count > 0 then
						(
							if $selection.count > 0 then
							(
								for d = 1 to $selection.count do
								(
									local newTran = getINISetting cryTools.cryAnim.UI.main._v.tempINIPath ("Object" + d as String) "1"
									if newTran != "" then
									(
										newTran = execute ( newTran )
										cryTools.cryAnim._f.pasteTransform $selection[d] newTran #pos
									)
								)
							)
						)			
					)	
				)catch()
			)
		)
		
		
		macroScript ATPasteRotation category:"CryAnim" toolTip:"paste rotation"
		(	
			on isVisible do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(				
						if $selection.count > 0 then
							return true
					)
					return false
				)catch( return false )
			)
			on isEnabled do
			(
				try
				(
					if (getFiles cryTools.cryAnim.UI.main._v.tempINIPath).count > 0 then
						return true
					else
						return false
				)catch( return false )
			)
			on execute do
			(
				try
				(
					if cryTools.cryAnim != undefined then
					(
						if (getFiles cryTools.cryAnim.UI.main._v.tempINIPath).count > 0 then
						(
							if $selection.count > 0 then
							(
								for d = 1 to $selection.count do
								(
									local newTran = getINISetting cryTools.cryAnim.UI.main._v.tempINIPath ("Object" + d as String) "1"
									if newTran != "" then
									(
										newTran = execute ( newTran )
										cryTools.cryAnim._f.pasteTransform $selection[d] newTran #rotation
									)
								)
							)
						)			
					)
				)catch(print "FUCK!!")
			)
		)
	)
	catch ( logOutput "!!> Error in cryAnim.base.setMacros" )
)
setMacros = undefined
logOutput "> Created cryAnim.base.setMacros function"










--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.base.setMenu
----------------------------------------------------------------------------------------------------------------------------------
--			creates menu for CryAnim
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.base.setMenu = function setMenu =
(
	try
	(
		local animToolsName = "CryAnim"

		tempInterface = menuMan.findMenu "Context CryAnim"
		if tempInterface != undefined then
			menuMan.unRegisterMenu tempInterface
		
		
		-- Menu
		local ATm = menuMan.createMenu "Context CryAnim"
		-- Seperator
		local ATSep = menuMan.createSeparatorItem()
		-- List of menUItems
		local ATmItem
			
		-- adding menUItems and set names
		ATmItem = menuMan.createActionItem "ATBipToZero2" animToolsName; ATmItem.setTitle "bip to [0,0]"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
		ATmItem = menuMan.createActionItem "ATBipToZero3" animToolsName; ATmItem.setTitle "bip to [0,0,0]"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
		--
		ATmItem = menuMan.createSeparatorItem(); ATm.addItem ATmItem -1
		--
		ATmItem = menuMan.createActionItem "ATShowTrajectory" animToolsName; ATmItem.setTitle "show trajectory"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
		ATmItem = menuMan.createActionItem "ATHideAllTrajectories" animToolsName; ATmItem.setTitle "hide all trajectories"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
		--
		ATmItem = menuMan.createSeparatorItem(); ATm.addItem ATmItem -1
		--
		ATmItem = menuMan.createActionItem "ATMoveToPivPoint" animToolsName; ATmItem.setTitle "move to pivot point"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
		ATmItem = menuMan.createActionItem "ATMoveRotToPivPoint" animToolsName; ATmItem.setTitle "move/rotate to pivot point"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
		ATmItem = menuMan.createActionItem "ATMoveToSnapshot" animToolsName; ATmItem.setTitle "move to snapshot"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
		ATmItem = menuMan.createActionItem "ATMoveRot" animToolsName; ATmItem.setTitle "move/rotate to snapshot"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
		--
		ATmItem = menuMan.createSeparatorItem(); ATm.addItem ATmItem -1
		--
		ATmItem = menuMan.createActionItem "ATLockRotation" animToolsName; ATmItem.setTitle "lock rotation"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
		ATmItem = menuMan.createActionItem "ATUnlockRotation" animToolsName; ATmItem.setTitle "unlock rotation"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
		--
		ATmItem = menuMan.createSeparatorItem(); ATm.addItem ATmItem -1
		--
		ATmItem = menuMan.createActionItem "ATResetRotation" animToolsName; ATmItem.setTitle "reset rotation"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
		ATmItem = menuMan.createActionItem "ATMoveToFloor" animToolsName; ATmItem.setTitle "move to floor"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
		--
		ATmItem = menuMan.createSeparatorItem(); ATm.addItem ATmItem -1
		--
		ATmItemSubMenu = menuMan.createMenu "create snapshot"
		ATmItemSubMenUItem1 = menuMan.createActionItem "ATCreateSingleSnapshot" animToolsName ; ATmItemSubMenUItem1.setTitle "single" ; ATmItemSubMenUItem1.setUseCustomTitle true; ATmItemSubMenu.addItem ATmItemSubMenUItem1 -1
		ATmItemSubMenUItem2 = menuMan.createActionItem "ATCreateChildrenSnapshot" animToolsName ; ATmItemSubMenUItem2.setTitle "children" ; ATmItemSubMenUItem2.setUseCustomTitle true; ATmItemSubMenu.addItem ATmItemSubMenUItem2 -1
		--
		ATmItem = menuMan.createSubMenUItem "create snapshot" ATmItemSubMenu ; ATmItem.setTitle "create snapshot"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
		--
		ATmItem = menuMan.createSeparatorItem(); ATm.addItem ATmItem -1
		--
		ATmItem = menuMan.createActionItem "ATCopyTransform" animToolsName; ATmItem.setTitle "copy"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
		--
		ATmItem = menuMan.createSeparatorItem(); ATm.addItem ATmItem -1
		--
		ATmItemSubMenu = menuMan.createMenu "paste"
		ATmItemSubMenUItem1 = menuMan.createActionItem "ATPasteTransform" animToolsName ; ATmItemSubMenUItem1.setTitle "transform" ; ATmItemSubMenUItem1.setUseCustomTitle true; ATmItemSubMenu.addItem ATmItemSubMenUItem1 -1
		ATmItemSubMenUItem2 = menuMan.createActionItem "ATPastePosition" animToolsName ; ATmItemSubMenUItem2.setTitle "position" ; ATmItemSubMenUItem2.setUseCustomTitle true; ATmItemSubMenu.addItem ATmItemSubMenUItem2 -1
		ATmItemSubMenUItem3 = menuMan.createActionItem "ATPasteRotation" animToolsName ; ATmItemSubMenUItem3.setTitle "rotation" ; ATmItemSubMenUItem3.setUseCustomTitle true; ATmItemSubMenu.addItem ATmItemSubMenUItem3 -1
		--
		ATmItem = menuMan.createSubMenUItem "paste" ATmItemSubMenu ; ATmItem.setTitle "paste"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
		--
		ATmItem = menuMan.createSeparatorItem(); ATm.addItem ATmItem -1
		--					
		ATmItem = menuMan.createActionItem "ATPivotSelect" animToolsName; ATmItem.setTitle "select pivot"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
			
		-- QuadMenu
		local ATqM = menuMan.getQuadMenu 1
		-- lower left corner of QuadMenu
		local ATqMCorner = ATqM.getMenu 4
			-- generates subMenu to put in the Menu
		local ATqMSub = menuMan.createSubMenUItem "Context CryAnim" ATm
		-- sets the subMenu to be flat
		ATqMSub.setDisplayFlat true
		-- adding subMenu to QuadMenuCorner
		ATqMCorner.addItem  ATqMSub -1
	
			--// initialize when struct --
		deleteAllChangeHandlers id:#lockRotation
	)
	catch ( logOutput "!!> Error in cryAnim.base.setMenu" )
)
setMenu = undefined
logOutput "> Created cryAnim.base.setMenu function"









--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.base.setCallbacks
----------------------------------------------------------------------------------------------------------------------------------
--			creates callbacks for cryAnim
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.base.setCallbacks = function setCallbacks =
(
	try
	(
		with undo off
		(
			callbacks.removeScripts id:#checkDialogVisible
			callbacks.addScript #preSystemShutdown "cryTools.cryAnim.base.killCryAnim()" id:#checkDialogVisible
		)
		return true
	)
	catch ( logOutput "!!> Error in cryAnim.base.setCallbacks" )
)
setCallbacks = undefined
logOutput "> Created cryAnim.base.setCallbacks function"













--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.base.initVars
----------------------------------------------------------------------------------------------------------------------------------
--			initializes variables
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.base.setVars = function setVars =
(
	try
	(	
		cryTools.cryAnim.UI.main._v.dialogMaxHeight = ((sysInfo.desktopSize)[2] - 135)
		cryTools.cryAnim.UI.main._v.tempINIPath = (cryTools.cryTemp + "cryAnim_temp.ini")
		cryTools.cryAnim.UI.main._v.animToolsINIPath = (cryTools.cryTemp + "cryAnim.ini")
		
		cryTools.cryAnim._v.tempATPath = (cryTools.cryTemp + "cryAnim_perforce.at")
		cryTools.cryAnim._v.perforceDir = cryTools.cryAnim.base.getPerforcePath()
		cryTools.cryAnim._v.various[35] = #(rotPosStruct())	
		
		cryTools.cryAnim.align._v.pivotLast.LFoot.pivObject = "Bip01 L Foot"
		cryTools.cryAnim.align._v.pivotLast.RFoot.pivObject = "Bip01 R Foot"
		cryTools.cryAnim.align._v.pivotLast.LHand.pivObject = "Bip01 L Hand"
		cryTools.cryAnim.align._v.pivotLast.RHand.pivObject = "Bip01 R Hand"

		cryTools.cryAnim.align._v.pivotLast.LHand.pivotName = "p6_(Bip01 L Hand)"
		cryTools.cryAnim.align._v.pivotLast.RHand.pivotName = "p6_(Bip01 R Hand)"
		cryTools.cryAnim.align._v.pivotLast.LFoot.pivotName = "p6_(Bip01 L Foot)"
		cryTools.cryAnim.align._v.pivotLast.RFoot.pivotName = "p6_(Bip01 R Foot)"
		
		return true
	)
	catch ( logOutput "!!> Error in cryAnim.base.initVars" )
)
setVars = undefined
logOutput "> Created cryAnim.base.initVars function"

















--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.base.killCryAnim
----------------------------------------------------------------------------------------------------------------------------------
--			kills the dialog if one is opened and cleans all used variables
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.base.killCryAnim = function killCryAnim unloadCryAnim:undefined =
(
	try
	(
		cryTools.cryAnim._v.various[10] = true
		
		try destroyDialog cryTools.cryAnim.UI.main.dialog catch()
		try destroyDialog cryTools.cryAnim.UI.main.models.editModelList catch()
		try destroyDialog cryTools.cryAnim.UI.main.models.editItemList catch()
		try destroyDialog cryTools.cryAnim.UI.batchProcess.customizeScript catch()
		try destroyDialog cryTools.cryAnim.UI.batchProcess.subRangeDialog catch()
		
		try closeRolloutFloater cryTools.cryAnim.UI.batchProcess.dialog catch()
		try destroyDialog cryTools.cryAnim.UI.batchProcess.editBoneList catch()
		
		
		
		if unloadCryAnim == true then
		(
			cryTools.cryAnim = undefined
			gc light:true
		)
		
		
		return true
	)
	catch ( logOutput "!!> Error in cryAnim.base.killCryAnim" )
)
killCryAnim = undefined
logOutput "> Created cryAnim.base.killCryAnim function"










--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.base.callStartUp
----------------------------------------------------------------------------------------------------------------------------------
--			sets up CryAnim
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.base.callStartUp = function callStartUp =
(
	try
	(
			--// deleting pivPoint if available
		for obj in objects do
			if ((findString obj.name "pivPoint") != undefined) or ((findString obj.name "pivSnap") != undefined) then
				delete obj
		
		
		cryTools.cryAnim.base.setVars()
		cryTools.cryAnim.base.setMacros #update
		cryTools.cryAnim.base.setMenu()
		cryTools.cryAnim.base.setCallbacks()
		
		
		
		
		if cryTools.cryAnim._v.perforceDir != undefined then
		(
			if (cryTools.cryAnim.base.iniFile #get #autoUpdate) == true then
			(
				if (local tempVar = cryTools.cryAnim.base.perforce "" #checkLoading) != false then
					cryTools.cryAnim.base.perforce tempVar #messageOnLoading
			)
		)
		
		
		
		
		local finishedLoading = cryTools.cryAnim.base.loadFiles #all
		
		
		if (cryTools.cryAnim.base.iniFile #get #visible) == true then
		(
			try crysplash.build.text = "LOADING DIALOG" catch()
			cryTools.cryAnim.UI.main._f.callDialog()
		)
		
		
		local tempLoadString = "Loaded"
		
		if reloadCryAnim == true then
			tempLoadString = "Reloaded"
		
		reloadCryAnim = undefined
		
		
		trackTime = ((timeStamp() - trackTime) / 1000.0)
		
		if finishedLoading == true then
			print (tempLoadString + " CryAnim in " +  trackTime as String + " seconds.\n")
		
		
		try destroyDialog crySplash catch()
		
	)
	catch ( logOutput "!!> Error in cryAnim.base.callStartUp" )
)
callStartUp = undefined
logOutput "> Created cryAnim.base.callStartUp function"














--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.base.reloadScript
----------------------------------------------------------------------------------------------------------------------------------
--			reloads the full script
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.base.reloadScript = function reloadScript =
(
	try
	(
		cryTools.cryAnim.base.loadFiles #load
	)
	catch ( logOutput "!!> Error in cryAnim.base.reloadScript" )
)
reloadScript = undefined
logOutput "> Created cryAnim.base.reloadScript function"




	--// creates aTools
cryTools.cryAnim.base.callStartUp()

logOutput ">> load.ms loaded"

trackTime = undefined