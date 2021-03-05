var Page = function(table){

	this.table = table; //hold Tabulator object

	this.mode = "local";
	this.progressiveLoad = false;

	this.size = 0;
	this.page = 1;
	this.count = 5;
	this.max = 1;

	this.displayIndex = 0; //index in display pipeline

	this.createElements();
};

Page.prototype.createElements = function(){

	var button;

	this.element = document.createElement("span");
	this.element.classList.add("tabulator-paginator");

	this.pagesElement = document.createElement("span");
	this.pagesElement.classList.add("tabulator-pages");

	button = document.createElement("button");
	button.classList.add("tabulator-page");
	button.setAttribute("type", "button");
	button.setAttribute("role", "button");
	button.setAttribute("aria-label", "");
	button.setAttribute("title", "");

	this.firstBut = button.cloneNode(true);
	this.firstBut.setAttribute("data-page", "first");

	this.prevBut = button.cloneNode(true);
	this.prevBut.setAttribute("data-page", "prev");

	this.nextBut = button.cloneNode(true);
	this.nextBut.setAttribute("data-page", "next");

	this.lastBut = button.cloneNode(true);
	this.lastBut.setAttribute("data-page", "last");
};

//setup pageination
Page.prototype.initialize = function(hidden){
	var self = this;

	//update param names
	for(let key in self.table.options.paginationDataSent){
		self.paginationDataSentNames[key] = self.table.options.paginationDataSent[key];
	}

	for(let key in self.table.options.paginationDataReceived){
		self.paginationDataReceivedNames[key] = self.table.options.paginationDataReceived[key];
	}

	//build pagination element

	//bind localizations
	self.table.modules.localize.bind("pagination|first", function(value){
		self.firstBut.innerHTML = value;
	});

	self.table.modules.localize.bind("pagination|first_title", function(value){
		self.firstBut.setAttribute("aria-label", value);
		self.firstBut.setAttribute("title", value);
	});

	self.table.modules.localize.bind("pagination|prev", function(value){
		self.prevBut.innerHTML = value;
	});

	self.table.modules.localize.bind("pagination|prev_title", function(value){
		self.prevBut.setAttribute("aria-label", value);
		self.prevBut.setAttribute("title", value);
	});

	self.table.modules.localize.bind("pagination|next", function(value){
		self.nextBut.innerHTML = value;
	});

	self.table.modules.localize.bind("pagination|next_title", function(value){
		self.nextBut.setAttribute("aria-label", value);
		self.nextBut.setAttribute("title", value);
	});

	self.table.modules.localize.bind("pagination|last", function(value){
		self.lastBut.innerHTML = value;
	});

	self.table.modules.localize.bind("pagination|last_title", function(value){
		self.lastBut.setAttribute("aria-label", value);
		self.lastBut.setAttribute("title", value);
	});

	//click bindings
	self.firstBut.addEventListener("click", function(){
		self.setPage(1);
	});

	self.prevBut.addEventListener("click", function(){
		self.previousPage();
	});

	self.nextBut.addEventListener("click", function(){
		self.nextPage().then(()=>{}).catch(()=>{});
	});

	self.lastBut.addEventListener("click", function(){
		self.setPage(self.max);
	});

	if(self.table.options.paginationElement){
		self.element = self.table.options.paginationElement;
	}

	//append to DOM
	self.element.appendChild(self.firstBut);
	self.element.appendChild(self.prevBut);
	self.element.appendChild(self.pagesElement);
	self.element.appendChild(self.nextBut);
	self.element.appendChild(self.lastBut);

	if(!self.table.options.paginationElement && !hidden){
		self.table.footerManager.append(self.element, self);
	}

	//set default values
	self.mode = self.table.options.pagination;
	self.size = self.table.options.paginationSize || Math.floor(self.table.rowManager.getElement().clientHeight / 24);
	self.count = self.table.options.paginationButtonCount;
};

Page.prototype.initializeProgressive = function(mode){
	this.initialize(true);
	this.mode = "progressive_" + mode;
	this.progressiveLoad = true;
};

Page.prototype.setDisplayIndex = function(index){
	this.displayIndex = index;
};

Page.prototype.getDisplayIndex = function(){
	return this.displayIndex;
};


//calculate maximum page from number of rows
Page.prototype.setMaxRows = function(rowCount){
	if(!rowCount){
		this.max = 1;
	}else{
		this.max = Math.ceil(rowCount/this.size);
	}

	if(this.page > this.max){
		this.page = this.max;
	}
};

