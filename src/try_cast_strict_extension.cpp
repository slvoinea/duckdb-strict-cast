#define DUCKDB_EXTENSION_MAIN

#include "try_cast_strict_extension.hpp"
#include "try_cast_strict_integer.h"
#include "try_cast_strict_decimal.h"

#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/main/extension_util.hpp"
#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>
#include <duckdb/planner/expression/bound_function_expression.hpp>

namespace duckdb {

/* Encapsulates information extracted during the parameter binding phase
 * to be made available during the execution phase.
 * This includes the resolved target LogicalType.
 */
struct TryCastStrictBindInfo : public FunctionData {
public:
	TryCastStrictBindInfo(const LogicalType &cast_type, const char decimal_separator): cast_type(cast_type), decimal_separator(decimal_separator) {}
public:
	unique_ptr<FunctionData> Copy() const {
		return make_uniq<TryCastStrictBindInfo>(cast_type, decimal_separator);
	}
	bool Equals(const FunctionData &other) const {
		return cast_type == dynamic_cast<const TryCastStrictBindInfo &>(other).cast_type;
	}
public:
	LogicalType cast_type;
	char decimal_separator;
};

/* Implements the actual scalar function.
 * It uses the core DuckDB implementation to cast to types other than the ones for which a more
 * strict custom implementation is needed, as mentioned below.
 * For INTEGER based and DECIMAL types, it defined custom cast implementations. These try to resuse
 * as much as possible the logic in the core implementation.
 */
inline void TryCastStrictScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
	/* ---  Retrieve or create a cast function.
	 * When the target type is one of the type to be made more strict (i.e., DECIMAL or INTEGER)
	 * we create a new cast function. Else we used the one already available in the core implementation.
	 * Ref:
	 * - common/vector_operations/vector_cast.hpp/TryCast,DefaultTryCast
	 */

	auto &string_vec = args.data[0];
	auto count = args.size();
	auto &func_expr = state.expr.Cast<BoundFunctionExpression>();
	auto &cast_spec = func_expr.bind_info->Cast<TryCastStrictBindInfo>();
	auto &cast_type = cast_spec.cast_type;

	// List of types for which we need a custom cast implementation
	const std::vector<LogicalTypeId> custom_types = {
		LogicalTypeId::TINYINT,
		LogicalTypeId::SMALLINT,
		LogicalTypeId::INTEGER,
		LogicalTypeId::BIGINT,
		LogicalTypeId::DECIMAL,
		LogicalTypeId::UTINYINT,
		LogicalTypeId::USMALLINT,
		LogicalTypeId::UINTEGER,
		LogicalTypeId::UBIGINT
	};

