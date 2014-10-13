<!DOCTYPE html>
<html>
        <head>
                <title>Ruler</title>
                <meta charset='utf-8'>
                <meta name='viewport' content='width=device-width, initial-scale=1'>
                <link rel='stylesheet' href='http://code.jquery.com/mobile/1.3.2/jquery.mobile-1.3.2.min.css' />
                <script src='http://code.jquery.com/jquery-1.9.1.min.js'></script>
                <script src='http://code.jquery.com/mobile/1.3.2/jquery.mobile-1.3.2.min.js'></script>
                <style>
                        .ui-header .ui-title { margin-left: 1em; margin-right: 1em; text-overflow: clip; }
                </style>
        </head>
		<body>
<div data-role="page" id="page1">
    <div data-theme="a" data-role="header" data-position="fixed">
        <h3>
            Ruler Watch Configuration
        </h3>
        <div class="ui-grid-a">
            <div class="ui-block-a">
                <input id="cancel" type="submit" data-theme="c" data-icon="delete" data-iconpos="left"
                value="Cancel" data-mini="true">
            </div>
            <div class="ui-block-b">
                <input id="save" type="submit" data-theme="b" data-icon="check" data-iconpos="right"
                value="Save" data-mini="true">
            </div>
        </div>
    </div>
    <div data-role="content">

	<div data-role="fieldcontain">
            <label for="invert">
                Invert Colors
			</label>
            <select name="invert" id="invert" data-theme="" data-role="slider" data-mini="true">
<?php
	if (!isset($_GET['invert'])) {
		$invert = 0;
	} else {
		$invert = $_GET['invert'];
	}
	
	if ($invert == 0) {
		$s1 = " selected";
		$s2 = "";
	} else {
		$s1 = "";
		$s2 = " selected";
	}
	echo '<option value="0"' . $s1 .'>Off</option><option value="1"' . $s2 . '>On</option>';
?>
            </select>
        </div>


        <div data-role="fieldcontain">
            <label for="vibration">
                Vibrate on Hour
            </label>
            <select name="vibration" id="vibration" data-theme="" data-role="slider" data-mini="true">
<?php
	if (!isset($_GET['vibration'])) {
		$vibration = 0;
	} else {
		$vibration = $_GET['vibration'];
	}
	
	if ($vibration == 0) {
		$s1 = " selected";
		$s2 = "";
	} else {
		$s1 = "";
		$s2 = " selected";
	}
	echo '<option value="0"' . $s1 .'>Off</option><option value="1"' . $s2 . '>On</option>';
?>
            </select>
        </div>
	</div>
	</div>
</div>

    <script>
      function saveOptions() {
        var options = {
			'invert': parseInt($("#invert").val(), 10),
			'vibration': parseInt($("#vibration").val(), 10)
        }
        return options;
      }

      $().ready(function() {
        $("#cancel").click(function() {
          console.log("Cancel");
          document.location = "pebblejs://close#";
        });

        $("#save").click(function() {
          console.log("Submit");
          
          var location = "pebblejs://close#" + encodeURIComponent(JSON.stringify(saveOptions()));
          console.log("Close: " + location);
          console.log(location);
          document.location = location;
        });

      });
    </script>
</body>
</html>
