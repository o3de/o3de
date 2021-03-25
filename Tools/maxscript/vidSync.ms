(
	rollout vidSync "vidSync"
	(
		fn shuttle =
		(
			try
			(
				offset = ((currenttime.frame as integer) + vidSync.offset.value)
				vidsync.the_bmp.fileName = vidsync.allframes[offset]
				vidsync.frameAssoc.text = ("Association: " + (currenttime as string) + " / " + getfilenamefile vidsync.allframes[offset]+\
					"."+vidSync.mediaType+"  /  "+(((offset/30)/60) as integer) as string+":"+ formattedprint ((mod (offset/30) 60) as integer) format:"02d")
			)
			catch(vidsync.frameAssoc.text = ("Association: " + (currenttime as string) + " / " + \
					" *** NO FRAME DATA ***"))
		)
		fn getSufFiles suffix path_name = ( return (for file in (getFiles ((getfilenamepath path_name) + "*." + suffix)) collect file))
		
		local allframes = #()
		local mediaType = "null"
		button loadFrameSeq "Load Frame Sequence" align:#left --offset:[-90,0]
		spinner offset "Offset:" scale:1 offset:[0,-23] range:[-10000,10000,1] fieldWidth:60 align:#right
		bitmap the_bmp height:220 width:300
		label frameAssoc "Association: no video loaded" align:#left
		label errorLbl "" align:#center
		
		on loadFrameSeq pressed do
		(
			f = getOpenFileName caption:"Open A Frame of a Sequence:" \
			filename:"f:/" types:"TGA(*.tga)|*.tga|PNG(*.png)|*.png|JPEG(*.jpg)|*.jpg"
			if f == undefined then (return undefined)
			else
			(
				suffix = filterstring f "."
				suffix = suffix[suffix.count]
				case suffix of
				(
					"tga": mediaType = "tga"
					"jpg": mediaType = "jpg"
					"png": mediaType = "png"
				)
				
				allFrames = getSufFiles mediaType f
				
				filepath = allframes[1]
				fileData = openBitMap filepath
				the_bmp.fileName = filepath
				the_bmp.width = fileData.width
				the_bmp.height = fileData.height
				vidsync.width = fileData.width + 20
				vidsync.height = filedata.height + 60
				vidsync.frameAssoc.pos += [0,((vidsync.height-20)-vidsync.frameAssoc.pos.y)]
				shuttle()
				registerTimeCallback vidSync.shuttle
			)
		)
		on offset changed val do
		(
			shuttle()
		)
		on vidSync close do
		(
			try (unregistertimecallback vidSync.shuttle)catch()
		)
	)
	createDialog vidSync width:320 height:280
)