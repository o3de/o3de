	
	function createBlendFromMaxMat obj idx =
	(
		
		local blendShader = ""
		local msg = ""
		
		global buildPathFull_crytools
		
		if buildPathFull_crytools == undefined then (
			local scriptPath = getSourceFileName()
			scriptPath = substituteString scriptPath "\\CryMakeBlendShader.ms" ""
			
			print scriptPath
			
			blendShader = (scriptPath + "\\fx\\cryBlendShader.fx")
		) else (
		
			blendShader = (buildPathFull_crytools + "Tools\\maxscript\\fx\\cryBlendShader.fx")
		)
		
		--local obj = $

		local mat = obj.material
		local numSubMats = getNumSubMtls mat
		local matSlotOrig = (idx * 2) - 1
		setMeditMaterial matSlotOrig mat

		if (numSubMats > 0) then
		(
			local newMat = multiMaterial()
			newMat.numsubs = numSubMats
			newMat.name = (mat.name + "_DX")

			local newSlot = matSlotOrig+1
			setMeditMaterial newSlot newMat
			activeMeditSlot = newSlot

			print ("New MultiMaterial: "+newMat.name)
			msg = ("Created mat: "+newMat.name+" in slot "+(newSlot as string))
			
			local subMats = mat.materialList

			for i = 1 to numSubMats do 
			(
				newMat.materialList[i] = DirectX_9_Shader ()
				newMat.materialList[i].effectFile = blendShader

				newMat.materialList[i].name = mat.materialList[i].name
				newMat.names[i] = mat.materialList[i].name
				
				if (isValidObj mat.materialList[i].diffuseMap) then
				(
					print ("Creating submat "+mat.materialList[i].name+"...")
					local bmFilename = mat.materialList[i].diffuseMap.fileName
					
					if (doesFileExist(bmFilename)) then
					(
						local bm = openBitMap bmFilename
						newMat.materialList[i].diffuseTexture1 = bm
					) else (
						print ("WARNING: File Not Found: "+bmFilename)
					)
				)

				if (isValidObj mat.materialList[i].bumpMap) then
				(
					local bmFilename = mat.materialList[i].bumpMap.fileName
					
					if (doesFileExist(bmFilename)) then
					(
						local bm = openBitMap bmFilename
						newMat.materialList[i].normalMap = bm
					) else (
						print ("WARNING: File Not Found: "+bmFilename)
					)
				)

			)

		) else (
				local newMat = Material()
				newMat.name = (mat.name + "_DX")

				local newSlot = matSlotOrig+1
				setMeditMaterial newSlot newMat
				activeMeditSlot = newSlot

				print ("New Material: "+newMat.name)
				msg = ("Created mat: "+newMat.name+" in slot "+(newSlot as string))

				newMat = DirectX_9_Shader ()
				newMat.effectFile = blendShader

				if (isValidObj mat.diffuseMap) then
				(
					local bmFilename = mat.diffuseMap.fileName
					if (doesFileExist(bmFilename)) then
					(
						local bm = openBitMap bmFilename
						newMat.diffuseTexture1 = bm
					) else (
						print ("WARNING: File Not Found: "+bmFilename)
					)
				)

				if (isValidObj mat.bumpMap) then
				(
					local bmFilename = mat.bumpMap.fileName
					if (doesFileExist(bmFilename)) then
					(
						local bm = openBitMap bmFilename
						newMat.normalMap = bm
					) else (
						print ("WARNING: File Not Found: "+bmFilename)
					)
				)
		)

		msg = (msg + "\n\nNow pick Dirt textures and apply new material to your object.")
		MatEditor.Open()
			
		messageBox msg title:"Cry Make Blend Shader"

	)

	try(destroyDialog CryBlendShader_rollout)catch()
	rollout CryBlendShader_rollout "CryMakeBlendShader" width:200
	(
		button cryMakeBlend_button "Create Blend Shader" toolTip:"Create a DirectX Blend shader from the selected object's Max material(s)"
		label cryMakeBlendMsg1 "Select an object, and press button"
		label cryMakeBlendMsg2 "to create Blend Shader."

		on cryMakeBlend_button pressed do
		(
			if ($ == undefined) then
			(
				cryMakeBlendMsg1.text = "You must select an object first"
			) else (
				for i=1 to selection.count do
				(
					createBlendFromMaxMat selection[i] i
				)
			)
		)
		
	)
	
	createDialog CryBlendShader_rollout
	
