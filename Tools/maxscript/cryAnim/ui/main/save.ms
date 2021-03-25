--###############################################################################
	--// rollout with all the file control _f like Load Biped File, Save, Export etc.
--###############################################################################
rollout loadSaveRO "Load / Save / Export"
(
	groupBox gbLoad " Load " pos:[2,8] width:153 height:50
	button btnLoad "Load Biped File" pos:[8,31] width:142 height:20 toolTip:"Loads biped file from the working directory or last used file"
	
	groupBox gbSave " Save  /  Export " pos:[2,70] width:153 height:315

	button btnBegin "Start" pos:[8,92] width:46 height:20 toolTip:"Sets the start time to save/export"
	button btnEnd "End" pos:[60,92] width:46 height:20 toolTip:"Sets the end time to save/export"
	button btnRange "Range" pos:[110,92] width:40 height:20 toolTip:"Sets the range for save/export"
	button btnAll "All" pos:[110,113] width:40 height:20 toolTip:"Sets the animation range for save/export"

	spinner spnBegin "" pos:[6,113] range:[0,9999,0] type:#integer fieldWidth:36
	spinner spnEnd "" pos:[58,113] range:[0,9999,0] type:#integer fieldWidth:36

	
	checkbox chkGlobalHuman "  GlobalHuman.cal" pos:[12,140] tooltip:"Creates entry in GlobalHuman.cal" checked:false enabled:false
	
	button btnExportAdd "Add" pos:[8,165] width:66 height:20 toolTip:"Adds a new node to the exporter list"
	button btnExportDelete "Delete" pos:[84,165] width:66 height:20 enabled:false toolTip:"Deletes selected node from the exporter list"
	listbox lbExport "" pos:[8,190] width:142 height:2 items:#("Bip01") selection:0 enabled:false

	button btnSave "Save" pos:[8,235] width:66 height:20 toolTip:"Saves the biped file"
	button btnExport "Export" pos:[84,235] width:66 height:20 toolTip:"Exports to .caf"
	button btnSaveExport "Save / Export" pos:[8,265] width:142 height:20 toolTip:"Saves to biped file and exports to .caf"
	button btnBatchProcess "Batch Process" pos:[8,360] width:142 height:20 toolTip:"Opens the Batch Process"
	
	listbox lbStatus "" pos:[8,290] width:142 height:4
	listbox lbStatusFilepath "" pos:[0,0] width:1 height:1 visible:false
	
	
	
	on loadSaveRO open do
	(
		try
		(
			try (if (cryTools.cryAnim.base.iniFile #get #rolloutStates) == true then (cryTools.cryAnim.UI.main._f.getUI "Load / Save / Export" "").open = cryTools.cryAnim.base.iniFile #get #loadSaveRO) catch()
			try cryTools.cryAnim.UI.main.loadSave._v.bipOpenPath = cryTools.cryAnim.base.iniFile #get #loadBiped catch()
			
			spnBegin.value = animationRange.start.frame
			spnEnd.value = animationRange.end.frame
			if $Bip01 != undefined then
			(
				lbExport.items = #($Bip01.name)
				lbExport.selection = 0
			)
			
			cryTools.cryAnim.UI.main._f.updateDialog()
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.loadSaveRO.open" )
	)
	
	on loadSaveRO rolledUp value do
	(
		try
		(
			if (cryTools.cryAnim.base.iniFile #get #loadSaveRO) != value then
				cryTools.cryAnim.base.iniFile #set #loadSaveRO
			
			cryTools.cryAnim.UI.main._f.updateDialog()
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.loadSaveRO.rolledUp" )
	)

	
	
	on btnBegin pressed do
	(
		try
			spnBegin.value = sliderTime
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.loadSaveRO.btnBegin.pressed" )
	)

	on btnEnd pressed do
	(
		try
			spnEnd.value = sliderTime
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.loadSaveRO.btnEnd.pressed" )
	)

	
	on btnRange pressed do
	(
		try
		(
			spnBegin.value = animationRange.start.frame
			spnEnd.value = animationRange.end.frame
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.loadSaveRO.btnRange.pressed" )
	)
	
	on btnAll pressed do
	(
		try
		(
			baseBip = cryTools.cryAnim._f.getBaseBip()
			if baseBip != undefined then
			(
				tempInterval = biped.getCurrentRange baseBip.controller
			
				spnBegin.value = tempInterval.start.frame
				spnEnd.value = tempInterval.end.frame
			)
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.loadSaveRO.btnAll.pressed" )
	)
	
	on btnExportAdd pressed do
	(
		try
		(
			if $selection.count > 0 then
			(		
				local tempArray = lbExport.items
				
				for obj in $selection do
				(
					local exist = false
					for i = 1 to lbExport.items.count do
						if obj.name == lbExport.items[i] then
							exist = true
					
					if exist == false then
						append tempArray obj.name
					
				)
				
				sort tempArray
				lbExport.items = tempArray
				if tempArray.count > 1 then
					
				
				if tempArray.count > 1 then
				(
					local tempInt = findItem tempArray "Bip01"
					if tempInt != 0 then
					(
						deleteItem tempArray tempInt
						lbExport.enabled = true					
					)
						
					lbExport.items = tempArray
					
					lbExport.selection = 0
					btnExportDelete.enabled = false
				)
			)
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.loadSaveRO.btnExportAdd.pressed" )
	)
	
	on btnExportDelete pressed do
	(
		try
		(
			if lbExport.selection > 0 then
			(
				local tempArray = lbExport.items
				deleteItem tempArray lbExport.selection
				sort tempArray
				lbExport.items = tempArray
			)					
			if lbExport.items.count == 0 then
				lbExport.items = #($Bip01.name)						
			
			if lbExport.items[1] == "Bip01" then
				lbExport.enabled = false
			else
				lbExport.enabled = true
			
			lbExport.selection = 0
			btnExportDelete.enabled = false
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.loadSaveRO.btnExportDelete.pressed" )
	)
	
	on lbExport selected value do
	(
		try
		(
			if lbExport.items.count <= 1 and lbExport.items[1] == "Bip01" then
			(
				lbExport.selection = 0
				btnExportDelete.enabled = false
			)
			else
				btnExportDelete.enabled = true
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.loadSaveRO.lbExport.selected" )
	)
	
	
	on btnLoad pressed do
	(
		try
		(
			local baseBip = cryTools.cryAnim._f.getBaseBip op:#load
			local loadedBiped = undefined
			
			if baseBip != undefined then
			(
				if cryTools.cryAnim.UI.batchProcess._v.fileQue == undefined then
					filepath = getOpenFileName filename:cryTools.cryAnim.UI.main.loadSave._v.bipOpenPath types:"Biped (*.bip)|*.bip"
				else
					filepath = cryTools.cryAnim.UI.batchProcess._v.fileQue

				if filepath != undefined then
				(				
					tempStatus = undefined
					if cryTools.cryAnim._v.perforceDir != undefined then
					(
						case (cryTools.cryAnim.UI.main._f.getUI "Perforce" "radLoadOpen").state of
						(
							1: tempStatus = cryTools.cryAnim.base.perforce filepath #open
							2: tempStatus = false
							3: tempStatus = cryTools.cryAnim.base.perforce filepath #checkForLoad
						)
					)
					else
						tempStatus = false
					
					if tempStatus != undefined then
					(
						local tempFilterFilename = cryTools.cryAnim.base.perforce filepath #getFilename
						local tempUndoString = ("L - " + cryTools.cryAnim.base.perforce filepath #getFilename)
						undo tempUndoString on
						(
							if (loadedBiped = biped.loadBipFile baseBip.controller filepath) == true then
							(
								cryTools.cryAnim.UI.main.loadSave._v.bipOpenPath = filepath
								cryTools.cryAnim.UI.main.loadSave._v.bipSavePath = filepath
								
								cryTools.cryAnim.base.iniFile #set #loadBiped
								
								if (exportPath = cryTools.cryAnim.UI.main._f.checkExport #ProductionToGame filepath) != false then
									cryTools.cryAnim.UI.main.loadSave._v.cafSavePath = exportPath
									
								--cryTools.cryAnim.UI.checkExport #createFolder exportPath
								
								tempInterval = biped.getCurrentRange baseBip.controller
								if tempInterval.start == tempInterval.end then
									tempInterval.end += 1

								spnBegin.value = tempInterval.start.frame
								spnEnd.value = tempInterval.end.frame
								
								animationRange = tempInterval
								
								try (animationRange = biped.getCurrentRange baseBip.controller) catch ( )
								
								local itemFound = false
								for i = 1 to cryTools.cryAnim.UI.main.models._v.itemList.count do
								(
									if (findString filepath cryTools.cryAnim.UI.main.models._v.itemList[i].external) != undefined then
									(
										cryTools.cryAnim.UI.main.models._f.selectItem i #set
										itemFound = true
									)
								)
								if itemFound == false then											
									cryTools.cryAnim.UI.main.models._f.selectItem 1 #set

								
								
								tempArray = lbStatus.items
								
								tempStatusString = ""								
								if tempStatus == true then
									tempStatusString = "L+o"
								else
									tempStatusString = "L"
									
								append tempArray (tempStatusString + " - " + tempFilterFilename)
								
								lbStatus.items = tempArray
								lbStatus.selection = tempArray.count
							
								tempFileArray = lbStatusFilepath.items
								append tempFileArray filepath
								lbStatusFilepath.items = tempFileArray

								
							)
						)
					)
				)
				return #(true, loadedBiped)
			)
			else
				return #(false, loadedBiped)
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.loadSaveRO.btnLoad.pressed" )
	)

	on btnSave pressed do
	(
		try
		(
			with undo off
			(
				local baseBip = cryTools.cryAnim._f.getBaseBip()
				if baseBip != undefined then
				(
					if cryTools.cryAnim.UI.main.loadSave._v.bipSavePath == (cryTools.cryAnim.UI.main._v.bipWorkingDir + "*.bip") or ((cryTools.cryAnim.UI.main._f.getUI "Settings" "radSavePrompt").state == 1) then
						filepath = getSaveFileName filename:cryTools.cryAnim.UI.main.loadSave._v.bipSavePath types:"Biped (*.bip)|*.bip"
					else
						filepath = cryTools.cryAnim.UI.main.loadSave._v.bipSavePath
					
					
					if filepath != undefined then
					(
						local tempFilterFilename = filterString filepath "\\" ; tempFilterFilename = tempFilterFilename[tempFilterFilename.count]
						local tempBool = true
						
						local tempStatus = undefined
						
						if cryTools.cryAnim._v.perforceDir != undefined then
						(
							case (cryTools.cryAnim.UI.main._f.getUI "Perforce" "radSaveOpen").state of
							(
								1: tempStatus = cryTools.cryAnim.base.perforce filepath #open								
								2: ( tempStatus = false ; cryTools.cryAnim.base.perforce filepath #setReadMessage )
								3: tempStatus = cryTools.cryAnim.base.perforce filepath #checkForSave
							)
						)
						else
							tempStatus = false
						
						if tempStatus != undefined then
						(
							if (animationRange.start != spnBegin.value) or (animationRange.end != spnEnd.value) then
							(
								tempInterval = cryTools.cryAnim.UI.main.loadSave._f.checkRange spnBegin.value spnEnd.value
								spnBegin.value = tempInterval.start
								spnEnd.value = tempInterval.end
							)
								
							tempBool = biped.saveBipFileSegment baseBip.controller filepath spnBegin.value spnEnd.value #keyPerFrame								
							
							cryTools.cryAnim.UI.main.loadSave._v.bipSavePath = filepath
							cryTools.cryAnim.UI.main.loadSave._v.bipOpenPath = filepath
							
							if (exportPath = cryTools.cryAnim.UI.main._f.checkExport #ProductionToGame filepath) != false then
									cryTools.cryAnim.UI.main.loadSave._v.cafSavePath = exportPath
							
							tempArray = lbStatus.items
							
							local tempStatusString = ""
							if tempStatus == true then
								tempStatusString = "S+o"
							else
								tempStatusString = "S"
								
							append tempArray (tempStatusString + " - " + tempFilterFilename)
							lbStatus.items = tempArray
							lbStatus.selection = tempArray.count
							
							tempFileArray = lbStatusFilepath.items
							append tempFileArray filepath
							lbStatusFilepath.items = tempFileArray
							
							if (cryTools.cryAnim.UI.main._f.checkExport #save filepath) == true then
							(
								if cryTools.cryAnim._v.perforceDir != undefined then
								(
									case (cryTools.cryAnim.UI.main._f.getUI "Perforce" "radSaveAdd").state of
									(
										1: cryTools.cryAnim.base.perforce filepath #add
										3: cryTools.cryAnim.base.perforce filepath #checkForAdd
									)
								)
							)
						)
					)
				)
			)
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.loadSaveRO.btnSave.pressed" )
	)


	




	on btnExport pressed do
	(
		try
		(
			local baseBip = cryTools.cryAnim._f.getBaseBip()
			if baseBip != undefined then
			(					

				if (cryTools.cryAnim.UI.main.loadSave._v.cafSavePath == (crytools.BuildPathFull + "Game\Animations\human\*.caf")) or ((cryTools.cryAnim.UI.main._f.getUI "Settings" "radExportPrompt").state == 1) then
					filepath = getSaveFileName filename:cryTools.cryAnim.UI.main.loadSave._v.cafSavePath types:"Crytek Bone Animation File (*.caf)|*.caf"
				else
					filepath = cryTools.cryAnim.UI.main.loadSave._v.cafSavePath
				
				
				

				if filepath != undefined then
				(
					cryTools.cryAnim.UI.main.loadSave._v.cafSavePath = filepath
					
					if (cryTools.cryAnim.UI.main._f.checkExport #export filepath) == true then
					(
						local tempStatus = undefined
						if cryTools.cryAnim._v.perforceDir != undefined then
						(
							case (cryTools.cryAnim.UI.main._f.getUI "Perforce" "radExportOpen").state of
							(
								1: tempStatus = cryTools.cryAnim.base.perforce filepath #open
								2: ( tempStatus = false ; cryTools.cryAnim.base.perforce filepath #setReadMessage )
								3: tempStatus = cryTools.cryAnim.base.perforce filepath #checkForSave
							)
						)
						else
							tempStatus = false
						
						if tempStatus != undefined then
						(	
							try(	if $'Bip01 R ForeTwist'.parent != $'Bip01 R Forearm' then $'Bip01 R ForeTwist'.parent = $'Bip01 R Forearm' ) catch()
							try(	if $'Bip01 L ForeTwist'.parent != $'Bip01 L Forearm' then $'Bip01 L ForeTwist'.parent = $'Bip01 L Forearm'	) catch()
						
							try (cryTools.cryAnim.UI.main.loadSave._f.bakeMuscleBones() ) catch()
						
							local tempMode = getCommandPanelTaskMode()
							UtilityPanel.OpenUtility CryEngine2_Exporter
							local boneList = #()
							for i = 1 to lbExport.items.count do
								append boneList (getNodeByName lbExport.items[i])
							
							csexport.set_bone_list boneList
							local tempBool = (csexport.export.export_anim filepath)									
							while tempBool != OK do ( escapeEnable = true ; sleep 0.1 )
							
							local tempFilterFilename = filterString filepath "\\"
							
							tempArray = lbStatus.items
							tempStatusString = ""
							if tempStatus == true then
								tempStatusString = "E+o"
							else
								tempStatusString = "E"
							
							append tempArray (tempStatusString + " - " + tempFilterFilename[tempFilterFilename.count])
							lbStatus.items = tempArray									
							lbStatus.selection = tempArray.count
							
							tempFileArray = lbStatusFilepath.items
							append tempFileArray filepath
							lbStatusFilepath.items = tempFileArray
							
							cryTools.cryAnim.UI.main.loadSave._v.cafSavePath = filepath
							setCommandPanelTaskMode mode:tempMode
							
							if cryTools.cryAnim._v.perforceDir != undefined then
							(
								case (cryTools.cryAnim.UI.main._f.getUI "Perforce" "radExportAdd").state of
								(
									1: cryTools.cryAnim.base.perforce filepath #add
									3: cryTools.cryAnim.base.perforce filepath #checkForAdd
								)
							)
						)
					)
				)
			)
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.loadSaveRO.btnExport.pressed" )
	)
	
	on btnSaveExport pressed do
	(
		try
		(
			baseBip = cryTools.cryAnim._f.getBaseBip()
			if baseBip != undefined then
			(
				if (cryTools.cryAnim.UI.main.loadSave._v.bipSavePath == (substring crytools.BuildPathFull 1 (crytools.BuildPathFull.count - 1)) + "_Production\Art\Animation\Human\*.bip") or ((cryTools.cryAnim.UI.main._f.getUI "Settings" "radSaveExportPrompt").state == 1) then						
					filepath = getSaveFileName filename:cryTools.cryAnim.UI.main.loadSave._v.bipSavePath types:"Biped (*.bip)|*.bip"
				else
					filepath = cryTools.cryAnim.UI.main.loadSave._v.bipSavePath

				if filepath != undefined then
				(							
					if (cryTools.cryAnim.UI.main._f.checkExport #saveExport filepath) == true then
					(
						local printBool = #(false,false)
						
						if (exportPath = cryTools.cryAnim.UI.main._f.checkExport #ProductionToGame filepath) != false then
								cryTools.cryAnim.UI.main.loadSave._v.cafSavePath = exportPath
						
						local tempFilename = ""
						
						local tempStatus = undefined
						
						if cryTools.cryAnim._v.perforceDir != undefined then
						(
							case (cryTools.cryAnim.UI.main._f.getUI "Perforce" "radSaveExportOpen").state of
							(
								1: tempStatus = cryTools.cryAnim.base.perforce filepath #open
								2: ( tempStatus = false ; cryTools.cryAnim.base.perforce filepath #setReadMessage )
								3: tempStatus = cryTools.cryAnim.base.perforce filepath #checkForSave
							)
						)
						else
							tempStatus = false
						
						if tempStatus != undefined then
						(
							tempFilterFilename = filterString filepath "\\"
							
							if (animationRange.start != spnBegin.value) or (animationRange.end != spnEnd.value) then
							(
								local tempInterval = cryTools.cryAnim.UI.main.loadSave._f.checkRange spnBegin.value spnEnd.value
								spnBegin.value = tempInterval.start
								spnEnd.value = tempInterval.end
							)
							
							biped.saveBipFileSegment baseBip.controller filepath spnBegin.value spnEnd.value #keyPerFrame
							cryTools.cryAnim.UI.main.loadSave._v.bipSavePath = filepath
							cryTools.cryAnim.UI.main.loadSave._v.bipOpenPath = filepath
							
							
							
							
							printBool[1] = true
							tempFilename = tempFilterFilename[tempFilterFilename.count]
							
							if (cryTools.cryAnim.UI.main._f.checkExport #save filepath) == true then
						
							if cryTools.cryAnim._v.perforceDir != undefined then
							(
								case (cryTools.cryAnim.UI.main._f.getUI "Perforce" "radSaveExportAdd").state of
								(
									1: cryTools.cryAnim.base.perforce filepath #add
									3: cryTools.cryAnim.base.perforce filepath #checkForAdd
								)
							)
						)
						
						local tempStatus = undefined
						
						if cryTools.cryAnim._v.perforceDir != undefined then
						(
							case (cryTools.cryAnim.UI.main._f.getUI "Perforce" "radSaveExportOpen").state of
							(
								1: tempStatus = cryTools.cryAnim.base.perforce exportPath #open
								2: ( tempStatus = false ; cryTools.cryAnim.base.perforce exportPath #setReadMessage )
								3: tempStatus = cryTools.cryAnim.base.perforce exportPath #checkForSave
							)
						)
						else
							tempStatus = false
						
						if tempStatus != undefined then
						(
							/*
							tempDirFilter = filterString exportPath "\\"
							tempDirString = ""
							for i = 1 to (tempDirFilter.count - 1) do
							(
								tempDirString += tempDirFilter[i] + "\\"
								makeDir tempDirString
							)
							*/
							
							cryTools.cryAnim.UI.main._f.checkExport #createFolder exportPath

							if $'Bip01 R ForeTwist'.parent != $'Bip01 R Forearm' then $'Bip01 R ForeTwist'.parent = $'Bip01 R Forearm'
							if $'Bip01 R ForeTwist'.parent != $'Bip01 L Forearm' then $'Bip01 L ForeTwist'.parent = $'Bip01 L Forearm'
						
							try ( cryTools.cryAnim.UI.main.loadSave._f.bakeMuscleBones() ) catch()
							
							
							local tempMode = getCommandPanelTaskMode()
							UtilityPanel.OpenUtility CryEngine2_Exporter
							
							local boneList = #()
							for i = 1 to lbExport.items.count do
								append boneList (getNodeByName lbExport.items[i])
							
							csexport.set_bone_list boneList
							
							local tempBool = (csexport.export.export_anim exportPath)
							while tempBool != OK do ( escapeEnable = true ; sleep 0.1 )
							
							tempFilterFilename = filterString exportPath "\\"
							
							printBool[2] = true
							
							setCommandPanelTaskMode mode:tempMode
							
							if cryTools.cryAnim._v.perforceDir != undefined then
							(
								case (cryTools.cryAnim.UI.main._f.getUI "Perforce" "radSaveExportAdd").state of
								(
									1: cryTools.cryAnim.base.perforce exportPath #add
									3: cryTools.cryAnim.base.perforce exportPath #checkForAdd
								)
							)
						)
						
						
						local tempArray = lbStatus.items
						local tempString = ""
						
						if printBool[1] != printBool[2] then
						(
							if printBool[1] == true then tempString = "S"
							if printBool[2] == true then tempString = "E"									
						)
						else
						(
							tempString = "SE"
						)
						
						append tempArray (tempString + " - " + tempFilename)
						lbStatus.items = tempArray
						lbStatus.selection = tempArray.count
						
						tempFileArray = lbStatusFilepath.items
						append tempFileArray filepath
						lbStatusFilepath.items = tempFileArray
						
					)
				)
			)
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.loadSaveRO.btnSaveExport.pressed" )
	)
	
	
	on lbStatus doubleClicked value do
	(
		try
		(
			try (destroyDialog cryTools.cryAnim.UI.main.loadSave.showSelectedFile) catch()
			
			rollout showSelectedFile "SelectedFile"
			(
				edittext edFilename "" width:(cryTools.cryAnim.UI.main.loadSave._f.getStatusExtents() + 40)
				edittext edFilepath "" width:(cryTools.cryAnim.UI.main.loadSave._f.getStatusExtents() + 40)			
				label labPerforce "Perforce Path:"
				edittext edFilepathPerforce "" width:(cryTools.cryAnim.UI.main.loadSave._f.getStatusExtents() + 40)
				
				on showSelectedFile open do
				(
					if cryTools.cryAnim._v.perforceDir == undefined then
					(
						labPerforce.visible = false
						edFilepathPerforce.visible = false
					)
					
					local tempSLB = cryTools.cryAnim.UI.main._f.getUI "Load / Save / Export" "lbStatus"
					local tempSFLB = cryTools.cryAnim.UI.main._f.getUI "Load / Save / Export" "lbStatusFilepath"
				
					local tempSize = getTextExtent tempSFLB.items[tempSLB.selection]
				
					cryTools.cryAnim.UI.main.loadSave.showSelectedFile.width = (cryTools.cryAnim.UI.main.loadSave._f.getStatusExtents() + 70)
				
					local tempString = filterString tempSLB.items[tempSLB.selection] " "
					local tempString = tempString[tempString.count]
					
					edFilename.text = tempString
					edFilepath.text = tempSFLB.items[tempSLB.selection]
					
					edFilepathPerforce.text = cryTools.cryAnim.base.perforce tempSFLB.items[tempSLB.selection] #localToDepot
				)
			)
			cryTools.cryAnim.UI.main.loadSave.showSelectedFile = showSelectedFile
			showSelectedFile = undefined
			
			createDialog cryTools.cryAnim.UI.main.loadSave.showSelectedFile pos:[(mouse.pos[1]),(mouse.pos[2] + 70)]
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.loadSaveRO.lbStatus.doubleClicked" )
	)
	
	
	
	
	on btnbatchProcess pressed do
	(
		try
		(
			cryTools.cryAnim.UI.batchProcess._f.callDialog()
		)
		catch ( logOutput "!!> Error loading batchProcess ms" )
	)	
)
logOutput "> Created loadSaveRO rollout"


try
(
	if cryTools.cryAnim.base.iniFile #get #multiRow == true then
		addSubRollout cryTools.cryAnim.UI.main.dialog.row2 loadSaveRO
	else
		addSubRollout cryTools.cryAnim.UI.main.dialog.row1 loadSaveRO
)
catch ( logOutput "!!> Error adding loadSaveRO to main dialog" )
loadSaveRO = undefined


logOutput ">> save.ms loaded"