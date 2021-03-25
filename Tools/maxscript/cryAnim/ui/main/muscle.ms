--###############################################################################
	--// rollout with elements to control models, weapons and other things like nanoMuscles
--###############################################################################
rollout musclesRO "Muscles"
(	
	groupBox gbMuscles " Muscles " pos:[2,2] width:153 height:62

	
	checkBox chkAutomateMuscles "Auto-Muscles" pos:[8,18] width:90 height:20 checked:true fieldWidth:0
	checkBox chkUseMusclesKeys "Use Keys" pos:[8,38] width:90 height:20 enabled:false fieldWidth:0
	button btnCreateMuscles "Create" pos:[100,16] width:50 height:20 toolTip:"Creates the nano muscles rig on Bip01"
	button btnBakeMuscles "Bake" pos:[100,36] width:50 height:20 toolTip:"Bakes down all keys of the muscle bones"
	
	on musclesRO open do
	(
		try
		(
			try ( if (cryTools.cryAnim.base.iniFile #get #rolloutStates) == true then (cryTools.cryAnim.UI.main._f.getUI "Muscles" "").open = cryTools.cryAnim.base.iniFile #get #musclesRO) catch()
			
			global automateAnimateMuscles = undefined
			global useMuscleKeys = undefined


			nanoConPosArray = #("_Bip01 L rear deltoid01_Con", "_Bip01 L rear deltoid02_Con", "_Bip01 L clavicular deltoid01_Con", "_Bip01 R rear deltoid01_Con", "_Bip01 R rear deltoid02_Con", "_Bip01 R clavicular deltoid01_Con")
			nanoConRotArray = #("_Bip01 L knee_Con", "_Bip01 R knee_Con")
			
			errorOutput = undefined
			
			for i = 1 to nanoConPosArray.count do
				if (getNodeByName nanoConPosArray[i]) == undefined then
					errorOutput = true
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.musclesRO.open" )
	)
	
	on musclesRO rolledUp value do
	(
		try
		(
			if (cryTools.cryAnim.base.iniFile #get #muscles) != value then
				cryTools.cryAnim.base.iniFile #set #muscles
			
			cryTools.cryAnim.UI.main._f.updateDialog()
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.musclesRO.rolledUp" )
	)
	
	
	on chkAutomateMuscles changed value do
	(
		try
		(
			if chkAutomateMuscles.checked == true then
				global automateAnimateMuscles = undefined
			else							
				global automateAnimateMuscles = false

			chkUseMusclesKeys.enabled = not chkAutomateMuscles.checked
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.musclesRO.chkAutomateMuscles.changed" )
	)
	
	on chkUseMusclesKeys changed value do
	(
		try
		(
			if chkUseMusclesKeys.checked == true then
				global useMuscleKeys = true
			else
				global useMuscleKeys = undefined
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.musclesRO.chkUseMusclesKeys.changed" )
	)
	
	on btnBakeMuscles pressed do
	(
		try
		(
			if $'_Bip01 L clavicular deltoid01_LA' != undefined then
			(
				if (queryBox "Bake all Muscle Bones?" title:"Muscle Rig") == true then
					cryTools.cryAnim.UI.main.loadSave._f.bakeMuscleBones()
			)
			else
				messageBox "No Muscle Rig on Bip01" title:"Muscle Rig"
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.musclesRO.btnBakeMuscles.pressed" )
	)
	
	
	
	on btnCreateMuscles pressed do
	(
		try
		(
			if $Bip01 != undefined then
			(						
				if (queryBox "Create MuscleRig for Bip01?" title:"Muscle Rig") == true then
				(
										
					undo "createMuscleRig" on
					(
						$Bip01.controller.figureMode = true								
						completeRedraw()
						
						saveSelection = getCurrentSelection()
						saveSliderTime = sliderTime
						
						clearSelection()								
						
						
						with redraw off
						(
							
							Bip01_R_rear_deltoid02_LA = dummy name:"_Bip01 R rear deltoid02_LA" boxsize:[2,2,2]
							Bip01_R_clavicular_deltoid01_LA = dummy name:"_Bip01 R clavicular deltoid01_LA" boxsize:[2,2,2]
							Bip01_R_rear_deltoid01_LA = dummy name:"_Bip01 R rear deltoid01_LA" boxsize:[2,2,2]
							
							Bip01_L_rear_deltoid02_LA = dummy name:"_Bip01 L rear deltoid02_LA" boxsize:[2,2,2]
							Bip01_L_clavicular_deltoid01_LA = dummy name:"_Bip01 L clavicular deltoid01_LA" boxsize:[2,2,2]
							Bip01_L_rear_deltoid01_LA = dummy name:"_Bip01 L rear deltoid01_LA" boxsize:[2,2,2]
							
							Bip01_R_Clavicle_Con = dummy name:"_Bip01 R Clavicle_Con" pos:$'Bip01 R UpperArm'.transform.pos boxsize:[6,6,6]
							Bip01_L_Clavicle_Con = dummy name:"_Bip01 L Clavicle_Con" pos:$'Bip01 L UpperArm'.transform.pos boxsize:[6,6,6]
							
							Bip01_R_Clavicle_Con.parent = $'Bip01 R Clavicle'
							Bip01_L_Clavicle_Con.parent = $'Bip01 L Clavicle'
							
							
							Bip01_R_knee_rotDif = dummy name:"_Bip01 R knee_rotDif" rotation:(inverse($'Bip01 R Calf'.transform.rotation)) pos:$'Bip01 R Calf'.transform.pos boxsize:[3,3,3]
							Bip01_L_knee_rotDif = dummy name:"_Bip01 L knee_rotDif" rotation:$'Bip01 L Calf'.transform.rotation pos:$'Bip01 L Calf'.transform.pos boxsize:[3,3,3]
							
							Bip01_R_knee_rotDif.parent = $'Bip01 R Calf'
							Bip01_L_knee_rotDif.parent = $'Bip01 L Calf'
							
							
								
							
							Bip01_R_knee_Con = dummy name:"_Bip01 R knee_Con" pos:$'Bip01 R Thigh'.transform.pos boxsize:[3,3,3]
							Bip01_L_knee_Con = dummy name:"_Bip01 L knee_Con" pos:$'Bip01 L Thigh'.transform.pos boxsize:[3,3,3]
							
							Bip01_R_knee_Con.parent = $'Bip01 R Thigh'
							Bip01_L_knee_Con.parent = $'Bip01 L Thigh'
							
							
							
							
							Bip01_R_rear_deltoid01_Con = dummy name:"_Bip01 R rear deltoid01_Con" boxsize:[3,3,3]
							Bip01_R_rear_deltoid02_Con = dummy name:"_Bip01 R rear deltoid02_Con" boxsize:[3,3,3]
							Bip01_R_clavicular_deltoid01_Con = dummy name:"_Bip01 R clavicular deltoid01_Con" boxsize:[3,3,3]
							
							Bip01_L_rear_deltoid01_Con = dummy name:"_Bip01 L rear deltoid01_Con" boxsize:[3,3,3]
							Bip01_L_rear_deltoid02_Con = dummy name:"_Bip01 L rear deltoid02_Con" boxsize:[3,3,3]
							Bip01_L_clavicular_deltoid01_Con = dummy name:"_Bip01 L clavicular deltoid01_Con" boxsize:[3,3,3]
								
							Bip01_R_rear_deltoid01_Con.position.controller = position_script() ; Bip01_R_rear_deltoid01_Con.position.controller.script = "RRear01 = (getVert $NanoSUIt 4068) if automateAnimateMuscles == undefined then ( with animate off $'_Bip01 R rear deltoid01_LA'.pos = RRear01 ) else ( if useMuscleKeys == undefined then ( with animate on $'_Bip01 R rear deltoid01_LA'.pos = RRear01 ) else RRear01 )"
							Bip01_R_rear_deltoid02_Con.position.controller = position_script() ; Bip01_R_rear_deltoid02_Con.position.controller.script = "RRear02 = (getVert $NanoSUIt 4221) if automateAnimateMuscles == undefined then ( with animate off $'_Bip01 R rear deltoid02_LA'.pos = RRear02 ) else ( if useMuscleKeys == undefined then ( with animate on $'_Bip01 R rear deltoid02_LA'.pos = RRear02 ) else RRear02 )"
							Bip01_R_clavicular_deltoid01_Con.position.controller = position_script() ; Bip01_R_clavicular_deltoid01_Con.position.controller.script = "RFront = (getVert $NanoSUIt 4064) if automateAnimateMuscles == undefined then ( with animate off $'_Bip01 R clavicular deltoid01_LA'.pos = RFront ) else ( if useMuscleKeys == undefined then ( with animate on $'_Bip01 R clavicular deltoid01_LA'.pos = RFront ) else RFront )"
								
							Bip01_L_rear_deltoid01_Con.position.controller = position_script() ; Bip01_L_rear_deltoid01_Con.position.controller.script = "LRear01 = (getVert $NanoSUIt 3809) if automateAnimateMuscles == undefined then ( with animate off $'_Bip01 L rear deltoid01_LA'.pos = LRear01 ) else ( if useMuscleKeys == undefined then ( with animate on $'_Bip01 L rear deltoid01_LA'.pos = LRear01 ) else LRear01 )"
							Bip01_L_rear_deltoid02_Con.position.controller = position_script() ; Bip01_L_rear_deltoid02_Con.position.controller.script = "LRear02 = (getVert $NanoSUIt 3962) if automateAnimateMuscles == undefined then ( with animate off $'_Bip01 L rear deltoid02_LA'.pos = LRear02 ) else ( if useMuscleKeys == undefined then ( with animate on $'_Bip01 L rear deltoid02_LA'.pos = LRear02 ) else LRear02 )"
							Bip01_L_clavicular_deltoid01_Con.position.controller = position_script() ; Bip01_L_clavicular_deltoid01_Con.position.controller.script = "LFront = (getVert $NanoSUIt 3805) if automateAnimateMuscles == undefined then ( with animate off $'_Bip01 L clavicular deltoid01_LA'.pos = LFront ) else ( if useMuscleKeys == undefined then ( with animate on $'_Bip01 L clavicular deltoid01_LA'.pos = LFront ) else LFront )"
						
						
						
						
							Bip01_L_knee_Con.rotation.controller = rotation_script() ; Bip01_L_knee_Con.rotation.controller.script = "fn LKneeMuscles = ( LKneeOriginRot = (quat -0.461687 0.548192 -0.440465 -0.540667) ; LKneeOriginPos = [43.0243,-0.746225,0.291201] ; LrotDiff = (in coordsys $'Bip01 L Thigh' $'_Bip01 L knee_rotDif'.rotation) as eulerangles ; in coordsys $'Bip01 L Thigh' ( $'Bip01 L knee'.rotation = LKneeOriginRot ; $'Bip01 L knee'.pos = LKneeOriginPos ) in coordsys $'Bip01 L knee' (rotate $'Bip01 L knee' (eulerangles (LrotDiff.z / -2) 0 0)) ) if automateAnimateMuscles == undefined then ( with animate off LKneeMuscles() ) else ( if useMusclesKeys == undefined then ( with animate on LKneeMuscles() ) ) ; $'Bip01 L Thigh'.transform.rotation"
							Bip01_R_knee_Con.rotation.controller = rotation_script() ; Bip01_R_knee_Con.rotation.controller.script = "fn RKneeMuscles = ( RKneeOriginRot = (quat -0.530717 -0.393879 0.549399 -0.511233) ; RKneeOriginPos = [43.0243,-0.746225,0.291201] ; RrotDiff = (in coordsys $'Bip01 R Thigh' $'_Bip01 R knee_rotDif'.rotation) as eulerangles ; in coordsys $'Bip01 R Thigh' ( $'Bip01 R knee'.rotation = RKneeOriginRot ; $'Bip01 R knee'.pos = RKneeOriginPos ) in coordsys $'Bip01 R knee' (rotate $'Bip01 R knee' (eulerangles (RrotDiff.z / -2) 0 0)) ) if automateAnimateMuscles == undefined then ( with animate off RKneeMuscles() ) else ( if useMusclesKeys == undefined then ( with animate on RKneeMuscles() ) ) ; $'Bip01 R Thigh'.transform.rotation"
								
								
								
							Bip01_R_Clavicle_Con.isHidden = true
							Bip01_L_Clavicle_Con.isHidden = true
							Bip01_R_knee_Con.isHidden = true
							Bip01_L_knee_Con.isHidden = true
							Bip01_R_knee_rotDif.isHidden = true
							Bip01_L_knee_rotDif.isHidden = true
							
							Bip01_R_rear_deltoid01_Con.isHidden = true
							Bip01_R_rear_deltoid02_Con.isHidden = true
							Bip01_R_clavicular_deltoid01_Con.isHidden = true
							
							Bip01_L_rear_deltoid01_Con.isHidden = true
							Bip01_L_rear_deltoid02_Con.isHidden = true
							Bip01_L_clavicular_deltoid01_Con.isHidden = true
								
							
							-- LOOKAT-SETUP --
							$'Bip01 R rear deltoid01'.transform = (matrix3 [0.800769,0.401167,0.444785] [-0.104045,-0.638128,0.762868] [0.589868,-0.657158,-0.469253] [-15.3582,10.8964,154.478])
							$'Bip01 R rear deltoid02'.transform = (matrix3 [0.864961,0.479963,0.14655] [0.0248794,-0.33268,0.942711] [0.501221,-0.811763,-0.299696] [-13.2617,14.8806,147.713])
							$'Bip01 R clavicular deltoid01'.transform = (matrix3 [0.879667,-0.134998,0.456029] [0.448849,-0.0813437,-0.889898] [0.157229, 0.987502,-0.0109619] [-11.1466,-5.93142,154.513])
							
							
							$'Bip01 L rear deltoid01'.transform = (matrix3 [0.800769,-0.401167,-0.444785] [-0.104045,0.638128,-0.762868] [0.589868,0.657159,0.469254] [15.3582,10.8963,154.478])
							$'Bip01 L rear deltoid02'.transform = (matrix3 [0.864962,-0.479963,-0.14655] [0.0248793,0.33268,-0.942712] [0.501221,0.811763,0.299697] [13.2617,14.8806,147.713])
							$'Bip01 L clavicular deltoid01'.transform = (matrix3 [0.879666,0.134998,-0.456029] [0.448849,0.0813439,0.889898] [0.157229,-0.987501,0.0109618] [11.1466,-5.93141,154.513])
							
							
							
							
							$'Bip01 R rear deltoid01'.rotation.controller = LookAt_constraint lookat_vector_length:0 upnode_world:false pickUpNode:$'_Bip01 R Clavicle_Con' relative:true
							$'Bip01 R rear deltoid02'.rotation.controller = LookAt_constraint lookat_vector_length:0 upnode_world:false pickUpNode:$'_Bip01 R Clavicle_Con' relative:true
							$'Bip01 R clavicular deltoid01'.rotation.controller = LookAt_constraint lookat_vector_length:0 upnode_world:false pickUpNode:$'_Bip01 R Clavicle_Con' relative:true
							
							$'Bip01 L rear deltoid01'.rotation.controller = LookAt_constraint lookat_vector_length:0 upnode_world:false pickUpNode:$'_Bip01 R Clavicle_Con' relative:true
							$'Bip01 L rear deltoid02'.rotation.controller = LookAt_constraint lookat_vector_length:0 upnode_world:false pickUpNode:$'_Bip01 R Clavicle_Con' relative:true
							$'Bip01 L clavicular deltoid01'.rotation.controller = LookAt_constraint lookat_vector_length:0 upnode_world:false pickUpNode:$'_Bip01 R Clavicle_Con' relative:true
							
							
							
							
							$'Bip01 R rear deltoid01'.rotation.controller.appendTarget $'_Bip01 R rear deltoid01_LA' 100
							$'Bip01 R rear deltoid02'.rotation.controller.appendTarget $'_Bip01 R rear deltoid02_LA' 100
							$'Bip01 R clavicular deltoid01'.rotation.controller.appendTarget $'_Bip01 R clavicular deltoid01_LA' 100
							
							$'Bip01 L rear deltoid01'.rotation.controller.appendTarget $'_Bip01 L rear deltoid01_LA' 100
							$'Bip01 L rear deltoid02'.rotation.controller.appendTarget $'_Bip01 L rear deltoid02_LA' 100
							$'Bip01 L clavicular deltoid01'.rotation.controller.appendTarget $'_Bip01 L clavicular deltoid01_LA' 100
						
						)
						
						completeRedraw()
						
						$Bip01.controller.figureMode = false	
						
						if sliderTime != animationRange.end then
							sliderTime += 1
						else
							sliderTime -= 1
						
						
						sliderTime = saveSliderTime
						
						for obj in saveSelection do
							selectMore obj
						
					)
						
				)						
			)
			else
			(
				messageBox "No Bip01 in Scene" title:"Error Generating MuscleRig"
			)
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.musclesRO.btnCreateMuscles.pressed" )
	)
)
logOutput "> Created muscleRO rollout"


try
(
	if cryTools.cryAnim.base.iniFile #get #multiRow == true then
		addSubRollout cryTools.cryAnim.UI.main.dialog.row2 musclesRO
	else
		addSubRollout cryTools.cryAnim.UI.main.dialog.row1 musclesRO rolledUp:true
)
catch ( logOutput "!!> Error adding musclesRO to main dialog" )
musclesRO = undefined


logOutput ">> muscle.ms loaded"