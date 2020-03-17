#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FMT_ENCODED_FILE "e%d.bin"
#define FMT_DECODED_FILE "%d.bin"
#define FMT_VALID_FILE "v%d.bin"
#define COMMAND_TEST "test"
#define COMMAND_MESSAGE "msg"
#define COMMAND_BENCHMARK "bench"
#define COMMAND_HELP "/?"
#define RAND_SEED 8000

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
    while (!feof(fe))
    {
        unsigned char r;
        fread(&r, 1, 1, fe);
        if (bit_count > err_position && (rand() & 0xFF) > 0xF0)
            r ^= (1 << (rand() % 8));
        fwrite(&r, 1, 1, fd);
        ++bit_count;
    }

    fclose(fd);
    fclose(fe);

    return 0;
}
int compare_files(const int id, int* eff_bits, int* all_bits, int* err_bits, int* lost_bits)
{
    char file_name[_MAX_PATH];
    sprintf(file_name, FMT_DECODED_FILE, id);
    FILE* fd = fopen(file_name, "rb");
    sprintf(file_name, FMT_ENCODED_FILE, id);
    FILE* fe = fopen(file_name, "rb");
    sprintf(file_name, FMT_VALID_FILE, id);
    FILE* fv = fopen(file_name, "rb");
    *eff_bits = 0;
    *all_bits = 0;
    *lost_bits = 0;
    *err_bits = 0;
    while (!feof(fe) && !feof(fd) && !feof(fv))
    {
        unsigned char r, rd, rv;
        fread(&r, 1, 1, fe);
        fread(&rd, 1, 1, fd);
        fread(&rv, 1, 1, fv);
        for (size_t i = 0; i < 8; i++)
        {
            unsigned char b, bd, bv;
            b = r & 1;
            bd = rd & 1;
            bv = rv & 1;
            (*all_bits)++;
            *err_bits += (b != bd);
            if (bv == 0)
                (*lost_bits)++;
            else
                *eff_bits += (!*err_bits);
            r >>= 1;
            rd >>= 1;
            rv >>= 1;
        }
    }

    fclose(fd);
    fclose(fe);
    fclose(fv);

    return 0;
}

void print_title()
{
    printf("%s\n", "Auxiliary Tool of Project 1 (Version 1.0)");
    printf("%s\n\n", "Author: Dr. Wei HUANG, School of Informatics, Xiamen University");
}

void usage(const char* file_name)
{
    printf("%s: %s %s\n", "Usage", file_name, "[opt] [count] [fsize]");
    printf("    * %-8s: %s\n", "opt", "Options.");
    printf("        - %-8s: %s\n", "test", "By default. It generates test files and simulates decoding process.");
    printf("        - %-8s: %s\n", "msg", "It generates message files only.");
    printf("        - %-8s: %s\n", "bench", "Benchmarking. It compares files and calculates effective bits and other indices.");
    printf("    * %-8s: %s\n", "count", "Number of repetitions.");
    printf("    * %-8s: %s\n", "opt", "Message file size.");
}

int main(int argc, const char** argv)
{
    const char* command = COMMAND_TEST;
    const int DEFAULT_TEST_COUNT = 5;
    const unsigned int DEFAULT_FILE_SIZE = 1024 * 1024;

    int test_count = DEFAULT_TEST_COUNT;
    unsigned int file_size = DEFAULT_FILE_SIZE;

    srand(RAND_SEED);

    print_title();

    int argid = 0;
    if (argc > ++argid)
        command = argv[argid];
    if (argc > ++argid)
    {
        if (sscanf(argv[argid], "%d", &test_count) != 1)
            test_count = DEFAULT_TEST_COUNT;
    }
    if (argc > ++argid)
    {
        if (sscanf(argv[argid], "%u", &file_size) != 1)
            file_size = DEFAULT_FILE_SIZE;
    }

    if (strcmp(command, COMMAND_HELP) == 0) {
        usage(argv[0]);
        return 1;
    }
    else if (strcmp(command, COMMAND_TEST) == 0)
    {
        printf("%s\n", "Starting testing ...");
        for (int i = 0; i < test_count; i++)
        {
            printf("%s %d / %d %s ", "Process", i + 1, test_count, "...");
            generate_message_file(i + 1, file_size);
            generate_error_file(i + 1, file_size);
            generate_simulated_decoded_file(i + 1);
            printf("%s\n", "Finished.");
        }
        printf("%s\n", "All are finished.");
    }
    else if (strcmp(command, COMMAND_MESSAGE) == 0)
    {
        printf("%s\n", "Starting generating messages ...");
        for (int i = 0; i < test_count; i++)
        {
            printf("%s %d / %d %s ", "Process", i + 1, test_count, "...");
            generate_message_file(i + 1, file_size);
            printf("%s\n", "All are finished.");
        }
    }
    else if (strcmp(command, COMMAND_BENCHMARK) == 0)
    {
        double avg_eff_bits = 0, avg_all_bits = 0, avg_err_bits = 0, avg_lost_bits = 0;
        int eff_bits = 0, all_bits = 0, err_bits = 0, lost_bits = 0;
        printf("%s\n", "Starting benchmarking ...");
        printf("%-5s\t%-10s\t%-10s\t%-10s\t%-10s\n", "ID", "Eff. (b)", "All (b)", "Err. (%)", "Lost (%)");
        printf("%-5s\t%-10s\t%-10s\t%-10s\t%-10s\n", "=====", "==========", "=========", "==========", "==========");
        for (int i = 0; i < test_count; i++)
        {
            compare_files(i + 1, &eff_bits, &all_bits, &err_bits, &lost_bits);
            printf("%5d\t%10.1lf\t%10.1lf\t%10.2lf\t%10.2lf\n", i + 1, (double)eff_bits, (double)all_bits, err_bits * 100. / all_bits, lost_bits * 100. / all_bits);
            avg_eff_bits += eff_bits;
            avg_all_bits += all_bits;
            avg_err_bits += err_bits;
            avg_lost_bits += lost_bits;
        }
        printf("%-5s\t%10.1lf\t%10.1lf\t%10.2lf\t%10.2lf\n", "Avg.", avg_eff_bits / test_count, avg_all_bits / test_count, avg_err_bits * 100. / avg_all_bits, avg_lost_bits * 100. / avg_all_bits);
    }

    return 0;
}
