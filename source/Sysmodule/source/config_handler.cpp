#include "switch.h"
#include "config_handler.h"
#include "Controllers.h"
#include "ControllerConfig.h"
#include "logger.h"
#include <cstring>
#include <cstdlib>
#include <stratosphere.hpp>
#include <stratosphere/util/util_ini.hpp>

namespace syscon::config
{
    namespace
    {
        class ConfigINIData
        {
        public:
            std::string ini_section;
            ControllerConfig *config;
            GlobalConfig *global_config;
        };

        // Utils function
        std::string convertToLowercase(const std::string &str)
        {
            std::string result = "";
            for (char ch : str)
                result += tolower(ch);
            return result;
        }

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

        ControllerAnalogConfig DecodeAnalogConfig(const std::string &cfg)
        {
            ControllerAnalogConfig analogCfg;
            std::string stickcfg = convertToLowercase(cfg);

            analogCfg.bind = ControllerAnalogBinding::ControllerAnalogBinding_Unknown;
            analogCfg.sign = stickcfg[0] == '-' ? -1.0 : 1.0;

            if (stickcfg[0] == '-' || stickcfg[0] == '+')
                stickcfg = stickcfg.substr(1);

            if (stickcfg == "x")
                analogCfg.bind = ControllerAnalogBinding::ControllerAnalogBinding_X;
            else if (stickcfg == "y")
                analogCfg.bind = ControllerAnalogBinding::ControllerAnalogBinding_Y;
            else if (stickcfg == "z")
                analogCfg.bind = ControllerAnalogBinding::ControllerAnalogBinding_Z;
            else if (stickcfg == "rz")
                analogCfg.bind = ControllerAnalogBinding::ControllerAnalogBinding_RZ;
            else if (stickcfg == "rx")
                analogCfg.bind = ControllerAnalogBinding::ControllerAnalogBinding_RX;
            else if (stickcfg == "ry")
                analogCfg.bind = ControllerAnalogBinding::ControllerAnalogBinding_RY;

            return analogCfg;
        }

        int ParseGlobalConfigLine(void *data, const char *section, const char *name, const char *value)
        {
            ConfigINIData *ini_data = static_cast<ConfigINIData *>(data);

            if (ini_data->ini_section != section)
                return 1; // Not the section we are looking for (return success to continue parsing)

            if (strcmp(name, "polling_frequency_ms") == 0)
                ini_data->global_config->polling_frequency_ms = atoi(value);
            else if (strcmp(name, "log_level") == 0)
                ini_data->global_config->log_level = atoi(value);
            else if (strcmp(name, "discovery_mode") == 0)
                ini_data->global_config->discovery_mode = static_cast<DiscoveryMode>(atoi(value));
            else if (strcmp(name, "discovery_vidpid") == 0)
            {
                char *tok = strtok(const_cast<char *>(value), ",");

                while (tok != NULL)
                {
                    ini_data->global_config->discovery_vidpid.push_back(ControllerVidPid(tok));
                    tok = strtok(NULL, ",");
                }
            }
            else
            {
                syscon::logger::LogError("Unknown key: %s", name);
                return 0; // Unknown key, return error
            }

            return 1; // Success
        }

        int ParseControllerConfigLine(void *data, const char *section, const char *name, const char *value)
        {
            ConfigINIData *ini_data = static_cast<ConfigINIData *>(data);

            // syscon::logger::LogTrace("Parsing controller config line: %s, %s, %s (expect: %s)", section, name, value, ini_data->ini_section.c_str());

            if (ini_data->ini_section != section)
                return 1; // Not the section we are looking for (return success to continue parsing)

            if (strcmp(name, "driver") == 0)
                ini_data->config->driver = value;
            else if (strcmp(name, "profile") == 0)
                ini_data->config->profile = value;
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
            else if (strcmp(name, "simulate_home_from_plus_minus") == 0)
                ini_data->config->simulateHomeFromPlusMinus = atoi(value) == 0 ? false : true;
            else if (strcmp(name, "left_stick_x") == 0)
                ini_data->config->stickConfig[0].X = DecodeAnalogConfig(value);
            else if (strcmp(name, "left_stick_y") == 0)
                ini_data->config->stickConfig[0].Y = DecodeAnalogConfig(value);
            else if (strcmp(name, "right_stick_x") == 0)
                ini_data->config->stickConfig[1].X = DecodeAnalogConfig(value);
            else if (strcmp(name, "right_stick_y") == 0)
                ini_data->config->stickConfig[1].Y = DecodeAnalogConfig(value);
            else if (strcmp(name, "left_trigger") == 0)
                ini_data->config->triggerConfig[0] = DecodeAnalogConfig(value);
            else if (strcmp(name, "right_trigger") == 0)
                ini_data->config->triggerConfig[1] = DecodeAnalogConfig(value);
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

        syscon::logger::LogDebug("Loading global config: '%s' ...", CONFIG_FULLPATH);

        cfg.ini_section = "global";
        R_TRY(ReadFromConfig(CONFIG_FULLPATH, ParseGlobalConfigLine, &cfg));

        R_SUCCEED();
    }

