#include "switch.h"
#include "config_handler.h"
#include "Controllers.h"
#include "ControllerConfig.h"
#include "logger.h"
#include <cstring>
#include <stratosphere.hpp>
#include <stratosphere/util/util_ini.hpp>

namespace syscon::config
{
    namespace
    {
        constexpr std::array keyNames{
            "DEFAULT",
            "NONE",
            "FACE_UP",
            "FACE_RIGHT",
            "FACE_DOWN",
            "FACE_LEFT",
            "LSTICK_CLICK",
            "RSTICK_CLICK",
            "LEFT_BUMPER",
            "RIGHT_BUMPER",
            "LEFT_TRIGGER",
            "RIGHT_TRIGGER",
            "BACK",
            "START",
            "DPAD_UP",
            "DPAD_RIGHT",
            "DPAD_DOWN",
            "DPAD_LEFT",
            "CAPTURE",
            "HOME",
            "SYNC",
            "TOUCHPAD",
        };

        ControllerButton StringToKey(const char *text)
        {
            for (std::size_t i = 0; i != keyNames.size(); ++i)
            {
                if (strcmp(keyNames[i], text) == 0)
                {
                    return static_cast<ControllerButton>(i);
                }
            }
            return NONE;
        }

        RGBAColor DecodeColorValue(const char *value)
        {
            RGBAColor color{255};
            uint8_t counter = 0;
            int charIndex = 0;
            while (value[charIndex] != '\0')
            {
                if (charIndex == 0)
                    color.values[counter++] = atoi(value + charIndex++);
                if (value[charIndex++] == ',')
                {
                    color.values[counter++] = atoi(value + charIndex);
                    if (counter == 4)
                        break;
                }
            }
            return color;
        }

        int ParseGlobalConfigLine(void *data, const char *section, const char *name, const char *value)
        {
            GlobalConfig *config = static_cast<GlobalConfig *>(data);
            if (strcmp(section, config->ini_section) == 0)
            {
                if (strcmp(name, "polling_frequency_ms") == 0)
                    config->polling_frequency_ms = atoi(value);
                else if (strcmp(name, "log_level") == 0)
                    config->log_level = atoi(value);
                else
                {
                    syscon::logger::LogError("Unknown key: %s", name);
                    return 0; // Unknown key, return error
                }
            }

            return 1; // Success
        }

        int ParseControllerConfigLine(void *data, const char *section, const char *name, const char *value)
        {
            ControllerConfig *config = static_cast<ControllerConfig *>(data);
            // syscon::logger::LogDebug("Parsing controller config line: %s, %s, %s (expect: %s)", section, name, value, config->ini_section);
            if (strcmp(section, config->ini_section) == 0)
            {
                if (strcmp(name, "driver") == 0)
                {
                    strncpy(config->driver, value, sizeof(config->driver));
                }
                else if (strncmp(name, "KEY_", 4) == 0)
                {
                    ControllerButton button = StringToKey(name + 4);
                    ControllerButton buttonValue = StringToKey(value);
                    if (button >= 2)
                    {
                        config->buttons[button - 2] = buttonValue;
                        return 1; // Success
                    }
                    else
                    {
                        syscon::logger::LogError("Invalid button name: %s", name);
                        return 0;
                    }
                }
                else if (strcmp(name, "left_stick_deadzone") == 0)
                    config->stickDeadzonePercent[0] = atoi(value);
                else if (strcmp(name, "right_stick_deadzone") == 0)
                    config->stickDeadzonePercent[1] = atoi(value);
                else if (strcmp(name, "left_stick_rotation") == 0)
                    config->stickRotationDegrees[0] = atoi(value);
                else if (strcmp(name, "right_stick_rotation") == 0)
                    config->stickRotationDegrees[1] = atoi(value);
                else if (strcmp(name, "left_trigger_deadzone") == 0)
                    config->triggerDeadzonePercent[0] = atoi(value);
                else if (strcmp(name, "right_trigger_deadzone") == 0)
                    config->triggerDeadzonePercent[1] = atoi(value);
                else if (strcmp(name, "swap_dpad_and_lstick") == 0)
                    config->swapDPADandLSTICK = (strcmp(value, "true") ? false : true);
                else if (strcmp(name, "color_body") == 0)
                    config->bodyColor = DecodeColorValue(value);
                else if (strcmp(name, "color_buttons") == 0)
                    config->buttonsColor = DecodeColorValue(value);
                else if (strcmp(name, "color_leftGrip") == 0)
                    config->leftGripColor = DecodeColorValue(value);
                else if (strcmp(name, "color_rightGrip") == 0)
                    config->rightGripColor = DecodeColorValue(value);
                else if (strcmp(name, "color_led") == 0)
                    config->ledColor = DecodeColorValue(value);
                else
                {
                    syscon::logger::LogError("Unknown key: %s", name);
                    return 0; // Unknown key, return error
                }
            }

            return 1; // Success
        }

        Result ReadFromConfig(const char *path, ams::util::ini::Handler h, void *config)
        {
            ams::fs::FileHandle file;
            {
                if (R_FAILED(ams::fs::OpenFile(std::addressof(file), path, ams::fs::OpenMode_Read)))
                {
                    syscon::logger::LogError("Unable to open configuration file: '%s' !", path);
                    return 1;
                }
            }
            ON_SCOPE_EXIT { ams::fs::CloseFile(file); };

            /* Parse the config. */
            syscon::logger::LogDebug("Parsing configuration file: '%s' ...", path);
            return ams::util::ini::ParseFile(file, config, h);
        }
    } // namespace

    Result LoadGlobalConfig(GlobalConfig *config)
    {
        snprintf(config->ini_section, sizeof(config->ini_section), "global");
        if (R_FAILED(ReadFromConfig(GLOBALCONFIG, ParseGlobalConfigLine, config)))
            return 1;

        return 0;
    }

    Result LoadControllerConfig(ControllerConfig *config, uint16_t vendor_id, uint16_t product_id)
    {
        // Load default config
        snprintf(config->ini_section, sizeof(config->ini_section), "default");
        if (R_FAILED(ReadFromConfig(GLOBALCONFIG, ParseControllerConfigLine, config)))
            return 1;

        // Override with vendor specific config
        snprintf(config->ini_section, sizeof(config->ini_section), "%04x-%04x", vendor_id, product_id);
        if (R_FAILED(ReadFromConfig(GLOBALCONFIG, ParseControllerConfigLine, config)))
            return 1;

        return 0;
    }
} // namespace syscon::config