//reset to first page without triggering action
Page.prototype.reset = function(force){
	if(this.mode == "local" || force){
		this.page = 1;
	}
	return true;
};

//set the maxmum page
Page.prototype.setMaxPage = function(max){
	this.max = max || 1;

	if(this.page > this.max){
		this.page = this.max;
		this.trigger();
	}
};

//set current page number
Page.prototype.setPage = function(page){
	return new Promise((resolve, reject)=>{
		if(page > 0 && page <= this.max){
			this.page = page;
			this.trigger()
			.then(()=>{
				resolve();
			})
			.catch(()=>{
				reject();
			});
		}else{
			console.warn("Pagination Error - Requested page is out of range of 1 - " + this.max + ":", page);
			reject();
		}
	});
};

Page.prototype.setPageSize = function(size){
	if(size > 0){
		this.size = size;
	}
};


//setup the pagination buttons
Page.prototype._setPageButtons = function(){
	var self = this;

	let leftSize = Math.floor((this.count-1) / 2);
	let rightSize = Math.ceil((this.count-1) / 2);
	let min = this.max - this.page + leftSize + 1 < this.count ? this.max-this.count+1: Math.max(this.page-leftSize,1);
	let max = this.page <= rightSize? Math.min(this.count, this.max) :Math.min(this.page+rightSize, this.max);

	while(self.pagesElement.firstChild) self.pagesElement.removeChild(self.pagesElement.firstChild);

	if(self.page == 1){
		self.firstBut.disabled = true;
		self.prevBut.disabled = true;
	}else{
		self.firstBut.disabled = false;
		self.prevBut.disabled = false;
	}

	if(self.page == self.max){
		self.lastBut.disabled = true;
		self.nextBut.disabled = true;
	}else{
		self.lastBut.disabled = false;
		self.nextBut.disabled = false;
	}

	for(let i = min; i <= max; i++){
		if(i>0 && i <= self.max){
			self.pagesElement.appendChild(self._generatePageButton(i));
		}
	}

	this.footerRedraw();
};

Page.prototype._generatePageButton = function(page){
	var self = this,
	button = document.createElement("button");

	button.classList.add("tabulator-page");
	if(page == self.page){
		button.classList.add("active");
	}

	button.setAttribute("type", "button");
	button.setAttribute("role", "button");
	button.setAttribute("aria-label", "Show Page " + page);
	button.setAttribute("title", "Show Page " + page);
	button.setAttribute("data-page", page);
	button.textContent = page;

	button.addEventListener("click", function(e){
		self.setPage(page);
	});

	return button;
};

//previous page
Page.prototype.previousPage = function(){
	return new Promise((resolve, reject)=>{
		if(this.page > 1){
			this.page--;
			this.trigger()
			.then(()=>{
				resolve();
			})
			.catch(()=>{
				reject();
			});
		}else{
			console.warn("Pagination Error - Previous page would be less than page 1:", 0);
			reject()
		}
	});
};

//next page
Page.prototype.nextPage = function(){
	return new Promise((resolve, reject)=>{
		if(this.page < this.max){
			this.page++;
			this.trigger()
			.then(()=>{
				resolve();
			})
			.catch(()=>{
				reject();
			});
		}else{
			if(!this.progressiveLoad){
				console.warn("Pagination Error - Next page would be greater than maximum page of " + this.max + ":", this.max + 1);
			}
			reject();
		}
	});
};

//return current page number
Page.prototype.getPage = function(){
	return this.page;
};

//return max page number
Page.prototype.getPageMax = function(){
	return this.max;
};

Page.prototype.getPageSize = function(size){
	return this.size;
};

Page.prototype.getMode = function(){
	return this.mode;
};

//return appropriate rows for current page
Page.prototype.getRows = function(data){
	var output, start, end;

	if(this.mode == "local"){
		output = [];
		start = this.size * (this.page - 1);
		end = start + parseInt(this.size);

		this._setPageButtons();

		for(let i = start; i < end; i++){
			if(data[i]){
				output.push(data[i]);
			}
		}

		return output;
	}else{

		this._setPageButtons();

		return data.slice(0);
	}
};

Page.prototype.trigger = function(){
	var left;

	return new Promise((resolve, reject)=>{

		switch(this.mode){
			case "local":
			left = this.table.rowManager.scrollLeft;

			this.table.rowManager.refreshActiveData("page");
			this.table.rowManager.scrollHorizontal(left);

			this.table.options.pageLoaded.call(this.table, this.getPage());
			resolve();
			break;

			case "remote":
			case "progressive_load":
			case "progressive_scroll":
			this.table.modules.ajax.blockActiveRequest();
			this._getRemotePage()
			.then(()=>{
				resolve();
			})
			.catch(()=>{
				reject();
			});
			break;

			default:
			console.warn("Pagination Error - no such pagination mode:", this.mode);
			reject();
		}
	});
};

