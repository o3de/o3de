--sceneView . Christopher Evans . Crytek

if sceneView != undefined then
(
	destroyDialog sceneView
)

ilTv = dotNetObject "System.Windows.Forms.ImageList"
ilTv.imageSize = dotNetObject "System.Drawing.Size" 16 15

rollout sceneView "SceneView v.001"
(
	
	fn getIconFromBitmap thePath number iconFileName =
	(
		theFileName = getDir #image +"\\icon_"+ iconFileName +".bmp"
		if not doesFileExist theFileName do
		(
			tempBmp = openBitmap thePath
			iconBmp = bitmap 16 15
			for v = 0 to 14 do
			setPixels iconBmp [0,v] (getPixels tempBmp [(number-1)*16, v] 16)
			iconBmp.filename = theFileName
			save iconBmp
			close iconBmp 
			close tempBmp
		) 
		img = dotNetClass "System.Drawing.Image" --create an image
		ilTv.images.add (img.fromFile theFileName) --add to the list
	)

	fn initTreeView tv =
	(
		tv.Indent= 28
		tv.CheckBoxes = true --same as in ActiveX
		tv.labelEdit = true
		tv.Indent = 15
		tv.Scrollable = true
		colorTest = dotNetClass "System.Drawing.Color"
		tv.BackColor = colorTest.FromArgb  255 196 196 196
		iconDir = (getDir #ui) + "\\icons\\"
		--We call our function for each icon, this time also passing a
		--third argument with the icon name suffix.
		getIconFromBitmap (iconDir + "Standard_16i.bmp") 2 "Sphere" 
		getIconFromBitmap (iconDir + "Standard_16i.bmp") 1 "Box"
		getIconFromBitmap (iconDir + "Lights_16i.bmp") 3 "Light"
		getIconFromBitmap (iconDir + "Cameras_16i.bmp") 2 "Camera"
		getIconFromBitmap (iconDir + "Helpers_16i.bmp") 1 "Helper"
		getIconFromBitmap (iconDir + "Splines_16i.bmp") 2 "Shape"
		getIconFromBitmap (iconDir + "Systems_16i.bmp") 1 "Bone"
		--At the end, we assign the ImageList to the TreeView.
		tv.imageList = ilTv 
	)


	fn addChildren theNode theChildren =
	(
		for c in theChildren do
		(
			newNode = theNode.Nodes.add c.name c.name
			newNode.tag = dotNetMXSValue c
			--newNode.count = c.handle
			--By default, all nodes will use icon 0 (the first one) unless
			--specified otherwise via the .iconIndex and .selectedIconIndex
			--properties. We set both of them to the icon corresponding to
			--the superclass of the scene object:
			newNode.imageIndex = newNode.selectedImageIndex = case superclassof c of
			(
				Default: 1
				GeometryClass:
				(
					case (c.classid[1]) of
					(
						Default: 1
						683634317: 6 -- bones
						37157: 6 -- biped objects
					)
				)
				Light: 2
				Camera: 3
				Helper: 4
			)
			
			newNode.checked = not c.isHidden --same as in ActiveX
			--For the color, we create a DotNet color class from the 
			--wirecolor of the object and assign to the .forecolor of
			--the TreeView node:
			--newNode.forecolor = (dotNetClass "System.Drawing.Color").fromARGB c.wirecolor.r c.wirecolor.g c.wirecolor.b
			addChildren newNode c.children
		)
	)

	 

	--Since every node uses icon with index 0 unless specified otherwise
	--the Root Node will use the first icon by default.
	fn fillInTreeView tv =
	(
		theRoot = sceneview.tv.Nodes.add "WORLD" "WORLD"
		rootNodes = for o in objects where o.parent == undefined collect o
		sceneview.addChildren theRoot rootNodes 
	)

	fn refresh = 
	(
		sceneview.tv.nodes.clear()
		sceneview.fillInTreeView tv
		sceneview.tv.topnode.expand()
	)
	
	fn getSelectedNode =
	(
		try
		(
			if selection[1] != undefined then
			(
				--print selection[1].name
				sceneview.tv.SelectedNode = (sceneview.tv.nodes.Find selection[1].name true)[1]
				sceneview.tv.SelectedNode.EnsureVisible()
				colorTest = dotNetClass "System.Drawing.Color"
				sceneview.tv.selectednode.backColor = colorTest.FromArgb  255 221 221 221
				sceneview.tv.refresh()
			)
		)
		catch -- for undo
		(
			refresh()
			if selection[1] != undefined then
			(
				sceneview.tv.SelectedNode = (sceneview.tv.nodes.Find selection[1].name true)[1]
				sceneview.tv.SelectedNode.EnsureVisible()
				colorTest = dotNetClass "System.Drawing.Color"
				sceneview.tv.selectednode.backColor = colorTest.FromArgb  255 221 221 221
			)
		)
	)
	
	fn hideNode = 
	(
		if selection != undefined then
		(
			for obj in selection do
			(
				sceneview.tv.SelectedNode = (sceneview.tv.nodes.Find obj.name true)[1]
				sceneview.tv.selectednode.checked = false
			)
		)
	)
	
	fn unhideNode = 
	(
		if selection != undefined then
		(
			for obj in selection do
			(
				sceneview.tv.SelectedNode = (sceneview.tv.nodes.Find obj.name true)[1]
				sceneview.tv.selectednode.checked = true
			)
		)
	)
	
	dotNetControl tv "TreeView" width:290 height:565 align:#center
	button layerM "Layer Manager" offset:[-9,0] Align:#left
	label info " (X) + all   (C/V) +- children" offset:[5,-22]
	
	on layerM pressed do
	(
		macros.run "layers" "layermanager"
	)
	
	on tv Click arg do 
	(
		hitNode = tv.GetNodeAt (dotNetObject "System.Drawing.Point" arg.x arg.y)
		if hitNode != undefined do 
		try(select hitNode.tag.value) catch(max select none) 
	)

	on tv AfterCheck arg do
	(
		try (arg.node.tag.value.isHidden = not arg.node.checked)catch()
	)
	
	on tv AfterLabelEdit arg do
	(
		if arg.label != undefined then
		(
			arg.node.tag.value.name = arg.label
		)
	)
			
	
	on tv keyUp arg do
	(
		--print arg.keyValue
		case arg.keyValue of
		(
			67: tv.selectedNode.collapse() -- c key
			88: tv.expandAll() -- x key
			86: tv.selectedNode.ExpandAll() -- v key
			13: tv.selectedNode.beginEdit() -- enter key
			113: tv.selectedNode.beginEdit() -- F2
			116: refresh() -- F5
		)
	)
	
	fn OnClick sender args =
	(
		--print sender.Text
		case sender.Text of
		(
			"Expand branches": if tv.selectedNode != undefined then tv.selectedNode.ExpandAll()
		)
	)	
	
	on tv beforeSelect arg do
	(
		colorTest = dotNetClass "System.Drawing.Color"
		try (sceneview.tv.selectednode.backColor = colorTest.FromArgb  255 196 196 196) catch()
	)
	
	on tv nodeMouseClick arg do
	(
		if arg.button == tv.mousebuttons.right then
		(
			contextMenu = dotNetObject "System.Windows.Forms.ContextMenu"
			contextMenu.MenuItems.Clear()
			dotnet.addeventhandler (contextMenu.MenuItems.Add("Select all children")) "Click" OnClick
			dotnet.addeventhandler (contextMenu.MenuItems.Add("Expand branches")) "Click" OnClick
			pointTest = (dotNetObject "System.Drawing.Point" arg.x arg.y)
			contextmenu.Show tv pointTest
		)
	)

	on sceneView open do 
	(
		initTreeView tv 
		fillInTreeView tv
		tv.topnode.expand()
		callbacks.addScript #nodeCreated "sceneView.refresh()" id:#upDateSceneView
		callbacks.addScript #nodePostDelete "sceneView.refresh()" id:#upDateSceneView
		callbacks.addScript #nodeRenamed "sceneView.refresh()" id:#upDateSceneView
		callbacks.addScript #postNodesCloned "sceneView.refresh()" id:#upDateSceneView
		callbacks.addScript #postMirrorNodes "sceneView.refresh()" id:#upDateSceneView
		callbacks.addScript #selectionSetChanged "sceneView.getSelectedNode()" id:#upDateSceneView
		callbacks.addScript #nodeHide "sceneView.hideNode()" id:#upDateSceneView
		--callbacks.addScript #nodeUnhide "sceneView.unhideNode()" id:#upDateSceneView
		callbacks.addScript #sceneUndo "sceneView.refresh()" id:#upDateSceneView
		callbacks.addScript #sceneRedo "sceneView.refresh()" id:#upDateSceneView
	)

	on sceneView close do
	(
		callbacks.removeScripts id:#upDateSceneView
	)

	on sceneView resized size do
		(
			size1 = size as string
			size2 = filterstring size1 "[],"
			layerM.pos = [4, (sceneView.height - 26)]
			info.pos = [100, (sceneView.height - 23)]
			tv.height = ((size2[2] as float) - 35)
			tv.width = ((size2[1] as float) - 10)
		)
)

createDialog sceneView 300 600 style:#(#style_resizing, #style_titlebar, #style_border, #style_sysmenu)

