<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<html xmlns:v>
<head>
<meta http-equiv="X-UA-Compatible" content="IE=Edge"/>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta HTTP-EQUIV="Pragma" CONTENT="no-cache">
<meta HTTP-EQUIV="Expires" CONTENT="-1">
<link rel="shortcut icon" href="images/favicon.png">
<link rel="icon" href="images/favicon.png">
<title><#Web_Title#> - <#vpnc_title#></title>
<link rel="stylesheet" type="text/css" href="index_style.css"> 
<link rel="stylesheet" type="text/css" href="form_style.css">
<link rel="stylesheet" type="text/css" href="usp_style.css">
<script type="text/javascript" src="/state.js"></script>
<script type="text/javascript" src="/popup.js"></script>
<script type="text/javascript" src="/help.js"></script>
<script type="text/javascript" src="/js/jquery.js"></script>
<script type="text/javascript" src="/general.js"></script>
<script type="text/javascript" src="/validator.js"></script>
<script type="text/javascript" src="/switcherplugin/jquery.iphone-switch.js"></script>
<style type="text/css">
.contentM_qis{
	position:absolute;
	-webkit-border-radius: 5px;
	-moz-border-radius: 5px;
	border-radius: 5px;
	z-index:200;
	display:none;
	margin-left: 30%;
	top: 290px;
	width:650px;
}
.contentM_qis_manual{
	position:absolute;
	-webkit-border-radius: 5px;
	-moz-border-radius: 5px;
	border-radius: 5px;
	z-index:200;
	background-color:#2B373B;
	margin-left: -30px;
	margin-left: -100px \9; 
	margin-top:-400px;
	width:740px;
	box-shadow: 3px 3px 10px #000;
}

.QISform_wireless{
	width:600px;
	font-size:12px;
	color:#FFFFFF;
	margin-top:10px;
	*margin-left:10px;
}

.QISform_wireless thead{
	font-size:15px;
	line-height:20px;
	color:#FFFFFF;	
}

.QISform_wireless th{
	padding-left:10px;
	*padding-left:30px;
	font-size:12px;
	font-weight:bolder;
	color: #FFFFFF;
	text-align:left; 
}

.description_down{
	margin-top:10px;
	margin-left:10px;
	padding-left:5px;
	font-weight:bold;
	line-height:140%;
	color:#ffffff;	
}
.vpnc_ipconflict_icon {
	background-image: url(/images/New_ui/notification.png);
	background-repeat: no-repeat;
	height: 25px;
	width: 25px;
	margin: auto;
}
</style>
<script>

var vpnc_clientlist_array = [];
var vpnc_pptp_options_x_list_array = [];
var restart_vpncall_flag = 0; //Viz add 2014.04 for Edit Connecting rule then restart_vpncall

function parseNvramToArray(_oriNvram) {
	var parseArray = [];
	var oriNvramRow = decodeURIComponent(_oriNvram).split('<');
	for(var i = 0; i < oriNvramRow.length; i += 1) {
		if(oriNvramRow[i] != "") {
			var oriNvramCol = oriNvramRow[i].split('>');
			var eachRuleArray = new Array();
			for(var j = 0; j < oriNvramCol.length; j += 1) {
				eachRuleArray.push(oriNvramCol[j]);
			}
			parseArray.push(eachRuleArray);
		}
	}
	return parseArray;
}

function initial(){
	show_menu();
	vpnc_clientlist_array = parseNvramToArray('<% nvram_char_to_ascii("","vpnc_clientlist"); %>');
	vpnc_pptp_options_x_list_array = parseNvramToArray('<% nvram_char_to_ascii("","vpnc_pptp_options_x_list"); %>');
	show_vpnc_rulelist();
}

var add_profile_flag = false;
function Add_profile(){
	add_profile_flag = true;
	gen_vpnc_tab_list("pptp");
	gen_vpnc_tab_list("openvpn");
	document.getElementById('importOvpnFile').style.display = "none";
	document.form.vpnc_des_edit.value = "";
	document.form.vpnc_svr_edit.value = "";
	document.form.vpnc_account_edit.value = "";
	document.form.vpnc_pwd_edit.value = "";
	document.form.selPPTPOption.value = "auto";	
	document.getElementById("pptpOptionHint").style.display = "none";
	document.vpnclientForm.vpnc_openvpn_des.value = "";
	document.vpnclientForm.vpnc_openvpn_username.value = "";
	document.vpnclientForm.vpnc_openvpn_pwd.value = "";	
	document.vpnclientForm.file.value = "";
	document.openvpnCAForm.file.value = "";
	document.getElementById("cbManualImport").checked = false;
	document.getElementById('edit_vpn_crt_client_ca').value = "";
	document.getElementById('edit_vpn_crt_client_crt').value = "";
	document.getElementById('edit_vpn_crt_client_key').value = "";
	document.getElementById('edit_vpn_crt_client_static').value = "";
	document.getElementById('edit_vpn_crt_client_crl').value = "";
	manualImport(false);
	tabclickhandler(0);
	$("#openvpnc_setting").fadeIn(300);
	document.getElementById("cancelBtn").style.display = "";
	document.getElementById("cancelBtn_openvpn").style.display = "";
}

function cancel_add_rule(){
	restart_vpncall_flag = 0;
	idx_tmp = "";
	$("#openvpnc_setting_openvpn").fadeOut(300);
	$("#openvpnc_setting").fadeOut(300);
}

