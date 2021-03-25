------Display Vertex Colors-------
for obj in selection do
(
	if obj.showVertexColors == false then
	(
		obj.vertexColorType = #color
		obj.showVertexColors = true
	)
	else
	(
		if obj.vertexColorType ==#alpha then
		(
			obj.vertexColorType = #color
			obj.showVertexColors = false
			obj.showVertexColors = true
		)
		else
		(
			obj.showVertexColors = false
		)
	)
)