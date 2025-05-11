#ifndef TI_MANAGER_H
#define TI_MANAGER_H

#include <Arduino.h>
#include <CBL2.h>
#include "launcher.h" // For the __launcher_var, etc.
#include "CameraModule.h"  // Camera setup / capture functions

class WebPageManager; // forward-declare the class

// The TIManager class handles TI-84 communication over the link cable
class TIManager {
public:
    TIManager();

    // Call in Arduino setup()
    void setup();

    // Call repeatedly in Arduino loop()
    void loop();
    
    // Callback methods that match the CBL2 signatures
    int onReceived(uint8_t type, Endpoint model, int datalen);
    int onRequest(uint8_t type, Endpoint model, int* headerlen, int* datalen, data_callback* data_callback);

private:
    WebPageManager* webPageManager = nullptr;
    bool apActive = false;  // Store the AP state here to
    
    // Link cable communication
    CBL2 cbl;

    // Pins for TIP/RING
    static constexpr int TIP = 1; //D4=1 - 13
    static constexpr int RING = 2; //D3=2 - 14

    // Buffers and state
    static constexpr int MAXHDRLEN = 16;
    static constexpr int MAXDATALEN = 4096;
    static constexpr int MAXARGS = 5;
    static constexpr int MAXSTRARGLEN = 256;
    uint8_t header[MAXHDRLEN];
    uint8_t data[MAXDATALEN];

    // Command arguments
    int    currentArg;
    char   strArgs[MAXARGS][MAXSTRARGLEN];
    double realArgs[MAXARGS];

    // Tracking current command
    int  command;
    bool status;
    bool errorState;
    char message[MAXSTRARGLEN];

    // For partial paging
    static constexpr int PAGE_SIZE = 100;
    int PAGE_PAGE; 
    String fullResponse;

    // The function pointer type for commands
    typedef void (TIManager::*CommandFunc)();

    // A command entry
    struct Command {
        int          id;
        const char*  name;
        int          num_args;
        CommandFunc  command_fp;
        bool         wifi; // Not used anymore, but left for reference if needed
    };

    // The array of known commands
    static constexpr int MAXCOMMAND = 31;
    Command commands[7];

    // Queued action pointer (used for sending a program)
    void (*queued_action)();

    // Private methods
    void initCommands();
    void startCommand(int cmd);
    void setError(const char* err);
    void setSuccess(const char* success);
    void fixStrVar(char* str);

    // Action methods bound to command IDs
    void gpt();         
    void launcherCommand();
    void sendPage();
    void startAP();
    void connectWiFi();
    void disconnectWiFi();
    void takeImage();

    // Utility for sending a program (launcher)
    static void _sendLauncherStatic();
    void _sendLauncher();
    int  sendProgramVariable(const char* name, uint8_t* program, size_t variableSize);

    // Camera
    bool cameraReady = false;
};

#endif // TI_MANAGER_H
