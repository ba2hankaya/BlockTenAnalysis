#include <cstdio>
#include <fstream>
#include <string>
#include <thread>
#include <cassert>

//#define SHOW_EACH_MOVE

const int NUM_THREADS = 16;

void print_array(int* arr, int size) {
    for (int i = 0; i < size; ++i) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

int calculate_score_for_line(const std::string& line) {
    if (line.empty()) {
        return -1;
    }
    int count = 0;
    int val_array[14] = {0};
    //parse char by char for faster processing, since we know the format is always correct, we can skip some checks
    const char* cstr = line.c_str();

    while(*cstr)
    {
        while(*cstr == ' '){
            cstr++;
        }
        if(*cstr == '\0' || *cstr == '\n' || *cstr == '\r' || !(*cstr)){
            break;
        }
        
        int num = 0;
        while(*cstr >= '0' && *cstr <= '9'){
            num = num * 10 + (*cstr - '0');
            cstr++;
        }

        if(num < 1 || num > 13)
        {
            return -1;
        }

        assert(count >= 0 && count <= 9);
        assert(val_array[0] <= 24);

        #if !defined(NDEBUG) && defined(SHOW_EACH_MOVE)
        printf("%d\n", num);
        #endif

        if (num < 10)
        {
            //if there is a corresponfing pair, remove it and increment the score, decrease count of active cards
            if(val_array[10 - num] > 0){
                val_array[10 - num]--;
                val_array[0]++;
                count--;
            }
            else{
                //if there is no pair, add the card to the active cards and increase count of active cards
                val_array[num]++;
                count++;
            }
        }else if(num > 10){
            //if there is a corresponfing pair, remove it and increment the score, decrease count of active cards
            if(val_array[num] > 0){
                val_array[num]--;
                val_array[0]++;
                count--;
            }else{
                //if there is no pair, add the card to the active cards and increase count of active cards
                val_array[num]++;
                count++;
            }
        }else if(num == 10){
            //10 cannot be paired, so just add it to the active cards and increase count of active cards
            val_array[10]++;
            count++;
        }

        #if !defined(NDEBUG) && defined(SHOW_EACH_MOVE)
        print_array(val_array, 14);
        printf("Active cards: %d\n", count);
        printf("Score: %d\n", val_array[0]);
        printf("---------------------------------\n");
        #endif

        //if there are more than 9 active cards, the game is over, so return score
        if(count > 9){
            assert(val_array[0] <= 24 && val_array[0] >= 0);
            return val_array[0];
        }
    }
    assert(val_array[0] <= 24 && val_array[0] >= 0);
    return val_array[0];
}



void analyze_file_part(const std::string* lines, const int start_line, const int end_line, int* result) {
    for (int i = start_line; i < end_line; ++i) {
        int score = calculate_score_for_line(lines[i]);
        if(score == -1){
            printf("Corrupted line at: %d\n", i);
        }
        else{
            result[score]++;
        }
    }
}

void process_file(const std::string& filename, const int num_lines,const std::string* lines, int* file_result) {
    int results[NUM_THREADS][25] = {0};
    std::thread threads[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; ++i) {
        int start_line = i * (num_lines / NUM_THREADS);
        int end_line = (i + 1) * (num_lines / NUM_THREADS);
        if (i == NUM_THREADS - 1) {
            end_line = num_lines;
        }
        threads[i] = std::thread(analyze_file_part, lines, start_line, end_line, results[i]);
    }

    for(int i = 0; i < NUM_THREADS; ++i) {
        threads[i].join();
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        for (int j = 0; j < 25; ++j) {
            file_result[j] += results[i][j];
        }
    }
}


int main(int argc, char* argv[]) {
    if(argc < 4){
        fprintf(stderr, "Usage: program <input_file> <input_file1> <input_file2> ... -o <output_file>\n");
        return 1;
    }

    std::string input_files[argc - 3];
    std::string output_file;

    for(int i = 1; i < argc - 1; ++i){
        if(std::string(argv[i]) == "-o"){
            if(i + 1 < argc){
                output_file = argv[i + 1];
            }
            else{
                fprintf(stderr, "Output file not specified after -o\n");
                return 1;
            }
        }else{
            input_files[i - 1] = argv[i];
        }
    }

    #ifndef NDEBUG
    printf("Output file: %s\n", output_file.c_str());
    for(int i = 0; i < argc - 3; ++i){
        printf("Input file %d: %s\n", i, input_files[i].c_str());
    }
    #endif

    int final_result[25] = {0};


    try{
        for(int i = 0; i < argc - 3; ++i){
            int num_lines = 0;
            std::ifstream file(input_files[i]);
            if (!file.is_open()) {
                throw std::runtime_error("Could not open file: " + input_files[i]);
            }

            file >> num_lines;

            file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            std::string* lines = new std::string[num_lines];

            for (int i = 0; i < num_lines; ++i) {
                if(!std::getline(file, lines[i]))
                {
                    fprintf(stderr, "Unexpected EOF at line %d in file %s\n", i, input_files[i].c_str());
                    lines[i] = "";
                }
            }
            file.close();
            int file_result[25] = {0};
            process_file(input_files[i], num_lines, lines, file_result);
            for (int j = 0; j < 25; ++j) {
                final_result[j] += file_result[j];
            }
            delete[] lines;
        }
    }
    catch(const std::exception& e){
        fprintf(stderr, "Error processing input file(s): %s\n", e.what());
        return 1;
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