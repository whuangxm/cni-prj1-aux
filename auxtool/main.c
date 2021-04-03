#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef _MAX_PATH
#define _MAX_PATH   260 // max. length of full pathname
#endif // !_MAX_PATH

#define FMT_ENCODED_FILE "e%d.bin"
#define FMT_DECODED_FILE "%d.bin"
#define FMT_VALID_FILE "v%d.bin"
#define COMMAND_TEST "test"
#define COMMAND_MESSAGE "msg"
#define COMMAND_BENCHMARK "bench"
#define COMMAND_OPTION_MARK '-'
#define COMMAND_HELP "-h"
#define COMMAND_SEED "-s"
#define COMMAND_ROUND "-r"
#define COMMAND_FILE_SIZE "-f"
#define RAND_SEED 8000
#define TORRENT_ERR_RATE 0.0003

int generate_message_file(const int id, const size_t size)
{
	char file_name[_MAX_PATH];
	sprintf(file_name, FMT_ENCODED_FILE, id);
	FILE* f = fopen(file_name, "wb");

	for (size_t i = 0; i < size; i++)
	{
		const unsigned char r = rand() & 0xFF;
		fwrite(&r, 1, 1, f);
	}

	fclose(f);

	return 0;
}

int generate_error_file(const int id, const size_t size)
{
	char file_name[_MAX_PATH];
	sprintf(file_name, FMT_VALID_FILE, id);
	FILE* f = fopen(file_name, "wb");

	for (size_t i = 0; i < size; i++)
	{
		const unsigned char r = ((rand() & 0xFF) < 0xF0) ? 0xFF : 0x00;
		fwrite(&r, 1, 1, f);
	}

	fclose(f);

	return 0;
}

int generate_simulated_decoded_file(const int id)
{
	char file_name[_MAX_PATH];
	sprintf(file_name, FMT_DECODED_FILE, id);
	FILE* fd = fopen(file_name, "wb");
	sprintf(file_name, FMT_ENCODED_FILE, id);
	FILE* fe = fopen(file_name, "rb");
	int bit_count = 0;
	fseek(fe, 0, SEEK_END);
	int filesize = ftell(fe);
	fseek(fe, 0, SEEK_SET);
	int err_position = rand() % filesize;

	while (1)
	{
		unsigned char r;
		size_t fread_size = fread(&r, 1, 1, fe);

		if (fread_size < 1)
			break;

		if (bit_count > err_position && (rand() & 0xFF) > 0xF0)
			r ^= (1 << (rand() % 8));

		fwrite(&r, 1, 1, fd);
		++bit_count;
	}
	printf("\n");

	fclose(fd);
	fclose(fe);

	return 0;
}

int compare_files(const int id, int* val_bits, int* all_bits, int* err_bits, int* lost_bits)
{
	char file_name[_MAX_PATH];
	sprintf(file_name, FMT_DECODED_FILE, id);
	FILE* fd = fopen(file_name, "rb");
	sprintf(file_name, FMT_ENCODED_FILE, id);
	FILE* fe = fopen(file_name, "rb");
	sprintf(file_name, FMT_VALID_FILE, id);
	FILE* fv = fopen(file_name, "rb");
	*val_bits = 0;
	*all_bits = 0;
	*lost_bits = 0;
	*err_bits = 0;

	if (!fd || !fe || !fv)
	{
		fprintf(stderr, "Error: File not found.\n");
		return 1;
	}
	fseek(fd, 0, SEEK_END);
	const int decoded_bit_length = ftell(fd) * 8;
	fseek(fd, 0, SEEK_SET);

	int found_error = 0;
	const int decoded_max_error_bit_length = (int)floor(decoded_bit_length * TORRENT_ERR_RATE);
	int count = 0;

	while (1)
	{
		unsigned char r, rd, rv;
		size_t fread_size = 0;
		fread_size += fread(&r, 1, 1, fe);
		fread_size += fread(&rd, 1, 1, fd);
		fread_size += fread(&rv, 1, 1, fv);

		if (fread_size < 3)
			break;

		for (size_t i = 0; i < 8; i++)
		{
			unsigned char b, bd, bv;
			b = r & 1;
			bd = rd & 1;
			bv = rv & 1;
			(*all_bits)++;
			if (b != bd)
			{
				(*err_bits)++;
				found_error += (bv == 1);
			}
			if (bv == 0)
				(*lost_bits)++;
			else
				*val_bits += (found_error <= decoded_max_error_bit_length);
			r >>= 1;
			rd >>= 1;
			rv >>= 1;
			count++;
		}
	}

	fclose(fd);
	fclose(fe);
	fclose(fv);

	return 0;
}

void print_title()
{
	printf("%s\n", "Auxiliary Tool of Project 1 (Version 1.1)");
	printf("%s\n\n", "Author: Dr. Wei HUANG, School of Informatics, Xiamen University");
}

