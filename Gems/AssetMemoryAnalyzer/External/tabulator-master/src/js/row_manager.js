var RowManager = function(table){

	this.table = table;
	this.element = this.createHolderElement(); //containing element
	this.tableElement = this.createTableElement(); //table element
	this.columnManager = null; //hold column manager object
	this.height = 0; //hold height of table element

	this.firstRender = false; //handle first render
	this.renderMode = "classic"; //current rendering mode

	this.rows = []; //hold row data objects
	this.activeRows = []; //rows currently available to on display in the table
	this.activeRowsCount = 0; //count of active rows

	this.displayRows = []; //rows currently on display in the table
	this.displayRowsCount = 0; //count of display rows

	this.scrollTop = 0;
	this.scrollLeft = 0;

	this.vDomRowHeight = 20; //approximation of row heights for padding

	this.vDomTop = 0; //hold position for first rendered row in the virtual DOM
	this.vDomBottom = 0; //hold possition for last rendered row in the virtual DOM

	this.vDomScrollPosTop = 0; //last scroll position of the vDom top;
	this.vDomScrollPosBottom = 0; //last scroll position of the vDom bottom;

	this.vDomTopPad = 0; //hold value of padding for top of virtual DOM
	this.vDomBottomPad = 0; //hold value of padding for bottom of virtual DOM

	this.vDomMaxRenderChain = 90; //the maximum number of dom elements that can be rendered in 1 go

	this.vDomWindowBuffer = 0; //window row buffer before removing elements, to smooth scrolling

	this.vDomWindowMinTotalRows = 20; //minimum number of rows to be generated in virtual dom (prevent buffering issues on tables with tall rows)
	this.vDomWindowMinMarginRows = 5; //minimum number of rows to be generated in virtual dom margin

	this.vDomTopNewRows = []; //rows to normalize after appending to optimize render speed
	this.vDomBottomNewRows = []; //rows to normalize after appending to optimize render speed
};

//////////////// Setup Functions /////////////////

RowManager.prototype.createHolderElement = function (){
	var el = document.createElement("div");

	el.classList.add("tabulator-tableHolder");
	el.setAttribute("tabindex", 0);

	return el;
};

RowManager.prototype.createTableElement = function (){
	var el = document.createElement("div");

	el.classList.add("tabulator-table");

	return el;
};

//return containing element
RowManager.prototype.getElement = function(){
	return this.element;
};

//return table element
RowManager.prototype.getTableElement = function(){
	return this.tableElement;
};

//return position of row in table
RowManager.prototype.getRowPosition = function(row, active){
	if(active){
		return this.activeRows.indexOf(row);
	}else{
		return this.rows.indexOf(row);
	}
};


//link to column manager
RowManager.prototype.setColumnManager = function(manager){
	this.columnManager = manager;
};

RowManager.prototype.initialize = function(){
	var self = this;

	self.setRenderMode();

	//initialize manager
	self.element.appendChild(self.tableElement);

	self.firstRender = true;

	//scroll header along with table body
	self.element.addEventListener("scroll", function(){
		var left = self.element.scrollLeft;

		//handle horizontal scrolling
		if(self.scrollLeft != left){
			self.columnManager.scrollHorizontal(left);

			if(self.table.options.groupBy){
				self.table.modules.groupRows.scrollHeaders(left);
			}

			if(self.table.modExists("columnCalcs")){
				self.table.modules.columnCalcs.scrollHorizontal(left);
			}
		}

		self.scrollLeft = left;
	});

	//handle virtual dom scrolling
	if(this.renderMode === "virtual"){

		self.element.addEventListener("scroll", function(){
			var top = self.element.scrollTop;
			var dir = self.scrollTop > top;

			//handle verical scrolling
			if(self.scrollTop != top){
				self.scrollTop = top;
				self.scrollVertical(dir);

				if(self.table.options.ajaxProgressiveLoad == "scroll"){
					self.table.modules.ajax.nextPage(self.element.scrollHeight - self.element.clientHeight - top);
				}
			}else{
				self.scrollTop = top;
			}

		});
	}
};


////////////////// Row Manipulation //////////////////

RowManager.prototype.findRow = function(subject){
	var self = this;

	if(typeof subject == "object"){

		if(subject instanceof Row){
			//subject is row element
			return subject;
		}else if(subject instanceof RowComponent){
			//subject is public row component
			return subject._getSelf() || false;
		}else if(subject instanceof HTMLElement){
			//subject is a HTML element of the row
			let match = self.rows.find(function(row){
				return row.element === subject;
			});

			return match || false;
		}

	}else if(typeof subject == "undefined" || subject === null){
		return false;
	}else{
		//subject should be treated as the index of the row
		let match = self.rows.find(function(row){
			return row.data[self.table.options.index] == subject;
		});

		return match || false;
	}

	//catch all for any other type of input

	return false;
};

RowManager.prototype.getRowFromPosition = function(position, active){
	if(active){
		return this.activeRows[position];
	}else{
		return this.rows[position];
	}
};

