-------------------------------------------------------------------------------
-- batch8.ms   #for 3DSMax v.8
-- Version 2.4 Internal
-- Batch Exporter for .bip and .fbx with upper body detection for upper body animations used in cryEngine
-- By: Mathias Lindner
--   eMail: devsupport@crytek.com
-------------------------------------------------------------------------------





--###############################################################################
	--// creates the dialog to customize the scripts
--###############################################################################
cryTools.cryAnim.UI.batchProcess._f.createScriptDialog = function createScriptDialog dialogTitle =
(
	try
	(
			--// rollout with edit option for the script which will be execute before exporting or at the end of check
		rollout batchProcessScriptCustomize dialogTitle
		(
				--// script container
			--edittext edScript "" pos:[2,4] height:360 width:392
			edittext edScript "" pos:[2,4] height:360 fieldWidth:392
				--// applies the script to the current scene
			button btnPreview "Preview" pos:[6,372] width:50 height:20 toolTip:"Executes the script on the current scene"
				--// applies the script to the current scene
			button btnClear "Clear" pos:[77,372] width:50 height:20 toolTip:"Clears the script in the edit box"
				--// imports other scripts
			button btnImport "Import" pos:[150,372] width:50 height:20 toolTip:"Opens a dialog to import .ms script files"
				--// saves the script and destroys dialog
			button btnCache "Cache" pos:[260,372] width:50 height:20 toolTip:"Stores the custom script in the memory"
				--// saves the script and destroys dialog
			button btnSaveAs "Save As" pos:[205,372] width:50 height:20 toolTip:"Saves the custom script in the edit box into a specific file"
				--// destroys dialog without saving
			button btnCancel "Cancel" pos:[345,372] width:50 height:20 toolTip:"Aborts the custom script generation"



			on batchProcessScriptCustomize open do
			(
				case cryTools.cryAnim.UI.batchProcess.customizeScript.title of
				(
					"First Script Customization": local tempVarScript = cryTools.cryAnim.UI.batchProcess._v.firstScript
					"Second Script Customization": local tempVarScript = cryTools.cryAnim.UI.batchProcess._v.secondScript
				)
					--// if there is a defined script already
				if tempVarScript != "" then
						--// set text of the script container to the already defined script
					edScript.text = tempVarScript
			)
			
			
								
			on btnPreview pressed do
			(
				try
				(
						--// tries executing the script
					tempString = execute( edScript.text )
					--print tempString
				)
				catch
				(
						--// if an error occured, print the error message
					format "*** % ***\n" (getCurrentException())
				)
			)
			
			
			on btnClear pressed do
			(
				edScript.text = ""
			)
			
			
			on btnImport pressed do
			(
				ret = "\r\n"
				local tempVar = getOpenFileName caption:"First Script Import" filename:(getDir #scripts + "\\*.ms") types:"Script Files (*.ms)|*.ms"
				if tempVar != undefined then
				(
					cryTools.cryAnim.UI.batchProcess.customizeScript.edScript.text += ret + ret + ret +"  -- Imported from " + tempVar + ret + ret
					
					local tempStream = openFile tempVar mode:"r"
					while (eof tempStream) != true do							
						cryTools.cryAnim.UI.batchProcess.customizeScript.edScript.text += readLine tempStream + ret
					
					close tempStream
				)
			)
			
			
			
				
			on btnCache pressed do
			(
				local tempVar = ""
				case cryTools.cryAnim.UI.batchProcess.customizeScript.title of
				(
					"First Script Customization": ( cryTools.cryAnim.UI.batchProcess._v.firstScript = edScript.text  ; tempVar = "First" )
					"Second Script Customization": ( cryTools.cryAnim.UI.batchProcess._v.secondScript = edScript.text ; tempVar = "Second" )
				)
				
				cryTools.cryAnim.UI.batchProcess._f.updateScriptLists scriptUpdate:tempVar
				
				--// destroys the dialog
				destroyDialog cryTools.cryAnim.UI.batchProcess.customizeScript
			)
			
			
			on btnSaveAs pressed do
			(
				local tempString = ""
				
				case cryTools.cryAnim.UI.batchProcess.customizeScript.title of
				(
					"First Script Customization": tempString = "First"
					"Second Script Customization": tempString = "Second"
				)
				
				local tempVar = getSaveFileName caption:"First Script Import" filename:(cryTools.buildPathFull + "Tools\\maxscript\\cryAnim\\ui\\batch\\" + tempString + "Script\\*.ms") types:"Script Files (*.ms)|*.ms"
				if tempVar != undefined then
				(
					local tempStream = openFile tempVar mode:"w"
					format edScript.text to:tempStream
					close tempStream
					
					local tempFilter = filterString tempVar "\\"
					local tempString = (filterString tempFilter[tempFilter.count] ".")[1]
					
					case cryTools.cryAnim.UI.batchProcess.customizeScript.title of
					(
						"First Script Customization": tempVar = "First|" + tempString
						"Second Script Customization": tempVar = "Second|" + tempString
					)
					cryTools.cryAnim.UI.batchProcess._f.updateScriptLists scriptUpdate:tempVar
					destroyDialog cryTools.cryAnim.UI.batchProcess.customizeScript
				)
				
			)
			
								
			on btnCancel pressed do
			(
				cryTools.cryAnim.UI.batchProcess._f.updateScriptLists scriptUpdate:"None"
					--// destroys the dialog
				destroyDialog cryTools.cryAnim.UI.batchProcess.customizeScript
			)
		)
			--// creates the dialog
		cryTools.cryAnim.UI.batchProcess.customizeScript = batchProcessScriptCustomize
		batchProcessScriptCustomize = undefined
		
		createDialog cryTools.cryAnim.UI.batchProcess.customizeScript 400 400
		
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess._f.createScriptDialog" )
)
createScriptDialog = undefined
logOutput "> Created cryTools.cryAnim.UI.batchProcess._f.createScriptDialog function"












--###############################################################################
	--// creates the batchProcess dialog
--###############################################################################
cryTools.cryAnim.UI.batchProcess._f.callDialog = function callDialog =
(
	try
	(
			--// if batchProcess is already opened, close the rollout floater
		try ( closeRolloutFloater cryTools.cryAnim.UI.batchProcess.dialog ) catch()
			--// create new batchProcess rollout floater
		cryTools.cryAnim.UI.batchProcess.dialog = newRolloutFloater "CryAnim  Batch  Process   v2.7" 600 422
		
			--// rollout with file and folder list
		rollout fileStatusRO "File Status" height:200
		(
				--// files list
			activeXControl lbFiles "MSComctlLib.ListViewCtrl" pos:[140,8] height:185 width:440
				--// sub folders list
			activeXControl lbSubFolders "MSComctlLib.TreeCtrl" pos:[8,8] height:185 width:120
				

			
			on fileStatusRO open do
			(
				try
				(
						--// initialise
					lbFiles.GridLines = true
					lbFiles.MousePointer = #ccArrow
					lbFiles.AllowColumnReorder = true
					lbFiles.view = #lvwReport
					lbFiles.LabelEdit = #lvwManual
					lbFiles.LabelWrap = true
					lbFiles.MultiSelect = true
					lbFiles.FullRowSelect = true
								
										
					lbSubFolders.LineStyle = #tvwTreeLines
					lbSubFolders.Style = #tvwTreelinesPlusMinusText
					lbSubFolders.sorted = true
					lbSubFolders.checkboxes = true
					lbSubFolders.Indentation = 50
										

						--// adds columns				
					lbFiles.columnHeaders.Add text:"Filename"
					lbFiles.columnHeaders.Add text:"Bone"
					lbFiles.columnHeaders.Add text:"Export"
					lbFiles.columnHeaders.Add text:"Ext"
				)
				catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess.dialog.fileStatusRO.open" )
			)
								
			
						
			on lbFiles Click do
			(
				try
				(
						--// gets the cursor position of the active rollout part
					screenPos = getCursorPos lbFiles
						--// applies a hit test of the current mouse position
					tempItem = lbFiles.hittest ((screenPos.x-2)*15) ((screenPos.y-2)*15)
					
						--// if nothing is selected
					if tempItem == undefined then
					(
							--// go through the files list and deselect every entry
						for i = 1 to lbFiles.ListItems.count do					
							lbFiles.ListItems[i].selected = false
					)
						--// if an item is selected
					else
					(
							--// if a folder is selected, deselect it
						if tempItem.ListSubItems[3].text == "folder" then
							tempItem.selected = false
							
					)
						--// update the files counter
					cryTools.cryAnim.UI.batchProcess._f.updateCounter()
				)
				catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess.dialog.fileStatusRO.lbFiles.click" )
			)



			on lbSubFolders NodeCheck checkedNode do
			(
				try
				(
					if cryTools.cryAnim._v.various[120] != true then
					(
						--// if a state changes in the sub folders list, update the whole dialog
						cryTools.cryAnim.UI.batchProcess._v.flags[1] = true
						cryTools.cryAnim.UI.batchProcess._f.updateDialog()
						cryTools.cryAnim.UI.batchProcess._f.updateSubFolderSelection #set
					)
				)
				catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess.dialog.fileStatusRO.lbSubFolders.nodeCheck" )
			)



			on lbFiles DblClick do
			(
				try
				(
						--// gets the cursor position of the active rollout part
					screenPos = getCursorPos lbFiles
						--// applies a hit test of the current mouse position
					tempItem = lbFiles.hittest ((screenPos.x-2)*15) ((screenPos.y-2)*15)
					
						--// if an item is selected
					if tempItem != undefined then
					(
							--// if a folder is selected
						if tempItem.ListSubItems[3].text == "folder" then
						(
								--// deselect the folder
							tempItem.selected = false
							
								--// go through the files list and select every on-coming entry (without folders) until another folder is reached
							for i = (tempItem.index + 1) to lbFiles.ListItems.count do
							(
								if lbFiles.ListItems[i].ListSubItems[3].text != "folder" then
									lbFiles.ListItems[i].selected = true
								else
									exit
							)
						)
							--// if no folder is selected
						else
						(
								--// go through the list and select every entry without folders
							for i = 1 to lbFiles.ListItems.count do
								if lbFiles.ListItems[i].ListSubItems[3].text != "folder" then
									lbFiles.ListItems[i].selected = true
						)
					)	
						--// update the files counter
					cryTools.cryAnim.UI.batchProcess._f.updateCounter()
				)
				catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess.dialog.fileStatusRO.lbFiles.dblClick" )
			)
			
		)
		logOutput "> Created fileStatusRO rollout"




			--// rollout with paths, file mask and pre export script
		rollout inputOutputRO "Input    /    Output" height:50
		(
				--// source folder
			button btnSourceFolder "Source" pos:[8,5] width:60 height:20 toolTip:"Opens a dialog to choose the source folder"
			label labSourceFolder "No Folder selected" pos:[80,7] width:300
						--// output folder
			button btnExportFolder "Export" pos:[8,27] width:60 height:20 toolTip:"Opens a dialog to choose the export folder"
			label labExportFolder "No Folder selected" pos:[80,29] width:300
			
				--// file mask
			label labFileMask "File Mask" pos:[16,52]
			edittext etFileMask "" text:"" pos:[75,50] fieldWidth:300
			
			groupbox gbScripts " Process Templates " pos:[400,5] width:180 height:62
			
			label labFirstOp "1." pos:[408,24]
			dropdownlist ddFirstOp "" pos:[425,21] width:150
			
			label labSecondOp "2." pos:[408,46]
			dropdownlist ddSecondOp "" pos:[425,42] width:150



			
			
			
			on inputOutputRO open do
			(
				try
				(
						--// if a source path is already set
					if cryTools.cryAnim.UI.batchProcess._v.sourcePath != undefined then
							--// set text to the old source path
						labSourceFolder.text = cryTools.cryAnim.UI.batchProcess._v.sourcePath

						--// if the ini setting for the file extension is found
					if (local tempText = cryTools.cryAnim.base.iniFile #get #batchProcessExt) != "" then
							--// set file mask to the ini setting
						etFileMask.text = tempText
					
						--// if the ini setting for the source path is found
					if (tempText = cryTools.cryAnim.base.iniFile #get #batchProcessSourcePath) != "" then
					(
							--// set source string to the ini setting
						labSourceFolder.text = tempText
						cryTools.cryAnim.UI.batchProcess._v.sourcePath = tempText
					)
					else
							--// otherwise to the default string
						labSourceFolder.text = "No Source Folder selected"
					
						--// if the ini setting for the export path is found
					if (tempText = cryTools.cryAnim.base.iniFile #get #batchProcessExportPath) != "" then
					(
							--// set export string to the ini setting
						labexportFolder.text = tempText
						cryTools.cryAnim.UI.batchProcess._v.exportPath = tempText
					)
					else
							--// otherwise to the default string
						labexportFolder.text = "No Export Folder selected"

						--// updates the first and second scripts from the folders
					cryTools.cryAnim.UI.batchProcess._f.updateScriptLists()
					

						--// fills the sub folders list
					cryTools.cryAnim.UI.batchProcess._f.updateSubFolders()
					cryTools.cryAnim.UI.batchProcess._f.updateSubFolderSelection #get
				)
				catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess.dialog.inputOutputRO.open" )
			)

			
			
			on btnSourceFolder pressed do
			(
				try
				(
					local tempPath = ""
						--// if a source path is defined
					if cryTools.cryAnim.UI.batchProcess._v.sourcePath != undefined then
							--// set temporary path to the source path
						tempPath = cryTools.cryAnim.UI.batchProcess._v.sourcePath
					else
							--// otherwise get new directory from an input dialog
						tempPath = (cryTools.cryAnim.base.perforce cryTools.cryAnim.UI.main._v.bipSavePath #getDirectory)

						--// temporary string gets open directory input for source path
					tempString = (getSavePath caption:"Select Source Folder" initialDir:tempPath)
					
						--// if a folder is selected by the open directory
					if tempString != undefined then
					(
							--// source path is the new folder with "\"
						cryTools.cryAnim.UI.batchProcess._v.sourcePath = tempString + (if (filterString tempString "\\").count > 1 then "\\" else "")
							--// updates the export path with the converted path to the project directory
						cryTools.cryAnim.UI.batchProcess._v.exportPath = (cryTools.cryAnim.base.perforce (cryTools.cryAnim.UI.main._f.checkExport #ProductionToGame cryTools.cryAnim.UI.batchProcess._v.sourcePath) #getDirectory)
						
							--// update export folder text
						labExportFolder.text = cryTools.cryAnim.UI.batchProcess._v.exportPath
							--// update source folder text
						labSourceFolder.text = cryTools.cryAnim.UI.batchProcess._v.sourcePath
						
							--// set ini setting for the source path
						cryTools.cryAnim.base.iniFile #set #batchProcessSourcePath value:labSourceFolder.text
						cryTools.cryAnim.base.iniFile #set #batchProcessExportPath value:labExportFolder.text
						
						if (findString labSourceFolder.text "\\") != undefined then
						(
								--// clears and fills sub folders
							cryTools.cryAnim.UI.batchProcess._f.updateSubFolders()
							(cryTools.cryAnim.UI.batchProcess.dialog.rollouts[1].lbSubFolders.Nodes.item 0).checked = true
								--// clears and fills the file list
							cryTools.cryAnim.UI.batchProcess._f.updateDialog()
						)
					)
						--// if no folder is selected
					else
					(
							--// if no export path is set before
						if cryTools.cryAnim.UI.batchProcess._v.sourcepath == undefined then
						(
								--// enable statistic button as it can be used without exporting
							cryTools.cryAnim.UI.batchProcess.dialog.rollouts[3].btnProcess.enabled = true
						)
					)
				)
				catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess.dialog.inputOutputRO.btnSourceFolder.pressed" )
			)
			
			
			on btnExportFolder pressed do
			(
				try
				(
						--// temporary string gets open directory input for export path
					tempString = (getSavePath caption:"Select Export Folder" initialDir:cryTools.cryAnim.UI.batchProcess._v.exportPath)
						--// if a folder is selected by the open directory
					if tempString != undefined then
					(
							--// set export path to the new folder path
						cryTools.cryAnim.UI.batchProcess._v.exportPath = tempString + "\\"
							--// update exportFolder text
						labExportFolder.text = tempString + "\\"
						
							--// set ini setting for the export path
						cryTools.cryAnim.base.iniFile #set #batchProcessExportPath value:labExportFolder.text
					)
				)
				catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess.dialog.inputOutputRO.btnExportFolder.pressed" )
			)
			
			
			on ddFirstOp selected value do
			(
				try
				(
					if ddFirstOp.selection == ddFirstOp.items.count then
						cryTools.cryAnim.UI.batchProcess._f.createScriptDialog "First Script Customization"
					else
						cryTools.cryAnim._v.various[33] = ddFirstOp.items[ddFirstOp.selection]
				)
				catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess.dialog.inputOutputRO.ddFirstOp.selected" )
			)
			
			
			on ddSecondOp selected value do
			(
				try
				(
					if ddSecondOp.selection == ddSecondOp.items.count then
						cryTools.cryAnim.UI.batchProcess._f.createScriptDialog "Second Script Customization"
					else
						cryTools.cryAnim._v.various[34] = ddSecondOp.items[ddSecondOp.selection]
				)
				catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess.dialog.inputOutputRO.ddSecondOp.selected" )
			)
			
			
			
			
			on etFileMask entered value do
			(
				try
				(
						--// set ini setting with changed file mask
					cryTools.cryAnim.base.iniFile #set #batchProcessExt value:etFileMask.text
						--// update whole dialog with new changes
					cryTools.cryAnim.UI.batchProcess._v.flags[1] = true
					cryTools.cryAnim.UI.batchProcess._f.updateDialog()	
				)
				catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess.dialog.inputOutputRO.etFileMask.entered" )
			)
		)
		logOutput "> Created inputOutputRO rollout"







				
							
			--// rollout with all config options and check or export button
		rollout checkExportRO "Check   /   Export" height:60
		(
				--// files / folders groub
			label labFilesFolders "Files / Folders :" pos:[10,11]
			groupBox gbFilesFolders "" pos:[100,-1] height:32 width:230

				--// keep the sub folder structure when exporting
			label labKeepSubFolders "Keep Sub Folders" pos:[210,11]
			checkBox chkKeepSubFolders "" pos:[300,11] checked:true
		
				--// counter for selected and maximumm files
			label labCount "Count :" pos:[130,11]
			
				--// bone detection for exporting specific body parts for specific file string parts
			label labBoneDetection "Bone Detection :" pos:[10,42]
			groupBox gbBoneDetection "" pos:[100,30] height:32 width:230
			
				--// config bone detection, manage sets of bone detections
			button btnConfig "Config" pos:[110,41] height:17 width:70 toolTip:"Opens a dialog to configure filename/bone detection"
				--// activate automatic file detection
			label labDetect "Detect" pos:[195,43]
			checkBox chkDetect "" pos:[235,42] checked:true
				--// only files which are detected will be shown
			label labOnlyBoneDetection " + " pos:[255,43]
			checkBox chkOnlyBoneDetection "" pos:[270,42]
				--// only files which are not detected will be shown
			label labNoBoneDetection " - " pos:[290,43]
			checkBox chkNoBoneDetection "" pos:[300,42]													
		
				--// executes the whole export process with pre export script, but without real exporting				
			button btnProcess "P     R     O     C     E     S     S" pos:[345,8] height:50 width:230 enabled:false toolTip:"Processes the files/selection in the list (executes scripts)"
							

			
							
			on checkExportRO open do
			(
				try
				(
						--// clears the file list
					cryTools.cryAnim.UI.batchProcess._v.exportFiles = #()
					
					try 
					(
						local tempSubFilter = cryTools.cryAnim.base.iniFile #get #batchProcessSubFolderSelection
						tempSubFilter = filterString tempSubFilter "#"
						
						if tempSubFilter.count == 0 then
							(cryTools.cryAnim.UI.batchProcess.dialog.rollouts[1].lbSubFolders.Nodes.item 0).checked = true 
					
					) catch()
					
					try cryTools.cryAnim.UI.batchProcess.dialog.rollouts[3].chkKeepSubFolders.checked = cryTools.cryAnim.base.iniFile #get #batchProcessSubFolders catch()
					
						--// if a source path is internal defined
					if cryTools.cryAnim.UI.batchProcess._v.sourcePath != undefined then
					(
							--// enable check and export button
						cryTools.cryAnim.UI.batchProcess.dialog.rollouts[3].btnProcess.enabled = true
					)
					
						--// get bone detection set up
					local tempBoneArray = cryTools.cryAnim.base.iniFile #get #bones
						--// if a bone setup is found
					if tempBoneArray != undefined then
					(
						tempListArray = #()
							--// goes through the bone list
						for i = 1 to tempBoneArray.count do
								--// if an entry is found
							if tempBoneArray[i].name != "" then
									--// add the entry to the list
								append tempListArray tempBoneArray[i]
								
							--// set new bone list
						cryTools.cryAnim.UI.batchProcess._v.boneList = tempBoneArray								
					)
					
						--// update whole dialog
					cryTools.cryAnim.UI.batchProcess._v.flags[1] = true
					cryTools.cryAnim.UI.batchProcess._f.updateDialog()
				)
				catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess.dialog.checkExportRO.open" )
			)			
									
								

			
			on chkKeepSubFolders changed value do
			(
				try					
					cryTools.cryAnim.base.iniFile #set #batchProcessSubFolders value:value
				catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess.dialog.checkExportRO.chkKeepSubFolder.changed" )
			)
			
								
			on btnProcess pressed do
			(
				try
					--// process all used files without export
					cryTools.cryAnim.UI.batchProcess._f.processFiles #statistic
				catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess.dialog.checkExportRO.btnProcess.pressed" )
			)
								
							
			on chkOnlyBoneDetection changed value do
			(
				try
				(
						--// if only bone detection is activated
					if cryTools.cryAnim.UI.batchProcess.dialog.rollouts[3].chkOnlyBoneDetection.checked == true then
							--// no bone detection is deactivated
						cryTools.cryAnim.UI.batchProcess.dialog.rollouts[3].chkNoBoneDetection.checked = false
					
						--// updates whole dialog
					cryTools.cryAnim.UI.batchProcess._v.flags[1] = true
					cryTools.cryAnim.UI.batchProcess._f.updateDialog()
					cryTools.cryAnim.UI.batchProcess._v.flags[1] = false
				)
				catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess.dialog.checkExportRO.chkOnlyBoneDetection.changed" )
			)

								
			on chkNoBoneDetection changed value do
			(
				try
				(
						--// if no bone detection is activated
					if cryTools.cryAnim.UI.batchProcess.dialog.rollouts[3].chkNoBoneDetection.checked == true then
							--// only bone detections is deactivated
						cryTools.cryAnim.UI.batchProcess.dialog.rollouts[3].chkOnlyBoneDetection.checked = false
					
						--// updates whole dialog
					cryTools.cryAnim.UI.batchProcess._v.flags[1] = true
					cryTools.cryAnim.UI.batchProcess._f.updateDialog()
					cryTools.cryAnim.UI.batchProcess._v.flags[1] = false
				)
				catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess.dialog.checkExportRO.chkNoBoneDetection.changed" )
			)

			on btnConfig pressed do
			(
				try
				(
						--// rollout to edit the bone list entry
					rollout entryDetailsRO "Entry Details"
					(
						label labName "Name :" pos:[8,10]
						label labExternal "External :" pos:[8,30]
						label labBones "Bones :" pos:[8,50]
					
							--// sets the name, file detection and bones column
						edittext edName "" text:(cryTools.cryAnim.UI.batchProcess.editBoneList.lbList.FocusedItem.SubItems.item 0).text pos:[70,10] fieldWidth:300
						edittext edExternal "" text:(cryTools.cryAnim.UI.batchProcess.editBoneList.lbList.FocusedItem.SubItems.item 1).text pos:[70,30] fieldWidth:300
						edittext edBones "" text:(cryTools.cryAnim.UI.batchProcess.editBoneList.lbList.FocusedItem.SubItems.item 2).text pos:[70,50] fieldWidth:245
							--// button to pick specific bones in the scene
						button btnPickBones "Pick" pos:[323,50] height:17 width:50 toolTip:"Opens dialog to select the bone associated to the filename detection"
						
							--// save the entry or cancel
						button btnSave "Save" pos:[100,80] height:20 width:80 toolTip:"Saves filename/bone detection"
						button btnCancel "Cancel" pos:[200,80] height:20 width:80 toolTip:"Aborts filename/bone detection"
						
						
						on btnSave pressed do
						(
								--// get currently selected item
							local tempItem = cryTools.cryAnim.UI.batchProcess.editBoneList.lbList.FocusedItem
							
								--// set new name
							(tempItem.SubItems.item 0).text = edName.text
								--// set new file detection
							(tempItem.SubItems.item 1).text = edExternal.text
								--// set new bone list
							(tempItem.SubItems.item 2).text = edBones.text
											
							local tempListArray = #()
							
							cryTools.cryAnim.UI.batchProcess._f.updateExtent()
								--// kills bone edit dialog
							destroyDialog cryTools.cryAnim.UI.batchProcess.entryDetails
						)
						
						
						on btnCancel pressed do
						(
								--// kills bone edit dialog
							destroyDialog cryTools.cryAnim.UI.batchProcess.entryDetails
						)
						
						on btnPickBones pressed do
						(
							local tempString = ""
								--// pick node from the scene
							objArray = selectByName title:("Select Nodes") showHidden:true
							
								--// if a node is selected
							if objArray != undefined then	
							(
									--// goes through the nodes
								for i = 1 to objArray.count do
										--// adds the name of the node with a ";" as seperator
									tempString += objArray[i].name + (if i != objArray.count then ";" else "")
									
									--// set the new bone list
								edBones.text = tempString
							)

						)
							
					)
					cryTools.cryAnim.UI.batchProcess.entryDetails = entryDetailsRO
					entryDetailsRO = undefined
					
					
					
					
					
					
					
					
					
						--// rollout to edit the bone detection list
					rollout editBoneListRO "Edit Bone List"
					(
							--// list of all bone setups
						activeXControl lbList "MSComctlLib.ListViewCtrl" pos:[1,1] height:185 width:440
							--// save current setup
						button btnSave "Save" pos:[8,195] height:20 width:80 toolTip:"Saves selected filename/bone detection entry"
						button btnDelete "Delete" pos:[150,195] height:20 width:60 toolTip:"Deletes selected entry"
						button btnDeleteAll "Delete All" pos:[220,195] height:20 width:60 toolTip:"Clears whole list"
						button btnCancel "Cancel" pos:[350,195] height:20 width:80 toolTip:"Abort filename/bone detection"
					
						
						on editBoneListRO open do
						(
							lbList.GridLines = true
							lbList.MousePointer = #ccArrow
							lbList.AllowColumnReorder = true
							lbList.view = #lvwReport
							lbList.LabelEdit = #lvwManual							
							lbList.Sorted = true
							lbList.FullRowSelect = true
							lbList.Checkboxes = true
							
								--// adds columns
							lbList.columnHeaders.Add text:"Name"
							lbList.columnHeaders.Add text:"External"
							lbList.columnHeaders.Add text:"Bones"

								--// goes through the bone list
							for i = 1 to cryTools.cryAnim.UI.batchProcess._v.boneList.count do
							(
									--// adds a new entry
								local lbListEntry = lbList.listItems.Add text:cryTools.cryAnim.UI.batchProcess._v.boneList[i].name
								
									--// if entry is active
								if cryTools.cryAnim.UI.batchProcess._v.boneList[i].active == "true" then
										--// set list entry active
									lbListEntry.checked = true
									--// if entry is not active
								if cryTools.cryAnim.UI.batchProcess._v.boneList[i].active == "false" then
										--// set list entry not active
									lbListEntry.checked = false
									
									--// add external entry
								lbListEntry.listSubItems.Add text:cryTools.cryAnim.UI.batchProcess._v.boneList[i].external
									--// add bone entry
								lbListEntry.listSubItems.Add text:cryTools.cryAnim.UI.batchProcess._v.boneList[i].bones
							)
						)
						
						
						on lbList DblClick do
						(
								--// gets the bone list position
							screenPos = getCursorPos lbList
								--// hit test the cursor position
							tempItem = lbList.hittest ((screenPos.x-2)*15) ((screenPos.y-2)*15)
							
								--// if an item is selected
							if tempItem != undefined then
							(
									--// creates bone entry edit
								createDialog cryTools.cryAnim.UI.batchProcess.entryDetails 385 110
								
							)
								--// if no item is selected
							else
							(
									--// empty boneStruct
								tempStringArray = (boneStruct name:"" external:"" bones:"")
								tempArray = #()
								
									--// get nodes to be added
								objArray = selectByName title:("Select Nodes to be added") showHidden:true
								
									--// if a node is selected
								if objArray != undefined then
								(
										--// goes through node list
									for obj in objArray do
											--// add the nodes via boneStruct to a seperate list
										append tempArray (boneStruct name:obj.name external:obj.name bones:obj.name)
									
										--// goes through the node list
									for i = 1 to tempArray.count do
									(
											--// adds name(s) of the node(s)
										tempStringArray.name += tempArray[i].name + (if i != tempArray.count then ";" else "")
											--// adds file detection(s)
										tempStringArray.external += tempArray[i].external + (if i != tempArray.count then ";" else "")
											--// adds bone(s) list
										tempStringArray.bones += tempArray[i].bones + (if i != tempArray.count then ";" else "")
									)
									
										--// adds entry to the bone list
									local lbListEntry = lbList.ListItems.Add text:tempStringArray.name
									lbListEntry.checked = true
									lbListEntry.ListSubItems.Add text:tempStringArray.external
									lbListEntry.ListSubItems.Add text:tempStringArray.bones
								)
							)
						)
						
						on btnSave pressed do
						(
							local tempArray = #()
							local tempList = #()
							
								--// goes through the bone list
							for i = 1 to lbList.ListItems.count do
							(								
								local tempItemArray = #()
								local tempItem = lbList.ListItems[i]
								
									--// save checked state
								tempItemArray[1] = tempItem.checked as String
									--// if nothing is typed in, the entry will have " " to filter correctly
								if tempItem.text != "" then tempItemArray[2] = tempItem.text else tempItemArray[2] = " "
								if tempItem.ListSubItems[1].text != "" then tempItemArray[3] = tempItem.ListSubItems[1].text else tempItemArray[3] = " "
								if tempItem.ListSubItems[2].text != "" then tempItemArray[4] = tempItem.ListSubItems[2].text else tempItemArray[4] = " "
							
									--// adds the entries to a seperate list
								append tempArray (boneStruct active:tempItemArray[1] name:tempItemArray[2] external:tempItemArray[3] bones:tempItemArray[4])
							)
							
								--// sets ini setting	
							cryTools.cryAnim.base.iniFile #set #bones value:tempArray
								--// sets new bone list					
							cryTools.cryAnim.UI.batchProcess._v.boneList = tempArray
							
								--// updates whole dialog
							cryTools.cryAnim.UI.batchProcess._f.updateDialog()
							
								--// kills bone list edit dialog
							destroyDialog cryTools.cryAnim.UI.batchProcess.editBoneList

							
						)
						
						on btnDelete pressed do
						(	
								--// goes through the bone list				
							for i = 1 to lbList.ListItems.count do
							(
									--// tries to delete the selected entry
								try
								(
									if lbList.ListItems[i].selected == true then
										lbList.ListItems.Remove i
								)
								catch()
							)
						)
							
						on btnDeleteAll pressed do
						(
								--// clears whole list
							lbList.ListItems.clear()
						)
						
						
						on btnCancel pressed do
						(
								--// kills bone list edit dialog
							destroyDialog cryTools.cryAnim.UI.batchProcess.editBoneList
						)
					)
					cryTools.cryAnim.UI.batchProcess.editBoneList = editBoneListRO
					editBoneListRO = undefined
					
						--// creates bone list edit dialog	
					createDialog cryTools.cryAnim.UI.batchProcess.editBoneList 443 220
				)
				catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess.dialog.checkExportRO.btnConfig.pressed" )
			)
		)
		logOutput "> Created checkExportRO rollout"
	
	
			--// adds all rollouts to the UI
		addRollout fileStatusRO cryTools.cryAnim.UI.batchProcess.dialog
		addRollout inputOutputRO cryTools.cryAnim.UI.batchProcess.dialog
		addRollout checkExportRO cryTools.cryAnim.UI.batchProcess.dialog
		
		fileStatusRO = undefined
		inputOutputRO = undefined
		checkExportRO = undefined
		
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess._f.callDialog" )
)
callDialog = undefined
logOutput "> Created cryTools.cryAnim.UI.batchProcess._f.callDialog function"



logOutput ">> batch8.ms loaded"
