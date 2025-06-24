#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>


const char* ssid = "suresh";
const char* password = "12345678";
String webapp = "/macros/s/AKfycbwpSuyxGnmyrED3lJcoT7Twz5WXZtmzLHi_2HpZaDukAgB4XM4JWlc4tp66muhMnPw/exec?cardID=";

void setup() {
    Serial.begin(9600);
    delay(1000);
   
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi...");

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 50) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi Connected Successfully.");
    } 
    else {
        Serial.println("\nWiFi Not Connected! Try to send WiFi details via AT+WIFI=<ssid,password>");
    }

    Serial.println("ESP8266 AT Command Mode Ready.");
    Serial.println("Use AT+WIFI=<SSID>,<PASSWORD> to connect to WiFi.");
    Serial.println("Use AT+SEND=<cardID> to send a request.");
    Serial.println("Use AT+URL=<webappurl> to change the URL.");
}


void loop() {

    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();  

        if (command.startsWith("AT+WIFI=")) {
            connectWiFi(command);
        }
        else if (command.startsWith("AT+SEND=")) {
            command=command.substring(8);
            sendRequest(command);
        }
        else if (command.startsWith("AT+URL=")) {
            configureURL(command);
        }
        else {
            Serial.println("Invalid AT command. Use AT+WIFI, AT+SEND, AT+URL.");
        }
    }
}


// Change Google Apps Script URL
void configureURL(String cmd) {
    webapp = cmd.substring(7);
    Serial.print("New Web App URL Set: ");
    Serial.println(webapp);
}


// Connect to WiFi using AT+WIFI=<SSID>,<PASSWORD>
void connectWiFi(String cmd) {
    int commaIndex = cmd.indexOf(",");
    if (commaIndex == -1) {
        Serial.println("Invalid AT+WIFI format. Use AT+WIFI=<SSID>,<PASSWORD>");
        return;
    }

    String newSSID = cmd.substring(8, commaIndex);
    String newPassword = cmd.substring(commaIndex + 1);


    Serial.print("Connecting to WiFi: ");
    Serial.println(newSSID);


    WiFi.begin(newSSID.c_str(), newPassword.c_str());
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }


    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to WiFi!");
    } else {
        Serial.println("\nFailed to connect to WiFi.");
    }
}

void sendRequest(String cardID) {
    WiFiClientSecure client;
    client.setInsecure();                                         // Bypass SSL verification
    client.setTimeout(20000);                                     // 20 second timeout


    // First request to get redirect URL
    Serial.println("\nConnecting to script.google.com...");
    if (!client.connect("script.google.com", 443)) {
        Serial.println("Connection to Google failed!");
        return;
    }

    String url = webapp + cardID;
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: script.google.com\r\n" +
               "User-Agent: ESP8266\r\n" +
               "Connection: close\r\n\r\n");

    // Wait for response
    while (!client.available()) {
        if (!client.connected()) {
            Serial.println("Connection lost while waiting");
            return;
        }
        delay(10);
    }

    // Read headers to find redirect location
    String location = "";
    while (client.connected()) {
        String line = client.readStringUntil('\n');
        line.trim();
    
        if (line.startsWith("Location: ")) {
            // Example redirected Location: https://script.googleusercontent.com/macros/echo?user_content_key=AehSKLhCF_Gz4jOkgB9yCmlfmGt5YRAOXc1YjACJQMu76DqyISietpRM8oUGb2hcENR5stz7BJ_kAMLqE_BCvWc6_yo-v9RE81OPZDyfumuyXB888OIw8nIMgHKpR_HiI6RFamXQt6VhD-0_L2VpMbKLExLzxzjgV9oUvIZi-RyCkz36Iy2vPNpa9idtlODzG3RqcQGa0YqWWSo3q6eTPcniHQMcIh1cPEPrG5Qt6CEilcd4gVjKJTvEJsjuItokxEMDQmcsi1c8&lib=MDO7FUfIYd6j1CnvrzPfe9yhpNJ8AGgI_
            location = line.substring(10);
            Serial.println("Found redirect location: " + location);
        }
        if (line == "") {
            break;                                        // End of headers
        }
    }

    // Read and print the entire raw response
    // Serial.println("\n=== RAW RESPONSE ===");
    while (client.connected() || client.available()) {
        if (client.available()) {
            String line = client.readStringUntil('\n');
            location = extractRedirectUrl(line);
            if (!location.isEmpty())
            {
                break;
            }
        }
        else {
            delay(1);
        }
    }
    client.stop();

    // If we found a redirect location, print it separately
    if (location != "") {
        //Serial.println("\n=== REDIRECT LOCATION ===");
        //Serial.println(location);
        //Serial.println("\r\nFollowing redirect to get data...\r\n");
        followRedirectAndGetData(location);
    }
 
  //Serial.println("\n=== END OF RESPONSE ===");
}


