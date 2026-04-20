#pragma once
#include <string>
#include <iostream>

#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"

class JsonParser
{
public:
    // Returns the buffer that MUST outlive the document
    static bool ReadJsonFileInsitu(const std::string& path, rapidjson::Document& out_document, std::vector<char>& out_buffer)
    {
        FILE* file_pointer = nullptr;
#ifdef _WIN32
        fopen_s(&file_pointer, path.c_str(), "rb");
#else
        file_pointer = fopen(path.c_str(), "rb");
#endif
        if (!file_pointer) return false;

        // Get file size
        fseek(file_pointer, 0, SEEK_END);
        size_t fileSize = ftell(file_pointer);
        fseek(file_pointer, 0, SEEK_SET);

        // Allocate buffer (plus padding for safety)
        out_buffer.resize(fileSize + 16);
        fread(out_buffer.data(), 1, fileSize, file_pointer);
        out_buffer[fileSize] = 0; // Null terminate
        fclose(file_pointer);

        // Parse In-Situ (modifies the buffer)
        out_document.ParseInsitu(out_buffer.data());

        if (out_document.HasParseError()) {
            std::cout << "Error parsing JSON at " << path << std::endl;
            exit(0);
        }
        return true;
    }
};

