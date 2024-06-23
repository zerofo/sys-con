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
        // Utils function
        std::string convertToLowercase(const std::string &str)
        {
            std::string result = "";
            for (char ch : str)
                result += tolower(ch);
            return result;
        }

        class ConfigINIData
        {
        public:
            std::string ini_section;
            ControllerConfig *controller_config;
            GlobalConfig *global_config;
            bool ini_section_found;

            ConfigINIData(const std::string &ini_section_str, ControllerConfig *config) : ini_section(convertToLowercase(ini_section_str)),
                                                                                          controller_config(config),
                                                                                          global_config(NULL),
                                                                                          ini_section_found(false)
            {
            }

            ConfigINIData(const std::string &ini_section_str, GlobalConfig *config) : ini_section(convertToLowercase(ini_section_str)),
                                                                                      controller_config(NULL),
                                                                                      global_config(config),
                                                                                      ini_section_found(false)
            {
            }
        };

        RGBAColor DecodeColorValue(const char *value)
        {
            RGBAColor color;
            color.rgbaValue = 0x000000FF;

            if (value[0] == '#')
                value = &value[1];

            if (strlen(value) == 8) // R G B A
            {
                color.rgbaValue = strtol(value, NULL, 16);
            }
            else if (strlen(value) == 6) // R G B
            {
                color.rgbaValue = strtol(value, NULL, 16);
                color.rgbaValue = (color.rgbaValue << 8) | 0xFF; // Add Alpha
            }
            else
            {
                syscon::logger::LogError("Invalid color value: %s (Invalid length (Expecting 8 or 6 got %d))", value, strlen(value));
            }

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
            std::string sectionStr = convertToLowercase(section);
            std::string nameStr = convertToLowercase(name);

            if (ini_data->ini_section != sectionStr)
                return 1; // Not the section we are looking for (return success to continue parsing)

            ini_data->ini_section_found = true;

            if (nameStr == "polling_frequency_ms")
                ini_data->global_config->polling_frequency_ms = atoi(value);
            else if (nameStr == "log_level")
                ini_data->global_config->log_level = atoi(value);
            else if (nameStr == "discovery_mode")
                ini_data->global_config->discovery_mode = static_cast<DiscoveryMode>(atoi(value));
            else if (nameStr == "auto_add_controller")
                ini_data->global_config->auto_add_controller = (atoi(value) == 0) ? false : true;
            else if (nameStr == "discovery_vidpid")
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
                syscon::logger::LogError("Unknown key: %s, continue anyway ...", name);
            }

            return 1; // Success
        }

        int ParseControllerConfigLine(void *data, const char *section, const char *name, const char *value)
        {
            ConfigINIData *ini_data = static_cast<ConfigINIData *>(data);
            std::string sectionStr = convertToLowercase(section);
            std::string nameStr = convertToLowercase(name);

            // Note: Below log cause a crash, need to fix it
            // syscon::logger::LogTrace("Parsing controller config line: %s, %s, %s (expect: %s)", section, name, value, ini_data->ini_section.c_str());

            if (ini_data->ini_section != sectionStr)
                return 1; // Not the section we are looking for (return success to continue parsing)

            ini_data->ini_section_found = true;

            if (nameStr == "driver")
                ini_data->controller_config->driver = convertToLowercase(value);
            else if (nameStr == "profile")
                ini_data->controller_config->profile = convertToLowercase(value);
            else if (nameStr == "b")
                ini_data->controller_config->buttons_pin[ControllerButton::B] = atoi(value);
            else if (nameStr == "a")
                ini_data->controller_config->buttons_pin[ControllerButton::A] = atoi(value);
            else if (nameStr == "x")
                ini_data->controller_config->buttons_pin[ControllerButton::X] = atoi(value);
            else if (nameStr == "y")
                ini_data->controller_config->buttons_pin[ControllerButton::Y] = atoi(value);
            else if (nameStr == "lstick_click")
                ini_data->controller_config->buttons_pin[ControllerButton::LSTICK_CLICK] = atoi(value);
            else if (nameStr == "rstick_click")
                ini_data->controller_config->buttons_pin[ControllerButton::RSTICK_CLICK] = atoi(value);
            else if (nameStr == "l")
                ini_data->controller_config->buttons_pin[ControllerButton::L] = atoi(value);
            else if (nameStr == "r")
                ini_data->controller_config->buttons_pin[ControllerButton::R] = atoi(value);
            else if (nameStr == "zl")
                ini_data->controller_config->buttons_pin[ControllerButton::ZL] = atoi(value);
            else if (nameStr == "zr")
                ini_data->controller_config->buttons_pin[ControllerButton::ZR] = atoi(value);
            else if (nameStr == "minus")
                ini_data->controller_config->buttons_pin[ControllerButton::MINUS] = atoi(value);
            else if (nameStr == "plus")
                ini_data->controller_config->buttons_pin[ControllerButton::PLUS] = atoi(value);
            else if (nameStr == "dpad_up")
                ini_data->controller_config->buttons_pin[ControllerButton::DPAD_UP] = atoi(value);
            else if (nameStr == "dpad_right")
                ini_data->controller_config->buttons_pin[ControllerButton::DPAD_RIGHT] = atoi(value);
            else if (nameStr == "dpad_down")
                ini_data->controller_config->buttons_pin[ControllerButton::DPAD_DOWN] = atoi(value);
            else if (nameStr == "dpad_left")
                ini_data->controller_config->buttons_pin[ControllerButton::DPAD_LEFT] = atoi(value);
            else if (nameStr == "capture")
                ini_data->controller_config->buttons_pin[ControllerButton::CAPTURE] = atoi(value);
            else if (nameStr == "home")
                ini_data->controller_config->buttons_pin[ControllerButton::HOME] = atoi(value);
            else if (nameStr == "simulate_home_from_plus_minus")
                ini_data->controller_config->simulateHomeFromPlusMinus = atoi(value) == 0 ? false : true;
            else if (nameStr == "left_stick_x")
                ini_data->controller_config->stickConfig[0].X = DecodeAnalogConfig(value);
            else if (nameStr == "left_stick_y")
                ini_data->controller_config->stickConfig[0].Y = DecodeAnalogConfig(value);
            else if (nameStr == "right_stick_x")
                ini_data->controller_config->stickConfig[1].X = DecodeAnalogConfig(value);
            else if (nameStr == "right_stick_y")
                ini_data->controller_config->stickConfig[1].Y = DecodeAnalogConfig(value);
            else if (nameStr == "left_trigger")
                ini_data->controller_config->triggerConfig[0] = DecodeAnalogConfig(value);
            else if (nameStr == "right_trigger")
                ini_data->controller_config->triggerConfig[1] = DecodeAnalogConfig(value);
            else if (nameStr == "left_stick_deadzone")
                ini_data->controller_config->stickDeadzonePercent[0] = atoi(value);
            else if (nameStr == "right_stick_deadzone")
                ini_data->controller_config->stickDeadzonePercent[1] = atoi(value);
            else if (nameStr == "left_trigger_deadzone")
                ini_data->controller_config->triggerDeadzonePercent[0] = atoi(value);
            else if (nameStr == "right_trigger_deadzone")
                ini_data->controller_config->triggerDeadzonePercent[1] = atoi(value);
            else if (nameStr == "color_body")
                ini_data->controller_config->bodyColor = DecodeColorValue(value);
            else if (nameStr == "color_buttons")
                ini_data->controller_config->buttonsColor = DecodeColorValue(value);
            else if (nameStr == "color_leftgrip")
                ini_data->controller_config->leftGripColor = DecodeColorValue(value);
            else if (nameStr == "color_rightgrip")
                ini_data->controller_config->rightGripColor = DecodeColorValue(value);
            else
            {
                syscon::logger::LogError("Unknown key: %s, continue anyway ...", name);
            }

            return 1; // Success
        }

        ams::Result ReadFromConfig(const char *path, ams::util::ini::Handler h, void *config)
        {
            ams::fs::FileHandle file;

            if (R_FAILED(ams::fs::OpenFile(std::addressof(file), path, ams::fs::OpenMode_Read)))
            {
                syscon::logger::LogError("Unable to open configuration file: '%s' !", path);
                return 1;
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
        ConfigINIData cfg("global", config);

        syscon::logger::LogDebug("Loading global config: '%s' ...", CONFIG_FULLPATH);

        R_TRY(ReadFromConfig(CONFIG_FULLPATH, ParseGlobalConfigLine, &cfg));

        R_SUCCEED();
    }

    ams::Result AddControllerToConfig(const char *path, std::string section, std::string profile)
    {
        s64 fileOffset = 0;
        std::stringstream ss;
        ams::fs::FileHandle file;

        /* Get the current time. */
        ams::time::PosixTime time;
        ams::time::StandardUserSystemClock::GetCurrentTime(&time);
        ams::time::CalendarTime calendar = ams::time::impl::util::ToCalendarTimeInUtc(time);

        /* Open the config file. */
        if (R_FAILED(ams::fs::OpenFile(std::addressof(file), path, ams::fs::OpenMode_Write | ams::fs::OpenMode_AllowAppend)))
        {
            syscon::logger::LogError("Unable to open configuration file: '%s' !", path);
            return 1;
        }

        ON_SCOPE_EXIT { ams::fs::CloseFile(file); };

        /* Write the config. */
        ss << std::endl;
        ss << "[" << section << "] ;Automatically added on " << static_cast<int>(calendar.year) << "-" << static_cast<int>(calendar.month) << "-" << static_cast<int>(calendar.day) << " " << static_cast<int>(calendar.hour) << ":" << static_cast<int>(calendar.minute) << ":" << static_cast<int>(calendar.second) << "UTC" << std::endl;
        if (profile != "")
        {
            ss << "profile=" << profile << std::endl;
        }
        else
        {
            ss << "b=1" << std::endl;
            ss << "a=2" << std::endl;
            ss << "x=3" << std::endl;
            ss << "y=4" << std::endl;
            ss << "l=5" << std::endl;
            ss << "r=6" << std::endl;
            ss << "zl=7" << std::endl;
            ss << "zr=8" << std::endl;
            ss << "minus=9" << std::endl;
            ss << "plus=10" << std::endl;
            ss << "capture=11" << std::endl;
            ss << "home=12" << std::endl;
            ss << "lstick_click=13" << std::endl;
            ss << "rstick_click=14" << std::endl;
        }

        R_TRY(ams::fs::GetFileSize(&fileOffset, file));

        ams::Result rc = ams::fs::WriteFile(file, fileOffset, ss.str().c_str(), ss.str().length(), ams::fs::WriteOption::Flush);
        if (R_FAILED(rc))
        {
            syscon::logger::LogError("Failed to write configuration file: '%s' !", path);
            return 1;
        }

        R_SUCCEED();
    }

    ams::Result LoadControllerConfig(ControllerConfig *config, uint16_t vendor_id, uint16_t product_id, bool auto_add_controller, const std::string &default_profile)
    {
        ControllerVidPid controllerVidPid(vendor_id, product_id);
        ConfigINIData cfg_default("default", config);
        ConfigINIData cfg_controller(controllerVidPid, config);

        syscon::logger::LogDebug("Loading controller config: '%s' [default] ...", CONFIG_FULLPATH);
        R_TRY(ReadFromConfig(CONFIG_FULLPATH, ParseControllerConfigLine, &cfg_default));

        // Override with vendor specific config
        syscon::logger::LogDebug("Loading controller config: '%s' [%s] ...", CONFIG_FULLPATH, std::string(controllerVidPid).c_str());
        R_TRY(ReadFromConfig(CONFIG_FULLPATH, ParseControllerConfigLine, &cfg_controller));

        if (!cfg_controller.ini_section_found && auto_add_controller)
        {
            syscon::logger::LogDebug("Controller not found in config file, adding it ...");
            R_TRY(AddControllerToConfig(CONFIG_FULLPATH, std::string(controllerVidPid), default_profile));

            syscon::logger::LogDebug("Reloading controller config: '%s' [%s] ...", CONFIG_FULLPATH, std::string(controllerVidPid).c_str());
            R_TRY(ReadFromConfig(CONFIG_FULLPATH, ParseControllerConfigLine, &cfg_controller));
        }

        // Check if have a "profile"
        if (config->profile.length() > 0)
        {
            syscon::logger::LogDebug("Loading controller config: '%s' (Profile: [%s]) ... ", CONFIG_FULLPATH, config->profile.c_str());
            ConfigINIData cfg_profile(config->profile, config);
            R_TRY(ReadFromConfig(CONFIG_FULLPATH, ParseControllerConfigLine, &cfg_profile));

            // Re-Override with vendor specific config
            // We are doing this to allow the profile to be overrided by the vendor specific config
            // In other words we would like to have [default] overrided by [profile] overrided by [vid-pid]
            R_TRY(ReadFromConfig(CONFIG_FULLPATH, ParseControllerConfigLine, &cfg_controller));
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