function addRow_Group(upper, flag, idx){
	document.openvpnManualForm.vpn_crt_client1_ca.disabled = true;
	document.openvpnManualForm.vpn_crt_client1_crt.disabled = true;
	document.openvpnManualForm.vpn_crt_client1_key.disabled = true;
	document.openvpnManualForm.vpn_crt_client1_static.disabled = true;
	document.openvpnManualForm.vpn_crt_client1_crl.disabled = true;
	document.openvpnManualForm.vpn_crt_client2_ca.disabled = true;
	document.openvpnManualForm.vpn_crt_client2_crt.disabled = true;
	document.openvpnManualForm.vpn_crt_client2_key.disabled = true;
	document.openvpnManualForm.vpn_crt_client2_static.disabled = true;
	document.openvpnManualForm.vpn_crt_client2_crl.disabled = true;
	document.openvpnManualForm.vpn_crt_client3_ca.disabled = true;
	document.openvpnManualForm.vpn_crt_client3_crt.disabled = true;
	document.openvpnManualForm.vpn_crt_client3_key.disabled = true;
	document.openvpnManualForm.vpn_crt_client3_static.disabled = true;
	document.openvpnManualForm.vpn_crt_client3_crl.disabled = true;
	document.openvpnManualForm.vpn_crt_client4_ca.disabled = true;
	document.openvpnManualForm.vpn_crt_client4_crt.disabled = true;
	document.openvpnManualForm.vpn_crt_client4_key.disabled = true;
	document.openvpnManualForm.vpn_crt_client4_static.disabled = true;
	document.openvpnManualForm.vpn_crt_client4_crl.disabled = true;
	document.openvpnManualForm.vpn_crt_client5_ca.disabled = true;
	document.openvpnManualForm.vpn_crt_client5_crt.disabled = true;
	document.openvpnManualForm.vpn_crt_client5_key.disabled = true;
	document.openvpnManualForm.vpn_crt_client5_static.disabled = true;
	document.openvpnManualForm.vpn_crt_client5_crl.disabled = true;
	idx = parseInt(idx);
	if(idx >= 0){		//idx: edit row		
		var table_id = "vpnc_clientlist_table";
		var rule_num = document.getElementById(table_id).rows.length;
		var item_num = document.getElementById(table_id).rows[0].cells.length;		
		if(flag == 'PPTP' || flag == 'L2TP'){
			description_obj = document.form.vpnc_des_edit;
			type_obj = document.form.vpnc_type;			
			server_obj = document.form.vpnc_svr_edit;
			username_obj = document.form.vpnc_account_edit;
			password_obj = document.form.vpnc_pwd_edit;
			
		}else{	//OpenVPN: openvpn
			description_obj = document.vpnclientForm.vpnc_openvpn_des;
			type_obj = document.vpnclientForm.vpnc_type;
			server_obj = document.vpnclientForm.vpnc_openvpn_unit_edit;
			username_obj = document.vpnclientForm.vpnc_openvpn_username;
			password_obj = document.vpnclientForm.vpnc_openvpn_pwd;
					
		}
		
		if(validForm(flag)){
			duplicateCheck.tmpIdx = "";
			duplicateCheck.saveTotmpIdx(idx);
			duplicateCheck.tmpDataArray = [];
			duplicateCheck.saveToTmpDataArray(type_obj);
			duplicateCheck.saveToTmpDataArray(server_obj);
			duplicateCheck.saveToTmpDataArray(username_obj);
			duplicateCheck.saveToTmpDataArray(password_obj);
			if(duplicateCheck.isDuplicate()){
				username_obj.focus();
				username_obj.select();
				alert("<#JS_duplicate#>")
				return false;
			}

			var editVPNCRuleArray = new Array();
			editVPNCRuleArray.push(description_obj.value);
			editVPNCRuleArray.push(type_obj.value);
			editVPNCRuleArray.push(server_obj.value);
			editVPNCRuleArray.push(username_obj.value);
			editVPNCRuleArray.push(password_obj.value);

			vpnc_clientlist_array[idx] = editVPNCRuleArray;

			//handle vpnc_pptp_options_x_list
			handlePPTPOPtion(idx, document.form.selPPTPOption.value); //idx: NaN=Add i=edit row

			show_vpnc_rulelist();
			
			if(restart_vpncall_flag == 1){	//restart_vpncall
				var vpnc_desc = editVPNCRuleArray[0];
				var vpnc_proto = editVPNCRuleArray[1];
				var vpnc_server = editVPNCRuleArray[2];
				var vpnc_openvpn_idx = editVPNCRuleArray[2];
				var vpnc_username = editVPNCRuleArray[3];
				var vpnc_userpwd = editVPNCRuleArray[4];

				document.form.vpnc_des_edit.value = vpnc_desc;
				if(vpnc_proto == "PPTP")
					document.form.vpnc_proto.value = "pptp";
				else if(vpnc_proto == "L2TP")
					document.form.vpnc_proto.value = "l2tp";
				else	//OpenVPN
					document.form.vpnc_proto.value = "openvpn";

				if(vpnc_proto == "OpenVPN")
					document.form.vpn_client_unit.value = vpnc_openvpn_idx;
				else
					document.form.vpnc_heartbeat_x.value = vpnc_server;

				if(vpnc_proto == "OpenVPN") {
					switch (vpnc_openvpn_idx) {
						case "1" :
							document.form.vpn_client1_username.value = vpnc_username;
							break;
						case "2" :
							document.form.vpn_client2_username.value = vpnc_username;
							break;
						case "3" :
							document.form.vpn_client3_username.value = vpnc_username;
							break;
						case "4" :
							document.form.vpn_client4_username.value = vpnc_username;
							break;
						case "5" :
							document.form.vpn_client5_username.value = vpnc_username;
							break;
					}
				}
				else
					document.form.vpnc_pppoe_username.value = vpnc_username;

				if(vpnc_proto == "OpenVPN") {
					switch (vpnc_openvpn_idx) {
						case "1" :
							document.form.vpn_client1_password.value = vpnc_userpwd;
							break;
						case "2" :
							document.form.vpn_client2_password.value = vpnc_userpwd;
							break;
						case "3" :
							document.form.vpn_client3_password.value = vpnc_userpwd;
							break;
						case "4" :
							document.form.vpn_client4_password.value = vpnc_userpwd;
							break;
						case "5" :
							document.form.vpn_client5_password.value = vpnc_userpwd;
							break;
					}
				}
				else
					document.form.vpnc_pppoe_passwd.value = vpnc_userpwd;

				// update vpnc_pptp_options_x
				document.form.vpnc_pptp_options_x.value = "";
				if(vpnc_proto == "PPTP" && document.form.selPPTPOption.value != "auto") {
					document.form.vpnc_pptp_options_x.value = document.form.selPPTPOption.value;
				}

				document.form.vpn_clientx_eas.disabled = true;
				document.form.vpnc_clientlist.value = parseArrayToStr_vpnc_clientlist();
				document.form.vpnc_pptp_options_x_list.value = parseArrayToStr_vpnc_pptp_options_x_list();
				document.getElementById("vpnc_clientlist_table").rows[idx].cells[0].innerHTML = "-";
				document.getElementById("vpnc_clientlist_table").rows[idx].cells[5].innerHTML = "<img src='/images/InternetScan.gif'>";
				document.form.submit();	
			}
			else{
				document.vpnclientForm.vpnc_clientlist.value = parseArrayToStr_vpnc_clientlist();
				document.vpnclientForm.vpnc_pptp_options_x_list.value = parseArrayToStr_vpnc_pptp_options_x_list();
				document.vpnclientForm.submit();		
			}
			
			cancel_add_rule();
		}	
	}
	else{	//Add Rule
		
		var rule_num = document.getElementById("vpnc_clientlist_table").rows.length;
		var item_num = document.getElementById("vpnc_clientlist_table").rows[0].cells.length;		
		if(rule_num >= upper){
			alert("<#JS_itemlimit1#> " + upper + " <#JS_itemlimit2#>");
			return false;	
		}	

		if(flag == 'PPTP' || flag == 'L2TP'){
			type_obj = document.form.vpnc_type;
			description_obj = document.form.vpnc_des_edit;
			server_obj = document.form.vpnc_svr_edit;
			username_obj = document.form.vpnc_account_edit;
			password_obj = document.form.vpnc_pwd_edit;

		}else{	//OpenVPN: openvpn
			description_obj = document.vpnclientForm.vpnc_openvpn_des;
			type_obj = document.vpnclientForm.vpnc_type;
			server_obj = document.vpnclientForm.vpnc_openvpn_unit_edit;	// vpn_client_unit: 1 or 2
			username_obj = document.vpnclientForm.vpnc_openvpn_username;
			password_obj = document.vpnclientForm.vpnc_openvpn_pwd;
			
		}
		
		if(validForm(flag)){
			duplicateCheck.tmpIdx = "";
			duplicateCheck.saveTotmpIdx(idx);
			duplicateCheck.tmpDataArray = [];
			duplicateCheck.saveToTmpDataArray(type_obj);
			duplicateCheck.saveToTmpDataArray(server_obj);
			duplicateCheck.saveToTmpDataArray(username_obj);
			duplicateCheck.saveToTmpDataArray(password_obj);
			if(duplicateCheck.isDuplicate()){
				alert("<#JS_duplicate#>")
				return false;
			}

			var newVPNCRuleArray = new Array();
			newVPNCRuleArray.push(description_obj.value);
			newVPNCRuleArray.push(type_obj.value);
			newVPNCRuleArray.push(server_obj.value);
			newVPNCRuleArray.push(username_obj.value);
			newVPNCRuleArray.push(password_obj.value);

			vpnc_clientlist_array.push(newVPNCRuleArray);

			//handle vpnc_pptp_options_x_list
			handlePPTPOPtion(idx, document.form.selPPTPOption.value); //idx: NaN=Add i=edit row

			show_vpnc_rulelist();
			cancel_add_rule();	
			document.vpnclientForm.vpn_clientx_eas.disabled = true;
			document.vpnclientForm.vpnc_pptp_options_x_list.value = parseArrayToStr_vpnc_pptp_options_x_list();
			document.vpnclientForm.vpnc_clientlist.value = parseArrayToStr_vpnc_clientlist();
			document.vpnclientForm.submit();			
		}				
	}	
}

function handlePPTPOPtion(idx, objValue) {
	var origPPTPOptionsListArray = vpnc_pptp_options_x_list_array.slice();
	vpnc_pptp_options_x_list_array = [];
	if(idx >= 0) { // edit
		for(var i = 0; i < vpnc_clientlist_array.length; i += 1) {
			var eachRuleArray = new Array();
			if(origPPTPOptionsListArray[i] == undefined) {
				eachRuleArray.push("auto");
			}
			else {
				eachRuleArray.push(origPPTPOptionsListArray[i][0]);
			}
			vpnc_pptp_options_x_list_array.push(eachRuleArray);
		}
		var editRuleArray = new Array();
		editRuleArray.push(objValue);
		vpnc_pptp_options_x_list_array[idx] = editRuleArray;
	}
	else { // add
		for(var i = 0; i < (vpnc_clientlist_array.length - 1); i += 1) {
			var eachRuleArray = new Array();
			if(origPPTPOptionsListArray[i] == undefined) {
				eachRuleArray.push("auto");
			}
			else {
				eachRuleArray.push(origPPTPOptionsListArray[i][0]);
			}
			vpnc_pptp_options_x_list_array.push(eachRuleArray);
		}
		var newRuleArray = new Array();
		newRuleArray.push(objValue);
		vpnc_pptp_options_x_list_array.push(newRuleArray);
	}
}

var duplicateCheck = {
	tmpIdx: "",
	saveTotmpIdx: function(obj){
		this.tmpIdx = obj;	
	},	
	tmpDataArray: [],
	saveToTmpDataArray: function(obj) {
		this.tmpDataArray.push(obj.value);
	},
	isDuplicate: function(){
		var index_del = parseInt(this.tmpIdx);
		for(var i = 0; i < vpnc_clientlist_array.length; i += 1) {
			var compareDataArray = vpnc_clientlist_array[i].slice();
			compareDataArray.splice(0, 1);
			if(i != index_del) {
				if(this.tmpDataArray.toString() == compareDataArray.toString())
					return true;
			}
		}
		return false;
	}
}

function validForm(mode){
	if(mode == "PPTP" || mode == "L2TP"){
		valid_des = document.form.vpnc_des_edit;
		valid_server = document.form.vpnc_svr_edit;
		valid_username = document.form.vpnc_account_edit;
		valid_password = document.form.vpnc_pwd_edit;

		if(valid_des.value==""){
			alert("<#JS_fieldblank#>");
			valid_des.focus();
			return false;		
		}else if(!Block_chars(valid_des, ["*", "+", "|", ":", "?", "<", ">", ",", ".", "/", ";", "[", "]", "\\", "=", "\"" ])){
			return false;		
		}

		if(valid_server.value==""){
			alert("<#JS_fieldblank#>");
			valid_server.focus();
			return false;
		}
		else{
			var isIPAddr = valid_server.value.replace(/\./g,"");
			var re = /^[0-9]+$/;
			if(!re.test(isIPAddr)) { //DDNS
				if(!Block_chars(valid_server, ["<", ">"])) {
					return false;
				}		
			}	
			else { // IP
				if(!validator.isLegalIP(valid_server,"")) {
					return false;
				}
			}
		}

		if(valid_username.value==""){
			alert("<#JS_fieldblank#>");
			valid_username.focus();
			return false;
		}else if(!Block_chars(valid_username, ["<", ">"])){
			return false;		
		}

		if(valid_password.value==""){
			alert("<#JS_fieldblank#>");
			valid_password.focus();
			return false;
		}else if(!Block_chars(valid_password, ["<", ">"])){
			return false;		
		}
			
	}
	else{		//OpenVPN
		valid_des = document.vpnclientForm.vpnc_openvpn_des;
		valid_username = document.vpnclientForm.vpnc_openvpn_username;
		valid_password = document.vpnclientForm.vpnc_openvpn_pwd;
		if(valid_des.value == ""){
			alert("<#JS_fieldblank#>");
			valid_des.focus();
			return false;		
		}
		else if(!Block_chars(valid_des, ["*", "+", "|", ":", "?", "<", ">", ",", ".", "/", ";", "[", "]", "\\", "=", "\"" ])){
			return false;		
		}
		
		if(valid_username.value != "" && !Block_chars(valid_username, ["<", ">"])){
			return false;		
		}

		if(valid_password.value != "" && !Block_chars(valid_password, ["<", ">"])){
			return false;		
		}
		
	}	
	
	return true;
}

