#pragma once

#include "system_error.h"
#include "logging.h"
#include <memory>
#include <cstring>

/*
#include "ezcli.h"

CLI_CMD_REG(touch, [](int argc, const char* argv[])->void {
    int x = atoi(argv[0]);
    int y = atoi(argv[1]);
    Log.info("touch, x: %d, y: %d", x, y);
}, "Format: touch x y\r\nExample: touch 25 25\r\n");

void loop() {
    ssize_t size = hal_uart_available_data_for_read(HAL_UART0);
    if (size > 0) {
        std::unique_ptr<char> dataBuf(new(std::nothrow) char[size]);
        char* p = dataBuf.get();
        hal_uart_read(HAL_UART0, p, size);
        EzcliParser.parse(p, size);
        Log.printf("RX: ");
        Log.dump(p, size);
        Log.printf("\r\n");
    }
}
 */

namespace wos { namespace ezcli {

static constexpr const char* DATA_MODE_STOP_STRING = "+++AAA+++";

static constexpr int MAX_CMD_COUNT = 10;
static constexpr int MAX_PARAMETER_COUNT = 5;
static constexpr int MAX_INPUT_BUFFER_SIZE = 256;

typedef struct CliCmdRegister {
    const char* name;
    const char* help;
    void (*handler)(int argc, const char* argv[]);
} CliCmdRegister;

#define CLI_CMD_REG(cmdName, cmdHandler, helpString)                                    \
    static const char* ezcli_cmd_##cmdName = #cmdName;                                  \
    static const char* ezcli_cmd_##cmdName##_help = helpString;                         \
    static const CliCmdRegister ezcli_cmd_##cmdName##_register = {                      \
        .name = ezcli_cmd_##cmdName,                                                    \
        .help = ezcli_cmd_##cmdName##_help,                                             \
        .handler = (cmdHandler)                                                         \
    };                                                                                  \
    struct clicmd_struct_##cmdName {                                                    \
        clicmd_struct_##cmdName() {                                                     \
            EzcliParser.registerCmd(&ezcli_cmd_##cmdName##_register);                   \
        }};                                                                             \
    static struct clicmd_struct_##cmdName __instance_##cmdName;

typedef void (*UndefineCliCmdHandler)(const char* data, size_t size);

class CliPaser {
public:
    static CliPaser& getInstance() {
        static CliPaser parser;
        return parser;
    }

    int registerCmd(const CliCmdRegister* cmd) {
        if (cmdCount_ >= MAX_CMD_COUNT) {
            return SYSTEM_ERROR_NO_MEMORY;

        }
        cmds_[cmdCount_++] = *cmd;
        return SYSTEM_ERROR_NONE;
    }

    int registerUndefinedCliCmdHandler(UndefineCliCmdHandler handler) {
        undefineCliCmdHandler_ = handler;
        return SYSTEM_ERROR_NONE;
    }

