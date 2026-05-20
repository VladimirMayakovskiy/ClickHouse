#include "CHTPDECompilerX64.h"

#if USE_EMBEDDED_COMPILER && USE_TPDE_BACKEND

#include <memory>

#include "CHTPDEAdaptor.h"
#include "CHTPDECompilerBase.h"

#include <tpde/x64/CompilerX64.hpp>

namespace ch_tpde::x64 {

struct CompilerConfig : tpde::x64::PlatformConfig {};

struct CHTPDECompilerX64 : tpde::x64::CompilerX64<CHTPDEAdaptor,
                                                  CHTPDECompilerX64,
                                                  CHTPDECompilerBase,
                                                  CompilerConfig> {
    using Base = tpde::x64::CompilerX64<CHTPDEAdaptor, CHTPDECompilerX64, CHTPDECompilerBase, CompilerConfig>;

    std::unique_ptr<CHTPDEAdaptor> tpde_adaptor;

    explicit CHTPDECompilerX64(std::unique_ptr<CHTPDEAdaptor> &&adaptor_)
        : Base{adaptor_.get()}, tpde_adaptor(std::move(adaptor_)) {
        static_assert(tpde::Compiler<CHTPDECompilerX64, tpde::x64::PlatformConfig>);
    }
};

std::unique_ptr<CHTPDECompiler> create_compiler() {
    auto adaptor = std::make_unique<CHTPDEAdaptor>();
    return std::make_unique<CHTPDECompilerX64>(std::move(adaptor));
}

} // namespace ch_tpde::x64

#endif
