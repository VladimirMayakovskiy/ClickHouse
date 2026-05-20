#include "CHTPDECompiler.h"

#if USE_EMBEDDED_COMPILER && USE_TPDE_BACKEND

#include "CHTPDECompilerX64.h"

namespace ch_tpde {

CHTPDECompiler::~CHTPDECompiler() = default;

std::unique_ptr<CHTPDECompiler> CHTPDECompiler::create() {
    return x64::create_compiler();
}

} // namespace ch_tpde

#endif