Page.prototype._getRemotePage = function(){
	var self = this,
	oldParams, pageParams;


	return new Promise((resolve, reject)=>{

		if(!self.table.modExists("ajax", true)){
			reject()
		}

		//record old params and restore after request has been made
		oldParams = Tabulator.prototype.helpers.deepClone(self.table.modules.ajax.getParams() || {});
		pageParams = self.table.modules.ajax.getParams();

		//configure request params
		pageParams[this.paginationDataSentNames.page] = self.page;

		//set page size if defined
		if(this.size){
			pageParams[this.paginationDataSentNames.size] = this.size;
		}

		//set sort data if defined
		if(this.table.options.ajaxSorting && this.table.modExists("sort")){
			let sorters = self.table.modules.sort.getSort();

			sorters.forEach(function(item){
				delete item.column;
			});

			pageParams[this.paginationDataSentNames.sorters] = sorters;
		}

		//set filter data if defined
		if(this.table.options.ajaxFiltering && this.table.modExists("filter")){
			let filters = self.table.modules.filter.getFilters(true, true);
			pageParams[this.paginationDataSentNames.filters] = filters;
		}

		self.table.modules.ajax.setParams(pageParams);

		self.table.modules.ajax.sendRequest(this.progressiveLoad)
		.then((data)=>{
			self._parseRemoteData(data);
			resolve();
		})
		.catch((e)=>{reject()});

		self.table.modules.ajax.setParams(oldParams);
	});
};



Page.prototype._parseRemoteData = function(data){
	var self = this,
	left, data, margin;

	if(typeof data[this.paginationDataReceivedNames.last_page] === "undefined"){
		console.warn("Remote Pagination Error - Server response missing '" + this.paginationDataReceivedNames.last_page + "' property");
	}

	if(data[this.paginationDataReceivedNames.data]){
		this.max = parseInt(data[this.paginationDataReceivedNames.last_page]) || 1;

		if(this.progressiveLoad){
			switch(this.mode){
				case "progressive_load":
				this.table.rowManager.addRows(data[this.paginationDataReceivedNames.data]);
				if(this.page < this.max){
					setTimeout(function(){
						self.nextPage().then(()=>{}).catch(()=>{});
					}, self.table.options.ajaxProgressiveLoadDelay);
				}
				break;

				case "progressive_scroll":
				data = this.table.rowManager.getData().concat(data[this.paginationDataReceivedNames.data]);

				this.table.rowManager.setData(data, true);

				margin = this.table.options.ajaxProgressiveLoadScrollMargin || (this.table.rowManager.element.clientHeight * 2);

				if(self.table.rowManager.element.scrollHeight <= (self.table.rowManager.element.clientHeight + margin)){
					self.nextPage().then(()=>{}).catch(()=>{});
				}
				break;
			}
		}else{
			left = this.table.rowManager.scrollLeft;

			this.table.rowManager.setData(data[this.paginationDataReceivedNames.data]);

			this.table.rowManager.scrollHorizontal(left);

			this.table.columnManager.scrollHorizontal(left);

			this.table.options.pageLoaded.call(this.table, this.getPage());
		}

	}else{
		console.warn("Remote Pagination Error - Server response missing '" + this.paginationDataReceivedNames.data + "' property");
	}

};




//handle the footer element being redrawn
Page.prototype.footerRedraw = function(){
	var footer = this.table.footerManager.element;

	if((Math.ceil(footer.clientWidth) - footer.scrollWidth) < 0){
		this.pagesElement.style.display = 'none';
	}else{
		this.pagesElement.style.display = '';

		if((Math.ceil(footer.clientWidth) - footer.scrollWidth) < 0){
			this.pagesElement.style.display = 'none';
		}
	}
};

//set the paramter names for pagination requests
Page.prototype.paginationDataSentNames = {
	"page":"page",
	"size":"size",
	"sorters":"sorters",
	// "sort_dir":"sort_dir",
	"filters":"filters",
	// "filter_value":"filter_value",
	// "filter_type":"filter_type",
};

//set the property names for pagination responses
Page.prototype.paginationDataReceivedNames = {
	"current_page":"current_page",
	"last_page":"last_page",
	"data":"data",
};

Tabulator.prototype.registerModule("page", Page);