RowManager.prototype.scrollToRow = function(row, position, ifVisible){
	var rowIndex = this.getDisplayRows().indexOf(row),
	rowEl = row.getElement(),
	rowTop,
	offset = 0;

	return new Promise((resolve, reject) => {
		if(rowIndex > -1){

			if(typeof position === "undefined"){
				position = this.table.options.scrollToRowPosition;
			}

			if(typeof ifVisible === "undefined"){
				ifVisible = this.table.options.scrollToRowIfVisible;
			}


			if(position === "nearest"){
				switch(this.renderMode){
					case"classic":
					rowTop = Tabulator.prototype.helpers.elOffset(rowEl).top;
					position = Math.abs(this.element.scrollTop - rowTop) > Math.abs(this.element.scrollTop + this.element.clientHeight - rowTop) ? "bottom" : "top";
					break;
					case"virtual":
					position = Math.abs(this.vDomTop - rowIndex) > Math.abs(this.vDomBottom - rowIndex) ? "bottom" : "top";
					break;
				}
			}

			//check row visibility
			if(!ifVisible){
				if(Tabulator.prototype.helpers.elVisible(rowEl)){
					offset = Tabulator.prototype.helpers.elOffset(rowEl).top - Tabulator.prototype.helpers.elOffset(this.element).top;

					if(offset > 0 && offset < this.element.clientHeight - rowEl.offsetHeight){
						return false;
					}
				}
			}

			//scroll to row
			switch(this.renderMode){
				case"classic":
				this.element.scrollTop = Tabulator.prototype.helpers.elOffset(rowEl).top - Tabulator.prototype.helpers.elOffset(this.element).top + this.element.scrollTop;
				break;
				case"virtual":
				this._virtualRenderFill(rowIndex, true);
				break;
			}

			//align to correct position
			switch(position){
				case "middle":
				case "center":
				this.element.scrollTop = this.element.scrollTop - (this.element.clientHeight / 2);
				break;

				case "bottom":
				this.element.scrollTop = this.element.scrollTop - this.element.clientHeight + rowEl.offsetHeight;
				break;
			}

			resolve();

		}else{
			console.warn("Scroll Error - Row not visible");
			reject("Scroll Error - Row not visible");
		}
	});
};


////////////////// Data Handling //////////////////

RowManager.prototype.setData = function(data, renderInPosition){
	var self = this;

	return new Promise((resolve, reject)=>{
		if(renderInPosition && this.getDisplayRows().length){
			if(self.table.options.pagination){
				self._setDataActual(data, true);
			}else{
				this.reRenderInPosition(function(){
					self._setDataActual(data);
				});
			}
		}else{
			this.resetScroll();
			this._setDataActual(data);
		}

		resolve();
	});
};

RowManager.prototype._setDataActual = function(data, renderInPosition){
	var self = this;

	self.table.options.dataLoading.call(this.table, data);

	self.rows.forEach(function(row){
		row.wipe();
	});

	self.rows = [];

	if(this.table.options.history && this.table.modExists("history")){
		this.table.modules.history.clear();
	}

	if(Array.isArray(data)){

		if(this.table.modExists("selectRow")){
			this.table.modules.selectRow.clearSelectionData();
		}

		data.forEach(function(def, i){
			if(def && typeof def === "object"){
				var row = new Row(def, self);
				self.rows.push(row);
			}else{
				console.warn("Data Loading Warning - Invalid row data detected and ignored, expecting object but received:", def);
			}
		});

		self.table.options.dataLoaded.call(this.table, data);

		self.refreshActiveData(false, false, renderInPosition);
	}else{
		console.error("Data Loading Error - Unable to process data due to invalid data type \nExpecting: array \nReceived: ", typeof data, "\nData:     ", data);
	}
};

RowManager.prototype.deleteRow = function(row){
	var allIndex = this.rows.indexOf(row),
	activeIndex = this.activeRows.indexOf(row);

	if(activeIndex > -1){
		this.activeRows.splice(activeIndex, 1);
	}

	if(allIndex > -1){
		this.rows.splice(allIndex, 1);
	}

	this.setActiveRows(this.activeRows);

	this.displayRowIterator(function(rows){
		var displayIndex = rows.indexOf(row);

		if(displayIndex > -1){
			rows.splice(displayIndex, 1);
		}
	});

	this.reRenderInPosition();

	this.table.options.rowDeleted.call(this.table, row.getComponent());

	this.table.options.dataEdited.call(this.table, this.getData());

	if(this.table.options.groupBy && this.table.modExists("groupRows")){
		this.table.modules.groupRows.updateGroupRows(true);
	}else if(this.table.options.pagination && this.table.modExists("page")){
		this.refreshActiveData(false, false, true);
	}else{
		if(this.table.options.pagination && this.table.modExists("page")){
			this.refreshActiveData("page");
		}
	}

};

RowManager.prototype.addRow = function(data, pos, index, blockRedraw){

	var row = this.addRowActual(data, pos, index, blockRedraw);

	if(this.table.options.history && this.table.modExists("history")){
		this.table.modules.history.action("rowAdd", row, {data:data, pos:pos, index:index});
	}

	return row;
};

//add multiple rows
RowManager.prototype.addRows = function(data, pos, index){
	var self = this,
	length = 0,
	rows = [];

	return new Promise((resolve, reject) => {
		pos = this.findAddRowPos(pos);

		if(!Array.isArray(data)){
			data = [data];
		}

		length = data.length - 1;

		if((typeof index == "undefined" && pos) || (typeof index !== "undefined" && !pos)){
			data.reverse();
		}

		data.forEach(function(item, i){
			var row = self.addRow(item, pos, index, true);
			rows.push(row);
		});

		if(this.table.options.groupBy && this.table.modExists("groupRows")){
			this.table.modules.groupRows.updateGroupRows(true);
		}else if(this.table.options.pagination && this.table.modExists("page")){
			this.refreshActiveData(false, false, true);
		}else{
			this.reRenderInPosition();
		}

		//recalc column calculations if present
		if(this.table.modExists("columnCalcs")){
			this.table.modules.columnCalcs.recalc(this.table.rowManager.activeRows);
		}

		resolve(rows);
	});
};

