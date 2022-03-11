

class Commons {


	constructor() {
		this.eleResult;
		this.eleBtnNext;
		this.eleBtnCommit;
	}
	

	initCommonEles = () => {
		this.eleResult = document.getElementsByClassName('result')[0];
		this.eleBtnNext = document.getElementById('btn-next');
		this.eleBtnCommit = document.getElementById('btn-commit');
		this.eleBtnNext.disabled = true;
	}

	setCommitButtonClickEvent(eventFn) {
		this.eleBtnCommit.addEventListener('click', (e)=> {
			eventFn(e);
		});
	}

	setNextButtonClickEvent(eventFn) {
		this.eleBtnNext.addEventListener('click', (e)=> {
			eventFn(e);
		});

	}

	showResult = (success, message) => {
		this.eleResult.innerHTML = message;
		this.eleResult.className = (this.eleResult.className + '').replace(/(fail)|(success)/ig, '');
		this.eleResult.className += success ? ' success' : ' fail'; 
		this.eleBtnNext.disabled = !success;
		
	}


	hideLoading() {
		document.getElementsByClassName("layout-loading")[0].style.display = "none"
	}

	showLoading() {
		console.log(document.getElementsByClassName("layout-loading")[0])
		this.changeLoadingMessage("Loading...", false);
		document.getElementById("wifi-loading");
		document.getElementById("wifi-loading").style.display = "block"
	}

	changeLoadingMessage(message, isError)  {
		var eleTextLoading = document.getElementById("text-loading");
		eleTextLoading.style.color = isError ? "red" : "white";
		eleTextLoading.innerHTML = `<div>${message}</div>`
	};


	showConnectionError() {
		let errorMsg = 'Unable to connect to the selected wifi or check your wifi connection.';
		alert(errorMsg);
		this.showResult(false, errorMsg);
	}



}


class Client  {

	static _retry  = 0;
	static MAX_RETRY = 3;

    static scanWifi(result) {
        this._retry = 0;
		let wifiList = [];
        let count = 0;
        ajax({
            url: `${DEV_URL}/api/wifi/scan/count`,
            complete: function(res) {
                count = res.data.count;
                Client._readWifiItem(count, 0, wifiList, result);
            },
            error: function(e) {
				console.log(e);
                if (this._retry >= MAX_RETRY) {
					result(false, undefined, e);
					return;
                }
				++this._retry;
                Client.scanWifi(result)
            }
        });
    }

    static _readWifiItem(max, currentCount, wifiList, result) {
        ajax({
            url: `${DEV_URL}/api/wifi/scan/item?count=${currentCount}`,
            complete: function(res) {
                ++currentCount;
                if (max <= currentCount) {
                    result(true,wifiList);
                    return
                }
                let isDuple = false; 
                for(let data of wifiList) {
                    if(data.ssid == res.data.ssid) {						
                        isDuple = true;
                    }
                }
                if(!isDuple)	{
                    wifiList.push(res.data);
                }
                Client._readWifiItem(max, currentCount,wifiList, result)
            },
            error: function(e) {
				console.log(e);
                if (this._retry > MAX_RETRY) {
					result(false, undefined, e);
					return;
                }
				++this._retry
                Client.scanWifi(result);
            }
        });
    }

	static connectWifi(ssid, password, result) {
		ajax({
			type: "POST",
			url: `${DEV_URL}/api/wifi/connect`,
			data: {
				ssid: ssid ? ssid : '',
				password: password ? password : ''
			},
			complete: function(res) {
				result(res.data.success, res.data, undefined);
			},
			error: function(e) {
				console.error(e);
				result(false, undefined, e);
			}
		})
	}

	static getTimeConfig(result)  {
		ajax({
			url: `${DEV_URL}/api/ntp/info`,
			complete: function(res) {
				result(res.data.success, res.data, undefined);
			},
			error: function(e) {
				console.error(e);
				result(false, undefined, e);
			}
		});
	}