var save_flag;	//type of Saving profile
function tabclickhandler(_type){
	var tab_id = "";
	switch(_type) {
		case 0 :
		case 1 :
			tab_id = "pptp";
			break;
		case 2 :
			tab_id = "openvpn";
			break;
	}
	document.getElementById('pptpcTitle_' + tab_id + '').className = "vpnClientTitle_td_unclick";
	document.getElementById('l2tpcTitle_' + tab_id + '').className = "vpnClientTitle_td_unclick";
	if(openvpnd_support)
		document.getElementById('opencTitle_' + tab_id + '').className = "vpnClientTitle_td_unclick";
	document.getElementById('openvpnc_setting').style.display = "none";
	document.getElementById('openvpnc_setting_openvpn').style.display = "none";	
	document.getElementById('trPPTPOptions').style.display = "none";
	if(_type == 0){
		save_flag = "PPTP";
		document.form.vpnc_type.value = "PPTP";
		document.vpnclientForm.vpnc_type.value = "PPTP";
		document.getElementById('pptpcTitle_' + tab_id + '').className = "vpnClientTitle_td_click";
		document.getElementById('openvpnc_setting').style.display = "block";
		document.getElementById('trPPTPOptions').style.display = "";
	}
	else if(_type == 1){
		save_flag = "L2TP";
		document.form.vpnc_type.value = "L2TP";
		document.vpnclientForm.vpnc_type.value = "L2TP";
		document.getElementById('l2tpcTitle_' + tab_id + '').className = "vpnClientTitle_td_click";
		document.getElementById('openvpnc_setting').style.display = "block";
	}
	else if(_type == 2){
		save_flag = "OpenVPN";
		document.form.vpnc_type.value = "OpenVPN";
		document.vpnclientForm.vpnc_type.value = "OpenVPN";
		document.getElementById('opencTitle_' + tab_id + '').className = "vpnClientTitle_td_click";
		document.getElementById('openvpnc_setting_openvpn').style.display = "block";
		if(add_profile_flag)
			update_unit_option();
	}

	if (openvpn_arrayLength == 5 && openvpnd_support && add_profile_flag) {
		document.getElementById('opencTitle_' + tab_id + '').style.display = "none";
	}
}

function update_unit_option(){
	var vpnc_openvpn_unit_array = [];
	for(var i = 1; i <= 5; i += 1) {
		vpnc_openvpn_unit_array["unit_" + i] = i;
	}
	for(var i = 0; i < vpnc_clientlist_array.length; i += 1) {
		if(vpnc_clientlist_array[i] != "") {
			var vpnc_proto = vpnc_clientlist_array[i][1];
			if(vpnc_proto == "OpenVPN") {
				var vpnc_openvpn_idx = vpnc_clientlist_array[i][2];
				delete vpnc_openvpn_unit_array["unit_" + vpnc_openvpn_idx];
			}
		}
	}

	// convert object into array
	var sortArray = [];
	for(var key in vpnc_openvpn_unit_array) {
		if(vpnc_openvpn_unit_array.hasOwnProperty(key))
			sortArray.push(vpnc_openvpn_unit_array[key]);
	}

	// sort items by value
	sortArray.sort(function(a, b){return b - a});
	document.vpnclientForm.vpnc_openvpn_unit_edit.value = sortArray[0];
}

var openvpn_arrayLength = 0;
function show_vpnc_rulelist(){
	all_profile_subnet_list = "";	
	openvpn_arrayLength = 0;

	var code = "";
	code +='<table style="margin-bottom:30px;" width="98%" border="1" align="center" cellpadding="4" cellspacing="0" class="list_table" id="vpnc_clientlist_table">';
	if(vpnc_clientlist_array.length == 0)
		code +='<tr><td style="color:#FC0;" colspan="6"><#IPConnection_VSList_Norule#></td></tr>';
	else{
		for(var i = 0; i < vpnc_clientlist_array.length; i += 1) {
			if(vpnc_clientlist_array[i] != "") {
				var vpnc_desc = vpnc_clientlist_array[i][0];
				var vpnc_proto = vpnc_clientlist_array[i][1];
				var vpnc_server = vpnc_clientlist_array[i][2];
				var vpnc_openvpn_idx = vpnc_clientlist_array[i][2];
				var vpnc_username = vpnc_clientlist_array[i][3];
				
				code +='<tr id="row'+i+'">';
				var client_state = "0";
				if(vpnc_proto == "OpenVPN"){
					openvpn_arrayLength++;
					var connect_idx = document.form.vpn_clientx_eas.value.replace(",", "");
						var client_errno = "";
						if(connect_idx == vpnc_openvpn_idx) {
							switch(vpnc_openvpn_idx) {
								case "1" :
									client_state = vpnc_state_t1;
									client_errno = vpnc_errno_t1;
									break;
								case "2" :
									client_state = vpnc_state_t2;
									client_errno = vpnc_errno_t2;
									break;
								case "3" :
									client_state = vpnc_state_t3;
									client_errno = vpnc_errno_t3;
									break;
								case "4" :
									client_state = vpnc_state_t4;
									client_errno = vpnc_errno_t4;
									break;
								case "5" :
									client_state = vpnc_state_t5;
									client_errno = vpnc_errno_t5;
									break;
							}
						}
						// state: default 0, connecting 1, connected 2, error -1
						// when state is -1, check errno, default 0, conflict 1 2 3, auth fail 4 5 6
						if(client_state == 0)
							code +='<td width="10%">-</td>';
						else if(client_state == 1)
							code +="<td width='10%'><img title='<#CTL_Add_enrollee#>' src='/images/InternetScan.gif'></td>";
						else if(client_state == 2)
							code +="<td width='10%'><img title='<#Connected#>' src='/images/checked_parentctrl.png' style='width:25px;'></td>";
						else if(client_errno == 1 || client_errno == 2 || client_errno == 3)
							code +="<td width='10%'><div title='<#vpn_openvpn_conflict#>' class='vpnc_ipconflict_icon'></div></td>";
						else if(client_errno == 4 || client_errno == 5 || client_errno == 6)
							code +="<td width='10%'><img title='<#qis_fail_desc1#>' src='/images/button-close2.png' style='width:25px;'></td>";
						else		//Stop connection
							code +="<td width='10%'><img title='<#ConnectionFailed#>' src='/images/button-close2.png' style='width:25px;'></td>";
				}
				else{
					if( vpnc_proto == document.form.vpnc_proto.value.toUpperCase() 
					 && vpnc_server == document.form.vpnc_heartbeat_x.value
					 && vpnc_username == document.form.vpnc_pppoe_username.value){		//matched connecting rule
						if(vpnc_state_t == 0 || vpnc_state_t ==1) // Initial or Connecting
							code +="<td width='10%'><img title='<#CTL_Add_enrollee#>' src='/images/InternetScan.gif'></td>";
						else if(vpnc_state_t == 2) // Connected
							code +="<td width='10%'><img title='<#Connected#>' src='/images/checked_parentctrl.png' style='width:25px;'></td>";
						else if(vpnc_state_t == 4 && vpnc_sbstate_t == 2)
							code +="<td width='10%'><img title='<#qis_fail_desc1#>' src='/images/button-close2.png' style='width:25px;'></td>";
						else // Stop connection
							code +="<td width='10%'><img title='<#ConnectionFailed#>' src='/images/button-close2.png' style='width:25px;'></td>";
					}
					else{
						code +='<td width="10%">-</td>';
					}	
				}
				
				//Description
				if(vpnc_desc.length >28) {
					var overlib_str = vpnc_desc.substring(0, 25) + "...";
					code +='<td width="30%" title="'+vpnc_desc+'">'+ overlib_str +'</td>';
				}
				else {
					code +='<td width="30%">'+ vpnc_desc +'</td>';					
				}
				//VPN type
				code += '<td width="15%">'+ vpnc_proto +'</td>';

				// EDIT
			 	code += '<td width="10%"><input class="edit_btn" type="button" onclick="Edit_Row(this, \'vpnc\');" value=""/></td>';
				if(vpnc_proto == "OpenVPN"){ 
					if(client_state != 0) {	//connecting
						code += '<td width="10%"><input class="remove_btn" type="button" onclick="del_Row(this, \'vpnc_enable\');" value=""/></td>';
						code += '<td width="25%"><input class="button_gen" type="button" onClick="connect_Row(this, \'disconnect\');" id="disonnect_btn" value="Deactivate" style="padding:0 0.3em 0 0.3em;" >';
					}
					else{			//OpenVPN is not connecting
						code += '<td width="10%"><input class="remove_btn" type="button" onclick="del_Row(this, \'vpnc\');" value=""/></td>';
						code += '<td width="25%"><input class="button_gen" type="button" onClick="connect_Row(this, \'vpnc\');" id="Connect_btn" name="Connect_btn" value="Activate" style="padding:0 0.3em 0 0.3em;" >';
					}
				}
				else{
					if( vpnc_proto == document.form.vpnc_proto.value.toUpperCase() 
					 && vpnc_server == document.form.vpnc_heartbeat_x.value 
					 && vpnc_username == document.form.vpnc_pppoe_username.value){		// This rule is connecting
						code += '<td width="10%"><input class="remove_btn" type="button" onclick="del_Row(this, \'vpnc_enable\');" value=""/></td>';
						code += '<td width="25%"><input class="button_gen" type="button" onClick="connect_Row(this, \'disconnect\');" id="disonnect_btn" value="Deactivate" style="padding:0 0.3em 0 0.3em;" >';
					}
					else{		// This rule is not connecting
						code += '<td width="10%"><input class="remove_btn" type="button" onclick="del_Row(this, \'vpnc\');" value=""/></td>';
						code += '<td width="25%"><input class="button_gen" type="button" onClick="connect_Row(this, \'vpnc\');" id="Connect_btn" name="Connect_btn" value="Activate" style="padding:0 0.3em 0 0.3em;" >';
					}
				}
			}
		}
	}
	
	code +='</table>';
	document.getElementById("vpnc_clientlist_Block").innerHTML = code;		
}
 
