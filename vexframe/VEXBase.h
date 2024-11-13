#pragma once

#if defined(VEX_USES_SPDLOG)
#include <spdlog/spdlog.h>
#endif

// Base header for PCH gen

// VEX
#include <vexcore/containers/Array.h>
#include <vexcore/containers/Tuple.h>
#include <vexcore/utils/CoreTemplates.h>
#include <vexcore/utils/TTraits.h>
#include <vexcore/utils/VMath.h>
#include <vexcore/utils/VUtilsBase.h>
// STL
#include <memory>
#include <vector>
#include <span>