	static setTimeConfig(ntp, interval, offset, result) {
		ajax({
			url: `${DEV_URL}/api/ntp/set`,
			type: 'POST',
			data: {
				ntp: ntp,
				interval: interval,
				offset: offset
			},
			complete: function(res) {
				let data = res.data;
				result(data.success, data);
			},
			error: function(e) {
				console.error(e);
				result(false, undefined, e);
			}
		});
	}

	static getMqttConfig(result) {
		ajax({
			type: 'GET',
			url: `${DEV_URL}/api/mqtt/info`,
			complete: function(res) {
				result(true, res.data, undefined);
			},
			error: function(e) {
				result(false, undefined, e);
			}
		});
	}

	static setMqttConfig(address, port, clientID, user,pass, result) {
		ajax({
			type: 'POST',
			data: {
				url: address,
				port: port,
				mid: clientID,
				muser: user,
				mpass: pass
			},
			url: `${DEV_URL}/api/mqtt/connect`,
			complete: function(res) {
				result(res.data.success, res.data, undefined)
			},
			error: function(e) {
				result(false, undefined, e);
			}
		});
	}

	
	static getOptionList(result) {
		let optionList = [];
		ajax({
			type: 'GET',
			url: `${DEV_URL}/api/option/count`,
			complete: function(res) {
				let optionCount = res.data.cnt; 
				if(optionCount == 0) {
					result(true, optionList,undefined);
					return;
				}
				Client._loadOption(optionCount, optionCount, optionList, result);
			},
			error: function(e) {
				console.error(e);
				result(false, undefined, e);
			}
		});
	}

	
	
	static _loadOption(optionCount, leftCount, optionList, result) {
		ajax({
			type: 'GET',
			url: `${DEV_URL}/api/option/get`,
			complete: function(res) {
				optionList.push(res.data); 
				if(--leftCount > 0) {
					Client._loadOption(optionCount, leftCount, optionList, result);
				} else {
					
					result(true, optionList,undefined);
				}
			},
			error: function(e) {
				console.error(e);
				loadOptionCount(result);
			}
		});
	}

	/**
	 * 
	 * @param {string} name 
	 * @param {string} value 
	 * @param {(boolean, message, error)} resultEvent(isSuccess, error message, error object)
	 */
	static updateOption(name, value, result) {
		ajax({
			type: 'POST',
			data: {name: name, value: value },
			url: `${DEV_URL}/api/option/set`,
			complete: function(res) {
				if(!res.data.success) {
					console.log(res.data.msg);
					result(false, res.data.msg);
				} else {
					result(true, '');
				}
			}, 
			error: function(e) {
				console.error(e);
				result(true, undefined, e);
			}
		});
	}

	static getDeviceInfo(result) {
		ajax({
			type: 'GET',
			url: `${DEV_URL}/api/info`,
			complete: function(res) {
				console.log(res.data)
				result(true, res.data, undefined);
			},
			error: function(e) {
				result(false, undefined, e);
			}
		});
	}

	static commit(result) {
		ajax({
			type: 'GET',
			url: `${DEV_URL}/api/commit`,
			


			complete: function(res) {
				result(res.data.success, res.data, undefined);
			},
			error: function(e) {
				result(false, undefined, e);
			}
		});
	}
	
}




