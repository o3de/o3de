-------------------------------------------------------------------------------
-- functions.ms
-- Version 2.9
-- functions file where almost all specific functions are defined
-- By: Mathias Lindner
--   eMail: devsupport@crytek.com
-------------------------------------------------------------------------------










--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim._f.getBipNames
----------------------------------------------------------------------------------------------------------------------------------
--			get the names of used links (used for non-default biped systems)
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim._f.getBipNames = function getBipNames index =
(
	try
	(
		if $Bip01 != undefined then
		(
			if $Bip01.classID[1] == 37157 then
			(
				case index of
				(
					/*Hand*/1:	( tempNode = biped.getNode $Bip01 #rArm link:4 )
					/*Foot*/2:	( tempNode = biped.getNode $Bip01 #rLeg link:3 )
					/*Toe0*/3:	( tempNode = biped.getNode $Bip01 #rToes link:1 )
					/*Finger*/4: ( return "Finger" )
				)
		
		
					--// stores the last part of the string
				tempFilterString = (filterString tempNode.name " ")[3]
			
				return tempFilterString
			)	
		)
		
		return undefined
	)
	catch ( logOutput "!!> Error in cryAnim._f.getBipNames" )
)
getBipNames = undefined
logOutput "> Created cryAnim._f.getBipNames function"






--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim._f.getBaseBip
----------------------------------------------------------------------------------------------------------------------------------
--			gets the root node of the biped hierarchy
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim._f.getBaseBip = function getBaseBip op:undefined =
(
	try
	(
		local baseBip = undefined
		try
		(
			baseBip = selection[1].controller.rootNode
		)
		catch
		(		
			for obj in Objects do
			(
				if obj.classID[1] == 37157 then
					baseBip = obj.controller.rootNode
			)
		)
			
		if baseBip == undefined then
		(
			if op == #load then
				messageBox "No Biped in Scene." title:"Load File"
			else
				print "No Biped in Scene"
		)
		
		return baseBip
	)
	catch ( logOutput "!!> Error in cryAnim._f.getBaseBip" )
)
getBaseBip = undefined
logOutput "> Created cryAnim._f.getBaseBip function"









--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim._f.getSelectedBipPart
----------------------------------------------------------------------------------------------------------------------------------
--			returns the biped part which is currently in selection
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim._f.getSelectedBipPart = function getSelectedBipPart =
(
	try
	(
			--// defines using "standard" constructor
		tempFilter = bipPartSt object:(objectSt selected:undefined parent:undefined child:undefined isPivot:false) part:(partNameSt name:undefined leftRight:undefined bodyPart:undefined) name:undefined  pivotSel:(pivotSelSt index:undefined pivotName:undefined position:undefined pivPoint:(pivPointSt name:undefined position:undefined) )

			--// gets selected biped object
		selectedBipPart = getCurrentSelection()
		
			--// stores the selected object in tempFilter	
		if selectedBipPart[1] != undefined then
			tempFilter.object.selected = selectedBipPart[1]
		else return undefined
		
		if tempFilter.object.selected.superClassID != 16 then
			return undefined
		
			--// filters the name using space (" ") for seperation
		tempFilterString = (filterString selectedBipPart[1].name " ")		
		-- check if selected part is child of bip01
		tempNodeParent = #()
		validBipedPart = false
		for i = 1 to 20 do
		(
			if tempNodeParent.count == 0 then
					--// initialise with the selected object
				append tempNodeParent tempFilter.object.selected
			else
					--// if the latest object is not undefined, then add the parent to the array
				if tempNodeParent[tempNodeParent.count] != undefined then
					append tempNodeParent tempNodeParent[tempNodeParent.count].parent
				--// if parent is undefined (end of parentList) then break the loop
			if tempNodeParent[tempNodeParent.count] == undefined then
				break
		)
		if (findItem tempNodeParent $Bip01 ) == 0 then
			return undefined
		else
				--// if Bip01 is in in the parent list, then set validBipedPart to true
			validBipedPart = true
		
				
			------// stores last part (body part, for example: "Foot" or "Hand" etc.) in .part
			----// necessary for resetRotation to get every possible biped part
			--// if there is more than 2 substrings (i.e. "Bip01 L Hand" or "Bip01 R Foot")		
		if tempFilterString.count > 2 then
		(
			tempFilter.part.leftRight = tempFilterString[2]
			tempFilter.part.bodyPart = tempFilterString[3]
			tempFilter.part.name = tempFilterString[2] + tempFilterString[3]
		)
			--// if there is less than 3 substrings (i.e. "Bip01 Spine" or "Bip01 Head"
		else
		(
			tempFilter.part.leftRight = ""
			tempFilter.part.bodyPart = tempFilterString[tempFilterString.count]
			tempFilter.part.name = tempFilterString[tempFilterString.count]
		)
		
		if findString tempFilter.part.bodyPart "Finger" != undefined then
			tempFilter.part.bodyPart = substring tempFilter.part.bodyPart 1 6
			
			--// stores the name of the selected object
		tempFilter.name = tempFilter.object.selected.name
		
		ok = false
		tempBipNames = #()
		
		for i = 1 to 3 do
			tempBipNames[i] = cryTools.cryAnim._f.getBipNames i

		
		function getObjectParent baseObject = 
		(
			try
			(
				for i = 1 to 20 do
				(
					if baseObject != undefined then
					(
						if findString baseObject.parent.name "Hand" != undefined  then
							return baseObject.parent
						else
							baseObject = baseObject.parent
					)				
				)
			)
			catch ()
			
			return undefined
		)
		
		
		case tempFilter.part.bodyPart of
		(		
			/*Hand*/(cryTools.cryAnim._f.getBipNames 1): ( ok = true )
			/*Finger*/(cryTools.cryAnim._f.getBipNames 4): ( ok = true
					  tempFilter.part.bodyPart = (cryTools.cryAnim._f.getBipNames 1)
					  tempFilter.part.name = tempFilter.part.leftRight + tempFilter.part.bodyPart
					  tempFilter.object.child = tempFilter.object.selected
					  tempFilter.object.selected = getObjectParent tempFilter.object.child
					  tempFilter.name = tempFilter.object.selected.name
					)
			/*Foot*/(cryTools.cryAnim._f.getBipNames 2):  ( ok = true )
			/*Toe0*/(cryTools.cryAnim._f.getBipNames 3): ( ok = true 
						  --// only necessary for resetRotation() to check, if the toe or finger is selected
					  tempFilter.part.bodyPart = (cryTools.cryAnim._f.getBipNames 2)
					  tempFilter.part.name = tempFilter.part.leftRight + tempFilter.part.bodyPart
					  tempFilter.object.child = tempfilter.object.selected
					  tempFilter.object.selected = tempFilter.object.selected.parent
					  tempFilter.name = tempFilter.object.selected.name
					)
		)
		
		
		if tempFilter.object.selected != $Bip01 then
			tempFilter.object.parent = tempFilter.object.selected.parent
			
		if ok == true then
			tempFilter.object.isPivot = true
		
		if tempFilter.object.isPivot == true then
		(
			try (tempFilter.pivotSel.index = execute ("cryTools.cryAnim.align._v.pivotLast." + tempFilter.part.name + ".index") )catch( print "Error getting pivotSel.index" )
			try (tempFilter.pivotSel.pivotName = execute ("cryTools.cryAnim.align._v.pivotLast." + tempFilter.part.name + ".pivotName") ) catch(print "Error getting pivotSel.pivotName" )
			try (tempFilter.pivotSel.position = execute ("cryTools.cryAnim.align._v.pivotLast." + tempFilter.part.name + ".position") ) catch( print "Error getting pivotSel.position" )
			try (tempFilter.pivotSel.pivObject = execute ("cryTools.cryAnim.align._v.pivotLast." + tempFilter.part.name + ".pivObject") ) catch( print "Error getting pivotSel.pivObject" )
			try (tempFilter.pivotSel.pivPoint.name = execute ("cryTools.cryAnim.align._v.pivotLast." + tempFilter.part.name + ".pivPoint.name") ) catch( print "Error getting pivotSel.pivPoint.name" )
			try (tempFilter.pivotSel.pivPoint.position = execute ("cryTools.cryAnim.align._v.pivotLast." + tempFilter.part.name + ".pivPoint.position") ) catch( print "Error getting pivotSel.pivPoint.position" )
		)
				
		
		if validBipedPart == true then
			return tempFilter
		else
				--// if no biped part for pivot selection is selected, then return
			return undefined
	)
	catch ( logOutput "!!> Error in cryAnim._f.getSelectedBipPart" )
)
getSelectedBipPart = undefined
logOutput "> Created cryAnim._f.getSelectedBipPart function"








--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim._f.getSelectedBipPart
----------------------------------------------------------------------------------------------------------------------------------
--			returns the biped part which is currently in selection
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim._f.getPivotPointPos = function getPivotPointPos index baseNode =
(
	try
	(
		vertPos = undefined
		
		selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()		
		
		
		case (selectedBipPart.part.bodyPart) of
		(
			
				--// checks what biped part is selected
			/*Foot*/(cryTools.cryAnim._f.getBipNames 2):
			(
				try ( child = baseNode.children[1] ) catch()
				if child == undefined then
					child = baseNode
				
				
				case (index) of
				(
						--// returns the pivot position values of the selected biped object
					1: vertPos = (in coordsys world baseNode.transform.pos)
					2: vertPos = (in coordsys world (getVert baseNode 7))
					3: vertPos = (in coordsys world ( ((getVert baseNode 6) + (getVert baseNode 7)) / 2) )
					4: vertPos = (in coordsys world (getVert baseNode 6))
					5: vertPos = (in coordsys world (getVert baseNode 3))
					6: vertPos = (in coordsys world ( ((getVert baseNode 2) + (getVert baseNode 3)) / 2) )
					7: vertPos = (in coordsys world (getVert baseNode 2))
					8: vertPos = (in coordsys world (getVert child 2))
					9: vertPos = (in coordsys world ( ((getVert child 2) + (getVert child 6)) / 2) )
					10: vertPos = (in coordsys world (getVert child 6))
				)
			)
			/*Hand*/(cryTools.cryAnim._f.getBipNames 1):
			(
				local childArray = cryTools.cryAnim._f.getChildren selectedBipPart.object.selected
				
				
				case (index) of
				(				
						--// STUPID EDIT POLY --
					1: vertPos = (in coordsys world baseNode.transform.pos )
					2: vertPos = (in coordsys world (getVert baseNode 8))
					3: vertPos = (in coordsys world ( ((getVert baseNode 8) + (getVert baseNode 4)) / 2) )
					4: vertPos = (in coordsys world (getVert baseNode 4))
					5: vertPos = (in coordsys world (getVert baseNode 7))
					6: vertPos = (in coordsys world ( ((getVert baseNode 7) + (getVert baseNode 3)) / 2) )
					7: vertPos = (in coordsys world (getVert baseNode 3))
				)
				
				
				if index > 7 then
					vertPos = (in coordsys world ( ((getVert childArray[index - 7] 7) + (getVert childArray[index - 7] 3)) / 2) )
			)	
		)
			
		return vertPos
	)
	catch ( logOutput "!!> Error in cryAnim._f.getPivotPointPos" )
)
getPivotPointPos = undefined
logOutput "> Created cryAnim._f.getPivotPointPos function"








--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim._f.getNearestSnapshot
----------------------------------------------------------------------------------------------------------------------------------
--			gets the nearest snapshot of the selected object in the scene
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim._f.getNearestSnapshot = function getNearestSnapshot =
(
	try
	(
		local tempSnapList = #()
		local usedSnapshot = undefined
		local selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()
		
		
		pivotDistance = pivotDistanceStruct()		
		
		
		if selectedBipPart != undefined then
		(
			if selectedBipPart.object.isPivot == true then
			(	
					--// tries to get the index the last used pivot
				local pivotIndex = selectedBipPart.pivotSel.index
					--// if not found, pivot will be set to 1
				if pivotIndex == undefined then pivotIndex = 1				
					
				
					--// snapshot function does not work with fingers, in this case the pivot point will be used
				if selectedBipPart.part.bodyPart == "Hand" then
					if selectedBipPart.pivotSel.pivObject != undefined then
						if findString selectedBipPart.pivotSel.pivObject "Finger" != undefined then
							return undefined
							
				addModifier selectedBipPart.object.selected (edit_mesh())
					--// gets the pivot position of the biped object
				local bipPivotPos = (cryTools.cryAnim._f.getPivotPointPos pivotIndex selectedBipPart.object.selected)
				deleteModifier selectedBipPart.object.selected 1

		
					
				for i = 1 to Objects.count do
				(
				
					if pivotIndex <= 7 then
							--// gets all available snapshots and stores it temporary in tempSnap
						local tempSnap = getNodeByName (selectedBipPart.name + "0" + i as String)					
					else
							--// gets all available snapshot toes and stores it temporary in tempSnap					
						local tempSnap = getNodeByName (selectedBipPart.object.selected.children[1].name + i as String)


					if tempSnap != undefined then
					(					
							--// stores founded snapshots in tempSnapList
						append tempSnapList tempSnap.name					
						
							--// gets pivot position of the snapshot
						local snapshotPivotPos = (cryTools.cryAnim._f.getPivotPointPos pivotIndex tempSnap)
						
						
						if pivotDistance.distance == undefined then
						(
								----// for initialisation
								--// gets the distance between the pivot of snapshot and biped object and stores it in pivotDistance
							pivotDistance.distance = distance bipPivotPos snapshotPivotPos
							pivotDistance.name = tempSnap.name
						)
						else
						(
								--// gets the distance from snapshot and biped object pivot
							local tempDistance = (distance bipPivotPos snapshotPivotPos)
								--// checks, which distance is smaller and stores the distance and name of the snapshot in pivotDistance
							if pivotDistance.distance > tempDistance then
							(
								pivotDistance.distance = distance bipPivotPos snapshotPivotPos
								pivotDistance.name =  tempSnap.name
							)
						)
					)					
				)
				
				if tempSnapList.count > 0 then
					return (getNodeByName pivotDistance.name)			
			)
		)
			--// otherwise undefined
		return undefined
	)
	catch ( logOutput "!!> Error in cryAnim._f.getNearestSnapshot" )
)
getNearestSnapshot = undefined
logOutput "> Created cryAnim._f.getNearestSnapshot function"









--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim._f.moveBipPartPivPoint
----------------------------------------------------------------------------------------------------------------------------------
--			moves node to the pivot point
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim._f.moveBipPartPivPoint = function moveBipPartPivPoint =
(
	try
	(
		if (selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart() ) != undefined then
		(
			if selectedBipPart.object.isPivot == true then
			(
				if selectedBipPart.pivotSel.pivPoint.position != undefined then
				(
					selectedObject = selectedBipPart.object.selected
					
						
					addModifier selectedBipPart.object.selected (edit_mesh())
					bipPivotPos = ( cryTools.cryAnim._f.getPivotPointPos selectedBipPart.pivotSel.index selectedBipPart.object.selected )
					deleteModifier selectedBipPart.object.selected 1
					
					offsetPos = (selectedBipPart.pivotSel.pivPoint.position - bipPivotPos)
					tempRot = selectedObject.transform.rotation as eulerangles 
					in coordsys world (move selectedObject offsetPos)
					
					if selectedBipPart.part.bodyPart == "Hand" then
						biped.setTransform selectedObject #rotation tempRot true

					
					return true
				)
			)		
		)
		return false
	)
	catch ( logOutput "!!> Error in cryAnim._f.moveBipPartPivPoint" )
)
moveBipPartPivPoint = undefined
logOutput "> Created cryAnim._f.moveBipPartPivPoint function"








--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim._f.moveRotBipPartPivPoint
----------------------------------------------------------------------------------------------------------------------------------
--			moves and rotates node to pivot point
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim._f.moveRotBipPartPivPoint = function moveRotBipPartPivPoint =
(
	try
	(
		selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()
		if selectedBipPart != undefined then
		(
			if selectedBipPart.object.isPivot == true then
			(
				if selectedBipPart.pivotSel.pivPoint.name != undefined then
				(
					--try
					--(
						local tempString = "pivSnap" + (substring selectedBipPart.pivotSel.pivPoint.name 9 selectedBipPart.pivotSel.pivPoint.name.count)
						biped.setTransform selectedBipPart.object.selected #rotation (getNodeByName tempString).transform.rotation true
						cryTools.cryAnim._f.moveBipPartPivPoint()
						return true
					--)catch()
				)
			)
		)
		else
			return undefined
		
		return false
	)
	catch ( logOutput "!!> Error in cryAnim._f.moveRotBipPartPivPoint" )
)
moveRotBipPartPivPoint = undefined
logOutput "> Created cryAnim._f.moveRotBipPartPivPoint function"









--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim._f.moveBipPart
----------------------------------------------------------------------------------------------------------------------------------
--			moves node to the pivot of the nearest snapshot
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim._f.moveBipPart = function moveBipPart = 
(
	try
	(
		pivotIndex = undefined
		selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()
		if selectedBipPart != undefined then
		(
			if selectedBipPart.object.isPivot == true then
			(
					--// if last used pivot is undefined then start selection tool
				if selectedBipPart.pivotSel.index == undefined then
				(	
					pivotIndex = 1
				)
				else
				(
						--// gets the initial letter of the pivot name ("e" or "p" (edge or point))
					case (substring selectedBipPart.pivotSel.pivotName 1 1) of
					(
						"p":
						(
							pivotIndex = selectedBipPart.pivotSel.index
						)
						"e":
						(
							angleY = 0.0
							angleZ = 0.0
							
								--// due to different order of the edges the pivot will be choosen for the edge
							case selectedBipPart.pivotSel.index of
							(
								1: pivotIndex = 2
								2: pivotIndex = 5
								3: pivotIndex = 7
								4: pivotIndex = 4
							)
							
								--// gets nearest snapshot and applys the transform matrix on the variable
							if (tempSnapMatrix = cryTools.cryAnim._f.getNearestSnapshot() ) != undefined then
							(							
								tempSnapMatrix = tempSnapMatrix.transform
									
									--// gets the transform matrix of the selected object
								tempBipMatrix = selectedBipPart.object.selected.transform
								
									--// which edges are selected
								if mod selectedBipPart.pivotSel.index 2 == 0.0 then
								(
										--// 2 or 4 is selected
									angleY = 0.0
										--// gets angle of row2			
									angleZ = acos ( dot (normalize [tempBipMatrix.row2.x, tempBipMatrix.row2.y, tempBipMatrix.row2.z]) (normalize [tempBipMatrix.row2.x, tempBipMatrix.row2.y, 0]))
										--// if row2 is smaller than 0 (rotated downward)
									if tempBipMatrix.row2.z < 0 then
											--// stores negative value
										angleZ -= angleZ*2					
								)
								else
								(
										--// 1 or 3 is selected
										--// gets angle of row3
									angleY = acos ( dot (normalize [tempBipMatrix.row3.x, tempBipMatrix.row3.y, tempBipMatrix.row3.z]) (normalize [tempBipMatrix.row3.x, tempBipMatrix.row3.y, 0]))
									angleZ = 0.0
										--// if row3 is bigger than 0 (rotated to the right)
									if tempBipMatrix.row3.z > 0 then
											--// stores negative value
										angleY -= angleY*2
								)
									--// creates new matrix with the rotation of the snapshot, but transition of the biped part
								newMatrix = matrix3 tempSnapMatrix.row1 tempSnapMatrix.row2 tempSnapMatrix.row3 tempBipMatrix.row4
									--// applys new transformation matrix to the selected biped part
								selectedBipPart.object.selected.transform = newMatrix
								
									--// rotates the selected biped part with the angle of the original rotation
								in coordsys local rotate selectedBipPart.object.selected (eulerangles 0 angleY angleZ )								
							)
						)
					)
				)			
				
				addModifier selectedBipPart.object.selected (edit_mesh())
					--// gets pivot position of the biped object
				local bipPivotPos = ( cryTools.cryAnim._f.getPivotPointPos pivotIndex  selectedBipPart.object.selected )
				deleteModifier selectedBipPart.object.selected 1
				
					--// gets nearest snapshot
				local tempSnap = cryTools.cryAnim._f.getNearestSnapshot()			
				
					--// if snapshot is not available or in the scene, then return false
				if tempSnap == undefined then return false
					--// gets pivot position of the nearest snapshot
				local snapshotPivotPos = (cryTools.cryAnim._f.getPivotPointPos pivotIndex tempSnap )
				
					--// generates the offset for moving the biped object to the new snapshot location
				local bipPivotOffset = snapshotPivotPos - bipPivotPos
					--// moves biped object to new location
				local tempRot = $.transform.rotation as eulerangles
				in coordsys world (move $ bipPivotOffset)
				
				if selectedBipPart.part.bodyPart == "Hand" then
					biped.setTransform $ #rotation tempRot true
				

				return true
			)
			
			return false
		)
	)
	catch ( logOutput "!!> Error in cryAnim._f.moveBipPart" )
)
moveBipPart = undefined
logOutput "> Created cryAnim._f.moveBipPart function"










--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim._f.moveRotBipPart
----------------------------------------------------------------------------------------------------------------------------------
--			moves and rotates node to the nearest snapshots pivot
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim._f.moveRotBipPart = function moveRotBipPart =
(
	try
	(
			--// get selected object and check if not unselected
		pivotIndex = undefined
		
		if ( selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart() ) != undefined then
		(
			if selectedBipPart.object.isPivot == true then
			(
				if selectedBipPart.pivotSel.index == undefined then
					pivotIndex = 1
				else
					pivotIndex = selectedBipPart.pivotSel.index
			
					--// gets nearest snapshot
				tempSnap = cryTools.cryAnim._f.getNearestSnapshot()
				if tempSnap != undefined then
				(
						--// gets the rotation of snapshot
					tempRot = tempSnap.rotation
						--// inverts the angle of rotation
					tempRot.w = tempRot.w - (tempRot.w * 2)
						--// checks, if the toe pivot is selected
					if pivotIndex <= 7 then
					(
							--// if available, then apply rotation of the snapshot to the selected biped object
						biped.setTransform selectedBipPart.object.selected #rotation tempRot true
					)
					else
					(
							--// if toe is selected, then apply rotation to the toe (child of foot)
						biped.setTransform selectedBipPart.object.selected.children[1] #rotation tempRot true
					)
				)
			)
		)
			--// tries to translate the biped object to the snapshot
		if cryTools.cryAnim._f.moveBipPart() == true then	
			return true
		else
			return false
	)
	catch ( logOutput "!!> Error in cryAnim._f.moveRotBipPart" )
)
moveRotBipPart = undefined
logOutput "> Created cryAnim._f.moveRotBipPart function"









--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim._f.moveToFloor
----------------------------------------------------------------------------------------------------------------------------------
--			moves the selected foot to the floor
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim._f.moveToFloor = function moveToFloor value:undefined getHeight:undefined =
(
	try
	(
		if ( selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart() ) != undefined then
		(
			if selectedBipPart.object.isPivot == true then
			(
					--// creates a matrix to rotate the selected object 
				tempMatrix = selectedBipPart.object.selected.transform
				
					--// creates the height of the foot out of the distance between the heighest vert (5) and lowest vert (6)
				if value == undefined then
				(
					addModifier selectedBipPart.object.selected (edit_mesh())
					tempHeight = distance (getVert selectedBipPart.object.selected 5) (getVert selectedBipPart.object.selected 6)
					deleteModifier selectedBipPart.object.selected 1
				)
				else
					tempHeight = value
				
				if getHeight != undefined then
					return tempHeight
				
					--// sets new rotation to the matrix, so that it keeps the horizontal rotation
				tempMatrix = matrix3 [0, 0, -1] [tempMatrix.row2.x, tempMatrix.row2.y, 0] [tempMatrix.row3.x, tempMatrix.row3.y, 0] [tempMatrix.row4.x, tempMatrix.row4.y, tempHeight]
					--// sets the foot to the floor				
				selectedBipPart.object.selected.transform = tempMatrix
				biped.setTransform selectedBipPart.object.selected #pos [tempMatrix.pos[1], tempMatrix.pos[2], tempMatrix.pos[3]] true
				
				return true
			)
			else
				return undefined
		)
	)
	catch ( logOutput "!!> Error in cryAnim._f.moveToFloor" )
)
moveToFloor = undefined
logOutput "> Created cryAnim._f.moveToFloor function"











--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim._f.resetRotation
----------------------------------------------------------------------------------------------------------------------------------
--			applies the parent rotation to the child
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim._f.resetRotation = function resetRotation =
(
	try
	(
		if selection.count > 0 then
		(
			for obj in selection do
			(
				if obj.classID[1] == 37157 then
				(
					if obj.parent != undefined then
					(
						local newRotation = obj.parent.transform.rotation
						
						if findString obj.name "Toe" != undefined then
							newRotation = (matrix3 obj.parent.transform.row2 -obj.parent.transform.row1 obj.parent.transform.row3 obj.transform.row4).rotation
						if findString obj.name "Hand" != undefined then
							newRotation = (matrix3 obj.parent.transform.row1 -obj.parent.transform.row3 obj.parent.transform.row2 obj.transform.row4).rotation
						
						biped.setTransform obj #rotation newRotation true
					)
				)
				else
				(
					if obj.parent != undefined then
					(
						local newMatrix = obj.transform
						
						newMatrix.rotation = obj.parent.transform.rotation
						newMatrix.position = obj.transform.position
						obj.transform = newMatrix
					)
				)
			)
		)
	)
	catch ( logOutput "!!> Error in cryAnim._f.resetRotation" )
)
resetRotation = undefined
logOutput "> Created cryAnim._f.resetRotation function"








--/////////////////////////////////////////////////////////////////////////////////////////////////////////////
--//################################################################
--//		DEF:cryTools.cryAnim._f.rotate180
--//--------------------------------------------------------------------------------------------------------------------------------
--//			rotates the biped around 180°
--//################################################################
--/////////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim._f.rotateAnim = function rotateAnim animDir range:undefined =
(
	try
	(
		if classOf animDir == Integer or classOf animDir == Float then
		(
			if selection.count > 0 then
				local usedNode = (getCurrentSelection())[1]
			else
				if $Bip01 != undefined then
					usedNode = $Bip01
				
			if usedNode != undefined then
			(
				if usedNode.classID[1] == 37157 then
				(
					usedNode = usedNode.controller.rootNode
					local undoString = "rotate " + animDir as String
					undo undoString on
					(
						if range != undefined then
						(
							at time animationRange.start
							(
								with animate off
								(
									usedNode.controller.moveAllMode = true
									rotate usedNode (eulerangles 0 0 animDir)
									biped.collapseMoveAllMode usedNode.controller
									usedNode.controller.moveAllMode = false
								)
							)
							
							
							
							if $Locator_Locomotion != undefined then
							(
								if $Locator_Locomotion.classID[1] == 37157 then
								(
									at time animationRange.start
									(
										with animate off
										(
											rotRoot = box pos:usedNode.transform.pos name:"rotRoot"	
											locClone = (cryTools.cryAnim._f.createSnapshot object:$Locator_Locomotion)[1]
											locClone.parent = rotRoot
										)
									)
									
									for i = animationRange.start to animationRange.end do
									(
										with animate on
											at time i	
												locClone.transform = $Locator_Locomotion.transform
									)
									
									with animate off
									(
										at time animationRange.start
											rotate rotRoot (eulerangles 0 0 animDir)
									)
									
									
									with redraw off
									(
										for i = animationRange.start to animationRange.end do
										(
											sliderTime = i
											(
												with animate on
												(
													biped.setTransform $Locator_Locomotion #rotation locClone.transform.rotation true
													biped.setTransform $Locator_Locomotion #pos locClone.transform.pos true
												)
											)
										)
									)
								)
							)
							
							sliderTime = animationRange.start + 1
							sliderTime = animationRange.start
						)
						else
						(
							with animate on
								rotate usedNode (eulerangles 0 0 animDir)
							
							if $Locator_Locomotion != undefined then
							(
								if $Locator_Locomotion.classID[1] == 37157 then
								(
									with animate off
									(
										rotRoot = box pos:usedNode.transform.pos name:"rotRoot"	
										locClone = (cryTools.cryAnim._f.createSnapshot object:$Locator_Locomotion)[1]
										locClone.parent = rotRoot
										rotate rotRoot (eulerangles 0 0 animDir)
									)
									
									with animate on
									(
										biped.setTransform $Locator_Locomotion #rotation locClone.transform.rotation true
										biped.setTransform $Locator_Locomotion #pos locClone.transform.pos true
									)
								)
							)
						)
					)
				)
			)
		)
		try 
		(
			delete rotRoot
			delete locClone
		)catch()
	)
	catch ( logOutput "!!> Error in cryAnim._f.rotateAnim" )
)
rotateAnim = undefined
logOutput "> Created cryAnim._f.rotateAnim function"









--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim._f.bipToZero2
----------------------------------------------------------------------------------------------------------------------------------
--			moves Bip01 to [0,0,z]
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim._f.bipToZero2 = function bipToZero2 =
(
	try
	(
		biped.setTransform $Bip01 #pos [0,0,($Bip01.transform.pos[3])] true
	)
	catch ( logOutput "!!> Error in cryAnim._f.bipToZero2" )
)
bipToZero2 = undefined
logOutput "> Created cryAnim._f.bipToZero2 function"









--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim._f.bipToZero3
----------------------------------------------------------------------------------------------------------------------------------
--			moves Bip01 to [0,0,0]
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim._f.bipToZero3 = function bipToZero3 =
(
	try
	(
		biped.setTransform $Bip01 #pos [0,0,0] true
	)
	catch ( logOutput "!!> Error in cryAnim._f.bipToZero3" )
)
bipToZero3 = undefined
logOutput "> Created cryAnim._f.bipToZero3 function"









--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim._f.setTrajectories
----------------------------------------------------------------------------------------------------------------------------------
--			shows trajectory of selected objects
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim._f.setTrajectories = function setTrajectories dir =
(
	try
	(
		selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()
		
		case dir of
		(
			#show:
			(
				for obj in $selection do
					setTrajectoryOn obj true
				
				return true
			)
			#hide:
			(
				for obj in Objects do
					setTrajectoryOn obj false
					
				return true
			)
			#checkHide:
			(			
				for obj in Objects do
					if (getTrajectoryOn obj) == true then
						return true

				return false
			)
		)
	)
	catch ( logOutput "!!> Error in cryAnim._f.setTrajectories" )
)
setTrajectories = undefined
logOutput "> Created cryAnim._f.setTrajectories function"










--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim._f.resetLocator
----------------------------------------------------------------------------------------------------------------------------------
--			resets locator to [0,0,0] Y+/Y-
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim._f.resetLocator = function resetLocator forceDir:undefined =
(
	try
	(
		if $Locator_Locomotion != undefined then
		(
			try
			(			
				biped.deleteKeys $'Locator_Locomotion'.controller #allKeys		
				biped.addNewKey $'Locator_Locomotion'.controller animationRange.start
			
				propKey = biped.getKey $'Locator_Locomotion'.controller 1

				propKey.posSpace = 0
				propKey.rotSpace = 0
				
				sliderTime = animationRange.start
				
				bipRot = $Bip01.transform.rotation as eulerangles
				
				biped.setTransform $'Locator_Locomotion' #pos (in coordsys world [0,0,0]) true
				
				if forceDir == undefined then
				(
					if bipRot.z > 0 and bipRot.z < 180 then
					(
							--// new, proper character rotation
						biped.setTransform $'Locator_Locomotion' #rotation (quat 0 0 0 1) true
					)
					else
					(
							--// used for Crysis but old character rotation
						biped.setTransform $'Locator_Locomotion' #rotation (quat 0 0 -1 0) true
					)
				)
				else
					biped.setTransform $'Locator_Locomotion' #rotation (quat 0 0 -1 0) true
					
				local tempItem = getCurrentSelection()
				if findItem tempItem $Locator_Locomotion == 0 then
					select $'Locator_Locomotion'
				
				return true
			)
			catch( return false)
		)
		else
			print "No Locator in Scene"
	)
	catch ( logOutput "!!> Error in cryAnim._f.resetLocator" )
)
resetLocator = undefined
logOutput "> Created cryAnim._f.resetLocator function"











--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim._f.moveToBodyMass
----------------------------------------------------------------------------------------------------------------------------------
--			moves the locator to the body mass
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim._f.moveToBodyMass = function moveToBodyMass =
(
	try
	(
		function getMaxValue root index =
		(	
			if root != undefined then
			(
				maxValue = #(index, root.transform.pos[index], root.transform.pos[index])	
				
					for obj in root.children do
						if obj.isHidden == false and obj.isFrozen == false then
							maxValue = cryTools.cryAnim._f.getChildExtent obj maxValue
					
				return maxValue
			)
		)



		partPos = [0,0,0]	

		for i = 1 to 3 do
		(
			local newPos = getMaxValue $'Bip01 Spine' i
			newPos = (newPos[2] + newPos[3]) / 2
			partPos[i] = newPos
		)

		
		diffPos = ($'Bip01 Pelvis'.transform.pos - $'Bip01 Spine3'.transform.pos) / 2
		bodyPos = $'Bip01 Pelvis'.transform.pos - diffPos

		
		partPos = partPos * 1/3
		bodyPos = bodyPos * 2/3
		
		finalPos = partPos + bodyPos
		finalPos[3] = 0
		
		biped.setTransform $Locator_Locomotion #pos finalPos true
		
		return true
	)
	catch ( logOutput "!!> Error in cryAnim._f.moveToBodyMass" )
)
moveToBodyMass = undefined
logOutput "> Created cryAnim._f.moveToBodyMass function"








--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim._f.getChildren
----------------------------------------------------------------------------------------------------------------------------------
--			gets all children of a node
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim._f.getChildren = function getChildren baseObject =
(
	try
	(
		function childToArray childArray =
		(
			local tempArray = #()
			for i = 1 to childArray.count do
			(
				if childArray[i].classID[1] == 37157 then
					if childArray[i].isHidden == false and childArray[i].isFrozen == false then
						append tempArray childArray[i]
			)
			
			return tempArray
		)
		
		
		
		local baseArray = childToArray baseObject.children

		for obj in baseArray do
		(
			if obj.children.count != 0 then
			(
				local tempArray = childToArray obj.children
				join baseArray tempArray
			)
		)			
				
		return baseArray
	)
	catch ( logOutput "!!> Error in cryAnim._f.getChildren" )
)
getChildren = undefined
logOutput "> Created cryAnim._f.getChildren function"










--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim._f.getChildExtent
----------------------------------------------------------------------------------------------------------------------------------
--			returns the extremes of the children positions
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim._f.getChildExtent = function getChildExtent root maxValue =
(
	try
	(
		if root.transform.pos[maxValue[1]] > maxValue[2] then
			maxValue[2] = root.transform.pos[maxValue[1]]

		if root.transform.pos[maxValue[1]] < maxValue[3] then
			maxValue[3] = root.transform.pos[maxValue[1]]

		if root.children != undefined then
		(
			for obj in root.children do
				if obj.isHidden == false and obj.isFrozen == false then
					cryTools.cryAnim._f.getChildExtent obj maxValue
		)
		return maxValue
	)
	catch ( logOutput "!!> Error in cryAnim._f.getChildExtent" )
)
getChildExtent = undefined
logOutput "> Created cryAnim._f.getChildExtent function"








--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim._f.createSnapshot
----------------------------------------------------------------------------------------------------------------------------------
--			creates snapshots of the selected node or with "object" specified node
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim._f.createSnapshot = function createSnapshot children:unsupplied object:unsupplied =
(
	try
	(
		baseArray = #()
		snapArray = #()

		function copyMesh baseObject =
		(
			try
			(
				local tempSnap = snapshot baseObject
				tempSnap.parent = undefined
				tempSnap.transform = baseObject.transform
				tempSnap.wirecolor = baseObject.wirecolor
				return tempSnap
			)
			catch
				return undefined
		)


		if object == unsupplied then
		(	
			for obj in $selection do
			(
				local newArray = #( obj )
				if children == true then
					join newArray (cryTools.cryAnim._f.getChildren obj)

				for obj in newArray do
				(
					append snapArray (copyMesh obj)
				)
					
				append baseArray newArray
			)
		)		
		else
		(
			append snapArray (copyMesh object)
		)
		
		if snapArray.count > 0 then
			return snapArray
		else
			return undefined
	)
	catch ( logOutput "!!> Error in cryAnim._f.createSnapshot" )
)
createSnapshot = undefined
logOutput "> Created cryAnim._f.createSnapshot function"








--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim._f.lockRotation
----------------------------------------------------------------------------------------------------------------------------------
--			locks rotation of the selected node
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim._f.lockRotation = function lockRotation =
(
	try
	(
		if selection.count > 0 then
		(
			if cryTools.cryAnim._v.lockRotation == undefined then
			(
				cryTools.cryAnim._v.lockRotation = $.transform.rotation
				return true
			)
			else
				return false
		)
	)
	catch ( logOutput "!!> Error in cryAnim._f.lockRotation" )
)
lockRotation = undefined
logOutput "> Created cryAnim._f.lockRotation function"









--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim._f.pasteTransform
----------------------------------------------------------------------------------------------------------------------------------
--			pastes the transform (#transform, #pos, #rotation)
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim._f.pasteTransform = function pasteTransform obj value op =
(
	try
	(
		try
		(
			case op of
			(
				#transform:
				(
					biped.setTransform obj #rotation value.rotation true
					biped.setTransform obj #pos value.pos true
					biped.setTransform obj #rotation value.rotation true
				)
				#pos:
				(
					biped.setTransform obj #pos value.pos true
				)
				#rotation:
				(
					biped.setTransform obj #rotation value.rotation true
					biped.setTransform obj #rotation value.pos true
				)
			)
		)
		catch
		(
			try
			(
				local tempTransform = obj.transform
				
				case op of
				(	
					#transform:
					(
						tempTransform = value
						/*
						obj.transform.rotation = value.rotation
						obj.transform.pos = value.pos
						obj.transform.rotation = value.rotation
						*/
					)
					#pos:
					(
						tempTransform.pos = value.pos
						tempTransform.rotation = obj.rotation
						--obj.transform.pos = value.pos
					)
					#rotation:
					(
						tempTransform.rotation = value.rotation
						tempTransform.pos = obj.pos
						--obj.transform.rotation = value.rotation
					)
				)
				obj.transform = tempTransform
			)
			catch()
		)
	)
	catch ( logOutput "!!> Error in cryAnim._f.pasteTransform" )
)
pasteTransform = undefined
logOutput "> Created cryAnim._f.pasteTransform function"
























--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.align._f.createPivotEdge
----------------------------------------------------------------------------------------------------------------------------------
--			creates the cylinders for visualizing selection and hovering of edges
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.align._f.createPivotEdge = function createPivotEdge index positionFrom positionTo =
(
	try
	(
		if (alignLayer = layerManager.newLayerFromName "alignTool") == undefined then
			alignLayer = layerManager.getLayerFromName "alignTool"
			
		alignLayer.current = true
					
			--// gets selected biped object
		selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()
		
			--// creates a cylinder and stores it in edgeList
		cryTools.cryAnim.align._v.edgeList.pivot[index] = cylinder()
			--// creates and stores name, and other various values
		cryTools.cryAnim.align._v.edgeList.pivot[index].name = ("e" + (index as String) + "_(" + selectedBipPart.name + ")")
		cryTools.cryAnim.align._v.edgeList.pivot[index].radius = 0.1
		cryTools.cryAnim.align._v.edgeList.pivot[index].sides = 5
			--// sets position to the inital position
		cryTools.cryAnim.align._v.edgeList.pivot[index].position = positionFrom
			--// calculates distance between 2 pivot points and applies to the height
		cryTools.cryAnim.align._v.edgeList.pivot[index].height = distance positionFrom positionTo
		
					
		
		if selectedBipPart.pivotSel.index != undefined then
		(
				--// checks, if this pivotEdge was used before	
			--if cryTools.cryAnim.align._v.edgeList.pivot[index].index == selectedBipPart.pivotSel.index then
			if index == selectedBipPart.pivotSel.index then
					--// if used, set the color to red
				cryTools.cryAnim.align._v.edgeList.pivot[index].wireColor = (color 255 0 0)
			else
					--// otherwise to "edgeBlue"
				cryTools.cryAnim.align._v.edgeList.pivot[index].wireColor = (color 20 50 100)
		)
		else 
			cryTools.cryAnim.align._v.edgeList.pivot[index].wireColor = (color 20 50 100)
		
			--// generates a tape with target for excact rotation of the cylinder
		tempTape = tape pos:positionFrom target:(targetObject pos:positionTo)
			--// applies transform from generated tape to the cylinder
		cryTools.cryAnim.align._v.edgeList.pivot[index].transform = tempTape.transform
			--// the cylinder is rotated 180°, so it will be rotated to the other side of the pivot
		cryTools.cryAnim.align._v.edgeList.pivot[index].position = positionTo
		
			--// temporary tape will be deleted
		cryTools.cryAnim.align._v.edgeList.selection[index] = cylinder radius:0.5 sides:5 height:cryTools.cryAnim.align._v.edgeList.pivot[index].height
		cryTools.cryAnim.align._v.edgeList.selection[index].name = ("sE" + (index as String) + "_(" + selectedBipPart.name + ")")
		cryTools.cryAnim.align._v.edgeList.selection[index].transform = tempTape.transform
		cryTools.cryAnim.align._v.edgeList.selection[index].position = positionTo
			--// hides the node (intersectRay still finds the mesh intersection of hidden nodes)
		cryTools.cryAnim.align._v.edgeList.selection[index].isHidden = true

		delete tempTape
		
		tempBaseLayer = layerManager.getLayer 0
		tempBaseLayer.current = true
		
		return true
	)
	catch ( logOutput "!!> Error in cryAnim.align._f.createPivotEdge" )
)
createPivotEdge = undefined
logOutput "> Created cryAnim.align._f.createPivotEdge function"











--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.align._f.createPivotPoint
----------------------------------------------------------------------------------------------------------------------------------
--			creates the spheres for visualizing selection and hovering of points
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.align._f.createPivotPoint = function createPivotPoint index position =
(
	try
	(
		if (alignLayer = layerManager.newLayerFromName "alignTool") == undefined then
			alignLayer = layerManager.getLayerFromName "alignTool"
			
		alignLayer.current = true


			--// gets selected object
		selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()
			--// generates sphere (point)	
		cryTools.cryAnim.align._v.pointList.pivot[index] = sphere()
			--// sets name and other values
		cryTools.cryAnim.align._v.pointList.pivot[index].name = ("p" + (index as String) + "_(" + selectedBipPart.name + ")")				
		cryTools.cryAnim.align._v.pointList.pivot[index].radius = 0.5
		cryTools.cryAnim.align._v.pointList.pivot[index].segs = 10		
		cryTools.cryAnim.align._v.pointList.pivot[index].position = position
		
		
		if selectedBipPart.pivotSel.index != undefined then
		(
				--// checks, if the pivot was used before
			--if cryTools.cryAnim.align._v.pointList.pivot[index]. == selectedBipPart.pivotSel.index then
			if index == selectedBipPart.pivotSel.index then
					--// if used, the color will be set to red
				cryTools.cryAnim.align._v.pointList.pivot[index].wireColor = (color 255 0 0)
			else
					--// otherwise to blue
				cryTools.cryAnim.align._v.pointList.pivot[index].wireColor = (color 0 0 255)
		)
		else
			cryTools.cryAnim.align._v.pointList.pivot[index].wireColor = (color 0 0 255)
					
			--// generates the selection area for pivot points		
		cryTools.cryAnim.align._v.pointList.selection[index] = sphere pos:cryTools.cryAnim.align._v.pointList.pivot[index].pos radius:1
		cryTools.cryAnim.align._v.pointList.selection[index].name = ("sP" + (index as String) + "_(" + selectedBipPart.name + ")")				
		
			--// hides the node (intersectRay still finds the mesh intersection of hidden nodes)
		cryTools.cryAnim.align._v.pointList.selection[index].isHidden = true
		
		tempBaseLayer = layerManager.getLayer 0
		tempBaseLayer.current = true

		
		return true
	)
	catch ( logOutput "!!> Error in cryAnim.align._f.createPivotPoint" )
)
createPivotPoint = undefined
logOutput "> Created cryAnim.align._f.createPivotPoint function"











--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.align.createPointEdge
----------------------------------------------------------------------------------------------------------------------------------
--			creates the points and edges for the selection
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.align._f.createPointEdge = function createPointEdge =
(
	try
	(
			--// gets selected object
		local selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()
		local editMeshAdded = #()
		
		if selectedBipPart != undefined then
		(
			with redraw off
			(
				local baseMeshNotFound = undefined
				
				try ( getVert selectedBipPart.object.selected 1 ) catch ( baseMeshNotFound = true)
				
				if baseMeshNotFound == true then
				(
					addModifier selectedBipPart.object.selected (edit_mesh())
					append editMeshAdded selectedBipPart.object.selected
				)
					--// checks, what body part is selected
				case (selectedBipPart.part.bodyPart) of
				(
					/*Foot*/(cryTools.cryAnim._f.getBipNames 2):
					(
						local child = undefined
						local meshNotFound = undefined
						
						try ( child = selectedBipPart.object.selected.children[1] ) catch()
						if child != undefined then
						(
							try ( getVert child 1 ) catch ( meshNotFound = true )
							
							if meshNotFound == true then
							(
								addModifier child (edit_mesh())
								append editMeshAdded child
							)
						)
						
						for i = 1 to 10 do
						(
								--// generates pivot points
							cryTools.cryAnim.align._f.createPivotPoint i (cryTools.cryAnim._f.getPivotPointPos i selectedBipPart.object.selected)
								--// generates pivot edges
							case i of
							(
								2:( cryTools.cryAnim.align._f.createPivotEdge 1 ( cryTools.cryAnim._f.getPivotPointPos 2 selectedBipPart.object.selected )	( cryTools.cryAnim._f.getPivotPointPos 5 selectedBipPart.object.selected ) )
								5:( cryTools.cryAnim.align._f.createPivotEdge 2 ( cryTools.cryAnim._f.getPivotPointPos 5 selectedBipPart.object.selected )	( cryTools.cryAnim._f.getPivotPointPos 7 selectedBipPart.object.selected ) )
								7:( cryTools.cryAnim.align._f.createPivotEdge 3 ( cryTools.cryAnim._f.getPivotPointPos 7 selectedBipPart.object.selected )	( cryTools.cryAnim._f.getPivotPointPos 4 selectedBipPart.object.selected ) )
								4:( cryTools.cryAnim.align._f.createPivotEdge 4 ( cryTools.cryAnim._f.getPivotPointPos 4 selectedBipPart.object.selected )	( cryTools.cryAnim._f.getPivotPointPos 2 selectedBipPart.object.selected ) )
							)
						)
					)
					/*Hand*/(cryTools.cryAnim._f.getBipNames 1):
					(
							--// gets children of the selected object (fingers)
						local childArray = cryTools.cryAnim._f.getChildren selectedBipPart.object.selected
						
						for i = 1 to 7 do
						(
								--// creates pivot points for the hand
							cryTools.cryAnim.align._f.createPivotPoint i (cryTools.cryAnim._f.getPivotPointPos i selectedBipPart.object.selected )
						)
						for i = 8 to (childArray.count + 7) do
						(
							try (getVert childArray[i - 7] 1)
							catch
							(
								addModifier childArray[i - 7] (edit_mesh())
								append editMeshAdded childArray[i - 7]
							)
								--// creates pivot points for the fingers
							cryTools.cryAnim.align._f.createPivotPoint i (cryTools.cryAnim._f.getPivotPointPos i selectedBipPart.object.selected )
						)
					)
				)
				for i = 1 to editMeshAdded.count do
					deleteModifier editMeshAdded[i] 1
			)
		)
		else
			#stop
	)
	catch ( logOutput "!!> Error in cryAnim.align._f.createPointEdge" )
)
createPointEdge = undefined
logOutput "> Created cryAnim.align._f.createPointEdge function"










--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.align._f.initVars
----------------------------------------------------------------------------------------------------------------------------------
--			deletes pivot points and initializes default pivot selection
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.align._f.initVars = function initVars =
(
	try
	(
		--// if there are still pivots in the scene, delete old pivots by deletePivotPoints() function
		try
		(
			if cryTools.cryAnim.align._v.pointList.pivot.count > 0 then
				cryTools.cryAnim.align._f.deletePivotPoints()
		) catch()

		return true
	)
	catch ( logOutput "!!> Error in cryAnim.align._f.initVars" )
)
initVars = undefined
logOutput "> Created cryAnim.align._f.initVars function"














--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.align._f.interactPivotPoints
----------------------------------------------------------------------------------------------------------------------------------
--			user interaction with pivot points and edges
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.align._f.interactPivotPoint = function interactPivotPoint =
(
	try
	(
			--// gets selected bip object
		local selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()
		
			--// var initialisation part			
		local worldRay = undefined
		local tempDistance = #()
			
		local activePivot = #()
		local nearestPivot = undefined
		
			--// stores all points and edges in pointEdgeSt structure via array
		local pointEdgeList = pointEdgeSt pivot:(cryTools.cryAnim.align._v.pointList.pivot + cryTools.cryAnim.align._v.edgeList.pivot) selection:(cryTools.cryAnim.align._v.pointList.selection + cryTools.cryAnim.align._v.edgeList.selection)
			
		for i = 1 to pointEdgeList.selection.count do
		(
				--// generates a ray from the mousecursor position into the viewport
			worldRay = mapScreenToWorldRay (in coordsys screen mouse.pos)
				--// checks, if the mouse cursor is over a pivot point or edge (worldRay intersects with geometry)
			tempPivotRay = intersectRay pointEdgeList.selection[i] worldRay
			
				--// if the ray intersects with the pivot selection
			if tempPivotRay != undefined then
			(
					--// if the pivot selection is not the pivot selected already
				if pointEdgeList.pivot[i].name != selectedBipPart.pivotSel.pivotName then
						--// adds the hovered pivot point or edge to the activePivot list struct
					append activePivot (activePivotSt interPivot:pointEdgeList.pivot[i] interPos:tempPivotRay.pos)
			)
		)			
		
			--// if a pivot is hovered
		if activePivot.count > 0 then
		(
				--// if more than 1 pivot is hovered (happens when in left or front view for example
			if activePivot.count > 1 then
			(
				for i = 1 to activePivot.count do
						--// stores all distances between the cursor pos (screen) and intersection position of the pivot point or edge
					tempDistance[i] = (distance worldRay.pos activePivot[i].interPos)									
				
				for i = 1 to activePivot.count do
				(
					if nearestPivot == undefined then
						-- if there is no nearestPivot before, make one
						nearestPivot = activePivot[i]
					else
							--// checks, if the distance of the activePivot before, has a bigger distance then the current one
						if (tempDistance[i]) < ( distance worldRay.pos nearestPivot.interPos ) then
								--// if so, the current activePivot is the new one
							nearestPivot = activePivot[i]
				)
					--// interPivot (the pivot point or edge) is the new nearestPivot
				nearestPivot = nearestPivot.interPivot
			)
			else
					--// if there is no further pivot point or edge in line, take the first one in list
				nearestPivot = activePivot[1].interPivot
		)
		else
				--// if no pivot point or edge is hovered, set nearestPivot to undefined
			nearestPivot = undefined

		if nearestPivot != undefined then
		(
				--// if the hovered nearest pivot is not an already selected one
			if nearestPivot.name != selectedBipPart.pivotSel.pivotName then
			(
					--// set the wireColor to the hovered color
				nearestPivot.wireColor = (color 250 190 30)
					--// and set to tempSelection - used for clickAction
				cryTools.cryAnim.align._v.pivotLast.tempSelection = nearestPivot
			)
		)
		else
				--// if there is no pivot selected, set tempSelection - used for clickAction - to undefined
			cryTools.cryAnim.align._v.pivotLast.tempSelection = undefined

		for i = 1 to pointEdgeList.pivot.count do
		(
			if pointEdgeList.pivot[i].name != selectedBipPart.pivotSel.pivotName then
			(
				if pointEdgeList.pivot[i] != nearestPivot then
				(
						--// if no pivot point or edge is already selected or hovered, then apply wireColor
					if (substring pointEdgeList.pivot[i].name 1 1) == "p" then
							--// for points
						pointEdgeList.pivot[i].wireColor = (color 0 0 255)
					else
							--// for edges
						pointEdgeList.pivot[i].wireColor = (color 20 50 100)
				)
			)
		)
	)
	catch ( logOutput "!!> Error in cryAnim.align._f.interactPivotPoint" )
)
interactPivotPoint = undefined
logOutput "> Created cryAnim.align._f.interactPivotPoint function"











--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.align._f.setPivotPoint
----------------------------------------------------------------------------------------------------------------------------------
--			sets the selected pivot point to the structure
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.align._f.setPivotPoint = function setPivotPoint tempPivot =
(
	try
	(
		local selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()
		
			--// filters the name to have only the number of the pivot
		local tempFilterString =  filterString tempPivot.name "_"
		tempFilterString = substring tempFilterString[1] 2 tempFilterString[1].count

			--// sets the pivotList.pivotLast part to the selected pivot
		execute ("cryTools.cryAnim.align._v.pivotLast." + selectedBipPart.part.name +  ".index = " + tempFilterString )
		execute ("cryTools.cryAnim.align._v.pivotLast." + selectedBipPart.part.name +  ".pivotName = \"" + tempPivot.name + "\"")
		execute ("cryTools.cryAnim.align._v.pivotLast." + selectedBipPart.part.name +  ".position = " + (tempPivot.pos as String))
		local childArray = cryTools.cryAnim._f.getChildren selectedBipPart.object.selected
		local tempPivotIndex = tempFilterString as Integer		
		
			
		if tempPivotIndex <= 7 then
			execute ("cryTools.cryAnim.align._v.pivotLast." + selectedBipPart.part.name +  ".pivObject = \"" + selectedBipPart.object.selected.name + "\"" )
		else
		(
			if selectedBipPart.part.bodyPart == "Hand" then
				execute ("cryTools.cryAnim.align._v.pivotLast." + selectedBipPart.part.name +  ".pivObject = \"" + childArray[tempPivotIndex - 7].name + "\"" )
			else
				execute ("cryTools.cryAnim.align._v.pivotLast." + selectedBipPart.part.name +  ".pivObject = \"" + childArray[1].name + "\"" )
		)
		return true
	)
	catch ( logOutput "!!> Error in cryAnim.align._f.setPivotPoint" )
)
setSelectedBipPart = undefined
logOutput "> Created cryAnim.align._f.setPivotPoint function"










--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.align._f.clickAction
----------------------------------------------------------------------------------------------------------------------------------
--			user interaction for the pivot selection
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.align._f.clickAction = function clickAction =
(
	try
	(
		local selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()
			--// checks, if there was a pivot selected before
		if selectedBipPart == undefined then return false
		
			--// if no pivot is selected before
		if selectedBipPart.pivotSel.index == undefined then
		(			
				--// if there is a pivot currently hovered
			if cryTools.cryAnim.align._v.pivotLast.tempSelection != undefined then
			(
					--// on mouse click, set color of the current pivot to red
				cryTools.cryAnim.align._v.pivotLast.tempSelection.wireColor = (color 255 0 0)

					--// stores all the infos in the state variable for use of other _f					
				cryTools.cryAnim.align._f.setPivotPoint cryTools.cryAnim.align._v.pivotLast.tempSelection					
			)
		)
			--// if a pivot were selected before		
		else
		(
				--// if a pivot is currently hovered
			if cryTools.cryAnim.align._v.pivotLast.tempSelection != undefined then
			(					
					--// on mouseclick sets color to red
				cryTools.cryAnim.align._v.pivotLast.tempSelection.wireColor = (color 255 0 0)
					--// stores the values in the variable for the use of other _f
				cryTools.cryAnim.align._f.setPivotPoint cryTools.cryAnim.align._v.pivotLast.tempSelection
					
				for i = 1 to cryTools.cryAnim.align._v.pointList.pivot.count do
				(
						--// checks, if the pivot in list is not the current pivot
					if cryTools.cryAnim.align._v.pointList.pivot[i] != cryTools.cryAnim.align._v.pivotLast.tempSelection then
							--// and sets the color to blue (because of not selected)
						cryTools.cryAnim.align._v.pointList.pivot[i].wireColor = (color 0 0 255)							
				)

				for i = 1 to cryTools.cryAnim.align._v.edgeList.pivot.count do
				(
						--// checks, if the current pivot is not the current edge
					if cryTools.cryAnim.align._v.edgeList.pivot[i] != cryTools.cryAnim.align._v.pivotLast.tempSelection then
							--// and sets the color to "edgeBlue"
						cryTools.cryAnim.align._v.edgeList.pivot[i].wireColor = (color 20 50 100)
				)
			)				
		)
	)
	catch ( logOutput "!!> Error in cryAnim.align._f.clickAction" )
)
clickAction = undefined
logOutput "> Created cryAnim.align._f.clickAction function"















--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.align._f.deletePivotPoints
----------------------------------------------------------------------------------------------------------------------------------
--			deletes pivot points (spheres and edges)
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.align._f.deletePivotPoints = function deletePivotPoints =
(
	try
	(
		for i = 1 to cryTools.cryAnim.align._v.pointList.pivot.count do
		(				
			try
			(		
					--// tries to delete the pivot points
				delete cryTools.cryAnim.align._v.pointList.pivot[i]
				delete cryTools.cryAnim.align._v.pointList.selection[i]
			)catch()
		)
		
		for i = 1 to cryTools.cryAnim.align._v.edgeList.pivot.count do
		(
			try
			(
					--// tries to delete the pivot edges
				delete cryTools.cryAnim.align._v.edgeList.pivot[i]
				delete cryTools.cryAnim.align._v.edgeList.selection[i]
			)catch()
		)
			
		cryTools.cryAnim.align._v.pointList.pivot = #()
		cryTools.cryAnim.align._v.pointList.selection = #()
		
		cryTools.cryAnim.align._v.edgeList.pivot = #()
		cryTools.cryAnim.align._v.edgeList.selection = #()
	)
	catch ( logOutput "!!> Error in cryAnim.align._f.deletePivotPoints" )
)
deletePivotPoints = undefined
logOutput "> Created cryAnim.align._f.deletePivotPoints function"

















--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.align._f.switchViewPort
----------------------------------------------------------------------------------------------------------------------------------
--			switches between viewports, if there are more than 1 viewport
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.align._f.switchViewPort = function switchViewPort =
(
	try
	(
		local numViews = viewport.numViews
		local viewSize = getViewSize()
		
		
		case numViews of
		(
			4:
			(
				if mouse.screenpos[1] < viewSize[1] then
				(
					if mouse.screenpos[2] < (viewSize[2]+74) then
						if viewport.activeViewport != 1 then viewport.activeViewport = 1
					if mouse.screenpos[2] > (viewSize[2]+74) then 
						if viewport.activeViewport != 3 then viewport.activeViewport = 3
				)
				
				if mouse.screenpos[1] > viewSize[1] then
				(
					if mouse.screenpos[2] < (viewSize[2]+74) then
						if viewport.activeViewport != 2 then viewport.activeViewport = 2
					if mouse.screenpos[2] > (viewSize[2]+74) then
						if viewport.activeViewport != 4 then viewport.activeViewport = 4
				)
			)
		)
	)
	catch ( logOutput "!!> Error in cryAnim.align._f.switchViewPort" )
)
switchViewPort = undefined
logOutput "> Created cryAnim.align._f.switchViewPort function"











--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.align._f.callPivotSelect
----------------------------------------------------------------------------------------------------------------------------------
--			creates the tool
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.align._f.callPivotSelect = function callPivotSelect =
(
	try
	(
		local selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart()
		if selectedBipPart == undefined then return undefined
		if selectedBipPart.object.isPivot == false then return false
		
		
		tool pivotSelect
		(
			on start do
			(
					--// initialise variables for global use
				cryTools.cryAnim.align._f.initVars()
				cryTools.cryAnim.align._f.createPointEdge()
			)
				
			on freeMove do
			(
					--// makes the recognition of mouse interaction
				cryTools.cryAnim.align._f.switchViewPort()
				cryTools.cryAnim.align._f.interactPivotPoint()
			)
			
			on mousePoint click do
			(
					--// switches trough pivots and highlights them, as well sets global variables for other _f
				cryTools.cryAnim.align._f.switchViewPort()
				cryTools.cryAnim.align._f.clickAction()
			)
			
			on mouseMove click do
			(	
					--// after the first click is finished, it waits for the mouse click to start the function
				if click == 1 then
						--// switches trough pivots and highlights them, as well sets global variables for other _f
					cryTools.cryAnim.align._f.clickAction()
				
					--// makes the recognition of mouse interaction
				cryTools.cryAnim.align._f.switchViewPort()
				cryTools.cryAnim.align._f.interactPivotPoint()
			)
			
			on stop do
			(
					--// deletes the used nodes
				cryTools.cryAnim.align._f.deletePivotPoints()
			)
		)
		cryTools.cryAnim.align.pivotSelect = pivotSelect
		pivotSelect = undefined
		
		startTool cryTools.cryAnim.align.pivotSelect
	)
	catch ( logOutput "!!> Error in cryAnim.align._f.callPivotSelect" )
)
callPivotSelect = undefined
logOutput "> Created cryAnim.align._f.callPivotSelect function"






































--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main._f.getUI
----------------------------------------------------------------------------------------------------------------------------------
--			returns the UI element (rollout, rollout controls/control)
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main._f.getUI = function getUI rolloutTitle element =
(
	try
	(
		tempInt = undefined
		tempCon = undefined
		tempElement = undefined
		local tempArray = #()
		
		
		try
		(
			for i = 1 to cryTools.cryAnim.UI.main.dialog.controls.count do
			(
				for f = 1 to cryTools.cryAnim.UI.main.dialog.controls[i].rollouts.count do
				(
					if cryTools.cryAnim.UI.main.dialog.controls[i].rollouts[f].title == rolloutTitle then
					(
						tempInt = f
						tempCon = i
					)
					if cryTools.cryAnim.UI.main.dialog.controls[i].rollouts[f].name == rolloutTitle then
					(
						tempInt = f
						tempCon = i
					)
					
					append tempArray cryTools.cryAnim.UI.main.dialog.controls[i].rollouts[f]
				)
			)
		)
		catch(return undefined)

		try
		(
			if element == "" then
			(
				if rolloutTitle == "" then
					return tempArray
				else
					return cryTools.cryAnim.UI.main.dialog.controls[tempCon].rollouts[tempInt]
			)
		)catch()
		
		
		try
		(
			for i = 1 to cryTools.cryAnim.UI.main.dialog.controls[tempCon].rollouts[tempInt].controls.count do
			(
				if cryTools.cryAnim.UI.main.dialog.controls[tempCon].rollouts[tempInt].controls[i].name == element then tempElement = i
				if cryTools.cryAnim.UI.main.dialog.controls[tempCon].rollouts[tempInt].controls[i].text == element then tempElement = i
			)
		)
		catch(return undefined)

		
		
		
			
		if tempCon != undefined and tempInt != undefined and tempElement != undefined then
			try ( execute ("return cryTools.cryAnim.UI.main.dialog.controls[" + tempCon as String + "].rollouts[" + tempInt as String + "].controls[" + tempElement as String + "]") ) catch()
		
		if tempInt != 0 then
			try ( execute ("return cryTools.cryAnim.UI.main.dialog.controls[" + tempCon as String + "].rollouts[" + tempInt as String + "].controls") ) catch()
		else
			return undefined
	)
	catch ( logOutput "!!> Error in cryAnim.UI.main._f." )
)
getUI = undefined
logOutput "> Created cryAnim.UI.main._f.getUI function"













--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main._f.checkExport
----------------------------------------------------------------------------------------------------------------------------------
--			checks the file path before saving or exporting, converts file path to .caf or to the project path, creates a folder
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main._f.checkExport = function checkExport op filepath =
(
	try
	(
		abortBool = false
		
		tempArray = filterString (cryTools.cryAnim.base.lowercase filepath) "\\"
		tempBUIldPath = (filterString (cryTools.cryAnim.base.lowercase crytools.BUIldPathFull) "\\")
		
		case op of
		(
			#save:
			(
				if (biped.getCurrentLayer $'Bip01'.controller) > 0 then
				(
					messageBox "Collapse Layers or choose Base Layer to Save" title:"Save Biped"
					return false
				)
				
				/*
				if tempArray[1] != tempBUIldPath[1] then abortBool = true
				
				if abortBool == true then
					return false
				else
					return true
				*/
				return true
			)

			#saveExport:
			(
				if (biped.getCurrentLayer $'Bip01'.controller) > 0 then
				(
					messageBox "Collapse Layers or choose Base Layer to Save" title:"Save Biped"
					return false
				)

				/*
				if tempArray[1] != tempBUIldPath[1] then abortBool = true
				if tempArray[2] != tempBUIldPath[2] + "_production" then abortBool = true
				if tempArray[3] != "art" then abortBool = true
				if tempArray[4] != "animation" then abortBool = true
				if tempArray[5] != "human" then	abortBool = true
				
				
				if abortBool == true then
				(
					messageBox ("\nSave to    " + (substring crytools.BUIldPathFull 1 (crytools.BUIldPathFull.count - 1)) + "_Production\\Art\\Animation\\Human\\Male\\     \n\n                        or use the \"Save\" button\n\n(Resource Compiler)") title:"Export Animation"
					return false
				)
				*/
				cryTools.cryAnim.UI.main._f.checkExport #createFolder filepath
				return true
			)
			#export:
			(
				/*
				if tempArray[1] != tempBUIldPath[1] then (abortBool = true; print "erste zeile")
				if tempArray[2] != tempBUIldPath[2] then (abortBool = true; print "zweite zeile")
				if tempArray[3] != "game" then (abortBool = true; print "dritte zeile")
				if tempArray[4] != "animations" then (abortBool = true; print "fünfte zeile")
				
				if abortBool == true then
				(
					messageBox ("\nSave to    " + crytools.BUIldPathFull + "Game\\Animations\\     \n\n(Resource Compiler)") title:"Export Animation"
					return false
				)
				
				cryTools.cryAnim.UI.dialog._f.checkExport #createFolder filepath
				*/
				return true
			)
			
			#ProductionToGame:
			(
				try
				(
					filepathFilter = filterString filepath "\\"
					workingDirFilter = filterString cryTools.cryAnim.UI.main._v.bipWorkingDir "\\"
					cafSavePath = cryTools.cryAnim.UI.main._v.cafWorkingDir
					
					if filepathFilter.count != workingDirFilter.count then
					(
						for i = (workingDirFilter.count + 1) to filepathFilter.count do
							cafSavePath += filepathFilter[i] + (if i != filepathFilter.count then "\\" else "")
									
					)
					
					return (cryTools.cryAnim.UI.main._f.checkExport #getCaf cafSavePath)
				)
				catch ( return cryTools.cryAnim.UI.main._v.cafSavePath )
				
			)
			
			#getCaf:
			(
				local tempArray = filterString filepath "."
				return (tempArray[1] + ".caf")
			)
			
			
			#createFolder:
			(
				if filePath != undefined then
				(					
					tempPath = filterString filepath "\\"
					maxItems = tempPath.count
					for i = 1 to tempPath.count do
					(
						if findString tempPath[i] "." != undefined then
						(
							maxItems = i - 1
							exit
						)
					)
					tempString = ""
					for i = 1 to maxItems do
					(
						if i == 1 then
							tempString = tempPath[i]
						else
							tempString += "\\" + tempPath[i]
							
						makeDir tempString
					)
					return tempString
				)
				else return false
			)
					
		)
	)
	catch ( logOutput "!!> Error in cryAnim.UI.main._f.checkExport" )
)
checkExport = undefined
logOutput "> Created cryAnim.UI.main._f.checkExport function"









--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main._f.updateDialog
----------------------------------------------------------------------------------------------------------------------------------
--			updates the main dialog, sets new size
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main._f.updateDialog = function updateDialog =
(
	try
	(
		local maxRowHeight = 0
		
		if cryTools.cryAnim.base.iniFile #get #multiRow == true then
		(
			local rowCount = 0
			
			for i = 1 to cryTools.cryAnim.UI.main.dialog.controls.count do
			(
				rowHeight = 0
				
				if cryTools.cryAnim.UI.main.dialog.controls[i].rollouts.count > 0 then
					rowCount = i
				
				for f = 1 to cryTools.cryAnim.UI.main.dialog.controls[i].rollouts.count do
				(
					if cryTools.cryAnim.UI.main.dialog.controls[i].rollouts[f].open == true then
						rowHeight += cryTools.cryAnim.UI.main.dialog.controls[i].rollouts[f].height + 4
					
					rowHeight += 21.5
				)
				
				
				cryTools.cryAnim.UI.main.dialog.controls[i].height = rowHeight
				
				if rowHeight > maxRowHeight then
					maxRowHeight = rowHeight + 4
			)
			
			if maxRowHeight > cryTools.cryAnim.UI.main._v.dialogMaxHeight then
				maxRowHeight = cryTools.cryAnim.UI.main._v.dialogMaxHeight
			
			if maxRowHeight > 637 then
				maxRowHeight = 637
			
			for i = 1 to cryTools.cryAnim.UI.main.dialog.controls.count do
				cryTools.cryAnim.UI.main.dialog.controls[i].height = maxRowHeight + 4
			
			
			cryTools.cryAnim.UI.main.dialog.height = maxRowHeight + 8
			cryTools.cryAnim.UI.main.dialog.width = rowCount * 173 
		)
		else
		(
			for i = 1 to cryTools.cryAnim.UI.main.dialog.row1.rollouts.count do
			(
				if cryTools.cryAnim.UI.main.dialog.row1.rollouts[i].open == true then					
					maxRowHeight += cryTools.cryAnim.UI.main.dialog.row1.rollouts[i].height + 4
				
				maxRowHeight += 21.5
			)
			
			if maxRowHeight > cryTools.cryAnim.UI.main._v.dialogMaxHeight then
				maxRowHeight = cryTools.cryAnim.UI.main._v.dialogMaxHeight
			
			cryTools.cryAnim.UI.main.dialog.height = maxRowHeight
			cryTools.cryAnim.UI.main.dialog.row1.height = maxRowHeight - 4
		)
	)
	catch ( logOutput "!!> Error in cryAnim.UI.main._f.updateDialog" )
)
updateDialog = undefined
logOutput "> Created cryAnim.UI.main._f.updateDialog function"










--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main._f.callDialog
----------------------------------------------------------------------------------------------------------------------------------
--			creates the main dialog with all the rollouts and elements
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////
	
cryTools.cryAnim.UI.main._f.callDialog = function callDialog =
(
	try
	(
		try
		(
			if cryTools.showSplash == true then
			(
				if crySplash.open == false then
				(
					bm = (openBitmap (crytools.BuildPathFull + "Tools\\maxscript\\icons\\splash.png"))
					createDialog crysplash style:#(#style_border) modal:false bitmap:bm width:bm.width height:bm.height fgcolor:white
				)
				try (crysplash.build.text = "LOADING DIALOG") catch()
			)
		)catch()
		
		
		if cryTools.cryAnim.UI.main.dialog != undefined then
			destroyDialog cryTools.cryAnim.UI.main.dialog
		
		
		
		
		global initializeDialog = true
		
			--// dialog template
		rollout cryAnimDialog "CryAnim  v3.2"
		(
			subrollout row1 pos:[2,2] width:170 height:(cryTools.cryAnim.UI.main.dialog.height-4)
			subrollout row2 pos:[175,2] width:170 height:(cryTools.cryAnim.UI.main.dialog.height-4)
			subrollout row3 pos:[348,2] width:170 height:(cryTools.cryAnim.UI.main.dialog.height-4)
			subrollout row4 pos:[521,2] width:170 height:(cryTools.cryAnim.UI.main.dialog.height-4)
			
			on cryAnimDialog moved value do
				if initializeDialog == undefined then
					cryTools.cryAnim.base.iniFile #set #dialogPos
		)
		cryTools.cryAnim.UI.main.dialog = cryAnimDialog
		cryAnimDialog = undefined
		
		createDialog cryTools.cryAnim.UI.main.dialog 173 579 style:#(#style_minimizebox,#style_titlebar,#style_border,#style_sysmenu) lockWidth:true lockHeight:false pos:[-1000,0]
		
		initializeDialog = undefined
		
		cryTools.cryAnim.base.iniFile #set #visible value:"1"
		
		fileIn (cryTools.buildPathFull + "Tools\\maxscript\\cryAnim\\ui\\main\\target.ms")
		fileIn (cryTools.buildPathFull + "Tools\\maxscript\\cryAnim\\ui\\main\\operation.ms")
		fileIn (cryTools.buildPathFull + "Tools\\maxscript\\cryAnim\\ui\\main\\pivot.ms")
		
		if (maxVersion())[1] >= 9000 then
		(
			fileIn (cryTools.buildPathFull + "Tools\\maxscript\\cryAnim\\ui\\batch\\batch9.ms")
			fileIn (cryTools.buildPathFull + "Tools\\maxscript\\cryAnim\\ui\\main\\models\\models9.ms")
		)
		else
		(
			fileIn (cryTools.buildPathFull + "Tools\\maxscript\\cryAnim\\ui\\batch\\batch8.ms")
			fileIn (cryTools.buildPathFull + "Tools\\maxscript\\cryAnim\\ui\\main\\models\\models8.ms")
		)
		
		
		if cryTools.domain == "INTERN.CRYTEK.DE" then
			fileIn (cryTools.buildPathFull + "Tools\\maxscript\\cryAnim\\ui\\main\\muscle.ms")
		
		fileIn (cryTools.buildPathFull + "Tools\\maxscript\\cryAnim\\ui\\main\\locator.ms")
		
		fileIn (cryTools.buildPathFull + "Tools\\maxscript\\cryAnim\\ui\\main\\save.ms")
		
		fileIn (cryTools.buildPathFull + "Tools\\maxscript\\cryAnim\\ui\\main\\pose.ms")
		
		if cryTools.cryAnim._v.perforceDir != undefined then
			fileIn (cryTools.buildPathFull + "Tools\\maxscript\\cryAnim\\ui\\main\\perforce.ms")
		fileIn (cryTools.buildPathFull + "Tools\\maxscript\\cryAnim\\ui\\main\\tools.ms")
		
		
		
		
		if (local dialogPos = cryTools.cryAnim.base.iniFile #get #dialogPos) != undefined then
			try	( setDialogPos cryTools.cryAnim.UI.main.dialog dialogPos )catch()
		else
			try	( setDialogPos cryTools.cryAnim.UI.main.dialog ( (getMAXWindowSize() / 2) - ((getDialogSize cryTools.cryAnim.UI.main.dialog) / 2) ) )catch()
		
		
		
		try destroyDialog crySplash catch()
		
	)
	catch ( logOutput "!!> Error in cryAnim.UI.main._f.callDialog" )
)
callDialog = undefined
logOutput "> Created cryAnim.UI.main._f.callDialog function"
















cryTools.cryAnim.UI.batchProcess._f.subRangeUpdateList = function subRangeUpdateList = --// updateList
(
	try
	(
		local tempList = #()
		local listEntries = cryTools.cryAnim.UI.batchProcess._v.exportFiles[cryTools.cryAnim.UI.batchProcess._v.selectedFile].subRanges
		
		
		for i = 1 to listEntries.count do
		(
			local tempStart = listEntries[i].range.start.frame as Integer
			local tempStop = listEntries[i].range.end.frame as Integer
			local tempObjectsName = ""
			
			for f = 1 to listEntries[i].objects.count do
				tempObjectsName += listEntries[i].objects[f] + (if f < listEntries[i].objects.count then "; " else "")
			
			local tempString = listEntries[i].export + "    (" + tempObjectsName + ")      <" + tempStart as String + ">  <" + tempStop as String + ">"
			
			append tempList tempString
		)
		
		cryTools.cryAnim.UI.batchProcess.subRangeDialog.lbObjects.items = tempList
	)
	catch()
	--catch ( logOutput "!!> Error in cryAnim.UI.batchProcess._f.subRangeUpdateList" )
)
subRangeUpdateList = undefined
logOutput "> Created cryAnim.UI.batchProcess._f.subRangeUpdateList function"








cryTools.cryAnim.UI.batchProcess._f.subRangeGetRange = function subRangeGetRange objectArray = --// getRange
(
	try
	(
		local tempStart = 0
		local tempStop = 0
		
		for obj in objectArray do
		(
			if obj != undefined then
			(
				for i = 1 to 2 do
				(
					if obj[3][i].keys.count > 0 then
					(
						if obj[3][i].keys[obj[3][i].keys.count].time.frame > tempStop then
							tempStop = obj[3][i].keys[obj[3][i].keys.count].time.frame
						
						if obj[3][i].keys.count > 1 then
							if obj[3][i].keys[1].time.frame < tempStart then
								tempStart = obj[3][i].keys[1].time.frame
					)
				)
			)
		)
		
		return (interval (tempStart as Integer) (tempStop as Integer) )
	)
	catch ( logOutput "!!> Error in cryAnim.UI.batchProcess._f.subRangeGetRange" )
)
subRangeGetRange = undefined
logOutput "> Created cryAnim.UI.batchProcess._f.subRangeGetRange function"








cryTools.cryAnim.UI.batchProcess._f.subRangeUpdateRange = function subRangeUpdateRange index = --// updateRange
(
	try
	(
		if cryTools.cryAnim.UI.batchProcess.subRangeDialog.open == true then
		(
			local listEntries = cryTools.cryAnim.UI.batchProcess._v.exportFiles[cryTools.cryAnim.UI.batchProcess._v.selectedFile].subRanges
			local newObjects = #()
			
			for i = 1 to listEntries[index].objects.count do
				newObjects[i] = getNodeByName listEntries[index].objects[i]
			
			
			
			local tempRange = cryTools.cryAnim.UI.batchProcess._f.subRangeGetRange newObjects
			
			cryTools.cryAnim.UI.batchProcess._v.exportFiles[cryTools.cryAnim.UI.batchProcess._v.selectedFile].subRanges[index].range = tempRange
		)
	)
	catch ( logOutput "!!> Error in cryAnim.UI.batchProcess._f.subRangeUpdateRange" )
)
subRangeUpdateRange = undefined
logOutput "> Created cryAnim.UI.batchProcess._f.subRangeUpdateRange function"









cryTools.cryAnim.UI.batchProcess._f.subRangeUpdateValues = function subRangeUpdateValues = --// updateValues
(
	try
	(
		if cryTools.cryAnim.UI.batchProcess.subRangeDialog.open == true then
		(
			local lbObjects = cryTools.cryAnim.UI.batchProcess.subRangeDialog.lbObjects
			local listEntries = cryTools.cryAnim.UI.batchProcess._v.exportFiles[cryTools.cryAnim.UI.batchProcess._v.selectedFile].subRanges
			local edAnimName = cryTools.cryAnim.UI.batchProcess.subRangeDialog.edAnimName
			local edPrefixName = cryTools.cryAnim.UI.batchProcess.subRangeDialog.edPrefixName
			local labObjects = cryTools.cryAnim.UI.batchProcess.subRangeDialog.labObjects
			
			local spnStart = cryTools.cryAnim.UI.batchProcess.subRangeDialog.spnStart
			local spnStop = cryTools.cryAnim.UI.batchProcess.subRangeDialog.spnStop
			
			edPrefixName.text = cryTools.cryAnim.UI.batchProcess._v.exportFiles[cryTools.cryAnim.UI.batchProcess._v.selectedFile].subRangePrefix
			
			if lbObjects.selection != 0 then
			(
				spnStart.value = listEntries[lbObjects.selection].range.start
				spnStop.value = listEntries[lbObjects.selection].range.end
				
				local tempObjectsName = ""
				for i = 1 to listEntries[lbObjects.selection].objects.count do
					tempObjectsName += listEntries[lbObjects.selection].objects[i] + (if i < listEntries[lbObjects.selection].objects.count then "; " else "")
				
				labObjects.text = tempObjectsName
				
				edAnimName.text = listEntries[lbObjects.selection].export
			)
			else
			(
				spnStart.value = 0
				spnStop.value = 0
				
				labObjects.text = "None"
				
				edAnimName.text = ""
			)
		)
	)
	catch()
	--catch ( logOutput "!!> Error in cryAnim.UI.batchProcess._f.subRangeUpdateValues" )
)
subRangeUpdateValues = undefined
logOutput "> Created cryAnim.UI.batchProcess._f.subRangeUpdateValues function"









cryTools.cryAnim.UI.batchProcess._f.subRangeSetList = function subRangeSetList objArray = --// setList
(
	try
	(
		if cryTools.cryAnim.UI.batchProcess.subRangeDialog.open == true then
		(
			local edPrefixName = cryTools.cryAnim.UI.batchProcess.subRangeDialog.edPrefixName
			local tempInterval = cryTools.cryAnim.UI.batchProcess._f.subRangeGetRange objArray
			local listEntries = cryTools.cryAnim.UI.batchProcess._v.exportFiles[cryTools.cryAnim.UI.batchProcess._v.selectedFile].subRanges
			
			try
			(
				for i = 1 to objArray.count do
					objArray[i] = objArray[i].name
			)catch()
			
			append listEntries (subRangeStruct objects:objArray range:tempInterval export:objArray[1])
			
			
			if cryTools.cryAnim.UI.batchProcess._v.exportFiles[cryTools.cryAnim.UI.batchProcess._v.selectedFile].subRangePrefix == "" then
				try (cryTools.cryAnim.UI.batchProcess._v.exportFiles[cryTools.cryAnim.UI.batchProcess._v.selectedFile].subRangePrefix = (filterString maxFileName "_")[1]) catch()
			
			cryTools.cryAnim.UI.batchProcess._v.exportFiles[cryTools.cryAnim.UI.batchProcess._v.selectedFile].subRanges = listEntries
			
			cryTools.cryAnim.UI.batchProcess._f.subRangeUpdateList()
			cryTools.cryAnim.UI.batchProcess._f.subRangeUpdateValues()
		)
	)
	catch ( logOutput "!!> Error in cryAnim.UI.batchProcess._f.subRangeSetList" )
)
subRangeSetList = undefined
logOutput "> Created cryAnim.UI.batchProcess._f.subRangeSetList function"











			
			
cryTools.cryAnim.UI.batchProcess._f.subRangeLoadFromDummy = function subRangeLoadFromDummy = --// loadFromDummy
(
	try
	(
		local listEntries = cryTools.cryAnim.UI.batchProcess._v.exportFiles[cryTools.cryAnim.UI.batchProcess._v.selectedFile].subRanges
		try (local edPrefixName = cryTools.cryAnim.UI.batchProcess.subRangeDialog.edPrefixName) catch()
		
		if (local rangeDummy = getNodeByName "subRangeDummy") != undefined then
		(
			try
			(
				local UDPArray = #()
				
				local prefix = getUserProp rangeDummy "subRangePrefix"
				
				local errorArray = #()
				
				
				for i = 1 to 100 do
				(
					if (local UDPString = (getUserProp rangeDummy ("subRange" + i as String))) != undefined then
					(
						local filterArray = filterString UDPString "#"
						local objectsArrayString = filterString filterArray[1] ";"
						local objectsArray = #()
						
						local range = execute (filterArray[2])
						
						if objectsArrayString.count > 0 then
							append listEntries (subRangeStruct objects:objectsArrayString range:range export:filterArray[3])
					)
				)
				
				cryTools.cryAnim.UI.batchProcess._v.exportFiles[cryTools.cryAnim.UI.batchProcess._v.selectedFile].subRanges = listEntries
				
				
				try (edPrefixName.text = prefix) catch()
				cryTools.cryAnim.UI.batchProcess._v.exportFiles[cryTools.cryAnim.UI.batchProcess._v.selectedFile].subRangePrefix = prefix
				
				cryTools.cryAnim.UI.batchProcess._f.subRangeUpdateDialog()
				
				return true
				
			)catch()
			
			return false
		)
	)
	catch ( logOutput "!!> Error in cryAnim.UI.batchProcess._f.subRangeLoadFromDummy" )
)
subRangeLoadFromDummy = undefined
logOutput "> Created cryAnim.UI.batchProcess._f.subRangeLoadFromDummy function"










cryTools.cryAnim.UI.batchProcess._f.subRangeUpdateDialog = function subRangeUpdateDialog =
(
	try
	(
		cryTools.cryAnim.UI.batchProcess._f.subRangeUpdateList()
		cryTools.cryAnim.UI.batchProcess._f.subRangeUpdateValues()
	)
	catch ( logOutput "!!> Error in cryAnim.UI.batchProcess._f.subRangeUpdateDialog" )
)
subRangeUpdateDialog = undefined
logOutput "> Created cryAnim.UI.batchProcess._f.subRangeUpdateDialog function"










cryTools.cryAnim.UI.batchProcess._f.subRangeCreateUDPString = function createUDPString importIndex:undefined =
(
	try
	(
		if importIndex == undefined then
			importIndex = cryTools.cryAnim.UI.batchProcess._v.selectedFile
			
		
		local listEntries = cryTools.cryAnim.UI.batchProcess._v.exportFiles[importIndex]--.subRanges
				
		if $subRangeDummy == undefined then
		(
			local saveDummy = dummy name:"subRangeDummy"
			saveDummy.isHidden = true
		)
		else
			local saveDummy = getNodeByName "subRangeDummy"
		
		
		local UDPArray = filterString (getUserPropBuffer saveDummy) "\r\n"
		
		local UDPString = ""
		for i = 1 to UDPArray.count do
			if (findString UDPArray[i] "subRange") == undefined then
				UDPString += UDPArray[i] + "\r\n"
		
		
		
		UDPString += "subRangePrefix=" + listEntries.subRangePrefix + "\r\n"--edPrefixName.text + "\r\n"
			
		for i = 1 to listEntries.subRanges.count do
		(
			local listString = "subRange" + i as String + "="
			for f = 1 to listEntries.subRanges[i].objects.count do
				listString += listEntries.subRanges[i].objects[f] + (if f < listEntries.subRanges[i].objects.count then ";" else "")
			
			listString += "#" + listEntries.subRanges[i].range as String + "#" + listEntries.subRanges[i].export as String + "\r\n"
			
			UDPString += listString
		)
		
		return UDPString
	)
	catch ( logOutput "!!> Error in cryAnim.UI.batchProcess._f.subRangeCreateUDPString" )
)
subRangeCreateUDPString = undefined
logOutput "> Created cryAnim.UI.batchProcess._f.subRangeCreateUDPString function"













cryTools.cryAnim.UI.batchProcess._f.subRangeCallDialog = function subRangeCallDialog =
(
	try
	(
		try ( destroyDialog cryTools.cryAnim.UI.batchProcess.subRangeDialog ) catch()

		rollout subRangeRO "Sub-Ranges" width:200 height:364
		(
			
			button btnAddObjects "Add Entry" pos:[8,10] width:184 height:20
			listbox lbObjects "" pos:[8,40] width:184 height:18
			label labStart "Start" pos:[22,287] width:32 height:16
			spinner spnStart "" pos:[8,303] width:60 height:16 range:[-30000,30000,0] type:#integer scale:0.5
			label labStop "Stop" pos:[82,287] width:32 height:14
			spinner spnStop "" pos:[72,303] width:56 height:16 range:[-30000,30000,0] type:#integer scale:0.5
			button btnDelete "Delete" pos:[140,300] width:50 height:20
			
			label labObjects "None" pos:[8,323] width:70 height:25
			button btnObject "Objects" pos:[140,327] width:50 height:20
			
			label labPrefixName "Prefix:" pos:[8,358]
			edittext edPrefixName "" pos:[70,355] fieldWidth:118 height:1
			
			label labAnimName "Anim Name:" pos:[8,378]
			edittext edAnimName "" pos:[70,375] fieldWidth:118 height:1
			 
			button btnSaveToScene "S A V E   T O   S C E N E" pos:[8,408] width:184 height:20
			button btnSaveToMaxFile "S A V E   T O   M A X    F I L E" pos:[8,438] width:184 height:20
			
			
			
			
			
			
			on subRangeExport open do
			(
				cryTools.cryAnim.UI.batchProcess._f.subRangeLoadFromDummy()
				
				
				if $selection.count > 0 then
				(
					if (queryBox "Load current Selection into the List?" title:"Sub-Range-Export") == true then
					(
						cryTools.cryAnim.UI.batchProcess._f.subRangeSetList (selection as Array)
						cryTools.cryAnim.UI.batchProcess._f.subRangeUpdateValues()
						return true
					)
				)
			)
			
			
			on btnAddObjects pressed do
			(
				if $selection.count > 0 then
				(
					if (queryBox "Load current Selection into the List?" title:"Sub-Range-Export") == true then
					(
						cryTools.cryAnim.UI.batchProcess._f.subRangeSetList (selection as Array)
						lbObjects.selection = 1
						cryTools.cryAnim.UI.batchProcess._f.subRangeUpdateValues()
						return true
					)
				)
				
				if (local tempSel = selectByName title:"Select Nodes to be Added") != undefined then
					cryTools.cryAnim.UI.batchProcess._f.subRangeSetList tempSel
				
				lbObjects.selection = 1
				cryTools.cryAnim.UI.batchProcess._f.subRangeUpdateValues()
			)
			
			
			
			on btnDelete pressed do
			(
				if lbObjects.selection != 0 then
				(
					deleteItem cryTools.cryAnim.UI.batchProcess._v.exportFiles[cryTools.cryAnim.UI.batchProcess._v.selectedFile].subRanges lbObjects.selection
					cryTools.cryAnim.UI.batchProcess._f.subRangeUpdateList()
					
					if lbObjects.selection <= 0 then
						lbObjects.selection = lbObjects.items.count
					
					cryTools.cryAnim.UI.batchProcess._f.subRangeUpdateValues()
				)
			)
			
			
			on btnObject pressed do
			(
				if lbObjects.selection != 0 then
				(
					local tempSel = selectByName title:"Select Nodes"
					local newObjects = #()
					
					if tempSel != undefined then
					(
						for i = 1 to tempSel.count do
							tempSel[i] = tempSel[i].name
						
						cryTools.cryAnim.UI.batchProcess._v.exportFiles[cryTools.cryAnim.UI.batchProcess._v.selectedFile].subRanges[lbObjects.selection].objects = tempSel
						cryTools.cryAnim.UI.batchProcess._f.subRangeUpdateRange lbObjects.selection
						cryTools.cryAnim.UI.batchProcess._f.subRangeUpdateList()
						cryTools.cryAnim.UI.batchProcess._f.subRangeUpdateValues()
					)
				)
			)
			
			
			on lbObjects selected value do
			(
				cryTools.cryAnim.UI.batchProcess._f.subRangeUpdateValues()
			)
			
			
			on spnStart changed value do
			(	
				if lbObjects.selection != 0 then
				(
					cryTools.cryAnim.UI.batchProcess._v.exportFiles[cryTools.cryAnim.UI.batchProcess._v.selectedFile].subRanges[lbObjects.selection].range.start = value
					cryTools.cryAnim.UI.batchProcess._f.subRangeUpdateList()
				)
			)
			
			
			on spnStop changed value do
			(	
				if lbObjects.selection != 0 then
				(	
					cryTools.cryAnim.UI.batchProcess._v.exportFiles[cryTools.cryAnim.UI.batchProcess._v.selectedFile].subRanges[lbObjects.selection].range.end = value
					cryTools.cryAnim.UI.batchProcess._f.subRangeUpdateList()
				)
			)
			
			
			on edPrefixName changed value do
			(
				cryTools.cryAnim.UI.batchProcess._v.exportFiles[cryTools.cryAnim.UI.batchProcess._v.selectedFile].subRangePrefix = value
				--cryTools.cryAnim.UI.batchProcess._f.subRangeUpdateList()
			)
			
			
			
			
			on edAnimName changed value do
			(
				if lbObjects.selection != 0 then
				(
					cryTools.cryAnim.UI.batchProcess._v.exportFiles[cryTools.cryAnim.UI.batchProcess._v.selectedFile].subRanges[lbObjects.selection].export = value
					cryTools.cryAnim.UI.batchProcess._f.subRangeUpdateList()
				)
			)
			
			
			
			
			
			
			
			
			
			on btnSaveToScene pressed do
			(
				if lbObjects.items.count == 0 and spnStart.value == 0 and spnStop.value == 0 and edPrefixName.text == "" and edAnimName.text == "" then
				(
					if (getNodeByName "subRangeDummy") != undefined then
						if (queryBox "Delete subRange Dummy in current Scene ?" title:"CryAnim Batch Process") == true then
							try delete (getNodeByName "subRangeDummy") catch()
				)
				else
				(
					if $subRangeDummy == undefined then
					(
						local saveDummy = dummy name:"subRangeDummy"
						saveDummy.isHidden = true
					)
					else
						local saveDummy = getNodeByName "subRangeDummy"
					
					
					local UDPString = cryTools.cryAnim.UI.batchProcess._f.subRangeCreateUDPString()
					local importANM = true
					
					if cryTools.cryAnim.UI.batchProcess._v.flags[10] == false then
						local importANM = queryBox "Overwrite current Scene ?" title:"CryAnim Batch Process"
					
						
					
					if importANM == true then
						setUserPropBuffer saveDummy UDPString
				)
			)
			
			
			
			
			on btnSaveToMaxFile pressed do
			(
				try
				(
					if (queryBox "Save .anm settings to selected .max file ?\n\n(Loads selected max file and saves settings)" title:"CryAnim Batch Process") == true then
					(
						local tempSel = cryTools.cryAnim.UI.batchProcess._v.selectedFile
						
						
						cryTools.cryAnim.UI.batchProcess._v.flags[10] = true
						
						cryTools.cryAnim.UI.batchProcess.dialog.rollouts[2].btnLoadFile.pressed()
						
						
						if $subRangeDummy == undefined then
						(
							local saveDummy = dummy name:"subRangeDummy"
							saveDummy.isHidden = true
						)
						else
							local saveDummy = getNodeByName "subRangeDummy"
						
						
						local UDPString = cryTools.cryAnim.UI.batchProcess._f.subRangeCreateUDPString importIndex:tempSel
						local importANM = true
						
						
						setUserPropBuffer saveDummy UDPString
						
						
						cryTools.cryAnim.UI.batchProcess.dialog.rollouts[2].btnImportANM.pressed()
						
						cryTools.cryAnim.UI.batchProcess._v.flags[10] = false
					
					)
				)
				catch()
			)
			
		)
		cryTools.cryAnim.UI.batchProcess.subRangeDialog = subRangeRO
		subRangeRO = undefined
		
		local newDialogPos = cryTools.cryAnim.UI.batchProcess.dialog.pos - [205,-23]		
		createDialog cryTools.cryAnim.UI.batchProcess.subRangeDialog 200 464 newDialogPos[1] newDialogPos[2] style:#(#style_border)
	)
	catch ( logOutput "!!> Error in cryAnim.UI.batchProcess._f.subRangeCallDialog" )
)
subRangeCallDialog = undefined
logOutput "> Created cryAnim.UI.batchProcess._f.subRangeCallDialog function"


















--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.batchProcess._f.addNode
----------------------------------------------------------------------------------------------------------------------------------
--			adds children to an array
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.batchProcess._f.addNode = function addNode treeView tempNode tempChildren =
(
	try
	(
		for child in tempChildren do
		(
				--// creates an string array of all subfolders
			tempString = filterString child "\\"
			tempString = tempString[tempString.count]
			
				--// adds the last sub folder to the visible sub folders list
			if classOf treeView == ActiveXControl then
			(
				local newNode = treeView.Nodes.add tempNode.index 4 "" tempString
					--// sorts the entry
				newNode.sorted = true
			)
			else
				local newNode = tempNode.Nodes.add tempString tempString
			
				--// adds the whole path to the seperate sub folders list
			append cryTools.cryAnim.UI.batchProcess._v.subFolders (subFolderStruct path:child node:newNode)

				
				--// gets all directories in the current sub folder
			folderArray = getDirectories (child + "*")
			cryTools.cryAnim.UI.batchProcess._f.addNode treeView newNode folderArray
		)
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess._f.addNode" )
)
addNode = undefined
logOutput "> Created cryTools.cryAnim.UI.batchProcess._f.addNode function"









--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.batchProcess._f.updateSubFolderSelection
----------------------------------------------------------------------------------------------------------------------------------
--			save and get all checked entries into the ini file
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.batchProcess._f.updateSubFolderSelection = function updateSubFolderSelection op =
(
	try
	(
		case op of
		(
			#set:
			(
				local tempString = "#"
				for i = 1 to cryTools.cryAnim.UI.batchProcess._v.subFolders.count do
				(
					if cryTools.cryAnim.UI.batchProcess._v.subFolders[i].node.checked == true then
						tempString += (cryTools.cryAnim.base.lowercase cryTools.cryAnim.UI.batchProcess._v.subFolders[i].path) + "#"
				)
		
				cryTools.cryAnim.base.iniFile #set #batchProcessSubFolderSelection value:tempString
			)
			
			#get:
			(
				local tempVar = cryTools.cryAnim.base.iniFile #get #batchProcessSubFolderSelection
				if tempVar != "" then
				(
					local tempFilter = filterString tempVar "#"
					
					cryTools.cryAnim._v.various[120] = true
					
					for i = 1 to cryTools.cryAnim.UI.batchProcess._v.subFolders.count do
					(
						local oldFolder = cryTools.cryAnim.base.lowercase cryTools.cryAnim.UI.batchProcess._v.subFolders[i].path
						if (index = findItem tempFilter oldFolder) != 0 then
						(
							local tempNode = cryTools.cryAnim.UI.batchProcess._v.subFolders[i].node
							tempNode.checked = true
							try
							(
								local parentNode = tempNode.parent
								
								while parentNode != undefined do
								(
									parentNode.expanded = true
									parentNode = parentNode.parent
								)
							)catch()
						)
					)
					cryTools.cryAnim._v.various[120] = undefined
				)
			)
		)
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess._f.updateSubFolderSelection" )
)
updateSubFolderSelection = undefined
logOutput "> Created cryTools.cryAnim.UI.batchProcess._f.updateSubFolderSelection function"








--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.batchProcess._f.getCheckedNodes
----------------------------------------------------------------------------------------------------------------------------------
--			clears the sub folder list and adds new entries
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.batchProcess._f.getCheckedNodes = function getCheckedNodes =
(
	try
	(
		local tempArray = #()
		
		for i = 1 to cryTools.cryAnim.UI.batchProcess._v.subFolders.count do
		(	
			if cryTools.cryAnim.UI.batchProcess._v.subFolders[i].node.checked == true then
				append tempArray i
		)
		return tempArray
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess._f.getCheckedNodes" )
)
getCheckedNodes = undefined
logOutput "> Created cryTools.cryAnim.UI.batchProcess._f.getCheckedNodes function"









--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.batchProcess._f.updateSubFolders
----------------------------------------------------------------------------------------------------------------------------------
--			clears the sub folder list and updates it with new entries
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.batchProcess._f.updateSubFolders = function updateSubFolders =
(
	try
	(
		local lbSubFolders = cryTools.cryAnim.UI.batchProcess.dialog.rollouts[1].lbSubFolders
			--// clears lists
		lbSubFolders.Nodes.clear()
		cryTools.cryAnim.UI.batchProcess._v.subFolders = #()

			--// if there is a folder selected in rollout[2]
		if findString cryTools.cryAnim.UI.batchProcess._v.sourcePath "\\" != undefined then
		(
			
				--// gets latest sub folder name for the root of the sub folders list
			rootText = filterString cryTools.cryAnim.UI.batchProcess._v.sourcePath "\\"
			rootText = rootText[rootText.count]

				--// adds a new entry to the sub folders list
			if classOf lbSubFolders != ActiveXControl then
			(
				root = lbSubFolders.Nodes.add rootText rootText
				root.tag = rootText
			)
			else
			(
				root = lbSubFolders.Nodes.add()
				root.text = rootText
			)
			
				--// adds the full path of the root folder to the seperate list
			append cryTools.cryAnim.UI.batchProcess._v.subFolders (subFolderStruct path:cryTools.cryAnim.UI.batchProcess._v.sourcePath node:root)
		
			if classOf lbSubFolders == ActiveXControl then
			(
				if (findString (cryTools.cryAnim.base.iniFile #get #batchProcessSubFolderSelection) "\\") == undefined then
				(
					root.checked = true
					cryTools.cryAnim.UI.batchProcess._v.subFolders[cryTools.cryAnim.UI.batchProcess._v.subFolders.count].checked = true
				)
			)
				--// gets sub directories of the root folder
			folderArray = getDirectories (cryTools.cryAnim.UI.batchProcess._v.sourcePath + "*")
				--// adds all sub folders in sub folders to the list
			cryTools.cryAnim.UI.batchProcess._f.addNode lbSubFolders root folderArray
			
			if classOf lbSubFolders == ActiveXControl then
			(
				root.Expanded = true
				root.Bold = true
			)
			else
				root.Expand()
		)
		
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess._f.updateSubFolders" )
)
updateSubFolders = undefined
logOutput "> Created cryTools.cryAnim.UI.batchProcess._f.updateSubFolders function"










--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.batchProcess._f.updateCounter
----------------------------------------------------------------------------------------------------------------------------------
--			gets all selected or unselected files and updates the count string
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.batchProcess._f.updateCounter = function updateCounter =
(
	try
	(
		counter = 0
		maxFiles = 0
		tempText = "Count :"
		
		for i = 1 to cryTools.cryAnim.UI.batchProcess._v.exportFiles.count do
		(				
			local tempVar = cryTools.cryAnim.UI.batchProcess._v.exportFiles[i]
			
			if tempVar.extension != "folder" then --and tempVar.extension != "scene" then
			(
				maxFiles += 1
								
				cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].used = tempVar.node.selected
				
				if tempVar.node.selected == true then
					counter += 1
				else
				(
					if tempVar.extension == "scene" then
						maxFiles -= 1
				)
			)
			else
				cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].used = false
			
		)
			
			--// if nothing is selected or everything is selected
		if (counter == 0) or (counter == maxFiles) then
		(
				--// go through the files list (scene entry only, if selected) and set used to true
			for i = (cryTools.cryAnim.UI.batchProcess._v.exportFiles.count - maxFiles + 1) to cryTools.cryAnim.UI.batchProcess._v.exportFiles.count do
				cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].used = true
			
				--// no addition to the counter string
			counter = ""
		)
			--// if something is selected
		else
				--// add a seperator to the counter string
			counter = counter as String + " / "
		
			--// update the count string with the "Count :" and the selected files (if some were selected) and the maximum file count
		cryTools.cryAnim.UI.batchProcess.dialog.rollouts[3].labCount.text = tempText + "   " + counter + maxFiles as String
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess._f.updateCounter" )
)
updateCounter = undefined
logOutput "> Created cryTools.cryAnim.UI.batchProcess._f.updateCounter function"









--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.batchProcess._f.getScriptList
----------------------------------------------------------------------------------------------------------------------------------
--			gets the list of all custom scripts in the batch directory
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.batchProcess._f.getScriptList = function getScriptList scriptPath =
(
	try
	(
		local tempArray = getFiles scriptPath
		local tempEntries = #()
		
		if tempArray.count > 0 then
		(
			
			for i = 1 to tempArray.count do
			(
				local tempFilter = filterString tempArray[i] "\\"
				local tempString = (filterString tempFilter[tempFilter.count] ".")[1]
				
				
				append tempEntries tempString
			)
		)

		append tempEntries "----------------------------------------------"
		append tempEntries "Cusomize Scripts"

		return tempEntries
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess._f.getScriptList" )
)
getScriptList = undefined
logOutput "> Created cryTools.cryAnim.UI.batchProcess._f.getScriptList function"









--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.batchProcess._f.updateScriptLists
----------------------------------------------------------------------------------------------------------------------------------
--			updates the list of scripts in the customize script drob-down lists
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.batchProcess._f.updateScriptLists = function updateScriptLists scriptUpdate:undefined =
(
	try
	(
		local ddOp1 = cryTools.cryAnim.UI.batchProcess.dialog.rollouts[2].ddOp1
		local ddOp2 = cryTools.cryAnim.UI.batchProcess.dialog.rollouts[2].ddOp2
		local ddOp3 = cryTools.cryAnim.UI.batchProcess.dialog.rollouts[2].ddOp3
		local ddOp4 = cryTools.cryAnim.UI.batchProcess.dialog.rollouts[2].ddOp4
		
		ddOp1.items = cryTools.cryAnim.UI.batchProcess._f.getScriptList (cryTools.buildpathfull + "Tools\\maxscript\\cryAnim\\ui\\batch\\Scripts\\*.ms")
		ddOp2.items = cryTools.cryAnim.UI.batchProcess._f.getScriptList (cryTools.buildpathfull + "Tools\\maxscript\\cryAnim\\ui\\batch\\Scripts\\*.ms")
		ddOp3.items = cryTools.cryAnim.UI.batchProcess._f.getScriptList (cryTools.buildpathfull + "Tools\\maxscript\\cryAnim\\ui\\batch\\Scripts\\*.ms")
		ddOp4.items = cryTools.cryAnim.UI.batchProcess._f.getScriptList (cryTools.buildpathfull + "Tools\\maxscript\\cryAnim\\ui\\batch\\Scripts\\*.ms")
		
		--ddSecondOp.items = cryTools.cryAnim.UI.batchProcess._f.getScriptList (cryTools.buildpathfull + "Tools\\maxscript\\cryAnim\\ui\\batch\\SecondScript\\*.ms")
		
		if scriptUpdate == undefined then
		(
			ddOp1.selection = ddOp1.items.count - 1
			ddOp2.selection = ddOp2.items.count - 1
			ddOp3.selection = ddOp3.items.count - 1
			ddOp4.selection = ddOp4.items.count - 1
		)
		else
		(
			local tempFilter = filterString scriptUpdate "|"
			
			if tempFilter.count > 1 then
			(
				case tempFilter[1] of
				(
					"1": ddOp1.selection = (findItem ddOp1.items tempFilter[2])
					"2": ddOp2.selection = (findItem ddOp2.items tempFilter[2])
					"3": ddOp3.selection = (findItem ddOp3.items tempFilter[2])
					"4": ddOp4.selection = (findItem ddOp4.items tempFilter[2])
				)
			)
			else
			(
				case tempFilter[1] of
				(
					"1": ddOp1.selection = ddOp1.items.count
					"2": ddOp2.selection = ddOp2.items.count
					"3": ddOp3.selection = ddOp3.items.count
					"4": ddOp4.selection = ddOp4.items.count
					"None":
					(
						--ddFirstOp.selection = (findItem ddFirstOp.items cryTools.cryAnim._v.various[33])
						--ddSecondOp.selection = (findItem ddSecondOp.items cryTools.cryAnim._v.various[34])
					)
				)
			)
		)
			
		return true
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess._f.updateScriptLists" )
)
updateScriptLists = undefined
logOutput "> Created cryTools.cryAnim.UI.batchProcess._f.updateScriptLists function"








--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.batchProcess._f.executeCustomScripts
----------------------------------------------------------------------------------------------------------------------------------
--			creates folder, runs pre-export script and/or exports the file, generates log file
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.batchProcess._f.executeCustomScripts = function executeCustomScripts op obj =
(
	try
	(
		local tempScript = cryTools.cryAnim.UI.batchProcess._v.customScripts[(op as Integer)]
			
	
		--local tempMaxCount = obj.items.count
		--local tempIndex = obj.selection
		--local tempSelected = obj.items[tempIndex]
		
		
		if obj.selection == obj.items.count then
		(
			if tempScript != "" then
			(
					--// try to execute it, otherwise print out the error message
				try
				(		
					execute( tempScript )
					gc()
				)
				catch
				(
					format "*** % ***\n" (getCurrentException())
					break
				)
			)
		)
		
		if obj.selection < (obj.items.count - 1) then
		(
			local tempString = (cryTools.buildPathFull + "Tools\\maxscript\\cryAnim\\ui\\batch\\Scripts\\" + obj.items[obj.selection] + ".ms")
			try ( fileIn tempString )
			catch
			(
				print ("Can't execute " + tempString)
				return false
			)
		)
		return true
		
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess._f.executeCustomScripts" )
)
executeCustomScripts = undefined
logOutput "> Created cryTools.cryAnim.UI.batchProcess._f.executeCustomScripts function"







--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.batchProcess._f.getAllFolders
----------------------------------------------------------------------------------------------------------------------------------
--			returns an array of all folders selected in the sub folders list
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.batchProcess._f.getAllFolders = function getAllFolders =
(
	try
	(
		local folderArray = #()
		local nodeArray = cryTools.cryAnim.UI.batchProcess._f.getCheckedNodes()
		
			--// for entries in the sub folders list
		for i = 1 to nodeArray.count do
				--// if the entry is checked, add the entry to the folderArray
			append folderArray cryTools.cryAnim.UI.batchProcess._v.subFolders[nodeArray[i]].path
				
			--// return checked sub folders
		return folderArray
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess._f.getAllFolders" )
)
getAllFolders = undefined
logOutput "> Created cryTools.cryAnim.UI.batchProcess._f.getAllfolders function"






--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.batchProcess._f.getAllFiles
----------------------------------------------------------------------------------------------------------------------------------
--			returns an array of all files of the checked folders in sub folders list
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.batchProcess._f.getAllFiles = function getAllFiles = 
(
	try
	(
		local fileArray = #()
			--// gets all checked sub folders
		local folderArray = cryTools.cryAnim.UI.batchProcess._f.getAllFolders()
			--// for all sub folders
		for i = 1 to folderArray.count do
		(
				--// Add subFolder to list
			local tempArray = #(("##" + folderArray[i]))
				--// Add subFolderFiles to list
			join tempArray (getFiles (folderArray[i] + "*.*" ))
				--// Add subFolder + Files to general files list
			join fileArray tempArray

		)
					
		return fileArray
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess._f.getAllFiles" )
)
getAllFiles = undefined
logOutput "> Created cryTools.cryAnim.UI.batchProcess._f.getAllFiles function"









--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.batchProcess._f.getBoneDetection
----------------------------------------------------------------------------------------------------------------------------------
--			compares the filename with the file parts specified in the boneList and returns the result if the bone is in the file name
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.batchProcess._f.getBoneDetection = function getBoneDetection filename =
(
	try
	(
		local resultArray = #()
		local resultActive = false
			--// for all items in the bone list
		for i = 1 to cryTools.cryAnim.UI.batchProcess._v.boneList.count do
		(
			local tempString = ""
				--// if the entry is activated
			if cryTools.cryAnim.UI.batchProcess._v.boneList[i].active == "true" then
			(
					--// entry is activated and so there is something to use
				resultActive = true
					--// gets all the parts of the file parts to compare
				local tempArray = filterString cryTools.cryAnim.UI.batchProcess._v.boneList[i].external ";"

				local foundBones = false
					--// for all items in the file parts					
				for d = 1 to tempArray.count do
						--// if the file part is in the filename string
					if findString filename tempArray[d] != undefined then
							--// bone entry was found and
						foundBones = true
				
					--// if an entry was found
				if foundBones == true then
						--// add the index and name to the result
					append resultArray (resultStruct index:i name:cryTools.cryAnim.UI.batchProcess._v.boneList[i].name)
			)
		)
			--// if there is no entry in the bone list is checked
		if resultActive == false then
				--// return just false
			resultArray = #(false)
			--// otherwise return the result array
		return resultArray
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess._f.getBoneDetection" )
)
getBoneDetection = undefined
logOutput "> Created cryTools.cryAnim.UI.batchProcess._f.getBoneDetection function"









--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.batchProcess._f.getFilename
----------------------------------------------------------------------------------------------------------------------------------
--			returns the filename of the full path
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.batchProcess._f.getFilename = function getFilename filepath =
(
	try
	(
			--// return new filename
		return (filterString (cryTools.cryAnim.base.perforce filepath #getFilename) ".")[1]
		
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess._f.getFilename" )
)
getFilename = undefined
logOutput "> Created cryTools.cryAnim.UI.batchProcess._f.getFilename function"








--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.batchProcess._f.getExtension
----------------------------------------------------------------------------------------------------------------------------------
--			returns the extension of a filepath
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.batchProcess._f.getExtension = function getExtension filepath =
(
	try
	(
			--// if a folder is found
		--if findString filepath "\\" != undefined and findString filepath "." == undefined then
			--return "folder"
		
		if (findString filepath "##") != undefined then
			return "folder"
			
			--// divides the string by a "."
		local tempFilterString = filterString filepath "."
			
			--// returns the last part of the filepath (extension)
		return tempFilterString[tempFilterString.count]
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess._f.getExtension" )
)
getExtension = undefined
logOutput "> Created cryTools.cryAnim.UI.batchProcess._f.getExtension function"









--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.batchProcess._f.findMaskEntry
----------------------------------------------------------------------------------------------------------------------------------
--			returns a mask struct with all the entries, in which the filename is part of
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.batchProcess._f.findMaskEntry = function findMaskEntry tempMask tempString  =
(
	try
	(
		local tempArray = #(false, false, false, false)
						
			--// if the normal criteria is not empty
		if tempMask.normal[1] != undefined then
		(
				--// file is part in normal
			tempArray[1] = true
			for i = 1 to tempMask.normal.count do
			(
					--// if the there is a missmatch in the file criteria
				if (findString tempString tempMask.normal[i]) == undefined then
						--// file is not part in normal
					tempArray[1] = false												
			)
		)
			--// normal criteria is empty
		else
				--// file is not part in normal criteria
			tempArray[1] = undefined
								
			--// goes through the addition criteria				
		for i = 1 to tempMask.add.count do
		(
				--// if the criteria is not empty
			if tempMask.add[i] != undefined then
					-- // if the criteria is found in the filename
				if (findString tempString tempMask.add[i]) != undefined then
						--// file is part in addition
					tempArray[2] = true
		)
		
			--// goes through the substraction criteria		
		for i = 1 to tempMask.remove.count do
		(
				--// if the criteria is not empty
			if tempMask.remove[i] != undefined then
					--// if the criteria is found in the filename
				if (findString tempString tempMask.remove[i]) != undefined then
						--// file is part in substraction
					tempArray[3] = true
		)
			--// if there is a extension criteria defined
		if tempMask.extension[1] != undefined then
		(
				--// goes through the extension criteria
			for i = 1 to tempMask.extension.count do
			(
					--// if criteria is not empty
				if tempMask.extension[i] != undefined then
				(
						--// seperates the filepath and the extension
					tempExtension = filterString tempString "."
						--// and gets the extension
					tempExtension = tempExtension[tempExtension.count]
						--// if the extension criteria is found in the file
					if (findString tempExtension tempMask.extension[i]) != undefined then
						--// file is part in extension criteria
						tempArray[4] = true
				)
			)
		)
			--// if there is no extension criteria defined
		else
			tempArray[4] = undefined
			
			--// return the list of founded criterias																											
		return tempArray
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess._f.findMaskEntry" )
)
findMaskEntry = undefined
logOutput "> Created cryTools.cryAnim.UI.batchProcess._f.findMaskEntry function"












--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.batchProcess._f.getFileFound
----------------------------------------------------------------------------------------------------------------------------------
--			returns true, if the file matchs the file mask criteria and other checks
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.batchProcess._f.getFileFound = function getFileFound filename =
(
	try
	(
			--// creates an empty mask struct
		fileMask = maskStruct()
			--// seperates all entries in the file mask and stores them as an array
		fileMaskFilter = filterString cryTools.cryAnim.UI.batchProcess.dialog.rollouts[2].etFileMask.text " "
			--// end result which will be returned
		fileMaskFound = false
		
			--// goes through the sperated mask entries
		for f = 1 to fileMaskFilter.count do
		(
				--// if a substract letter is found, put the part of the string into the remove part
			if findString fileMaskFilter[f] "#" != undefined then
				append fileMask.remove (substring fileMaskFilter[f] 2 fileMaskFilter[f].count)
				
				--// if an addition letter is found, put the part of the string into the addition part
			if findString fileMaskFilter[f] "*" != undefined then
				append fileMask.add (substring fileMaskFilter[f] 2 fileMaskFilter[f].count)

				--// if an additon letter and a substract letter is not found, put the part of the string into the normal part
			if (findString fileMaskFilter[f] "*" == undefined) and (findString fileMaskFilter[f] "#" == undefined) and (findString fileMaskFilter[f] "$" == undefined) then
				append fileMask.normal fileMaskFilter[f]
				
				--// if a file type letter is found, put the part of the string into the extension part
			if findString fileMaskFilter[f] "$" != undefined then												
				append fileMask.extension (cryTools.cryAnim.base.lowercase(substring fileMaskFilter[f] 2 fileMaskFilter[f].count))
		)


			--// gets the result, if the file is part of the substract, add, normal or extension criteria
		maskArray = cryTools.cryAnim.UI.batchProcess._f.findMaskEntry fileMask filename


			--// if the file extension is found
		if maskArray[4] != false then
		(
				--// if the file is part in normal or is not specified then file is found
			if (maskArray[1] == true or maskArray[1] == undefined) then
					--// file is found
				fileMaskFound = true
			else
					--// otherwise not
				fileMaskFound = false
			
				--// if the file is part in substraction
			if maskArray[3] == true then
					--// file is hidden
				fileMaskFound = false
				--// if the file is part in addition
			if maskArray[2] == true then
					--// file is found
				fileMaskFound = true
		)
			--// if the file extension is not found
		else
				--// file is hidden
			fileMaskFound = false
			
			--// returns the result if the file is visible or not
		return fileMaskFound
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess._f.getFileFound" )
)
getFileFound = undefined
logOutput "> Created cryTools.cryAnim.UI.batchProcess._f.getFileFound function"










--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.batchProcess._f.refreshFiles
----------------------------------------------------------------------------------------------------------------------------------
--			clears the file list, fills it with found files and sets the extent for the columns
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.batchProcess._f.refreshFiles = function refreshFiles =
(
	try
	(
			--// empty array of column text extents
		local newExtent = #(0,0,0,0,0)
		local lbFiles = cryTools.cryAnim.UI.batchProcess.dialog.rollouts[1].lbFiles
			--// clears the file list
		
		if cryTools.cryAnim.UI.batchProcess._v.flags[1] == true then
		(
			if classOf lbFiles == ActiveXControl then
				lbFiles.listItems.clear()
			else
				lbFiles.items.clear()
		)

		
		
			--// goes through the file list
		for i = 1 to cryTools.cryAnim.UI.batchProcess._v.exportFiles.count do
		(
			local lbFilesFile = ""
			local foreColor = [0,0,0]
			
			if cryTools.cryAnim.UI.batchProcess._v.flags[1] == true then
			(
					--// adds the line with the name
				if classOf lbFiles == ActiveXControl then
				(
					lbFilesFile = lbFiles.listItems.Add text:cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].filename
										--// adds the Details
					lbFilesFile.listSubItems.Add text:cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].details
						--// adds the Export
					lbFilesFile.listSubItems.Add text:""
						--// adds the Extension
					lbFilesFile.listSubItems.Add text:cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].extension
						--// adds the Anm
					lbFilesFile.listSubItems.Add text:cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].anm
				)
				else
				(
					lbFilesFile = lbFiles.items.Add cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].filename cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].filename
					lbFilesFile.subItems.Add cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].details
					lbFilesFile.subItems.Add ""
					lbFilesFile.subItems.Add cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].extension
					lbFilesFile.subItems.Add cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].anm
				)
			)
			else
			(
				if classOf lbFiles == ActiveXControl then
				(
					lbFilesFile = lbFiles.listItems[i]
					lbFilesFile.listSubItems[1].text = cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].details
					lbFilesFile.listSubItems[2].text = cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].process
					lbFilesFile.listSubItems[4].text = cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].anm
				)
				else
				(
					lbFilesFile = lbFiles.items.item (i - 1)
					(lbFilesFile.subItems.get_item 1).text = cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].details
					(lbFilesFile.subItems.get_item 2).text = cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].process
					(lbFilesFile.subItems.get_item 4).text = cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].anm
				)
			)	
			
			cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].node = lbFilesFile
			
			local textExtent = #(0,0,0,0,0)
			
			
			if classOf lbFiles == ActiveXControl then
			(
					--// if a the "file" is a folder
				case cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].extension of
				(
					"folder": lbFilesFile.Bold = true
					"scene": lbFilesFile.Bold = true
				)
				
					--// extent of the current filename
				textExtent[1] = ((getTextExtent cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].filename)[1]) * 30
					--// extent of the current details
				textExtent[2] = ((getTextExtent cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].details)[1]) * 40
					--// extent of the current export
				textExtent[3] = ((getTextExtent cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].process)[1]) * 40
					--// extent of the current extension
				textExtent[4] = ((getTextExtent cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].extension)[1]) * 50
					--// extent of the current .anm
				textExtent[5] = ((getTextExtent cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].anm)[1]) * 50

					--// goes through the extent list
				for f = 1 to textExtent.count do
						--// if the current extent is higher than the result extent
					if textExtent[f] > newExtent[f] then
								--// result extent is current extent
							newExtent[f] = textExtent[f]
				
					--// files which are selected before will be selected again
			)
			else
			(
					--// adds the Details
				if cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].details != "" then
				(
					if (findString cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].details ";") != undefined then
						foreColor = [200,150,50]
					else
						foreColor = [0,0,150]
				)
					--// adds the Process
				lbFilesFile.subItems.Add cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].process
				if cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].process != "" then
				(	
					if cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].process == "Done" then
						foreColor = [0,200,0]
					else
						foreColor = [255,0,0]
					
					if cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].process == "Load" then
						foreColor = [0,0,255]
				)
				
					--// adds the Extension
				case cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].extension of
				(
					"folder": foreColor = [200,0,0]
					"scene": foreColor = [0,0,200]
				)	
				

				lbFilesFile.ForeColor = lbFilesFile.ForeColor.FromArgb foreColor[1] foreColor[2] foreColor[3]

					--// extent of the current filename
				textExtent[1] = ((getTextExtent cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].filename)[1])
					--// extent of the current details
				textExtent[2] = ((getTextExtent cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].details)[1])
					--// extent of the current process
				textExtent[3] = ((getTextExtent cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].process)[1])
					--// extent of the current extension
				textExtent[4] = ((getTextExtent cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].extension)[1])
					--// extent of the current .anm
				textExtent[5] = ((getTextExtent cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].anm)[1])

					--// goes through the extent list
				for f = 1 to textExtent.count do
						--// if the current extent is higher than the result extent
					if textExtent[f] > newExtent[f] then
								--// result extent is current extent
							newExtent[f] = textExtent[f] + 12 --// additional number so entries are not cut
			
			)
		)
			
	
	
			--// goes through the result extent list
		for f = 1 to newExtent.count do
		(
				--// applys the weidth of the column with the text extent
			if classOf lbFiles == ActiveXControl then
				lbFiles.columnHeaders[f].width = newExtent[f]
			else
				(lbFiles.Columns.item (f - 1)).width = newExtent[f]
		)
			
			
		cryTools.cryAnim.UI.batchProcess._v.flags[1] = false
		
			 --// updates the counter
		cryTools.cryAnim.UI.batchProcess._f.updateCounter()
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess._f.refreshFiles" )
)
refreshFiles = undefined
logOutput "> Created cryTools.cryAnim.UI.batchProcess._f.refreshFiles function"