void usage(const char* file_name)
{
	printf("%s: %s %s\n", "Usage", file_name, " [opts] [mode]");// [count] [fsize]
	printf("    * %-8s: %s\n", "opts", "Options.");
	printf("        * %-16s: %s\n", COMMAND_SEED " [seed]", "An integer. Set the seed of the pseudorandom number generator. ");
	printf("        * %-16s: %s\n", COMMAND_ROUND " [round]", "An integer. Set the number of repetitions. ");
	printf("        * %-16s: %s\n", COMMAND_FILE_SIZE " [size]", "An integer. Set the message file size, in bytes. ");
	printf("        * %-16s: %s\n", COMMAND_HELP , "Show this help message. ");
	printf("    * %-8s: %s\n", "mode", "Modes.");
	printf("        * %-16s: %s\n", COMMAND_TEST, "It generates test files and simulates decoding process.");
	printf("        * %-16s: %s\n", COMMAND_MESSAGE, "It generates message files only.");
	printf("        * %-16s: %s\n", COMMAND_BENCHMARK, "It compares files and calculates effective bits and other indices.");
}

void usage_and_exit(const char* file_name)
{
	fprintf(stderr, "Command error.");
	usage(file_name);
	exit(1);
}

void opt_test(const unsigned int rand_seed, const unsigned int test_count, const unsigned int file_size)
{
	srand(rand_seed);
	printf("%s\n", "Starting testing ...");
	for (unsigned int i = 0; i < test_count; i++)
	{
		printf("%s %u / %u %s ", "Process", i + 1, test_count, "...");
		generate_message_file(i + 1, file_size);
		generate_error_file(i + 1, file_size);
		generate_simulated_decoded_file(i + 1);
		printf("%s\n", "Finished.");
	}
	printf("%s\n", "All are finished.");
}

void opt_message(const unsigned int rand_seed, const unsigned int test_count, const unsigned int file_size)
{
	srand(rand_seed);
	printf("%s\n", "Starting generating messages ...");
	for (unsigned int i = 0; i < test_count; i++)
	{
		printf("%s %u / %u %s ", "Process", i + 1, test_count, "...");
		generate_message_file(i + 1, file_size);
		printf("%s\n", "All are finished.");
	}
}

void opt_benchmark(const unsigned int rand_seed, const unsigned int test_count, const unsigned int file_size) {
	srand(rand_seed);
	double avg_val_bits = 0, avg_all_bits = 0, avg_err_bits = 0, avg_lost_bits = 0;
	int val_bits = 0, all_bits = 0, err_bits = 0, lost_bits = 0;
	printf("%s\n", "Starting benchmarking ...");
	printf("%-5s\t%-10s\t%-12s\t%-10s\t%-10s\n", "ID", "Val. (b)", "All (b)", "Err. (%)", "Lost (%)");
	printf("%-5s\t%-10s\t%-12s\t%-10s\t%-10s\n", "=====", "==========", "============", "==========", "==========");
	for (unsigned int i = 0; i < test_count; i++)
	{
		if (compare_files(i + 1, &val_bits, &all_bits, &err_bits, &lost_bits) == 0)
		{
			printf("%5d\t%10.1lf\t%12.1lf\t%10.2lf\t%10.2lf\n", i + 1, (double)val_bits, (double)all_bits, err_bits * 100. / all_bits, lost_bits * 100. / all_bits);
			avg_val_bits += val_bits;
			avg_all_bits += all_bits;
			avg_err_bits += err_bits;
			avg_lost_bits += lost_bits;
		}
	}
	printf("%-5s\t%10.1lf\t%12.1lf\t%10.2lf\t%10.2lf\n", "Avg.", avg_val_bits / test_count, avg_all_bits / test_count, avg_err_bits * 100. / avg_all_bits, avg_lost_bits * 100. / avg_all_bits);
}

int main(int argc, const char** argv)
{
	const char* command = COMMAND_BENCHMARK;
	const unsigned int DEFAULT_TEST_COUNT = 5;
	const unsigned int DEFAULT_FILE_SIZE = 3 * 1024 * 1024;

	unsigned int test_count = DEFAULT_TEST_COUNT;
	unsigned int file_size = DEFAULT_FILE_SIZE;
	unsigned int rand_seed = RAND_SEED;

	print_title();

	int argid = 0;

	if (argc==1)
		usage_and_exit(argv[0]);

	while (++argid < argc)
	{
		command = argv[argid];
		if (command[0] == COMMAND_OPTION_MARK) {
			if (strcmp(command, COMMAND_HELP) == 0) {
				usage(argv[0]);
				exit(0);
			}
			else if (strcmp(command, COMMAND_SEED) == 0) {
				if (++argid >= argc || sscanf(argv[argid], "%u", &rand_seed) != 1)
					usage_and_exit(argv[0]);
			}
			else if (strcmp(command, COMMAND_ROUND) == 0) {
				if (++argid >= argc || sscanf(argv[argid], "%u", &test_count) != 1)
					usage_and_exit(argv[0]);
			}
			else if (strcmp(command, COMMAND_FILE_SIZE) == 0) {
				if (++argid >= argc || sscanf(argv[argid], "%u", &file_size) != 1)
					usage_and_exit(argv[0]);
			}
		}
		else
		{
			if (strcmp(command, COMMAND_TEST) == 0)
			{
				opt_test(rand_seed, test_count, file_size);
			}
			else if (strcmp(command, COMMAND_MESSAGE) == 0)
			{
				opt_message(rand_seed, test_count, file_size);
			}
			else if (strcmp(command, COMMAND_BENCHMARK) == 0)
			{
				opt_benchmark(rand_seed, test_count, file_size);
			}
			else
				usage_and_exit(argv[0]);
		}
	}

	return 0;
}
