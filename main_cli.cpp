#include <random>
#include <stdint.h>
#include <windows.h>
#include <intrin.h>
#include "utils.h"
#include "deps/simdjson/simdjson.h"

#if !defined(LANE_WIDTH)
#define LANE_WIDTH 8
#endif

// int max_horizontal_epi32(__m128i x) {
// 	__m128i max1 = _mm_shuffle_epi32(x, _MM_SHUFFLE(0,0,3,2));
// 	__m128i max2 = _mm_max_epi32(x,max1);
// 	__m128i max3 = _mm_shuffle_epi32(max2, _MM_SHUFFLE(0,0,0,1));
// 	__m128i max4 = _mm_max_epi32(max2,max3);
// 	return _mm_cvtsi128_si32(max4);
// }

int32_t max_horizontal_epi32(__m256i vec) {
    __declspec(align(32)) int32_t result[8] = {INT32_MIN};
    _mm256_store_si256((__m256i*) result, vec);
    auto a = max(result[0], result[1]);
    auto b = max(result[2], result[3]);
    auto c = max(result[4], result[5]);
    auto d = max(result[6], result[7]);
    auto x = max(a, b);
    auto y = max(c, d);
    return max(x, y);
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		eprintf("Missing input file\nUsage: %s <input file> > <output file>\n", argv[0]);
		return ERROR_INVALID_PARAMETER;
	}

	std::string_view input_file_path{argv[1]};
	simdjson::padded_string json;
	auto error = simdjson::padded_string::load(input_file_path).get(json);
	if (error) {
		eprintf("Error loading json file\n%s", simdjson::error_message(error));
		return error;
	}
	simdjson::ondemand::parser parser;
	simdjson::ondemand::document doc;
	error = parser.iterate(json).get(doc);
	if (error) {
		eprintf("Error parsing json string\n%s", simdjson::error_message(error));
		return error;
	}

	// Note(Viktor): this could store __m256i's directly but then we would have to deal with masking of non lane-sized arrays (this is good enough)
	std::vector<std::vector<int32_t>> int_data;
	for (auto data : doc)
	{
		simdjson::ondemand::array array;
		error = data.get_array().get(array);
		if (error) {
			eprintf("Error reading json array\n%s", simdjson::error_message(error));
			return error;
		}
		size_t array_count;
		error = array.count_elements().get(array_count);
		int_data.emplace_back(array_count);
		auto& int_array = int_data.back();
		size_t i = 0;
		for (auto result : array)
		{
			int64_t value;
			error = result.get(value);
			if (error) {
				// snprintf(buf, buf_size, "Error reading array element\n%s", simdjson::error_message(error));
				eprintf("Error reading array element\n%s", simdjson::error_message(error));
				return error;
			}
			int_array[i++] = (int32_t)value;
		}
	}

	// NOTE(Viktor): Could be SIMD-ified using a custom PRNG implementation
	std::mt19937 gen(0x5702135);
	std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
	std::vector<std::vector<int32_t>> random_data(int_data.size());

	for (size_t i = 0; i < int_data.size(); i++)
	{
		random_data[i].resize(int_data[i].size());
		for (size_t j = 0; j < random_data[i].size(); j++)
		{
			random_data[i][j] = distrib(gen);
		}
	}

#if LANE_WIDTH==8
	for (size_t i = 0; i < int_data.size(); i++)
	{
		size_t lane_width = LANE_WIDTH;
		size_t array_count = int_data[i].size();
		size_t loop_count = array_count / lane_width;
		for (size_t j = 0; j < loop_count; j++)
		{
			size_t lane_index = j * lane_width;
			__m256i a = _mm256_loadu_si256((__m256i*)&int_data[i][lane_index]);
			__m256i b = _mm256_loadu_si256((__m256i*)&random_data[i][lane_index]);
			__m256i c = _mm256_add_epi32(a, b);
			_mm256_storeu_si256((__m256i*)&int_data[i][lane_index], c);
		}
		for (size_t j = loop_count*lane_width; j < array_count; j++)
		{
			int_data[i][j] += random_data[i][j];
		}
	}
#else
	for (size_t i = 0; i < int_data.size(); i++)
	{
		for (size_t j = 0; j < int_data[i].size(); j++)
		{
			int_data[i][j] += random_data[i][j];
		}
	}
#endif

	std::vector<int32_t> max_data(int_data.size());

#if LANE_WIDTH==8
	for (size_t i = 0; i < int_data.size(); i++)
	{
		size_t lane_width = LANE_WIDTH;
		size_t array_count = int_data[i].size();
		size_t loop_count = array_count / lane_width;

		__m256i max_result_v = _mm256_set1_epi32(INT32_MIN);
		for (size_t j = 0; j < loop_count; j++)
		{
			size_t lane_index = j * lane_width;
			__m256i a = _mm256_loadu_si256((__m256i*)&int_data[i][lane_index]);
			max_result_v = _mm256_max_epi32(max_result_v, a);
		}
		int32_t max_result = max_horizontal_epi32(max_result_v);
		for (size_t j = loop_count*lane_width; j < array_count; j++)
		{
			max_result = max(max_result, int_data[i][j]);
		}
		max_data[i] = max_result;
	}
#else
	for (size_t i = 0; i < int_data.size(); i++)
	{
		int32_t max_result = INT32_MIN;
		for (size_t j = 0; j < int_data[i].size(); j++)
		{
			max_result = max(max_result, int_data[i][j]);
		}
		max_data[i] = max_result;
	}
#endif

	// ExecutePipeline();

	for (auto value : max_data)
	{
		printf("%d\n", value);
	}

	return ERROR_SUCCESS;
}