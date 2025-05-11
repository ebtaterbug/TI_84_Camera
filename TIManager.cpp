#include "TIManager.h"
#include "launcher.h"
#include "CBL2.h"
#include "TIVar.h"
#include "OpenAIClient.h"
#include <Preferences.h>
#include "WebPageManager.h"
#include <WiFi.h>
#include "CameraModule.h"

WiFiManager wifiManager;
// ---------------------------------------------------------
// Global pointer for proxy callbacks
// ---------------------------------------------------------
TIManager* g_ti = nullptr; 

// ---------------------------------------------------------
// Proxy functions for CBL2 setupCallbacks
// Matching the signatures: 
//    int (*get_callback)(uint8_t, Endpoint, int)
//    int (*req_callback)(uint8_t, Endpoint, int*, int*, uint8_t(**)(int))
// ---------------------------------------------------------
int onReceivedProxy(uint8_t t, Endpoint m, int d) {
    return g_ti ? g_ti->onReceived(t, m, d) : -1;
}

int onRequestProxy(uint8_t t, Endpoint m, int* hl, int* dl, data_callback* cb) {
    return g_ti ? g_ti->onRequest(t, m, hl, dl, cb) : -1;
}

// ---------------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------------
TIManager::TIManager()
    : currentArg(0),
      command(-1),
      status(false),
      errorState(false),
      PAGE_PAGE(0),
      queued_action(nullptr)
{
    memset(header, 0, MAXHDRLEN);
    memset(data, 0, MAXDATALEN);
    memset(message, 0, MAXSTRARGLEN);

    fullResponse = "";
    initCommands();
}

// ---------------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------------
void TIManager::setup() {
    Serial.begin(115200);
    Serial.println("[TIManager] Setup...");
    delay(2000);

    // Assign the global pointer so the proxy functions can call this instance
    g_ti = this;

    // Initialize link cable
    cbl.setLines(TIP, RING);
    cbl.resetLines();

    // Register the proxy callbacks with cbl.setupCallbacks
    cbl.setupCallbacks(
        header,
        data,
        MAXDATALEN,
        onReceivedProxy,  // free function pointer
        onRequestProxy    // free function pointer
    );

    pinMode(TIP, INPUT);
    pinMode(RING, INPUT);

    strcpy(message, "default message");
    Serial.println("[TIManager] Setup complete");
}

// ---------------------------------------------------------------------------------
// Loop
// ---------------------------------------------------------------------------------
void TIManager::loop() {
    // Execute any queued action
    if (queued_action) {
        delay(1000);
        Serial.println("[TIManager] Executing queued action...");
        void (*temp)() = queued_action;
        queued_action  = nullptr;
        temp();
    }

    // If AP is active, handle requests
    if (apActive && webPageManager) {
        webPageManager->handleClient();
    }
    
    // Check for a valid command
    if (command >= 0 && command <= MAXCOMMAND) {
        for (auto &cmdEntry : commands) {
            if (cmdEntry.id == command && cmdEntry.num_args == currentArg) {
                Serial.print("[TIManager] Processing command: ");
                Serial.println(cmdEntry.name);
                (this->*cmdEntry.command_fp)();
            }
        }
    }

    // Process CBL2 events
    cbl.eventLoopTick();
}

// ---------------------------------------------------------------------------------
// Initialize Commands
// ---------------------------------------------------------------------------------
void TIManager::initCommands() {
    commands[0] = { 2,  "gpt", 1, &TIManager::gpt,  true };
    commands[1] = { 5,  "launcher",       0, &TIManager::launcherCommand, false };
    commands[2] = { 15, "sendPage",       1, &TIManager::sendPage,        false };
    commands[3] = { 3, "startAP",       0, &TIManager::startAP,        false };
    commands[4] = { 0, "connectWiFi",       0, &TIManager::connectWiFi,        false };
    commands[5] = { 1, "disconnectWiFi",       0, &TIManager::disconnectWiFi,        false };
    commands[6] = { 4, "takeImage",       0, &TIManager::takeImage,        false };

}

// ---------------------------------------------------------------------------------
// Start a command
// ---------------------------------------------------------------------------------
void TIManager::startCommand(int cmd) {
    command    = cmd;
    status     = false;
    errorState = false;
    currentArg = 0;

    // Clear out arguments
    for (int i = 0; i < MAXARGS; ++i) {
        memset(&strArgs[i], 0, MAXSTRARGLEN);
        realArgs[i] = 0;
    }
    strcpy(message, "no command");
}