--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.batchProcess._f.updateDialog
----------------------------------------------------------------------------------------------------------------------------------
--			updates the whole dialog analyze the file mask and files
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.batchProcess._f.updateDialog = function updateDialog updateFromExport:false =
(
	try
	(
			--// if the function is called after the process file function
		if updateFromExport == false then
				--// don't clear the file list, as it stores the export results
			cryTools.cryAnim.UI.batchProcess._v.exportFiles = #()
		

			--// if the source path is not empty							
		if cryTools.cryAnim.UI.batchProcess._v.sourcePath != undefined then
		(
				--// gets all selected folders
			local folderArray = cryTools.cryAnim.UI.batchProcess._f.getAllFolders()
				--// creates array with the scene entry first
			local fileArray = #(" ---   Current Scene   ---.scene")
				--// attaches all files in the selected folders
			join fileArray (cryTools.cryAnim.UI.batchProcess._f.getAllFiles())
			
			
				--// goes through all files in the folders
			for i = 1 to fileArray.count do
			(
				tempFilename = #()
					--// gets the filename with extension
				tempFilename[1] = cryTools.cryAnim.base.perforce fileArray[i] #getFilename
					--// gets filename without extension
				tempFilename[2] = cryTools.cryAnim.UI.batchProcess._f.getFilename fileArray[i]
					--// gets the whole file path
				tempFilename[3] = fileArray[i]


					--// checks, if the filename is part of the bone detection criteria
				boneFound = cryTools.cryAnim.UI.batchProcess._f.getBoneDetection tempFilename[2]
					--// checks, if the filename is part of show-criteria
				fileFound = cryTools.cryAnim.UI.batchProcess._f.getFileFound tempFilename[1]
				
				
					--// if file will be shown at the end
				fileUsed = false
					--// if the entry is a folder
				folderFound = false
					--// if the entry is the scene
				sceneFound = false
					--// if a bone criteria was detected
				tempDetail = ""
				
					--// if a folder is found (definition of a folder is ##)
				if (findString tempFilename[3] "##") != undefined then
				(
						--// gets all subfolders of the file
					tempFolderFilter = filterString tempFilename[3] "\\"
						--// gets all subfolders of the source path
					tempOpenFilter = filterString cryTools.cryAnim.UI.batchProcess._v.sourcePath "\\"
						--// seperator at the beginning
					tempFolderString = " ---   "
					
						--// differentiate the subfolders from source and file path
					for i = tempOpenFilter.count to tempFolderFilter.count do
							--// adds the sub folder to the temporary string
						tempFolderString += tempFolderFilter[i] + (if i != tempFolderFilter.count then " \\ " else "   ---")
						
						--// filename is now sub folder structure
					tempFilename[2] = tempFolderString
					
						--// folder is found
					folderFound = true
				)
				
				if (cryTools.cryAnim.UI.batchProcess._f.getExtension tempFilename[1]) == "scene" then
					sceneFound = true
			
				
				if folderFound == false then
				(
						--// if a boneDetection criteria is found
					if (boneFound.count > 0) and (boneFound[1] != false) then
					(
						local tempBoneString = ""
							--// goes through the criteria
						for f = 1 to boneFound.count do
								--// adds the name of the criteria
							tempBoneString += boneFound[f].name + (if f != boneFound.count then "; " else "")
						
							--// "Details" is bone criteria
						tempDetail = tempBoneString
					)
				)
				
				
				
				
					--// only bone detected files will be shown				
				if cryTools.cryAnim.UI.batchProcess.dialog.rollouts[3].chkOnlyBoneDetection.checked == true then
				(
						--// if a bone detection criteria is found
					if boneFound.count > 0 then						
					(
							--// if file is visible
						if fileFound == true then
								--// file will be used
							fileUsed = true
					)
				)
					--// if not only bone detected files will be shown
				else
				(
						--// if only not bone detected files be shown
					if cryTools.cryAnim.UI.batchProcess.dialog.rollouts[3].chkNoBoneDetection.checked == true then
					(
							--// if no bone detect criteria is found or no bone detect criteria is set up
						if (boneFound.count == 0) or (boneFound[1] == false) then							
						(
								--// if file is visible
							if fileFound == true then
									--// file will be used
								fileUsed = true
						)
					)
						--// if both, only and no bone detection is checked
					else
					(
							--// if file is visible
						if fileFound == true then
								--// file will be used
							fileUsed = true
					)
				)

				local tempExt = (cryTools.cryAnim.UI.batchProcess._f.getExtension tempFilename[3])
				case tempExt of
				(
					"bip":()
					"fbx":()
					"max":()
					"folder":()
					"scene":()
					default: (tempDetail = "File type not supported")
				)
				
				
					--// if file is visible or a folder is found
				if (fileFound == true) or (folderFound == true) or (sceneFound == true) then
				(
						--// if the update dialog is not called after the files where processed
					if updateFromExport == false then								
						if boneFound[1] == false or ((cryTools.cryAnim.UI.batchProcess.dialog.rollouts[3].chkOnlyBoneDetection.checked == true and boneFound.count > 0) or (cryTools.cryAnim.UI.batchProcess.dialog.rollouts[3].chkNoBoneDetection.checked == true and boneFound.count == 0) or (cryTools.cryAnim.UI.batchProcess.dialog.rollouts[3].chkOnlyBoneDetection.checked == false and cryTools.cryAnim.UI.batchProcess.dialog.rollouts[3].chkNoBoneDetection.checked == false)) or folderFound == true or sceneFound == true then
								--// add the file details to the internal files list
							append cryTools.cryAnim.UI.batchProcess._v.exportFiles (batchFileStruct filename:tempFilename[2] path:tempFilename[3] details:tempDetail process:"" used:fileUsed extension:(cryTools.cryAnim.UI.batchProcess._f.getExtension tempFilename[3]) anm:"")
						
				)
			)
			
				--// clears the list, fills it with files and sets text extent
			cryTools.cryAnim.UI.batchProcess._f.refreshFiles()
				--// updates the counter
			cryTools.cryAnim.UI.batchProcess._f.updateCounter()

		)
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess._f.updateDialog" )
)
updateDialog = undefined
logOutput "> Created cryTools.cryAnim.UI.batchProcess._f.updateDialog function"











--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.batchProcess._f.generateOutput
----------------------------------------------------------------------------------------------------------------------------------
--			creates and fills log file
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.batchProcess._f.generateOutput = function generateOutput op filepath =
(
	try
	(
			--// generates the log file and adds time tag
		if op == #generate then
		(
				--// seperates date and time
			tempLocalTime = filterString localTime " "
				--// gets date
			tempDate = tempLocalTime[1]
			
				--// if a different region other then GMT is set
			if tempLocalTime.count > 2 then
			(
					--// seperate hour, minute second
				tempTimeArray = filterString tempLocalTime[2] ":"
					--// if it is afternoon
				if tempLocalTime[3] == "PM" then
						--// converts 12h to 24h system
					tempTimeArray[1] = tempTimeArray[1] as Integer + 12
				
					--// sets 24h system
				tempLocalTime[2] = tempTimeArray[1] as String
				
					--// adds the rest of the time with ":"
				for index = 2 to tempTimeArray.count do
					tempLocalTime[2] += ":" + tempTimeArray[index]
				
				
			)
			
				--// sets new time
			tempTime = tempLocalTime[2]

				--// if non GMT is found			
			if findString tempDate "/" != undefined then
			(
					--// seperate month,day,year
				tempDateFilter = filterString tempDate "/"
					--// adds "." to the date and converts to GMT system
				tempDate = tempDateFilter[2] + "." + tempDateFilter[1] + "." + tempDateFilter[3]
			)											
			
			
				--// seperates hour,minute,second
			local timeExtension = filterString tempTime ":"
						
			tempString = ""
				--// goes through the list and replaces ":" with "'"
			for i = 1 to timeExtension.count do
			(
				tempString += timeExtension[i]
				if i != timeExtension.count then
					tempString += "'"
			)
			
				--// set new time string
			timeExtension = tempString
			
				--// batchProcess folder
			local filepath = cryTools.cryTemp + "batchAnimExport"
				--// if there is no folder, create one
			cryTools.cryAnim.UI.main._f.checkExport #createFolder filepath
			
				--// generates file path out of day and time
			filepath += "\\" + tempDate + "_" + timeExtension + ".txt"										
			
				--// if file is already there, delete it
			try ( deleteFile filepath ) catch()	
				--// create file and filestream														
			local fileStream = createFile filepath
			

				--// header of the log file: date, time, source folder and insert it to the file
			format "%      %\n===================================================================================================================\n\n" localTime cryTools.cryAnim.UI.batchProcess.dialog.rollouts[2].labSourceFolder.text  to:fileStream
				--// cleses file/filestream
			close filestream
			
				--// returns the log file path
			return filepath
		)
		
		
		
			--// if a file should be logged
		if op != #generate and op != undefined then
		(
				--// open the file
			local filestream = openFile filepath mode:"a+"
				--// add the file and status
			format "%\n" op to:filestream
				--// close file again
			close filestream
		)
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess._f.generateOutput" )
)
generateOutput = undefined
logOutput "> Created cryTools.cryAnim.UI.batchProcess._f.generateOutput function"












--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.batchProcess._f.updateExtent
----------------------------------------------------------------------------------------------------------------------------------
--			updates the extent of the text in pixel
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.batchProcess._f.updateExtent = function updateExtent =
(
	try
	(
		newExtent = #(0,0,0)
		local lbList = cryTools.cryAnim.UI.batchProcess.editBoneList.lbList
		
		for i = 0 to (lbList.Items.count - 1) do
		(
			local tempItem = lbList.Items.item i
			local textExtent = #(0,0,0)
			--// extent of the current filename
			textExtent[1] = (getTextExtent (tempItem.SubItems.item 0).text)[1] + 12
			textExtent[2] = (getTextExtent (tempItem.SubItems.item 1).text)[1]
			textExtent[3] = (getTextExtent (tempItem.SubItems.item 2).text)[1]
			
			for f = 1 to textExtent.count do
					--// if the current extent is higher than the result extent
				if textExtent[f] > newExtent[f] then
							--// result extent is current extent
					newExtent[f] = textExtent[f] + 12 --// additional number so entries are not cut
		)
		
		for f = 1 to newExtent.count do
			if (lbList.Columns.item (f - 1)).width < newExtent[f] then
				--// applys the weidth of the column with the text extent
				(lbList.Columns.item (f - 1)).width = newExtent[f]
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess._f.updateExtent" )
)
updateExtent = undefined
logOutput "> Created cryTools.cryAnim.UI.batchProcess._f.updateExtent function"










cryTools.cryAnim.UI.batchProcess._f.loadBatchFile = function loadBatchFile filepath =
(
	try
	(
		local extension = cryTools.cryAnim.UI.batchProcess._f.getExtension filepath
		
		case extension of
		(
				--// if a bip file is in queue
			"bip":
			(
					--// if there is no biped character in the scene
				if cryTools.cryAnim._f.getBaseBip() == undefined then
				(
					return "Error: No Biped in Scene     " --false
				)
					--// wait until the biped file is opened
				while ((tempBool = (cryTools.cryAnim.UI.main._f.getUI "Load / Save / Export" "Load Biped File").pressed())[1] != true) do
					escapeEnable = true
				
					--// if an error occured while loading the file
				if tempBool[2] == false then
					return "Error loading file     "
			)
			
				--// if a fbx file is in queue
			"fbx":
			(	
					--// load the NullScene which has nothing in it, just to reset the scene without reset dialog
				--loadMaxFile (cryTools.BUIldPathFull + "Tools\\maxscript\\ref\\NullSceneMax8.max")
				resetMaxFile #noPrompt
				
					--// imports the fbx file in queue								
				if (importFile filepath #noPrompt) != true then
					return "Error loading file     "
				
					--// creates a dummy used for "Object Export" in CryEngine2 exporter
				local tempDummy = dummy name:"tempDummy"


				
					--// tries to get the animation range, if it can't, the animation range is in most cases 0,0 which 3dmax can't handle
				try
				(
						--// sets animation range by the key times of the pelvis
					animationRange = interval $'Bip01 Pelvis'.pos.controller.keys[1].time $'Bip01 Pelvis'.pos.controller.keys[$'Bip01 Pelvis'.pos.controller.keys.count].time
				) catch( animationRange = interval 0 1 )
				
			)
			
				--// if a max file is in queue
			"max":
			(
				try
					loadMaxFile filepath useFileUnits:true quiet:true
				catch
					return "Error loading file     "
			)
			
			
			"scene": return ""
			
				--// if nothing useful is in queue
			default:
			(
				return "File extension not supported"
			)
		)
		
		return true
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess._f.loadBatchFile" )
)
loadBatchFile = undefined
logOutput "> Created cryTools.cryAnim.UI.batchProcess._f.loadBatchFile function"















--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.batchProcess._f.processFiles
----------------------------------------------------------------------------------------------------------------------------------
--			creates folder, runs pre-export script and/or exports the file, generates log file
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.batchProcess._f.processFiles = function processFiles op =
(
	try
	(
			--// rollout to show the progress of the check/export
		rollout exportProgress "Animation Export Progress" width:525 height:32
		(				
			label ProgressInfo "Processing Animation :" pos:[10,4] width:150 height:15
				--// animation name
			label CurrentAnimation "" pos:[160,4] width:370 height:15
				--// queue of animations
			label CurrentCount "0 / 0" pos:[470,4] width:80 height:15
				--// progress bar for all animations
			progressBar MainProgress "" pos:[10,23] width:505 height:7 color:green
		)
		cryTools.cryAnim.UI.batchProcess.progressDialog = exportProgress
		exportProgress = undefined
		
			--// creates the progress dialog				
		createdialog cryTools.cryAnim.UI.batchProcess.progressDialog
		
			--// generates log file
		local filepath = cryTools.cryAnim.UI.batchProcess._f.generateOutput #generate ""
		
			
			--// maximum files used
		maxFiles = 0
		
		for i = 1 to cryTools.cryAnim.UI.batchProcess._v.exportFiles.count do
			if (cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].extension != "folder") and (cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].used == true) then
				maxFiles += 1
		
			--// counter for files only, not folders
		counter = 0
		
		with undo off
		(
		
				--// goes through the files/folder list
			for i = 1 to cryTools.cryAnim.UI.batchProcess._v.exportFiles.count do
			(
				tempStatus = ""
				cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].process = ""
				ready2export = true
					--// if entry is visible and not a folder
				if (cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].used == true) and (cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].extension != "folder") then
				(
						--// increment the counter
					counter += 1
						--// allow loop cancelation
					escapeEnable = true
						--// save variables
					tempCheckBeforeExport = cryTools.checkbeforeexport
					tempSuppressWarnings = cryTools.suppresswarnings
					tempInterval = animationRange
										
						--// set file to be the next in the loading and exporting queue
					cryTools.cryAnim.UI.batchProcess._v.fileQue = cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].path
					cryTools.cryAnim.UI.batchProcess._v.selectedFile = i
					
						--// changes the progress text to the new file name
					cryTools.cryAnim.UI.batchProcess.progressDialog.CurrentAnimation.text = cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].filename
						--// and updates the counter text
					cryTools.cryAnim.UI.batchProcess.progressDialog.CurrentCount.text = ((counter as String) + " / " + (maxFiles as String) as String)
					
					if cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].extension != "scene" then
					(
						ready2Export = cryTools.cryAnim.UI.batchProcess._f.loadBatchFile cryTools.cryAnim.UI.batchProcess._v.fileQue
						
						if ready2Export != true and ready2Export != false then
						(
							tempStatus = ready2Export
							ready2Export = false
						)
					)
					
					
					case cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].extension of
					(
						"fbx":
						(
								--// deactivate checks, which show error messages (are not needed here)
							cryTools.checkbeforeexport = false
							cryTools.suppresswarnings = true
							
								--// opens the CryEngine2 Exporter
							UtilityPanel.OpenUtility CryEngine2_Exporter
								--// sets the "Object Export" list with the dummy
							csexport.set_node_list #(tempDummy)
							try csexport.set_bone_list #($Bip01) catch()
						)
					)
					
					
						--// converts the file path to caf path
					tempCafPath = (cryTools.cryAnim.UI.main._f.checkExport #ProductionToGame cryTools.cryAnim.UI.batchProcess._v.fileQue)

						--// use sub folder structure for export
					if cryTools.cryAnim.UI.batchProcess.dialog.rollouts[3].chkKeepSubFolders.checked == true then
					(
							--// seperate sub folders from the source directory
						openFilter = filterString cryTools.cryAnim.UI.batchProcess._v.sourcePath "\\"
							--// seperate sub folders from the file directory
						fileFilter = filterString cryTools.cryAnim.UI.batchProcess._v.fileQue "\\"

							--// new export path is set from the batch export dialog
						newCafPath = cryTools.cryAnim.UI.batchProcess._v.exportPath
						
							--// if there is a difference in the folder structure = sub folder of source folder
						if (fileFilter.count - (openFilter.count - 1)) > 0 then
						(
								--// goes through the sub folders
							for i = (openFilter.count + 1) to fileFilter.count do
									--// adds sub folders to the export path
								newCafPath += fileFilter[i] + (if i != fileFilter.count then "\\" else "")
							
								--// adds .caf extension to the path
							newCafPath = (substring newCafPath 1 (newCafPath.count - 4)) + ".caf"
						)
							--// if there is no difference
						else							
							newCafPath = cryTools.cryAnim.UI.batchProcess._v.exportPath + (cryTools.cryAnim.UI.main._f.checkExport #getCaf (cryTools.cryAnim.base.perforce cryTools.cryAnim.UI.batchProcess._v.fileQue #getFilename))
						
							--// set new export path
						cryTools.cryAnim.UI.main.loadSave._v.cafSavePath = newCafPath
					)
						--// if no sub folders should be used
					else
						cryTools.cryAnim.UI.main.loadSave._v.cafSavePath = cryTools.cryAnim.UI.batchProcess._v.exportPath + (cryTools.cryAnim.UI.main._f.checkExport #getCaf (cryTools.cryAnim.base.perforce cryTools.cryAnim.UI.batchProcess._v.fileQue #getFilename))
					

						--// if there is no export path folder
					if (getDirectories (cryTools.cryAnim.base.perforce cryTools.cryAnim.UI.main.loadSave._v.cafSavePath #getDirectory)).count == 0 then
							--// create one
						cryTools.cryAnim.UI.main._f.checkExport #createFolder cryTools.cryAnim.UI.main.loadSave._v.cafSavePath


						--// if bone detection is activated
					if cryTools.cryAnim.UI.batchProcess.dialog.rollouts[3].chkDetect.checked == true then
					(
							--// get detected bones list
						local tempBoneArray = cryTools.cryAnim.UI.batchProcess._f.getBoneDetection cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].filename
							--// if the list is not empty and if there are bones defined
						if (tempBoneArray.count > 0) and (tempBoneArray[1] != false) then
						(
								--// seperate bones names
							local tempBoneFilter = filterString cryTools.cryAnim.UI.batchProcess._v.boneList[tempBoneArray[1].index].bones ";"
									--// add the bones to the used bones list
								(cryTools.cryAnim.UI.main._f.getUI "Load / Save / Export" "lbExport").items = tempBoneFilter
						)
							--// if no bone is detected
						else
								--// use Bip01 as default
							(cryTools.cryAnim.UI.main._f.getUI "Load / Save / Export" "lbExport").items = #("Bip01")
					)

																					
						--// if there is no locator
					if $Locator_Locomotion == undefined then
					(
							--// if the pelvis and Bip01 position are the same, no locator is in the scene
						try
						(
							if $'Bip01 Pelvis'.transform.pos == $Bip01.transform.pos then
									--// set export message
								tempStatus += "Error: No Locator found    "
						)
						catch()
					)
						--// if a locator is in the scene
					else
					(
							--// if the locator has no keys (not animated and in default position from biped)
						if $Locator_Locomotion.controller.keys.count == 0 then
								--// set export message
							tempStatus += "Error: No Locator Keys found    "
					)

					
						--// if a pre-export script is defined
					cryTools.cryAnim.UI.batchProcess._f.executeCustomScripts "1" cryTools.cryAnim.UI.batchProcess.dialog.rollouts[2].ddOp1
					cryTools.cryAnim.UI.batchProcess._f.executeCustomScripts "2" cryTools.cryAnim.UI.batchProcess.dialog.rollouts[2].ddOp2
					cryTools.cryAnim.UI.batchProcess._f.executeCustomScripts "3" cryTools.cryAnim.UI.batchProcess.dialog.rollouts[2].ddOp3
					cryTools.cryAnim.UI.batchProcess._f.executeCustomScripts "4" cryTools.cryAnim.UI.batchProcess.dialog.rollouts[2].ddOp4
					
					tempStatus += cryTools.cryAnim.UI.batchProcess._v.customScriptStatus
					
					cryTools.cryAnim.UI.batchProcess._v.customScriptStatus = ""
					
						--// if no error occured
					if tempStatus == "" then
							--// set export message
						tempStatus = "Done"
				
						--// if bone detection is enabled
					if cryTools.cryAnim.UI.batchProcess.dialog.rollouts[3].chkDetect.checked == true then
							--// reset export bone list
						(cryTools.cryAnim.UI.main._f.getUI "Load / Save / Export" "lbExport").items = #("Bip01")
					
						--// restore export checks
					cryTools.checkbeforeexport = tempCheckBeforeExport
					cryTools.suppresswarnings = tempSuppressWarnings
					
						--// update progress bar
					cryTools.cryAnim.UI.batchProcess.progressDialog.MainProgress.value = (100 * i) / maxFiles
						
						--// add file to the log
					cryTools.cryAnim.UI.batchProcess._f.generateOutput (cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].filename + " | " + cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].details + " | " + tempStatus) filepath
						
						--// update export status of the file
					cryTools.cryAnim.UI.batchProcess._v.exportFiles[i].process = tempStatus
				)
			)
		)
		
			--// update the whole dialog with export status						
		--cryTools.cryAnim.UI.batchProcess._f.updateDialog updateFromExport:true
		cryTools.cryAnim.UI.batchProcess._f.refreshFiles()
			--// reset file queue
		cryTools.cryAnim.UI.batchProcess._v.fileQue = undefined
			--// reset source path
		cryTools.cryAnim.UI.main.loadSave._v.bipOpenPath = cryTools.cryAnim.UI.main._v.bipWorkingDir + "*.bip"
			--// kill progress dialog
		destroyDialog cryTools.cryAnim.UI.batchProcess.progressDialog
		
		return true
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.batchProcess._f.processFiles" )
)
processFiles = undefined
logOutput "> Created cryTools.cryAnim.UI.batchProcess._f.processFiles function"























