#pragma once

#include "config.h"

#if USE_EMBEDDED_COMPILER && USE_TPDE_BACKEND

#include <memory>

#include <Interpreters/JIT/CHTPDECompiler.h>

namespace ch_tpde::x64 {

std::unique_ptr<CHTPDECompiler> create_compiler();

} // namespace ch_tpde::x64

#endif
