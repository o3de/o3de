--###############################################################################
	--// rollout with elements to control models and items
--###############################################################################
rollout modelsRO "Models"
(		
	button btnLoadModel "Load Model" pos:[8,8] width:142 height:20 toolTip:"Loads often used models and shows dialog to edit them"
	
	groupBox gbItems " Items " pos:[2,35] width:153 height:50
	dropDownList ddItemSelect "" pos:[8,55] width:142 height:21
	
	
	
	on modelsRO open do
	(
		try
		(
			try ( if (cryTools.cryAnim.base.iniFile #get #rolloutStates) == true then (cryTools.cryAnim.UI.main._f.getUI "Models" "").open = cryTools.cryAnim.base.iniFile #get #modelsRO) catch()
			
			cryTools.cryAnim.UI.main.models._v.itemList = cryTools.cryAnim.UI.main.models._f.selectItem "" #getList
			
			local tempListArray = cryTools.cryAnim.UI.main.models._v.itemList
			local tempListArray2 = #()
			
			for i = 1 to tempListArray.count do
				tempListArray2[i] = tempListArray[i].name
			
			
			join tempListArray2 #("-------------------------------------------", "Edit Entries")
			
			ddItemSelect.items = tempListArray2
			
			
			if (local tempVar = cryTools.cryAnim.UI.main.models._f.selectItem "" #getIndex) != 0 then
				ddItemSelect.selection = tempVar
			
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.modelsRO.open" )
	)
	
	on modelsRO rolledUp value do
	(
		try
		(
			if (cryTools.cryAnim.base.iniFile #get #modelsRO) != value then
				cryTools.cryAnim.base.iniFile #set #modelsRO
			
			cryTools.cryAnim.UI.main._f.updateDialog()
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.modelsRO.rolledUp" )
	)
	
	
	on ddItemSelect selected value do
	(
		try
		(
			if value < (ddItemSelect.items.count - 1) then
			(
				local tempVar = cryTools.cryAnim.UI.main.models._f.selectItem value #set
				if tempVar == false then
					ddItemSelect.selection = 1
			)
			else
				ddItemSelect.selection = ddItemSelect.items.count
				
				
			if ddItemSelect.selection == ddItemSelect.items.count then
			(
				try ( destroyDialog cryTools.cryAnim.UI.main.models.editItemList ) catch()
				
				rollout editItemListRO "Edit Item List"
				(
					dotNetControl lbItems "System.Windows.Forms.ListView" pos:[1,1] height:185 width:440
					button btnSave "Save" pos:[8,195] height:20 width:80 toolTip:"Save item list"
					button btnAdd "Add" pos:[120,195] height:20 width:60 toolTip:"Adds new entry"
					button btnDelete "Delete" pos:[190,195] height:20 width:60 toolTip:"Deletes selected entry"
					button btnDeleteAll "Delete All" pos:[260,195] height:20 width:60 toolTip:"Clears whole list"
					button btnCancel "Cancel" pos:[350,195] height:20 width:80 toolTip:"Aborts dialog to edit item list"
					
				

					on editItemListRO open do
					(
						lbItems.GridLines = true			
						lbItems.AllowColumnReorder = true
						lbItems.View = lbItems.View.Details
						lbItems.LabelEdit = false
						lbItems.LabelWrap = true					
						lbItems.FullRowSelect = true
						lbItems.HideSelection = false
						lbItems.Sorting = lbItems.Sorting.Ascending
						
						lbItems.Columns.Add "ID"
						lbItems.Columns.Add "Name"
						lbItems.Columns.Add "External"
						lbItems.Columns.Add "Model"
						lbItems.Columns.Add "Reference"
						lbItems.Columns.Add "Parent"
						lbItems.Columns.Add "Rotation"
						lbItems.Columns.Add "Position"
						
						

						for i = 1 to cryTools.cryAnim.UI.main.models._v.itemList.count do
						(
							local lbItemsEntry = lbItems.Items.Add (i as String)
							lbItemsEntry.SubItems.Add cryTools.cryAnim.UI.main.models._v.itemList[i].name
							lbItemsEntry.SubItems.Add cryTools.cryAnim.UI.main.models._v.itemList[i].external
							
							local maxCount = cryTools.cryAnim.UI.main.models._v.itemList[i].model.count
							tempStringArray = #("","","","","")
							
							
							for f = 1 to maxCount do
							(
								tempStringArray[1] += cryTools.cryAnim.UI.main.models._v.itemList[i].model[f] + (if f < maxCount then ";" else "")
								tempStringArray[2] += cryTools.cryAnim.UI.main.models._v.itemList[i].reference[f] + (if f < maxCount then ";" else "")
								tempStringArray[3] += cryTools.cryAnim.UI.main.models._v.itemList[i].parent[f] + (if f < maxCount then ";" else "")
								tempStringArray[4] += cryTools.cryAnim.UI.main.models._v.itemList[i].rotation[f] as String + (if f < maxCount then ";" else "")
								tempStringArray[5] += cryTools.cryAnim.UI.main.models._v.itemList[i].position[f] as String + (if f < maxCount then ";" else "")
							)
							
							lbItemsEntry.SubItems.Add tempStringArray[1]
							lbItemsEntry.SubItems.Add tempStringArray[2]
							lbItemsEntry.SubItems.Add tempStringArray[3]
							lbItemsEntry.SubItems.Add tempStringArray[4]
							lbItemsEntry.SubItems.Add tempStringArray[5]
						)
						
						cryTools.cryAnim.UI.main.models._f.updateExtent()
						cryTools.cryAnim.UI.main.models._f.sortList()
					)
					
					
					on lbItems DoubleClick do
					(
													
						tempItem = lbItems.FocusedItem
						tempItem.checked = not tempItem.checked
																				
						if tempItem != undefined then
						(
							try ( destroyDialog cryTools.cryAnim.UI.main.models.editDetails ) catch()
							
							rollout entryDetailsRO "Entry Details"
							(
								label labID "ID :" pos:[8,10]
								label labName "Name :" pos:[8,30]
								label labExternal "External :" pos:[8,50]
								label labModel "Model :" pos:[8,70]
								label labReference "Reference :" pos:[8,90]
								label labParent "Parent :" pos:[8,110]
								label labRotation "Rotation :" pos:[8,130]
								label labPosition "Position :" pos:[8,150]
							
								edittext edID "" text:cryTools.cryAnim.UI.main.models.editItemList.lbItems.FocusedItem.text pos:[70,10] fieldWidth:300
								edittext edName "" text:(cryTools.cryAnim.UI.main.models.editItemList.lbItems.FocusedItem.SubItems.item 1).text pos:[70,30] fieldWidth:300
								edittext edExternal "" text:(cryTools.cryAnim.UI.main.models.editItemList.lbItems.FocusedItem.SubItems.item 2).text pos:[70,50] fieldWidth:300
								edittext edModel "" text:(cryTools.cryAnim.UI.main.models.editItemList.lbItems.FocusedItem.SubItems.item 3).text pos:[70,70] fieldWidth:300
								edittext edReference "" text:(cryTools.cryAnim.UI.main.models.editItemList.lbItems.FocusedItem.SubItems.item 4).text pos:[70,90] fieldWidth:300
								edittext edParent "" text:(cryTools.cryAnim.UI.main.models.editItemList.lbItems.FocusedItem.SubItems.item 5).text pos:[70,110] fieldWidth:300
								edittext edRotation "" text:(cryTools.cryAnim.UI.main.models.editItemList.lbItems.FocusedItem.SubItems.item 6).text pos:[70,130] fieldWidth:300
								edittext edPosition "" text:(cryTools.cryAnim.UI.main.models.editItemList.lbItems.FocusedItem.SubItems.item 7).text pos:[70,150] fieldWidth:300
								
								button btnSave "Save" pos:[40,180] height:20 width:80 toolTip:"Save item to item list"
								button btnSetOffset "Set Offset" pos:[150,180] height:20 width:80 toolTip:"Generates new offset of the selected item"
								button btnCancel "Cancel" pos:[260,180] height:20 width:80 toolTip:"Aborts edit dialog for the selected item"
								
								
								on btnSave pressed do
								(
									local tempItem = cryTools.cryAnim.UI.main.models.editItemList.lbItems.FocusedItem
									
									tempItem.text = edID.text
									(tempItem.SubItems.item 1).text = edName.text
									(tempItem.SubItems.item 2).text = edExternal.text
									(tempItem.SubItems.item 3).text = edModel.text
									(tempItem.SubItems.item 4).text = edReference.text
									(tempItem.SubItems.item 5).text = edParent.text
									(tempItem.SubItems.item 6).text = edRotation.text
									(tempItem.SubItems.item 7).text = edPosition.text
									
									cryTools.cryAnim.UI.main.models._f.updateExtent()
									cryTools.cryAnim.UI.main.models._f.sortList()
									
									destroyDialog cryTools.cryAnim.UI.main.models.entryDetails
								)
								
								
								on btnSetOffset pressed do
								(
									local tempItem = cryTools.cryAnim.UI.main.models.editItemList.lbItems.FocusedItem
									if tempItem != undefined then
									(
										local refArray = filterString (tempItem.SubItems.item 4).text ";"
										local parentArray = filterString (tempItem.SubItems.item 5).text ";"
										local rotString = ""
										local posString = ""
										
										for i = 1 to refArray.count do
										(
											
											
											local tempObj = (cryTools.cryAnim._f.createSnapshot object:(getNodeByName refArray[i]))[1]
											local tempParent = getNodeByName parentArray[i]
											
											if (tempObj != undefined) and (tempParent != undefined) then
											(
												if tempParent.classID[1] != 37157 then
												(
													rotString += (in coordsys tempObj tempParent.rotation) as String + (if i < refArray.count then ";" else "")
													posString += (in coordsys tempObj tempParent.pos) as String + (if i < refArray.count then ";" else "")
												)
											)
											
											try (delete tempObj)catch()
										)
										
										edRotation.text = rotString
										edPosition.text = posString
									)
									else
									(
										messageBox "No Item selected." title:"Reset Offset"
									)
									index = undefined
								)
								
								
								
								on btnCancel pressed do
								(	
									destroyDialog cryTools.cryAnim.UI.main.models.entryDetails
								)
							)
							cryTools.cryAnim.UI.main.models.entryDetails = entryDetailsRO
							entryDetailsRO = undefined
								
							createDialog cryTools.cryAnim.UI.main.models.entryDetails 385 210
							
						)
						tempItem = undefined
					)
					
					
					
					on btnAdd pressed do
					(
						local tempStringArray = (itemStruct name:"" external:"" model:"" reference:"" parent:"" rotation:"" position:"")
						local tempArray = #()
						
						
						local objArray = selectByName title:("Select Nodes to be added") showHidden:true filter:cryTools.cryAnim.UI.main.models._f.selectByNameFilterNode
						
						if objArray != undefined then
						(
							local objLink = #()
							
							for obj in objArray do
							(
								global tempObj = obj
								
								objLink = selectByName title:("Select " + obj.name + " Reference") showHidden:true single:true filter:cryTools.cryAnim.UI.main.models._f.selectByNameFilterParent
								if objLink != undefined then
								(
									objRotation = (in coordsys objLink obj.rotation) as String
									objPosition = (in coordsys objLink obj.position) as String
									
									objLink = objLink.name
								)
								else
								(
									objLink = ""
									objRotation = ""
									objPosition = ""
								)
								
								if obj.parent != undefined then
									objParent = obj.parent.name
								else
									objParent = ""
										
								append tempArray (itemStruct name:obj.name model:obj.name external:obj.name parent:objParent reference:objLink rotation:objRotation position:objPosition )
							)
							tempObj = undefined
						)
						
						if tempArray.count > 0 then
						(
							tempStringArray.name = tempArray[1].name
							tempStringArray.external = tempArray[1].external
							
							for i = 1 to tempArray.count do
							(
								tempStringArray.model += tempArray[i].model + (if i != tempArray.count then ";" else "")
								tempStringArray.reference += tempArray[i].reference as String + (if i != tempArray.count then ";" else "")
								tempStringArray.parent += tempArray[i].parent as String + (if i != tempArray.count then ";" else "")
								tempStringArray.rotation += tempArray[i].rotation as String + (if i != tempArray.count then ";" else "")
								tempStringArray.position += tempArray[i].position as String + (if i != tempArray.count then ";" else "")
							)
							
							local lbItemsEntry = lbItems.Items.Add ((lbItems.Items.count + 1) as String)
							lbItemsEntry.SubItems.Add tempStringArray.name
							lbItemsEntry.SubItems.Add tempStringArray.external
							lbItemsEntry.SubItems.Add tempStringArray.model
							lbItemsEntry.SubItems.Add tempStringArray.reference
							lbItemsEntry.SubItems.Add tempStringArray.parent
							lbItemsEntry.SubItems.Add tempStringArray.rotation
							lbItemsEntry.SubItems.Add tempStringArray.position
							
							cryTools.cryAnim.UI.main.models._f.updateExtent()
							cryTools.cryAnim.UI.main.models._f.sortList()
						)
					)
					
					
					
					on btnSave pressed do
					(
						local tempArray = #()
						local tempList = #()
						
						for i = 0 to (lbItems.Items.count - 1) do
						(
							local tempItemArray = #()
							local tempItem = lbItems.Items.item i
							for d = 1 to (tempItem.SubItems.count - 1) do
							(
								if (tempItem.SubItems.item d).text != "" then tempItemArray[d] = (tempItem.SubItems.item d).text else tempItemArray[d] = " "
							)
						
							append tempArray (itemStruct name:tempItemArray[1] external:tempItemArray[2] model:(filterString tempItemArray[3] ";") reference:(filterString tempItemArray[4] ";") parent:(filterString tempItemArray[5] ";") rotation:(filterString tempItemArray[6] ";") position:(filterString tempItemArray[7] ";"))
							
							for f = 1 to tempArray[tempArray.count].model.count do
							(
								tempArray[tempArray.count].rotation[f] = execute (tempArray[tempArray.count].rotation[f])
								tempArray[tempArray.count].position[f] = execute (tempArray[tempArray.count].position[f])
							)
						)
							
						cryTools.cryAnim.base.iniFile #set #items value:tempArray
						
						cryTools.cryAnim.UI.main.models._v.itemList = tempArray
						
						
						
						for i = 1 to cryTools.cryAnim.UI.main.models._v.itemList.count do
							append tempList cryTools.cryAnim.UI.main.models._v.itemList[i].name
							
						join tempList #("-------------------------------------------", "Edit Entries")
						
						(cryTools.cryAnim.UI.main._f.getUI "Models" "ddItemSelect").items = tempList
						
						destroyDialog cryTools.cryAnim.UI.main.models.editItemList

						
					)
					
					on btnDelete pressed do
					(
						if lbItems.FocusedItem != undefined then					
							lbItems.FocusedItem.remove()
						
						cryTools.cryAnim.UI.main.models._f.updateExtent()
						cryTools.cryAnim.UI.main.models._f.sortList()
					)
						
					on btnDeleteAll pressed do
					(
						lbItems.Items.clear()
					)
					
					
					on btnCancel pressed do
					(
						(cryTools.cryAnim.UI.main._f.getUI "Models" "ddItemSelect").selection = (cryTools.cryAnim.UI.main._f.getUI "Models" "ddItemSelect").items.count - 1
						destroyDialog cryTools.cryAnim.UI.main.models.editItemList
					)
					
					
					on editItemListRO close do
					(
						try
						(
							local tempVar = cryTools.cryAnim.UI.main.models._f.selectItem "" #getIndex
							
							if tempVar != 0 then
								(cryTools.cryAnim.UI.main._f.getUI "Models" "ddItemSelect").selection = tempVar
						)catch()
					)
				)
				cryTools.cryAnim.UI.main.models.editItemList = editItemListRO
				editItemListRO = undefined
				
				createDialog cryTools.cryAnim.UI.main.models.editItemList 443 220
			)
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.modelsRO.ddItemSelect.selected" )
	)

				
	on btnLoadModel pressed do
	(
		try
		(
			rcmenu loadModelRC
			(
				
				menUItem mi1 "None" checked:false filter:(fn temp = (cryTools.cryAnim.UI.main.models._f.setVisible 1))
				menUItem mi2 "None" checked:false filter:(fn temp = (cryTools.cryAnim.UI.main.models._f.setVisible 2))
				menUItem mi3 "None" checked:false filter:(fn temp = (cryTools.cryAnim.UI.main.models._f.setVisible 3))
				menUItem mi4 "None" checked:false filter:(fn temp = (cryTools.cryAnim.UI.main.models._f.setVisible 4))
				menUItem mi5 "None" checked:false filter:(fn temp = (cryTools.cryAnim.UI.main.models._f.setVisible 5))
				menUItem mi6 "None" checked:false filter:(fn temp = (cryTools.cryAnim.UI.main.models._f.setVisible 6))
				menUItem mi7 "None" checked:false filter:(fn temp = (cryTools.cryAnim.UI.main.models._f.setVisible 7))
				menUItem mi8 "None" checked:false filter:(fn temp = (cryTools.cryAnim.UI.main.models._f.setVisible 8))
				menUItem mi9 "None" checked:false filter:(fn temp = (cryTools.cryAnim.UI.main.models._f.setVisible 9))
				menUItem mi10 "None" checked:false filter:(fn temp = (cryTools.cryAnim.UI.main.models._f.setVisible 10))
				menUItem mi11 "None" checked:false filter:(fn temp = (cryTools.cryAnim.UI.main.models._f.setVisible 11))
				menUItem mi12 "None" checked:false filter:(fn temp = (cryTools.cryAnim.UI.main.models._f.setVisible 12))
				menUItem mi13 "None" checked:false filter:(fn temp = (cryTools.cryAnim.UI.main.models._f.setVisible 13))
				menUItem mi14 "None" checked:false filter:(fn temp = (cryTools.cryAnim.UI.main.models._f.setVisible 14))
				menUItem mi15 "None" checked:false filter:(fn temp = (cryTools.cryAnim.UI.main.models._f.setVisible 15))
				menUItem mi16 "None" checked:false filter:(fn temp = (cryTools.cryAnim.UI.main.models._f.setVisible 16))
				menUItem mi17 "None" checked:false filter:(fn temp = (cryTools.cryAnim.UI.main.models._f.setVisible 17))
				menUItem mi18 "None" checked:false filter:(fn temp = (cryTools.cryAnim.UI.main.models._f.setVisible 18))
				menUItem mi19 "None" checked:false filter:(fn temp = (cryTools.cryAnim.UI.main.models._f.setVisible 19))
				menUItem mi20 "None" checked:false filter:(fn temp = (cryTools.cryAnim.UI.main.models._f.setVisible 20))					
				

				seperator miSep checked:false
				menUItem miEdit "Edit Entries" checked:false


				on loadModelRC open do
				(
					tempPathArray = #()
					tempArray = cryTools.cryAnim.base.iniFile #get #models
					if tempArray != "" and tempArray != undefined then
					(							
						mi1.text = cryTools.cryAnim.UI.main.models._f.getEntries 1 tempArray
						mi2.text = cryTools.cryAnim.UI.main.models._f.getEntries 2 tempArray
						mi3.text = cryTools.cryAnim.UI.main.models._f.getEntries 3 tempArray
						mi4.text = cryTools.cryAnim.UI.main.models._f.getEntries 4 tempArray
						mi5.text = cryTools.cryAnim.UI.main.models._f.getEntries 5 tempArray
						mi6.text = cryTools.cryAnim.UI.main.models._f.getEntries 6 tempArray
						mi7.text = cryTools.cryAnim.UI.main.models._f.getEntries 7 tempArray
						mi8.text = cryTools.cryAnim.UI.main.models._f.getEntries 8 tempArray
						mi9.text = cryTools.cryAnim.UI.main.models._f.getEntries 9 tempArray
						mi10.text = cryTools.cryAnim.UI.main.models._f.getEntries 10 tempArray
						mi11.text = cryTools.cryAnim.UI.main.models._f.getEntries 11 tempArray
						mi12.text = cryTools.cryAnim.UI.main.models._f.getEntries 12 tempArray
						mi13.text = cryTools.cryAnim.UI.main.models._f.getEntries 13 tempArray
						mi14.text = cryTools.cryAnim.UI.main.models._f.getEntries 14 tempArray
						mi15.text = cryTools.cryAnim.UI.main.models._f.getEntries 15 tempArray
						mi16.text = cryTools.cryAnim.UI.main.models._f.getEntries 16 tempArray
						mi17.text = cryTools.cryAnim.UI.main.models._f.getEntries 17 tempArray
						mi18.text = cryTools.cryAnim.UI.main.models._f.getEntries 18 tempArray
						mi19.text = cryTools.cryAnim.UI.main.models._f.getEntries 19 tempArray
						mi20.text = cryTools.cryAnim.UI.main.models._f.getEntries 20 tempArray
					)
					
					cryTools.cryAnim._v.various[17] = tempArray
				)
				
				
				
				
				on mi1 picked do (cryTools.cryAnim.UI.main.models._f.loadModel cryTools.cryAnim._v.various[17][1])
				on mi2 picked do (cryTools.cryAnim.UI.main.models._f.loadModel cryTools.cryAnim._v.various[17][2])
				on mi3 picked do (cryTools.cryAnim.UI.main.models._f.loadModel cryTools.cryAnim._v.various[17][3])
				on mi4 picked do (cryTools.cryAnim.UI.main.models._f.loadModel cryTools.cryAnim._v.various[17][4])
				on mi5 picked do (cryTools.cryAnim.UI.main.models._f.loadModel cryTools.cryAnim._v.various[17][5])
				on mi6 picked do (cryTools.cryAnim.UI.main.models._f.loadModel cryTools.cryAnim._v.various[17][6])
				on mi7 picked do (cryTools.cryAnim.UI.main.models._f.loadModel cryTools.cryAnim._v.various[17][7])
				on mi8 picked do (cryTools.cryAnim.UI.main.models._f.loadModel cryTools.cryAnim._v.various[17][8])
				on mi9 picked do (cryTools.cryAnim.UI.main.models._f.loadModel cryTools.cryAnim._v.various[17][9])
				on mi10 picked do (cryTools.cryAnim.UI.main.models._f.loadModel cryTools.cryAnim._v.various[17][10])
				on mi11 picked do (cryTools.cryAnim.UI.main.models._f.loadModel cryTools.cryAnim._v.various[17][11])
				on mi12 picked do (cryTools.cryAnim.UI.main.models._f.loadModel cryTools.cryAnim._v.various[17][12])
				on mi13 picked do (cryTools.cryAnim.UI.main.models._f.loadModel cryTools.cryAnim._v.various[17][13])
				on mi14 picked do (cryTools.cryAnim.UI.main.models._f.loadModel cryTools.cryAnim._v.various[17][14])
				on mi15 picked do (cryTools.cryAnim.UI.main.models._f.loadModel cryTools.cryAnim._v.various[17][15])
				on mi16 picked do (cryTools.cryAnim.UI.main.models._f.loadModel cryTools.cryAnim._v.various[17][16])
				on mi17 picked do (cryTools.cryAnim.UI.main.models._f.loadModel cryTools.cryAnim._v.various[17][17])
				on mi18 picked do (cryTools.cryAnim.UI.main.models._f.loadModel cryTools.cryAnim._v.various[17][18])
				on mi19 picked do (cryTools.cryAnim.UI.main.models._f.loadModel cryTools.cryAnim._v.various[17][19])
				on mi20 picked do (cryTools.cryAnim.UI.main.models._f.loadModel cryTools.cryAnim._v.various[17][20])
				
				
				on miEdit picked do
				(
					rollout editModelListRO "Edit Model List"
					(
						dotNetControl lbModels "System.Windows.Forms.ListView" pos:[1,1] height:185 width:440
						button btnSave "Save" pos:[8,195] height:20 width:80 toolTip:"Save model list"
						button btnAdd "Add" pos:[120,195] height:20 width:60 toolTip:"Add new entry"
						button btnDelete "Delete" pos:[190,195] height:20 width:60 toolTip:"Deletes selected entry"
						button btnDeleteAll "Delete All" pos:[260,195] height:20 width:60 toolTip:"Clears whole list"
						button btnCancel "Cancel" pos:[350,195] height:20 width:80 toolTip:"Aborts dialog to edit model list"

						

						on editModelListRO open do
						(
							lbModels.GridLines = true			
							lbModels.AllowColumnReorder = true
							lbModels.View = lbModels.View.Details
							lbModels.LabelEdit = true
							lbModels.LabelWrap = true					
							lbModels.FullRowSelect = true
							lbModels.HideSelection = false
							lbModels.Sorting = lbModels.Sorting.Ascending
											
							tempArray = cryTools.cryAnim.base.iniFile #get #models
							if tempArray != "" and tempArray != undefined then
							(
								for i = 1 to tempArray.count do
								(
									local lbModelsEntry = lbModels.Items.Add tempArray[i].name											
									lbModelsEntry.SubItems.Add tempArray[i].path
								)
							)

												
							lbModels.Columns.Add "Name"
							lbModels.Columns.Add "Path"
							
							cryTools.cryAnim.UI.main.models._f.updateExtentModel()
						)
						
						on lbModels DoubleClick do
						(
							tempItem = lbModels.FocusedItem
							
							if tempItem != undefined then
							(								
								tempValue = getOpenFileName caption:"Select Model to open" filename:(lbModels.FocusedItem.SubItems.item 1).text types:"3ds max (*.max)|*.max"
								if (tempValue != false) and (tempValue != undefined) then
								(
									(lbModels.FocusedItem.SubItems.item 1).text = tempValue
									lbModels.FocusedItem.text = cryTools.cryAnim.base.perforce tempValue #getFilename
								)
							)
							else
							(
								lastIndex = lbModels.ListItems.count
								if lbModels.ListItems.count > 0 then
									tempPath = lbModels.ListItems[lastIndex].listSubItems[1].text
								else
									tempPath = maxFilePath + maxFileName

								tempValue = getOpenFileName caption:"Select Model to open" filename:tempPath types:"3ds max (*.max)|*.max"
								if (tempValue != false) and (tempValue != undefined) then
								(
									local tempEntry = lbModels.ListItems.Add text:(cryTools.cryAnim.base.perforce tempValue #getFilename)
									tempEntry.ListSubItems.Add text:tempValue
								)
							)

							cryTools.cryAnim.UI.main.models._f.updateExtentModel()
						)
						
						on btnAdd pressed do
						(
							lastIndex = (lbModels.Items.count - 1)
							if lbModels.Items.count > 0 then
								tempPath = ((lbModels.Items.item lastIndex).SubItems.item 1).text
							else
								tempPath = maxFilePath + maxFileName

							tempValue = getOpenFileName caption:"Select Model to open" filename:tempPath types:"3ds max (*.max)|*.max"
							if (tempValue != false) and (tempValue != undefined) then
							(
								local tempEntry = lbModels.Items.Add (cryTools.cryAnim.base.perforce tempValue #getFilename)
								tempEntry.SubItems.Add tempValue
							)
							
							cryTools.cryAnim.UI.main.models._f.updateExtentModel()
						)
						
						on btnSave pressed do
						(
							local tempArray = #()
							for i = 0 to (lbModels.Items.count - 1) do
								append tempArray (modelPathStruct name:(lbModels.Items.item i).text path:((lbModels.Items.item i).SubItems.item 1).text)
								
							cryTools.cryAnim.base.iniFile #set #models value:tempArray
							destroyDialog cryTools.cryAnim.UI.main.models.editModelList
						)
						
						on btnDelete pressed do
						(
							if lbModels.FocusedItem != undefined then
								lbModels.FocusedItem.remove()
						)
						
						on btnDeleteAll pressed do
						(
							lbModels.Items.clear()
						)
						
						on btnCancel pressed do
						(
							destroyDialog cryTools.cryAnim.UI.main.models.editModelList
						)
						
					)
					cryTools.cryAnim.UI.main.models.editModelList = editModelListRO
					editModelListRO = undefined
					
					createDialog cryTools.cryAnim.UI.main.models.editModelList 443 220
				)

			)
			cryTools.cryAnim.UI.main.models.loadModelRC = loadModelRC
			loadModelRC = undefined
			
			
			registerRightClickMenu cryTools.cryAnim.UI.main.models.loadModelRC

			popUpMenu cryTools.cryAnim.UI.main.models.loadModelRC pos:[(mouse.screenpos[1] - 10), (mouse.screenpos[2] - 10)]					
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.modelsRO.btnLoadModel.pressed" )
	)
)
logOutput "> Created modelsRO rollout"


--try
(
	if cryTools.cryAnim.base.iniFile #get #multiRow == true then
		addSubRollout cryTools.cryAnim.UI.main.dialog.row1 modelsRO
	else
		addSubRollout cryTools.cryAnim.UI.main.dialog.row1 modelsRO
)
--catch ( logOutput "!!> Error adding modelsRO to main dialog" )
modelsRO = undefined


logOutput ">> models9.ms loaded"