	/* -- Create and use custom implementation for cast
	 *  Idea: casts are implemented as operators
	 *  We need to define such an operator to run with unary operator
	 *  We can reuse the integer try cast witch has to be configured with a different OP implementation
	 * Ref:
	 * - common/operator/decimal_cast_operators.hpp (TryDecimalStringCast)
	 * - common/operator/cast_operators.cpp
	 * - common/operator/cast_operators.hpp
	 */
	if (std::find(custom_types.begin(), custom_types.end(), cast_type.id()) != custom_types.end()) {

		/* Map the LogicalType associated PhysicalType to the appropriate cast function.
		 * To use the unary executor we need to specify the C++ types involved in conversion.
		 * Ref: function/cast/numeric_casts.cpp (BoundCastInfo)
		 */
		PhysicalType pysical_type = cast_type.InternalType();
		if (cast_type.id() == LogicalTypeId::DECIMAL) {
			// Convert to DECIMAL
			if (cast_spec.decimal_separator == '.') {
				switch (pysical_type) {
					case PhysicalType::INT16:
						TryCastStrictToDecimal<int16_t>(string_vec, result, count, cast_type);
					break;
					case PhysicalType::INT32:
						TryCastStrictToDecimal<int32_t>(string_vec, result, count, cast_type);
					break;
					case PhysicalType::INT64:
						TryCastStrictToDecimal<int64_t>(string_vec, result, count, cast_type);
					break;
					case PhysicalType::INT128:
						TryCastStrictToDecimal<hugeint_t>(string_vec, result, count, cast_type);
					break;
					case PhysicalType::INT8:
						// This is not possible
							default:
								throw std::runtime_error("Unsupported DECIMAL type");
				}
			}
			else {
				switch (pysical_type) {
					case PhysicalType::INT16:
						TryCastStrictToDecimal<int16_t,','>(string_vec, result, count, cast_type);
					break;
					case PhysicalType::INT32:
						TryCastStrictToDecimal<int32_t,','>(string_vec, result, count, cast_type);
					break;
					case PhysicalType::INT64:
						TryCastStrictToDecimal<int64_t,','>(string_vec, result, count, cast_type);
					break;
					case PhysicalType::INT128:
						TryCastStrictToDecimal<hugeint_t,','>(string_vec, result, count, cast_type);
					break;
					case PhysicalType::INT8:
						// This is not possible
							default:
								throw std::runtime_error("Unsupported DECIMAL type");
				}
			}
		}
		else { // Convert to INTEGER
			if (cast_spec.decimal_separator == '.') {
				switch (pysical_type) {
					case PhysicalType::INT8:
						TryCastStrictToInteger<int8_t>(string_vec, result, count);
					break;
					case PhysicalType::INT16:
						TryCastStrictToInteger<int16_t>(string_vec, result, count);
					break;
					case PhysicalType::INT32:
						TryCastStrictToInteger<int32_t>(string_vec, result, count);
					break;
					case PhysicalType::INT64:
						TryCastStrictToInteger<int32_t>(string_vec, result, count);
					break;
					case PhysicalType::INT128:
						TryCastStrictToInteger<hugeint_t>(string_vec, result, count);
					break;
					case PhysicalType::UINT8:
						TryCastStrictToInteger<u_int8_t>(string_vec, result, count);
					break;
					case PhysicalType::UINT16:
						TryCastStrictToInteger<u_int16_t>(string_vec, result, count);
					break;
					case PhysicalType::UINT32:
						TryCastStrictToInteger<u_int32_t>(string_vec, result, count);
					break;
					case PhysicalType::UINT64:
						TryCastStrictToInteger<u_int64_t>(string_vec, result, count);
					break;
					case PhysicalType::UINT128:
						TryCastStrictToInteger<uhugeint_t>(string_vec, result, count);
					break;
					default:
						throw std::runtime_error("Unsupported INTEGER type");
				}
			}
			else {
				switch (pysical_type) {
					case PhysicalType::INT8:
						TryCastStrictToInteger<int8_t,','>(string_vec, result, count);
					break;
					case PhysicalType::INT16:
						TryCastStrictToInteger<int16_t,','>(string_vec, result, count);
					break;
					case PhysicalType::INT32:
						TryCastStrictToInteger<int32_t,','>(string_vec, result, count);
					break;
					case PhysicalType::INT64:
						TryCastStrictToInteger<int32_t,','>(string_vec, result, count);
					break;
					case PhysicalType::INT128:
						TryCastStrictToInteger<hugeint_t,','>(string_vec, result, count);
					break;
					case PhysicalType::UINT8:
						TryCastStrictToInteger<u_int8_t,','>(string_vec, result, count);
					break;
					case PhysicalType::UINT16:
						TryCastStrictToInteger<u_int16_t,','>(string_vec, result, count);
					break;
					case PhysicalType::UINT32:
						TryCastStrictToInteger<u_int32_t,','>(string_vec, result, count);
					break;
					case PhysicalType::UINT64:
						TryCastStrictToInteger<u_int64_t,','>(string_vec, result, count);
					break;
					case PhysicalType::UINT128:
						TryCastStrictToInteger<uhugeint_t,','>(string_vec, result, count);
					break;
					default:
						throw std::runtime_error("Unsupported INTEGER type");
				}
			}
		}
	}
	//-- Use core implementation of cast function
	else {
		CastFunctionSet set;
		// TODO: Check whether there is a context at all?
		GetCastFunctionInput input(state.GetContext());
		BoundCastInfo cast_function =
			set.GetCastFunction(LogicalType::VARCHAR, cast_type, input);

		// Create function parameters if needed
		unique_ptr<FunctionLocalState> local_state;
		if (cast_function.init_local_state) {
			CastLocalStateParameters lparameters(input.context, cast_function.cast_data);
			local_state = cast_function.init_local_state(lparameters);
		}
		CastParameters parameters(cast_function.cast_data.get(), false, nullptr, local_state.get(), true);

		cast_function.function(string_vec, result, args.size(), parameters);
	}
}