function connect_Row(rowdata, flag){
	var idx = rowdata.parentNode.parentNode.rowIndex;
	var vpnc_desc = vpnc_clientlist_array[idx][0];
	var vpnc_proto = vpnc_clientlist_array[idx][1];
	var vpnc_server = vpnc_clientlist_array[idx][2];
	var vpnc_openvpn_idx = vpnc_clientlist_array[idx][2];
	var vpnc_username = vpnc_clientlist_array[idx][3];
	var vpnc_userpwd = vpnc_clientlist_array[idx][4];
	
	if(flag == "disconnect"){	//Disconnect the connected rule 
		
		if(vpnc_proto == "OpenVPN"){
			document.form.vpnc_proto.value = "disable";
			document.form.vpn_client_unit.value = vpnc_openvpn_idx;
			switch (vpnc_openvpn_idx) {
				case "1" :
					document.form.vpn_client1_username.value = "";
					document.form.vpn_client1_password.value = "";
					break;
				case "2" :
					document.form.vpn_client2_username.value = "";
					document.form.vpn_client2_password.value = "";
					break;
				case "3" :
					document.form.vpn_client3_username.value = "";
					document.form.vpn_client3_password.value = "";
					break;
				case "4" :
					document.form.vpn_client4_username.value = "";
					document.form.vpn_client4_password.value = "";
					break;
				case "5" :
					document.form.vpn_client5_username.value = "";
					document.form.vpn_client5_password.value = "";
					break;
			}
			document.form.vpn_clientx_eas.disabled = false;
			document.form.vpn_clientx_eas.value = document.form.vpn_clientx_eas.value.replace(vpnc_openvpn_idx + ",", "");
			document.vpnclientForm.vpn_clientx_eas.value = document.form.vpn_clientx_eas.value.replace(vpnc_openvpn_idx + ",", "");
		}else{ //pptp/l2tp
			document.form.vpnc_proto.value = "disable";
			document.form.vpnc_heartbeat_x.value = "";
			document.form.vpnc_pppoe_username.value = "";
			document.form.vpnc_pppoe_passwd.value = "";
						
			if(vpnc_proto == "PPTP") {
				document.form.vpnc_pptp_options_x.value = "";
			}
			
		}			
	}
	else{		//"vpnc" making connection
		document.form.vpnc_des_edit.value = vpnc_desc;

		if(vpnc_proto == "PPTP")
			document.form.vpnc_proto.value = "pptp";
		else if(vpnc_proto == "L2TP")
			document.form.vpnc_proto.value = "l2tp";
		else	//OpenVPN
			document.form.vpnc_proto.value = "openvpn";

		if(vpnc_proto == "OpenVPN")
			document.form.vpn_client_unit.value = vpnc_openvpn_idx;
		else
			document.form.vpnc_heartbeat_x.value = vpnc_server;

		if(vpnc_proto == "OpenVPN") {
			switch (vpnc_openvpn_idx) {
				case "1" :
					document.form.vpn_client1_username.value = vpnc_username;
					break;
				case "2" :
					document.form.vpn_client2_username.value = vpnc_username;
					break;
				case "3" :
					document.form.vpn_client3_username.value = vpnc_username;
					break;
				case "4" :
					document.form.vpn_client4_username.value = vpnc_username;
					break;
				case "5" :
					document.form.vpn_client5_username.value = vpnc_username;
					break;
			}
		}
		else
			document.form.vpnc_pppoe_username.value = vpnc_username;

		if(vpnc_proto == "OpenVPN") {
			switch (vpnc_openvpn_idx) {
				case "1" :
					document.form.vpn_client1_password.value = vpnc_userpwd;
					break;
				case "2" :
					document.form.vpn_client2_password.value = vpnc_userpwd;
					break;
				case "3" :
					document.form.vpn_client3_password.value = vpnc_userpwd;
					break;
				case "4" :
					document.form.vpn_client4_password.value = vpnc_userpwd;
					break;
				case "5" :
					document.form.vpn_client5_password.value = vpnc_userpwd;
					break;
			}
		}
		else	
			document.form.vpnc_pppoe_passwd.value = vpnc_userpwd;

			
		if(vpnc_proto == "OpenVPN"){
			document.form.vpn_clientx_eas.disabled = false;
			if(document.form.vpn_clientx_eas.value.search(vpnc_openvpn_idx) < 0 ) 
				document.form.vpn_clientx_eas.value = vpnc_openvpn_idx + ",";
			if(document.vpnclientForm.vpn_clientx_eas.value.search(vpnc_openvpn_idx) < 0 ) 
				document.vpnclientForm.vpn_clientx_eas.value = vpnc_openvpn_idx + ",";
		}
		else{
			document.form.vpnc_auto_conn.value = 1;
		}	
			

		//handle vpnc_pptp_options_x
		if(vpnc_proto == "PPTP"){
			document.form.vpnc_pptp_options_x.value = "";
			if(vpnc_pptp_options_x_list_array[idx] != undefined) {
				var setPPTPOption = vpnc_pptp_options_x_list_array[idx][0];
				if(setPPTPOption != "auto" && setPPTPOption != "undefined") {
					document.form.vpnc_pptp_options_x.value = setPPTPOption;
				}
			}
		}
	}
	
	document.form.vpnc_pptp_options_x_list.value = parseArrayToStr_vpnc_pptp_options_x_list();
	document.form.vpnc_clientlist.value = parseArrayToStr_vpnc_clientlist();
	rowdata.parentNode.innerHTML = "<img src='/images/InternetScan.gif'>";
	document.form.submit();	
}

