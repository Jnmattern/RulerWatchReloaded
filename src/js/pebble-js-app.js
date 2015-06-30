var invert = 0;
var vibration = 0;
var legacy = 0;
var battery = 0;
var dateonshake = 0;
var dial = 1;

function logVariables() {
	console.log("	invert: " + invert);
  console.log("	vibration: " + vibration);
  console.log("	legacy: " + vibration);
  console.log("	battery: " + battery);
  console.log("	dateonshake: " + dateonshake);
  console.log("	dial: " + dial);
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

	dateonshake = localStorage.getItem("dateonshake");
	if (!dateonshake) {
		dateonshake = 1;
	}

	dial = localStorage.getItem("dial");
	if (!dial) {
		dial = 1;
	}

	logVariables();
						
	Pebble.sendAppMessage(JSON.parse('{"invert":'+invert+',"vibration":'+vibration+',"legacy":'+legacy+',"battery":'+battery+
    ',"dateonshake":'+dateonshake+',"dial":'+dial+'}'));
});

Pebble.addEventListener("showConfiguration", function(e) {
	console.log("showConfiguration Event");

	logVariables();

  var url = "http://www.famillemattern.com/jnm/pebble/Ruler/Ruler_3.8.html?invert=" + invert + "&vibration=" + vibration +
    "&legacy=" + legacy + "&battery=" + battery + "&dateonshake=" + dateonshake + "&dial=" + dial;

  console.log("Opening URL: "+url);

	Pebble.openURL(url);
});

Pebble.addEventListener("webviewclosed", function(e) {
	console.log("Configuration window closed");
	console.log(e.type);
	console.log(e.response);

	var configuration = JSON.parse(decodeURIComponent(e.response));
  Pebble.sendAppMessage(configuration);
	
	invert = configuration["invert"];
	localStorage.setItem("invert", invert);
	
	vibration = configuration["vibration"];
	localStorage.setItem("vibration", vibration);

  legacy = configuration["legacy"];
	localStorage.setItem("legacy", legacy);

  battery = configuration["battery"];
	localStorage.setItem("battery", battery);

  dateonshake = configuration["dateonshake"];
	localStorage.setItem("dateonshake", dateonshake);

  dial = configuration["dial"];
	localStorage.setItem("dial", dial);
});