RowManager.prototype.findAddRowPos = function(pos){
	if(typeof pos === "undefined"){
		pos = this.table.options.addRowPos;
	}

	if(pos === "pos"){
		pos = true;
	}

	if(pos === "bottom"){
		pos = false;
	}

	return pos;
};


RowManager.prototype.addRowActual = function(data, pos, index, blockRedraw){
	var row = data instanceof Row ? data : new Row(data || {}, this),
	top = this.findAddRowPos(pos),
	dispRows;

	if(!index && this.table.options.pagination && this.table.options.paginationAddRow == "page"){
		dispRows = this.getDisplayRows();

		if(top){
			if(dispRows.length){
				index = dispRows[0];
			}else{
				if(this.activeRows.length){
					index = this.activeRows[this.activeRows.length-1];
					top = false;
				}
			}
		}else{
			if(dispRows.length){
				index = dispRows[dispRows.length - 1];
				top = dispRows.length < this.table.modules.page.getPageSize() ? false : true;
			}
		}
	}

	if(index){
		index = this.findRow(index);
	}

	if(this.table.options.groupBy && this.table.modExists("groupRows")){
		this.table.modules.groupRows.assignRowToGroup(row);

		var groupRows = row.getGroup().rows;

		if(groupRows.length > 1){

			if(!index || (index && groupRows.indexOf(index) == -1)){
				if(top){
					if(groupRows[0] !== row){
						index = groupRows[0];
						this._moveRowInArray(row.getGroup().rows, row, index, top);
					}
				}else{
					if(groupRows[groupRows.length -1] !== row){
						index = groupRows[groupRows.length -1];
						this._moveRowInArray(row.getGroup().rows, row, index, top);
					}
				}
			}else{
				this._moveRowInArray(row.getGroup().rows, row, index, top);
			}
		}
	}

	if(index){
		let allIndex = this.rows.indexOf(index),
		activeIndex = this.activeRows.indexOf(index);

		this.displayRowIterator(function(rows){
			var displayIndex = rows.indexOf(index);

			if(displayIndex > -1){
				rows.splice((top ? displayIndex : displayIndex + 1), 0, row);
			}
		});

		if(activeIndex > -1){
			this.activeRows.splice((top ? activeIndex : activeIndex + 1), 0, row);
		}

		if(allIndex > -1){
			this.rows.splice((top ? allIndex : allIndex + 1), 0, row);
		}

	}else{

		if(top){

			this.displayRowIterator(function(rows){
				rows.unshift(row);
			});

			this.activeRows.unshift(row);
			this.rows.unshift(row);
		}else{
			this.displayRowIterator(function(rows){
				rows.push(row);
			});

			this.activeRows.push(row);
			this.rows.push(row);
		}
	}

	this.setActiveRows(this.activeRows);

	this.table.options.rowAdded.call(this.table, row.getComponent());

	this.table.options.dataEdited.call(this.table, this.getData());

	if(!blockRedraw){
		this.reRenderInPosition();
	}

	return row;
};

RowManager.prototype.moveRow = function(from, to, after){
	if(this.table.options.history && this.table.modExists("history")){
		this.table.modules.history.action("rowMove", from, {pos:this.getRowPosition(from), to:to, after:after});
	}

	this.moveRowActual(from, to, after);

	this.table.options.rowMoved.call(this.table, from.getComponent());
};


RowManager.prototype.moveRowActual = function(from, to, after){
	var self = this;
	this._moveRowInArray(this.rows, from, to, after);
	this._moveRowInArray(this.activeRows, from, to, after);

	this.displayRowIterator(function(rows){
		self._moveRowInArray(rows, from, to, after);
	});

	if(this.table.options.groupBy && this.table.modExists("groupRows")){
		var toGroup = to.getGroup();
		var fromGroup = from.getGroup();

		if(toGroup === fromGroup){
			this._moveRowInArray(toGroup.rows, from, to, after);
		}else{
			if(fromGroup){
				fromGroup.removeRow(from);
			}

			toGroup.insertRow(from, to, after);
		}
	}
};


RowManager.prototype._moveRowInArray = function(rows, from, to, after){
	var	fromIndex, toIndex, start, end;

	if(from !== to){

		fromIndex = rows.indexOf(from);

		if (fromIndex > -1) {

			rows.splice(fromIndex, 1);

			toIndex = rows.indexOf(to);

			if (toIndex > -1) {

				if(after){
					rows.splice(toIndex+1, 0, from);
				}else{
					rows.splice(toIndex, 0, from);
				}

			}else{
				rows.splice(fromIndex, 0, from);
			}
		}

		//restyle rows
		if(rows === this.getDisplayRows()){

			start = fromIndex < toIndex ? fromIndex : toIndex;
			end = toIndex > fromIndex ? toIndex : fromIndex +1;

			for(let i = start; i <= end; i++){
				if(rows[i]){
					this.styleRow(rows[i], i);
				}
			}
		}
	}
};

RowManager.prototype.clearData = function(){
	this.setData([]);
};

RowManager.prototype.getRowIndex = function(row){
	return this.findRowIndex(row, this.rows);
};


RowManager.prototype.getDisplayRowIndex = function(row){
	var index = this.getDisplayRows().indexOf(row);
	return index > -1 ? index : false;
};

