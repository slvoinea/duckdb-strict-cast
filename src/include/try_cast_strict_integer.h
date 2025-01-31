//
// Created by Lucian Voinea on 31/01/2025.
//

#ifndef TRY_CAST_STRICT_INTEGER_H
#define TRY_CAST_STRICT_INTEGER_H

#include <duckdb/common/operator/integer_cast_operator.hpp>

namespace duckdb {

struct StrictIntegerCastOperation: IntegerCastOperation {
    /* This should specialize the DuckDB provided implementation if needed.
     */
};

template <class DST, char decimal_separator = '.'>
static inline void TryCastStrictToInteger(Vector &string_vec, Vector &result, idx_t count) {
    UnaryExecutor::ExecuteWithNulls<string_t, DST>(
        string_vec,result, count,
        [](string_t input, ValidityMask &mask, idx_t idx) {

            /*Similar to the TrySimpleIntegerCast implementation in the DuckDB
             * Ref: duckdb/common/operator/integer_cast_operator.hpp
             */
            IntegerCastData<DST> cast_data;
            bool has_result = TryIntegerCast<IntegerCastData<DST>, true, true, StrictIntegerCastOperation, true, decimal_separator>(
                input.GetData(), input.GetSize(), cast_data, false);

            if (!has_result) {
                mask.SetInvalid(idx);
                return static_cast<DST>(0);
            }
            return cast_data.result;
        });
}

}
#endif //TRY_CAST_STRICT_INTEGER_H
