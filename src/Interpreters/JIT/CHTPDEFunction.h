#pragma once

#include "config.h"

#if USE_EMBEDDED_COMPILER && USE_TPDE_BACKEND

#include <memory>
#include <vector>

#include <Functions/IFunction.h>
#include <Interpreters/JIT/CompileDAG.h>
#include <Interpreters/ActionsDAG.h>

#include <base/sort.h>
#include <Columns/ColumnConst.h>
#include <Common/typeid_cast.h>
#include <DataTypes/DataTypesNumber.h>
#include <DataTypes/Native.h>

namespace ch_tpde
{
    struct CHTPDEAdaptor;
}

namespace DB
{

class CompiledFunctionHolder;

class CHTPDEFunction : public IFunctionBase
{
    friend struct ch_tpde::CHTPDEAdaptor;

public:
    explicit CHTPDEFunction(const CompileDAG & dag_);

    ~CHTPDEFunction() override;

    void setCompiledFunction(std::shared_ptr<CompiledFunctionHolder> compiled_function_holder_);

    bool isCompilable() const override { return true; }

    bool isSuitableForShortCircuitArgumentsExecution(const DataTypesWithConstInfo & arguments) const override;

    String getName() const override { return name; }

    const DataTypes & getArgumentTypes() const override { return argument_types; }
    
    const DataTypePtr & getResultType() const override { return dag.back().result_type; }

    ExecutableFunctionPtr prepare(const ColumnsWithTypeAndName &) const override;

    bool isDeterministic() const override;

    bool isDeterministicInScopeOfQuery() const override;

    bool isSuitableForConstantFolding() const override;

    bool isInjective(const ColumnsWithTypeAndName & sample_block) const override;

    bool hasInformationAboutMonotonicity() const override;

    IFunctionBase::Monotonicity getMonotonicityForRange(const IDataType & type, const Field & left, const Field & right) const override;

private:
    std::string name;
    CompileDAG dag;
    DataTypes argument_types;
    std::vector<FunctionBasePtr> nested_functions;
    std::shared_ptr<CompiledFunctionHolder> compiled_function_holder;    
};
    
} // namespace DB

#endif