RowManager.prototype.nextDisplayRow = function(row, rowOnly){
	var index = this.getDisplayRowIndex(row),
	nextRow = false;


	if(index !== false && index < this.displayRowsCount -1){
		nextRow = this.getDisplayRows()[index+1];
	}

	if(nextRow && (!(nextRow instanceof Row) || nextRow.type != "row")){
		return this.nextDisplayRow(nextRow, rowOnly);
	}

	return nextRow;
};

RowManager.prototype.prevDisplayRow = function(row, rowOnly){
	var index = this.getDisplayRowIndex(row),
	prevRow = false;

	if(index){
		prevRow = this.getDisplayRows()[index-1];
	}

	if(prevRow && (!(prevRow instanceof Row) || prevRow.type != "row")){
		return this.prevDisplayRow(prevRow, rowOnly);
	}

	return prevRow;
};

RowManager.prototype.findRowIndex = function(row, list){
	var rowIndex;

	row = this.findRow(row);

	if(row){
		rowIndex = list.indexOf(row);

		if(rowIndex > -1){
			return rowIndex;
		}
	}

	return false;
};


RowManager.prototype.getData = function(active, transform){
	var self = this,
	output = [];

	var rows = active ? self.activeRows : self.rows;

	rows.forEach(function(row){
		output.push(row.getData(transform || "data"));
	});

	return output;
};

RowManager.prototype.getHtml = function(active){
	var data = this.getData(active),
	columns = [],
	header = "",
	body = "",
	table = "";

		//build header row
		this.table.columnManager.getColumns().forEach(function(column){
			var def = column.getDefinition();

			if(column.visible && !def.hideInHtml){
				header += `<th>${(def.title || "")}</th>`;
				columns.push(column);
			}
		});

		//build body rows
		data.forEach(function(rowData){
			var row = "";

			columns.forEach(function(column){
				var value = column.getFieldValue(rowData);

				if(typeof value === "undefined" || value === null){
					value = ":";
				}

				row += `<td>${value}</td>`;
			});

			body += `<tr>${row}</tr>`;
		});

		//build table
		table = `<table>
		<thead>
		<tr>${header}</tr>
		</thead>
		<tbody>${body}</tbody>
		</table>`;

		return table;
	};

	RowManager.prototype.getComponents = function(active){
		var self = this,
		output = [];

		var rows = active ? self.activeRows : self.rows;

		rows.forEach(function(row){
			output.push(row.getComponent());
		});

		return output;
	}

	RowManager.prototype.getDataCount = function(active){
		return active ? this.rows.length : this.activeRows.length;
	};

	RowManager.prototype._genRemoteRequest = function(){
		var self = this,
		table = self.table,
		options = table.options,
		params = {};

		if(table.modExists("page")){
		//set sort data if defined
		if(options.ajaxSorting){
			let sorters = self.table.modules.sort.getSort();

			sorters.forEach(function(item){
				delete item.column;
			});

			params[self.table.modules.page.paginationDataSentNames.sorters] = sorters;
		}

		//set filter data if defined
		if(options.ajaxFiltering){
			let filters = self.table.modules.filter.getFilters(true, true);

			params[self.table.modules.page.paginationDataSentNames.filters] = filters;
		}


		self.table.modules.ajax.setParams(params, true);
	}

	table.modules.ajax.sendRequest()
	.then((data)=>{
		self.setData(data);
	})
	.catch((e)=>{});

};

//choose the path to refresh data after a filter update
RowManager.prototype.filterRefresh = function(){
	var table = this.table,
	options = table.options,
	left = this.scrollLeft;


	if(options.ajaxFiltering){
		if(options.pagination == "remote" && table.modExists("page")){
			table.modules.page.reset(true);
			table.modules.page.setPage(1);
		}else if(options.ajaxProgressiveLoad){
			table.modules.ajax.loadData();
		}else{
			//assume data is url, make ajax call to url to get data
			this._genRemoteRequest();
		}
	}else{
		this.refreshActiveData("filter");
	}

	this.scrollHorizontal(left);
};

//choose the path to refresh data after a sorter update
RowManager.prototype.sorterRefresh = function(){
	var table = this.table,
	options = this.table.options,
	left = this.scrollLeft;

	if(options.ajaxSorting){
		if((options.pagination == "remote" || options.progressiveLoad) && table.modExists("page")){
			table.modules.page.reset(true);
			table.modules.page.setPage(1);
		}else if(options.ajaxProgressiveLoad){
			table.modules.ajax.loadData();
		}else{
			//assume data is url, make ajax call to url to get data
			this._genRemoteRequest();
		}
	}else{
		this.refreshActiveData("sort");
	}

	this.scrollHorizontal(left);
};

RowManager.prototype.scrollHorizontal = function(left){
	this.scrollLeft = left;
	this.element.scrollLeft = left;

	if(this.table.options.groupBy){
		this.table.modules.groupRows.scrollHeaders(left);
	}

	if(this.table.modExists("columnCalcs")){
		this.table.modules.columnCalcs.scrollHorizontal(left);
	}
};