// ---------------------------------------------------------------------------------
// Error and Success
// ---------------------------------------------------------------------------------
void TIManager::setError(const char* err) {
    Serial.print("[TIManager ERROR] ");
    Serial.println(err);
    errorState = true;
    status     = true;
    command    = -1;
    strncpy(message, err, MAXSTRARGLEN);
}

void TIManager::setSuccess(const char* success) {
    Serial.print("[TIManager SUCCESS] ");
    Serial.println(success);
    errorState = false;
    status     = true;
    command    = -1;
    strncpy(message, success, MAXSTRARGLEN);
}

// ---------------------------------------------------------------------------------
// Fix String (removes lowercase letters as in original code)
// ---------------------------------------------------------------------------------
void TIManager::fixStrVar(char* str) {
    int end = strlen(str);
    for (int i = 0; i < end; ++i) {
        if (islower(str[i])) {
            // Remove that character by shifting
            for (int j = i; j < end; ++j) {
                str[j] = str[j + 1];
            }
            end--;
            i--;
        }
    }
    str[end] = '\0';
}

// ---------------------------------------------------------------------------------
// onReceived Callback
// ---------------------------------------------------------------------------------
int TIManager::onReceived(uint8_t type, Endpoint model, int datalen) {
    char varName = header[3];

    // If the variable name is 'C', it's a command
    if (varName == 'C') {
        if (type != VarTypes82::VarReal) return -1;
        int cmd = TIVar::realToLong8x(data, model);
        if (cmd >= 0 && cmd <= MAXCOMMAND) {
            Serial.print("[TIManager] Received command: ");
            Serial.println(cmd);
            startCommand(cmd);
            return 0;
        } else {
            Serial.print("[TIManager] Invalid command: ");
            Serial.println(cmd);
            return -1;
        }
    }

    // If 'V', it's a page number for sendPage
    if (varName == 'V') {
        if (type != VarTypes82::VarReal) return -1;
        PAGE_PAGE = TIVar::realToLong8x(data, model);
        Serial.print("[TIManager] Received page number: ");
        Serial.println(PAGE_PAGE);
        sendPage();
        return 0;
    }

    // If 'X', we reset the fullResponse
    if (varName == 'X') {
        if (type != VarTypes82::VarReal) return -1;
        Serial.println("[TIManager] Resetting fullResponse");
        fullResponse = "";
        return 0;
    }

    // Otherwise, it's an argument for the current command
    if (currentArg >= MAXARGS) {
        setError("Argument overflow");
        return -1;
    }

    switch (type) {
        case VarTypes82::VarString: {
            String strVar = TIVar::strVarToString8x(data, model);
            strncpy(strArgs[currentArg], strVar.c_str(), MAXSTRARGLEN);
            fixStrVar(strArgs[currentArg]);
            Serial.print("StrArg");
            Serial.print(currentArg);
            Serial.print(": ");
            Serial.println(strArgs[currentArg]);
            currentArg++;
            break;
        }
        case VarTypes82::VarReal: {
            realArgs[currentArg] = TIVar::realToFloat8x(data, model);
            Serial.print("RealArg");
            Serial.print(currentArg);
            Serial.print(": ");
            Serial.println(realArgs[currentArg]);
            currentArg++;
            break;
        }
        default:
            return -1;
    }
    return 0;
}

// ---------------------------------------------------------------------------------
// onRequest Callback
// ---------------------------------------------------------------------------------
int TIManager::onRequest(uint8_t type, Endpoint model, int* headerlen,
                         int* datalen, data_callback* data_callback)
{
    char varName = header[3];
    memset(header, 0, sizeof(header));

    switch (varName) {
    case 0xAA: {
        // Return 'message' as a string
        if (type != VarTypes82::VarString) return -1;
        *datalen = TIVar::stringToStrVar8x(String(message), data, model);
        TIVar::intToSizeWord(*datalen, header);
        header[2] = VarTypes82::VarString;
        header[3] = 0xAA;
        header[4] = '\0';
        *headerlen = 13;
        return 0;
    }
    case 'E': {
        // Return errorState as real
        if (type != VarTypes82::VarReal) return -1;
        *datalen = TIVar::longToReal8x(errorState ? 1 : 0, data, model);
        TIVar::intToSizeWord(*datalen, header);
        header[2] = VarTypes82::VarReal;
        header[3] = 'E';
        header[4] = '\0';
        *headerlen = 13;
        return 0;
    }
    case 'S': {
        // Return status as real
        if (type != VarTypes82::VarReal) return -1;
        *datalen = TIVar::longToReal8x(status ? 1 : 0, data, model);
        TIVar::intToSizeWord(*datalen, header);
        header[2] = VarTypes82::VarReal;
        header[3] = 'S';
        header[4] = '\0';
        *headerlen = 13;
        return 0;
    }
    default:
        return -1;
    }
}

