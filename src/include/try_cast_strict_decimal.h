//
// Created by Lucian Voinea on 31/01/2025.
//

#ifndef TRY_CAST_STRICT_DECIMAL_H
#define TRY_CAST_STRICT_DECIMAL_H

#include <duckdb/common/operator/decimal_cast_operators.hpp>

namespace duckdb {

template <class T>
struct StrictDecimalCastData: DecimalCastData<T> {
	// Used to count the number of trailing decimal zeros
	uint8_t trailing_decimal_zeros;
};

struct StrictDecimalCastOperation: DecimalCastOperation {
	/* Specialize the DuckDB provided implementation to count trailing decimal zeros. */
	template <class T, bool NEGATIVE, bool ALLOW_EXPONENT>
	static bool HandleDecimal(T &state, uint8_t digit) {

		// Count trailing decimal zeros
		if (digit == 0) {
			state.trailing_decimal_zeros++;
		} else {
			state.trailing_decimal_zeros = 0;
		}

		return DecimalCastOperation::HandleDecimal<T,NEGATIVE,ALLOW_EXPONENT>(state, digit);
	}
};

template <class DST, char decimal_separator = '.'>
static inline void TryCastStrictToDecimal(Vector &string_vec, Vector &result, const idx_t count, const LogicalType& cast_type) {

	uint8_t width=0;
	uint8_t scale=0;
	// This is available for both INTEGER and DECIMAL types
	cast_type.GetDecimalProperties(width,scale);

	UnaryExecutor::ExecuteWithNulls<string_t, DST>(
		string_vec,result, count,
		[&](string_t input, ValidityMask &mask, idx_t idx) {

			/*Similar to the TryDecimalStringCast implementation in the DuckDB
			 * Ref: duckdb/common/operator/decimal_cast_operators.hpp
			 */
			StrictDecimalCastData<DST> cast_data;
			cast_data.result = 0;
			cast_data.width = width;
			cast_data.scale = scale;
			cast_data.digit_count = 0;
			cast_data.decimal_count = 0;
			cast_data.excessive_decimals = 0;
			cast_data.trailing_decimal_zeros = 0;
			cast_data.exponent_type = ExponentType::NONE;
			cast_data.round_set = false;
			cast_data.should_round = false;
			cast_data.limit = UnsafeNumericCast<DST>(DecimalCastTraits<DST>::POWERS_OF_TEN_CLASS::POWERS_OF_TEN[width]);

			bool has_result = TryIntegerCast<StrictDecimalCastData<DST>, true, true, StrictDecimalCastOperation, false, decimal_separator>(
				input.GetData(), input.GetSize(), cast_data, false);

			if (!has_result || cast_data.excessive_decimals > cast_data.trailing_decimal_zeros) {
				mask.SetInvalid(idx);
				return static_cast<DST>(0);
			}
			return cast_data.result;
		});
}

}
#endif //TRY_CAST_STRICT_DECIMAL_H
