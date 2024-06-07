#include "switch.h"
#include "config_handler.h"
#include "Controllers.h"
#include "ControllerConfig.h"
#include "logger.h"
#include <cstring>
#include <cstdlib>
#include <vector>
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
            std::vector<ControllerVidPid> *vid_pid;
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
                else if (strcmp(name, "discovery_mode") == 0)
                    ini_data->global_config->discovery_mode = static_cast<DiscoveryMode>(atoi(value));
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

        int ParseControllerList(void *data, const char *section, const char *name, const char *value)
        {
            (void)name;
            (void)value;

            ConfigINIData *ini_data = static_cast<ConfigINIData *>(data);

            std::string sectionStr = section;

            std::size_t delimIdx = sectionStr.find('-');
            if (delimIdx != std::string::npos)
            {
                std::string vid = sectionStr.substr(0, delimIdx);
                std::string pid = sectionStr.substr(delimIdx + 1);

                ControllerVidPid value((uint16_t)strtol(vid.c_str(), NULL, 16), (uint16_t)strtol(pid.c_str(), NULL, 16));

                if (std::find(ini_data->vid_pid->begin(), ini_data->vid_pid->end(), value) == ini_data->vid_pid->end())
                    ini_data->vid_pid->push_back(value);
            }

            return 1; // Success
        }

        ams::Result ReadFromConfig(const char *path, ams::util::ini::Handler h, void *config)
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
            Result rc = ams::util::ini::ParseFile(file, config, h);
            if (R_FAILED(rc))
            {
                syscon::logger::LogError("Failed to parse configuration file: '%s' !", path);
                return 1;
            }

            R_SUCCEED();
        }
    } // namespace

    ams::Result LoadGlobalConfig(GlobalConfig *config)
    {
        ConfigINIData cfg;
        cfg.global_config = config;
        snprintf(cfg.ini_section, sizeof(cfg.ini_section), "global");

        syscon::logger::LogDebug("Loading global config: '%s' ...", CONFIG_FULLPATH);

        R_TRY(ReadFromConfig(CONFIG_FULLPATH, ParseGlobalConfigLine, &cfg));

        R_SUCCEED();
    }

    ams::Result LoadControllerList(std::vector<ControllerVidPid> *vid_pid)
    {
        ConfigINIData cfg;
        cfg.vid_pid = vid_pid;

        syscon::logger::LogDebug("Loading controller list: '%s' ...", CONFIG_FULLPATH);

        R_TRY(ReadFromConfig(CONFIG_FULLPATH, ParseControllerList, &cfg));

        R_SUCCEED();
    }

    ams::Result LoadControllerConfig(ControllerConfig *config, uint16_t vendor_id, uint16_t product_id)
    {
        ConfigINIData cfg;
        cfg.config = config;

        syscon::logger::LogDebug("Loading controller config: '%s' [default] ...", CONFIG_FULLPATH);

        snprintf(cfg.ini_section, sizeof(cfg.ini_section), "default");
        R_TRY(ReadFromConfig(CONFIG_FULLPATH, ParseControllerConfigLine, &cfg));

        // Override with vendor specific config
        syscon::logger::LogDebug("Loading controller config: '%s' [%04x-%04x] ...", CONFIG_FULLPATH, vendor_id, product_id);
        snprintf(cfg.ini_section, sizeof(cfg.ini_section), "%04x-%04x", vendor_id, product_id);
        R_TRY(ReadFromConfig(CONFIG_FULLPATH, ParseControllerConfigLine, &cfg));

        // Check if have a "profile"
        if (strlen(config->profile) != 0)
        {
            syscon::logger::LogDebug("Loading controller config: '%s' (Profile: %s) ... ", CONFIG_FULLPATH, config->profile);
            snprintf(cfg.ini_section, sizeof(cfg.ini_section), "%s", config->profile);
            R_TRY(ReadFromConfig(CONFIG_FULLPATH, ParseControllerConfigLine, &cfg));

            // Re-Override with vendor specific config
            // We are doing this to allow the profile to be overrided by the vendor specific config
            // In other words we will like to have default overrided by profile overrided by vendor specific
            snprintf(cfg.ini_section, sizeof(cfg.ini_section), "%04x-%04x", vendor_id, product_id);
            R_TRY(ReadFromConfig(CONFIG_FULLPATH, ParseControllerConfigLine, &cfg));
        }

        for (int i = 0; i < ControllerButton::COUNT; i++)
        {
            if (config->buttons_pin[i] >= MAX_CONTROLLER_BUTTONS)
            {
                syscon::logger::LogError("Invalid button pin: %d (Max: %d)", config->buttons_pin[i], MAX_CONTROLLER_BUTTONS);
                return 1;
            }
        }

        if (config->buttons_pin[ControllerButton::B] == 0 && config->buttons_pin[ControllerButton::A] == 0 && config->buttons_pin[ControllerButton::Y] == 0 && config->buttons_pin[ControllerButton::X] == 0)
            syscon::logger::LogError("No buttons configured for this controller [%04x-%04x] - Stick might works but buttons will not !", vendor_id, product_id);
        else
            syscon::logger::LogInfo("Controller successfully loaded (B=%d, A=%d, Y=%d, X=%d, ...) !", config->buttons_pin[ControllerButton::B], config->buttons_pin[ControllerButton::A], config->buttons_pin[ControllerButton::Y], config->buttons_pin[ControllerButton::X]);

        R_SUCCEED();
    }
} // namespace syscon::config