let WifiConfig = new function() {

    let _wifiList = [];
	let _selectedWifi = {};
	let _commons = new Commons();
	let _timeoutInterval = -1;
	let _connectTimeout = 60;
	this.init = ()=> {
		_commons.initCommonEles();
		setEvents();
		scanWifi();

	}

	function setEvents() {
		_commons.setCommitButtonClickEvent(()=> { connectWifi(); });
		_commons.setNextButtonClickEvent(() => {
			location.href = "time.html";
		});
	}

	function map(x, in_min, in_max, out_min, out_max) {
		return Math.round((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min)
	}

	function rssiToSignalTag(rssi) {
		rssi = rssi > -65 ? -65 : rssi < -95 ? -95 : rssi;
		let signal = map(rssi, -95, -65, 0, 4);
		let signalTag = '';
		signalTag = `<ul class="signal-strength"><li class="very-weak"><div class="${signal > 0 ? 'sig-' +signal : 'sig-0'}" ></div></li><li class="weak"><div class="${signal > 1 ? 'sig-' +signal : 'sig-0'}" ></div></li><li class="strong"><div class="${signal > 2 ? 'sig-' +signal : 'sig-0'}"></div></li><li class="pretty-strong"><div class="${signal > 3 ? 'sig-' +signal : 'sig-0'}"></div></li></ul>`;
		return signalTag
	}

	function onSelectedWifiItem(ev) {
		let classes = document.getElementsByClassName('wifi-item');
		for (let idx = 0, n = classes.length - 1; idx <= n; ++idx) {
			classes.item(idx).className = `wifi-item ${idx == n ? 'bottom' : idx == 0 ? 'top' : '' }`
		}
		let target = ev.target;
		while (!target.className.includes('wifi-item')) {
			target = target.parentNode;
			if (target.className.includes('wifi-list')) {
				return
			}
		}
		_selectedWifi = _wifiList[target.id.replace('wifi-item', '')];
		let passwordTextEle = document.getElementById('wifi-passwd');
		let ssidTextEle = document.getElementById('wifi-ssid');
		ssidTextEle.value = _selectedWifi.ssid;
		passwordTextEle.value = "";
		if(_selectedWifi.type == 'None' || _selectedWifi.type == 'Auto') {
			passwordTextEle.disabled = true;
		} else {
			passwordTextEle.disabled = false;
		}
		target.className += ' select'
	}

	function showWifiList(list) {
		let wifiListEle = document.getElementById('wifi-list');
		let tag = '';
		for (let idx = 0, n = list.length - 1; idx <= n; ++idx) {
			tag += `<div id="wifi-item${idx}" class="wifi-item ${idx == n ? 'bottom' : idx == 0 ? 'top' : '' }"><div class="wifi-rssi" >` + rssiToSignalTag(list[idx].rssi) + '</div><div class="item-text"><div class="wifi-ssid">' + list[idx].ssid + '</div><div class="wifi-type">(' + list[idx].type + ')</div></div></div>'
		}
		wifiListEle.innerHTML = tag;
		let classes = document.getElementsByClassName('wifi-item');
		for (let idx = 0; idx < classes.length; ++idx) {
			classes.item(idx).addEventListener("click", onSelectedWifiItem, !1)
		}
	}


	function scanWifi() {
		_commons.showLoading();
		_wifiList = [];
		Client.scanWifi((isSuccess,wifiList) => {
			if(!isSuccess) {
				_commons.changeLoadingMessage(`Check your device wifi connection.`, false);
				return;
			}
			_commons.hideLoading();
			_wifiList = wifiList;
			showWifiList(_wifiList);
		});
	}
	

	function endTimeoutInterval() {
		if(_timeoutInterval > -1) {
			clearInterval(_timeoutInterval);
			_timeoutInterval = -1;
		}
		_commons.changeLoadingMessage(`Loading...`);
	}


	function connectWifi() {
		_commons.showLoading();
		
		if(_timeoutInterval > -1) {
			clearInterval(_timeoutInterval);
		}
		_connectTimeout = 60;
		_timeoutInterval = setInterval(()=> {
			_commons.changeLoadingMessage(`Connecting...  <span style="font-size: 15pt">( ${--_connectTimeout} )</span>`);
			if(_connectTimeout < 1) {
				_commons.changeLoadingMessage(`Connecting...<span style="font-size: 15pt">( pending )</span>`);	
				endTimeoutInterval();
			}
		}, 1090);
		
		let ssidTextEle = document.getElementById('wifi-ssid');
		let passwordTextEle = document.getElementById('wifi-passwd');
		Client.connectWifi(ssidTextEle.value,passwordTextEle.value, (isSuccess, data) => {
			if(data) {
				endTimeoutInterval();
				_commons.hideLoading();
				if (isSuccess == true) {
					_commons.showResult(true,`Ok. Connected. (${data.ip})`);
				} else {
					_commons.showResult(false,'Unable to connect to the selected wifi.');
				}
			} else {
				setTimeout(()=> {
					Client.getDeviceInfo((success, data) => {
						endTimeoutInterval();
						_commons.hideLoading();
						if(!success || data.ssid != ssidTextEle.value) {
							_commons.showResult(false, 'Unable to connect to the selected wifi or check your wifi connection.')
						} else {
							_commons.showResult(true,`Ok. Connected. (${data.ip})`);
						}
					});
				},3000);
			}
		});
	}

	

};



let TimeConfig = new function() {
	
	let _defInfo = {};
	let _eleInputNtp;
	let _eleInputInterval;
	let _eleSelectUtc;
	let _eleInputManuallyUtc;
	let _eleBlockTimeOffset;
	let _intervalID;
	let _commons = new Commons();

	/** Date **/
	let _date;

	this.init = () => {
		initEleValue();
		putUTCOptions();
		setEvents();
		loadConfig();
	}

	function initEleValue() {
		_eleInputNtp = document.getElementById('input-ntp');
		_eleInputInterval = document.getElementById('input-interval');
		_eleSelectUtc = document.getElementById('select-utc');
		_eleInputManuallyUtc = document.getElementById('input-manually-utc');
		_eleBlockTimeOffset = document.getElementById('block-timeoffset');
		_commons.initCommonEles();
	}

	function putUTCOptions() {
		let tag = '';
		for(let i = -12; i < 13; ++i) {
			tag += `<option value="${i * 60 * 60}">UTC${i > 0 ? '+' : i == 0 ? ' ' : ''}${i}:00</option>`
		}
		tag += `<option value="manually">manually</option>`
		let selectUtc = _eleSelectUtc;
		selectUtc.innerHTML = tag;
	}


	function onChangeSelectTimezone(e) {
		let value = e.target.value;
		if(value == 'manually') {
			_eleBlockTimeOffset.style = 'display: '
		} else {
			_eleBlockTimeOffset.style = 'display: none'
		}
	}

	function setEvents() {
		_eleSelectUtc.addEventListener('change',onChangeSelectTimezone);
		_commons.setCommitButtonClickEvent(()=>  { setConfig();});
		_commons.setNextButtonClickEvent(() => {
			location.href = "mqtt.html";
		});
	}

	function loadConfig()  {
		Client.getTimeConfig((success, data, error) => {
			
			if(data) {
				_commons.hideLoading();
				_defInfo = data;
				console.log('--');
				console.log(_defInfo.ntp);
				_eleInputNtp.value = _defInfo.ntp;
				_eleInputInterval.value = _defInfo.interval;

				let timeOffset = _defInfo.offset;
				if(timeOffset % 3600 != 0) {
					_eleSelectUtc.value = 'manually';
					_eleInputManuallyUtc.value = timeOffset;

				} else {
					_eleSelectUtc.value = timeOffset;
					_eleInputManuallyUtc.value = 0;
				}
			} else {
				if(e.status == 404) {
					_commons.showConnectionError();
				}
				loadConfig();
			}
		});
	}

	function showCurrentTime() {
		_date.setSeconds(_date.getSeconds() + 1);
		let hours = _date.getHours();
		let min = _date.getMinutes();
		let sec = _date.getSeconds();
		_commons.eleResult.innerHTML = `Success - ${hours < 10 ? '0' : ''}${hours}:${min < 10 ? '0' : ''}${min}:${sec < 10 ? '0' : ''}${sec}`

	}

	function startClock() {		
		_commons.showResult(true, '');
		showCurrentTime();
		_intervalID = setInterval(()=> {
			showCurrentTime();
		}, 1000);
	}




	function setConfig()  {
		_commons.showLoading();
		_commons.eleResult.innerHTML = '';
		if(_intervalID) {
			clearInterval(_intervalID);
		}
		let timeOffset = _eleSelectUtc.value;
		console.log(timeOffset)

		if(timeOffset == 'manually') {
			timeOffset = _eleInputManuallyUtc.value;
		}

		if(_eleInputNtp.value.trim() == '') {
			_eleInputNtp.value = _defInfo.ntp;
		}
		if(_eleInputInterval.value < 1) {
			_eleInputInterval.value = _defInfo.interval;
		} 
		if(timeOffset < -86400 || timeOffset > 86400) {
			timeOffset = 0;
			_eleSelectUtc.value = 0;
			_eleInputManuallyUtc.value = 0;
			onChangeSelectTimezone({target: _eleInputManuallyUtc});
		}

		Client.setTimeConfig(_eleInputNtp.value,_eleInputInterval.value,timeOffset, (success, data, e) => {
			console.log(data)
			if(success == true && data) {
				_commons.hideLoading();
				_date = new Date();
				_date.setHours(data.h,data.m,data.s);
				startClock();
			} else {
				_commons.hideLoading();
				if(e && e.status == 404) {
					_commons.showConnectionError();
					return;
				} 
				_commons.showResult(false, `Fail...<br/>All values ​​are initialized.<br/>please try again.`);
				loadConfig();
			}
		} );
	}

}


let MqttConfig = new function() {
	let _eleInputAddress;
	let _eleInputPort;
	let _eleInputClientID;
	let _eleInputUser;
	let _eleInputPassword;
	let _commons = new Commons();


	this.init = () => {
		initEleValue();
		loadInfo();
		setEvents();
	}


	
	function setEvents() {
		_commons.setCommitButtonClickEvent(()=>  { setConfig();});
		_commons.setNextButtonClickEvent(() => {
			location.href = "option.html";
		});
	}
	  
	function initEleValue() {
		_eleInputAddress = document.getElementById('input-mqtt-addr');
		_eleInputPort = document.getElementById('input-mqtt-port');
		_eleInputClientID = document.getElementById('input-mqtt-clientid');
		_eleInputUser = document.getElementById('input-mqtt-user');
		_eleInputPassword = document.getElementById('input-mqtt-pass');
		_commons.initCommonEles();
	}


	function setConfig() {
		if (_eleInputAddress.value === null || _eleInputAddress.value === '') {
			_commons.showResult(false, 'Address is empty');
			return;
		} else if (_eleInputPort.value === null || _eleInputPort.value > 65353 || _eleInputPort.value < 1) {
			_commons.showResult(false, 'Invalid port number.');
			return;
		}
		else if (_eleInputClientID.value === null ||_eleInputClientID.value == '') {
			_commons.showResult(false, 'ClientID is empty');
			return;
		}
		_commons.showLoading();
		Client.setMqttConfig(_eleInputAddress.value,_eleInputPort.value,_eleInputClientID.value,_eleInputUser.value,_eleInputPassword.value,
			(success, data, error) => {
				if (success === true) {
					_commons.showResult(true,'Ok. Connected.');
				} 
				else if(error) {
					_commons.showConnectionError();
					_commons.hideLoading();
				}
				else {
					_commons.showResult(false,'Can not connect to MQTT server.');
				}
				_commons.hideLoading();

			});
	}


	function loadInfo() {
		_commons.showLoading();
		Client.getMqttConfig((success, data, error) => {
			if(success) {
				_eleInputAddress.value = data.url;
				_eleInputPort.value = data.port + '';
				_eleInputClientID.value = data.mid;
				_eleInputUser.value = data.muser + '';
				_eleInputPassword.value = data.mpass + '';
				_commons.hideLoading();
			} else {
				if(error && e.status == 404) {
					_commons.showConnectionError();
				}   
				loadInfo();
			}
		});
	}

	
}



let OptionConfig = new function() {
	
	let _eleOptions;
	let _optionList = [];
	let _uploadedOptionCount = -1;
	let _isSuccess = true;
	let _commons = new Commons();

	this.init = () => {
		initEleValue();
		loadOption();
		setEvents();
	}

	function setEvents() {
		_commons.setCommitButtonClickEvent(() => {
			setOption();
		});
		_commons.setNextButtonClickEvent(() => {
			location.href = "finish.html";
		});
	}

	  
	function initEleValue() {
		_eleOptions = document.getElementById('options');
		_commons.initCommonEles();
	}


	function setOption() {
		_commons.showLoading();
		_isSuccess = true; 
		_uploadedOptionCount = _optionList.length;
		let labels = document.getElementsByClassName('label-option-name');
		let inputValueEles = document.getElementsByClassName('input-option-value');
		let errorMsgEles = document.getElementsByClassName('error-msg');
		for(let i = 0; i < inputValueEles.length; ++i) {
			errorMsgEles[i].textContent = '';
			labels[i].style.color = 'black';   
			if(!isNull(inputValueEles[i].name) && inputValueEles[i].value == '') {
				errorMsgEles[i].textContent = 'Empty values ​​are not allowed.';
				labels[i].style.color = 'red';
				--_uploadedOptionCount;
				_isSuccess = false; 
				continue;
			}
			uploadOption(inputValueEles[i].name,inputValueEles[i].value,labels[i],errorMsgEles[i]);
		}


	}

	function uploadOption(name, value,lebelEle, eleResultMsg) {

		Client.updateOption(name, value, (success, message, error) => {
			try {
				console.log(message)

				if(message && message != '') {
					eleResultMsg.textContent = (message ? message : 'Invalid value.');
					lebelEle.style.color = 'red';
					_isSuccess = false; 
				} else if(!success) {
					console.log('쉴패!!')
					console.log(message)
					console.log(error)
					if(error && error.status == 404) {
						_commons.showConnectionError();
					}
					_isSuccess = false; 
				}

				if(--_uploadedOptionCount == 0) {				
					_commons.hideLoading();
					if(_isSuccess) _commons.showResult(true, 'Options applied.');
					else _commons.showResult(false, 'Invalid option value.');
				} 
			} catch(e) {
				console.error(e);
			}
		});
	}

	function isNull(name) {
		for(let i = 0; i < _optionList.length; ++i) {
			if(_optionList[i].name == name && _optionList[i].isNull) {
				return true;
			}
		}
		return false; 
	}


	
	function loadOption() {
		_commons.showLoading();
		
		Client.getOptionList((success, list, error) => {
			if(success == true) {
				_optionList = list;
				renderOptionForms();
				_commons.hideLoading();
			} else {
				_commons.showConnectionError();
			}
		});
	}

	function renderOptionForms() {
		if(_optionList.length == 0) {
			_commons.showResult(true,'No options.');
			_commons.eleResult.style.setProperty('color','#ccc');
			_commons.eleResult.style.setProperty('font-size', '28pt');
			_commons.eleResult.style.setProperty('margin', '100px 10px 100px 10px','important');
			_commons.eleResult.style.setProperty('text-align', 'center');
			_commons.eleBtnCommit.disabled = true;
			return;
		}
		let tag = '';
		for(let i = 0; i < _optionList.length; ++i) {
			let option =  _optionList[i];
			tag += `<div class='form'><span class='label-option-name'>${option.name}${!isNull(option.name) ? '*' : ''}: </span><input type='text' class='input-option-value' maxlength='32' name='${option.name}' value='${option.value}' /></div>`
			tag += `<div class='error-msg'  ></div>`;
		}
		_eleOptions.innerHTML = tag;        
		let maxWidth = 0;
		let labels = document.getElementsByClassName('label-option-name');
		let inputValues = document.getElementsByClassName('input-option-value');  
		let errorMsgs = document.getElementsByClassName('error-msg');  
		for(let i = 0; i < labels.length; ++i) {
			let eleLabel = labels[i];
			maxWidth = Math.max(eleLabel.offsetWidth,maxWidth);
		}
		if(maxWidth == 0) {
			return;
		}
		else if(maxWidth > 210) {
			maxWidth = 210;
		}
		for(let i = 0; i < labels.length; ++i) {
			labels[i].style.width = maxWidth + 'px';
			errorMsgs[i].style.margin = `2px 0 -3px ${maxWidth + 5}px`
			inputValues[i].style.width = (280 - maxWidth) + 'px';
			errorMsgs[i].style.width = (300 - maxWidth) + 'px';
		}
	}


	
}


let FinishView = new function() {
	let _eleConfigInfo;
	let _tag = '';
	let _commons = new Commons();

	this.init = () => {
		_tag = '';
		_eleConfigInfo = document.getElementById('config-info');
		_commons.initCommonEles();
		_commons.showLoading();
		_commons.setCommitButtonClickEvent(() => {
			_commons.showLoading();
			Client.commit((success) => {
				_commons.hideLoading();
				if(success) {
					alert('Configuration complete. Restart your device.');
					location.href = 'about:blank';
				}
				else {
					alert('Error. Failed to save configuration values.');
				} 
			});
			
			
		});
		loadConfigInfo();

	}

	function loadConfigInfo() {
		Client.getDeviceInfo((success, data) => {
			console.log(data);
			_tag += `<div class='info-line'><span class="config-name">Device name :</span><span class="config-value">${data.device}</span></div>`;
			_tag += `<div class='info-line'><span class="config-name">Version :</span><span class="config-value">${data.version}</span></div>`;
			_tag += `<br/>`
			_tag += `<div class='info-line'><span class="config-name">SSID :</span><span class="config-value">${data.ssid}</span></div>`;
			_tag += `<div class='info-line'><span class="config-name">IP :</span><span class="config-value">${data.ip}</span></div>`;
			_tag += `<br/>`
			renderInfo();
			loadTimeInfo();
		});						
	}

	function _toTwoDigit(num) {
		return num < 10 ? '0' + num : num;
	}


	function loadTimeInfo() {
		Client.getTimeConfig((success, data) => {
			console.log(data);
			_tag += `<div class='info-line'><span class="config-name">Device name :</span><span class="config-value">${data.ntp}</span></div>`;
			_tag += `<div class='info-line'><span class="config-name">Version :</span><span class="config-value">${data.interval} min</span></div>`;
			_tag += `<div class='info-line'><span class="config-name">Time zone :</span><span class="config-value">UTC${data.offset > 0 ? '+' : data.offset < 0 ? '-' : ' ' }${_toTwoDigit(Math.abs(data.offset) / 3600) }:${_toTwoDigit(Math.abs(data.offset) % 3600)}</span></div>`;
			_tag += `<br/>`
			loadMqttInfo();
		});						
	}

	function loadMqttInfo() {
		Client.getMqttConfig((success, data) => {
			console.log(data);
			_tag += `<div class='info-line'><span class="config-name">Address :</span><span class="config-value">${data.url}</span></div>`;
			_tag += `<div class='info-line'><span class="config-name">Port :</span><span class="config-value">${data.port}</span></div>`;
			_tag += `<div class='info-line'><span class="config-name">Client ID :</span><span class="config-value">${data.port}</span></div>`;
			if(data.muser != '') {
				_tag += `<div class='info-line'><span class="config-name">User :</span><span class="config-value">${data.muser}</span></div>`;
			}
			if(data.mpass != '') {
				_tag += `<div class='info-line'><span class="config-name">Password :</span><span class="config-value">${data.mpass}</span></div>`;
			}
			_tag += `<br/>`;
			loadOptions();
		});						
	}

	function loadOptions() {
		Client.getOptionList((success, list) => {
			console.log(list)
			for(let i = 0; i < list.length; ++i) {
				console.log(list[i]);
				_tag += `<div class='info-line'><span class="config-name">${list[i].name} :</span><span class="config-value">${list[i].value}</span></div>`;
			}
			renderInfo();
			_commons.hideLoading();
		});						
	}

	function renderInfo() {
		console.log(_eleConfigInfo)
		_eleConfigInfo.innerHTML = _tag;

	}

}
