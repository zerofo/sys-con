#pragma once

#include "IController.h"
#include <vector>

class BaseController : public IController
{
protected:
    std::vector<IUSBEndpoint *> m_inPipe;
    std::vector<IUSBEndpoint *> m_outPipe;
    std::vector<IUSBInterface *> m_interfaces;

public:
    BaseController(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger);
    virtual ~BaseController() override;

    virtual ams::Result Initialize() override;
    virtual void Exit() override;

    virtual ams::Result OpenInterfaces();
    virtual void CloseInterfaces();

    virtual bool Support(ControllerFeature feature) override;

    virtual uint16_t GetInputCount() override;

    virtual ams::Result ReadInput(NormalizedButtonData *normalData, uint16_t *input_idx) = 0;

    ams::Result SetRumble(uint16_t input_idx, float amp_high, float amp_low) override;

    // Helper functions
    float Normalize(uint8_t deadzonePercent, int32_t value, int32_t min, int32_t max);
};