String extractRedirectUrl(String locationHeader) {
    // Remove "Location: " prefix
    int prefixLength = 10; // Length of "Location: "
    if (locationHeader.startsWith("Location: ")) {
        String url = locationHeader.substring(prefixLength);
        url.trim(); // Remove any whitespace or CRLF
        return url;
    }
    return ""; // Return empty string if not a Location header
}
void followRedirectAndGetData(String redirectUrl) {
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(15000);


    // Extract host and path from redirect URL
    int hostStart = redirectUrl.indexOf("//") + 2;
    int hostEnd = redirectUrl.indexOf("/", hostStart);
    String redirectHost = redirectUrl.substring(hostStart, hostEnd);
    String redirectPath = redirectUrl.substring(hostEnd);


    //Serial.print("Connecting to redirect host: ");
    //Serial.println(redirectHost);


    if (!client.connect(redirectHost.c_str(), 443)) {
        Serial.println("Connection to redirect host failed");
        return;
    }


    client.print(String("GET ") + redirectPath + " HTTP/1.1\r\n" +
                "Host: " + redirectHost + "\r\n" +
                "User-Agent: ESP8266\r\n" +
                "Connection: close\r\n\r\n");


    // Wait for response
    unsigned long timeout = millis();
    while (!client.available()) {
        if (millis() - timeout > 15000) {
            Serial.println("Redirect response timeout");
            client.stop();
            return;
        }
        delay(10);
    }


    // Skip headers
    while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
            break; // End of headers
        }
    }


    // Read response body
    String jsonResponse = "";
    String back="";
    while (client.available()) {
        jsonResponse = client.readString();
        //Serial.println("\r\njsonresponse**************************\r\n" + jsonResponse);
        back = extractJsonFromResponse(jsonResponse);
        // Serial.println("\r\nBackresponse**************************\r\n" + back);
        if (!back.isEmpty())
        {
            jsonResponse=back;
            //Serial.println(jsonResponse);
            break;
        }
    }
    client.stop();


    jsonResponse.trim();
    
    //Serial.println("\n=== JSON RESPONSE ===");
    //Serial.println(jsonResponse);
    //Serial.println("=== END OF RESPONSE ===");


    parseJsonResponse(jsonResponse);
}


void parseJsonResponse(String json) {
    if (json == "") {
        Serial.println("Empty JSON response");
        return;
    }


    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, json);


    if (error) {
        Serial.print("JSON parse error: ");
        Serial.println(error.c_str());
        Serial.println("Raw response:");
        Serial.println(json);
        return;
    }

    //Serial.println("\nParsed Data:");
    Serial.print("Card ID: "); Serial.println(doc["cardID"].as<String>());
    Serial.print("Name: "); Serial.println(doc["name"].as<String>());
    Serial.print("Date: "); Serial.println(doc["date"].as<String>());
    Serial.print("Time: "); Serial.println(doc["time"].as<String>());
    Serial.print("Status: "); Serial.println(doc["status"].as<String>());
    
    if (doc["status"] == "Logout") {
        Serial.print("Total Time: "); Serial.println(doc["totalTime"].as<String>());
    }
}


String extractJsonFromResponse(String httpResponse) {
    // Find the start of JSON (first '{' after headers)
    int jsonStart = httpResponse.indexOf('{');
    
    // Find the end of JSON (last '}' before the "0" chunk terminator)
    int jsonEnd = httpResponse.lastIndexOf('}');
    
    if (jsonStart == -1 || jsonEnd == -1) {
        return ""; // No JSON found
    }
    
    // Extract the JSON substring
    String jsonContent = httpResponse.substring(jsonStart, jsonEnd + 1);
    
    return jsonContent;
}
