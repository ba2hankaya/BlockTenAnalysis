#include <cstdint>
#include <cstdio>
#include <fstream>
#include <ios>
#include <string>
#include <sys/types.h>
#include <cassert>

//#define SHOW_EACH_MOVE

const std::size_t BUFFER_SIZE = 4*52*1024*1024*8/8; //208 MB

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

//void process_file(const std::string& filename, const int num_lines,const std::string* lines, int* file_result) {
//    int results[NUM_THREADS][25] = {0};
//    std::thread threads[NUM_THREADS];
//
//    for (int i = 0; i < NUM_THREADS; ++i) {
//        int start_line = i * (num_lines / NUM_THREADS);
//        int end_line = (i + 1) * (num_lines / NUM_THREADS);
//        if (i == NUM_THREADS - 1) {
//            end_line = num_lines;
//        }
//        threads[i] = std::thread(analyze_file_part, lines, start_line, end_line, results[i]);
//    }
//
//    for(int i = 0; i < NUM_THREADS; ++i) {
//        threads[i].join();
//    }
//
//    for (int i = 0; i < NUM_THREADS; ++i) {
//        for (int j = 0; j < 25; ++j) {
//            file_result[j] += results[i][j];
//        }
//    }
//}


int main(int argc, char* argv[]) {
    if(argc != 3){
        fprintf(stderr, "Usage: program <input_file> <output_file>\n");
        return 1;
    }

    std::string input_file;
    std::string output_file;

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
        return 1;
    }

    std::streamsize size = file.tellg();

    file.seekg(0, std::ios::beg);

    uint32_t num_entries = size * 8 / (4*52);
    char* buffer = new char[BUFFER_SIZE];
    int local_result_array[25] = {0};
    while(file.read(buffer, BUFFER_SIZE) || file.gcount() > 0){
        std::streamsize bytes_read = file.gcount();
        if(bytes_read > 0){
            process_buffer(buffer, local_result_array, bytes_read);
        }
    }

    for(int i = 0; i < 25; i++){
        final_result[i] = local_result_array[i];
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