// ---------------------------------------------------------------------------------
// Command Methods
// ---------------------------------------------------------------------------------
void TIManager::startAP() {
    // If AP is not active, start it
    if (!apActive) {
        if (!webPageManager) {
            webPageManager = new WebPageManager(wifiManager); 
        }
        webPageManager->begin(); // Start the AP & serve config page
        apActive = true;
        setSuccess("ON");
    } else {
        WiFi.softAPdisconnect(true);
        apActive = false;
        setSuccess("OFF");
    }
}


void TIManager::gpt() {
    if (currentArg < 1) {
        setError("Missing argument");
        return;
    }
  
    // Read the user's input from strArgs[0]
    const char* userPrompt = strArgs[0];
    Serial.print("[TIManager::gpt] User prompt: ");
    Serial.println(userPrompt);
    
    String openAIKey;

    Preferences prefs;
    prefs.begin("WiFiCreds", true); // read-only
    openAIKey = prefs.getString("openAIKey", "");
    prefs.end();
    OpenAIClient openAI(openAIKey);


    // 4. If OpenAI key is present, demonstrate usage
    if (!openAIKey.isEmpty()) {
        Serial.println("OpenAI key found in Preferences. Testing API request...");

        // Query ChatGPT using your existing getChatGPT(...) function
        // Example usage: Add "Just give the answer without explanation unless asked." 
        // or any other instructions you want:
        String prompt = String("You are a calculator for solving college physics, algebra, ")
        + String("pre-calculus, calculus 1-3, engineering, english and chemistry problems. ")
        + String("Always provide the correct answer. First, outline the solution steps briefly. ")
        + String("Then solve the problem. Finally, confirm if the answer is correct. ")
        + String("Be concise, never verbose. Output in plaintext only—no LaTeX or special formatting. ")
        + String("Use only simple ASCII characters. Never use these characters #$%&;@\_`|~. Respond with one long line and never newlines.")
        + String("Format equations compactly (-> for implies, ^ for exponents, * for multiplication, / for division). ")
        + String("Prioritize exact values when possible. Now solve this problem: ");


        String response = openAI.getChatGPT(prompt + String(userPrompt));

        // Replace symbols
        response.replace(";", "()");
        response.replace("#", "()");
        response.replace("$", "()");
        response.replace("%", " PERCENT ");
        response.replace("&", " AND ");
        response.replace("@", "()");
        response.replace("\\", "()");
        response.replace("_", "()");
        response.replace("`", "()");
        response.replace("|", "()");
        response.replace("~", "()");
        response.replace("\n", " ");
        response.replace("≈", " IS APPROXIMATELY ");

        // Build a combined conversation string, or simply store the AI response
        fullResponse = "User: " + String(userPrompt) + " | AI: " + response;
        PAGE_PAGE = 0;
    
        // Display the first "page" of the conversation
        sendPage();
    } else {
        Serial.println("No OpenAI key found. Configure via the AP webpage.");
    }
}

void TIManager::takeImage() {
    // Get API Key
    String openAIKey;

    Preferences prefs;
    prefs.begin("WiFiCreds", true); // read-only
    openAIKey = prefs.getString("openAIKey", "");
    prefs.end();
    OpenAIClient openAI(openAIKey);
    
    if (!openAIKey.isEmpty()) {
        Serial.println("OpenAI key found in Preferences. Testing API request...");
      // Take Image & Send To ChatGPT
      uint8_t* jpeg = nullptr; size_t len = 0;

      if (!captureImage(&jpeg, &len)) {       // grab frame
        Serial.println("capture fail");
      }

      String reply = openAI.sendImageToOpenAI(jpeg, len);   // <<<<<
      // free(jpeg); // always free

      // Build a combined conversation string, or simply store the AI response
      fullResponse = reply;
      PAGE_PAGE = 0;
    
      // Display the first "page" of the conversation
      sendPage();
      //setSuccess(reply.c_str());
    
    } else {
        Serial.println("No OpenAI key found. Configure via the AP webpage.");
    }
}


