#include "OpenAIClient.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>  
#include "CameraModule.h" // for 'config' if you re‑init the camera
#include "base64.h"

OpenAIClient::OpenAIClient(const String& key) : apiKey(key) {}

// Extract the "content" field from a JSON response
String OpenAIClient::extractContent(const char* json) {
    String message;

    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, json);

    if (error) {
        Serial.println("Error: Failed to parse JSON");
        return "Error: Failed to parse JSON";
    }

    const char* content = doc["choices"][0]["message"]["content"];
    message = content ? String(content) : "Error: Content not found";

    Serial.println("Extracted Content: " + message);
    return message;
}

// Send image to OpenAI API
String OpenAIClient::sendImageToOpenAI(uint8_t* image, size_t imageSize)
{
    if (WiFi.status() != WL_CONNECTED) return "Wi-Fi disconnected";

    /* 1. Base64 */
    String b64 = base64::encode(image, imageSize);
    free(image);                             // we’re done with the frame
    if (b64.isEmpty()) return "B64 fail";

    /* 2. Build JSON */
    StaticJsonDocument<2048> doc;
    doc["model"] = "gpt-4o";
    JsonArray msgs = doc.createNestedArray("messages");
    JsonObject u = msgs.createNestedObject();
    u["role"] = "user";
    JsonArray content = u.createNestedArray("content");
    content.add("Briefly answer the problem in the photo.");

    JsonObject img = content.createNestedObject();
    img["type"] = "image_url";
    img["image_url"]["url"] = "data:image/jpeg;base64," + b64;  // correct field

    String body;
    serializeJson(doc, body);

    /* 3. HTTPS POST */
    WiFiClientSecure client;
    client.setInsecure();                    // or load root cert
    HTTPClient http;
    const char* url = "https://api.openai.com/v1/chat/completions";

    if (!http.begin(client, url)) return "begin() fail";

    http.addHeader("Authorization", "Bearer " + apiKey);
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(body);

    String result;
    if (code == 200) {
        result = extractContent(http.getString().c_str());
    } else {
        result = "HTTP err " + String(code);
        Serial.println(result);
    }
    http.end();
    return result;
}



// Send prompt to OpenAI API, return the extracted response
String OpenAIClient::getChatGPT(const String& prompt) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Error: WiFi not connected.");
        return "WiFi Error";
    }

    const String url = "https://api.openai.com/v1/chat/completions";
    const String payload = "{\"model\": \"gpt-4o\", \"messages\": [{\"role\": \"user\", \"content\": \"" + prompt + "\"}], \"max_completion_tokens\": 4096}";

    WiFiClientSecure client;
    HTTPClient http;
    client.setInsecure();

    Serial.println("Connecting to OpenAI...");
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + apiKey);
    http.setTimeout(25000);  // Increase timeout for longer responses

    Serial.println("Sending request...");
    Serial.println("Payload: " + payload);

    int httpResponseCode = http.POST(payload);
    String response;

    if (httpResponseCode == 200) {
        response = http.getString();
        Serial.println("OpenAI API Response: " + response);
    } else {
        Serial.print("HTTP Error: ");
        Serial.println(httpResponseCode);
        response = "API Error";
    }

    http.end();
    Serial.println("Free Heap: " + String(ESP.getFreeHeap()));  // Debug memory
    return extractContent(response.c_str());
}


