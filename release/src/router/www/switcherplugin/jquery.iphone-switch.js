jQuery.fn.iphoneSwitch = function(start_state, switched_on_callback, switched_off_callback, options) {
	var state = start_state == '1' ? start_state : '0';

	// define default settings
	var settings = {
		mouse_over: 'pointer',
		mouse_out:  'default',
		switch_container_path: '/switcherplugin/iphone_switch_container_on.png',
		switch_path: '/switcherplugin/iphone_switch.png',
		switch_height: 32,
		switch_width: 74
	};

	if(options) {
		jQuery.extend(settings, options);
	}

	// create the switch
	return this.each(function() {

		var container;
		var image;
		
		// make the container
		container = '<div class="iphone_switch_container" style="height:'+settings.switch_height+'px; width:'+settings.switch_width+'px; position: relative; overflow: hidden">';
		
		// make the switch image based on starting state
		image = '<img id="iphone_switch" class="iphone_switch" src="'+settings.switch_container_path+'" style="border-radius:7px;height:'+settings.switch_height+'px; width:'+settings.switch_width+'px; background-image:url('+settings.switch_path+'); background-repeat:no-repeat; background-position:'+(state == '1' ? 0 : -37)+'px" /></div>';

		// insert into placeholder
		jQuery(this).html(container + image);

		jQuery(this).mouseover(function(){
			jQuery(this).css("cursor", settings.mouse_over);
		});

		jQuery(this).mouseout(function(){
			jQuery(this).css("background", settings.mouse_out);
		});

		// click handling
		jQuery(this).unbind("click"); // initial click event
		jQuery(this).click(function() {
			var radioControllerList = "";
			radioControllerList += "(";
			radioControllerList += "stream_ad_enable|pop_ad_enable|"; //AiProtection_AdBlock
			radioControllerList += "radio_protection_enable|radio_mals_enable|radio_vp_enable|radio_cc_enable|"; //AiProtection_HomeProtection
			radioControllerList += "radio_web_restrict_enable|"; //AiProtection_AppProtector, AiProtection_WebProtector
			radioControllerList += "traffic_analysis_enable|"; //TrafficAnalyzer_Statistic
			radioControllerList += "apps_analysis_enable|"; //AdaptiveQoS_Bandwidth_Monitor
			radioControllerList += "bwdpi_wh_enable|"; //AdaptiveQoS_WebHistory
			radioControllerList += "radio_clouddisk_enable|"; //Cloud_main
			radioControllerList += "radio_wps_enable|"; //Advanced_WWPS_Content
			radioControllerList += "nm_radio_dualwan_enable|"; //Internet
			radioControllerList += "simdetect_switch|"; //Advanced_MobileBroadband_Content
			radioControllerList += "dns_switch|"; //Advanced_IPTV_Content
			radioControllerList += "radio_fbwifi_enable|"; //Guest_network_fbwifi
			radioControllerList += "vlan_enable|"; //Advanced_TagBasedVLAN_Content
			radioControllerList += "ad_radio_dualwan_enable|"; //Advanced_WANPort_Content
			radioControllerList += ")";
			var radioControllerArrayRE = new RegExp(radioControllerList, "i");
			if(Boolean(this.id.match(radioControllerArrayRE)) && typeof(curState) != "undefined") {
				state = curState;
			}
			else if(this.id.length > 18 && this.id.substr(0, 18) == "wtfast_rule_enable"){
				var index = (this.id).substr(18);
				var index_int = parseInt(index);
				state = rule_enable_array[index_int];
			}
			else if(this.id.substr(0, 16) == "vlan_rule_enable"){
				var index = (this.id).substr(16);
			}

			if((this.id == "wandhcp_switch") && typeof(curWandhcpState))
				state = curWandhcpState;

			if(state == '1') {
				jQuery(this).find('.iphone_switch').animate({backgroundPosition: -37}, "slow", function() {
					jQuery(this).attr('src', settings.switch_container_path);
					if(typeof(index))
						switched_off_callback(index);
					else
					switched_off_callback();
				});
				state = '0';
			}
			else {
				jQuery(this).find('.iphone_switch').animate({backgroundPosition: 0}, "slow", function() {
					jQuery(this).find('.iphone_switch').attr('src', settings.switch_container_path);
					if(typeof(index))
						switched_on_callback(index);
					else
					switched_on_callback();
				});
				state = '1';
			}
		});		

	});	
	
};
