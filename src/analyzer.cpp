#include <cstdint>
#include <cstdio>
#include <fstream>
#include <ios>
#include <string>
#include <sys/types.h>
#include <cassert>
#include "concurrentque.hpp"
#include <thread>

//#define SHOW_EACH_MOVE

const std::size_t BUFFER_SIZE_BYTES = 4*52*1024*1024*8/8; //208 MB

const int NUM_THREADS = 16;

int score_of_subarray(char* buffer, int start_index){
    int val_array[14] = {0};
    int count = 0;
    for(u_int32_t i = 0; i < 26; i++){
        u_char first = (u_char)buffer[start_index + i] >> 4;
        u_char second = buffer[start_index + i] & 0x0F;

        if (first < 10)
        {
            //if there is a corresponfing pair, remove it and increment the score, decrease count of active cards
            if(val_array[10 - first] > 0){
                val_array[10 - first]--;
                val_array[0]++;
                count--;
            }
            else{
                //if there is no pair, add the card to the active cards and increase count of active cards
                val_array[first]++;
                count++;
            }
        }else if(first > 10){
            //if there is a corresponfing pair, remove it and increment the score, decrease count of active cards
            if(val_array[first] > 0){
                val_array[first]--;
                val_array[0]++;
                count--;
            }else{
                //if there is no pair, add the card to the active cards and increase count of active cards
                val_array[first]++;
                count++;
            }
        }else if(first == 10){
            //10 cannot be paired, so just add it to the active cards and increase count of active cards
            val_array[10]++;
            count++;
        }

        //if there are more than 9 active cards, the game is over, so return score
        if(count > 9){
            assert(val_array[0] <= 24 && val_array[0] >= 0);
            return val_array[0];
        }

        if (second < 10)
        {
            //if there is a corresponfing pair, remove it and increment the score, decrease count of active cards
            if(val_array[10 - second] > 0){
                val_array[10 - second]--;
                val_array[0]++;
                count--;
            }
            else{
                //if there is no pair, add the card to the active cards and increase count of active cards
                val_array[second]++;
                count++;
            }
        }else if(second > 10){
            //if there is a corresponfing pair, remove it and increment the score, decrease count of active cards
            if(val_array[second] > 0){
                val_array[second]--;
                val_array[0]++;
                count--;
            }else{
                //if there is no pair, add the card to the active cards and increase count of active cards
                val_array[second]++;
                count++;
            }
        }else if(second== 10){
            //10 cannot be paired, so just add it to the active cards and increase count of active cards
            val_array[10]++;
            count++;
        }

        if(count > 9){
            assert(val_array[0] <= 24 && val_array[0] >= 0);
            return val_array[0];
        }
    }

    assert(val_array[0] <= 24 && val_array[0] >= 0);
    return val_array[0];
}

void process_buffer(char* buffer, int* result_array, std::streamsize bytes_read){
    for(int i = 0; i + 25 < bytes_read; i+=26){
        int result = score_of_subarray(buffer, i);
        result_array[result]++;
    }
}

void producer(std::ifstream& file, int buffer_size, ConcurrentQueue& queue, int* file_result){
    file.seekg(0, std::ios::beg);
    while(true){
        char* buffer = new char[buffer_size];
        file.read(buffer, buffer_size);
        std::streamsize bytes_read = file.gcount();
        if(bytes_read > 0){
            queue.Enqueue(buffer, bytes_read);
        }else{
            delete [] buffer;
            break;
        }
    }
    queue.Shutdown();
}

void worker(int id, int* results,ConcurrentQueue& queue){
    while(true){
        char* buffer;
        int buf_size; 
        if(queue.Dequeue(buffer,buf_size) == -1){
            break;
        }else{
            process_buffer(buffer, results, buf_size);
            delete [] buffer;
            buffer = nullptr;
        }
    }
}


int main(int argc, char* argv[]) {
    std::string input_file;
    std::string output_file;
    int max_memory_bytes;
    if(argc == 3)
    {
        max_memory_bytes = -1;
    }
    else if(argc == 4){
        max_memory_bytes = std::stoi(argv[3]) * 1024 * 1024;
        int remainder = max_memory_bytes % 26 != 0;
        if(remainder > 0){
            max_memory_bytes -= remainder;
            fprintf(stdout, "Max memory has been reduced to %d bytes to match deck size.\n", max_memory_bytes);
        }
    }else{
        fprintf(stderr, "Usage: program <input_file> <output_file>\n");
        return 1;
    }

    input_file = argv[1];
    output_file = argv[2];

    #ifndef NDEBUG
    printf("Output file: %s\n", output_file.c_str());
    printf("Input file: %s\n", input_file.c_str());
    #endif

    int final_result[25] = {0};

    std::ifstream file(input_file, std::ios::binary | std::ios::ate);
    if(!file)
    {
        fprintf(stderr, "failed to open the input file.\n");
        exit(1);
    }

    std::streamsize total_size_bytes = file.tellg();
    int total_deck_count = total_size_bytes / 26;
    int buffer_size;

    if(max_memory_bytes == -1){
        buffer_size = BUFFER_SIZE_BYTES < (total_size_bytes/NUM_THREADS/2) ? BUFFER_SIZE_BYTES : total_size_bytes/NUM_THREADS;
        max_memory_bytes = INT32_MAX;
    }else{
        buffer_size = max_memory_bytes / NUM_THREADS / 2;
    }

    if(buffer_size % 26 != 0){
        buffer_size = buffer_size - (buffer_size % 26);
    }

    ConcurrentQueue queue(max_memory_bytes);

    int local_result_array[NUM_THREADS][25] = {0};

    std::thread workers[NUM_THREADS];


    for(int i = 0; i < NUM_THREADS; i++){
        workers[i] = std::thread(worker, i, local_result_array[i], std::ref(queue));
    }

    producer(file, buffer_size, queue, final_result);

    for(int i = 0; i < NUM_THREADS; i++){
        workers[i].join();
    }

    for(int i = 0; i < NUM_THREADS; i++){
        for(int j = 0; j < 25; j++){
            final_result[j] += local_result_array[i][j];
        }
    }

    try{
        int total_games = 0;
        std::ofstream output(output_file);
        output << "{\n";
        for(int i = 0; i < 25; ++i) {
            output << "  \"" << i << "\": " << final_result[i];
            if(i != 24){
                output << ",";
            }
            output << "\n";
            total_games += final_result[i];
        }
        output << "}\n";
        output.close();
        printf("Total number of games analyzed: %d\n", total_games);
        printf("Wrote the results to %s\n", output_file.c_str());
    }
    catch(const std::exception& e){
        fprintf(stderr, "Error writing to output file: %s\n", e.what());
        return 1;
    }

    return 0;
}