static unique_ptr<FunctionData> Bind(ClientContext &context, ScalarFunction &bound_function, vector<unique_ptr<Expression>> &arguments) {
	if ((arguments.size() != 2) && (arguments.size() != 3)) {
		throw BinderException("Function 'try_cast_strict' requires two arguments");
	}

	if (!arguments[1]->IsFoldable()) {
		throw BinderException("The 'type' argument should be an accepted type name (e.g., 'INTEGER')");
	}

	char decimal_separator = '.';
	if (arguments.size() == 3) {
		string err_msg = "The 'decimal_separator' argument should be either '.' or ','";
		if (!arguments[2]->IsFoldable()) {
			throw BinderException(err_msg);
		}
		auto arg_serparator = ExpressionExecutor::EvaluateScalar(context, *arguments[2]).GetValue<string>();
		if (arg_serparator.length() != 1) {
			throw BinderException(err_msg);
		}
		decimal_separator = arg_serparator[0];
		if ((decimal_separator != '.') && (decimal_separator != ',')) {
			throw BinderException(err_msg);
		}
	}

	// Get the target LogicalType
	auto value_type = ExpressionExecutor::EvaluateScalar(context, *arguments[1]);
	auto str_value = value_type.GetValue<string>();
	auto target_type = TransformStringToLogicalType(str_value);
	bound_function.return_type = target_type;

	return make_uniq<TryCastStrictBindInfo>(target_type, decimal_separator);
}

static void LoadInternal(DatabaseInstance &instance) {

	// Three parameter variant (value, type, decimal_separator)
	auto try_cast_strict_scalar_function_3p = ScalarFunction(
    	"try_cast_strict_sp",
    	{LogicalType::VARCHAR, LogicalType::VARCHAR, LogicalType::VARCHAR},
    	LogicalType::ANY,
    	TryCastStrictScalarFun,
    	Bind);
    ExtensionUtil::RegisterFunction(instance, try_cast_strict_scalar_function_3p);

	// Two parameter variant (value, type)
	auto try_cast_strict_scalar_function_2p = ScalarFunction(
		"try_cast_strict",
		{LogicalType::VARCHAR, LogicalType::VARCHAR},
		LogicalType::ANY,
		TryCastStrictScalarFun,
		Bind);
	ExtensionUtil::RegisterFunction(instance, try_cast_strict_scalar_function_2p);

}

void TryCastStrictExtension::Load(DuckDB &db) {
	LoadInternal(*db.instance);
}
std::string TryCastStrictExtension::Name() {
	return "try_cast_strict";
}

std::string TryCastStrictExtension::Version() const {
#ifdef EXT_VERSION_TRY_CAST_STRICT
	return EXT_VERSION_TRY_CAST_STRICT;
#else
	return "";
#endif
}

} // namespace duckdb

extern "C" {

DUCKDB_EXTENSION_API void try_cast_strict_init(duckdb::DatabaseInstance &db) {
    duckdb::DuckDB db_wrapper(db);
    db_wrapper.LoadExtension<duckdb::TryCastStrictExtension>();
}

DUCKDB_EXTENSION_API const char *try_cast_strict_version() {
	return duckdb::DuckDB::LibraryVersion();
}
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
