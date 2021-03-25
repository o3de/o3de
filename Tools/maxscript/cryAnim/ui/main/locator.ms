--###############################################################################
	--// rollout with elements to control the locator
--###############################################################################
rollout locatorRO "Locator"
(
	button btnCreate "Create" pos:[8,8] width:70 height:20 toolTip:"Creates Locator_Locomotion biped prop"
	button btnDelete "Delete" pos:[80,8] width:70 height:20 toolTip:"Deletes Locator_Locomotion"
	button btnAutoLoc "Auto-Locator" pos:[8,35] width:142 height:20 toolTip:"Animates the Locator_Locomotion contextual of the animation"
	button btnResetLocator "Reset Locator" pos:[8,55] width:142 height:20 toolTip:"Resets Locator_Locomotion to the origin facing in the direction the character faces"
	button btnSetToBodyMass "Move to Body Mass" pos:[8,75] width:142 height:20 toolTip:"Moves the Locator_Locomotion to the calculated body mass of Bip01"

	
	
	on locatorRO open do
	(
		try
		(
			if (cryTools.cryAnim.base.iniFile #get #rolloutStates) == true then
				(cryTools.cryAnim.UI.main._f.getUI "Locator" "").open = cryTools.cryAnim.base.iniFile #get #locatorRO
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.locatorRO.open" )
	)
	
	
	on locatorRO rolledUp value do
	(
		try
		(
			if (cryTools.cryAnim.base.iniFile #get #locatorRO) != value then
				cryTools.cryAnim.base.iniFile #set #locatorRO
			
			local lolStream = createFile "C:\\Yeah.txt"
			
			cryTools.cryAnim.UI.main._f.updateDialog()
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.locatorRO.rolledUp" )
	)
	
	
	
	
	on btnCreate pressed do
	(
		try
		(
			if $Bip01 != undefined then
			(
				if queryBox "Create Locator_Locomotion?" title:"Locator_Locomotion" == true then
				(
					undo "create Locator_Locomotion" on
					(
						try
						(
							local locVertArray = #([-2.60474,5.627,0], [-2.60474,5.627,5.306], [-2.60501,-5.627,5.306], [-2.60501,-5.627,2.40557e-006], [2.60475,5.627,0], [2.60475,5.627,5.306], [2.60499,-5.627,5.306], [2.605,-5.627,1.88326e-007], [-5.71044,-5.62699,5.306], [-5.71044,-5.62699,3.72612e-006], [5.71043,-5.62701,-1.24204e-006], [5.71043,-5.62701,5.306], [-9.4768e-006,-11.63,-1.24204e-006], [-9.48043e-006,-11.63,5.306])
							$Bip01.controller.figureMode = true
							$Bip01.controller.prop1Exists = false
							
							tempSaveRot = $Bip01.transform.rotation
							tempBipRot = $Bip01.transform.rotation as eulerangles
							if tempBipRot.z > 0 and tempBipRot.z < 180 then
								tempBipRot.z = -90
							
							biped.setTransform $Bip01 #rotation tempBipRot false
							
							$Bip01.controller.prop1Exists = true
							
							tempSel = biped.getNode $Bip01 20
							tempSel.name = "Locator_Locomotion"
							select tempSel
							
							cryTools.cryAnim._f.resetLocator forceDir:true
							
							tempPanel = getCommandPanelTaskMode()
							setCommandPanelTaskMode #modify
							
							addModifier tempSel (Edit_Poly())
							
							
							
							
							tempBit = #{1..(polyOp.getNumVerts tempSel)}
							tempSel.modifiers[1].setSelection 1 tempBit
							tempSel.modifiers[1].setOperation #DeleteVertex
							tempSel.modifiers[1].commit()


							for i = 1 to locVertArray.count do
							(
								tempSel.modifiers[1].CreateVertex [0,0,0]
							)
							tempSel.modifiers[1].commit()
							tempSel.modifiers[1].SetEPolySelLevel #Vertex

							for i = 1 to locVertArray.count do
							(
								tempSel.modifiers[1].SetSelection #Vertex #{}
								tempSel.modifiers[1].Select #Vertex #{i}
								tempSel.modifiers[1].Commit()
								tempSel.modifiers[1].moveSelection locVertArray[i]
								tempSel.modifiers[1].Commit()
								
							)

							tempSel.modifiers[1].CreateFace #(6,5,1)
							tempSel.modifiers[1].CreateFace #(6,1,2)
							tempSel.modifiers[1].CreateFace #(7,6,2)
							tempSel.modifiers[1].CreateFace #(2,3,7)
							tempSel.modifiers[1].CreateFace #(8,5,6)
							tempSel.modifiers[1].CreateFace #(6,7,8)
							tempSel.modifiers[1].CreateFace #(1,4,3)
							tempSel.modifiers[1].CreateFace #(3,2,1)
							tempSel.modifiers[1].CreateFace #(1, 4,5)
							tempSel.modifiers[1].CreateFace #(4,5, 8)
							tempSel.modifiers[1].CreateFace #(11,8,7)
							tempSel.modifiers[1].CreateFace #(7,12,11)
							tempSel.modifiers[1].CreateFace #(3,4, 10)
							tempSel.modifiers[1].CreateFace #(3, 9,10)
							tempSel.modifiers[1].CreateFace #(9,14,12)
							tempSel.modifiers[1].CreateFace #(11,12,14)
							tempSel.modifiers[1].CreateFace #(14,13,11)
							tempSel.modifiers[1].CreateFace #(10,13,14)
							tempSel.modifiers[1].CreateFace #(10,14,9)
							tempSel.modifiers[1].CreateFace #(10,13,11)

							tempSel.modifiers[1].SetSelection #Vertex #{}
							tempSel.modifiers[1].SetEPolySelLevel #Object
							
							tempSel.wireColor = (color 255 0 0)

							tempSel.modifiers[1].Commit()
							
							biped.setTransform $Bip01 #rotation tempSaveRot true
							
							$Bip01.controller.figureMode = false
							
							setCommandPanelTaskMode tempPanel
							
							cryTools.cryAnim._f.resetLocator()
							
							redrawviews()
						)catch (print "Error Creating Locator_Locomotion")
					)
				)
			)
			else
				messageBox "No Biped in Scene." title:"Error Creating Locator_Locomotion"
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.locatorRO.btnCreate.pressed" )
	)
	
	
	on btnDelete pressed do
	(
		try
		(
			if $Bip01 != undefined then
			(
				if queryBox "Delete Locator_Locomotion?" title:"Locator_Locomotion" == true then
				(
					undo "delete Locator_Locomotion" on
					(
						try
						(
							$Bip01.controller.figureMode = true
							$Bip01.controller.prop1Exists = false 
							$Bip01.controller.figureMode = false
						) catch ( print "Error Deleting Locatot_Locomtion" )
					)
				)
			)
			else
				messageBox "No Biped in Scene." title:"Error Deleting Locator_Locomotion"
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.locatorRO.btnDelete.pressed" )
	)
	
	

	on btnResetLocator pressed do
	(
		try
		(
			if $Locator_Locomotion != undefined then
				undo "Reset Locator" on
					cryTools.cryAnim._f.resetLocator()
			else print "No Locator in Scene."
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.locatorRO.btnResetLocator.pressed" )	
	)
	
	
	on btnAutoLoc pressed do
	(
		try
		(
			if $Locator_Locomotion != undefined then
				undo "Auto Locator" on
					try (	fileIn (cryTools.buildPathFull + "Tools\\maxscript\\cryAnim\\ui\\batch\\Scripts\\AutoLoc.ms") ) catch()
			else print "No Locator in Scene."
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.locatorRO.btnAutoLoc.pressed" )
	)
	
	on btnSetToBodyMass pressed do
	(
		try
		(
			if $Locator_Locomotion != undefined then
				undo "Move to BodyMass" on
					try (	cryTools.cryAnim._f.moveToBodyMass() ) catch( print "Can't execute: Move to Body Mass")
			else print "No Locator in Scene."
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.locatorRO.btnSetToBodyMass.pressed" )
	)
	
)
logOutput "> Created locatorRO rollout"

try
(
	if cryTools.cryAnim.base.iniFile #get #multiRow == true then
		addSubRollout cryTools.cryAnim.UI.main.dialog.row2 locatorRO
	else
		addSubRollout cryTools.cryAnim.UI.main.dialog.row1 locatorRO
)
catch ( logOutput "!!> Error adding locatorRO to main dialog" )
locatorRO = undefined


logOutput ">> locator.ms loaded"