--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.models._f.selectItem
----------------------------------------------------------------------------------------------------------------------------------
--			selects or returns the used item in the filename
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.models._f.selectItem = function selectItem itemSelect op =
(
	try
	(
		case op of
		(
			#set:
			(
				if cryTools.cryAnim.UI.main.models._v.itemList.count > 0 then
				(
					local itemArray = cryTools.cryAnim.UI.main.models._v.itemList
					
					try
					(
						local tempNodeForDelete = ($_IC_* as array)
						for i = 1 to tempNodeForDelete.count do
							delete tempNodeForDelete[i]
					)catch()
					
					if itemArray[itemSelect].name != "" then
					(
						if $Bip01 != undefined then
						(
							local itemUsed = #()
							local itemFound = #()
							
							for i = 1 to itemArray.count do
							(
								if i == itemSelect then
								(
									for d = 1 to itemArray[i].model.count do
									(
										local tempNode = getNodeByName itemArray[i].model[d]
										
										if tempNode == undefined then
										(
											mergeMAXfile (cryTools.BuildPathFull + "tools\\maxscript\\ref\\weapon_positions.max") #(itemArray[itemSelect].model[d]) #alwaysReparent #skipDups #useMergedMtlDups 
											try
											(
												local tempNode = getNodebyName itemArray[itemSelect].model
												
												append itemUsed tempNode
												append itemFound d
											)catch()
										)
										else
										(
											if (findItem itemFound d) == 0 then
											(
												append itemUsed tempNode
												append itemFound d
											)
											for f = 1 to itemArray[i].model.count do
											(
												if f != d and (findItem itemFound f) == 0 then
												(
													if itemArray[i].model[d] == itemArray[i].model[f] then
													(
														local tempCopy = (cryTools.cryAnim._f.createSnapshot object:(getNodeByName itemArray[i].model[f]) )[1]
														tempCopy.name = "_IC_" + tempCopy.name
														tempCopy.isFrozen = (getNodeByName itemArray[i].model[f]).isFrozen
														
														append itemUsed tempCopy
														append itemFound f
													)	
												)
											)
										)
									)
								)
								else
								(
									for d = 1 to itemArray[i].model.count do
									(
										local tempNode = getNodeByName itemArray[i].model[d]
										
										if tempNode != undefined then
										(
											if tempNode.isHidden == false then
												tempNode.isHidden = true
										)
									)
								)	
							)
							
							for i = 1 to itemUsed.count do
							(
								if itemUsed[i].isHidden == true then
									itemUsed[i].isHidden = false
							)
							
							
							
							
							for i = 1 to itemArray[itemSelect].model.count do
							(
								try
								(
									if (local nodeRef = getNodeByName itemArray[itemSelect].reference[i]) != undefined then
									(
										in coordsys nodeRef
										(
											if (local nodeParent = getNodeByName itemArray[itemSelect].parent[i]) != undefined then
											(
												try
												(
													itemUsed[i].transform = nodeParent.transform
													itemUsed[i].parent = nodeParent
												)catch()
												
												with animate off
												(
													nodeParent.rotation = itemArray[itemSelect].rotation[i]
													nodeParent.position = itemArray[itemSelect].position[i]
												)
											)
										)
									)
								)catch()
							)
							
							
							(cryTools.cryAnim.UI.main._f.getUI "Models" "ddItemSelect").selection = itemSelect
							return true
						)
					)
				)
				return false
			)
			
			#getIndex:
			(
				if cryTools.cryAnim.UI.main.models._v.itemList.count > 0 then
				(
					for i = 1 to cryTools.cryAnim.UI.main.models._v.itemList.count do
					(
						local tempModels = cryTools.cryAnim.UI.main.models._v.itemList[i].model
						local found = false
						for d = 1 to tempModels.count do
						(
							local tempNode = getNodeByName tempModels[d]
							if tempNode != undefined then
							(
								if tempNode.isHidden == false then
									found = true
								else
									found = false
							)
						)
						if found == true then
								return i
					)
					return 1
				)
				else
					return 0
			)
			
			#getList:
			(
				local itemArray = cryTools.cryAnim.base.iniFile #get #items
				
				try
				(
						--// gets ini info
					if itemArray != undefined then
					(					
						tempListArray = #()
						tempListArray2 = #()
						
							--// gets the array of properties in the structure of itemArray
						local subItemArray = getPropNames itemArray[1]
							
						
						
						for i = 1 to itemArray.count do
						(
							if itemArray[i].name != "" then
							(
								itemArray[i].model = filterString itemArray[i].model ";"
								itemArray[i].reference = filterString itemArray[i].reference ";"
								itemArray[i].parent = filterString itemArray[i].parent ";"
								itemArray[i].position = filterString itemArray[i].position ";"
								itemArray[i].rotation = filterString itemArray[i].rotation ";"
								
								for f = 1 to itemArray[i].model.count do
								(
									itemArray[i].position[f] = execute (itemArray[i].position[f])
									itemArray[i].rotation[f] = execute (itemArray[i].rotation[f])
								)
								
							)
						)
						
						return itemArray
					)
				)
				catch
				(
					itemArray = #()
					return itemArray
				)
			)
		)
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.models._f.selectItem" )
)
selectItem = undefined
logOutput "> Created cryTools.cryAnim.UI.main.models._f.selectItem function"











--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.models._f.selectByNameFilterNode
----------------------------------------------------------------------------------------------------------------------------------
--			checks, if the node is not an biped object
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.models._f.selectByNameFilterNode = function selectByNameFilterNode obj =
(
	try
	(
		if obj.classID[1] != 37157 then
			return true
		else
			return false
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.models._f.selectByNameFilterNode" )
)
selectByNameFilterNode = undefined
logOutput "> Created cryTools.cryAnim.UI.main.models._f.selectByNameFilterNode function"









--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.models._f.selectByNameFilterParent
----------------------------------------------------------------------------------------------------------------------------------
--			checks, if the node is not tempObj
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.models._f.selectByNameFilterParent = function selectByNameFilterParent objParent =
(
	try
	(
		if objParent.name != tempObj.name then
			return true
		else
			return false
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.models._f.selectByNameFilterParent" )
)
selectByNameFilterParent = undefined
logOutput "> Created cryTools.cryAnim.UI.main.models._f.selectByNameFilterParent function"









--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.models._f.setVisible
----------------------------------------------------------------------------------------------------------------------------------
--			returns true, if the selected index should be visible
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.models._f.setVisible = function setVisible index =
(
	try
	(
		tempArray = cryTools.cryAnim.base.iniFile #get #models
		if (tempArray != "") and (tempArray != undefined) then
		(
			if tempArray[index] != undefined then
				return true
			else
				return false
		)
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.models._f.setVisible" )
)
setVisible = undefined
logOutput "> Created cryTools.cryAnim.UI.main.models._f.setVisible function"











--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.models._f.getEntries
----------------------------------------------------------------------------------------------------------------------------------
--			returns an array of model entries (filter function)
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.models._f.getEntries = function getEntries index entriesArray =
(
	try
	(
		if entriesArray[index] != undefined then
			return entriesArray[index].name
		else
			return ""
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.models._f.getEntries" )
)
getEntries = undefined
logOutput "> Created cryTools.cryAnim.UI.main.models._f.getEntries function"










--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.models._f.loadModel
----------------------------------------------------------------------------------------------------------------------------------
--			loads the model specified in "entry"
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.models._f.loadModel = function loadModel entry =
(
	try
	(
		local tempVar = loadMaxFile entry.path
		if tempVar != false then
		(
			try
			(
				$'Bip01 R ForeTwist'.parent = $'Bip01 R Forearm'
				$'Bip01 L ForeTwist'.parent = $'Bip01 L Forearm'
				print "TwistBones reparented..."
			)
			catch
			(
				print "No Biped Loaded..."
			)
			
			select $Bip01
			setCommandPanelTaskMode #motion
			
			return true
		)
		else
		(
			messageBox ("File: " + entry.path + "    not found") title:"Error Loading Model"
			return false
		)
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.models._f.loadModel" )
)
loadModel = undefined
logOutput "> Created cryTools.cryAnim.UI.main.models._f.loadModel function"












--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.models._f.updateExtent
----------------------------------------------------------------------------------------------------------------------------------
--			updates the extent of the list view in editItemList
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.models._f.updateExtent = function updateExtent =
(
	try
	(
		local newExtent = #(0,0,0,0,0,0,0,0)
		local columnExtent = #(0,0,0,0,0,0,0,0)
		
		local lbItems = cryTools.cryAnim.UI.main.models.editItemList.lbItems
		
		
		for d = 0 to (lbItems.Columns.count - 1) do
		(
			columnExtent[d+1] = (getTextExtent (lbItems.Columns.item d).text)[1]
			
			if columnExtent[d+1] > newExtent[d+1] then
					newExtent[d+1] = columnExtent[d+1] + 12
		)
		
		
		for i = 0 to (lbItems.Items.count - 1) do
		(
			local tempItem = lbItems.Items.item i
			local textExtent = #(0,0,0,0,0,0,0,0)
			
			--// extent of the current filename
			for d = 0 to (lbItems.Columns.count - 1) do
				try ( textExtent[d+1] = (getTextExtent (tempItem.SubItems.item d).text)[1] + 12) catch()
				
			
			
			for f = 1 to textExtent.count do
			(
					--// if the current extent is higher than the result extent
				if textExtent[f] > newExtent[f] then
							--// result extent is current extent
					newExtent[f] = textExtent[f] --+ 12 --// additional number so entries are not cut
			)
				
		)
		
		for f = 1 to newExtent.count do
				--// applys the weidth of the column with the text extent
			(lbItems.Columns.item (f - 1)).width = newExtent[f]
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.models._f.updateExtent" )
)
updateExtent = undefined
logOutput "> Created cryTools.cryAnim.UI.main.models._f.updateExtent function"










--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.models._f.sortList
----------------------------------------------------------------------------------------------------------------------------------
--			sorts the list view in editItemList
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.models._f.sortList = function sortList =
(
	try
	(
		local lbItems = cryTools.cryAnim.UI.main.models.editItemList.lbItems
		local newArray = #()
		local oldArray = #()
		local sortedArray = #()
		if classOf lbItems == ActiveXControl then
		(
			local maxCount = lbItems.ListItems.count
			local forStart = 1
		)	
		else
		(
			local maxCount = lbItems.Items.count - 1
			local forStart = 0
		)
		
		for i = forStart to maxCount do
		(
			if classOf lbItems == ActiveXControl then
				append oldArray (#(lbItems.ListItems[i].text, lbItems.ListItems[i].ListSubItems[1].text, lbItems.ListItems[i].ListSubItems[2].text, lbItems.ListItems[i].ListSubItems[3].text, lbItems.ListItems[i].ListSubItems[4].text, lbItems.ListItems[i].ListSubItems[5].text, lbItems.ListItems[i].ListSubItems[6].text, lbItems.ListItems[i].ListSubItems[7].text))
			else
				append oldArray (#((lbItems.Items.item i).text, ((lbItems.Items.item i).SubItems.item 1).text, ((lbItems.Items.item i).SubItems.item 2).text, ((lbItems.Items.item i).SubItems.item 3).text, ((lbItems.Items.item i).SubItems.item 4).text, ((lbItems.Items.item i).SubItems.item 5).text, ((lbItems.Items.item i).SubItems.item 6).text, ((lbItems.Items.item i).SubItems.item 7).text))
		)
		
		for i = 1 to oldArray.count do
			append newArray oldArray[i][1]
		
		sort newArray
		
		for i = 1 to newArray.count do
		(
			findEntry = newArray[i]
			entryFound = 0
			for f = 1 to oldArray.count do
			(
				if oldArray[f][1] == newArray[i] then
				(
					append sortedArray oldArray[f]
					sortedArray[sortedArray.count][1] = newArray[i]
				)
			)
		)
		
		if classOf lbItems != ActiveXControl then
		(
			lbItems.BeginUpdate()
			lbItems.Items.clear()
		)
		else
			lbItems.ListItems.clear()
		
		for i = 1 to newArray.count do
		(
			if classOf lbItems == ActiveXControl then
			(
				local tempItem = lbItems.ListItems.Add text:sortedArray[i][1]
				tempItem.listSubItems.Add text:sortedArray[i][2]
				tempItem.listSubItems.Add text:sortedArray[i][3]
				tempItem.listSubItems.Add text:sortedArray[i][4]
				tempItem.listSubItems.Add text:sortedArray[i][5]
				tempItem.listSubItems.Add text:sortedArray[i][6]
				tempItem.listSubItems.Add text:sortedArray[i][7]
				tempItem.listSubItems.Add text:sortedArray[i][8]
			)
			else
			(	
				local tempItem = lbItems.Items.Add sortedArray[i][1]
				tempItem.SubItems.Add sortedArray[i][2]
				tempItem.SubItems.Add sortedArray[i][3]
				tempItem.SubItems.Add sortedArray[i][4]
				tempItem.SubItems.Add sortedArray[i][5]
				tempItem.SubItems.Add sortedArray[i][6]
				tempItem.SubItems.Add sortedArray[i][7]
				tempItem.SubItems.Add sortedArray[i][8]
			)
		)
		
		if classOf lbItems != ActiveXControl then
			lbItems.EndUpdate()
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.models._f.sortList" )
)
sortList = undefined
logOutput "> Created cryTools.cryAnim.UI.main.models._f.sortList function"










--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.models._f.updateExtentModel
----------------------------------------------------------------------------------------------------------------------------------
--			updates the extent of the list view in editModelList
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.models._f.updateExtentModel = function updateExtentModel =
(
	try
	(
		local newExtent = #(0,0)
		local lbModels = cryTools.cryAnim.UI.main.models.editModelList.lbModels
		
		for i = 0 to (lbModels.Items.count - 1) do
		(
			local tempItem = lbModels.Items.item i
			local textExtent = #(0,0)
			--// extent of the current filename
			for d = 1 to lbModels.Columns.count do						
				textExtent[d] = (getTextExtent (tempItem.SubItems.item (d - 1)).text)[1]-- + (if d == 1 then 12 else 0)
			
			
			for f = 1 to textExtent.count do
					--// if the current extent is higher than the result extent
				if textExtent[f] > newExtent[f] then
							--// result extent is current extent
					newExtent[f] = textExtent[f] + 12 --// additional number so entries are not cut
		)
		
		for f = 1 to newExtent.count do
			if (lbModels.Columns.item (f - 1)).width < newExtent[f] then
				--// applys the weidth of the column with the text extent
				(lbModels.Columns.item (f - 1)).width = newExtent[f]
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.models._f.updateExtentModel" )
)
updateExtentModel = undefined
logOutput "> Created cryTools.cryAnim.UI.main.models._f.updateExtentModel function"
























--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.operation._f.updateDialog
----------------------------------------------------------------------------------------------------------------------------------
--			updates the dialog of operationRO
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.operation._f.updateDialog = function updateDialog output:unsupplied =
(
	try
	(
		local ddOperationItems = #( "Planted Key", "Sliding Key", "Free Key", "--------------------------------------------", "Move","Move/Rotate","--------------------------------------------", "Move to Floor","Reset Rotation","Rotate Anim","--------------------------------------------", "Copy" , "Paste", "--------------------------------------------","Bip to [0,0]","Bip to [0,0,0]" )
		local ddOperationToItems = #( "Snapshot", "Pivot Point" )
		
		case output of
		(
			#ddOp:			
				return ddOperationItems
			#ddOpTo:
				return ddOperationToItems
		)
		
		
		local ddSel = cryTools.cryAnim.UI.main._f.getUI "Target" "ddSelection"
		local ddOp = cryTools.cryAnim.UI.main._f.getUI "Operation" "ddOperation"
		local ddOpTo = cryTools.cryAnim.UI.main._f.getUI "Operation" "ddOperationTo"
		local spnDistance = cryTools.cryAnim.UI.main._f.getUI "Operation" "spnDistance"
		local spnDirection = cryTools.cryAnim.UI.main._f.getUI "Operation" "spnDirection"
		local spnSteps = cryTools.cryAnim.UI.main._f.getUI "Operation" "spnSteps"
		local labOp = cryTools.cryAnim.UI.main._f.getUI "Operation" "labOperation"
		local labDistanceDirection = cryTools.cryAnim.UI.main._f.getUI "Operation" "labDistanceDirection"
		local gbOp = cryTools.cryAnim.UI.main._f.getUI "Operation" "gbOperation"
		local btnApply = cryTools.cryAnim.UI.main._f.getUI "Operation" "btnApply"
		local btnApplyBeginEnd = cryTools.cryAnim.UI.main._f.getUI "Operation" "btnApplyBeginEnd"
		local btnApplyRange = cryTools.cryAnim.UI.main._f.getUI "Operation" "btnApplyRange"
		local operationRO = cryTools.cryAnim.UI.main._f.getUI "Operation" ""
					
					
		local ddOperationHidePos = 129
		local ddOperationShowPos = 170
		local ddSelectionChange = (cryTools.cryAnim.UI.main._f.getUI "Target" "labChange").text
		local ddSelIndex = #(5,6,8,10,13)
		
		if ddSel.selection < 2 then
		(
			ddOp.items = ddOperationItems
			
			if (findItem ddSelIndex ddOp.selection) != 0 then
			(
				if ddOp.selection == 5 or ddOp.selection == 6 then
						ddOpTo.items = #( "Snapshot", "Pivot Point" )
				else
					ddOpTo.items = #( "Transform", "Position", "Rotation" )
				
				if ddOp.selection == 8 or ddOp.selection == 10 then
				(
					
					labDistanceDirection.visible = true
					ddOpTo.visible = false
					labOp.visible = false
				)
				else
				(
					ddOpTo.visible = true
					labOp.visible = true
					labDistanceDirection.visible = false
				)
				
				if ddOp.selection == 8 then
					spnDistance.visible = true
				else
					spnDistance.visible = false
				
				if ddOp.selection == 10 then
					spnDirection.visible = true
				else
					spnDirection.visible = false
				
				
				btnApply.enabled = true
				
				btnApply.pos.y = ddOperationShowPos
				btnApplyBeginEnd.pos.y = ddOperationShowPos
				btnApplyRange.pos.y = ddOperationShowPos
				gbOp.height = ( ddOperationShowPos - gbOp.pos.y ) + 28
				operationRO.height = ddOperationShowPos + 32
				
				spnSteps.enabled = true
				btnApplyBeginEnd.enabled = true
				
				
				if ddOp.selection == 8 then
				(
					labDistanceDirection.text = "Floor Distance :"
					local tempVar = cryTools.cryAnim._f.moveToFloor getHeight:true
					if tempVar != undefined then
						if (queryBox "Get Height from Selection?" title:"Move to Floor") == true then
							spnDistance.value = tempVar
				)
				else
				(
					labDistanceDirection.text = "Direction :"
					spnSteps.enabled = false
					btnApplyBeginEnd.enabled = false
				)
				
				
			)
			else
			(
				ddOpTo.visible = false
				labOp.visible = false
				spnDistance.visible = false
				spnDirection.visible = false
				labDistanceDirection.visible = false
				
				if ddOp.selection == 10 then
				(
					spnSteps.enabled = false
					btnApplyBeginEnd.enabled = false
				)
				else
				(
					spnSteps.enabled = true
					btnApplyBeginEnd.enabled = true
				)
				
				btnApply.pos.y = ddOperationHidePos
				btnApplyBeginEnd.pos.y = ddOperationHidePos
				btnApplyRange.pos.y = ddOperationHidePos
				gbOp.height = ( ddOperationHidePos - gbOp.pos.y ) + 28
				operationRO.height = ddOperationHidePos + 32							
			)
			
			
		)
		else
		(
			ddOp.items = #( "Move", "Line" )
			if ddOp.selection == 0 then ddOp.selection = 1
														
			if ddSelectionChange == "2:Changed" then
			(
				ddOp.selection = 1
				(cryTools.cryAnim.UI.main._f.getUI "Target" "labChange").text = "2:Unchanged"
			)
			
			
			spnSteps.enabled = true
			btnApplyBeginEnd.enabled = true
			
			ddOpTo.visible = false
			labOp.visible = false
			
			spnDistance.visible = false
			spnDirection.visible = false
			labDistanceDirection.visible = false
			
			
			btnApply.pos.y = ddOperationHidePos
			btnApplyBeginEnd.pos.y = ddOperationHidePos
			btnApplyRange.pos.y = ddOperationHidePos
			gbOp.height = ( ddOperationHidePos - gbOp.pos.y ) + 28
			operationRO.height = ddOperationHidePos + 32
			
			
			if ddOp.selection == 2 then
			(
				btnApply.enabled = false
				cryTools.cryAnim.UI.main.target._f.updateDDSelection op:false
				
			)
			else
			(
				btnApply.enabled = true
				cryTools.cryAnim.UI.main.target._f.updateDDSelection op:true
				
			)
			
		)
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.operation._f.updateDialog" )
)
updateDialog = undefined
logOutput "> Created cryTools.cryAnim.UI.main.operation._f.updateDialog function"









--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.operation._f.getSelectedAxis
----------------------------------------------------------------------------------------------------------------------------------
--			returns an array of axis which are checked in targetRO
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.operation._f.getSelectedAxis = function getSelectedAxis op =
(
	try
	(
		local posAxis = #(false, false, false, false)
		local rotAxis = #(false, false, false, false)
		
		if (cryTools.cryAnim.UI.main._f.getUI "Target" "").chkX.checked == true then	posAxis[1] = true
		if (cryTools.cryAnim.UI.main._f.getUI "Target" "").chkY.checked == true then	posAxis[2] = true
		if (cryTools.cryAnim.UI.main._f.getUI "Target" "").chkZ.checked == true then	posAxis[3] = true
		if (cryTools.cryAnim.UI.main._f.getUI "Target" "").chkPosOffset.checked == true then	posAxis[4] = true
		
		if (cryTools.cryAnim.UI.main._f.getUI "Target" "").chkXRot.checked == true then	rotAxis[1] = true
		if (cryTools.cryAnim.UI.main._f.getUI "Target" "").chkYRot.checked == true then	rotAxis[2] = true
		if (cryTools.cryAnim.UI.main._f.getUI "Target" "").chkZRot.checked == true then	rotAxis[3] = true
		if (cryTools.cryAnim.UI.main._f.getUI "Target" "").chkRotOffset.checked == true then	rotAxis[4] = true

		case op of
		(
			#pos: return posAxis
			#rot: return rotAxis
		)
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.operation._f.getSelectedAxis" )
)
getSelectedAxis = undefined
logOutput "> Created cryTools.cryAnim.UI.main.operation._f.updateDialog function"












--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.operation._f.align
----------------------------------------------------------------------------------------------------------------------------------
--			align the selected node to the list node
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.operation._f.align = function align tempSel tempNode snapshots:undefined =
(
	try
	(
		local posAxis = cryTools.cryAnim.UI.main.operation._f.getSelectedAxis #pos
		local rotAxis = cryTools.cryAnim.UI.main.operation._f.getSelectedAxis #rot

		for i = 1 to tempSel.count do
		(
			local tempPos = tempSel[i].transform.pos
			local tempRot = tempSel[i].transform.rotation as eulerangles
			
			if cryTools.cryAnim._v.various[35] != undefined then						
			(
				with undo off
				(
					in coordsys tempNode
					(
						snapshots[i].rotation = cryTools.cryAnim._v.various[35][i].rotation
						snapshots[i].pos = cryTools.cryAnim._v.various[35][i].position
					)
				)
			)
			
			
			
			if rotAxis[4] == false then
			(
				if rotAxis[1] == true then tempRot.x = (tempNode.transform.rotation as eulerangles).x
				if rotAxis[2] == true then tempRot.y = (tempNode.transform.rotation as eulerangles).y
				if rotAxis[3] == true then tempRot.z = (tempNode.transform.rotation as eulerangles).z
			)
			else
			(
				if rotAxis[1] == true then tempRot.x = (snapshots[i].transform.rotation as eulerangles).x
				if rotAxis[2] == true then tempRot.y = (snapshots[i].transform.rotation as eulerangles).y
				if rotAxis[3] == true then tempRot.z = (snapshots[i].transform.rotation as eulerangles).z
			)
			
			
			
			if posAxis[4] == false then
			(
				if posAxis[1] == true then tempPos[1] = tempNode.transform.pos[1]
				if posAxis[2] == true then tempPos[2] = tempNode.transform.pos[2]
				if posAxis[3] == true then tempPos[3] = tempNode.transform.pos[3]							
			)						
			else
			(
				if posAxis[1] == true then tempPos[1] = snapshots[i].transform.pos[1]
				if posAxis[2] == true then tempPos[2] = snapshots[i].transform.pos[2]
				if posAxis[3] == true then tempPos[3] = snapshots[i].transform.pos[3]
			)
			
			local tempMatrix = snapshots[i].transform
			tempMatrix.rotation = tempRot
			tempMatrix.pos = tempPos
				
			
			try tempSel[i].rotation = tempMatrix.rotation
			catch biped.setTransform tempSel[i] #rotation tempMatrix.rotation true
				
			try tempSel[i].pos = tempMatrix.pos
			catch biped.setTransform tempSel[i] #pos tempMatrix.pos true

			
		)
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.operation._f.align" )
)
align = undefined
logOutput "> Created cryTools.cryAnim.UI.main.operation._f.align function"










--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.operation._f.alignLine
----------------------------------------------------------------------------------------------------------------------------------
--			align the selected node to the list node in a line
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.operation._f.alignLine = function alignLine startPos endPos axis tempSel tempNode =
(
	try
	(
		local diffAll = endPos[axis] - startPos[axis]
		local diffSel = tempNode.transform.pos[axis] - startPos[axis]
		
		local tempPos = 0
		try ( tempPos = biped.getTransform tempNode #pos )
		catch ( tempPos = tempNode.pos )
			
			
			
		local time = diffSel / diffAll
			
		local newPos = (endPos * time) + (startPos * (1 - time))
			
		try
			biped.setTransform tempSel #pos newPos true
		catch
			tempSel.pos = newPos
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.operation._f.alignLine" )
)
alignLine = undefined
logOutput "> Created cryTools.cryAnim.UI.main.operation._f.alignLine function"












--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.operation._f.applyOperation
----------------------------------------------------------------------------------------------------------------------------------
--			applies the in operationRO selected operation to the selected node
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.operation._f.applyOperation = function applyOperation operation opTo start end steps =
(
	try
	(
		local ddSelection = cryTools.cryAnim.UI.main._f.getUI "Target" "ddSelection"
		local ddOp = cryTools.cryAnim.UI.main._f.getUI "Operation" "ddOperation"
		local ddOpTo = cryTools.cryAnim.UI.main._f.getUI "Operation" "ddOperationTo"
		
		
		if ddSelection.selection != 1 then
			snapshots = cryTools.cryAnim._f.createSnapshot()

		
		axis = 0
		local startPos = [0,0,0]
		local endPos = [0,0,0]
		
		
		if (ddSelection.selection > 2)	and (ddOp.selection == 2) then
		(
			tempNode = $
			
			sliderTime = animationRange.start
			startPos = tempNode.transform.pos
			sliderTime = animationRange.end
			endPos = tempNode.transform.pos
			tempStart = 0
			
			if startPos == endPos then
			(
				messageBox ("Start point is equal to End Point") title:"Line Operation"
				return false
			)
				
				
			
			
			
			for i = 1 to 3 do
			(
				local tempStartPos = startPos[i]
				local tempEndPos = endPos[i]
				
				if tempStartPos < 0 then tempStartPos -= tempStartPos * 2
				if tempEndPos < 0 then tempEndPos -= tempEndPos * 2
				
				
				
				local tempPos =  tempEndPos - tempStartPos
				if tempPos > tempStart then
				(
					axis = i
					tempStart = tempPos
				)
			)
		)
		
		
		
		
		
		if operation == 10 then
		(
			local animDir = (cryTools.cryAnim.UI.main._f.getUI "Operation" "spnDirection").value
			local undoString = "rotate " +  animDir as String
			if start == end then
				undo undoString on ( cryTools.cryAnim._f.rotateAnim animDir )
			else
			(
				undo undoString on ( cryTools.cryAnim._f.rotateAnim animDir range:true )
				return false
			)
		)
		
		saveSliderTime = sliderTime
		sliderTime = start
		
		if selection.count == 1 then
			disableSceneRedraw()
			
		
		
		
		

		tempIndex = 1
		if operation == 12 then
		(
			deleteFile cryTools.cryAnim.UI.main._v.tempINIPath
			cryTools.cryAnim._v.transformList = #()
			for f = 1 to $selection.count do
				append cryTools.cryAnim._v.transformList (#())
		)


		if selection.count == 0 then
		(
			print "Nothing selected."
			return false
		)
		
		
		
		
		
		for i = start to end by steps do
		(
			escapeEnable = true
			
			sliderTime = i
			case operation of
			(
				1:
				(
					if ddSelection.selection == 1 then
					(
						undo "make planted key" on
						(
							for obj in selection do
							(
								tempArray = obj.controller.keys
								keyIndex = 0
								for i = 1 to tempArray.count do
									if tempArray[i].time == sliderTime then keyIndex = i
								try
								(	
									tempKey = biped.getKey obj.controller keyIndex
									if tempKey.ikSpace != 1 or tempKey.ikJoinedPivot == false or tempKey.ikBlend != 1.0 then
										biped.setPlantedKey obj											
								)
								catch
								(
									try ( biped.setPlantedKey obj ) catch ( )
								)
							)
						)
					)
					else
					(
						undo "align biped object" on
							cryTools.cryAnim.UI.main.operation._f.align (getCurrentSelection()) (getNodeByName ddSelection.selected) snapshots:snapshots
					)
				)
				2:
				(
					if ddSelection.selection == 1 then
					(
						undo "make sliding key" on
						(
							for obj in selection do
							(
								local tempArray = obj.controller.keys
								local keyIndex = 0
								for i = 1 to tempArray.count do
									if tempArray[i].time == sliderTime then keyIndex = i

								try
								(
									local tempKey = biped.getKey obj.controller keyIndex
									if tempKey.ikSpace != 1 or tempKey.ikJoinedPivot == true or tempKey.ikBlend != 1.0 then 
										biped.setSlidingKey obj
								)
								catch
								(
									try ( biped.setSlidingKey obj ) catch ( )
								)
							)
						)
					)
					else
					(
						undo "lineOperation" on
						(
							for obj in selection do
								if obj.isHidden == false then												
									cryTools.cryAnim.UI.main.operation._f.alignLine startPos endPos axis obj (getNodeByName ddSelection.selected)
						)
					)										
				)
				3:
				(
					undo "make free key" on
					(
						for obj in selection do
						(
							tempArray = obj.controller.keys
							keyIndex = 0
							for i = 1 to tempArray.count do
								if tempArray[i].time == sliderTime then keyIndex = i
								
							try
							(
								tempKey = biped.getKey obj.controller keyIndex
								if tempKey.ikSpace != 0 or tempKey.ikJoinedPivot == false or tempKey.ikBlend != 0.0 then 
									biped.setFreeKey obj
							)
							catch
							(
								try	( biped.setFreeKey obj ) catch ( )
							)
						)
					)
				)				
			
				5:
				(
					case ddOpTo.selection of
					(
						1:( undo "move to snapshot" on ( cryTools.cryAnim._f.moveBipPart() ) )
						2:( undo "move to pivot point" on ( cryTools.cryAnim._f.moveBipPartPivPoint() ) )
					)
				)
				6:
				(
					case ddOpTo.selection of
					(
						1:( undo "move/rotate to snapshot" on ( cryTools.cryAnim._f.moveRotBipPart() ) )	
						2:( undo "move/rotate to pivot point" on ( cryTools.cryAnim._f.moveRotBipPartPivPoint() ) )	
					)
				)
				
				8:( undo "move to floor" on ( cryTools.cryAnim._f.moveToFloor value:((cryTools.cryAnim.UI.main._f.getUI "Operation" "spnDistance").value) ) )
				9:( undo "reset rotation" on ( cryTools.cryAnim._f.resetRotation() ) )
				
				12:
				(
					for d = 1 to $selection.count do
					(								
						setINISetting cryTools.cryAnim.UI.main._v.tempINIPath ("Object" + d as String) (tempIndex as String) ($selection[d].transform as String)
					)
				)
				13:
				(
					undo "paste transform" on
					(
						for d = 1 to $selection.count do
						(
							local newTran = getINISetting cryTools.cryAnim.UI.main._v.tempINIPath ("Object" + d as String) (tempIndex as String)								
							if newTran != "" then
							(
								newTran = execute ( newTran )
								local newObjTran = $selection[d].transform
								
								case ddOpTo.selection of
								(	
									1:( cryTools.cryAnim._f.pasteTransform $selection[d] newTran #transform )
									2:( cryTools.cryAnim._f.pasteTransform $selection[d] newTran #pos )
									3:( cryTools.cryAnim._f.pasteTransform $selection[d] newTran #rotation	)
								)
							)								
						)
					)
				)

				
				15:( undo "bip to [0,0]" on ( cryTools.cryAnim._f.bipToZero2() ) )
				16:( undo "bip to [0,0,0]" on ( cryTools.cryAnim._f.bipToZero3() ) )
			)
			
			tempIndex += 1
			
		)
		if isSceneRedrawDisabled() == true then
			enableSceneRedraw()
		
		if snapshots != undefined then
			for obj in snapshots do
				delete obj
			
		sliderTime = saveSliderTime
		
		if operation >= 11 and operation <= 13 then
			return false
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.operation._f.applyOperation" )
)
applyOperation = undefined
logOutput "> Created cryTools.cryAnim.UI.main.operation._f.applyOperation function"









































--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.pivot._f.pivotPoint
----------------------------------------------------------------------------------------------------------------------------------
--			applies the in operationRO selected operation to the selected node
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.pivot._f.pivotPoint = function pivotPoint operation =
(
	try
	(
		if (selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart() ) != undefined then
		(
			case operation of
			(
				#check:
				(									
					if selectedBipPart.pivotSel.pivPoint.position == undefined then
					(								
						return false
					)
					else
					(							
						return true
					)
				)
				#create:
				(
					ready = true
					try
					(
						addModifier selectedBipPart.object.selected (edit_mesh())
						local tempPos = (cryTools.cryAnim._f.getPivotPointPos selectedBipPart.pivotSel.index selectedBipPart.object.selected)
						deleteModifier selectedBipPart.object.selected 1
						
						local tempSnap = (cryTools.cryAnim._f.createSnapshot object:(getNodeByName selectedBipPart.pivotSel.pivObject))[1]
						tempSnap.wireColor = (color 255 0 0)
						tempSnap.showFrozenInGray = false
						tempSnap.isFrozen = true
						tempSnap.name = ("pivSnap_(" + selectedBipPart.name + ")")
						
						tempSphere = sphere pos:tempPos radius:0.2
						tempSphere.wireColor = (color 255 0 0)
						tempSphere.showFrozenInGray = false
						tempSphere.isFrozen = true
						tempSphere.name = ("pivPoint_(" + selectedBipPart.name + ")")
						
						execute ("cryTools.cryAnim.align._v.pivotLast." + selectedBipPart.part.name + ".pivPoint.name = \"" + tempSphere.name + "\"")
						execute ("cryTools.cryAnim.align._v.pivotLast." + selectedBipPart.part.name + ".pivPoint.position = " + (tempPos as String))
					)catch()									
				)
				#delete:
				(
					try
					(										
						execute ("cryTools.cryAnim.align._v.pivotLast." + selectedBipPart.part.name + ".pivPoint.name = undefined")
						execute ("cryTools.cryAnim.align._v.pivotLast." + selectedBipPart.part.name + ".pivPoint.position = undefined")
						delete (getNodeByName selectedBipPart.pivotSel.pivPoint.name)
						delete (getNodeByName ("pivSnap" + ( substring selectedBipPart.pivotSel.pivPoint.name 9 selectedBipPart.pivotSel.pivPoint.name.count )) )
						return true										
					)catch()
					
					return false
				)
			)
		)
		return undefined
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.pivot._f.pivotPoint" )
)
pivotPoint = undefined
logOutput "> Created cryTools.cryAnim.UI.main.pivot._f.pivotPoint function"







































--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.poseManager._f.outputINI
----------------------------------------------------------------------------------------------------------------------------------
--			outputs the poses into the temp\ini file of the character
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.poseManager._f.outputINI = function outputINI =
(
	try
	(
		local tempCharArray = #()
		local tempColArray = #()
		local tempPoseArray = #()
		
		local poses = cryTools.cryAnim.UI.main.poseManager._v.poses
		local tempString = ""
		local nodeString = ""
		
		
		local charFileArray = getFiles (cryTools.cryTemp + "CryAnimPoseManager\\*.ini")
		for i = 1 to charFileArray.count do
			try ( deleteFile charFileArray[i] ) catch()
		
		
			--// goes through the characters
		for i = 1 to poses.count do
		(
			local poseCount = 1
			local tempCharString = ""
				--// sets character filename
			for h = 1 to poses[i].nodes.count do
				tempCharString += "#" + poses[i].nodes[h]
			
			local tempFilename = (cryTools.cryTemp + "CryAnimPoseManager\\" + poses[i].name + ".ini")
			cryTools.cryAnim.UI.main._f.checkExport #createFolder tempFilename
			
			setINISetting tempFilename poses[i].name "nodes" tempCharString
			
			local tempColString = ""
			for f = 1 to poses[i].collections.count do
			(
				tempColString += "#" + poses[i].collections[f].name
				
				local tempPoseString
				for d = 1 to poses[i].collections[f].poses.count do
				(
					setINISetting tempFilename ("pose" + poseCount as String) "Collection" (f as String)
					setINISetting tempFilename ("pose" + poseCount as String) "Name" poses[i].collections[f].poses[d].name
					
					for k = 1 to poses[i].collections[f].poses[d].nodes.count do
					(
						local tempPoseString = poses[i].collections[f].poses[d].nodes[k].name + "/" + poses[i].collections[f].poses[d].nodes[k].transform as String
						setINISetting tempFilename ("pose" + poseCount as String) ("node" + k as String) tempPoseString
					)
					
					poseCount += 1
				)
			)
			setINISetting tempFilename poses[i].name "collections" tempColString
			
		)
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.poseManager._f.pivotPoint" )
)
outputINI = undefined
logOutput "> Created cryTools.cryAnim.UI.main.poseManager._f.outputINI function"














--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.poseManager._f.inputINI
----------------------------------------------------------------------------------------------------------------------------------
--			inputs the poses from the temp\ini file of the character
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.poseManager._f.inputINI = function inputINI =
(
	try
	(
		global superTempArray = #()
		local charFileArray = getFiles (cryTools.cryTemp + "CryAnimPoseManager\\*.ini")
		if charFileArray.count > 0 then
		(
			cryTools.cryAnim.UI.main.poseManager._v.poses = #()
			
			for i = 1 to charFileArray.count do
			(
				local charEntry = cryTools.cryAnim.base.perforce charFileArray[i] #getFilename
				charEntry = substring charEntry 1 (charEntry.count - 4)
				local charNodes = getINISetting charFileArray[i] charEntry "nodes"
				local charCollections = getINISetting charFileArray[i] charEntry "collections"
				
				if charNodes != "" then
					charNodes = filterString charNodes "#"
				else
					charNodes = #()
				
				if charCollections != "" then
					charCollections = filterString charCollections "#"
				else
					charCollections = #()
				
				for i = 1 to charCollections.count do
					charCollections[i] = collectionStruct name:charCollections[i]
				
				
				
				
				local tempPoseArray = #()
				for h = 1 to 200 do
				(
					if (local poseName = getINISetting charFileArray[i] ("pose" + h as String) "Name" ) != "" then
					(
						local poseNodeArray = #()
						local colIndex = (getINISetting charFileArray[i] ("pose" + h as String) "Collection") as Integer
						
						for g = 1 to 600 do
						(
							if (local tempNode = getINISetting charFileArray[i] ("pose" + h as String) ("node" + g as String)) != "" then
							(
								local tempArray = filterString tempNode "/"
								local tempValue = execute (tempArray[2])
								append poseNodeArray (nodeStruct name:tempArray[1] transform:tempValue )
							)
						)
							
						append charCollections[colIndex].poses (poseStruct name:poseName nodes:poseNodeArray)
					)
				)
				append cryTools.cryAnim.UI.main.poseManager._v.poses (charStruct name:charEntry nodes:charNodes collections:charCollections)
				
			)
		)
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.poseManager._f.inputINI" )
)
inputINI = undefined
logOutput "> Created cryTools.cryAnim.UI.main.poseManager._f.inputINI function"









--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.poseManager._f.createPoseBitmap
----------------------------------------------------------------------------------------------------------------------------------
--			renders a bitmap of the current scene and outputs them to the character folder
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.poseManager._f.createPoseBitmap = function createPoseBitmap outputPath =
(
	try
	(
		local chkNodesOnly = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "chkNodesOnly"
		local lbNodes = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "lbNodes"
		
		
		if chkNodesOnly.checked == true then
		(
			nodeRenderArray = Objects as Array
			newNodeRenderArray = #()
			for obj in Objects do
				append newNodeRenderArray obj.renderable
			
			for i = 1 to nodeRenderArray.count do
			(
				if (findItem lbNodes.items nodeRenderArray[i].name) != 0 then
					nodeRenderArray[i].renderable = true
				else
					nodeRenderArray[i].renderable = false
			)
		)
		
		local tempRender = render frame:#current outputsize:[144,110] renderhiddenobjects:false rendSimplifyAreaLights:true renderatmosphericeffects:false vfb:false outputHDRbitmap:false ComputeRadiosity:false progressbar:false renderMultiPassEffects:false mapping:false shadows:false autoReflect:false objectMotionBlur:false imageMotionBlur:false quiet:true
		if tempRender != undefined then
		(
			cryTools.cryAnim.UI.main._f.checkExport #createFolder outputPath
			tempRender.filename = outputPath
			save tempRender
			(cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "bmpPreview").bitmap = tempRender
			
			if chkNodesOnly.checked == true then
			(
				for i = 1 to nodeRenderArray.count do
					nodeRenderArray[i].renderable = newNodeRenderArray[i]
			)		
		)
		
		
		
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.poseManager._f.outputPath" )
)
outputPath = undefined
logOutput "> Created cryTools.cryAnim.UI.main.poseManager._f.outputPath function"











--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.poseManager._f.updateList
----------------------------------------------------------------------------------------------------------------------------------
--			updates the pose managers lists with the input ini settings
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.poseManager._f.updateList = function updateList objectName initial:undefined =
(
	try
	(
		local ddCharacter = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "ddCharacter"
		local btnDeleteCharacter = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "btnDeleteCharacter"
		local ddCollection = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "ddCollection"
		local btnDeleteCollection = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "btnDeleteCollection"
		local btnCreateCollection = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "btnCreateCollection"
		local ddPose = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "ddPose"
		local btnDeletePose = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "btnDeletePose"
		local btnCreatePose = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "btnCreatePose"
		local btnPastePose = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "btnPastePose"
		local lbNodes = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "lbNodes"
		local edCharacter = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "edCharacter"
		local edCollection = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "edCollection"
		local edPose = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "edPose"
		
		
		
		
		
		case objectName of
		(
			#character:
			(
				local tempCharNameArray = #()
				local tempColNameArray = #()
				local tempPoseNameArray = #()
				
				if cryTools.cryAnim.UI.main.poseManager._v.poses.count > 0 then
				(
					if initial == undefined then
						ddCharSel = ddCharacter.selection
					else
					(
						for i = 1 to cryTools.cryAnim.UI.main.poseManager._v.poses.count do
							append tempCharNameArray cryTools.cryAnim.UI.main.poseManager._v.poses[i].name
						
						ddCharacter.items = tempCharNameArray
						ddCharSel = 1
					)
					
					ddCollection.selection = 1
					ddPose.selection = 1
					
					lbNodes.items = cryTools.cryAnim.UI.main.poseManager._v.poses[ddCharSel].nodes
				
					if cryTools.cryAnim.UI.main.poseManager._v.poses[ddCharSel].collections.count > 0 then
					(
						for i = 1 to cryTools.cryAnim.UI.main.poseManager._v.poses[ddCharSel].collections.count do
							append tempColNameArray cryTools.cryAnim.UI.main.poseManager._v.poses[ddCharSel].collections[i].name
						
						if cryTools.cryAnim.UI.main.poseManager._v.poses[ddCharSel].collections[1].poses.count > 0 then
						for i = 1 to cryTools.cryAnim.UI.main.poseManager._v.poses[ddCharSel].collections[1].poses.count do
							append tempPoseNameArray cryTools.cryAnim.UI.main.poseManager._v.poses[ddCharSel].collections[1].poses[i].name
					)
				)
				
				ddCollection.items = tempColNameArray
				ddPose.items = tempPoseNameArray
				
			)
			
			#collection:
			(
				local tempPoseNameArray = #()
				
				if cryTools.cryAnim.UI.main.poseManager._v.poses[ddCharacter.selection].collections.count > 0 then
				(
					if cryTools.cryAnim.UI.main.poseManager._v.poses[ddCharacter.selection].collections[ddCollection.selection].poses.count > 0 then
						for i = 1 to cryTools.cryAnim.UI.main.poseManager._v.poses[ddCharacter.selection].collections[ddCollection.selection].poses.count do
							append tempPoseNameArray cryTools.cryAnim.UI.main.poseManager._v.poses[ddCharacter.selection].collections[ddCollection.selection].poses[i].name
				)
				
				ddPose.items = tempPoseNameArray
				ddPose.selection = 1
			)
		)
		
		if ddCharacter.items.count > 0 then
		(
			try(edCharacter.text = ddCharacter.items[ddCharSel])catch()
			btnDeleteCharacter.enabled = true
			ddCharacter.enabled = true
			edCharacter.enabled = true
			btnCreateCollection.enabled = true
		)
		else
		(
			edCharacter.text = ""
			btnDeleteCharacter.enabled = false
			ddCharacter.enabled = false
			edCharacter.enabled = false
			btnCreateCollection.enabled = false
		)
		
		
		if ddCollection.items.count > 0 then
		(
			try(edCollection.text = ddCollection.items[ddCollection.selection])catch()
			btnDeleteCollection.enabled = true
			ddCollection.enabled = true
			edCollection.enabled = true
			btnCreatePose.enabled = true
			btnPastePose.enabled = true
		)
		else
		(
			edCollection.text = ""
			btnDeleteCollection.enabled = false
			ddCollection.enabled = false
			edCollection.enabled = false
			btnCreatePose.enabled = false
			btnPastePose.enabled = false
		)
		
		if ddPose.items.count > 0 then
		(
			edPose.text = ddPose.items[ddPose.selection]
			btnDeletePose.enabled = true
			ddPose.enabled = true
			edPose.enabled = true
			btnPastePose.enabled = true
		)
		else
		(
			edPose.text = ""
			btnDeletePose.enabled = false
			ddPose.enabled = false
			edPose.enabled = false
			btnPastePose.enabled = false
		)
		
		
		try ( local tempBitmap = openBitMap (cryTools.cryTemp + "CryAnimPoseManager\\" + ddCharacter.items[ddCharacter.selection] + "\\" + ddCollection.items[ddCollection.selection] + "_" + ddPose.items[ddPose.selection] + ".jpg") )catch()
		if tempBitmap == undefined then
			tempBitmap = bitmap 144 110 color:(color 190 190 190)
		
		(cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "bmpPreview").bitmap = tempBitmap
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.poseManager._f.updateList" )
)
updateList = undefined
logOutput "> Created cryTools.cryAnim.UI.main.poseManager._f.updateList function"










--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.poseManager._f.addEntry
----------------------------------------------------------------------------------------------------------------------------------
--			updates the pose managers lists with the input ini settings
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.poseManager._f.addEntry = function addEntry edText objectName =
(
	try
	(
		local lbNodes = (cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "lbNodes")
		local ddChar = (cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "ddCharacter")
		local ddCol = (cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "ddCollection")
		local ddPose = (cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "ddPose")
		local poses = cryTools.cryAnim.UI.main.poseManager._v.poses
		
		local tempNodesArray = #()
		local tempCharArray = #()
		local tempVar = #()
		
		if edText != "" then
		(
			
			case objectName of
			(
				"Character":
				(
					if $selection.count != 0 then
					(
						if (queryBox "Take Nodes from Selection?" title:"Adding Nodes") == true then
							tempVar = getCurrentSelection()
						else
							tempVar = selectByName title:"Create Character Nodes Selection" showHidden:false single:false
					)
					else
						tempVar = selectByName title:"Create Character Nodes Selection" showHidden:false single:false
					
					if classOf tempVar == Array and tempVar.count > 0 then
					(
							--// sorts nodes list by root->children
						tempNodesArray = cryTools.sortRootChildren tempVar
						
						append cryTools.cryAnim.UI.main.poseManager._v.poses (charStruct name:edText)
						
						
						cryTools.cryAnim.UI.main.poseManager._v.poses[cryTools.cryAnim.UI.main.poseManager._v.poses.count].nodes = deepcopy tempNodesArray
						
						tempCharArray = ddChar.items
						append tempCharArray edText
						
						
						ddChar.items = tempCharArray
						lbNodes.items = tempNodesArray
						ddChar.selection = tempCharArray.count
						
						cryTools.cryAnim.UI.main.poseManager._f.updateList #character
					)
					else
						messageBox "No nodes selected." title:"Error creating Character"
				)
				
				"Collection":
				(
					local tempNodesArray = ddCol.items
					append tempNodesArray edText
					append cryTools.cryAnim.UI.main.poseManager._v.poses[ddChar.selection].collections (collectionStruct name:edText)
					ddCol.items = tempNodesArray
					ddCol.selection = ddCol.items.count
					
					cryTools.cryAnim.UI.main.poseManager._f.updateList #collection
				)
				
				"Pose":
				(
					local tempArray = #()
					
					in coordsys world
					(
						for i = 1 to poses[ddChar.selection].nodes.count do
						(
							local tempObj = getNodeByName poses[ddChar.selection].nodes[i]
							
							if tempObj != undefined then
							(
									--// find sliderManipulator
								if tempObj.classID[1] == 1205540079 then
									local tempTransform = tempObj.value
									--// or normal node
								else
									local tempTransform = tempObj.transform
								
								try
								(
									if getClassName tempObj.controller.position.controller == "Path Constraint" then
										tempTransform = tempObj.controller.position.controller.percent
								)catch()
								
								append tempArray (nodeStruct name:tempObj.name transform:tempTransform)
							)
						)
					)
					
					local tempNodesArray = ddPose.items
					append tempNodesArray edText
					append cryTools.cryAnim.UI.main.poseManager._v.poses[ddChar.selection].collections[ddCol.selection].poses (poseStruct name:edText nodes:tempArray)
					ddPose.items = tempNodesArray
					ddPose.selection = ddPose.items.count
					
					cryTools.cryAnim.UI.main.poseManager._f.createPoseBitmap (cryTools.cryTemp + "CryAnimPoseManager\\" + ddChar.items[ddChar.selection] + "\\" + ddCol.items[ddCol.selection] + "_" + ddPose.items[ddPose.selection] + ".jpg")
					
					cryTools.cryAnim.UI.main.poseManager._f.updateList #default
				)
			)
			
			cryTools.cryAnim.UI.main.poseManager._f.outputINI()
		)
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.poseManager._f.addEntry" )
)
addEntry = undefined
logOutput "> Created cryTools.cryAnim.UI.main.poseManager._f.addEntry function"















--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.poseManager._f.createEntry
----------------------------------------------------------------------------------------------------------------------------------
--			creates a new entry and adds the default text to the corresponding edittext
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.poseManager._f.createEntry = function createEntry objectName =
(
	try
	(	
		local ddCharacter = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "ddCharacter"
		local ddCollection = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "ddCollection"
		
		
		local edName = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" ("ed" + objectName)
		local ddList = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" ("dd" + objectName)
		
		cryTools.cryAnim._v.various[55] = objectName
		local maxDefaultEntriesFound = 0
		
		for i = 1 to ddList.items.count do
		(
			local tempString = objectName + (if i < 10 then "0" else "") + i as String
			if (findItem ddList.items tempString ) != 0 then
				maxDefaultEntriesFound += 1
		)
		
		maxDefaultEntriesFound += 1
		
		
		local entryText = objectName + (if maxDefaultEntriesFound < 10 then "0" else "") + maxDefaultEntriesFound  as String
		
		
		cryTools.cryAnim.UI.main.poseManager._f.addEntry entryText objectName
		
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.poseManager._f.createEntry" )
)
createEntry = undefined
logOutput "> Created cryTools.cryAnim.UI.main.poseManager._f.createEntry function"









--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.poseManager._f.deleteEntry
----------------------------------------------------------------------------------------------------------------------------------
--			deletes the selected entry of the corresponding listbox
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.poseManager._f.deleteEntry = function deleteEntry objectName =
(
	try
	(
		local lbNodes = (cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "lbNodes")
		local ddChar = (cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "ddCharacter")
		local ddCol = (cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "ddCollection")
		local ddPose = (cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "ddPose")
		local edName = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" ("ed" + objectName)
		local poses = cryTools.cryAnim.UI.main.poseManager._v.poses
		
		case objectName of
		(
			"Character":
			(
				if ddChar.items.count > 0 then
				(
					if queryBox ("Delete " + ddChar.items[ddChar.selection] + " ?") title:"Delete Item" == true then
					(
						lbNodes.items = #()
						local tempArray = ddChar.items
						
						try (deleteFile (cryTools.cryTemp + "CryAnimPoseManager\\" + ddChar.items[ddChar.selection] + ".ini") )catch()
						
						local tempFileArray = getFiles (cryTools.cryTemp + "CryAnimPoseManager\\" + ddChar.items[ddChar.selection] + "\\*.jpg")
						
						for i = 1 to tempFileArray.count do
							try (deleteFile tempFileArray[i] )catch()
						
									
						local tempSelection = ddChar.selection
						deleteItem cryTools.cryAnim.UI.main.poseManager._v.poses ddChar.selection
						deleteItem tempArray ddChar.selection
						ddChar.items = tempArray
						ddChar.selection = ddChar.items.count
						
						if tempSelection > 1 then
							ddChar.selection = tempSelection - 1
						else
							ddChar.selection = tempSelection
						
						cryTools.cryAnim.UI.main.poseManager._f.updateList #character
						
					)
				)
			)
			
			"Collection":
			(
				if ddCol.items.count > 0 then
				(
					if queryBox ("Delete " + ddCol.items[ddCol.selection] + " ?") title:"Delete Item" == true then
					(
						local tempArray = ddCol.items
						for i = 1 to poses[ddChar.selection].collections[ddCol.selection].poses.count do
							try (deleteFile (cryTools.cryTemp + "CryAnimCharacter_" + poses[ddChar.selection].name + "_" + poses[ddChar.selection].collections[ddCol.selection].name + "_" + poses[ddChar.selection].collections[ddCol.selection].poses[i].name + ".jpg") )catch()
						local tempSelection = ddCol.selection
						deleteItem cryTools.cryAnim.UI.main.poseManager._v.poses[ddChar.selection].collections ddCol.selection
						deleteItem tempArray ddCol.selection
						ddCol.items = tempArray
								
						if tempSelection > 1 then
							ddCol.selection = tempSelection - 1
						else
							ddCol.selection = tempSelection
						
						cryTools.cryAnim.UI.main.poseManager._f.updateList #collection
					)
				)
			)
			
			"Pose":
			(
				if ddPose.items.count > 0 then
				(
					if queryBox ("Delete " + ddPose.items[ddPose.selection] + " ?") title:"Delete Item" == true then
					(
						local tempArray = ddPose.items
						try (deleteFile (cryTools.cryTemp + "CryAnimPoseManager\\" + ddCharacter.items[ddCharacter.selection] + "\\" + ddCollection.items[ddCollection.selection] + "_" + ddPose.items[ddPose.selection] + ".jpg") )catch()
						local tempSelection = ddPose.selection
						deleteItem cryTools.cryAnim.UI.main.poseManager._v.poses[ddChar.selection].collections[ddCol.selection].poses ddPose.selection
						deleteItem tempArray ddPose.selection
						ddPose.items = tempArray
						
						if tempSelection > 1 then
							ddPose.selection = tempSelection - 1
						else
							ddPose.selection = tempSelection
						
						cryTools.cryAnim.UI.main.poseManager._f.updateList #default
					)
				)
			)
		)
		cryTools.cryAnim.UI.main.poseManager._f.outputINI()
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.poseManager._f.deleteEntry" )
)
deleteEntry = undefined
logOutput "> Created cryTools.cryAnim.UI.main.poseManager._f.deleteEntry function"










--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.poseManager._f.renameEntry
----------------------------------------------------------------------------------------------------------------------------------
--			renames the selected entry of the corresponding listbox
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.poseManager._f.renameEntry = function renameEntry objectName =
(
	try
	(
		local poses = cryTools.cryAnim.UI.main.poseManager._v.poses
		local tempPath = (cryTools.cryTemp + "CryAnimPoseManager\\")
			
		local ddCharacter = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "ddCharacter"
		local ddCollection = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "ddCollection"
		local ddPose = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "ddPose"
		local ddList = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" ("dd" + objectName)
		local edName = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" ("ed" + objectName)
		
		local tempArray = ddList.items
		
		case objectName of
		(
			"Character":
			(
				try ( renameFile (tempPath + ddCharacter.items[ddCharacter.selection]) (tempPath + edName.text) ) catch()
				
				poses[ddCharacter.selection].name = edName.text
			)
			
			"Collection":
			(
				local tempFileArray = getFiles (tempPath + ddCharacter.items[ddCharacter.selection] + "\\" + ddCollection.items[ddCollection.selection] + "*.jpg")
				if tempFileArray.count > 0 then
				(
					for i = 1 to tempFileArray.count do
					(
						local tempFileFilter = filterString tempFileArray[i] "\\"
						local tempFileString = ""
						for f = 1 to (tempFileFilter.count - 1) do
							tempFileString += tempFileFilter[f] + "\\"
						
						tempFileString += edName.text + "_" + (filterString tempFileFilter[tempFileFilter.count] "_")[2]
						
						try ( renameFile tempFileArray[i] tempFileString ) catch()
					)
				)
				
				poses[ddCharacter.selection].collections[ddCollection.selection].name = edName.text
			)
			
			"Pose":
			(
				tempFilePath = tempPath + ddCharacter.items[ddCharacter.selection] + "\\" + ddCollection.items[ddCollection.selection] + "_"
				try ( renameFile (tempFilePath + ddPose.items[ddPose.selection] + ".jpg") (tempFilePath + edName.text + ".jpg") ) catch()
				
				poses[ddCharacter.selection].collections[ddCollection.selection].poses[ddPose.selection].name = edName.text
			)
		)


		tempArray[ddList.selection] = edName.text
		ddList.items = tempArray

		cryTools.cryAnim.UI.main.poseManager._f.outputINI()
		cryTools.cryAnim.UI.main.poseManager._f.updateList #collection
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.poseManager._f.renameEntry" )
)
renameEntry = undefined
logOutput "> Created cryTools.cryAnim.UI.main.poseManager._f.renameEntry function"











--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.poseManager._f.applyPose
----------------------------------------------------------------------------------------------------------------------------------
--			applies the selected pose to the character
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.poseManager._f.applyPose = function applyPose =
(
	try
	(
			--// poses array with characters, collections and poses
		local poses = cryTools.cryAnim.UI.main.poseManager._v.poses
		local ddCharacter = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "ddCharacter"
		local ddCollection = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "ddCollection"
		local ddPose = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "ddPose"
		local lbNodes = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "lbNodes"
		
		local chkBoneOnly = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "chkBoneOnly"
		local chkControllerValue = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "chkControllerValue"
		local chkPasteRotation = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "chkPasteRotation"
		local chkPastePosition = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "chkPastePosition"
		local chkByVelocity = cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "chkByVelocity"
		---------------------------------------------------------------------------
		local tempNodeArray = #()
		
		if chkBoneOnly.checked == false then
			tempNodeArray = poses[ddCharacter.selection].collections[ddCollection.selection].poses[ddPose.selection].nodes
		else
		(
			local tempSelArray = lbNodes.selection as Array
			for i = 1 to tempSelArray.count do
				append tempNodeArray poses[ddCharacter.selection].collections[ddCollection.selection].poses[ddPose.selection].nodes[tempSelArray[i]]
		)
		
		---------------------------------------------------------------------------
		local boneMissingMessage = "Could not find Bones:\n\n"
		local boneMissing = false
		
		for i = 1 to tempNodeArray.count do
		(
			if (getNodeByName tempNodeArray[i].name) == undefined then
			(
				boneMissingMessage += tempNodeArray[i].name + "\n"
				boneMissing = true
			)
		)
		
		if boneMissing == true then
		(
			if queryBox (boneMissingMessage + "\n\nContinue with pasting the pose?") title:"Bones Missing" == false then
				return undefined
		)
		---------------------------------------------------------------------------
			--// get root node
		local root = getNodeByName poses[ddCharacter.selection].collections[ddCollection.selection].poses[ddPose.selection].nodes[1].name
		
		if root == undefined then
			return undefined
		
		--// get Locator_Locomotion (prop1) if available
		try (local bipProp = biped.getNode (getNodeByName poses[ddCharacter.selection].nodes[1]) #prop1) catch()
		---------------------------------------------------------------------------
		
		with redraw off
		(
			for i = 1 to tempNodeArray.count do
			(
				try
				(
					in coordsys world
					(
						local tempNode = getNodeByName tempNodeArray[i].name
						if tempNode != undefined then
						(
							local newPos = tempNode.transform.pos
							local safePos = tempNode.transform.pos
							
								--// node is a sliderManipulator
							case tempNode.classID[1] of
							(
									--// sliderManipulator
								1205540079: 
								(
									try
									(
										if chkControllerValue.checked == true then
											tempNode.value = tempNodeArray[i].transform
									)catch()
								)
									--// biped object
								37157:
								(
									if chkPasteRotation.checked == true then
										try ( biped.setTransform tempNode #rotation tempNodeArray[i].transform.rotation true ) catch()
									
									if chkPastePosition.checked == true then
									(
										if chkByVelocity.checked == false then
										(
											try ( biped.setTransform tempNode #pos tempNodeArray[i].transform.pos true ) catch()
										)
										else
										(
											if tempNodeArray[i].name != root.name then
											(
												local offsetPos = tempNodeArray[1].transform.pos - tempNodeArray[i].transform.pos
												local newOffsetPos = root.transform.pos - offsetPos
												
												try ( biped.setTransform tempNode #pos newOffsetPos true ) catch()
											)
										)
									)
								)
								
								default:
								(
									local isNormalNode = false
									
										--// if node is path constraint
									try
									(
										if getClassName tempNode.controller.position.controller == "Path Constraint" then
										(
											if chkControllerValue.checked == true then
												try ( tempNode.controller.position.controller.percent = tempNodeArray[i].transform ) catch()
										)
										else
											isNormalNode = true
									)
									catch( isNormalNode = true)
									
										--// normal node
									if isNormalNode == true then
									(
										if chkPasteRotation.checked == true then
											try ( tempNode.rotation = inverse tempNodeArray[i].transform.rotation ) catch()
										
										if chkPastePosition.checked == true then
										(
											if chkByVelocity.checked == false then
												newPos = tempNodeArray[i].transform.pos
											else
											(
												if tempNodeArray[i].name != root.name then
												(
													local offsetPos = tempNodeArray[1].transform.pos - tempNodeArray[i].transform.pos
													newPos = root.transform.pos - offsetPos
												)
											)
										)
										else
											newPos = safePos
										
										try ( tempNode.pos = newPos ) catch()
									)
								)
							)
						)
					)
				)
				catch()
			)
		)
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.poseManager._f.applyPose" )
)
applyPose = undefined
logOutput "> Created cryTools.cryAnim.UI.main.poseManager._f.applyPose function"



































--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.loadSave._f.checkRange
----------------------------------------------------------------------------------------------------------------------------------
--			checks the range of the animation and timeline range
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.loadSave._f.checkRange = function checkRange begin end =
(
	try
	(
		local tempTextString = "The current timeline range differs from the selected range.\nDo you want to use the current time range?\n\n"
						
		local tempTimeString = #( ( (animationRange.start.frame as Integer) as String + " -> " + (animationRange.end.frame as Integer) as String), ( begin as String + " -> " + end as String ) )
											
		local tempInt = #( (getTextExtent tempTimeString[1])[1], (getTextExtent tempTimeString[2])[1] )
		local tempSep = #()
						

						
		for i = 1 to tempInt.count do
		(
			tempSepString = ""
			for f = 1 to tempInt[i] by 3 do
				tempSepString += " "
		
			tempSep[i] = tempSepString										
		)
						
		tempSep[2] = substring tempSep[2] 1 (tempSep[2].count - (tempSep[1].count - tempSep[2].count))
						
						
						
		if ( queryBox (tempTextString + "                  " + tempTimeString[1] + "    " + tempSep[2] + tempTimeString[2] ) title:("Animation Range") ) == true then					
			return interval animationRange.start.frame animationRange.end.frame
		else
			return interval begin end
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.loadSave._f.checkRange" )
)
checkRange = undefined
logOutput "> Created cryTools.cryAnim.UI.main.loadSave._f.checkRange function"










--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.loadSave._f.getProgressInfo
----------------------------------------------------------------------------------------------------------------------------------
--			returns the current progress text or the width
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.loadSave._f.getProgressInfo = function getProgressInfo op =
(
	try
	(
		local tempString = ("      Processing Animation:          " + cryTools.cryAnim.base.perforce cryTools.cryAnim.UI.main.loadSave._v.cafSavePath #getFilename)
		local tempSize = getTextExtent tempString
		case op of
		(
			#width:
				return tempSize[1]
			#text:
				return tempString
		)
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.loadSave._f.getProgressInfo" )
)
getProgressInfo = undefined
logOutput "> Created cryTools.cryAnim.UI.main.loadSave._f.getProgressInfo function"









--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.loadSave._f.updateProgressBar
----------------------------------------------------------------------------------------------------------------------------------
--			refreshes the progressbar
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.loadSave._f.updateProgressBar = function updateProgressBar tempBitmap phase =
(
	try
	(
		maxColor = (color 255 0 0)--(color 230 100 0)
		minColor = (color 0 0 0)--(color 110 120 130)	
		
		
		for x = 0 to tempBitmap.width do
		(			
			tempColor = (getPixels tempBitmap [x,0] 1)[1]			
			
			for y = 0 to tempBitmap.height + 1 do
			(
				diff = phase - x
				if diff < 0 then
					diff -= diff * 2
								
				
				percent = ((diff * 100) /  tempBitmap.width)
				newColor = (maxColor / percent) + minColor
				
				setPixels tempBitmap [x,y] #(newColor)					
			)
		)		
		
		return tempBitmap
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.loadSave._f.updateProgressBar" )
)
updateProgressBar = undefined
logOutput "> Created cryTools.cryAnim.UI.main.loadSave._f.updateProgressBar function"










--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.loadSave._f.progressDialog
----------------------------------------------------------------------------------------------------------------------------------
--			creates or deletes the test bar progress dialog
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.loadSave._f.progressDialog = function progressDialog op =
(
	try
	(
		case op of
		(
			#destroy:
			(
				destroyDialog cryTools.cryAnim.UI.main.loadSave.testBar
			)
			#generate:
			(
				try (destroyDialog cryTools.cryAnim.UI.main.loadSave.testBar) catch()
				rollout testBar "Exporting Animation" width:300 height:32
				(
					label progressInfo "" pos:[10,5] height:15 align:#left
					bitmap progressBar pos:[10,20] bitmap:(bitmap ((getProgressInfo #width) + 20) 5 color:(color 80 80 80))	
					timer clock interval:80 active:true
					
					on testBar open do
					(
						progressInfo.text = cryTools.cryAnim.UI.main.loadSave._f.getProgressInfo #text
						testBar.width = (cryTools.cryAnim.UI.main.loadSave._f.getProgressInfo #width) + 40
						forward = true
						global colorDirection = true
						global tempX = 0
					)
					
					on clock tick do
					(		
						progressBar.bitmap = cryTools.cryAnim.UI.main.loadSave._f.updateProgressBar progressBar.bitmap tempX
						if tempX >= progressBar.bitmap.width then
							colorDirection = false
						if tempX <= 0 then
							colorDirection = true
						
						if colorDirection == true then
							tempX += 10
						else			
							tempX -= 10
					)
					
				)
				cryTools.cryAnim.UI.main.loadSave.testBar = testBar
				testBar = undefined

				createDialog cryTools.cryAnim.UI.main.loadSave.testBar --style:#(#style_sunkenedge)
			)
		)
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.loadSave._f.progressDialog" )
)
progressDialog = undefined
logOutput "> Created cryTools.cryAnim.UI.main.loadSave._f.progressDialog function"









--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.loadSave._f.bakeMuscleBones
----------------------------------------------------------------------------------------------------------------------------------
--			bakes muscle bones to keys
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.loadSave._f.bakeMuscleBones = function bakeMuscleBones =
(
	try
	(
		if (cryTools.cryAnim.UI.main._f.getUI "Muscles" "Auto-Muscles").checked == true then
		(
			if $'_Bip01 L knee_Con' != undefined then
			(
				global automateAnimateMuscles = false
				global useMuscleKeys = undefined
				
				saveSliderTime = sliderTime
				sliderTime = animationRange.end
				
				local tempMode = getCommandPanelTaskMode()
				setCommandPanelTaskMode #motion


				for i = animationRange.start to animationRange.end do
				(
					escapeEnable = true
					sliderTime = i
				)
				
				sliderTime = saveSliderTime	
			
				global automateAnimateMuscles = false
				global useMuscleKeys = true							
			)
			else
				return undefined
		)
		else return false
			
		
		
		if (cryTools.cryAnim.UI.main._f.getUI "Muscles" "Auto-Muscles").checked == true then
			global automateAnimateMuscles = undefined
		else							
			global automateAnimateMuscles = false

		if (cryTools.cryAnim.UI.main._f.getUI "Muscles" "Use Keys").checked == true then
			global useMusclesKeys = true
		else
			global useMusclesKeys = undefined
			
		return true
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.loadSave._f.bakeMuscleBones" )
)
bakeMuscleBones = undefined
logOutput "> Created cryTools.cryAnim.UI.main.loadSave._f.bakeMuscleBones function"








--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.loadSave._f.getStatusExtents
----------------------------------------------------------------------------------------------------------------------------------
--			returns the extent of the lbStatus entry
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.loadSave._f.getStatusExtents = function getStatusExtents =
(
	try
	(
		local tempSLB = (cryTools.cryAnim.UI.main._f.getUI "Load / Save / Export" "").lbStatus
		local tempSFLB = (cryTools.cryAnim.UI.main._f.getUI "Load / Save / Export" "").lbStatusFilepath
		local tempSize = getTextExtent tempSFLB.items[tempSLB.selection]
			
		return tempSize[1]
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.loadSave._f.getStatusExtents" )
)
getStatusExtents = undefined
logOutput "> Created cryTools.cryAnim.UI.main.loadSave._f.getStatusExtents function"





































--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.target._f.updateDDSelection
----------------------------------------------------------------------------------------------------------------------------------
--			updates the listbox in targetRO and changes settings in operationRO
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.target._f.updateDDSelection = function updateDDSelection op:undefined =
(
	try
	(
		local ddSelection = cryTools.cryAnim.UI.main._f.getUI "Target" "ddSelection"
		local grpAxis = cryTools.cryAnim.UI.main._f.getUI "Target" "grpAxis"
		local labX = cryTools.cryAnim.UI.main._f.getUI "Target" "labX"
		local labY = cryTools.cryAnim.UI.main._f.getUI "Target" "labY"
		local labZ = cryTools.cryAnim.UI.main._f.getUI "Target" "labZ"
		local labOffset = cryTools.cryAnim.UI.main._f.getUI "Target" "labOffset"
		local labPos = cryTools.cryAnim.UI.main._f.getUI "Target" "labPos"
		local chkX = cryTools.cryAnim.UI.main._f.getUI "Target" "chkX"
		local chkY = cryTools.cryAnim.UI.main._f.getUI "Target" "chkY"
		local chkZ = cryTools.cryAnim.UI.main._f.getUI "Target" "chkZ"
		local chkPosOffset = cryTools.cryAnim.UI.main._f.getUI "Target" "chkPosOffset"
		local labRot = cryTools.cryAnim.UI.main._f.getUI "Target" "labRot"
		local chkXRot = cryTools.cryAnim.UI.main._f.getUI "Target" "chkXRot"
		local chkYRot = cryTools.cryAnim.UI.main._f.getUI "Target" "chkYRot"
		local chkZRot = cryTools.cryAnim.UI.main._f.getUI "Target" "chkZRot"
		local chkRotOffset = cryTools.cryAnim.UI.main._f.getUI "Target" "chkRotOffset"
		local btnSetOffset = cryTools.cryAnim.UI.main._f.getUI "Target" "btnSetOffset"
		local labEditOffset1 = cryTools.cryAnim.UI.main._f.getUI "Target" "labEditOffset1"
		local labEditOffset2 = cryTools.cryAnim.UI.main._f.getUI "Target" "labEditOffset2"
		local edOffset1 = cryTools.cryAnim.UI.main._f.getUI "Target" "edOffset1"
		local edOffset2 = cryTools.cryAnim.UI.main._f.getUI "Target" "edOffset2"
		
		
		local tempArray = #()
		local tempArrayNames = #()
							
		for obj in objects do
		(
			if obj.isHidden == false then					
				append tempArray obj.name
		)
		
		sort tempArray
		
		if tempArray.count != 0 then
			insertItem "----------------------------------------" tempArray 1

		insertItem "--------- Update List -----------" tempArray 1
		insertItem "----- Current Selection ------" tempArray 1
			
		if ddSelection.items.count != tempArray.count then
			ddSelection.items = tempArray

		local groupArray = #(\
			grpAxis, labX, labY, labZ, labOffset, \
			labPos, chkX, chkY, chkZ, chkPosOffset, \
			labRot, chkXRot, chkYRot, chkZRot, chkRotOffset, btnSetOffset, \
			labEditOffset1, labEditOffset2, edOffset1, edOffset2 )

		if op == false then
		(
			for i = 1 to groupArray.count do
				groupArray[i].enabled = false
			
		)
		if op == true then
		(
			for i = 1 to groupArray.count do
				groupArray[i].enabled = true
			
		)
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.target._f.updateDDSelection" )
)
updateDDSelection = undefined
logOutput "> Created cryTools.cryAnim.UI.main.target._f.updateDDSelection function"








--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.target._f.updateOffset
----------------------------------------------------------------------------------------------------------------------------------
--			generates the offset of the selected node and selected item in the list
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.target._f.updateOffset = function updateOffset =
(
	try
	(
		local tempArray = #([0,0,0], (eulerangles 0 0 0))
		local chkPosOffset = cryTools.cryAnim.UI.main._f.getUI "Target" "chkPosOffset"
		local chkRotOffset = cryTools.cryAnim.UI.main._f.getUI "Target" "chkRotOffset"
		local labEditOffset1 = cryTools.cryAnim.UI.main._f.getUI "Target" "labEditOffset1"
		local labEditOffset2 = cryTools.cryAnim.UI.main._f.getUI "Target" "labEditOffset2"
		local edOffset1 = cryTools.cryAnim.UI.main._f.getUI "Target" "edOffset1"
		local edOffset2 = cryTools.cryAnim.UI.main._f.getUI "Target" "edOffset2"
		local grpAxis = cryTools.cryAnim.UI.main._f.getUI "Target" "grpAxis"
		
		try
		(
			if cryTools.cryAnim._v.various[35].count > 0 then
			(
				if chkPosOffset.checked == true then
					tempArray[1] = cryTools.cryAnim._v.various[35][1].position as String
				if chkRotOffset.checked == true then
					tempArray[2] = cryTools.cryAnim._v.various[35][1].rotation as eulerangles
				
				tempArray[2] = (tempArray[2].x as Integer) as String + " " + (tempArray[2].y as Integer) as String + " " + (tempArray[2].z as Integer) as String
			)
		)catch()
		
		
		
		if chkPosOffset.checked != chkRotOffset.checked then
		(
			labEditOffset1.visible = true
			edOffset1.visible = true
			labEditOffset2.visible = false
			edOffset2.visible = false
			grpAxis.height = 107			
			
			(cryTools.cryAnim.UI.main._f.getUI "Target" "").height = 148
			
			if chkPosOffset.checked == true then
			(
				labEditOffset1.text = "Pos"
				edOffset1.text = tempArray[1]
			)
			if chkRotOffset.checked == true then
			(
				labEditOffset1.text = "Rot"
				edOffset1.text = tempArray[2]
			)
		)
		
		if chkPosOffset.checked == false and chkRotOffset.checked == false then
		(
			labEditOffset1.visible = false
			labEditOffset2.visible = false
			edOffset1.visible = false
			edOffset2.visible = false
			grpAxis.height = 85
			
			(cryTools.cryAnim.UI.main._f.getUI "Target" "").height = 128
		)
		
		if chkPosOffset.checked == true and chkRotOffset.checked == true then
		(
			labEditOffset1.visible = true
			labEditOffset2.visible = true
			edOffset1.visible = true
			edOffset2.visible = true
			grpAxis.height = 125
			
			labEditOffset1.text = "Pos"
			edOffset1.text = tempArray[1]
			labEditOffset2.text = "Rot"
			edOffset2.text = tempArray[2]
			
			(cryTools.cryAnim.UI.main._f.getUI "Target" "").height = 166
		)
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.target._f.updateOffset" )
)
updateOffset = undefined
logOutput "> Created cryTools.cryAnim.UI.main.target._f.updateOffset function"









--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim.UI.main.target._f.setManualOffset
----------------------------------------------------------------------------------------------------------------------------------
--			sets the new value for the offset of the selected node and the selected item in the listbox
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryTools.cryAnim.UI.main.target._f.setManualOffset = function setManualOffset lab ed =
(
	try
	(
		local tempVar
		local error = false
		try
		(
			tempVar = execute ("eulerangles " + ed.text)
		)
		catch ( error = true )
		
		if lab.text == "Pos" then
		(
			if error == false then
			(
				if classOf tempVar == Point3 then
				(
					cryTools.cryAnim._v.various[35][1].position = tempVar
					print "Changed Offset Pos"
				)
				else
				(
					ed.text = cryTools.cryAnim._v.various[35][1].position as String
					error = true
				)
			)
			else
			(
				ed.text = cryTools.cryAnim._v.various[35][1].position as String
			)
		)
		if lab.text == "Rot" then
		(
			local tempRot = cryTools.cryAnim._v.various[35][1].rotation as eulerangles
			tempRot = (tempRot.x as Integer) as String + " " + (tempRot.y as Integer) as String + " " + (tempRot.z as Integer) as String
			if error == false then
			(
				if classOf tempVar == EulerAngles then
				(
					cryTools.cryAnim._v.various[35][1].rotation = tempVar
					print "Changed Offset Rot"
				)
				else
				(
					ed.text = tempRot
					error = true
				)
			)
			else
			(
				ed.text = tempRot
			)
		)
		
		if error == true then
			messageBox "Invalid value entered" title:("Manual " + lab.text + " Offset")
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.target._f.setManualOffset" )
)
setManualOffset = undefined
logOutput "> Created cryTools.cryAnim.UI.main.target._f.setManualOffset function"












cryTools.cryAnim.UI.main.settings._f.callCustomizeDialog = function callCustomizeDialog =
(
	try
	(
		rollout customizeRolloutsRO "Customize Rollouts"
		(
			
			label labRow1 "Row 1" pos:[28,8]
			dropdownlist ddRow1_1 "" pos:[8,25] width:90 height:15
			dropdownlist ddRow1_2 "" pos:[8,45] width:90 height:15
			dropdownlist ddRow1_3 "" pos:[8,65] width:90 height:15
			dropdownlist ddRow1_4 "" pos:[8,85] width:90 height:15
			dropdownlist ddRow1_5 "" pos:[8,105] width:90 height:15
			dropdownlist ddRow1_6 "" pos:[8,125] width:90 height:15
			dropdownlist ddRow1_7 "" pos:[8,145] width:90 height:15
			dropdownlist ddRow1_8 "" pos:[8,165] width:90 height:15
			dropdownlist ddRow1_9 "" pos:[8,185] width:90 height:15
			dropdownlist ddRow1_10 "" pos:[8,205] width:90 height:15
			
			
			
			label labRow2 "Row 2" pos:[128,8]
			dropdownlist ddRow2_1 "" pos:[108,25] width:90 height:15
			dropdownlist ddRow2_2 "" pos:[108,45] width:90 height:15
			dropdownlist ddRow2_3 "" pos:[108,65] width:90 height:15
			dropdownlist ddRow2_4 "" pos:[108,85] width:90 height:15
			dropdownlist ddRow2_5 "" pos:[108,105] width:90 height:15
			dropdownlist ddRow2_6 "" pos:[108,125] width:90 height:15
			dropdownlist ddRow2_7 "" pos:[108,145] width:90 height:15
			dropdownlist ddRow2_8 "" pos:[108,165] width:90 height:15
			dropdownlist ddRow2_9 "" pos:[108,185] width:90 height:15
			dropdownlist ddRow2_10 "" pos:[108,205] width:90 height:15
			
			
			
			label labRow3 "Row 3" pos:[228,8]
			dropdownlist ddRow3_1 "" pos:[208,25] width:90 height:15
			dropdownlist ddRow3_2 "" pos:[208,45] width:90 height:15
			dropdownlist ddRow3_3 "" pos:[208,65] width:90 height:15
			dropdownlist ddRow3_4 "" pos:[208,85] width:90 height:15
			dropdownlist ddRow3_5 "" pos:[208,105] width:90 height:15
			dropdownlist ddRow3_6 "" pos:[208,125] width:90 height:15
			dropdownlist ddRow3_7 "" pos:[208,145] width:90 height:15
			dropdownlist ddRow3_8 "" pos:[208,165] width:90 height:15
			dropdownlist ddRow3_9 "" pos:[208,185] width:90 height:15
			dropdownlist ddRow3_10 "" pos:[208,205] width:90 height:15
			
			
			
			
			label labRow4 "Row 4" pos:[328,8]
			dropdownlist ddRow4_1 "" pos:[308,25] width:90 height:15
			dropdownlist ddRow4_2 "" pos:[308,45] width:90 height:15
			dropdownlist ddRow4_3 "" pos:[308,65] width:90 height:15
			dropdownlist ddRow4_4 "" pos:[308,85] width:90 height:15
			dropdownlist ddRow4_5 "" pos:[308,105] width:90 height:15
			dropdownlist ddRow4_6 "" pos:[308,125] width:90 height:15
			dropdownlist ddRow4_7 "" pos:[308,145] width:90 height:15
			dropdownlist ddRow4_8 "" pos:[308,165] width:90 height:15
			dropdownlist ddRow4_9 "" pos:[308,185] width:90 height:15
			dropdownlist ddRow4_10 "" pos:[308,205] width:90 height:15
			
			
			
			
			label labRow5 "Row 5" pos:[428,8]
			dropdownlist ddRow5_1 "" pos:[408,25] width:90 height:15
			dropdownlist ddRow5_2 "" pos:[408,45] width:90 height:15
			dropdownlist ddRow5_3 "" pos:[408,65] width:90 height:15
			dropdownlist ddRow5_4 "" pos:[408,85] width:90 height:15
			dropdownlist ddRow5_5 "" pos:[408,105] width:90 height:15
			dropdownlist ddRow5_6 "" pos:[408,125] width:90 height:15
			dropdownlist ddRow5_7 "" pos:[408,145] width:90 height:15
			dropdownlist ddRow5_8 "" pos:[408,165] width:90 height:15
			dropdownlist ddRow5_9 "" pos:[408,185] width:90 height:15
			dropdownlist ddRow5_10 "" pos:[408,205] width:90 height:15
			
			
			
			
			label labRow6 "Row 6" pos:[528,8]
			dropdownlist ddRow6_1 "" pos:[508,25] width:90 height:15
			dropdownlist ddRow6_2 "" pos:[508,45] width:90 height:15
			dropdownlist ddRow6_3 "" pos:[508,65] width:90 height:15
			dropdownlist ddRow6_4 "" pos:[508,85] width:90 height:15
			dropdownlist ddRow6_5 "" pos:[508,105] width:90 height:15
			dropdownlist ddRow6_6 "" pos:[508,125] width:90 height:15
			dropdownlist ddRow6_7 "" pos:[508,145] width:90 height:15
			dropdownlist ddRow6_8 "" pos:[508,165] width:90 height:15
			dropdownlist ddRow6_9 "" pos:[508,185] width:90 height:15
			dropdownlist ddRow6_10 "" pos:[508,205] width:90 height:15
			
			
			
			
			label labRow7 "Row 7" pos:[628,8]
			dropdownlist ddRow7_1 "" pos:[608,25] width:90 height:15
			dropdownlist ddRow7_2 "" pos:[608,45] width:90 height:15
			dropdownlist ddRow7_3 "" pos:[608,65] width:90 height:15
			dropdownlist ddRow7_4 "" pos:[608,85] width:90 height:15
			dropdownlist ddRow7_5 "" pos:[608,105] width:90 height:15
			dropdownlist ddRow7_6 "" pos:[608,125] width:90 height:15
			dropdownlist ddRow7_7 "" pos:[608,145] width:90 height:15
			dropdownlist ddRow7_8 "" pos:[608,165] width:90 height:15
			dropdownlist ddRow7_9 "" pos:[608,185] width:90 height:15
			dropdownlist ddRow7_10 "" pos:[608,205] width:90 height:15
			
			
			
			
			label labRow8 "Row 8" pos:[728,8]
			dropdownlist ddRow8_1 "" pos:[708,25] width:90 height:15
			dropdownlist ddRow8_2 "" pos:[708,45] width:90 height:15
			dropdownlist ddRow8_3 "" pos:[708,65] width:90 height:15
			dropdownlist ddRow8_4 "" pos:[708,85] width:90 height:15
			dropdownlist ddRow8_5 "" pos:[708,105] width:90 height:15
			dropdownlist ddRow8_6 "" pos:[708,125] width:90 height:15
			dropdownlist ddRow8_7 "" pos:[708,145] width:90 height:15
			dropdownlist ddRow8_8 "" pos:[708,165] width:90 height:15
			dropdownlist ddRow8_9 "" pos:[708,185] width:90 height:15
			dropdownlist ddRow8_10 "" pos:[708,205] width:90 height:15
			
			
			
			
			label labRow9 "Row 9" pos:[828,8]
			dropdownlist ddRow9_1 "" pos:[808,25] width:90 height:15
			dropdownlist ddRow9_2 "" pos:[808,45] width:90 height:15
			dropdownlist ddRow9_3 "" pos:[808,65] width:90 height:15
			dropdownlist ddRow9_4 "" pos:[808,85] width:90 height:15
			dropdownlist ddRow9_5 "" pos:[808,105] width:90 height:15
			dropdownlist ddRow9_6 "" pos:[808,125] width:90 height:15
			dropdownlist ddRow9_7 "" pos:[808,145] width:90 height:15
			dropdownlist ddRow9_8 "" pos:[808,165] width:90 height:15
			dropdownlist ddRow9_9 "" pos:[808,185] width:90 height:15
			dropdownlist ddRow9_10 "" pos:[808,205] width:90 height:15
			
			
			
			
			label labRow10 "Row 10" pos:[928,8]
			dropdownlist ddRow10_1 "" pos:[908,25] width:90 height:15
			dropdownlist ddRow10_2 "" pos:[908,45] width:90 height:15
			dropdownlist ddRow10_3 "" pos:[908,65] width:90 height:15
			dropdownlist ddRow10_4 "" pos:[908,85] width:90 height:15
			dropdownlist ddRow10_5 "" pos:[908,105] width:90 height:15
			dropdownlist ddRow10_6 "" pos:[908,125] width:90 height:15
			dropdownlist ddRow10_7 "" pos:[908,145] width:90 height:15
			dropdownlist ddRow10_8 "" pos:[908,165] width:90 height:15
			dropdownlist ddRow10_9 "" pos:[908,185] width:90 height:15
			dropdownlist ddRow10_10 "" pos:[908,205] width:90 height:15
			
			
			
			button btnSave "Save" pos:[408,235] width:90 height:20
			button btnCancel "Cancel" pos:[508,235] width:90 height:20
			
			
			
			
			
			
			function updateDialog init:undefined =
			(
				local rolloutList = #("---------------------------", "Target", "Operation", "Pivot", "Models", "Muscles", "Locator", "Load / Save / Export", "Pose Manager", "Perforce", "Settings")
				
				if init != undefined then
				(
					for i = 1 to 10 do
					(
						for f = 1 to 10 do
						(
							local tempDD = execute ("cryTools.cryAnim.UI.main.settings.customizeRollouts.ddRow" + i as String + "_" + f as String)
							tempDD.items = rolloutList
						)
					)
				)
			)
			
			
			
			on customizeRolloutsRO open do
			(
				updateDialog init:true
			)
			
			
			
			on btnCancel pressed do
			(
				try destroyDialog cryTools.cryAnim.UI.main.settings.customizeRollouts catch()
			)
			
			
			
		)
		
		
		cryTools.cryAnim.UI.main.settings.customizeRollouts = customizeRolloutsRO
		customizeRolloutsRO = undefined
		
		createDialog cryTools.cryAnim.UI.main.settings.customizeRollouts 1000 260
		
	)
	catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.settings._f.callCustomizeDialog" )
)
callCustomizeDialog = undefined
logOutput "> Created cryTools.cryAnim.UI.main.settings._f.callCustomizeDialog function"






logOutput ">> functions.ms loaded"