    ams::Result LoadControllerConfig(ControllerConfig *config, uint16_t vendor_id, uint16_t product_id)
    {
        ControllerVidPid controllerVidPid(vendor_id, product_id);
        ConfigINIData cfg;
        cfg.config = config;

        syscon::logger::LogDebug("Loading controller config: '%s' [default] ...", CONFIG_FULLPATH);

        cfg.ini_section = "default";
        R_TRY(ReadFromConfig(CONFIG_FULLPATH, ParseControllerConfigLine, &cfg));

        // Override with vendor specific config
        syscon::logger::LogDebug("Loading controller config: '%s' [%s] ...", CONFIG_FULLPATH, std::string(controllerVidPid).c_str());

        cfg.ini_section = convertToLowercase(controllerVidPid);
        R_TRY(ReadFromConfig(CONFIG_FULLPATH, ParseControllerConfigLine, &cfg));

        // Check if have a "profile"
        if (config->profile.length() > 0)
        {
            syscon::logger::LogDebug("Loading controller config: '%s' (Profile: [%s]) ... ", CONFIG_FULLPATH, config->profile);
            cfg.ini_section = config->profile;
            R_TRY(ReadFromConfig(CONFIG_FULLPATH, ParseControllerConfigLine, &cfg));

            // Re-Override with vendor specific config
            // We are doing this to allow the profile to be overrided by the vendor specific config
            // In other words we would like to have [default] overrided by [profile] overrided by [vid-pid]
            cfg.ini_section = convertToLowercase(controllerVidPid);
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

        if (config->stickConfig[0].X.bind == ControllerAnalogBinding_Unknown)
            config->stickConfig[0].X.bind = ControllerAnalogBinding_X;
        if (config->stickConfig[0].Y.bind == ControllerAnalogBinding_Unknown)
            config->stickConfig[0].Y.bind = ControllerAnalogBinding_Y;
        if (config->stickConfig[1].X.bind == ControllerAnalogBinding_Unknown)
            config->stickConfig[1].X.bind = ControllerAnalogBinding_RZ;
        if (config->stickConfig[1].Y.bind == ControllerAnalogBinding_Unknown)
            config->stickConfig[1].Y.bind = ControllerAnalogBinding_Z;

        if (config->triggerConfig[0].bind == ControllerAnalogBinding_Unknown)
            config->triggerConfig[0].bind = ControllerAnalogBinding_RX;
        if (config->triggerConfig[1].bind == ControllerAnalogBinding_Unknown)
            config->triggerConfig[1].bind = ControllerAnalogBinding_RY;

        if (config->buttons_pin[ControllerButton::B] == 0 && config->buttons_pin[ControllerButton::A] == 0 && config->buttons_pin[ControllerButton::Y] == 0 && config->buttons_pin[ControllerButton::X] == 0)
            syscon::logger::LogError("No buttons configured for this controller [%04x-%04x] - Stick might works but buttons will not work (https://github.com/o0Zz/sys-con/blob/master/doc/Troubleshooting.md)", vendor_id, product_id);
        else
            syscon::logger::LogInfo("Controller successfully loaded (B=%d, A=%d, Y=%d, X=%d, ...) !", config->buttons_pin[ControllerButton::B], config->buttons_pin[ControllerButton::A], config->buttons_pin[ControllerButton::Y], config->buttons_pin[ControllerButton::X]);

        R_SUCCEED();
    }
} // namespace syscon::config