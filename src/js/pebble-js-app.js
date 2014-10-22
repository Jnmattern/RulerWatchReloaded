var invert = 0;
var vibration = 0;
var legacy = 0;
var battery = 0;

function logVariables() {
	console.log("	invert: " + invert);
  console.log("	vibration: " + vibration);
  console.log("	legacy: " + vibration);
  console.log("	battery: " + battery);
}

Pebble.addEventListener("ready", function() {
	console.log("Ready Event");
	invert = localStorage.getItem("invert");
	if (!invert) {
		invert = 0;
	}
	
	vibration = localStorage.getItem("vibration");
	if (!vibration) {
		vibration = 0;
	}

	legacy = localStorage.getItem("legacy");
	if (!legacy) {
		legacy = 0;
	}

	battery = localStorage.getItem("battery");
	if (!battery) {
		battery = 0;
	}

	logVariables();
						
	Pebble.sendAppMessage(JSON.parse('{"invert":'+invert+',"vibration":'+vibration+',"legacy":'+legacy+',"battery":'+battery+'}'));
});

Pebble.addEventListener("showConfiguration", function(e) {
	console.log("showConfiguration Event");

	logVariables();
						
	Pebble.openURL("http://www.famillemattern.com/jnm/pebble/Ruler/Ruler_3.5.php?invert=" + invert + "&vibration=" + vibration + "&legacy=" + legacy + "&battery=" + battery );
});

Pebble.addEventListener("webviewclosed", function(e) {
	console.log("Configuration window closed");
	console.log(e.type);
	console.log(e.response);

	var configuration = JSON.parse(e.response);
	Pebble.sendAppMessage(configuration);
	
	invert = configuration["invert"];
	localStorage.setItem("invert", invert);
	
	vibration = configuration["vibration"];
	localStorage.setItem("vibration", vibration);

  legacy = configuration["legacy"];
	localStorage.setItem("legacy", legacy);

  battery = configuration["battery"];
	localStorage.setItem("battery", battery);
});
