<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<meta http-equiv="X-UA-Compatible" content="IE=Edge"/>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
<meta HTTP-EQUIV="Pragma" CONTENT="no-cache">
<meta HTTP-EQUIV="Expires" CONTENT="-1">
<link rel="shortcut icon" href="images/favicon.png">
<link rel="icon" href="images/favicon.png">
<title><#Web_Title#> - Blocking Page</title>
<script type="text/JavaScript" src="/js/jquery.js"></script>
<script type="text/javascript" src="/require/require.min.js"></script>
<style>
body{
	color:#FFF;
	font-family: Arial;
}
.wrapper{
	background-size: 1330px 580px;
}

.title_div {
		padding-top:45px;
		text-align: center;
}

.title_name {
	font-family: Arial;
	font-weight:bold;
	font-size: 29px;
	color: #181818;
}

.title_div1 {
	padding-top:8px;
	text-align: center;
}

.title_text {
	font-family: Arial;
	font-weight:bold;
	font-size: 18px;
	color: #181818
}

.title_div2 {
	padding-top:12px;
	text-align: center;
}

.span_word {
	font-family: Arial;
	font-weight:bold;
	font-size: 15px;
	color: #181818
}

.div_img {
	text-align:center;
	padding-top: 20px;
}

.blocking_img{
	width:372px;
	height:303px;
	background-position:center;
	background-attachment:fixed;
	background:url(images/New_ui/asus_hive_block_internet_web.png) no-repeat center;
	margin:auto;
}

.block_info_div {
	width: 100%;
	padding-top:30px;
}

.block_info_div1 {
	width:60%;
	margin: 0px 200px auto auto;
}

.suggestion_div{
	padding-top:10px;
}

.block_info_word {
	font-family: Arial;
	font-weight:bold;
	font-size: 17px;
	color: #181818;
}

ul {
	margin-left: -21px;
	color: #28d1a5;
}

li span {
	font-family: Arial;
	font-weight:bold;
	font-size: 15px;
	color: #181818;
}

.button{
	width: 240px;
	height: 50px;
	background-color: #FFFFFF; /* Can be set to transparent */
	border: 2px #2adbb9;
	border-style:solid;
	-webkit-border-radius: 25px;
	text-align: center;
	margin: auto;
	cursor:pointer;
}

.wait_button{
	width: 240px;
	height: 50px;
	background-color: #FFFFFF; /* Can be set to transparent */
	border: 2px #475A5F;
	border-style:solid;
	-webkit-border-radius: 25px;
	text-align: center;
	margin: auto;
}

.circle_span{
	color: #2adbb9;
	font-size: 25px;
	line-height:50px;
}

.wait_circle_span{
	color: #475A5F;
	font-size: 25px;
	line-height:50px;
}

.button_div{
	padding-top:50px;
	text-align: center;
}

.rtime_div{
	text-align: center;
}

.rtime_span{
	color: #475A5F;
	font-size: 20px;
	line-height:50px;
}

/*for mobile device*/
@media screen and (max-width: 1000px){
	.title_div {
		padding-top:30px;
		text-align: center;
	}

	.title_name {
		font-size: 24px;
	}

	.title_div1 {
		padding-top:4px;
	}

	.title_text {
		font-size: 16px;
	}

	.title_div2 {
		padding-top:9px;
	}

	.span_word {
		font-size: 14px;
	}

	.div_img {
		padding-top: 3px;
	}

	.blocking_img{
		background-size: 75%;
	}

	.block_info_div {
		padding-top:3px;
	}

	.block_info_div1 {
		width:88%;
		margin: auto;
	}

	.block_info_word {
		font-size: 16px;
	}

	li span {
		font-size: 14px;
	}

	.suggest_div{
		padding-top:20px;
	}

	.button_div{
		padding-top:30px;
		text-align: center;
}
}
</style>
<script type="text/javascript">

