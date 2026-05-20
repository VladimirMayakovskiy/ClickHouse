#pragma once

#if USE_EMBEDDED_COMPILER && USE_TPDE_BACKEND

#include <Interpreters/JIT/CHTPDEFunction.h>
#include <tpde/ValLocalIdx.hpp>

#include <ranges>

namespace ch_tpde 
{
    enum class Inst : uint8_t
    {
        DagNode  = 0,
        LoopInit = 1,
        LoopCond = 2,
        LoopExit = 3,
        Return   = 4,
    };
}

namespace
{

constexpr ch_tpde::Inst inst_t(uint32_t v) { return ch_tpde::Inst(v & 0b111); }
constexpr uint32_t inst_v(uint32_t v) { return v >> 3; }
constexpr uint32_t create_inst(ch_tpde::Inst t, uint32_t v = 0) { return (v << 3) | uint32_t(t); }

}

namespace ch_tpde {

struct CHTPDEAdaptor
{
    using IRValueRef = uintptr_t;
    using IRInstRef = uint32_t;
    using IRBlockRef = uint32_t;
    using IRFuncRef = const DB::CHTPDEFunction *;

    static constexpr IRValueRef INVALID_VALUE_REF = static_cast<IRBlockRef>(~0u);
    static constexpr IRBlockRef INVALID_BLOCK_REF = static_cast<IRBlockRef>(~0u);
    static constexpr IRFuncRef INVALID_FUNC_REF = nullptr;

    static constexpr bool TPDE_PROVIDES_HIGHEST_VAL_IDX = true;
    static constexpr bool TPDE_LIVENESS_VISIT_ARGS = true;

    const DB::CHTPDEFunction *function = nullptr;

    using Inst = ch_tpde::Inst;

    enum class EBlock
    {
        Entry = 0,
        Loop  = 1,
        Exit  = 2,
    };

    struct BlockAux
    {
        uint32_t aux1 = 0;
        uint32_t aux2 = 0;
    };

    struct BlockInfo
    {
        EBlock block;
        BlockAux aux;
    };

    std::array<BlockInfo, 3> blocks = {
        BlockInfo{.block = EBlock::Entry, .aux = {}},
        BlockInfo{.block = EBlock::Loop,  .aux = {}},
        BlockInfo{.block = EBlock::Exit,  .aux = {}},
    };

    CHTPDEAdaptor() = default;

    uint32_t func_count() const { return 1; } // all dag is func
    auto funcs() const { return std::ranges::views::single(function); }
    auto funcs_to_compile() const { return funcs(); }

    static std::string func_link_name(const IRFuncRef func) { return func->dag.dump(); }
    static bool func_extern(const IRFuncRef) { return false; }
    static bool func_only_local(const IRFuncRef) { return true; }
    static bool func_has_weak_linkage(const IRFuncRef) { return false; }

    bool cur_needs_unwind_info() const { return false; }
    bool cur_is_vararg() const { return false; }
    size_t cur_highest_val_idx() const { return function->dag.getNodesCount(); }

    auto cur_args() const {
        return std::array<IRValueRef, 2>{
            get_rows_count_value_arg(),
            get_columns_ptr_value_arg()
        };
    }

    bool cur_arg_is_byval(const uint32_t) const { return false; }
    uint32_t cur_arg_byval_size(const uint32_t) const { return 0; }
    uint32_t cur_arg_byval_align(const uint32_t) const { return 0; }
    bool cur_arg_is_sret(const uint32_t) const { return false; }

    auto cur_static_allocas() const { return std::views::empty<IRValueRef>; }
    bool cur_has_dynamic_alloca() const { return false; }

    static IRBlockRef cur_entry_block() { return 0; }
    auto cur_blocks() const { return std::ranges::views::iota(IRBlockRef{0}, static_cast<IRBlockRef>(blocks.size())); }
    auto block_succs(const IRBlockRef block) const
    {
        switch (blocks[block].block)
        {
            case EBlock::Entry:
                return std::views::iota(IRBlockRef(1), IRBlockRef(2)); // {1}
            case EBlock::Loop:
                return std::views::iota(IRBlockRef(2), IRBlockRef(3)); // {2}
            default:
                return std::views::iota(IRBlockRef(0), IRBlockRef(0));
        }
    }

