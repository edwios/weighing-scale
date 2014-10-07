-- Replace <<Access_token>> using your Spark.io access token
-- Replace all <<CoreID>> by your Spare Core's core IDs
-- Do NOT include the << and >> in the above!

-- Software provided as is and available under MIT License
-- Written by Ed Wios, Sept 2014.

set ACCESS_TOKEN to "<<Access_token>>"
set myCores to {"Weigh scale", "Some other core"}
set CoreIDList to {{key:"Weigh scale", value:"<<CoreID>>"}, {key:"Some other core", value:"<<CoreID>>"}}
choose from list myCores with title "Which Core?" with prompt "Pick the weighing scale Core, Cancel to abort."
if the result is false then
	return
else
	set ans to the result
	set selectedCore to ans as text
	repeat with theCore in CoreIDList
		if (key of theCore) as text is equal to selectedCore then
			set coreID to the value of theCore as text
		end if
	end repeat
	display dialog "Function name?" with title selectedCore & coreID default answer "calibrate"
	set funcName to text returned of result
	
	display dialog "Parameter values?" default answer "500"
	set paramValues to text returned of result
	
	set theURL to "https://api.spark.io/v1/devices/" & coreID & "/" & funcName
	set theParams to "params=" & paramValues
	do shell script "curl -d access_token=" & ACCESS_TOKEN & " -d " & theParams & " " & quoted form of theURL
	
end if