var bwdpi_support = ('<% nvram_get("rc_support"); %>'.search('bwdpi') == -1) ? false : true;
var mac_parameter = '<% get_parameter("mac"); %>'.toUpperCase();
var client_list_array = '<% get_client_detail_info(); %>';
var custom_name = decodeURIComponent('<% nvram_char_to_ascii("", "custom_clientlist"); %>').replace(/&#62/g, ">").replace(/&#60/g, "<");
var block_wait_list = decodeURIComponent('<% nvram_char_to_ascii("", "MULTIFILTER_TMP_T"); %>').replace(/&#62/g, ">").replace(/&#60/g, "<");
var block_info ="";
var block_wait_match_flag = 0;
var countdownid, rtime_obj;
var remaining_time;

var target_info = {
	name: "",
	mac: ""
}

function initial(){
	get_target_info();
	change_request_button();
}

function change_request_button(){
	var block_wait_list_row = block_wait_list.split('<');

	for(i=1;i<block_wait_list_row.length;i++){
		var block_wait_name_col = block_wait_list_row[i].split('>');
		if(block_wait_name_col[0] == mac_parameter && (block_wait_name_col[1] - Math.floor(Date.now()/1000) > 0))
			block_wait_match_flag = 1;
	}

	if(block_wait_match_flag == 1){
		disable_button(1);
		remaining_time = block_wait_name_col[1] - Math.floor(Date.now()/1000);
		rtime_obj=document.getElementById("rtime");
		rtime_obj.innerHTML=secondsToTime(remaining_time);
		countdownid = window.setInterval(countdownfunc,1000);
	}
}

function disable_button(val){
	if(val == 1){
		document.getElementById("rtime").style.display ="";
		document.getElementById("request_button_span").className = "wait_circle_span";
		document.getElementById("request_button").className = "wait_button";
		document.getElementById('request_button_span').innerHTML = "REQUESTING";
	}else{
		document.getElementById("rtime").style.display ="none";
		document.getElementById("request_button_span").className = "circle_span";
		document.getElementById("request_button").className = "button";
		document.getElementById('request_button_span').innerHTML = "REQUEST";
	}
}

function countdownfunc(){
	rtime_obj.innerHTML=secondsToTime(remaining_time);
	if (remaining_time==0){
		clearInterval(countdownid);
		setTimeout(refresh_page(), 2000);
	}
	remaining_time--;
}

function refresh_page(){
	top.location.href='/blocking.asp?mac='+mac_parameter;
}

function secondsToTime(secs)
{
    secs = Math.round(secs);
    var hours = Math.floor(secs / (60 * 60));

    var divisor_for_minutes = secs % (60 * 60);
    var minutes = Math.floor(divisor_for_minutes / 60);

    var divisor_for_seconds = divisor_for_minutes % 60;
    var seconds = Math.ceil(divisor_for_seconds);

    var time_str = (minutes<10?("0"+minutes):minutes)+":"+(seconds<10?("0"+seconds):seconds);
    return time_str;
}

function get_target_info(){

	var block_hostname = "";
	var client_list_row = client_list_array.split('<');
	var custom_name_row = custom_name.split('<');
	var match_flag = 0;

	for(i=1;i<custom_name_row.length;i++){
		var custom_name_col = custom_name_row[i].split('>');
		if(custom_name_col[1] == block_info[0] || custom_name_col[1] == mac_parameter){
			target_info.name = custom_name_col[0];
			match_flag =1;
		}
	}

	if(match_flag == 0){
		for(i=1;i< client_list_row.length;i++){
			var client_list_col = client_list_row[i].split('>');
			if(client_list_col[3] == block_info[0] || client_list_col[3] == mac_parameter){
				target_info.name = client_list_col[1];
			}
		}
	}

	target_info.mac = mac_parameter;

	if(target_info.name == target_info.mac)
		block_hostname += target_info.name;
	else
		block_hostname += target_info.name + " (" + target_info.mac.toUpperCase() + ")";

	document.getElementById('block_hostname').innerHTML = block_hostname;
}

function apply(){
	if(block_wait_match_flag == 1) return;
	var timestap_now = Date.now();
	var timestr_start = Math.floor(timestap_now/1000);
	var timestr_end = Math.floor(timestap_now/1000) + 3600;
	var timestap_start = new Date(timestap_now);
	var timestap_end = new Date(timestap_now + 3600000);
	var timestap_start_hours = (timestap_start.getHours()<10? "0" : "") + timestap_start.getHours();
	var timestap_end_hours = (timestap_end.getHours()<10? "0" : "") + timestap_end.getHours();
	var timestap_start_Min = (timestap_start.getMinutes()<10? "0" : "") + timestap_start.getMinutes();
	var timestap_end_Min = (timestap_end.getMinutes()<10? "0" : "") + timestap_end.getMinutes();

	var interval = timestap_start.getDay().toString() + timestap_end.getDay().toString() + timestap_start_hours + timestap_end_hours + timestap_start_Min + timestap_end_Min;

	document.form.CName.value = target_info.name;
	document.form.mac.value = target_info.mac;
	document.form.timestap.value = timestr_end;
	document.form.interval.value = interval;
	document.form.submit();
}

</script>
</head>
<body class="" onload="initial();">
<iframe name="hidden_frame" id="hidden_frame" width="0" height="0" frameborder="0"></iframe>
<form method="post" name="form" action="blocking_request.cgi" target="hidden_frame">
<input type="hidden" name="CName" value="">
<input type="hidden" name="mac" value="">
<input type="hidden" name="timestap" value="">
<input type="hidden" name="interval" value="">
<input type="hidden" name="current_page" value="blocking.asp">
<input type="hidden" name="next_page" value="blocking.asp">
	<div class="wrapper">
		<div class="title_div">
			<span class="title_name">Oops!</span>
		</div>
		<div class="title_div1">
			<span class="title_text"><#block_TS_title#></span>
		</div>
		<div class="title_div2">
			<span class="span_word"><#Web_Title2#></span>
		</div>
		<div class="div_img">
			<div class="blocking_img"></div>
		</div>
		<div class="block_info_div">
			<div class="block_info_div1">
				<div>
					<span class="block_info_word"><#block_DetailInfo#></span>
				</div>
				<div style="margin-top: -10px;">
					<ul>
							<li><span>You can’t access the Internet now.</span></li>
							<li style="padding-top:5px"><span>Device Name (MAC): </span><span id="block_hostname"></span></li>
					</ul>
				</div>
				<div class="suggestion_div">
					<span class="block_info_word"><#web_redirect_suggestion0#>:</span>
				</div>
				<div style="margin-top: -10px;">
					<ul>
							<li><span id ="suggestion_word">Click request to ask exception from your parents.</span></li>
					</ul>
				</div>
			</div>
		</div>
		<div class="button_div">
			<div class="button" id="request_button" onclick="apply();">
				<span class="circle_span" id="request_button_span">REQUEST</span>
			</div>
		</div>
		<div class="rtime_div"><span class="rtime_span" id="rtime"></span></div>
	</div>
</form>
</body>
</html>