//set active data set
RowManager.prototype.refreshActiveData = function(stage, skipStage, renderInPosition){
	var self = this,
	table = this.table,
	displayIndex;

	if(!stage){
		stage = "all";
	}

	if(table.options.selectable && !table.options.selectablePersistence && table.modExists("selectRow")){
		table.modules.selectRow.deselectRows();
	}

	//cascade through data refresh stages
	switch(stage){
		case "all":

		case "filter":
		if(!skipStage){
			if(table.modExists("filter")){
				self.setActiveRows(table.modules.filter.filter(self.rows));
			}else{
				self.setActiveRows(self.rows.slice(0));
			}
		}else{
			skipStage = false;
		}

		case "sort":
		if(!skipStage){
			if(table.modExists("sort")){
				table.modules.sort.sort();
			}
		}else{
			skipStage = false;
		}

		//generic stage to allow for pipeline trigger after the data manipulation stage
		case "display":
		this.resetDisplayRows();

		case "freeze":
		if(!skipStage){
			if(this.table.modExists("frozenRows")){
				if(table.modules.frozenRows.isFrozen()){
					if(!table.modules.frozenRows.getDisplayIndex()){
						table.modules.frozenRows.setDisplayIndex(this.getNextDisplayIndex());
					}

					displayIndex = table.modules.frozenRows.getDisplayIndex();

					displayIndex = self.setDisplayRows(table.modules.frozenRows.getRows(this.getDisplayRows(displayIndex - 1)), displayIndex);

					if(displayIndex !== true){
						table.modules.frozenRows.setDisplayIndex(displayIndex);
					}
				}
			}
		}else{
			skipStage = false;
		}

		case "group":
		if(!skipStage){
			if(table.options.groupBy && table.modExists("groupRows")){

				if(!table.modules.groupRows.getDisplayIndex()){
					table.modules.groupRows.setDisplayIndex(this.getNextDisplayIndex());
				}

				displayIndex = table.modules.groupRows.getDisplayIndex();

				displayIndex = self.setDisplayRows(table.modules.groupRows.getRows(this.getDisplayRows(displayIndex - 1)), displayIndex);

				if(displayIndex !== true){
					table.modules.groupRows.setDisplayIndex(displayIndex);
				}
			}
		}else{
			skipStage = false;
		}



		case "tree":

		if(!skipStage){
			if(table.options.dataTree && table.modExists("dataTree")){
				if(!table.modules.dataTree.getDisplayIndex()){
					table.modules.dataTree.setDisplayIndex(this.getNextDisplayIndex());
				}

				displayIndex = table.modules.dataTree.getDisplayIndex();

				displayIndex = self.setDisplayRows(table.modules.dataTree.getRows(this.getDisplayRows(displayIndex - 1)), displayIndex);

				if(displayIndex !== true){
					table.modules.dataTree.setDisplayIndex(displayIndex);
				}
			}
		}else{
			skipStage = false;
		}

		if(table.options.pagination && table.modExists("page") && !renderInPosition){
			if(table.modules.page.getMode() == "local"){
				table.modules.page.reset();
			}
		}

		case "page":
		if(!skipStage){
			if(table.options.pagination && table.modExists("page")){

				if(!table.modules.page.getDisplayIndex()){
					table.modules.page.setDisplayIndex(this.getNextDisplayIndex());
				}

				displayIndex = table.modules.page.getDisplayIndex();

				if(table.modules.page.getMode() == "local"){
					table.modules.page.setMaxRows(this.getDisplayRows(displayIndex - 1).length);
				}


				displayIndex = self.setDisplayRows(table.modules.page.getRows(this.getDisplayRows(displayIndex - 1)), displayIndex);

				if(displayIndex !== true){
					table.modules.page.setDisplayIndex(displayIndex);
				}
			}
		}else{
			skipStage = false;
		}
	}


	if(Tabulator.prototype.helpers.elVisible(self.element)){
		if(renderInPosition){
			self.reRenderInPosition();
		}else{
			self.renderTable();
			if(table.options.layoutColumnsOnNewData){
				self.table.columnManager.redraw(true);
			}
		}
	}

	if(table.modExists("columnCalcs")){
		table.modules.columnCalcs.recalc(this.activeRows);
	}
};

RowManager.prototype.setActiveRows = function(activeRows){
	this.activeRows = activeRows;
	this.activeRowsCount = this.activeRows.length;
};

//reset display rows array
RowManager.prototype.resetDisplayRows = function(){
	this.displayRows = [];

	this.displayRows.push(this.activeRows.slice(0));

	this.displayRowsCount = this.displayRows[0].length;

	if(this.table.modExists("frozenRows")){
		this.table.modules.frozenRows.setDisplayIndex(0);
	}

	if(this.table.options.groupBy && this.table.modExists("groupRows")){
		this.table.modules.groupRows.setDisplayIndex(0);
	}

	if(this.table.options.pagination && this.table.modExists("page")){
		this.table.modules.page.setDisplayIndex(0);
	}
};


RowManager.prototype.getNextDisplayIndex = function(){
	return this.displayRows.length;
};

//set display row pipeline data
RowManager.prototype.setDisplayRows = function(displayRows, index){

	var output = true;

	if(index && typeof this.displayRows[index] != "undefined"){
		this.displayRows[index] = displayRows;
		output = true;
	}else{
		this.displayRows.push(displayRows);
		output = index = this.displayRows.length -1;
	}

	if(index == this.displayRows.length -1){
		this.displayRowsCount = this.displayRows[this.displayRows.length -1].length;
	}

	return output;
};

RowManager.prototype.getDisplayRows = function(index){
	if(typeof index == "undefined"){
		return this.displayRows.length ? this.displayRows[this.displayRows.length -1] : [];
	}else{
		return this.displayRows[index] || [];
	}

};

//repeat action accross display rows
RowManager.prototype.displayRowIterator = function(callback){
	this.displayRows.forEach(callback);

	this.displayRowsCount = this.displayRows[this.displayRows.length -1].length;
};

