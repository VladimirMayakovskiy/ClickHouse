#pragma once

#include "config.h"

#if USE_EMBEDDED_COMPILER && USE_TPDE_BACKEND

#include <memory>
#include <Interpreters/JIT/CHTPDEFunction.h>

namespace ch_tpde {

class CHTPDECompiler {
protected:
    CHTPDECompiler() = default;

public:
    virtual ~CHTPDECompiler();

    CHTPDECompiler(const CHTPDECompiler &) = delete;
    CHTPDECompiler &operator=(const CHTPDECompiler &) = delete;

    static std::unique_ptr<CHTPDECompiler> create();

    virtual bool compile_and_map(const DB::CHTPDEFunction& function, std::function<void *(std::string_view)> resolver) = 0;
};

} // namespace ch_tpde

#endif
