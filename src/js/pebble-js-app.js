var invert = 0;
var vibration = 0;
var legacy = 0;

function logVariables() {
	console.log("	invert: " + invert);
  console.log("	vibration: " + vibration);
  console.log("	legacy: " + vibration);
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

	logVariables();
						
	Pebble.sendAppMessage(JSON.parse('{"invert":'+invert+',"vibration":'+vibration+',"legacy":'+legacy+'}'));
});

Pebble.addEventListener("showConfiguration", function(e) {
	console.log("showConfiguration Event");

	logVariables();
						
	Pebble.openURL("http://www.famillemattern.com/jnm/pebble/Ruler/Ruler_3.4.php?invert=" + invert + "&vibration=" + vibration + "&legacy=" + legacy );
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

});
