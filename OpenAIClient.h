#ifndef OPENAI_CLIENT_H
#define OPENAI_CLIENT_H

#include <Arduino.h>

class OpenAIClient {
private:
    String apiKey;
    
    // Extract the content field from a JSON response
    String extractContent(const char* json);

public:
    // Constructor accepts your OpenAI API key
    OpenAIClient(const String& key);

    // Send an image to OpenAI
    String sendImageToOpenAI(uint8_t* image, size_t imageSize);

    // Send a prompt to the OpenAI API and get a response
    String getChatGPT(const String& prompt);
};

#endif // OPENAI_CLIENT_H
