-------------------------------------------------------------------------------
--	CryRiggingTools.ms
--	Version 2.6 External
--	by Christopher Evans
-------------------------------------------------------------------------------

collapsedCache = #()
global CryMorphTools
(
	if CryMorphTools != undefined do ( closerolloutfloater CryMorphTools )
	CryMorphTools = newrolloutfloater "CryMorphTools 2.7" 190 800

--used for index2loc
Index2Loc
obj1 = ""
obj1_points = #()
collapsedCache = #()
getVariations = #()
hiddenVerts = #{}

--used by FacialTools
numMorphs = ""
exportedMorphs = #()

-------------------------------------------------------------------------------
--FacialTools
-------------------------------------------------------------------------------
	rollout FacialTools "Facial Tools"
	(
		group "Auto Generation/Extraction"
		(
			label labelMORPHS "No Head Loaded"
			checkbutton LoadMorphsBTN "Load Morphs From Selected"
			checkbox alignToDummy "Organize (Align to helpers)" offset:[13,0]
			checkbox dirty "Enable dirty output" offset:[13,-4]
			edittext layer_txt "New Layer:" fieldWidth:95
			button doit "Bake Morphs" enabled:false
			progressbar doit_prog color:red
			button compile "Add organized morphs to head" enabled:false
			pickbutton compile2 "Add selected morphs to picked"
		)
		
		group "Delete Faces From Targets"
		(
			checkbutton loadSelection "Load Polygon Selection"
			button deletePolys "Remove From Selected" enabled:false
		)
		group "Mirror Eye Animation"
		(
			button mirrorEyesLR "Mirror Left Eye to Right"
			button mirrorEyesRL "Mirror Right Eye to Left"
		)
		group "Morph Transfer"
		(
			button selectOriginal "Load Original head"
			button selectTransfer "Load Target Head"
			button bakeTransfer "Bake Out Transfer"
			checkbox addSuffix "Add obj name suffix"
			checkbox moveAside "Translate right 20 units"
		)


--FacialTools open/close
-------------------------
		on FacialTools open do
		(
			alignToDummy.checked = true
			if polysSelected != undefined then
			(
					deletepolys.enabled = true
			)
			
			local tempVar = cryTools.inFromINI "CryTools" "morph_pos"
			if tempVar == "" then tempVar = [7,7]
			else
			(
				try
				(
					tempVar = execute tempVar
					crymorphtools.pos = tempVar
				)
				catch
				(
					cryTools.outToINI "CryTools" "morph_pos" "[7,7]"
					crymorphtools.pos = [7,7]
				)
			)
		)
		
		on FacialTools close do
		(
			cryTools.outToINI "CryTools" "morph_pos" (crymorphtools.pos as String)
		)


--LoadMorphsBTN 
----------------
		on LoadMorphsBTN changed state do
		(
			if state == on then
			(	
				try
				(
					testMorph = $.morpher
				)
				catch
				(
					messageBox "Object has no Morpher modifier"
					LoadMorphsBTN.checked = false
					return undefined
				)
				for i = 1 to 100 do
				(
					test = $.morpher[i].name
					print $.morpher[i].name
					if test == "- empty -" then
						(
							numMorphs = (i - 1)
							exit loop
						)
				)
				labelMORPHS.text = ((numMorphs as string) + " Morphs Loaded From:")
				LoadMorphsBTN.text = ($.name)
				doit.enabled = true
			)
			else
			(
				LoadMorphsBTN.text = "No Head Loaded"
				doit.enabled = false
			)
		)


--Align/Dirty Checked 
----------------------
		on alignToDummy changed state do
		(
			if alignToDummy.checked == true then
			(
				dirty.checked = false
			)
			else
			(
				dirty.checked = true
			)
		)
		
		on dirty changed state do
		(
			if dirty.checked == true then
			(
				alignToDummy.checked = false
			)
			else
			(
				alignToDummy.checked = true
			)
		)


--BakeMorphs pressed 
---------------------
		on doit pressed do
		(
			if selection[1] == undefined do
			(
				messageBox "Please select a character head." title:"Error"
				return undefined
			)
			
			try
			(
				testMorph = $.morpher
			)
			catch
			(
				messageBox "Object has no Morpher modifier"
				return undefined
			)
			
			undo "make heads" on
			(
				with redraw off
				(
					max create mode 
					--make layer
					layer_name = layer_txt.text
					layer01 = LayerManager.newLayer()
					layer01.setname layer_name
					layer01.current = true
					
					channelcount = numMorphs
					exportedMorphs = #()
					for i = 1 to channelcount do
					(
						old = $
						$.morpher[i].value = 100.0
						morph_name = filterString ($.morpher[i].name) " "
						new = snapshot $ name:morph_name[2]
						append exportedMorphs new
						old.morpher[i].value = 0.0
						select old
						doit_prog.value = (i*(100/channelcount))
					)
					max modify mode 
					doit_prog.value = 0
				)
			)
			
			--arbitrary morph dump
			if dirty.checked == true then
			(
				i=1
				for obj in exportedMorphs do
				(
					move obj [(20*i),0,0]
					i += 1
				)
			)
			
			--move and color
			if alignToDummy.checked == true then
			(
				undo "organize" on
				(
					mn = 1
					for obj in exportedMorphs do
					(
						try
						(
							obj.transform = (getNodeByName ("Dummy_" + obj.name)).transform
						)
						catch
						(
							move obj [(20*mn),0,0]
							mn += 1
						)
					)
				)
			)
			compile.enabled = true
		)
		
		-- Add morphs to picked
		on compile pressed do
		(
			if selection[1] == undefined then
			(
				messageBox "Please select a character head." title:"Error"
				return undefined
			)
			m = 1
			for obj in exportedMorphs do
			(
				wm3_mc_buildfromnode $.morpher m obj
				m += 1
			)
		)
	
		on compile2 picked pick do
		(
			try
			(
				for i=1 to selection.count do
				(
					wm3_mc_buildfromnode pick.morpher i selection[i]
				)
			)
			catch()
		)
		
	--Delete Faces From Targets
		on loadSelection changed state do
		(
			if state == on do
			(
				try
				(
					try
					(
						testMorph = $.morpher
					)
					catch
					(
						messageBox "Object has no Morpher modifier"
						return undefined
					)
					if $selection.count > 0 then
					(
						if (getFaceSelection $) == undefined then 
						polysSelected = (getFaceSelection $)
						loadSelection.text = ((polysSelected.count as string) + " Polygons Selected")
						deletePolys.enabled = true
					)
				)
				catch(loadSelection.checked = false)
			)
			if state == off do
			(
				loadSelection.text = "Load Polygon Selection"
				deletePolys.enabled = false
			)
		)
		
		on deletePolys pressed do
		(
			undo "delete from targets" on
			(
				whatHeadsSelected = ($selection as array)
				for obj in whatHeadsSelected do
				(
					obj.selectedfaces = polysSelected
					max modify mode
					modPanel.setCurrentObject obj
					subObjectLevel = 4
					meshops.delete obj
					obj.selectedfaces = #()
				)
			)
		)


--Mirror Eyes pressed
---------------------
		on mirrorEyesLR pressed do
		(
			left_eye = #($Eye_lid_L_U_01, $Eye_lid_L_U_02, $Eye_lid_L_U_03, $Eye_lid_L_L_01, $Eye_lid_L_L_02, $Eye_lid_L_L_03)
			right_eye = #($Eye_lid_R_U_01, $Eye_lid_R_U_02, $Eye_lid_R_U_03, $Eye_lid_R_L_01, $Eye_lid_R_L_02, $Eye_lid_R_L_03)
			try
			(
				undo "mirror eye anim L R" on
				(
					for i=1 to right_eye.count do
					(
						deleteKeys right_eye[i].rotation.controller.keys #allkeys
						deleteKeys right_eye[i].position.controller.keys #allkeys
						deleteKeys right_eye[i].scale.controller.keys #allkeys
						right_eye[i].position.controller = copy right_eye[i].position.controller
						right_eye[i].rotation.controller = copy right_eye[i].rotation.controller
						right_eye[i].rotation.controller = tcb_rotation ()
						left_eye[i].rotation.controller = tcb_rotation ()
					)
					for i=1 to right_eye.count do
					(
						for r = 1 to left_eye[i].rotation.controller.keys.count do
						(
							with animate on
							(			
								slidertime = left_eye[i].rotation.controller.keys[r].time
								crytools.MirrorObjs right_eye[i] left_eye[i] $bip01 #y #y
							)
						)
					)
				)
			)catch()
		)
		
		on mirrorEyesRL pressed do
		(
			left_eye = #($Eye_lid_L_U_01, $Eye_lid_L_U_02, $Eye_lid_L_U_03, $Eye_lid_L_L_01, $Eye_lid_L_L_02, $Eye_lid_L_L_03)
			right_eye = #($Eye_lid_R_U_01, $Eye_lid_R_U_02, $Eye_lid_R_U_03, $Eye_lid_R_L_01, $Eye_lid_R_L_02, $Eye_lid_R_L_03)
			try
			(
				undo "mirror eye anim R L" on
				(
					for i=1 to left_eye.count do
					(
						deleteKeys left_eye[i].rotation.controller.keys #allkeys
						deleteKeys left_eye[i].position.controller.keys #allkeys
						deleteKeys left_eye[i].scale.controller.keys #allkeys
						left_eye[i].position.controller = copy left_eye[i].position.controller
						left_eye[i].rotation.controller = copy left_eye[i].rotation.controller
						left_eye[i].rotation.controller = tcb_rotation ()
						right_eye[i].rotation.controller = tcb_rotation ()
					)
					for i=1 to left_eye.count do
					(
						for r = 1 to right_eye[i].rotation.controller.keys.count do
						(
							with animate on
							(			
								slidertime = right_eye[i].rotation.controller.keys[r].time
								crytools.MirrorObjs left_eye[i] right_eye[i] $bip01 #y #y
							)
						)
					)	
				)
			)catch()
		)


--Morph Transfer
-----------------
		on selectOriginal pressed do
		(
			try
			(
				testMorph = $.morpher
			)
			catch
			(
				messageBox "Object has no Morpher modifier"
				return undefined
			)
			global originalHead = $
			selectOriginal.text = $.name
		)
		
		on selectTransfer pressed do
		(
			try
			(
				testMorph = $.morpher
			)
			catch
			(
				messageBox "Object has no Morpher modifier"
				return undefined
			)
			if $selection.count > 0 then
			(
				global transferHead = $
				selectTransfer.text = $.name
			)
			else
				messageBox "Select Node before." title:"Error"
		)
		
		on bakeTransfer pressed do
		(
			if originalHead == undefined then
			(
				messagebox "No Original Head Loaded"
				return undefined
			)
			
			if $selection.count > 0 then
			(
				moveVal = 0
				for i = 1 to 1000 do
				(
					testName = originalHead.morpher[i].name
					if testName == "- empty -" then
					(
						exit loop
					)
					originalHead.morpher[i].value = 100.0
					morph_name = filterString (originalHead.morpher[i].name) " "
					new = morph_name[2]
					if addSuffix.checked == true then
					(
						snapshot transferHead name:(new + "_" + transferHead.name) isSelected:true
					)
					else
					(
						snapshot transferHead name:new isSelected:true
					)
					if moveAside.checked == true then
					(
						moveVal += 20
						$.position.x = moveVal
					)
					originalHead.morpher[i].value = 0.0
				)	
			)
			else
				messageBox "Select Node before." title:"Error"
		)
	)
	
	
-------------------------------------------------------------------------------
--DrivenMorphs
-------------------------------------------------------------------------------
	rollout DrivenMorphs "Pose Driven Morphs"
	(
		group "Head/Neck"
		(
			pickbutton headSel "Select Character Head"
			dropdownlist headVectors items:#("Select A Pose Vector...","Look_Up","Look_Down","Look_Right", "Look_Left", "Tilt_Left", "Tilt_Right") height:25
			button createHeadshape "Create A Pose Shape"
			button addTarget "Add Pose Shape To Morpher"
			button extractTarget "Extract Relative Pose Shape "
			--button testMapping "Test Pose Space Mapping"
		)
		
		group "Fleshy Eyes (Auto-created)"
		(
			dropdownlist eyeVectors items:#("Select An Eye Direction...","Eyelid_left_upper_middle","Eyelid_left_upper_right","Eyelid_left_upper_left","Eyelid_left_lower_middle","Eyelid_left_lower_left","Eyelid_left_lower_right","Eyelid_left_middle_right","Eyelid_left_middle_left","Eyelid_right_upper_middle","Eyelid_right_upper_left","Eyelid_right _upper_right","Eyelid_right_lower_middle","Eyelid_right_lower_right","Eyelid_right_lower_left","Eyelid_right_middle_left","Eyelid_right_middle_right") height:25
			checkbox createEyeLayer ""
			edittext layerEye_txt "+Layer:" fieldWidth:85 enabled:false offset:[18,-20]
			button bakeEyeShapes "Bake Eye Shapes" --enabled:false
			button addEyeTarget "Add Current Shape To Morpher" enabled:false
			button extractEyeTarget "Extract Relative Shape " --enabled:false
		)
		
		group "Shoulders"
		(
			dropdownlist armVectors items:#("Select A Pose Vector...","Left Arm Up","Left Arm Down","Left Arm Forward","Left Arm Back","Right Arm Up","Right Arm Down","Right Arm Forward","Right Arm Back") height:25  enabled:false
			button createShoulderShape "Auto-Create All Shapes" enabled:false
			button addShoulderTarget "Add Pose Shape To Morpher" enabled:false
			button extractShoulderTarget "Extract Relative Pose Shape " enabled:false
		)
		
		on headSel picked obj do
		(
			global objItem = obj
			headSel.text = (obj.name + " Head Loaded")
		)
		
		on headVectors selected i do
		(
			if i == 1 then
				(
					createHeadshape.text = "Create A Pose Shape"
					createHeadshape.enabled = false
					addTarget.enabled = false
					extractTarget.enabled = false
				)
			else
			(
				if i == 2 then
				(
					if $Look_Up_sculpt != undefined then
					(
						createHeadshape.enabled = false
						createHeadshape.text = "Look_Up Pose Shape Exists"
						addTarget.enabled = true
						extractTarget.enabled = true
						return undefined
					)
				)
				if i == 3 then
				(
					if $Look_Down_sculpt != undefined then
					(
						createHeadshape.enabled = false
						addTarget.enabled = true
						extractTarget.enabled = true
						createHeadshape.text = "Look_Down Pose Shape Exists"
						return undefined
					)
				)
				if i == 4 then
				(
					if $Look_Right_sculpt != undefined then
					(
						createHeadshape.enabled = false
						addTarget.enabled = true
						extractTarget.enabled = true
						createHeadshape.text = "Look_Down Pose Shape Exists"
						return undefined
					)
				)
				if i == 5 then
				(
					if $Look_Left_sculpt != undefined then
					(
						createHeadshape.enabled = false
						addTarget.enabled = true
						extractTarget.enabled = true
						createHeadshape.text = "Look_Down Pose Shape Exists"
						return undefined
					)
				)
				if i == 6 then
				(
					if $Tilt_Left_sculpt != undefined then
					(
						createHeadshape.enabled = false
						addTarget.enabled = true
						extractTarget.enabled = true
						createHeadshape.text = "Tilt_Left Pose Shape Exists"
						return undefined
					)
				)
				if i == 7 then
				(
					if $Tilt_Right_sculpt != undefined then
					(
						createHeadshape.enabled = false
						addTarget.enabled = true
						extractTarget.enabled = true
						createHeadshape.text = "Tilt_Right Pose Shape Exists"
						return undefined
					)
				)
			addTarget.enabled = false
			extractTarget.enabled = false
			createHeadshape.enabled = true
			createHeadshape.text = "Create Pose Shape"
			)
		)
		
		on DrivenMorphs open do
			(
				createHeadshape.enabled = false
				addTarget.enabled = false
				extractTarget.enabled = false
				if $Eye_lid_L_L_02 == undefined then
				(
					bakeEyeShapes.enabled = false
					addEyeTarget.enabled = false
					extractEyeTarget.enabled = false
				)
			)
		
		on createHeadShape pressed do
		undo "create_snapshot" on
		(
			(
				if objItem == undefined then
					(
						messageBox "Please load a character head." title:"Error"
						return undefined
					)
				
				newSnap = headVectors.selected
				
				if slidertime != 0 then
				(
					slidertime = 0
				)
				
				case headVectors.selection of
				(
					2: in coordsys parent rotate $'Bip01 Head' (angleaxis -40 [0,0,1])
					3: in coordsys parent rotate $'Bip01 Head' (angleaxis 40 [0,0,1])
					4: in coordsys parent rotate $'Bip01 Head' (angleaxis -65 [1,0,0])
					5: in coordsys parent rotate $'Bip01 Head' (angleaxis 65 [1,0,0])
					6: in coordsys parent rotate $'Bip01 Head' (angleaxis -30 [0,1,0])
					7: in coordsys parent rotate $'Bip01 Head' (angleaxis 30 [0,1,0])
				)
					
				print ("creating " + newSnap)
				snapshot objItem name: (newSnap + "_sculpt") isSelected:true
				convertTo $ PolyMeshObject
				
				createHeadshape.text = (newSnap + " Pose Shape Exists")
				createHeadshape.enabled = false
				
				case headVectors.selection of
				(
					2: move $ [0,0,60]
					3: move $ [0,0,30]
					4: move $ [-30,0,0]
					5: move $ [30,0,0]
					6: move $ [30,0,30]
					7: move $ [-30,0,30]
				)
				
				case headVectors.selection of
				(
					2: in coordsys parent rotate $'Bip01 Head' (angleaxis 40 [0,0,1])
					3: in coordsys parent rotate $'Bip01 Head' (angleaxis -40 [0,0,1])
					4: in coordsys parent rotate $'Bip01 Head' (angleaxis 65 [1,0,0])
					5: in coordsys parent rotate $'Bip01 Head' (angleaxis -65 [1,0,0])
					6: in coordsys parent rotate $'Bip01 Head' (angleaxis 30 [0,1,0])
					7: in coordsys parent rotate $'Bip01 Head' (angleaxis -30 [0,1,0])
				)
				select objItem
				addTarget.enabled = true
				extractTarget.enabled = true
			)
		)
		
		on addTarget pressed do
		undo "add_target" on
		(
			for i = 1 to 1000 do
			(
				test = objItem.morpher[i].name
				if test == "- empty -" then
				(
					global numMorphs = (i - 1)
					exit loop
				)
			)
			
			select objItem
			max modify mode
			
			case headVectors.selection of
			(
				2: in coordsys parent rotate $'Bip01 Head' (angleaxis -40 [0,0,1])
				3: in coordsys parent rotate $'Bip01 Head' (angleaxis 40 [0,0,1])
				4: in coordsys parent rotate $'Bip01 Head' (angleaxis -65 [1,0,0])
				5: in coordsys parent rotate $'Bip01 Head' (angleaxis 65 [1,0,0])
				6: in coordsys parent rotate $'Bip01 Head' (angleaxis -30 [0,1,0])
				7: in coordsys parent rotate $'Bip01 Head' (angleaxis 30 [0,1,0])
			)
			
			case headVectors.selection of
			(
				2: crytools.CreateBindPoseMorph $Look_Up_sculpt objItem 1 1 1 "Look_Up"
				3: crytools.CreateBindPoseMorph $Look_Down_sculpt objItem 1 1 2 "Look_Down"
				4: crytools.CreateBindPoseMorph $Look_Right_sculpt objItem 1 1 3 "Look_Right"
				5: crytools.CreateBindPoseMorph $Look_Left_sculpt objItem 1 1 4 "Look_Left"
				6: crytools.CreateBindPoseMorph $Tilt_Left_sculpt objItem 1 1 5 "Tilt_Left"
				7: crytools.CreateBindPoseMorph $Tilt_Right_sculpt objItem 1 1 6 "Tilt_Right"
			)
			
			case headVectors.selection of
			(
				2: in coordsys parent rotate $'Bip01 Head' (angleaxis 40 [0,0,1])
				3: in coordsys parent rotate $'Bip01 Head' (angleaxis -40 [0,0,1])
				4: in coordsys parent rotate $'Bip01 Head' (angleaxis 65 [1,0,0])
				5: in coordsys parent rotate $'Bip01 Head' (angleaxis -65 [1,0,0])
				6: in coordsys parent rotate $'Bip01 Head' (angleaxis 30 [0,1,0])
				7: in coordsys parent rotate $'Bip01 Head' (angleaxis -30 [0,1,0])
			)
		)
		
		on extractTarget pressed do
		(
			undo "extract_target" on
			(
				select objItem
				max modify mode
				modPanel.setCurrentObject $.modifiers[#Skin]
				
				case headVectors.selection of
				(
					2: in coordsys parent rotate $'Bip01 Head' (angleaxis -40 [0,0,1])
					3: in coordsys parent rotate $'Bip01 Head' (angleaxis 40 [0,0,1])
					4: in coordsys parent rotate $'Bip01 Head' (angleaxis -65 [1,0,0])
					5: in coordsys parent rotate $'Bip01 Head' (angleaxis 65 [1,0,0])
					6: in coordsys parent rotate $'Bip01 Head' (angleaxis -30 [0,1,0])
					7: in coordsys parent rotate $'Bip01 Head' (angleaxis 30 [0,1,0])
				)
				
				case headVectors.selection of
				(
					2: crytools.CreateBindPoseMorph $Look_Up_sculpt objItem 0 0 0 "Look_Up"
					3: crytools.CreateBindPoseMorph $Look_Down_sculpt objItem 0 0 0 "Look_Down"
					4: crytools.CreateBindPoseMorph $Look_Right_sculpt objItem 0 0 0 "Look_Right"
					5: crytools.CreateBindPoseMorph $Look_Left_sculpt objItem 0 0 0 "Look_Left"
					6: crytools.CreateBindPoseMorph $Tilt_Left_sculpt objItem 0 0 0 "Tilt_Left"
					7: crytools.CreateBindPoseMorph $Tilt_Right_sculpt objItem 0 0 0 "Tilt_Right"
				)
				case headVectors.selection of
				(
					2: in coordsys parent rotate $'Bip01 Head' (angleaxis 40 [0,0,1])
					3: in coordsys parent rotate $'Bip01 Head' (angleaxis -40 [0,0,1])
					4: in coordsys parent rotate $'Bip01 Head' (angleaxis 65 [1,0,0])
					5: in coordsys parent rotate $'Bip01 Head' (angleaxis -65 [1,0,0])
					6: in coordsys parent rotate $'Bip01 Head' (angleaxis 30 [0,1,0])
					7: in coordsys parent rotate $'Bip01 Head' (angleaxis -30 [0,1,0])
				)	
			)
		)
		
		on eyeVectors selected i do
		(
			case i of
			(
				2: sliderTime = 1f
				3: sliderTime = 2f
				4: sliderTime = 3f
				5: sliderTime = 4f
				6: sliderTime = 5f
				7: sliderTime = 6f
				8: sliderTime = 7f
				9: sliderTime = 8f
				10: sliderTime = 11f
				11: sliderTime = 12f
				12: sliderTime = 13f
				13: sliderTime = 14f
				14: sliderTime = 15f
				15: sliderTime = 16f
				16: sliderTime = 17f
				17: sliderTime = 18f
				18: sliderTime = 19f
			)
		)
		
		on bakeEyeShapes  pressed do
		(
			undo "bake out eye morphs" on
			(
				if $ == undefined then
				(
					messagebox "Please select a head with the proper bone rig."
					return undefined
				)
				if createEyeLayer.checked == true then
				(
					layer_name = layerEye_txt.text
					layer01 = LayerManager.newLayer()
					layer01.setname layer_name
					layer01.current = true
				)
				sliderTime = 1f
				snapshot $ name:eyeVectors.items[2] --isSelected:true
				sliderTime = 2f
				snapshot $ name:eyeVectors.items[3]
				sliderTime = 3f
				snapshot $ name:eyeVectors.items[4]
				sliderTime = 4f
				snapshot $ name:eyeVectors.items[5]
				sliderTime = 5f
				snapshot $ name:eyeVectors.items[6]
				sliderTime = 6f
				snapshot $ name:eyeVectors.items[7]
				sliderTime = 7f
				snapshot $ name:eyeVectors.items[8]
				sliderTime = 8f
				snapshot $ name:eyeVectors.items[9]
				sliderTime = 11f
				snapshot $ name:eyeVectors.items[10]
				sliderTime = 12f
				snapshot $ name:eyeVectors.items[11]
				sliderTime = 13f
				snapshot $ name:eyeVectors.items[12]
				sliderTime = 14f
				snapshot $ name:eyeVectors.items[13]
				sliderTime = 15f
				snapshot $ name:eyeVectors.items[14]
				sliderTime = 16f
				snapshot $ name:eyeVectors.items[15]
				sliderTime = 17f
				snapshot $ name:eyeVectors.items[16]
				sliderTime = 18f
				snapshot $ name:eyeVectors.items[17]
				sliderTime = 19f
				slidertime = 0f
			)	
		)
		
		on extractEyeTarget pressed do
		(
			if $ == undefined then
			(
				messagebox "Please select a head with the proper bone rig."
				return undefined
			)
			
			undo "create eye target" on
			(
				if createEyeLayer.checked == true then
				(
					layer_name = layerEye_txt.text
					layer01 = LayerManager.newLayer()
					layer01.setname layer_name
					layer01.current = true
				)
				if eyeVectors.selection == 1 then
				(
					messagebox "Please select an eye vector."
					return undefined
				)
				snapshot $ name:eyevectors.items[eyeVectors.selection]
			)
		)
		
		on createEyeLayer changed state do
		(
			if createEyeLayer.checked == true then
			(
				layerEye_txt.enabled = true
				layerEye_txt.text = "eyeMorphs"
			)
			else
			(
				layerEye_txt.enabled = false
			)
		)
	)


-------------------------------------------------------------------------------
--DiagnosticTools
-------------------------------------------------------------------------------
	rollout GeneralTools "Diagnostic Tools"
	(
		button openManager "Open morphManager"
		button morphList "Generate morphList"
		checkbox addNum "Include channel numbers" align:#center
	
		on openManager pressed do
		(
			if selection[1] == undefined do
			(
				messageBox "Please select a character head." title:"Error"
				return undefined
			)
			
			if $.modifiers[#morpher] == undefined then
			(
				messagebox "No morph modifier on selected object." title:"Error"
				return undefined
			)
		
			global mf_float, mf_morpher_mod
			(
				mf_morpher_mod = $.modifiers[#morpher]
				used_channels = #()
				txt ="rollout mf_main \"morphManager\" (\n"
				for i = 1 to 65 do
				(
					if WM3_MC_HasData mf_morpher_mod i then
					(
						append used_channels i
						txt +="progressbar mf_slider_"+ i as string
						txt +=" value:"+ (WM3_MC_GetValue mf_morpher_mod i) as string
						txt +=" width:150 height:18 across:4 align:#left\n"
						txt +="edittext mf_label_"+i as string
						txt +=" align:#right text:\""+i as string+": "
						txt +=(WM3_MC_GetName mf_morpher_mod i) +"\"\n"
						txt +="on mf_slider_"+i as string+" clicked val do (\n"
						txt +="WM3_MC_SetValue mf_morpher_mod "
						txt += i as string+" (val as float) \n"
						txt +="SliderTime +=0)\n"
					)
				)--end i loop
				txt +=")\n"
				createDialog (execute txt) 680 800 bgcolor:black fgcolor:white
				txt ="fn mf_update_slider = (\n"
				
				for i in used_channels do
					(
						txt +="mf_main.mf_slider_"+i as string
						txt +=".value = WM3_MC_GetValue mf_morpher_mod "+i as string+" \n"
					)--end i loop
				txt +=")\n"
				global mf_update_slider = execute txt
				registertimecallback mf_update_slider
				deleteAllChangeHandlers id:#morpher_floater
				when parameters mf_morpher_mod changes \
				HandleAt:#RedrawViews \
				id:#morpher_floater do mf_update_slider()
			)
		)
			
		on morphList pressed do
		(
			try
			(
				if $.modifiers[#morpher] == undefined then
				(
					messagebox "No morph modifier on selected object." title:"Error"
					return undefined
				)
				if $selection.count > 0 then
				(
					for i = 1 to 1000 do
					(
						mname = $.morpher[i].name
						mnameStripped = (filterstring mname " ")
						if addNum.checked == true then
						(
							print (mnameStripped[1] + " - " + mnameStripped[2])
						)
						else
						(
							print mnameStripped[2]
						)
						if mname == "- empty -" then
						(
							global numMorphs = (i - 1)
							exit loop
						)
					)
				)
				else
					messageBox "Select Node before." title:"Error"
			)
			catch()
		)
	)


-------------------------------------------------------------------------------
--BakingTools
-------------------------------------------------------------------------------
	rollout bakeTools "Baking Tools"
	(
		group "Bake Deformation to Morphs"
		(
			button loadBakeObj "Load Selected Object"
			edittext bakeObj_txt "baseName:" fieldWidth:85 enabled:false
			spinner bakeStart "Start " range:[-10000,10000,0] type:#integer scale:1 fieldWidth:40 offset:[-80,0] enabled:false
			spinner bakeEnd "End "  range:[-10000,10000,100] type:#integer scale:1 fieldWidth:40 offset:[0,-20] enabled:false
			spinner bakeStep "Number of Morphs " range:[-10000,10000,0] type:#integer scale:1 fieldWidth:40 enabled:false
			button bakeSequence "-EMPTY-" enabled:false
		)
		
		on loadBakeObj pressed do
		(
			if $ == undefined then
			(
				messagebox "please select a deforming object"
				return undefined
			)
			global bakeObj = $
			loadBakeObj.text = (bakeObj.name + " Loaded")
			bakeObj_txt.text = (bakeObj.name + "_#")
			bakeObj_txt.enabled = true
			bakeStart.enabled = true
			bakeEnd.enabled = true
			bakeStart.value = animationrange.start
			bakeEnd.value = animationrange.end
			global bakeName = filterstring bakeObj_txt.text "#"
			bakeSequence.text = "Bake " + (bakeObj.name + "_#")
			bakeStep.enabled = true
			bakeStep.value = 10
			bakesequence.enabled = true
		)
		
		on bakeSequence pressed do
		(
			numFrames = (bakeEnd.value - bakeStart.value)
			bakeInterval = (numframes / bakestep.value)
			print bakeInterval
			t1 = 0
			nameIter = 1
			while t1 < bakeEnd.value do
			(
				slidertime = t1
				snapshot bakeObj name:(bakeName[1] + (nameIter as string))
				nameIter += 1
				t1 += bakeInterval
			)
		)
	)


-------------------------------------------------------------------------------
--Sync-Collapse by Location (Index2Loc)
-------------------------------------------------------------------------------
	fn getVertByLoc node loc =
	(
		for i = 1 to (polyOp.getNumVerts node) do
		(
			if (polyOp.getVert node i) == loc then
			(
				return i
			)
		)
	)
	
	fn collapseVerts =
	(
		try
		(
			undo "collapse by loc" on
			(
				obj1 = (cryTools.retrieveVar "obj1")
				selectedVerts = (getVertSelection obj1 as array)
				
				if selectedVerts.count != 2 then
				(
					messagebox "Please select two verts to collapse."
					return undefined
				)
				
				selectedVertsWorldLoc = #()
				for i = 1 to selectedVerts.count do
				(
					append selectedVertsWorldLoc (polyOp.getVert obj1 selectedVerts[i])
				)
				
				polyOp.collapseVerts obj1 (selectedVerts as bitarray)
				update obj1
				
				if (cryTools.retrieveVar "hideCollapsedPass")  == true then
				(
					hiddenVerts += getVertSelection obj1
					polyOp.setHiddenVerts obj1 hiddenVerts
				)
				
				append collapsedCache #((finditem obj1_points selectedVertsWorldLoc[1]),(finditem obj1_points selectedVertsWorldLoc[2]))
				print ((finditem obj1_points selectedVertsWorldLoc[1]) as string + " " + (finditem obj1_points selectedVertsWorldLoc[2]) as string + " collapsed")
				
				if (cryTools.retrieveVar "syncVariationsPass") == true then
				(
					for obj in getVariations do
					(
						execme = ("objPointData = " + obj.name + "_points")
						execute execme
						main_point1 = (findItem obj1_points selectedVertsWorldLoc[1])
						main_point2 = (findItem obj1_points selectedVertsWorldLoc[2])
						pointsInVar = #(main_point1, main_point2) as bitarray
						print pointsinvar
						polyOp.collapseVerts obj pointsInVar
						update obj
					)
				)
			)
		)
		catch(format "*** % ***\n" (getCurrentException()))
	)
	cryTools.storeFn collapseVerts true
	
	rollout LocCollapse "Sync-Collapse by Location"
	(
		group "Main Object"
		(
			checkbutton object1 "Select Main Object"
			label numVertsUpperLbl "0000 Vertices"
		)
		group "Apply Collapse to Variations"
		(
			checkbutton object2 "Select Variations"
			label variationNumLbl "Zero Variations Loaded"
		)
		group "Collapse Verts"
		(
			button collapseVerts "Collapse Verts"
			checkbox hideCollapsed "Hide Collapsed"
			button unhideAll "Unhide All" offset:[52,-22]
			checkbox syncVariations "Collapse Variations as I Work" enabled:false
		)
		group "Load/Save Data"
		(
			button saveCollapse "Save Collapse Data"
			label dataReadOut "No Data Loaded."
			button loadCollapse "Load Collapse Data"offset:[-26,0]
			button applyCollapse "Apply" enabled:false offset:[57,-26]
			checkbox markCollapsed "Mark collapsed verts in red"
		)
		Group "Debug Tools"
		(
			edittext indexNum width:35 offset:[-5,0]
			button getIndexNum "Select by Point Index" offset:[20,-23]
			button markVariations "Mark collapsedCache Verts"
			label numVertsLbl "000" offset:[-50,5]
			button numVertsBtn "Number of Verts" offset:[20,-22]
		)
		
		on LocCollapse open do
		(
			object2.enabled = false
			collapseVerts.enabled = false
		)
		
--Object1 Button
----------------
		on object1 changed state do
		(
			if object1.checked == true then
			(
				if $ == undefined then
				(
					messagebox "Select an object."
					return undefined
				)
		
				obj1 = $
				cryTools.storeVar $ "obj1" true
				object1.text = obj1.name
				numVertsUpperLbl.text = (obj1.mesh.verts.count as string + " vertices.")
				
				for i = 1 to (polyOp.getNumVerts obj1) do
				(
					vertLoc = polyOp.getVert obj1 i
					append obj1_points vertLoc
					print (vertLoc as string + " cached")
				)
				print "All vertices cached"
				object2.enabled = true
				collapseVerts.enabled = true
				syncVariations.enabled = true
			)
			else
			(
				object1.text = "Select Object 1"
				obj1 = undefined
				object2.enabled = false
				collapseVerts.enabled = false
				syncVariations.enabled = false
				numVertsUpperLbl.text = "0000 Vertices"
			)
		)
		
--Object2 Button
----------------
		on object2 changed state do
		(			
			if object2.checked == true then
			(
				if $ == undefined then
				(
					messagebox "Select an object."
					return undefined
				)
				
				for i = 1 to selection.count do
				(	
					if selection[i] == obj1 then
					(
						messagebox "The original object should not be in the current selection.."
						object2.checked = false
						return undefined
					)
					--ADD: check for same vert count
					
					tempPoints = #()
					
					append getVariations selection[i]
					
					for n = 1 to (polyOp.getNumVerts selection[i]) do
					(
						vertLoc = polyOp.getVert selection[i] n
						append tempPoints vertLoc
					)
					
					execme = ("global " + selection[i].name + "_points = tempPoints")
					execute execme
					
					object2.text = "Variations Loaded"
					variationNumLbl.text = (selection.count as string + " variations loaded.")
				)
			)
			else
			(
				object2.text = "Select Variations"
				getVariations = #()
			)
		)
		
--Collapse button
-----------------
		on collapseVerts pressed do
		(
			undo "collapseVert" on
			(
				cryTools.storeVar hideCollapsed.checked "hideCollapsedPass" true
				cryTools.storeVar syncVariations.checked "syncVariationsPass" true
				(crytools.retrieveFn "collapseVerts()")()
				--crytools.fnstore[(crytools.storedFn #collapseVerts)]()
			)
		)
		
		on unhideAll pressed do
		(
			try (	polyOp.unHideAllVerts obj1 ) catch()
		)
		
--Save Collapse Data
---------------------
		on saveCollapse pressed do
		(
			fileSaveLoc = getSaveFileName types:"Index2Loc(*.i2l)|*.i2l" initialDir:"c:\\"
			if fileSaveLoc == undefined do (return undefined)
			outputFile = createFile fileSaveLoc
			outputFile = openFile fileSaveLoc mode:"w"
			
			format ("obj1 = " + obj1.name + "+" + ((filterstring numVertsUpperLbl.text " ")[1]) as string +"\n") to: outputFile
			for i = 1 to obj1_points.count do
			(
				format (("obj1_" + obj1_points[i] as string) + "\n") to: outputFile
			)
			format "collapsed verts\n" to: outputFile
			for i = 1 to collapsedCache.count do
			(
				format ((collapsedCache[i] as string) + "\n") to: outputFile
			)	
			close outputFile
			
			if doesfileexist fileSaveLoc == true then
			(
				dataReadOut.text = ("File saved successfully.")
			)
			else
			(
				dataReadOut.text = ("File was not saved?")
			)
		)

--Load Collapse Data
---------------------
		on loadCollapse pressed do
		(
			fileInLoc = getOpenFileName getSaveFileName types:"Index2Loc(*.i2l)|*.i2l" initialDir:"c:\\"
			if fileInLoc == undefined do (return undefined)
			incData = (crytools.readIN fileInLoc echo:false)
			object1.checked = true
			object1.text = (filterstring (filterstring incData[1] " =")[2] "+")[1]
			obj1PtCount = (filterstring incData[1] "+")[2]
			numVertsUpperLbl.text = ((obj1PtCount as string) + " vertices.")
			obj1_points = #()
			cacheStart = 0
			object2.enabled = true
			
			fileInArr = (filterstring fileInLoc "\\")
			dataReadOut.text = (fileInArr[fileInArr.count] + " loaded.")
			
			for i = 2 to incData.count do
			(
				if incData[i] == "collapsed verts" then
				(
					cacheStart = i
					exit
				)
				append obj1_points (filterstring incData[i] "_")[2]
			)
			
			for i = (cacheStart + 1) to (incData.count - 1) do
			(
				execute ("append collapsedCache " + incData[i])
			)
			
			--ADD: check point count against orig main
			applyCollapse.enabled = true
		)


--Apply Collapse Data
----------------------
		on applyCollapse pressed do
		(
			if getVariations.count == 0 or getVariations == undefined then
			(
				messagebox "Please select a variation."
				return undefined
			)
			
			for obj in getVariations do
			(
				if obj.mesh.verts.count != obj1_points.count then
				(
					messagebox ("Variation " + obj.name + " has " + obj.mesh.verts.count as string + " vertices,\n" + "But original model " + object1.text + " had " + obj1_points.count as string)
					return undefined
				)
			)
			
			undo "apply_collapse" on
			(
				for obj in getVariations do
				(
					for i = 1 to collapsedcache.count do
					(
						pointGet1 = (getVertByLoc obj (execute (obj.name + "_points"))[collapsedcache[i][1]])
						pointGet2 = (getVertByLoc obj (execute (obj.name + "_points"))[collapsedcache[i][2]])
						selectedVerts = #(pointGet1,pointGet2)
						--print selectedVerts
						locOfNew = ((polyOp.getVert obj pointGet1) + (polyOp.getVert obj pointGet2))/2
						polyOp.collapseVerts obj (selectedVerts as bitarray)
						update obj
						if markCollapsed.checked == true then
						(
							if (layermanager.getLayerFromName "_collapseMarkers") == undefined then
							(
								layer01 = LayerManager.newLayer()
								layer01.setname "_collapseMarkers"
								layer01.current = true
							)
							else
							(
								layer01 = (layermanager.getLayerFromName "_collapseMarkers")
								layer01.current = true
							)
							box name:("collapsed_" + collapsedcache[i][1] as string) length:.1 width:.1 height:.1 position:(locOfNew + [0,0,-.05]) wirecolor:(color 255 0 0)
						)
					)
				)
			)
		)
		
--debug group tools
-------------------
		on getIndexNum pressed do
		(
			try
			(
				if selection.count != 1 then
				(
					messagebox "please select an object"
					return undefined
				)
				polyOp.setVertSelection $ (#(indexNum.text as float) as bitarray)
			)
		catch(print "must be in sub object level (edit poly)")
		)
		
		on markVariations pressed do
		(
			if getVariations.count == 0 or getVariations == undefined then
			(
				messagebox "Please select a variation."
				return undefined
			)
			
			for obj in getVariations do
			(
				if obj.mesh.verts.count != obj1_points.count then
				(
					messagebox ("Variation " + obj.name + " has " + obj.mesh.verts.count as string + " vertices,\n" + "But original model " + object1.text + " had " + obj1_points.count as string)
					return undefined
				)
			)
			
			undo "mark verts" on
			(
				if (layermanager.getLayerFromName "_collapseMarkers") == undefined then
				(
					layer01 = LayerManager.newLayer()
					layer01.setname "_collapseMarkers"
					layer01.current = true
				)
				else
				(
					layer01 = (layermanager.getLayerFromName "_collapseMarkers")
					layer01.current = true
				)
				for obj in getVariations do
				(
					pointGet = ""
					for i = 1 to collapsedcache.count do
					(
						pointGet = (getVertByLoc obj (execute (obj.name + "_points"))[collapsedcache[i][1]])
						dummy name:("marker_" + collapsedcache[i][1] as string) position:(execute (obj.name + "_points"))[pointGet] scale:[.01,.01,.01]
						pointGet = (getVertByLoc obj (execute (obj.name + "_points"))[collapsedcache[i][2]])
						dummy name:("marker_" + collapsedcache[i][2] as string) position:(execute (obj.name + "_points"))[pointGet] scale:[.01,.01,.01]
					)
				)
			)
		)
		
		on numVertsBtn pressed do
		(
			if selection.count != 1 then
			(
				messagebox "You have more than one mesh selected"
				return undefined
			)
			numVertsLbl.text = $.mesh.verts.count as string
		)
		
	)

-------------------------------------------------------------------------------
--Mirror Deformation (Morphs)
-------------------------------------------------------------------------------
	rollout mirrorMorphs "Mirror Deformation (Morphs)"
	(

	local sourceObj = ""
	local vTotalPairs = #()

		group "Load/Save Data"
		(
			button sourceObjBTN "Select Source Object"
			label sourceName "No Source Object"
			label centDone "" offset:[-65,0]
			button saveData "Save" offset:[-25,-22]
			button loadData "Load" offset:[25,-26]
		)
		group "Mirror"
		(
			button rL "RIGHT ------> LEFT" enabled:false
			button lR "RIGHT <------ LEFT" enabled:false
			checkbox newBox "New Obj" checked:true enabled:false
			checkbox morpherBox "Morpher" offset:[80,-20] enabled:false
			checkBox epolyBox "Edit Poly" enabled:false
			checkBox sameBox "Same Obj" offset:[80,-20] enabled:false
		)
		
		on mirrorMorphs open do
		(
			
		)
		
		on sourceObjBTN pressed do
		(
			if $ == undefined then
			(
				messagebox "Nothing selected!"
				return undefined
			)
			sourceName.text = $.name
			sourceObj = $
		)
		
		on saveData pressed do
		(
			saveName = getSaveFileName types:"Data(*.dat)|*.dat|"
			if saveName == undefined then
			(
				return undefined
			)
			outputFile = createFile saveName
			totalVerts = #()
			
			numVerts = sourceObj.numVerts
			
			for i = 1 to numVerts do
			(
				append totalVerts i
			)
			
			selectedVerts = sourceObj.selectedVerts.count
			format "%\n" selectedVerts to:outputFile 
			
			vertSel = #()
			unSelVerts = #()
			
			for i=1 to selectedVerts do
			(
				append vertSel (sourceObj.selectedVerts[i].index)
			)
			
			for i=1 to totalVerts.count do
			(
				if 	(findItem vertSel i) == 0 then
				(
					append unSelVerts i
				)
			)
			
			for i = 1 to selectedVerts do
			(
				mVertPos = sourceObj.verts[vertSel[i]].pos 
				mVertPos.x = abs(mVertPos.x)
				
				dist = crytools.vertDistance mVertPos sourceObj.verts[unSelVerts[1]].pos
				mVertIndex = unSelVerts[1]
				
				for m = 2 to unSelVerts.count do
				(
					test = cryTools.vertDistance mVertPos sourceObj.verts[unSelVerts[m]].pos
					
					if test < dist then
					(
						dist = test
						mVertIndex = unSelVerts[m]
					)
				)
				
				vPair = #()
				append vPair vertSel[i]
				append vPair mVertIndex
				
				format "%\n"  vPair to:outputFile
				vIndex = findItem unSelVerts mVertIndex
				deleteItem unSelVerts vIndex
				centDone.text = ("%" + (((((i as float) / (selectedVerts as float) ) * 100) as integer) as string))
			)
			subobjectLevel = 0
			close outputFile
			centDone.text = ""
			rL.enabled = true
			lR.enabled = true
			newBox.enabled = true
			--morpherBox.enabled = true
			--epolyBox.enabled = true
			sameBox.enabled = true
		)
		
		on loadData pressed do
		(
			vPairs = #()
			vTotalPairs = #()
			loadFile = getOpenFileName types:"Data(*.dat)|*.dat|"
			if loadFile == undefined then
			(
				return undefined
			)
			outputfile = openFile loadFile
			--vPairs = (filterString (readline outputFile) " ")[1] as integer
			vPairs = (readline outputFile) as integer
			vPair
			
			for i=1 to vPairs do
			(
				vPair = execute((readline outputFile))
			   	append vTotalPairs vPair
			)
			close outputfile
			rL.enabled = true
			lR.enabled = true
			newBox.enabled = true
			--morpherBox.enabled = true
			--epolyBox.enabled = true
			sameBox.enabled = true
		)
		
		on rL pressed do
		(
			undo "mirror R --> L" on
			(
				for obj in selection do
				(	
					local obj2 = ""
					if newBox.checked or morpherBox.checked == true then
					(
						snapshot sourceObj name:(obj.name + "_mirrored")
						obj2 = (execute("$" + obj.name + "_mirrored"))
						obj2.transform = sourceObj.transform
					)
					baseVertPos
					oldPos = obj.transform
					obj.transform = sourceObj.transform
										
					if ((classof obj as string) == "Editable_Poly" or (classof obj as string) == "Editable_mesh") then
					(
						for i = 1 to vTotalPairs.count do
						(
							if newBox.checked == true then
							(
								baseVertPos = obj.verts[vTotalPairs[i][2]].pos
								baseVertPos.x = -1 * baseVertPos.x
								if (classof obj2 as string) == "Editable_Poly" then
								( 
									polyOp.setVert obj2 vTotalPairs[i][1] baseVertPos
								)
								else
								(
									meshOp.setVert obj2 vTotalPairs[i][1] baseVertPos 
								)
							)
							if sameBox.checked == true then
							(
								baseVertPos = obj.verts[vTotalPairs[i][2]].pos
								baseVertPos.x = -1 * baseVertPos.x
								if (classof obj as string) == "Editable_Poly" then
								( 
									polyOp.setVert obj vTotalPairs[i][1] baseVertPos
								)
								else
								(
									meshOp.setVert obj vTotalPairs[i][1] baseVertPos 
								)
							)
						)
						if samebox.checked == false then
						(
							obj2.transform = oldPos
						)
						if morpherBox.checked == true then
						(
							addModifier obj (morpher())
							wm3_mc_buildfromnode obj.morpher 1 obj2
							delete obj2
						)
					)
					else
					(
						messagebox "Object needs to be edit poly or edit mesh!"
					)
					obj.transform = oldPos
				)
			)
		)
		
		on lR pressed do
		(
			undo "mirror R --> L" on
			(
				for obj in selection do
				(	
					obj2 = ""
					if newBox.checked or morpherBox.checked == true then
					(
						snapshot sourceObj name:(obj.name + "_mirrored")
						obj2 = (execute("$" + obj.name + "_mirrored"))
						obj2.transform = sourceObj.transform
					)

					local baseVertPos
					oldPos = obj.transform
					obj.transform = sourceObj.transform
					
					if ((classof obj as string) == "Editable_Poly" or (classof obj as string) == "Editable_mesh") then
					(
						for i = 1 to vTotalPairs.count do
						(
							if newBox.checked == true then
							(
								baseVertPos = obj.verts[vTotalPairs[i][1]].pos
								baseVertPos.x = -1 * baseVertPos.x
								if (classof obj2 as string) == "Editable_Poly" then
								( 
									polyOp.setVert obj2 vTotalPairs[i][2] baseVertPos
								)
								else
								(
									meshOp.setVert obj2 vTotalPairs[i][2] baseVertPos 
								)
							)
							if sameBox.checked == true then
							(
								baseVertPos = obj.verts[vTotalPairs[i][1]].pos
								baseVertPos.x = -1 * baseVertPos.x
								if (classof obj as string) == "Editable_Poly" then
								( 
									polyOp.setVert obj vTotalPairs[i][2] baseVertPos
								)
								else
								(
									meshOp.setVert obj vTotalPairs[i][2] baseVertPos 
								)
							)
						)
						if newbox.checked == true then
						(
							obj2.transform = oldPos
						)
						if morpherBox.checked == true then
						(
							addModifier obj (morpher())
							wm3_mc_buildfromnode obj.morpher 1 obj2
							delete obj2
						)
					)
					else
					(
						messagebox "Object needs to be edit poly or edit mesh!"
					)
					obj.transform = oldPos
				)
			)
		)
		
		on newBox changed state do
		(
			if newBox.checked == true then
			(
				sameBox.checked = false
				morpherBox.checked = false
				epolybox.checked = false
			)
		)
		on sameBox changed state do
		(
			if sameBox.checked == true then
			(
				newBox.checked = false
				morpherBox.checked = false
				epolybox.checked = false
			)
		)
		on morpherBox changed state do
		(
			if morpherBox.checked == true then
			(
				newBox.checked = false
				sameBox.checked = false
				epolybox.checked = false
			)
		)
		on epolyBox changed state do
		(
			if epolyBox.checked == true then
			(
				newBox.checked = false
				morpherBox.checked = false
				samebox.checked = false
			)
		)
	)

	addRollout FacialTools CryMorphTools
	addRollout LocCollapse CryMorphTools rolledup:true
	addRollout DrivenMorphs CryMorphTools rolledup:true
	addRollout BakeTools CryMorphTools rolledup:true
	addRollout GeneralTools CryMorphTools rolledup:true
	addRollout mirrorMorphs CryMorphTools rolledup:true
)