#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "loop.h"

// Threshold in decibels
#define DEFAULT_THRESHOLD -6.0

// Add -EXTENDED to file
// i.e. input.wav becomes input-EXTENDED.wav
void add_str(char *str, char *new_str, const char *replacement);

// Convert mm:ss to seconds
float get_sec(char *time);

// Get value in [min, max]
int constrain(int value, int min, int max);

int main(int argc, char *argv[])
{
    // Check command-line arguments
    if (argc != 5 && argc != 6)
    {
        printf("Usage: ./loop input_file new_length begin_loop end_loop [threshold_value]\n");
        printf("input_file: name of audio file to loop, with header\n");
        printf("new_length: integer representing new audio file length, in minutes\n");
        printf("begin_loop: number representing when to start loop. The more precise, the better.\n");
        printf("end_loop: number representing when to end loop. The more precise, the better.\n");
        printf("NOTE: for begin_loop and end_loop, put the time BEFORE the actual loop.\n");
        printf("(optional) threshold_value: a negative double, representing the decibels at a certain point. Default value is %f.\n",
               DEFAULT_THRESHOLD);
        return 1;
    }

    // Check if the input has a filename extension
    // i.e. input.mp3, input.wav, input.c
    char *file_type = strrchr(argv[1], '.');
    if (file_type == NULL)
    {
        printf("Please include the filename extension.\n");
        return 1;
    }

    // Check if the file is a WAVE type
    if (strcmp(file_type, ".wav") != 0)
    {
        // Warn user about audio file
        printf("Only .wav files are supported. Continue? [Y/N]\n");
        char ans = 'N';
        scanf("%c", &ans);
        if (ans != 'Y' && ans != 'y')
        {
            return 1;
        }
    }

    // Open file, read as binary
    FILE *input = fopen(argv[1], "rb");
    if (input == NULL)
    {
        printf("Could not open file: %s\n", argv[1]);
        return 1;
    }

    // Allocate data for WAVE header
    wav_header *header = malloc(sizeof(*header));
    if (header == NULL)
    {
        printf("Could not allocate memory for header data\n");
        fclose(input);
        return 1;
    }

    // Get information about the file
    fread(header->chunk_id, 4, 1, input);
    fread(&header->chunk_size, 4, 1, input);
    fread(header->format, 4, 1, input);
    fread(header->subchunk1_id, 4, 1, input);
    fread(&header->subchunk1_size, 4, 1, input);
    fread(&header->audio_format, 2, 1, input);
    fread(&header->num_channels, 2, 1, input);
    fread(&header->sample_rate, 4, 1, input);
    fread(&header->byte_rate, 4, 1, input);
    fread(&header->block_align, 2, 1, input);
    fread(&header->bits_per_sample, 2, 1, input);
    fread(header->subchunk2_id, 4, 1, input);
    fread(&header->subchunk2_size, 4, 1, input);

    // Print warnings about unusual header values
    int warnings = 0;
    if (!(header->chunk_id[0] == 'R' && header->chunk_id[1] == 'I'
          && header->chunk_id[2] == 'F' && header->chunk_id[3] == 'F'))
    {
        printf("WARNING: ChunkID is not 'RIFF'\n");
        warnings++;
    }
    if (!(header->format[0] == 'W' && header->format[1] == 'A'
          && header->format[2] == 'V' && header->format[3] == 'E'))
    {
        printf("WARNING: Format is not 'WAVE'\n");
        warnings++;
    }
    if (!(header->subchunk1_id[0] == 'f' && header->subchunk1_id[1] == 'm'
          && header->subchunk1_id[2] == 't' && header->subchunk1_id[3] == ' '))
    {
        printf("WARNING: SubChunk1ID is not 'fmt '\n");
        warnings++;
    }
    if (!(header->subchunk2_id[0] == 'd' && header->subchunk2_id[1] == 'a' &&
          header->subchunk2_id[2] == 't' && header->subchunk2_id[3] == 'a'))
    {
        printf("WARNING: SubChunk2ID is not 'data'\n");
        warnings++;
    }
    if (!(header->bits_per_sample == 8 || header->bits_per_sample == 16 || header->bits_per_sample == 32))
    {
        printf("WARNING: BitsPerSample is not 8, 16, or 32\n");
        warnings++;
    }
    if (warnings > 0)
    {
        printf("\n");
    }

    // Print header values
    printf("ChunkID (1-4): %s\n", header->chunk_id);
    printf("ChunkSize (5-8): %i\n", header->chunk_size);
    printf("Format (9-12): %s\n", header->format);
    printf("SubChunk1ID (13-16): %s\n", header->subchunk1_id);
    printf("SubChunk1Size (17-20): %i\n", header->subchunk1_size);
    printf("AudioFormat (21-22): %d\n", header->audio_format);
    printf("NumChannels (23-24): %d\n", header->num_channels);
    printf("SampleRate (25-28): %i\n", header->sample_rate);
    printf("ByteRate (29-32): %i\n", header->byte_rate);
    printf("BlockAlign (33-34): %d\n", header->block_align);
    printf("BitsPerSample (35-36): %d\n", header->bits_per_sample);
    printf("SubChunk2ID (37-40): %s\n", header->subchunk2_id);
    printf("SubChunk2Size (41-44): %i\n\n", header->subchunk2_size);

    // Read samples in 8-bit, 16-bit, 32-bit, or 64-bit
    int8_t *one_byte_data = NULL;
    int16_t *two_byte_data = NULL;
    int32_t *four_byte_data = NULL;

    const int BYTES_PER_SAMPLE = header->bits_per_sample / 8;
    switch (BYTES_PER_SAMPLE)
    {
        case 1:
            // Read samples in 8-bit
            one_byte_data = malloc(header->chunk_size - 36);
            if (one_byte_data == NULL)
            {
                printf("Could not allocate memory for data.\n");
                free(header);
                fclose(input);
                return 1;
            }
            for (int i = 0; fread(&one_byte_data[i], BYTES_PER_SAMPLE, 1, input); i++);
            break;
        case 2:
            // Read samples in 16-bit
            two_byte_data = malloc(header->chunk_size - 36);
            if (two_byte_data == NULL)
            {
                printf("Could not allocate memory for data.\n");
                free(header);
                fclose(input);
                return 1;
            }
            for (int i = 0; fread(&two_byte_data[i], BYTES_PER_SAMPLE, 1, input); i++);
            break;
        case 4:
            // Read samples in 32-bit
            four_byte_data = malloc(header->chunk_size - 36);
            if (four_byte_data == NULL)
            {
                printf("Could not allocate memory for data.\n");
                free(header);
                fclose(input);
                return 1;
            }
            for (int i = 0; fread(&four_byte_data[i], BYTES_PER_SAMPLE, 1, input); i++);
            break;
    }

    // Setup for determining looping point
    // Start and end variables as the nth samples instead of time
    int list_size = (header->chunk_size - 36) / BYTES_PER_SAMPLE;
    int start = constrain(get_sec(argv[3]) * header->byte_rate / BYTES_PER_SAMPLE, 0, list_size / 2);
    int end = constrain(get_sec(argv[4]) * header->byte_rate / BYTES_PER_SAMPLE, list_size / 2, list_size);

    // Determine looping point by calculating decibels
    // Set looping point to a time when change in volume is large
    float dB;
    float threshold = (argc == 6) ? atof(argv[5]) : DEFAULT_THRESHOLD;
    bool found_loop = false;

    // For stereo (2 channels), the left can be different from the right, so skip a channel
    for (int i = start; i < list_size && !found_loop; i += header->num_channels)
    {
        switch (BYTES_PER_SAMPLE)
        {
            // Algorithm for calculating decibels
            case 1:
                dB = 20 * log(abs(one_byte_data[i]) / 128.0);
                break;
            case 2:
                dB = 20 * log(abs(two_byte_data[i]) / 32768.0);
                break;
            case 4:
                dB = 20 * log(abs(four_byte_data[i]) / 2147483648.0);
                break;
        }

        // When there is a sudden loud noise
        // Condition is false if dB is undefined (buffer = 0)
        if (dB >= threshold)
        {
            // Set starting point for loop
            if (i < list_size / 2)
            {
                start = i;
                i = end;
            }
            // Set ending point for loop
            else
            {
                end = i;
                found_loop = true;
            }
        }
    }

    if (found_loop)
    {
        // Get starting and ending times
        float start_sec = (float) start / (header->byte_rate / BYTES_PER_SAMPLE);
        float end_sec = (float) end / (header->byte_rate / BYTES_PER_SAMPLE);
        int start_min = start_sec / 60;
        int end_min = end_sec / 60;
        while (start_sec >= 60)
        {
            start_sec -= 60;
        }
        while (end_sec >= 60)
        {
            end_sec -= 60;
        }

        // Print looping point
        // Warn user about looping point being incorrect
        printf("Found and updated looping point!\n");
        printf("If the output doesn't sound right, try adjusting the threshold value and/or section to loop.\n");
        printf("Starting loop at %i:%09f\n", start_min, start_sec);
        printf("Ending loop at %i:%09f\n", end_min, end_sec);
    }
    else
    {
        printf("Unable to find exact looping point.\n");
        printf("Try decreasing the threshold value.\n");
    }

    // Determine data size for the outputted file
    // Determine number of loops for the outputted file
    const int BUFFERS_PER_SEC = header->byte_rate / BYTES_PER_SAMPLE;
    const int MINUTES = atoi(argv[2]);
    int buffers = start + list_size - end;
    int loops = 0;

    while (buffers < BUFFERS_PER_SEC * 60 * MINUTES)
    {
        buffers += end - start;
        loops++;
    }

    // Change parts of header to allow more data to be written
    header->subchunk2_size = buffers * BYTES_PER_SAMPLE;
    header->chunk_size = header->subchunk2_size + 36;

    // Add -EXTENDED to file
    // i.e. input.wav becomes input-EXTENDED.wav
    char extended_file[strlen(argv[1]) + strlen("-EXTENDED")];
    memset(extended_file, 0, sizeof(extended_file));
    add_str(argv[1], extended_file, "-EXTENDED");

    // Open file, write as binary
    FILE *output = fopen(extended_file, "wb");
    if (output == NULL)
    {
        printf("Could not open file: %s\n", extended_file);
        free(header);
        switch (BYTES_PER_SAMPLE)
        {
            case 1:
                free(one_byte_data);
                break;
            case 2:
                free(two_byte_data);
                break;
            case 4:
                free(four_byte_data);
                break;
        }
        fclose(input);
        return 1;
    }

    // Write header to outputted file
    fwrite(header->chunk_id, 4, 1, output);
    fwrite(&header->chunk_size, 4, 1, output);
    fwrite(header->format, 4, 1, output);
    fwrite(header->subchunk1_id, 4, 1, output);
    fwrite(&header->subchunk1_size, 4, 1, output);
    fwrite(&header->audio_format, 2, 1, output);
    fwrite(&header->num_channels, 2, 1, output);
    fwrite(&header->sample_rate, 4, 1, output);
    fwrite(&header->byte_rate, 4, 1, output);
    fwrite(&header->block_align, 2, 1, output);
    fwrite(&header->bits_per_sample, 2, 1, output);
    fwrite(header->subchunk2_id, 4, 1, output);
    fwrite(&header->subchunk2_size, 4, 1, output);

    // Write data
    // Write the beginning
    for (int i = 0; i < start; i++)
    {
        switch (BYTES_PER_SAMPLE)
        {
            case 1:
                fwrite(&one_byte_data[i], BYTES_PER_SAMPLE, 1, output);
                break;
            case 2:
                fwrite(&two_byte_data[i], BYTES_PER_SAMPLE, 1, output);
                break;
            case 4:
                fwrite(&four_byte_data[i], BYTES_PER_SAMPLE, 1, output);
                break;
        }
    }

    // Write the middle
    // Loop for X times
    for (int i = 0; i < loops; i++)
    {
        for (int j = start; j < end; j++)
        {
            switch (BYTES_PER_SAMPLE)
            {
                case 1:
                    fwrite(&one_byte_data[j], BYTES_PER_SAMPLE, 1, output);
                    break;
                case 2:
                    fwrite(&two_byte_data[j], BYTES_PER_SAMPLE, 1, output);
                    break;
                case 4:
                    fwrite(&four_byte_data[j], BYTES_PER_SAMPLE, 1, output);
                    break;
            }
        }
    }

    // Write the end
    for (int i = end; i < list_size; i++)
    {
        switch (BYTES_PER_SAMPLE)
        {
            case 1:
                fwrite(&one_byte_data[i], BYTES_PER_SAMPLE, 1, output);
                break;
            case 2:
                fwrite(&two_byte_data[i], BYTES_PER_SAMPLE, 1, output);
                break;
            case 4:
                fwrite(&four_byte_data[i], BYTES_PER_SAMPLE, 1, output);
                break;
        }
    }

    // End of program
    // Free up resources
    switch (BYTES_PER_SAMPLE)
    {
        case 1:
            free(one_byte_data);
            break;
        case 2:
            free(two_byte_data);
            break;
        case 4:
            free(four_byte_data);
            break;
    }
    free(header);
    fclose(input);
    fclose(output);

    if (found_loop)
    {
        printf("Done!\n");
    }
    return 0;
}

// Add -EXTENDED to file
// i.e. input.wav becomes input-EXTENDED.wav
void add_str(char *str, char *new_str, const char *replacement)
{
    char *without_header = strrchr(str, '.');
    strncpy(new_str, str, strlen(str) - strlen(without_header));
    strcat(new_str, replacement);
    strcat(new_str, without_header);
}

// Convert mm:ss to seconds
float get_sec(char *time)
{
    char *tmp_seconds = strrchr(time, ':');
    // No minute counter
    if (tmp_seconds == NULL)
    {
        return atof(time);
    }

    // Get correct value of seconds
    char seconds[strlen(tmp_seconds)];
    memcpy(seconds, tmp_seconds, strlen(tmp_seconds) + 1);
    seconds[0] = '0';

    // Get the minutes
    int n = strlen(time) - strlen(seconds);
    char minutes[n];
    memcpy(minutes, time, n + 1);
    minutes[n] = '\0';

    // Return correct value of seconds, given minutes and seconds
    return atoi(minutes) * 60 + atof(seconds);
}

// Get value in [min, max]
int constrain(int value, int min, int max)
{
    if (value < min)
    {
        return min;
    }
    else if (value > max)
    {
        return max;
    }
    else
    {
        return value;
    }
}
