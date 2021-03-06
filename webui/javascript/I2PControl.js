var I2PControl = I2PControl || {}

I2PControl.Session = function(password) {
    this.token = "";
    this.ready = false;
    this.error = false;
    this.password = password;
};

I2PControl.Session.prototype = {

    request : function(method, params, handler) {
        var request = new XMLHttpRequest();
        request.open("POST", "", true);
        request.setRequestHeader('Content-Type', 'application/json');
        var self = this;
        request.onreadystatechange = function() {
            if(this.readyState == 4 && this.status == "200" && this.responseText != "") {
                var data = JSON.parse(this.responseText);
                if(data.hasOwnProperty("error")) {
                    if(data["error"]["code"] == -32003 || data["error"]["code"] == -32004) {
                        // Get a new token and resend the request
                        self.start(function() {
                            self.request(method, params, handler);
                        }); 
                        return;
                    }
                    // Cannot fix the error, report it
                    self.error = data["error"];
                }
                handler(data.result, self);
            }
        };
        if(this.token != "")
            params["Token"] = this.token;

        var rpc = {
           "id" : 0,
           "method" : method ,
           "params" : params,
           "jsonrpc": "2.0"
        }
        request.send(JSON.stringify(rpc));
    },

    start : function(onReady) {
        var self = this;

        var handleAuthenticate = function(result) {
            self.token = result["Token"];
            self.ready = true;
            onReady();
        };

        this.request(
            "Authenticate",
            {"API" : 1, "Password" : this.password},
            handleAuthenticate
        );
    },

};

I2PControl.statusToString = function(status) {
    switch(status) {
        case 0:  return "OK";
        case 1:  return "TESTING";
        case 2:  return "FIREWALLED";
        case 3:  return "HIDDEN";
        case 4:  return "WARN_FIREWALLED_AND_FAST";
        case 5:  return "WARN_FIREWALLED_AND_FLOODFILL";
        case 6:  return "WARN_FIREWALLED_WITH_INBOUND_TCP";
        case 7:  return "WARN_FIREWALLED_WITH_UDP_DISABLED";
        case 8:  return "ERROR_I2CP";
        case 9:  return "ERROR_CLOCK_SKEW";
        case 10: return "ERROR_PRIVATE_TCP_ADDRESS";
        case 11: return "ERROR_SYMMETRIC_NAT";
        case 12: return "ERROR_UDP_PORT_IN_USE";
        case 13: return "ERROR_NO_ACTIVE_PEERS_CHECK_CONNECTION_AND_FIREWALL";
        case 14: return "ERROR_UDP_DISABLED_AND_TCP_UNSET";
        default: return "UNKNOWN";
    }
};

I2PControl.msToString = function(mseconds) {
    var seconds = mseconds / 1000;
    var numdays = Math.floor(seconds / 86400);
    var numhours = Math.floor((seconds % 86400) / 3600);
    var numminutes = Math.floor(((seconds % 86400) % 3600) / 60);
    var numseconds = ((seconds % 86400) % 3600) % 60;

    return numdays + "d " + numhours + "h " + numminutes + "m " + numseconds + "s";
}

I2PControl.updateDocument = function(values) {

    for(id in values) {
        if(!values.hasOwnProperty(id))
            continue;
        document.getElementById(id).innerHTML = values[id];
    }
};
