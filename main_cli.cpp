#include "pipeline.h"

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		fprintf(stderr, "Missing input file\nUsage: %s <input file> > <output file>\n", argv[0]);
		return ERROR_INVALID_PARAMETER;
	}

	std::string_view input_file_path{argv[1]};
	unsigned int random_seed = 0x5702135;
	char message_buffer[MAX_MSG_SIZE] = {};
	auto [ error, data ] = ExecutePipeline(message_buffer, sizeof(message_buffer), {argv[1]}, random_seed);
	if (error != PipelineError::SUCCESS)
	{
		fprintf(stderr, message_buffer);
		return (int)error;
	}

	for (auto value : data)
	{
		fprintf(stdout, "%d\n", value);
	}

	return (int)PipelineError::SUCCESS;
}