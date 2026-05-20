#pragma once

#if USE_EMBEDDED_COMPILER && USE_TPDE_BACKEND

#include <Interpreters/JIT/CHTPDECompiler.h>
#include "CHTPDEAdaptor.h"

#include <tpde/CompilerBase.hpp>
#include <Interpreters/JIT/CHTPDEFunction.h>
#include <Common/logger_useful.h>
#include <tpde/x64/CompilerX64.hpp>

namespace DB
{
static LoggerPtr getLogger()
{
    return ::getLogger("CHTPDECompilerBase");
}
} // namespace DB

namespace ch_tpde
{

template <typename Adaptor, typename Derived, typename Config>
struct CHTPDECompilerBase : public CHTPDECompiler, tpde::CompilerBase<CHTPDEAdaptor, Derived, Config>
{
    using Base = tpde::CompilerBase<CHTPDEAdaptor, Derived, Config>;

    using IRInstRef = typename Base::IRInstRef;
    using IRValueRef = typename Base::IRValueRef;
    using IRBlockRef = typename Base::IRBlockRef;
    using IRFuncRef = typename Base::IRFuncRef;
    using ScratchReg = typename Base::ScratchReg;
    using ValuePartRef = typename Base::ValuePartRef;
    using ValuePart = typename Base::ValuePart;
    using ValueRef = typename Base::ValueRef;
    using InstRange = typename Base::InstRange;

    struct ValRefSpecial {
        uint8_t mode = 4;
        IRValueRef value;
    };

    CHTPDECompilerBase(CHTPDEAdaptor *adaptor_) : Base{adaptor_} 
    {
        static_assert(tpde::Compiler<Derived, Config>);
        static_assert(std::is_same_v<Adaptor, CHTPDEAdaptor>);
    }

    Derived *derived() noexcept { return static_cast<Derived *>(this); }
    const Derived *derived() const { return static_cast<Derived *>(this); }

    bool cur_func_may_emit_calls() const { return false; }
    static tpde::SymRef cur_personality_func() { return {}; }
    bool try_force_fixed_assignment(IRValueRef value) const noexcept { return false; }

    struct ValueParts {
        static uint32_t count() { return 1; }
        static uint32_t size_bytes(uint32_t) { return 32; }
        static tpde::RegBank reg_bank(uint32_t) { return tpde::x64::PlatformConfig::FP_BANK; }
    };

    static ValueParts val_parts(IRValueRef) { return ValueParts{}; }
    static std::optional<ValRefSpecial> val_ref_special(IRValueRef) { return {}; }
    ValuePart val_part_ref_special(ValRefSpecial&, uint32_t /* part_idx */) noexcept { return {}; }
    static void define_func_idx(IRFuncRef, uint32_t) {}

    bool compile_inst(IRInstRef inst, InstRange remaining) noexcept
    {
        LOG_TRACE(DB::getLogger(), "Compiling inst {}", this->adaptor->inst_fmt_ref(inst));
        return true;
    }

    bool compile(const DB::CHTPDEFunction& function);
    bool compile_and_map(const DB::CHTPDEFunction& function, std::function<void *(std::string_view)> resolver) override;
};

template <typename Adaptor, typename Derived, typename Config>
bool CHTPDECompilerBase<Adaptor, Derived, Config>::compile(const DB::CHTPDEFunction& function_)
{
    if (!this->adaptor->switch_func(&function_))
        return false;
    return Base::compile();
}

template <typename Adaptor, typename Derived, typename Config>
bool CHTPDECompilerBase<Adaptor, Derived, Config>::compile_and_map(const DB::CHTPDEFunction& function_, std::function<void *(std::string_view)> resolver)
{
    if (!compile(function_))
        return false;
    return true;
}

} // namespace ch_tpde

#endif
