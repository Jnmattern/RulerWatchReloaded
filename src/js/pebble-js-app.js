var invert = 0;
var vibration = 0;
var legacy = 0;
var battery = 0;
var dateonshake = 0;
var dial = 1;
var pebblefont = 0;
var themecode = "d0e0e0d0c0c0c0e0fdfdf4";

function logVariables() {
	console.log("	invert: " + invert);
  console.log("	vibration: " + vibration);
  console.log("	legacy: " + vibration);
  console.log("	battery: " + battery);
  console.log("	dateonshake: " + dateonshake);
  console.log("	dial: " + dial);
  console.log("	pebblefont: " + pebblefont);
  console.log("	themecode: " + themecode);
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

	pebblefont = localStorage.getItem("pebblefont");
	if (!pebblefont) {
		pebblefont = 0;
	}

	themecode = localStorage.getItem("themecode");
	if (themecode === null) {
		themecode = "d0e0e0d0c0c0c0e0fdfdf4";
	}

	logVariables();
						
	Pebble.sendAppMessage(JSON.parse('{"invert":'+invert+',"vibration":'+vibration+',"legacy":'+legacy+',"battery":'+battery+
    ',"dateonshake":'+dateonshake+',"dial":'+dial+',"pebblefont":'+pebblefont+',"themecode":"'+themecode+'"}'));
});

function isWatchRound() {
  if (typeof Pebble.getActiveWatchInfo === "function") {
    try {
      if (Pebble.getActiveWatchInfo().platform != 'chalk') {
        return false;
      } else {
        return true;
      }
    } catch(err) {
      console.log('ERROR calling Pebble.getActiveWatchInfo() : ' + JSON.stringify(err));
      // Assuming Pebble is not round
      return false;
    }
  }
}

function isWatchColorCapable() {
  if (typeof Pebble.getActiveWatchInfo === "function") {
    try {
      if (Pebble.getActiveWatchInfo().platform != 'aplite') {
        return true;
      } else {
        return false;
      }
    } catch(err) {
      console.log('ERROR calling Pebble.getActiveWatchInfo() : ' + JSON.stringify(err));
      // Assuming Pebble App 3.0
      return true;
    }
  }
  //return ((typeof Pebble.getActiveWatchInfo === "function") && Pebble.getActiveWatchInfo().platform!='aplite');
}

Pebble.addEventListener("showConfiguration", function(e) {
	console.log("showConfiguration Event");

	logVariables();

  var url = "http://www.famillemattern.com/jnm/pebble/Ruler/Ruler_3.14.html?invert=" + invert + "&vibration=" + vibration +
    "&legacy=" + legacy + "&battery=" + battery + "&dateonshake=" + dateonshake + "&dial=" + dial + "&themecode=" + themecode +
    "&colorCapable=" + isWatchColorCapable() + "&round=" + isWatchRound() + "&pebblefont=" + pebblefont;

  console.log("Opening URL: "+url);

	Pebble.openURL(url);
});

Pebble.addEventListener("webviewclosed", function(e) {
	console.log("Configuration window closed");
	console.log(e.type);
  console.log("Response: " + decodeURIComponent(e.response));

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

  pebblefont = configuration["pebblefont"];
	localStorage.setItem("pebblefont", pebblefont);

  themecode = configuration["themecode"];
	localStorage.setItem("themecode", themecode);
});