//return only actual rows (not group headers etc)
RowManager.prototype.getRows = function(){
	return this.rows;
};

///////////////// Table Rendering /////////////////

//trigger rerender of table in current position
RowManager.prototype.reRenderInPosition = function(callback){
	if(this.getRenderMode() == "virtual"){

		var scrollTop = this.element.scrollTop;
		var topRow = false;
		var topOffset = false;

		var left = this.scrollLeft;

		var rows = this.getDisplayRows();

		for(var i = this.vDomTop; i <= this.vDomBottom; i++){

			if(rows[i]){
				var diff = scrollTop - rows[i].getElement().offsetTop;

				if(topOffset === false || Math.abs(diff) < topOffset){
					topOffset = diff;
					topRow = i;
				}else{
					break;
				}
			}
		}

		if(callback){
			callback();
		}

		this._virtualRenderFill((topRow === false ? this.displayRowsCount - 1 : topRow), true, topOffset || 0);

		this.scrollHorizontal(left);
	}else{
		this.renderTable();
	}
};

RowManager.prototype.setRenderMode = function(){
	if((this.table.element.clientHeight || this.table.options.height) && this.table.options.virtualDom){
		this.renderMode = "virtual";
	}else{
		this.renderMode = "classic";
	}
};


RowManager.prototype.getRenderMode = function(){
	return this.renderMode;
};

RowManager.prototype.renderTable = function(){
	var self = this;

	self.table.options.renderStarted.call(this.table);

	self.element.scrollTop = 0;

	switch(self.renderMode){
		case "classic":
		self._simpleRender();
		break;

		case "virtual":
		self._virtualRenderFill();
		break;
	}

	if(self.firstRender){
		if(self.displayRowsCount){
			self.firstRender = false;
			self.table.modules.layout.layout();
		}else{
			self.renderEmptyScroll();
		}
	}

	if(self.table.modExists("frozenColumns")){
		self.table.modules.frozenColumns.layout();
	}


	if(!self.displayRowsCount){
		if(self.table.options.placeholder){

			if(this.renderMode){
				self.table.options.placeholder.setAttribute("tabulator-render-mode", this.renderMode);
			}

			self.getElement().appendChild(self.table.options.placeholder);
		}
	}

	self.table.options.renderComplete.call(this.table);
};

//simple render on heightless table
RowManager.prototype._simpleRender = function(){
	var self = this,
	element = this.tableElement;

	self._clearVirtualDom();

	if(self.displayRowsCount){

		var onlyGroupHeaders = true;

		self.getDisplayRows().forEach(function(row, index){
			self.styleRow(row, index);
			element.appendChild(row.getElement());
			row.initialize(true);

			if(row.type !== "group"){
				onlyGroupHeaders = false;
			}
		});

		if(onlyGroupHeaders){
			element.style.minWidth = self.table.columnManager.getWidth() + "px";
		}
	}else{
		self.renderEmptyScroll();
	}
};

//show scrollbars on empty table div
RowManager.prototype.renderEmptyScroll = function(){
	this.tableElement.style.minWidth = this.table.columnManager.getWidth();
	this.tableElement.style.minHeight = "1px";
	// this.tableElement.style.visibility = "hidden";
};

RowManager.prototype._clearVirtualDom = function(){
	var element = this.tableElement;

	if(this.table.options.placeholder && this.table.options.placeholder.parentNode){
		this.table.options.placeholder.parentNode.removeChild(this.table.options.placeholder);
	}

	// element.children.detach();
	while(element.firstChild) element.removeChild(element.firstChild);

	element.style.paddingTop = "";
	element.style.paddingBottom = "";
	element.style.minWidth = "";
	element.style.minHeight = "";
	element.style.visibility = "";

	this.scrollTop = 0;
	this.scrollLeft = 0;
	this.vDomTop = 0;
	this.vDomBottom = 0;
	this.vDomTopPad = 0;
	this.vDomBottomPad = 0;
};

RowManager.prototype.styleRow = function(row, index){
	var rowEl = row.getElement();

	if(index % 2){
		rowEl.classList.add("tabulator-row-even");
		rowEl.classList.remove("tabulator-row-odd");
	}else{
		rowEl.classList.add("tabulator-row-odd");
		rowEl.classList.remove("tabulator-row-even");
	}
};

