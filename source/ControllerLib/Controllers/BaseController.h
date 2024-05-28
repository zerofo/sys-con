#pragma once

#include "IController.h"

class BaseController : public IController
{
protected:
    IUSBEndpoint *m_inPipe = nullptr;
    IUSBEndpoint *m_outPipe = nullptr;
    IUSBInterface *m_interface = nullptr;

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

    ams::Result SetRumble(uint16_t input_idx, uint8_t strong_magnitude, uint8_t weak_magnitude);

    // Helper functions
    float Normalize(uint8_t deadzonePercent, int32_t value, int32_t min, int32_t max);
};