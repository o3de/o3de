
fn saveOutChr = 
(
	if $ == undefined then
	(
		messagebox "Select meshes.."
		return undefined
	)
	
	nodes = selection as array
	max modify mode
	
	--check that all objs have skin
	for obj in nodes do
	(
		if obj.modifiers[#Skin] == undefined then
		(
			messagebox (obj.name + " has no Skin modifier")
			return undefined
		)
	)
	
	modPanel.setCurrentObject nodes[1].modifiers[#Skin]
	root = (crytools.findroot (skinOps.GetBoneName nodes[1].modifiers[#Skin] 1 0))
	
	--check that they all use the same skeleton
	for obj in nodes do
	(
		if (crytools.findroot (skinOps.GetBoneName obj.modifiers[#Skin] 1 0)) != root then
		(
			messagebox "hierarchy mismatch!"
		)
	)
	
	savePath = getSavePath initialDir:crytools.buildPathFull caption:"Please select a folder to dump character data:"
	if savePath == undefined then
	(
		return undefined
	)
	savePath += "\\"
	print ("Saving to " + savePath)
	global savePathCHR_crytools = savePath
	
	--save out envelopes
	for obj in nodes do
	(
			modPanel.setCurrentObject obj.modifiers[#Skin]
			skinOps.SaveEnvelope obj.modifiers[#Skin] (savePath + obj.name + ".env")
	)
	
	--save out bone list
	for obj in nodes do
	(
		boneList = #()
		for i=1 to (skinOps.getNumberBones obj.skin) do
		(
			append boneList (skinOps.GetBoneName obj.modifiers[#Skin] i 1)
		)
		crytools.writeOUT boneList (savePath + obj.name + ".bones")
	)
	
	--save out node list
	nodeNames = #()
	for obj in nodes do (append nodeNames obj.name)
	crytools.writeOUT nodenames (savePath + "nodes.txt")
	
	--save out OBJ files
	for obj in nodes do
	(
		select obj
		exportFile (savePath + obj.name + ".obj") #noPrompt selectedOnly:true using:Wavefront_ObjectExporterPlugin
	)
)
--saveOutChr()

fn readInChr = 
(
	nodes = #()
	
	if savePathCHR_crytools != undefined then
	(
		savePath = getSavePath initialDir:savePathCHR_crytools caption:"Please select a folder to load character data:"
	)
	else
	(
		savePath = getSavePath initialDir:savePathCHR_crytools caption:"Please select a folder to load character data:"
	)
	
	if savePath == undefined then
	(
		return undefined
	)
	savePath += "\\"
	print ("Loading from " + savePath)
	
	nodenames = crytools.readIN (savePath + "nodes.txt")
	
	for name in nodenames do
	(
		file = importFile (savePath + name + ".obj") #noPrompt
		$.name = name
	)
	
	for name in nodenames do (append nodes (getnodebyname name))

	for obj in nodes do
	(	
		addModifier obj (Skin ())
	)
)
--readInChr()

fn addBones savePath =
(
	nodes = #()
	nodenames = crytools.readIN (savePath + "nodes.txt")
	for name in nodenames do (append nodes (getnodebyname name))
	print nodes
	
	if crytools.maxversionnum >= 9 then
		(
			DialogMonitorOPS.RegisterNotification ANoon_EnvelopeCallbackFunction ID:#ANoon_Envelopes
			DialogMonitorOPS.Enabled = true
		)
	
	for obj in nodes do
	(
		boneNames = crytools.readIN (savePath + obj.name + ".bones")
		bones = #()
		for name in boneNames do (append bones (getnodebyname name))
		max modify mode
		modPanel.setCurrentObject obj.modifiers[#Skin]
		for bone in bones do
		(
			skinOps.addbone obj.modifiers[#Skin] bone 1
		)
		skinOps.LoadEnvelope obj.modifiers[#Skin] (savePath + obj.name + ".env")
		skinOps.LoadEnvelope obj.modifiers[#Skin] (savePath + obj.name + ".env")
	)
	
	if crytools.maxversionnum >= 9 then
	(
		DialogMonitorOPS.Enabled = false
		DialogMonitorOPS.UnRegisterNotification ID:#ANoon_Envelopes
	)
)