//full virtual render
RowManager.prototype._virtualRenderFill = function(position, forceMove, offset){
	var self = this,
	element = self.tableElement,
	holder = self.element,
	topPad = 0,
	rowsHeight = 0,
	topPadHeight = 0,
	i = 0,
	onlyGroupHeaders = true,
	rows = self.getDisplayRows();

	position = position || 0;

	offset = offset || 0;

	if(!position){
		self._clearVirtualDom();
	}else{
		// element.children().detach();
		while(element.firstChild) element.removeChild(element.firstChild);

		//check if position is too close to bottom of table
		let heightOccpied = (self.displayRowsCount - position + 1) * self.vDomRowHeight;

		if(heightOccpied < self.height){
			position -= Math.ceil((self.height - heightOccpied) / self.vDomRowHeight);

			if(position < 0){
				position = 0;
			}
		}

		//calculate initial pad
		topPad = Math.min(Math.max(Math.floor(self.vDomWindowBuffer / self.vDomRowHeight),  self.vDomWindowMinMarginRows), position);
		position -= topPad;
	}

	if(self.displayRowsCount && Tabulator.prototype.helpers.elVisible(self.element)){

		self.vDomTop = position;

		self.vDomBottom = position -1;

		while ((rowsHeight <= self.height + self.vDomWindowBuffer || i < self.vDomWindowMinTotalRows) && self.vDomBottom < self.displayRowsCount -1){
			var index = self.vDomBottom + 1,
			row = rows[index];

			self.styleRow(row, index);

			element.appendChild(row.getElement());
			if(!row.initialized){
				row.initialize(true);
			}else{
				if(!row.heightInitialized){
					row.normalizeHeight(true);
				}
			}

			if(i < topPad){
				topPadHeight += row.getHeight();
			}else{
				rowsHeight += row.getHeight();
			}

			if(row.type !== "group"){
				onlyGroupHeaders = false;
			}

			self.vDomBottom ++;
			i++;
		}

		if(!position){
			this.vDomTopPad = 0;
			//adjust rowheight to match average of rendered elements
			self.vDomRowHeight = Math.floor((rowsHeight + topPadHeight) / i);
			self.vDomBottomPad = self.vDomRowHeight * (self.displayRowsCount - self.vDomBottom -1);

			self.vDomScrollHeight = topPadHeight + rowsHeight + self.vDomBottomPad - self.height;
		}else{
			self.vDomTopPad = !forceMove ? self.scrollTop - topPadHeight : (self.vDomRowHeight * this.vDomTop) + offset;
			self.vDomBottomPad = self.vDomBottom == self.displayRowsCount-1 ? 0 : Math.max(self.vDomScrollHeight - self.vDomTopPad - rowsHeight - topPadHeight, 0);
		}

		element.style.paddingTop = self.vDomTopPad + "px";
		element.style.paddingBottom = self.vDomBottomPad + "px";

		if(forceMove){
			this.scrollTop = self.vDomTopPad + (topPadHeight) + offset - (this.element.scrollWidth > this.element.clientWidth ? this.element.offsetHeight - this.element.clientHeight : 0);
		}

		this.scrollTop = Math.min(this.scrollTop, this.element.scrollHeight - this.height);

		//adjust for horizontal scrollbar if present
		if(this.element.scrollWidth > this.element.offsetWidth){
			this.scrollTop += this.element.offsetHeight - this.element.clientHeight;
		}

		this.vDomScrollPosTop = this.scrollTop;
		this.vDomScrollPosBottom = this.scrollTop;

		holder.scrollTop = this.scrollTop;

		element.style.minWidth = onlyGroupHeaders ? self.table.columnManager.getWidth() + "px" : "";

		if(self.table.options.groupBy){
			if(self.table.modules.layout.getMode() != "fitDataFill" && self.displayRowsCount == self.table.modules.groupRows.countGroups()){
				self.tableElement.style.minWidth = self.table.columnManager.getWidth();
			}
		}

	}else{
		this.renderEmptyScroll();
	}
};

//handle vertical scrolling
RowManager.prototype.scrollVertical = function(dir){
	var topDiff = this.scrollTop - this.vDomScrollPosTop;
	var bottomDiff = this.scrollTop - this.vDomScrollPosBottom;
	var margin = this.vDomWindowBuffer * 2;

	if(-topDiff > margin || bottomDiff > margin){
		//if big scroll redraw table;
		var left = this.scrollLeft;
		this._virtualRenderFill(Math.floor((this.element.scrollTop / this.element.scrollHeight) * this.displayRowsCount));
		this.scrollHorizontal(left);
	}else{

		if(dir){
			//scrolling up
			if(topDiff < 0){
				this._addTopRow(-topDiff);
			}

			if(topDiff < 0){

				//hide bottom row if needed
				if(this.vDomScrollHeight - this.scrollTop > this.vDomWindowBuffer){
					this._removeBottomRow(-bottomDiff);
				}
			}
		}else{
			//scrolling down
			if(topDiff >= 0){

				//hide top row if needed
				if(this.scrollTop > this.vDomWindowBuffer){
					this._removeTopRow(topDiff);
				}
			}

			if(bottomDiff >= 0){
				this._addBottomRow(bottomDiff);
			}
		}
	}
};

RowManager.prototype._addTopRow = function(topDiff, i=0){
	var table = this.tableElement,
	rows = this.getDisplayRows();

	if(this.vDomTop){
		let index = this.vDomTop -1,
		topRow = rows[index],
		topRowHeight = topRow.getHeight() || this.vDomRowHeight;

		//hide top row if needed
		if(topDiff >= topRowHeight){
			this.styleRow(topRow, index);
			table.insertBefore(topRow.getElement(), table.firstChild);
			if(!topRow.initialized || !topRow.heightInitialized){
				this.vDomTopNewRows.push(topRow);

				if(!topRow.heightInitialized){
					topRow.clearCellHeight();
				}
			}
			topRow.initialize();

			this.vDomTopPad -= topRowHeight;

			if(this.vDomTopPad < 0){
				this.vDomTopPad = index * this.vDomRowHeight;
			}

			if(!index){
				this.vDomTopPad = 0;
			}

			table.style.paddingTop = this.vDomTopPad + "px";
			this.vDomScrollPosTop -= topRowHeight;
			this.vDomTop--;
		}

		topDiff = -(this.scrollTop - this.vDomScrollPosTop);

		if(i < this.vDomMaxRenderChain && this.vDomTop && topDiff >= (rows[this.vDomTop -1].getHeight() || this.vDomRowHeight)){
			this._addTopRow(topDiff, i+1);
		}else{
			this._quickNormalizeRowHeight(this.vDomTopNewRows);
		}

	}

};

