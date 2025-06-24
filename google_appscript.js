function doGet(e) {

    // First create the output with JSON mime type
    var output = ContentService.createTextOutput();
    output.setMimeType(ContentService.MimeType.JSON);

    try {
        var sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName("Attendance_Data");
        var validCardsSheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName("Valid_Cards");

        if (!sheet || !validCardsSheet) {
            output.setContent(JSON.stringify({error: "Sheets not found"}));
            return output;
        }

        var cardID = e.parameter.cardID;
        if (!cardID) {
            output.setContent(JSON.stringify({error: "Missing cardID"}));
            return output;
        }

        var currentDate = Utilities.formatDate(new Date(), "Asia/Kolkata", "yyyy-MM-dd");
        var currentTime = Utilities.formatDate(new Date(), "Asia/Kolkata", "hh:mm:ss a");

        var validData = validCardsSheet.getDataRange().getValues();
        var name = "INVALID CARD";

        for (var i = 1; i < validData.length; i++) {
            if (validData[i][0] == cardID) {
                name = validData[i][1];
                break;
            }
        }
        sheet.appendRow([cardID, name, currentDate, currentTime]);
        moveToDateSheet(cardID, name, currentDate, currentTime);

        var logType = checkLoginStatus(name, currentDate, currentTime); // Get login/logout status

        // Prepare JSON response with user details
        var response = { cardID: cardID, name: name, date: currentDate, time: currentTime,status: logType[0] }; // "Login" or "Logout"
        
        // If the user is logging out, add total time from column E
        if (logType[0] === "Logout") {
            response.totalTime = logType[1]; // Total hours worked
        }
        output.setContent(JSON.stringify(response));
        return output;
    }
    catch (error) {
        output.setContent(JSON.stringify({error: error.toString()}));
        return output;
    }
}

// Function to check if it's a login or logout
function checkLoginStatus(name, date, time) {
    var ss = SpreadsheetApp.getActiveSpreadsheet();
    var dateSheet = ss.getSheetByName(date);

    if (!dateSheet) return ["Login", ""]; // First entry is always login

    var data = dateSheet.getDataRange().getValues();

    for (var i = data.length - 1; i > 0; i--) {
        if (data[i][0] == name) {
            if (data[i][1] && !data[i][2]) {
                // If there's an IN time but no OUT time, return Logout with total time from column E
                return ["Login", ""]; // If both IN and OUT exist, next is login
            }
            if (data[i][1] && data[i][2]) {
                return ["Logout", data[i][4] || "00:00:00"];
            
            }
        }
    }
    return ["Login", ""]; // Default to login if no history is found
}

function moveToDateSheet(cardID, name, date, time) {
    var ss = SpreadsheetApp.getActiveSpreadsheet();
    var dateSheet = ss.getSheetByName(date);

    if (!dateSheet) {
        dateSheet = ss.insertSheet(date);
        dateSheet.appendRow(["Name", "In", "Out", "Total (min)", "Total Time"]); // Added new column header
    }

    var data = dateSheet.getDataRange().getValues();
    var lastInRow = -1;

    for (var i = 1; i < data.length; i++) {
        if (data[i][0] == name && data[i][1] && !data[i][2]) {
            lastInRow = i;
            break;
        }
    }

    if (lastInRow !== -1) {
        var rowNumber = lastInRow + 1;
        dateSheet.getRange(rowNumber, 3).setValue(time);


        // Insert formula to calculate minutes (Column D)
        dateSheet.getRange(rowNumber, 4).setFormula(`=IF(AND(B${rowNumber}<>"", C${rowNumber}<>""), ROUND((TIMEVALUE(C${rowNumber}) - TIMEVALUE(B${rowNumber})) * 1440, 2), "")`);
    
        // Insert formula to convert minutes into HH:MM:SS format (Column E)
        dateSheet.getRange(rowNumber, 5).setFormula(`=TEXT(INT(D${rowNumber}/60), "00") & ":" & TEXT(INT(MOD(D${rowNumber},60)), "00") & ":" & TEXT(ROUND((MOD(D${rowNumber},1) * 60), 0), "00")`);
    }
    else 
    {
        dateSheet.appendRow([name, time, "", "", ""]);
    }
    createOrUpdateMonthlySheet();
}


function createOrUpdateMonthlySheet() {
    var ss = SpreadsheetApp.getActiveSpreadsheet();
    var validSheets = ss.getSheetByName("Valid_Cards").getRange("B2:B").getValues().flat().filter(String);
    var today = new Date();
    var year = today.getFullYear();
    var month = today.getMonth(); // 0-based index (Jan = 0, Feb = 1, etc.)
    var sheetName = Utilities.formatDate(today, Session.getScriptTimeZone(), "yyyy-MM"); // Example: "2025-03"
    var sheet = ss.getSheetByName(sheetName);
    //  Create the monthly sheet only if it doesn't exist
    if (!sheet) {
        sheet = ss.insertSheet(sheetName);
    
        //  Get the correct last day of the month
        var lastDay = new Date(year, month + 1, 0).getDate(); // Finds last day of the month

        //  Generate date headers from "YYYY-MM-01" to "YYYY-MM-lastDay"
        var dateHeaders = [];
        for (var day = 1; day <= lastDay; day++) {
            dateHeaders.push(Utilities.formatDate(new Date(year, month, day), Session.getScriptTimeZone(), "yyyy-MM-dd"));
        }

        //  Set headers (dates)
        sheet.getRange(1, 2, 1, lastDay).setValues([dateHeaders]);

        //  Set names in Column A
        if (validSheets.length > 0) {
            sheet.getRange(2, 1, validSheets.length, 1).setValues(validSheets.map(name => [name]));
        }
    
        // Insert formulas dynamically
        var lastRow = sheet.getLastRow();
        var lastCol = sheet.getLastColumn();
        var formulaTemplate = `=IFERROR(TEXT(INT(SUMIFS(INDIRECT("'" & TEXT(B$1, "YYYY-MM-DD") & "'!D:D"), INDIRECT("'" & TEXT(B$1, "YYYY-MM-DD") & "'!A:A"), $A2)/60), "00") & ":" & TEXT(MOD(SUMIFS(INDIRECT("'" & TEXT(B$1, "YYYY-MM-DD") & "'!D:D"), INDIRECT("'" & TEXT(B$1, "YYYY-MM-DD") & "'!A:A"), $A2), 60), "00") & ":00", "")`;


        for (var row = 2; row <= lastRow; row++) {
            for (var col = 2; col <= lastCol; col++) {
                sheet.getRange(row, col).setFormula(formulaTemplate.replace(/\$A2/g, `$A${row}`).replace(/B\$1/g, `${sheet.getRange(1, col).getA1Notation()}`));
            }
        }
    }
}
