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
        class ConfigINIData
        {
        public:
            char ini_section[32] = {0};
            struct ControllerConfig *config;
            struct GlobalConfig *global_config;
        };

        RGBAColor DecodeColorValue(const char *value)
        {
            RGBAColor color = {0, 0, 0, 255};

            if (value[0] != '#')
            {
                syscon::logger::LogError("Invalid color value: %s (Missing # at the begining)", value);
                return color;
            }

            value = &value[1];

            if (strlen(value) != 8)
            {
                syscon::logger::LogError("Invalid color value: %s (Invalid length (Expectred 8 got %d))", value, strlen(value));
                return color;
            }

            color.rgbaValue = strtol(value, NULL, 16);
            return color;
        }

        int ParseGlobalConfigLine(void *data, const char *section, const char *name, const char *value)
        {
            ConfigINIData *ini_data = static_cast<ConfigINIData *>(data);

            if (strcmp(section, ini_data->ini_section) == 0)
            {
                if (strcmp(name, "polling_frequency_ms") == 0)
                    ini_data->global_config->polling_frequency_ms = atoi(value);
                else if (strcmp(name, "log_level") == 0)
                    ini_data->global_config->log_level = atoi(value);
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
            ConfigINIData *ini_data = static_cast<ConfigINIData *>(data);

            // syscon::logger::LogTrace("Parsing controller config line: %s, %s, %s (expect: %s)", section, name, value, config->ini_section);

            if (strcmp(section, ini_data->ini_section) != 0)
                return 1; // Not the section we are looking for (return success to continue parsing)

            if (strcmp(name, "driver") == 0)
                strncpy(ini_data->config->driver, value, sizeof(ini_data->config->driver));
            else if (strcmp(name, "profile") == 0)
                strncpy(ini_data->config->profile, value, sizeof(ini_data->config->profile));
            else if (strcmp(name, "B") == 0)
                ini_data->config->buttons_pin[ControllerButton::B] = atoi(value);
            else if (strcmp(name, "A") == 0)
                ini_data->config->buttons_pin[ControllerButton::A] = atoi(value);
            else if (strcmp(name, "X") == 0)
                ini_data->config->buttons_pin[ControllerButton::X] = atoi(value);
            else if (strcmp(name, "Y") == 0)
                ini_data->config->buttons_pin[ControllerButton::Y] = atoi(value);
            else if (strcmp(name, "lstick_click") == 0)
                ini_data->config->buttons_pin[ControllerButton::LSTICK_CLICK] = atoi(value);
            else if (strcmp(name, "rstick_click") == 0)
                ini_data->config->buttons_pin[ControllerButton::RSTICK_CLICK] = atoi(value);
            else if (strcmp(name, "L") == 0)
                ini_data->config->buttons_pin[ControllerButton::L] = atoi(value);
            else if (strcmp(name, "R") == 0)
                ini_data->config->buttons_pin[ControllerButton::R] = atoi(value);
            else if (strcmp(name, "ZL") == 0)
                ini_data->config->buttons_pin[ControllerButton::ZL] = atoi(value);
            else if (strcmp(name, "ZR") == 0)
                ini_data->config->buttons_pin[ControllerButton::ZR] = atoi(value);
            else if (strcmp(name, "minus") == 0)
                ini_data->config->buttons_pin[ControllerButton::MINUS] = atoi(value);
            else if (strcmp(name, "plus") == 0)
                ini_data->config->buttons_pin[ControllerButton::PLUS] = atoi(value);
            else if (strcmp(name, "dpad_up") == 0)
                ini_data->config->buttons_pin[ControllerButton::DPAD_UP] = atoi(value);
            else if (strcmp(name, "dpad_right") == 0)
                ini_data->config->buttons_pin[ControllerButton::DPAD_RIGHT] = atoi(value);
            else if (strcmp(name, "dpad_down") == 0)
                ini_data->config->buttons_pin[ControllerButton::DPAD_DOWN] = atoi(value);
            else if (strcmp(name, "dpad_left") == 0)
                ini_data->config->buttons_pin[ControllerButton::DPAD_LEFT] = atoi(value);
            else if (strcmp(name, "capture") == 0)
                ini_data->config->buttons_pin[ControllerButton::CAPTURE] = atoi(value);
            else if (strcmp(name, "home") == 0)
                ini_data->config->buttons_pin[ControllerButton::HOME] = atoi(value);
            else if (strcmp(name, "left_stick_deadzone") == 0)
                ini_data->config->stickDeadzonePercent[0] = atoi(value);
            else if (strcmp(name, "right_stick_deadzone") == 0)
                ini_data->config->stickDeadzonePercent[1] = atoi(value);
            else if (strcmp(name, "left_trigger_deadzone") == 0)
                ini_data->config->triggerDeadzonePercent[0] = atoi(value);
            else if (strcmp(name, "right_trigger_deadzone") == 0)
                ini_data->config->triggerDeadzonePercent[1] = atoi(value);
            else if (strcmp(name, "color_body") == 0)
                ini_data->config->bodyColor = DecodeColorValue(value);
            else if (strcmp(name, "color_buttons") == 0)
                ini_data->config->buttonsColor = DecodeColorValue(value);
            else if (strcmp(name, "color_leftGrip") == 0)
                ini_data->config->leftGripColor = DecodeColorValue(value);
            else if (strcmp(name, "color_rightGrip") == 0)
                ini_data->config->rightGripColor = DecodeColorValue(value);
            else
            {
                syscon::logger::LogError("Unknown key: %s", name);
                return 0; // Unknown key, return error
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
            return ams::util::ini::ParseFile(file, config, h);
        }
    } // namespace

    Result LoadGlobalConfig(GlobalConfig *config)
    {
        ConfigINIData cfg;
        cfg.global_config = config;
        snprintf(cfg.ini_section, sizeof(cfg.ini_section), "global");

        syscon::logger::LogDebug("Loading global config: '%s' ...", CONFIG_FULLPATH);

        if (R_FAILED(ReadFromConfig(CONFIG_FULLPATH, ParseGlobalConfigLine, &cfg)))
            return 1;

        return 0;
    }

    Result LoadControllerConfig(ControllerConfig *config, uint16_t vendor_id, uint16_t product_id)
    {
        ConfigINIData cfg;
        cfg.config = config;

        syscon::logger::LogDebug("Loading controller config: '%s' (default) ...", CONFIG_FULLPATH);

        snprintf(cfg.ini_section, sizeof(cfg.ini_section), "default");
        if (R_FAILED(ReadFromConfig(CONFIG_FULLPATH, ParseControllerConfigLine, &cfg)))
            return 1;

        // Override with vendor specific config
        syscon::logger::LogDebug("Loading controller config: '%s' (%04x-%04x) ...", CONFIG_FULLPATH, vendor_id, product_id);
        snprintf(cfg.ini_section, sizeof(cfg.ini_section), "%04x-%04x", vendor_id, product_id);
        if (R_FAILED(ReadFromConfig(CONFIG_FULLPATH, ParseControllerConfigLine, &cfg)))
            return 1;

        // Check if have a "profile"
        if (strlen(config->profile) != 0)
        {
            syscon::logger::LogDebug("Loading controller config: '%s' (Profile: %s) ... ", CONFIG_FULLPATH, config->profile);
            snprintf(cfg.ini_section, sizeof(cfg.ini_section), "%s", config->profile);
            if (R_FAILED(ReadFromConfig(CONFIG_FULLPATH, ParseControllerConfigLine, &cfg)))
                return 1;
        }

        for (int i = 0; i < ControllerButton::COUNT; i++)
        {
            if (config->buttons_pin[i] >= MAX_CONTROLLER_BUTTONS)
            {
                syscon::logger::LogError("Invalid button pin: %d (Max: %d)", config->buttons_pin[i], MAX_CONTROLLER_BUTTONS);
                return 1;
            }
        }

        syscon::logger::LogDebug("Loading controller successfull (B=%d, A=%d, Y=%d, X=%d, ...)", config->buttons_pin[ControllerButton::B], config->buttons_pin[ControllerButton::A], config->buttons_pin[ControllerButton::Y], config->buttons_pin[ControllerButton::X]);

        return 0;
    }
} // namespace syscon::config