    int parse(const char* data, size_t size) {
        int ret = SYSTEM_ERROR_NOT_ENOUGH_DATA;
        for (size_t i = 0; i < size; i++) {
            // Receive data
            inputBuffer_[inputIndex_++] = data[i];

            if (inputIndex_ < MAX_INPUT_BUFFER_SIZE) {
                if (parserEnabled_) {
                    // Handle the command when getting '\n'
                    // NOTE: in parser mode, inputBuffer_ will be modified to remove \r and \n
                    if (data[i] == '\r') {
                        inputBuffer_[inputIndex_ - 1] = '\0';
                        inputIndex_--;
                    } else if (data[i] == '\n') {
                        inputBuffer_[inputIndex_ - 1] = '\0';
                        ret = handleCommand(inputBuffer_, inputIndex_);
                        if (ret == SYSTEM_ERROR_NONE) {
                            resetBuffer();
                            continue;
                        } else if (undefineCliCmdHandler_) {
                            undefineCliCmdHandler_(inputBuffer_, inputIndex_);
                        }
                    }
                } else {
                    // Nothing, handle the packet until buffer is full. 
                    continue;
                }
            } else {
                if (parserEnabled_) {
                    Log.warn("user inputs are more than MAX_INPUT_BUFFER_SIZE, drop the lagacy data!");
                    resetBuffer();
                } else {
                    // // Search stop string when the buffer is full, NOTE: based on performance consideration
                    // bool foundStopString = false;
                    // for (int i = 0; i < inputIndex_ && !foundStopString; i++) {
                    //     for (size_t j = 0; j < strlen(DATA_MODE_STOP_STRING); j++) {
                    //         if (inputBuffer_[i] == DATA_MODE_STOP_STRING[j]) {
                    //             if (j == strlen(DATA_MODE_STOP_STRING) - 1) {
                    //                 // Found stop string
                    //                 foundStopString = true;
                    //                 break;
                    //             } else {
                    //                 continue;
                    //             }
                    //         }
                    //     }
                    // }

                    // if (foundStopString) {
                    //     resetBuffer();
                    //     parserEnabled_ = true;
                    //     continue;
                    // }

                    // When paser is disabled and buffer is full, handle the packet to free the buffer to receive new byte
                    if (inputIndex_ == MAX_INPUT_BUFFER_SIZE && undefineCliCmdHandler_) {
                        undefineCliCmdHandler_(inputBuffer_, inputIndex_);
                    }
                    resetBuffer();
                }
            }
        }
        return ret;
    }

    int enableParser() {
        if (parserEnabled_) {
            return SYSTEM_ERROR_NONE;
        }
        resetBuffer();
        parserEnabled_ = true;
        return SYSTEM_ERROR_NONE;
    }

    int disableParser() {
        if (!parserEnabled_) {
            return SYSTEM_ERROR_NONE;
        }
        resetBuffer();
        parserEnabled_ = false;
        return SYSTEM_ERROR_NONE;
    }

private:
    CliPaser() :
            cmdCount_(0),
            cmds_{},
            inputIndex_(0),
            inputBuffer_{},
            undefineCliCmdHandler_(nullptr),
            parserEnabled_(true) {};

    int handleCommand(char* data, size_t size) {
        CHECK_TRUE(size > 0, SYSTEM_ERROR_NOT_ENOUGH_DATA);
        const char* pCmd = nullptr;
        const char* pParams[MAX_PARAMETER_COUNT] = {};
        int paramCount = 0;
        size_t index = 0;
        for (size_t i = 0; i < size; i++) {
            // ' ' or last character
            if (data[i] != ' ' && i != size - 1) {
                continue;
            } else {
                if (!pCmd) {
                    pCmd = data + index;
                } else {
                    if (paramCount < MAX_PARAMETER_COUNT) {
                        pParams[paramCount++] = data + index;
                    } else {
                        LOG(WARN, "Parameter exceeds the buffer we provide.");
                        break;
                    }
                }
                if (data[i] == ' ') {
                    data[i] = '\0';
                }
                index = i + 1;
            }
        }
        if (strcmp(pCmd, "help") == 0) {
            // TODO: print all commands
            // TODO: clear
        } else {
            for (int i = 0; i < cmdCount_; i++) {
                if (strcmp(pCmd, cmds_[i].name) == 0) {
                    // TODO: check the first parameter, if it is 'help' print help string
                    (*cmds_[i].handler)(paramCount, pParams);
                    return SYSTEM_ERROR_NONE;
                }
            }
        }

        return SYSTEM_ERROR_NOT_FOUND;
    }

    int resetBuffer() {
        memset(inputBuffer_, 0, sizeof(inputBuffer_));
        inputIndex_ = 0;
        return SYSTEM_ERROR_NONE;
    }

private:
    int                     cmdCount_;
    CliCmdRegister          cmds_[MAX_CMD_COUNT];
    int                     inputIndex_;
    char                    inputBuffer_[MAX_INPUT_BUFFER_SIZE + 1]; // 1: for the last parameter without space char at the tail
    UndefineCliCmdHandler   undefineCliCmdHandler_;
    bool                    parserEnabled_;
};

#define EzcliParser wos::ezcli::CliPaser::getInstance()

}} // wos::ezcli