RowManager.prototype._removeTopRow = function(topDiff){
	var table = this.tableElement,
	topRow = this.getDisplayRows()[this.vDomTop],
	topRowHeight = topRow.getHeight() || this.vDomRowHeight;

	if(topDiff >= topRowHeight){

		var rowEl = topRow.getElement();
		rowEl.parentNode.removeChild(rowEl);

		this.vDomTopPad += topRowHeight;
		table.style.paddingTop = this.vDomTopPad + "px";
		this.vDomScrollPosTop += this.vDomTop ? topRowHeight : topRowHeight + this.vDomWindowBuffer;
		this.vDomTop++;

		topDiff = this.scrollTop - this.vDomScrollPosTop;

		this._removeTopRow(topDiff);
	}

};

RowManager.prototype._addBottomRow = function(bottomDiff, i=0){
	var table = this.tableElement,
	rows = this.getDisplayRows();

	if(this.vDomBottom < this.displayRowsCount -1){
		let index = this.vDomBottom + 1,
		bottomRow = rows[index],
		bottomRowHeight = bottomRow.getHeight() || this.vDomRowHeight;

		//hide bottom row if needed
		if(bottomDiff >= bottomRowHeight){
			this.styleRow(bottomRow, index);
			table.appendChild(bottomRow.getElement());

			if(!bottomRow.initialized || !bottomRow.heightInitialized){
				this.vDomBottomNewRows.push(bottomRow);

				if(!bottomRow.heightInitialized){
					bottomRow.clearCellHeight();
				}
			}

			bottomRow.initialize();

			this.vDomBottomPad -= bottomRowHeight;

			if(this.vDomBottomPad < 0 || index == this.displayRowsCount -1){
				this.vDomBottomPad = 0;
			}

			table.style.paddingBottom = this.vDomBottomPad + "px";
			this.vDomScrollPosBottom += bottomRowHeight;
			this.vDomBottom++;
		}

		bottomDiff = this.scrollTop - this.vDomScrollPosBottom;

		if(i < this.vDomMaxRenderChain && this.vDomBottom < this.displayRowsCount -1 && bottomDiff >= (rows[this.vDomBottom + 1].getHeight() || this.vDomRowHeight)){
			this._addBottomRow(bottomDiff, i+1);
		}else{
			this._quickNormalizeRowHeight(this.vDomBottomNewRows);
		}
	}
};

RowManager.prototype._removeBottomRow = function(bottomDiff){
	var table = this.tableElement,
	bottomRow = this.getDisplayRows()[this.vDomBottom],
	bottomRowHeight = bottomRow.getHeight() || this.vDomRowHeight;

	if(bottomDiff >= bottomRowHeight){

		var rowEl = bottomRow.getElement();

		if(rowEl.parentNode){
			rowEl.parentNode.removeChild(rowEl);
		}

		this.vDomBottomPad += bottomRowHeight;

		if(this.vDomBottomPad < 0){
			this.vDomBottomPad = 0;
		}

		table.style.paddingBottom = this.vDomBottomPad + "px";
		this.vDomScrollPosBottom -= bottomRowHeight;
		this.vDomBottom--;

		bottomDiff = -(this.scrollTop - this.vDomScrollPosBottom);

		this._removeBottomRow(bottomDiff);
	}
};

RowManager.prototype._quickNormalizeRowHeight = function(rows){
	rows.forEach(function(row){
		row.calcHeight();
	});

	rows.forEach(function(row){
		row.setCellHeight();
	});

	rows.length = 0;
};

//normalize height of active rows
RowManager.prototype.normalizeHeight = function(){
	this.activeRows.forEach(function(row){
		row.normalizeHeight();
	});
};

//adjust the height of the table holder to fit in the Tabulator element
RowManager.prototype.adjustTableSize = function(){

	if(this.renderMode === "virtual"){
		this.height = this.element.clientHeight;
		this.vDomWindowBuffer = this.table.options.virtualDomBuffer || this.height;

		let otherHeight = this.columnManager.getElement().offsetHeight + (this.table.footerManager && !this.table.footerManager.external ? this.table.footerManager.getElement().offsetHeight : 0);

		this.element.style.minHeight = "calc(100% - " + otherHeight + "px)";
		this.element.style.height = "calc(100% - " + otherHeight + "px)";
		this.element.style.maxHeight = "calc(100% - " + otherHeight + "px)";
	}
};

//renitialize all rows
RowManager.prototype.reinitialize = function(){
	this.rows.forEach(function(row){
		row.reinitialize();
	});
};


//redraw table
RowManager.prototype.redraw = function (force){
	var pos = 0,
	left = this.scrollLeft;

	this.adjustTableSize();

	if(!force){

		if(self.renderMode == "classic"){

			if(self.table.options.groupBy){
				self.refreshActiveData("group", false, false);
			}else{
				this._simpleRender();
			}

		}else{
			this.reRenderInPosition();
			this.scrollHorizontal(left);
		}

		if(!this.displayRowsCount){
			if(this.table.options.placeholder){
				this.getElement().appendChild(this.table.options.placeholder);
			}
		}

	}else{
		this.renderTable();
	}
};

RowManager.prototype.resetScroll = function(){
	this.element.scrollLeft = 0;
	this.element.scrollTop = 0;

	if(this.table.browser === "ie"){
		var event = document.createEvent("Event");
		event.initEvent("scroll", false, true);
		this.element.dispatchEvent(event);
	}else{
		this.element.dispatchEvent(new Event('scroll'));
	}
};
