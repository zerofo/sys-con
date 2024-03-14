#pragma once
#include "switch.h"
#include "vapours/results/results_common.hpp"

namespace syscon::psc
{
    ams::Result Initialize();
    void Exit();
};