    auto block_insts(const IRBlockRef block) const
    {
        std::vector<IRInstRef> r;
        switch (blocks[block].block)
        {
            case EBlock::Entry:
            {
                r.push_back(create_inst(Inst::LoopInit));
                r.push_back(create_inst(Inst::LoopCond));
                break;
            }
            case EBlock::Loop:
            {
                for (IRInstRef idx = 0; idx < function->dag.getNodesCount(); ++idx)
                    if (function->dag[idx].type == DB::CompileDAG::CompileType::FUNCTION)
                        r.push_back(create_inst(Inst::DagNode, idx));
                break;
            }
            case EBlock::Exit:
            {
                r.push_back(create_inst(Inst::LoopExit));
                r.push_back(create_inst(Inst::Return));
                break;
            }
        }
        return r;
    }

    auto block_phis(const IRBlockRef block) const { return std::views::empty<IRInstRef>; }

    uint32_t block_info(const IRBlockRef block) const { return blocks[block].aux.aux1; }
    void block_set_info(const IRBlockRef block, const uint32_t aux) { blocks[block].aux.aux1 = aux; }
    uint32_t block_info2(const IRBlockRef block) const { return blocks[block].aux.aux2; }
    void block_set_info2(const IRBlockRef block, const uint32_t aux) { blocks[block].aux.aux2 = aux; }

    std::string block_fmt_ref(const IRBlockRef block) const
    {
        switch (blocks[block].block)
        {
            case EBlock::Entry: return "Entry";
            case EBlock::Loop: return "Loop";
            case EBlock::Exit: return "Exit";
        }
    }

    tpde::ValLocalIdx val_local_idx(const IRValueRef value) const { return tpde::ValLocalIdx(value); }

    bool val_ignore_in_liveness_analysis(const IRValueRef value) const { return false; }

    bool val_is_phi(IRValueRef value) const { return false; }
    auto val_as_phi(const IRValueRef value) const 
    {
        struct PHIRef
        {
            const CHTPDEAdaptor *self;

            uint32_t incoming_count() const { return 0; }
            IRValueRef incoming_val_for_slot(const uint32_t) const { return 0; }
            IRBlockRef incoming_block_for_slot(const uint32_t) const { return 0; }
            IRValueRef incoming_val_for_block(const IRBlockRef) const { return 0; }
        };
        return PHIRef { .self = this };
    }
    uint32_t val_alloca_size(const IRValueRef value) const { return 0; }
    uint32_t val_alloca_align(const IRValueRef value) const { return 0; }
    std::string value_fmt_ref(const IRValueRef) const { return "val"; }

    auto inst_operands(const IRInstRef inst) const { return function->dag[inst].arguments; }
    auto inst_results(const IRInstRef inst) const { return std::views::single(function->dag[inst].arguments.back()); }
    bool inst_fused(const IRInstRef inst) const { return false; }
    std::string inst_fmt_ref(const IRInstRef inst) const { return "inst" + std::to_string(inst); }

    static void start_compile() {}
    static void end_compile() {}

    bool switch_func(const IRFuncRef function_)
    {
        if (!function_)
            return false;
        if (function_->dag.getNodesCount() == 0)
            return false;
        this->function = function_;
        return true;
    }
    void reset()
    {
        function = nullptr;
    }

private:
    size_t rows_count_value;
    uintptr_t columns_ptr_value;
    IRValueRef get_rows_count_value_arg() const { return reinterpret_cast<IRValueRef>(&rows_count_value); }
    IRValueRef get_columns_ptr_value_arg() const { return reinterpret_cast<IRValueRef>(&columns_ptr_value); }
};

} // namespcae ch_tpde

#endif