function setManualTable (unit) {
	switch (unit) {
		case "1" :
			document.getElementById('edit_vpn_crt_client_ca').value = vpn_crt_client1_ca[0].replace(/&#10/g, "\n").replace(/&#13/g, "\r");
			document.getElementById('edit_vpn_crt_client_crt').value = vpn_crt_client1_crt[0].replace(/&#10/g, "\n").replace(/&#13/g, "\r");
			document.getElementById('edit_vpn_crt_client_key').value = vpn_crt_client1_key[0].replace(/&#10/g, "\n").replace(/&#13/g, "\r");
			document.getElementById('edit_vpn_crt_client_static').value = vpn_crt_client1_static[0].replace(/&#10/g, "\n").replace(/&#13/g, "\r");
			document.getElementById('edit_vpn_crt_client_crl').value = vpn_crt_client1_crl[0].replace(/&#10/g, "\n").replace(/&#13/g, "\r");
			break;
		case "2" :
			document.getElementById('edit_vpn_crt_client_ca').value = vpn_crt_client2_ca[0].replace(/&#10/g, "\n").replace(/&#13/g, "\r");
			document.getElementById('edit_vpn_crt_client_crt').value = vpn_crt_client2_crt[0].replace(/&#10/g, "\n").replace(/&#13/g, "\r");
			document.getElementById('edit_vpn_crt_client_key').value = vpn_crt_client2_key[0].replace(/&#10/g, "\n").replace(/&#13/g, "\r");
			document.getElementById('edit_vpn_crt_client_static').value = vpn_crt_client2_static[0].replace(/&#10/g, "\n").replace(/&#13/g, "\r");
			document.getElementById('edit_vpn_crt_client_crl').value = vpn_crt_client2_crl[0].replace(/&#10/g, "\n").replace(/&#13/g, "\r");
			break;
		case "3" :
			document.getElementById('edit_vpn_crt_client_ca').value = vpn_crt_client3_ca[0].replace(/&#10/g, "\n").replace(/&#13/g, "\r");
			document.getElementById('edit_vpn_crt_client_crt').value = vpn_crt_client3_crt[0].replace(/&#10/g, "\n").replace(/&#13/g, "\r");
			document.getElementById('edit_vpn_crt_client_key').value = vpn_crt_client3_key[0].replace(/&#10/g, "\n").replace(/&#13/g, "\r");
			document.getElementById('edit_vpn_crt_client_static').value = vpn_crt_client3_static[0].replace(/&#10/g, "\n").replace(/&#13/g, "\r");
			document.getElementById('edit_vpn_crt_client_crl').value = vpn_crt_client3_crl[0].replace(/&#10/g, "\n").replace(/&#13/g, "\r");
			break;
		case "4" :
			document.getElementById('edit_vpn_crt_client_ca').value = vpn_crt_client4_ca[0].replace(/&#10/g, "\n").replace(/&#13/g, "\r");
			document.getElementById('edit_vpn_crt_client_crt').value = vpn_crt_client4_crt[0].replace(/&#10/g, "\n").replace(/&#13/g, "\r");
			document.getElementById('edit_vpn_crt_client_key').value = vpn_crt_client4_key[0].replace(/&#10/g, "\n").replace(/&#13/g, "\r");
			document.getElementById('edit_vpn_crt_client_static').value = vpn_crt_client4_static[0].replace(/&#10/g, "\n").replace(/&#13/g, "\r");
			document.getElementById('edit_vpn_crt_client_crl').value = vpn_crt_client4_crl[0].replace(/&#10/g, "\n").replace(/&#13/g, "\r");
			break;
		case "5" :
			document.getElementById('edit_vpn_crt_client_ca').value = vpn_crt_client5_ca[0].replace(/&#10/g, "\n").replace(/&#13/g, "\r");
			document.getElementById('edit_vpn_crt_client_crt').value = vpn_crt_client5_crt[0].replace(/&#10/g, "\n").replace(/&#13/g, "\r");
			document.getElementById('edit_vpn_crt_client_key').value = vpn_crt_client5_key[0].replace(/&#10/g, "\n").replace(/&#13/g, "\r");
			document.getElementById('edit_vpn_crt_client_static').value = vpn_crt_client5_static[0].replace(/&#10/g, "\n").replace(/&#13/g, "\r");
			document.getElementById('edit_vpn_crt_client_crl').value = vpn_crt_client5_crl[0].replace(/&#10/g, "\n").replace(/&#13/g, "\r");
			break;
	}
}
function ovpnFileUpdate(unit){
	$.ajax({
		url: '/ajax_openvpn_server.asp',
		dataType: 'script',
		timeout: 1500,
		error: function(xhr){
			setTimeout("ovpnFileUpdate(" + unit + ");",1000);
		},	
		success: function(){
			setManualTable(unit);
		}
	});
}
var idx_tmp = "";
function Edit_Row(rowdata, flag){
	add_profile_flag = false;
	document.getElementById("cancelBtn").style.display = "";
	document.getElementById("cancelBtn_openvpn").style.display = "";
	idx_tmp = rowdata.parentNode.parentNode.rowIndex; //update idx
	var idx = rowdata.parentNode.parentNode.rowIndex;
	if(document.getElementById("vpnc_clientlist_table").rows[idx].cells[0].innerHTML != "-")
		restart_vpncall_flag = 1;

	var vpnc_clientlist_edit_array = vpnc_clientlist_array.slice(idx, (idx + 1));
	var vpnc_desc = vpnc_clientlist_edit_array[0][0];
	var vpnc_proto = vpnc_clientlist_edit_array[0][1];
	var vpnc_server = vpnc_clientlist_edit_array[0][2];
	var vpnc_openvpn_idx = vpnc_clientlist_edit_array[0][2];
	var vpnc_username = vpnc_clientlist_edit_array[0][3];
	var vpnc_userpwd = vpnc_clientlist_edit_array[0][4];

	//get idx of PPTP option value
	var pptpOptionValue = "";
	if(idx >= 0){
		if(vpnc_pptp_options_x_list_array[idx] == undefined) {
			pptpOptionValue = "auto";
		}
		else {
			pptpOptionValue = vpnc_pptp_options_x_list_array[idx];
		}
	}
	else{	//default is auto
		pptpOptionValue = "auto";
	}
	
	document.form.selPPTPOption.value = pptpOptionValue;
	pptpOptionChange();

	if(vpnc_proto == "PPTP" || vpnc_proto == "L2TP") {
		gen_vpnc_tab_list("pptp");
		$("#openvpnc_setting").fadeIn(300);
		document.getElementById("pptpcTitle_pptp").style.display = "none";
		document.getElementById("trPPTPOptions").style.display = "none";
		document.getElementById("l2tpcTitle_pptp").style.display = "none";
		if(openvpnd_support)
			document.getElementById("opencTitle_pptp").style.display = "none";
		if(vpnc_proto == "PPTP") {
			document.getElementById("pptpcTitle_pptp").style.display = "";
			document.getElementById("trPPTPOptions").style.display = "";
		}
		else {
			document.getElementById("l2tpcTitle_pptp").style.display = "";
		}
	}
	else if(vpnc_proto == "OpenVPN") {
		gen_vpnc_tab_list("openvpn");
		$("#openvpnc_setting_openvpn").fadeIn(300);
		document.getElementById("pptpcTitle_openvpn").style.display = "none";
		document.getElementById("l2tpcTitle_openvpn").style.display = "none";
		document.getElementById("opencTitle_openvpn").style.display = "";
	}

	if(vpnc_proto == "OpenVPN"){
		document.vpnclientForm.file.value = "";
		document.openvpnCAForm.file.value = "";
		document.vpnclientForm.vpnc_openvpn_des.value = vpnc_desc;
		document.vpnclientForm.vpnc_openvpn_username.value = vpnc_username;
		document.vpnclientForm.vpnc_openvpn_pwd.value = vpnc_userpwd;	

		ovpnFileUpdate(vpnc_openvpn_idx);
					
		tabclickhandler(2);
		//fixed unit
		document.vpnclientForm.vpnc_openvpn_unit_edit.value = vpnc_openvpn_idx;

		document.getElementById("caFiled").style.display = "";
		document.getElementById("manualFiled").style.display = "";
		document.getElementById('importOvpnFile').style.display = "none"; 
		document.getElementById('loadingicon').style.display = "none"; 
		document.getElementById("cbManualImport").checked = false;
		manualImport(false);
	}
	else{
		document.form.vpnc_des_edit.value = vpnc_desc;
		if(vpnc_proto == "PPTP")
			tabclickhandler(0);
		else if(vpnc_proto == "L2TP")
			tabclickhandler(1);
		else
			tabclickhandler(2);
		document.form.vpnc_svr_edit.value = vpnc_server;
		document.form.vpnc_account_edit.value = vpnc_username;
		document.form.vpnc_pwd_edit.value = vpnc_userpwd;
	}	
}

function del_Row(rowdata, flag){
	if(!confirm("Are you sure to delete this profile?"))/*untranslated*/
		return false;

	var idx = rowdata.parentNode.parentNode.rowIndex;
	document.getElementById("vpnc_clientlist_table").deleteRow(idx);

	var vpnc_clientlist_delete_array = vpnc_clientlist_array.slice(idx, (idx + 1));
	vpnc_clientlist_array.splice(idx, 1);
				
	//del vpnc_pptp_options_x_list
	if(vpnc_pptp_options_x_list_array[idx] != undefined) {
		vpnc_pptp_options_x_list_array.splice(idx, 1);
	}

	if(flag == "vpnc_enable"){	//remove connected rule.
		document.vpnclientForm.vpnc_proto.value = "disable";
		document.vpnclientForm.vpnc_proto.disabled = false;
		document.vpnclientForm.action = "/start_apply.htm";
		document.vpnclientForm.enctype = "application/x-www-form-urlencoded";
  		document.vpnclientForm.encoding = "application/x-www-form-urlencoded";
		document.vpnclientForm.action_script.value = "restart_vpncall";		
		if(vpnc_clientlist_delete_array[0][1] == "OpenVPN"){	
			document.vpnclientForm.vpn_clientx_eas.disabled = false;
			var openvpn_unit = vpnc_clientlist_delete_array[0][2];
			document.vpnclientForm.vpn_client_unit.value = parseInt(openvpn_unit);
			document.vpnclientForm.vpn_clientx_eas.value = document.vpnclientForm.vpn_clientx_eas.value.replace("" + openvpn_unit + ",", "");
			document.form.vpn_clientx_eas.value = document.form.vpn_clientx_eas.value.replace("" + openvpn_unit + ",", "");
		}	
	}

	show_vpnc_rulelist();
	document.vpnclientForm.vpnc_clientlist.value = parseArrayToStr_vpnc_clientlist();
	document.vpnclientForm.vpnc_pptp_options_x_list.value = parseArrayToStr_vpnc_pptp_options_x_list();
	document.vpnclientForm.submit();
}

function ImportOvpn(unit){
	document.getElementById('importOvpnFile').style.display = "none"; 
	document.getElementById('loadingicon').style.display = ""; 
	document.vpnclientForm.action = "vpnupload.cgi";
	document.vpnclientForm.enctype = "multipart/form-data";
	document.vpnclientForm.encoding = "multipart/form-data";
	document.vpnclientForm.vpn_upload_unit.value = unit;
	document.vpnclientForm.submit();
	setTimeout("ovpnFileChecker();",2000);
}

function manualImport(_flag){
	if(_flag){
		document.getElementById("caFiled").style.display = "";
		document.getElementById("manualFiled").style.display = "";
	}
	else{
		document.getElementById("caFiled").style.display = "none";
		document.getElementById("manualFiled").style.display = "none";
	}
}

var vpn_upload_state = "init";
function ovpnFileChecker(){
	document.getElementById("importOvpnFile").innerHTML = "<#Main_alert_proceeding_desc3#>";
	document.getElementById("manualCRList").style.color = "#FFF";
	document.getElementById("manualStatic").style.color = "#FFF";
	document.getElementById("manualKey").style.color = "#FFF";
	document.getElementById("manualCert").style.color = "#FFF";
	document.getElementById("manualCa").style.color = "#FFF";
	$.ajax({
		url: '/ajax_openvpn_server.asp',
		dataType: 'script',
		timeout: 1500,
		error: function(xhr){
			setTimeout("ovpnFileChecker();",1000);
		},	
		success: function(){
			document.getElementById('importOvpnFile').style.display = "";
			document.getElementById('loadingicon').style.display = "none";
			document.getElementById('importCA').style.display = "";
			document.getElementById('loadingiconCA').style.display = "none";
			if(vpn_upload_state == "init"){
				setTimeout("ovpnFileChecker();",1000);
			}
			else{
				setManualTable(document.vpnclientForm.vpnc_openvpn_unit_edit.value);
				
				var vpn_upload_state_tmp = "";
				vpn_upload_state_tmp = parseInt(vpn_upload_state) - 16;
				if(vpn_upload_state_tmp > -1){
					document.getElementById("importOvpnFile").innerHTML += ", Lack of Certificate Revocation List(Optional)";
					document.getElementById("manualCRList").style.color = "#FC0";
					vpn_upload_state = vpn_upload_state_tmp;
				}
					
				vpn_upload_state_tmp = parseInt(vpn_upload_state) - 8;
				if(vpn_upload_state_tmp > -1){
					document.getElementById("importOvpnFile").innerHTML += ", Lack of Static Key(Optional)";
					document.getElementById("manualStatic").style.color = "#FC0";
					vpn_upload_state = vpn_upload_state_tmp;
				}				

				vpn_upload_state_tmp = parseInt(vpn_upload_state) - 4;
				if(vpn_upload_state_tmp > -1){
					document.getElementById("importOvpnFile").innerHTML += ", Lack of Client Key!";
					document.getElementById("manualKey").style.color = "#FC0";
					vpn_upload_state = vpn_upload_state_tmp;
				}

				vpn_upload_state_tmp = parseInt(vpn_upload_state) - 2;
				if(vpn_upload_state_tmp > -1){
					document.getElementById("importOvpnFile").innerHTML += ", Lack of Client Certificate";
					document.getElementById("manualCert").style.color = "#FC0";
					vpn_upload_state = vpn_upload_state_tmp;
				}

				vpn_upload_state_tmp = parseInt(vpn_upload_state) - 1;
				if(vpn_upload_state_tmp > -1){
					document.getElementById("importOvpnFile").innerHTML += ", Lack of Certificate Authority";
					document.getElementById("manualCa").style.color = "#FC0";
				}
			}
		}
	});
}

var vpn_upload_state = "init";
function cancel_Key_panel(){
	document.getElementById("manualFiled_panel").style.display = "none";
}

function saveManual(unit){
	switch (unit) {
		case "1" :
			document.openvpnManualForm.vpn_crt_client1_ca.value = document.getElementById('edit_vpn_crt_client_ca').value;
			document.openvpnManualForm.vpn_crt_client1_crt.value = document.getElementById('edit_vpn_crt_client_crt').value;
			document.openvpnManualForm.vpn_crt_client1_key.value = document.getElementById('edit_vpn_crt_client_key').value;
			document.openvpnManualForm.vpn_crt_client1_static.value = document.getElementById('edit_vpn_crt_client_static').value;
			document.openvpnManualForm.vpn_crt_client1_crl.value = document.getElementById('edit_vpn_crt_client_crl').value;
			document.openvpnManualForm.vpn_crt_client1_ca.disabled = false;
			document.openvpnManualForm.vpn_crt_client1_crt.disabled = false;
			document.openvpnManualForm.vpn_crt_client1_key.disabled = false;
			document.openvpnManualForm.vpn_crt_client1_static.disabled = false;
			document.openvpnManualForm.vpn_crt_client1_crl.disabled = false;
			break;
		case "2" :
			document.openvpnManualForm.vpn_crt_client2_ca.value = document.getElementById('edit_vpn_crt_client_ca').value;
			document.openvpnManualForm.vpn_crt_client2_crt.value = document.getElementById('edit_vpn_crt_client_crt').value;
			document.openvpnManualForm.vpn_crt_client2_key.value = document.getElementById('edit_vpn_crt_client_key').value;
			document.openvpnManualForm.vpn_crt_client2_static.value = document.getElementById('edit_vpn_crt_client_static').value;
			document.openvpnManualForm.vpn_crt_client2_crl.value = document.getElementById('edit_vpn_crt_client_crl').value;
			document.openvpnManualForm.vpn_crt_client2_ca.disabled = false;
			document.openvpnManualForm.vpn_crt_client2_crt.disabled = false;
			document.openvpnManualForm.vpn_crt_client2_key.disabled = false;
			document.openvpnManualForm.vpn_crt_client2_static.disabled = false;
			document.openvpnManualForm.vpn_crt_client2_crl.disabled = false;
			break;
		case "3" :
			document.openvpnManualForm.vpn_crt_client3_ca.value = document.getElementById('edit_vpn_crt_client_ca').value;
			document.openvpnManualForm.vpn_crt_client3_crt.value = document.getElementById('edit_vpn_crt_client_crt').value;
			document.openvpnManualForm.vpn_crt_client3_key.value = document.getElementById('edit_vpn_crt_client_key').value;
			document.openvpnManualForm.vpn_crt_client3_static.value = document.getElementById('edit_vpn_crt_client_static').value;
			document.openvpnManualForm.vpn_crt_client3_crl.value = document.getElementById('edit_vpn_crt_client_crl').value;
			document.openvpnManualForm.vpn_crt_client3_ca.disabled = false;
			document.openvpnManualForm.vpn_crt_client3_crt.disabled = false;
			document.openvpnManualForm.vpn_crt_client3_key.disabled = false;
			document.openvpnManualForm.vpn_crt_client3_static.disabled = false;
			document.openvpnManualForm.vpn_crt_client3_crl.disabled = false;
			break;
		case "4" :
			document.openvpnManualForm.vpn_crt_client4_ca.value = document.getElementById('edit_vpn_crt_client_ca').value;
			document.openvpnManualForm.vpn_crt_client4_crt.value = document.getElementById('edit_vpn_crt_client_crt').value;
			document.openvpnManualForm.vpn_crt_client4_key.value = document.getElementById('edit_vpn_crt_client_key').value;
			document.openvpnManualForm.vpn_crt_client4_static.value = document.getElementById('edit_vpn_crt_client_static').value;
			document.openvpnManualForm.vpn_crt_client4_crl.value = document.getElementById('edit_vpn_crt_client_crl').value;
			document.openvpnManualForm.vpn_crt_client4_ca.disabled = false;
			document.openvpnManualForm.vpn_crt_client4_crt.disabled = false;
			document.openvpnManualForm.vpn_crt_client4_key.disabled = false;
			document.openvpnManualForm.vpn_crt_client4_static.disabled = false;
			document.openvpnManualForm.vpn_crt_client4_crl.disabled = false;
			break;
		case "5" :
			document.openvpnManualForm.vpn_crt_client5_ca.value = document.getElementById('edit_vpn_crt_client_ca').value;
			document.openvpnManualForm.vpn_crt_client5_crt.value = document.getElementById('edit_vpn_crt_client_crt').value;
			document.openvpnManualForm.vpn_crt_client5_key.value = document.getElementById('edit_vpn_crt_client_key').value;
			document.openvpnManualForm.vpn_crt_client5_static.value = document.getElementById('edit_vpn_crt_client_static').value;
			document.openvpnManualForm.vpn_crt_client5_crl.value = document.getElementById('edit_vpn_crt_client_crl').value;
			document.openvpnManualForm.vpn_crt_client5_ca.disabled = false;
			document.openvpnManualForm.vpn_crt_client5_crt.disabled = false;
			document.openvpnManualForm.vpn_crt_client5_key.disabled = false;
			document.openvpnManualForm.vpn_crt_client5_static.disabled = false;
			document.openvpnManualForm.vpn_crt_client5_crl.disabled = false;
			break;
	}
	
	document.openvpnManualForm.submit();
	cancel_Key_panel();
}

function startImportCA(unit){
	document.getElementById('importCA').style.display = "";
	document.getElementById('loadingiconCA').style.display = "";
	document.openvpnCAForm.vpn_upload_unit.value = unit;
	setTimeout('ovpnFileChecker();',2000);
	document.openvpnCAForm.submit();
}

function addOpenvpnProfile(){
	document.vpnclientForm.action = "/start_apply.htm";
	document.vpnclientForm.enctype = "application/x-www-form-urlencoded";
	document.vpnclientForm.encoding = "application/x-www-form-urlencoded";
	addRow_Group(10, save_flag, idx_tmp);
}
function pptpOptionChange() {
	document.getElementById("pptpOptionHint").style.display = "none";
	if(document.form.selPPTPOption.value == "+mppe-40") {
		document.getElementById("pptpOptionHint").style.display = "";
	}
}

function gen_vpnc_tab_list(_type) {
	var code = "";
	$('#divTabMenu_' + _type + '').empty();
	code += "<table width='100%' border='0' align='left' cellpadding='0' cellspacing='0' style='table-layout: fixed;'>";
	code += "<tr>";
	code += "<td align='center' id='pptpcTitle_" + _type + "' onclick='tabclickhandler(0);'>PPTP</td>";
	code += "<td align='center' id='l2tpcTitle_" + _type + "' onclick='tabclickhandler(1);'>L2TP</td>";
	if(openvpnd_support) {
		code += "<td align='center' id='opencTitle_" + _type + "' onclick='tabclickhandler(2);'>OpenVPN</td>";
	}
	code += "</tr>";
	code += "</table>";
	$('#divTabMenu_' + _type + '').html(code);
}

function parseArrayToStr_vpnc_clientlist() {
	var vpnc_clientlist_str = "";
	for(var i = 0; i < vpnc_clientlist_array.length; i += 1) {
		if(vpnc_clientlist_array[i].length != 0) {
			if(i != 0)
				vpnc_clientlist_str += "<";
			for(var j = 0; j < vpnc_clientlist_array[i].length; j += 1) {
				vpnc_clientlist_str += vpnc_clientlist_array[i][j];
				if( (j + 1) != vpnc_clientlist_array[i].length)
					vpnc_clientlist_str += ">";
			}
		}
	}
	return vpnc_clientlist_str;
}
function parseArrayToStr_vpnc_pptp_options_x_list() {
	var vpnc_pptp_options_x_list_str = "";
	for(var i = 0; i < vpnc_pptp_options_x_list_array.length; i += 1) {
		if(vpnc_pptp_options_x_list_array[i].length != 0) {
			vpnc_pptp_options_x_list_str += "<";
			for(var j = 0; j < vpnc_pptp_options_x_list_array[i].length; j += 1) {
				vpnc_pptp_options_x_list_str += vpnc_pptp_options_x_list_array[i][j];
			}
		}
	}
	return vpnc_pptp_options_x_list_str;
}
</script>
</head>

<body onload="initial();" onunload="unload_body();">
<div id="TopBanner"></div>
<div id="Loading" class="popup_bg"></div>
<iframe name="hidden_frame" id="hidden_frame" width="0" height="0" frameborder="0"></iframe>
<form method="post" name="form" action="/start_apply.htm" target="hidden_frame">
<input type="hidden" name="current_page" value="Advanced_VPNClient_Content.asp">
<input type="hidden" name="next_page" value="Advanced_VPNClient_Content.asp">
<input type="hidden" name="modified" value="0">
<input type="hidden" name="flag" value="background">
<input type="hidden" name="action_mode" value="apply">
<input type="hidden" name="action_script" value="restart_vpncall">
<input type="hidden" name="action_wait" value="3">
<input type="hidden" name="preferred_lang" id="preferred_lang" value="<% nvram_get("preferred_lang"); %>">
<input type="hidden" name="firmver" value="<% nvram_get("firmver"); %>">
<input type="hidden" name="vpnc_pppoe_username" value="<% nvram_get("vpnc_pppoe_username"); %>">
<input type="hidden" name="vpnc_pppoe_passwd" value="<% nvram_get("vpnc_pppoe_passwd"); %>">
<input type="hidden" name="vpnc_heartbeat_x" value="<% nvram_get("vpnc_heartbeat_x"); %>">
<input type="hidden" name="vpnc_dnsenable_x" value="1">
<input type="hidden" name="vpnc_proto" value="<% nvram_get("vpnc_proto"); %>">
<input type="hidden" name="vpnc_clientlist" value='<% nvram_get("vpnc_clientlist"); %>'>
<input type="hidden" name="vpnc_type" value="PPTP">
<input type="hidden" name="vpnc_auto_conn" value="<% nvram_get("vpnc_auto_conn"); %>">
<input type="hidden" name="vpn_client_unit" value="1">
<input type="hidden" name="vpn_client1_username" value="<% nvram_get("vpn_client1_username"); %>">
<input type="hidden" name="vpn_client1_password" value="<% nvram_get("vpn_client1_password"); %>">
<input type="hidden" name="vpn_client2_username" value="<% nvram_get("vpn_client2_username"); %>">
<input type="hidden" name="vpn_client2_password" value="<% nvram_get("vpn_client2_password"); %>">
<input type="hidden" name="vpn_client3_username" value="<% nvram_get("vpn_client3_username"); %>">
<input type="hidden" name="vpn_client3_password" value="<% nvram_get("vpn_client3_password"); %>">
<input type="hidden" name="vpn_client4_username" value="<% nvram_get("vpn_client4_username"); %>">
<input type="hidden" name="vpn_client4_password" value="<% nvram_get("vpn_client4_password"); %>">
<input type="hidden" name="vpn_client5_username" value="<% nvram_get("vpn_client5_username"); %>">
<input type="hidden" name="vpn_client5_password" value="<% nvram_get("vpn_client5_password"); %>">
<input type="hidden" name="vpn_clientx_eas" value="<% nvram_get("vpn_clientx_eas"); %>">
<input type="hidden" name="vpnc_pptp_options_x" value="<% nvram_get("vpnc_pptp_options_x"); %>">
<input type="hidden" name="vpnc_pptp_options_x_list" value="<% nvram_get("vpnc_pptp_options_x_list"); %>">
<div id="openvpnc_setting"  class="contentM_qis pop_div_bg" style="box-shadow: 1px 5px 10px #000;">
	<table class="QISform_wireless" border=0 align="center" cellpadding="5" cellspacing="0">
		<tr style="height:32px;">
			<td>
				<div id="divTabMenu_pptp"></div>
			</td>
		</tr>
		<tr>
			<td>
				<!---- vpnc_pptp/l2tp start  ---->
				<div>
				<table width="100%" border="1" align="center" cellpadding="4" cellspacing="0" class="FormTable">
			 		<tr>
						<th><#IPConnection_autofwDesc_itemname#></th>
					  <td>
					  	<input type="text" maxlength="64" name="vpnc_des_edit" value="" class="input_32_table" style="float:left;" autocorrect="off" autocapitalize="off"></input>
					  </td>
					</tr>  		
					<tr>
						<th><#BOP_isp_heart_item#></th>
						<td>
							<input type="text" maxlength="64" name="vpnc_svr_edit" value="" class="input_32_table" style="float:left;" autocorrect="off" autocapitalize="off"></input>
						</td>
					</tr>  		
					<tr>
						<th><#HSDPAConfig_Username_itemname#></th>
						<td>
							<input type="text" maxlength="64" name="vpnc_account_edit" value="" class="input_32_table" style="float:left;" autocomplete="off" autocorrect="off" autocapitalize="off"></input>
						</td>
					</tr>  		
					<tr>
						<th><#HSDPAConfig_Password_itemname#></th>
						<td>
							<input type="text" maxlength="64" name="vpnc_pwd_edit" value="" class="input_32_table" style="float:left;" autocomplete="off" autocorrect="off" autocapitalize="off"></input>
						</td>
					</tr>
					<tr id="trPPTPOptions">
						<th><#PPPConnection_x_PPTPOptions_itemname#></th>
						<td>
							<select name="selPPTPOption" class="input_option" onchange="pptpOptionChange();">
								<option value="auto"><#Auto#></option>
								<option value="-mppc"><#No_Encryp#></option>
								<option value="+mppe-40">MPPE 40</option>
								<option value="+mppe-128">MPPE 128</option>
							</select>
							<div id="pptpOptionHint" style="display:none;">
								<span><#PPTPOptions_OpenVPN_hint#><!--untranslated--></span>
							</div>
						</td>	
					</tr>		 
		 		</table>
		 		</div>
		 		<!---- vpnc_pptp/l2tp end  ---->		 			 	
			</td>
		</tr>
	</table>		
	<div style="margin-top:5px;padding-bottom:10px;width:100%;text-align:center;">
		<input class="button_gen" type="button" onclick="cancel_add_rule();" id="cancelBtn" value="<#CTL_Cancel#>">
		<input class="button_gen" type="button" onclick="addRow_Group(10,save_flag, idx_tmp);" value="<#CTL_ok#>">	
	</div>	
      <!--===================================Ending of openvpnc setting Content===========================================-->			
</div>
<table class="content" align="center" cellpadding="0" cellspacing="0">
	<tr>
	<td width="17">&nbsp;</td>
	
	<!--=====Beginning of Main Menu=====-->
	<td valign="top" width="202">
	  <div id="mainMenu"></div>
	  <div id="subMenu"></div>
	</td>
	
    <td valign="top">
			<div id="tabMenu" class="submenuBlock"></div>
		<!--===================================Beginning of Main Content===========================================-->
		<table width="95%" border="0" align="left" cellpadding="0" cellspacing="0" class="FormTitle" id="FormTitle">
  		<tr>
    		<td bgcolor="#4D595D" valign="top">
    			<table width="760px" border="0" cellpadding="4" cellspacing="0">
					<tr>
						<td bgcolor="#4D595D" valign="top">
							<div>&nbsp;</div>
							<div class="formfonttitle">VPN - <#vpnc_title#></div>
							<div style="margin-left:5px;margin-top:10px;margin-bottom:10px"><img src="/images/New_ui/export/line_export.png"></div>
							<div class="formfontdesc">
								<#vpnc_desc1#><br>
								<#vpnc_desc2#><br>
								<#vpnc_desc3#><br><br>		
								<#vpnc_step#>
								<ol>
									<li><#vpnc_step1#>
									<li><#vpnc_step2#>
									<li><#vpnc_step3#>
								</ol>
							</div>
						</td>		
        			</tr>						
        			<tr>
          				<td>
							<table width="98%" border="1" align="center" cellpadding="4" cellspacing="0" class="FormTable_table">
								<thead>
									<tr>
										<td colspan="6" id="VPNServerList" style="border-right:none;height:22px;"><#BOP_isp_VPN_list#> <span id="rules_limit" style="color:#FFFFFF"></span></td>
									</tr>
								</thead>			
									<tr>
										<th width="10%" style="height:30px;"><#PPPConnection_x_WANLink_itemname#></th>
										<th><div><#IPConnection_autofwDesc_itemname#></div></th>
										<th width="15%"><div><#QIS_internet_vpn_type#></div></th>
										<th width="10%"><div><#pvccfg_edit#></div></th>
										<th width="10%"><div><#CTL_del#></div></th>
										<th width="25%"><div><#Connecting#></div></th>
									</tr>											
							</table>          					
							<div id="vpnc_clientlist_Block"></div>
							<div class="apply_gen">
								<input class="button_gen_long" onclick="Add_profile()" type="button" value="<#vpnc_step1#>">
							</div>
          				</td>
        			</tr>        			
      			</table>
			</td>  
      	</tr>
		</table>
		<!--===================================End of Main Content===========================================-->
		</td>		
	</tr>
</table>
</form>
<!---- vpnc_OpenVPN start  ---->
<form method="post" name="vpnclientForm" action="/start_apply.htm" target="hidden_frame">
<input type="hidden" name="current_page" value="Advanced_VPNClient_Content.asp">
<input type="hidden" name="next_page" value="Advanced_VPNClient_Content.asp">
<input type="hidden" name="modified" value="0">
<input type="hidden" name="flag" value="background">
<input type="hidden" name="action_mode" value="apply">
<input type="hidden" name="action_script" value="saveNvram">
<input type="hidden" name="action_wait" value="1">
<input type="hidden" name="preferred_lang" id="preferred_lang" value="<% nvram_get("preferred_lang"); %>">
<input type="hidden" name="firmver" value="<% nvram_get("firmver"); %>">
<input type="hidden" name="vpnc_clientlist" value='<% nvram_get("vpnc_clientlist"); %>'>
<input type="hidden" name="vpn_upload_type" value="ovpn">
<input type="hidden" name="vpn_upload_unit" value="1">
<input type="hidden" name="vpn_client_unit" value="<% nvram_get("vpn_client_unit"); %>">
<input type="hidden" name="vpnc_type" value="PPTP">
<input type="hidden" name="vpnc_proto" value="<% nvram_get("vpnc_proto"); %>" disabled>
<input type="hidden" name="vpn_clientx_eas" value="<% nvram_get("vpn_clientx_eas"); %>">
<input type="hidden" name="vpnc_auto_conn" value="<% nvram_get("vpnc_auto_conn"); %>">
<input type="hidden" name="vpnc_pptp_options_x_list" value="<% nvram_get("vpnc_pptp_options_x_list"); %>">
<input type="hidden" name="vpnc_openvpn_unit_edit" value="1">
<div id="openvpnc_setting_openvpn" class="contentM_qis pop_div_bg" style="box-shadow: 1px 5px 10px #000;"> 
	<table class="QISform_wireless" border=0 align="center" cellpadding="5" cellspacing="0">
		<tr style="height:32px;">
			<td>		
				<div id="divTabMenu_openvpn"></div>
			</td>
		</tr>
		<tr>
			<td>
		 		<div>
		 			<table width="100%" border="1" align="center" cellpadding="4" cellspacing="0" class="FormTable">
		 				<tr>
							<th><#IPConnection_autofwDesc_itemname#></th>
							<td>
								<input type="text" maxlength="64" name="vpnc_openvpn_des" value="" class="input_32_table" style="float:left;" autocorrect="off" autocapitalize="off"></input>
							</td>
						</tr>  			
						<tr>
							<th><#HSDPAConfig_Username_itemname#> (option)</th>
							<td>
								<input type="text" maxlength="64" name="vpnc_openvpn_username" value="" class="input_32_table" style="float:left;" autocomplete="off" autocorrect="off" autocapitalize="off"></input>
							</td>
						</tr>  
						<tr>
							<th><#HSDPAConfig_Password_itemname#> (option)</th>
							<td>
								<input type="text" maxlength="64" name="vpnc_openvpn_pwd" value="" class="input_32_table" style="float:left;" autocomplete="off" autocorrect="off" autocapitalize="off"></input>
							</td>
						</tr>
						<tr>
							<th><#vpn_openvpnc_importovpn#></th>
							<td>
								<input type="file" name="file" class="input" style="color:#FC0;*color:#000;"><br>
								<input class="button_gen" onclick="ImportOvpn(document.vpnclientForm.vpnc_openvpn_unit_edit.value);" type="button" value="<#CTL_upload#>" />
								<img id="loadingicon" style="margin-left:5px;display:none;" src="/images/InternetScan.gif">
								<span id="importOvpnFile" style="display:none;"><#Main_alert_proceeding_desc3#></span>
							</td>
						</tr> 
					</table> 	
</form>	 			
			 		<br>
					<div style="color:#FC0;margin-bottom: 10px;">
						<input type="checkbox" id="cbManualImport" class="input" onclick="manualImport(this.checked)"><#vpn_openvpnc_manual#>
					</div>
					
<form method="post" name="openvpnCAForm" action="vpnupload.cgi" target="hidden_frame" enctype="multipart/form-data">
<input type="hidden" name="current_page" value="Advanced_VPNClient_Content.asp">
<input type="hidden" name="next_page" value="Advanced_VPNClient_Content.asp">
<input type="hidden" name="modified" value="0">
<input type="hidden" name="flag" value="background">
<input type="hidden" name="action_mode" value="apply">
<input type="hidden" name="action_script" value="saveNvram">
<input type="hidden" name="action_wait" value="1">
<input type="hidden" name="preferred_lang" id="preferred_lang" value="<% nvram_get("preferred_lang"); %>">
<input type="hidden" name="firmver" value="<% nvram_get("firmver"); %>">
<input type="hidden" name="vpn_upload_type" value="ca">
<input type="hidden" name="vpn_upload_unit" value="1">
					<table width="100%" border="1" align="center" cellpadding="4" cellspacing="0" class="FormTable" id="caFiled" style="display:none">
					<tr>
						<th><#vpn_openvpnc_importCA#></th>
						<td>								
							<input type="file" name="file" class="input" style="color:#FC0;*color:#000;"><br>
							<input id="importCA" class="button_gen" onclick="startImportCA(document.vpnclientForm.vpnc_openvpn_unit_edit.value);" type="button" value="<#CTL_upload#>" />
							<img id="loadingiconCA" style="margin-left:5px;display:none;" src="/images/InternetScan.gif">	
						</td>
					</tr> 
					</table>
</form>				
<form method="post" name="openvpnCertForm" action="vpnupload.cgi" target="hidden_frame" enctype="multipart/form-data">												
<input type="hidden" name="current_page" value="Advanced_VPNClient_Content.asp">
<input type="hidden" name="next_page" value="Advanced_VPNClient_Content.asp">
<input type="hidden" name="modified" value="0">
<input type="hidden" name="flag" value="background">
<input type="hidden" name="action_mode" value="apply">
<input type="hidden" name="action_script" value="saveNvram">
<input type="hidden" name="action_wait" value="1">
<input type="hidden" name="preferred_lang" id="preferred_lang" value="<% nvram_get("preferred_lang"); %>">
<input type="hidden" name="firmver" value="<% nvram_get("firmver"); %>">
<input type="hidden" name="vpn_upload_type" value="cert">
<input type="hidden" name="vpn_upload_unit" value="1">
					<table width="100%" border="1" align="center" cellpadding="4" cellspacing="0" class="FormTable" id="certFiled" style="display:none">
						<tr>
							<th>Import Cert file</th>
							<td>
								<input type="file" name="file" class="input" style="color:#FC0;*color:#000;"><br>
								<input class="button_gen" onclick="setTimeout('ovpnFileChecker();',2000);document.openvpnCertForm.submit();" type="button" value="<#CTL_upload#>" />					  		
							</td>
						</tr> 					
					</table>
</form>
<form method="post" name="openvpnKeyForm" action="vpnupload.cgi" target="hidden_frame" enctype="multipart/form-data">						
<input type="hidden" name="current_page" value="Advanced_VPNClient_Content.asp">
<input type="hidden" name="next_page" value="Advanced_VPNClient_Content.asp">
<input type="hidden" name="modified" value="0">
<input type="hidden" name="flag" value="background">
<input type="hidden" name="action_mode" value="apply">
<input type="hidden" name="action_script" value="saveNvram">
<input type="hidden" name="action_wait" value="1">
<input type="hidden" name="preferred_lang" id="preferred_lang" value="<% nvram_get("preferred_lang"); %>">
<input type="hidden" name="firmver" value="<% nvram_get("firmver"); %>">
<input type="hidden" name="vpn_upload_type" value="key">
<input type="hidden" name="vpn_upload_unit" value="1">
					<table width="100%" border="1" align="center" cellpadding="4" cellspacing="0" class="FormTable" style="display:none">					
						<tr>
							<th>Import Key file</th>
							<td>
								<input type="file" name="file" class="input" style="color:#FC0;*color:#000;"><br>
								<input class="button_gen" onclick="setTimeout('ovpnFileChecker();',2000);document.openvpnKeyForm.submit();" type="button" value="<#CTL_upload#>" />
							</td>
						</tr> 					
					</table>
</form>
<form method="post" name="openvpnStaticForm" action="vpnupload.cgi" target="hidden_frame" enctype="multipart/form-data">			
<input type="hidden" name="current_page" value="Advanced_VPNClient_Content.asp">
<input type="hidden" name="next_page" value="Advanced_VPNClient_Content.asp">
<input type="hidden" name="modified" value="0">
<input type="hidden" name="flag" value="background">
<input type="hidden" name="action_mode" value="apply">
<input type="hidden" name="action_script" value="saveNvram">
<input type="hidden" name="action_wait" value="1">
<input type="hidden" name="preferred_lang" id="preferred_lang" value="<% nvram_get("preferred_lang"); %>">
<input type="hidden" name="firmver" value="<% nvram_get("firmver"); %>">
<input type="hidden" name="vpn_upload_type" value="static">
<input type="hidden" name="vpn_upload_unit" value="1">
					<table width="100%" border="1" align="center" cellpadding="4" cellspacing="0" class="FormTable" id="staticFiled" style="display:none">	
						<tr>
							<th>Import Static file</th>
							<td>
								<input type="file" name="file" class="input" style="color:#FC0;*color:#000;"><br>
								<input class="button_gen" onclick="setTimeout('ovpnFileChecker();',2000);document.openvpnStaticForm.submit();" type="button" value="<#CTL_upload#>" />					  		
							</td>
						</tr> 
						
					</table>
</form>
					<table width="100%" border="1" align="center" cellpadding="4" cellspacing="0" class="FormTable" id="manualFiled" style="display:none">
						<tr>
							<th><#Manual_Setting_btn#></th>
							<td>
		  					<input class="button_gen" onclick="document.getElementById('manualFiled_panel').style.display='';" type="button" value="<#pvccfg_edit#>">
					  	</td>
						</tr> 
					</table>
<form method="post" name="openvpnManualForm" action="/start_apply.htm" target="hidden_frame">
<input type="hidden" name="current_page" value="Advanced_VPNClient_Content.asp">
<input type="hidden" name="next_page" value="Advanced_VPNClient_Content.asp">
<input type="hidden" name="modified" value="0">
<input type="hidden" name="flag" value="background">
<input type="hidden" name="action_mode" value="apply">
<input type="hidden" name="action_script" value="saveNvram">
<input type="hidden" name="action_wait" value="1">
<input type="hidden" name="preferred_lang" id="preferred_lang" value="<% nvram_get("preferred_lang"); %>">
<input type="hidden" name="firmver" value="<% nvram_get("firmver"); %>">
<input type="hidden" name="vpn_upload_unit" value="1">
<input type="hidden" name="vpn_crt_client1_ca" value="" disabled>
<input type="hidden" name="vpn_crt_client1_crt" value="" disabled>
<input type="hidden" name="vpn_crt_client1_key" value="" disabled>
<input type="hidden" name="vpn_crt_client1_static" value="" disabled>
<input type="hidden" name="vpn_crt_client1_crl" value="" disabled>
<input type="hidden" name="vpn_crt_client2_ca" value="" disabled>
<input type="hidden" name="vpn_crt_client2_crt" value="" disabled>
<input type="hidden" name="vpn_crt_client2_key" value="" disabled>
<input type="hidden" name="vpn_crt_client2_static" value="" disabled>
<input type="hidden" name="vpn_crt_client2_crl" value="" disabled>
<input type="hidden" name="vpn_crt_client3_ca" value="" disabled>
<input type="hidden" name="vpn_crt_client3_crt" value="" disabled>
<input type="hidden" name="vpn_crt_client3_key" value="" disabled>
<input type="hidden" name="vpn_crt_client3_static" value="" disabled>
<input type="hidden" name="vpn_crt_client3_crl" value="" disabled>
<input type="hidden" name="vpn_crt_client4_ca" value="" disabled>
<input type="hidden" name="vpn_crt_client4_crt" value="" disabled>
<input type="hidden" name="vpn_crt_client4_key" value="" disabled>
<input type="hidden" name="vpn_crt_client4_static" value="" disabled>
<input type="hidden" name="vpn_crt_client4_crl" value="" disabled>
<input type="hidden" name="vpn_crt_client5_ca" value="" disabled>
<input type="hidden" name="vpn_crt_client5_crt" value="" disabled>
<input type="hidden" name="vpn_crt_client5_key" value="" disabled>
<input type="hidden" name="vpn_crt_client5_static" value="" disabled>
<input type="hidden" name="vpn_crt_client5_crl" value="" disabled>
					<div id="manualFiled_panel" class="contentM_qis_manual" style="display:none">
						<table class="QISform_wireless" border=0 align="center" cellpadding="5" cellspacing="0">											
							<tr>
								<td valign="top">
					     		<table class="QISform_wireless" border=0 align="center" cellpadding="5" cellspacing="0">
					         		<tr>
					         			<td>
											<div class="description_down"><#vpn_openvpn_Keys_Cert#></div>
										</td>		
									</tr>
									<tr>
										<td>
											<div style="margin-left:30px; margin-top:10px;">
												<p><#vpn_openvpn_KC_Edit1#> <span style="color:#FC0;">----- BEGIN xxx ----- </span>/<span style="color:#FC0;"> ----- END xxx -----</span> <#vpn_openvpn_KC_Edit2#>
												<p><#vpn_openvpn_KC_Limit#>
											</div>													
											<div style="margin:5px;*margin-left:-5px;"><img style="width: 700px; height: 2px;" src="/images/New_ui/export/line_export.png"></div>
										</td>	
									</tr>
									<tr>
										<td valign="top">
											<table width="100%" border="1" align="center" cellpadding="4" cellspacing="0" class="FormTable">
												<tr>
													<th id="manualCa">Certificate Authority</th>
													<td>
														<textarea rows="8" class="textarea_ssh_table" id="edit_vpn_crt_client_ca" name="edit_vpn_crt_client_ca" cols="65" maxlength="3999"></textarea>
													</td>
												</tr>
												<tr>
													<th id="manualCert">Client Certificate</th>
													<td>
														<textarea rows="8" class="textarea_ssh_table" id="edit_vpn_crt_client_crt" name="edit_vpn_crt_client_crt" cols="65" maxlength="3999"></textarea>
													</td>
												</tr>
												<tr>
													<th id="manualKey">Client Key</th>
													<td>
														<textarea rows="8" class="textarea_ssh_table" id="edit_vpn_crt_client_key" name="edit_vpn_crt_client_key" cols="65" maxlength="3999"></textarea>
													</td>
												</tr>
												<tr>
													<th id="manualStatic">Static Key (Optional)</th>
													<td>
														<textarea rows="8" class="textarea_ssh_table" id="edit_vpn_crt_client_static" name="edit_vpn_crt_client_static" cols="65" maxlength="3999"></textarea>
													</td>
												</tr>
												<tr>
													<th id="manualCRList">Certificate Revocation List (Optional)</th>
													<td>
														<textarea rows="8" class="textarea_ssh_table" id="edit_vpn_crt_client_crl" name="edit_vpn_crt_client_crl" cols="65" maxlength="3999"></textarea>
													</td>
												</tr>
											</table>
								  		</td>
								  	</tr>				
					  			</table>
									<div style="margin-top:5px;width:100%;text-align:center;">
										<input class="button_gen" type="button" onclick="cancel_Key_panel();" value="<#CTL_Cancel#>">
										<input class="button_gen" type="button" onclick="saveManual(document.vpnclientForm.vpnc_openvpn_unit_edit.value);" value="<#CTL_onlysave#>">	
									</div>					
								</td>
					  		</tr>
						</table>	
</form>					  
					</div>
				</div>
			</td>
		</tr>
	</table>
	<div style="margin-top:5px;padding-bottom:10px;width:100%;text-align:center;">
		<input class="button_gen" type="button" onclick="cancel_add_rule();" id="cancelBtn_openvpn" value="<#CTL_Cancel#>">
		<input class="button_gen" onclick='addOpenvpnProfile();' type="button" value="<#CTL_ok#>">
	</div>	
      <!--===================================Ending of openvpn setting Content===========================================-->			
</div>
<div id="footer"></div>
</body>
</html>
