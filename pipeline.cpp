#include "pipeline.h"
#include "utils.h"
#include "deps/simdjson/simdjson.h"
#include <stdint.h>
#include <windows.h>
#include <intrin.h>
#include <random>

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

PipelineResult ExecutePipeline(char* message_buffer, size_t message_buffer_size, std::string_view input_file_path, unsigned int seed)
{
	// Load input file

	simdjson::padded_string json;
	auto json_error = simdjson::padded_string::load(input_file_path).get(json);
	if (json_error) {
		snprintf(message_buffer, message_buffer_size, "Error loading json file: %s", simdjson::error_message(json_error));
		return {(PipelineError)json_error, {}};
	}

	// Parse input file

	simdjson::ondemand::parser parser;
	simdjson::ondemand::document doc;
	json_error = parser.iterate(json).get(doc);
	if (json_error) {
		snprintf(message_buffer, message_buffer_size, "Error parsing json string: %s", simdjson::error_message(json_error));
		return {(PipelineError)json_error, {}};
		// return (PipelineError)json_error;
	}

	// NOTE(Viktor): this could store __m256i's directly but then we would have to deal with masking of non lane-sized arrays (this is good enough for now)
	std::vector<std::vector<int32_t>> int_data;
	for (auto data : doc)
	{
		simdjson::ondemand::array array;
		json_error = data.get_array().get(array);
		if (json_error) {
			snprintf(message_buffer, message_buffer_size, "Error reading json array: %s", simdjson::error_message(json_error));
			return {(PipelineError)json_error, {}};
			// return (PipelineError)json_error;
		}
		size_t array_count;
		json_error = array.count_elements().get(array_count);
		if (json_error) {
			snprintf(message_buffer, message_buffer_size, "Error reading json array count: %s", simdjson::error_message(json_error));
			return {(PipelineError)json_error, {}};
			// return (PipelineError)json_error;
		}
		int_data.emplace_back(array_count);
		auto& int_array = int_data.back();
		size_t i = 0;
		for (auto result : array)
		{
			int64_t value;
			json_error = result.get(value);
			if (json_error) {
				snprintf(message_buffer, message_buffer_size, "Error reading json array element: %s", simdjson::error_message(json_error));
				return {(PipelineError)json_error, {}};
				// return (PipelineError)json_error;
			}
			int_array[i++] = (int32_t)value;
		}
	}

	// Generate random data

	// Note(Viktor): Could be SIMD-ified using a custom PRNG implementation
	std::mt19937 gen(seed);
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
#elif LANE_WIDTH==1
	for (size_t i = 0; i < int_data.size(); i++)
	{
		for (size_t j = 0; j < int_data[i].size(); j++)
		{
			int_data[i][j] += random_data[i][j];
		}
	}
#endif

	// Generate max values

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
			__m256i* int_lane = (__m256i*)&int_data[i][lane_index];
			__m256i a = _mm256_loadu_si256(int_lane);
			max_result_v = _mm256_max_epi32(max_result_v, a);
		}
		int32_t max_result = max_horizontal_epi32(max_result_v);
		for (size_t j = loop_count*lane_width; j < array_count; j++)
		{
			max_result = max(max_result, int_data[i][j]);
		}
		max_data[i] = max_result;
	}
#elif LANE_WIDTH==1
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

	return {PipelineError::SUCCESS, max_data};
}
