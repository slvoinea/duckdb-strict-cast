//
// Created by Lucian Voinea on 31/01/2025.
//

#ifndef TRY_CAST_STRICT_INTEGER_H
#define TRY_CAST_STRICT_INTEGER_H

#include <duckdb/common/operator/integer_cast_operator.hpp>

namespace duckdb {

struct StrictIntegerCastOperation: IntegerCastOperation {
    /* Specializes the DuckDB provided implementation by stopping as soon as non-zero decimals are detected. */
    template <class T, bool NEGATIVE, bool ALLOW_EXPONENT>
    static bool HandleDecimal(T &state, uint8_t digit) {
        return (digit == 0);
    }
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
            return static_cast<DST>(cast_data.result);
        });
}

}
#endif //TRY_CAST_STRICT_INTEGER_H