void TIManager::launcherCommand() {
    // We queue sending the launcher program
    queued_action = _sendLauncherStatic;
    setSuccess("queued launcher transfer");
}

void TIManager::sendPage() {
    int startIdx = PAGE_PAGE * PAGE_SIZE;
    String pageContent = fullResponse.substring(startIdx, 
                       min(startIdx + PAGE_SIZE, (int)fullResponse.length()));

    strncpy(message, pageContent.c_str(), MAXSTRARGLEN);
    setSuccess(message);
}

void TIManager::connectWiFi() {
    // Call WiFiManager::connect() with no args 
    // => it will load credentials from Preferences if needed
    wifiManager.connect();

    // Optionally store a message
    setSuccess("Connected");
}

void TIManager::disconnectWiFi() {
    wifiManager.disconnect();
    setSuccess("Disconnected");
}

// ---------------------------------------------------------------------------------
// Program Sending
// ---------------------------------------------------------------------------------
void TIManager::_sendLauncherStatic() {
    // We need a static function to call the instance method.
    extern TIManager* g_ti;
    if (g_ti) g_ti->_sendLauncher();
}

void TIManager::_sendLauncher() {
    // Use the external launcher var
    sendProgramVariable("CHATGPT", (uint8_t*)__launcher_var, __launcher_var_len);
}

int TIManager::sendProgramVariable(const char* name, uint8_t* program, size_t variableSize) {
    Serial.print("[TIManager] Transferring program: ");
    Serial.print(name);
    Serial.print(" (");
    Serial.print(variableSize);
    Serial.println(" bytes)");

    // The silent link handshake
    uint8_t msg_header[4] = { COMP83P, RTS, 13, 0 };
    uint8_t rtsdata[13]   = {
        (uint8_t)(variableSize & 0xff),
        (uint8_t)(variableSize >> 8),
        VarTypes82::VarProgram,
        0,0,0,0,0,0,0,0,0,0
    };

    int nameSize = strlen(name);
    if (nameSize == 0) {
        return 1;
    }
    memcpy(&rtsdata[3], name, min(nameSize, 8));

    int dataLength = 0;

    // Send RTS
    int rtsVal = cbl.send(msg_header, rtsdata, 13);
    if (rtsVal) {
        Serial.printf("[TIManager] RTS return: %d\n", rtsVal);
        return rtsVal;
    }
    cbl.resetLines();

    // Wait for ACK
    int ackVal = cbl.get(msg_header, NULL, &dataLength, 0);
    if (ackVal || msg_header[1] != ACK) {
        Serial.printf("[TIManager] ACK return: %d\n", ackVal);
        return ackVal;
    }

    // Wait for CTS
    int ctsRet = cbl.get(msg_header, NULL, &dataLength, 0);
    if (ctsRet || msg_header[1] != CTS) {
        Serial.printf("[TIManager] CTS return: %d\n", ctsRet);
        return ctsRet;
    }

    // ACK to CTS
    msg_header[1] = ACK;
    msg_header[2] = 0x00;
    msg_header[3] = 0x00;
    ackVal = cbl.send(msg_header, NULL, 0);
    if (ackVal) {
        Serial.printf("[TIManager] ack cts return: %d\n", ackVal);
        return ackVal;
    }

    // Send DATA
    msg_header[1] = DATA;
    msg_header[2] = variableSize & 0xff;
    msg_header[3] = (variableSize >> 8) & 0xff;
    int dataRet = cbl.send(msg_header, program, variableSize);
    if (dataRet) {
        Serial.printf("[TIManager] data return: %d\n", dataRet);
        return dataRet;
    }

    // Wait for ACK of data
    ackVal = cbl.get(msg_header, NULL, &dataLength, 0);
    if (ackVal || msg_header[1] != ACK) {
        Serial.printf("[TIManager] ack data: %d\n", ackVal);
        return ackVal;
    }

    // Send EOT
    msg_header[1] = EOT;
    msg_header[2] = 0x00;
    msg_header[3] = 0x00;
    int eotVal = cbl.send(msg_header, NULL, 0);
    if (eotVal) {
        Serial.printf("[TIManager] EOT return: %d\n", eotVal);
        return eotVal;
    }

    Serial.print("[TIManager] Transferred: ");
    Serial.println(name);
    return 0;
}
