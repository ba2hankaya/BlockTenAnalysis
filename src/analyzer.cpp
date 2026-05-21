#include <cstdio>
#include <fstream>
#include <string>
#include <thread>
#include <cassert>

const int NUM_THREADS = 16;

void print_array(int* arr, int size) {
    for (int i = 0; i < size; ++i) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

int calculate_score_for_line(const std::string& line) {
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

        #ifndef NDEBUG
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

        #ifndef NDEBUG
        print_array(val_array, 14);
        printf("Active cards: %d\n", count);
        printf("Score: %d\n", val_array[0]);
        printf("---------------------------------\n");
        #endif

        //if there are more than 9 active cards, the game is over, so return score
        if(count > 9){
            return val_array[0];
        }
    }
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

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s %s %s", argv[0], argv[1], argv[2]);
        return 1;
    }

    std::string input_file = argv[1];
    std::string output_file = argv[2];

    int num_lines = 0;
    std::string* lines = nullptr;

    try{
        std::ifstream file(input_file);
        if (!file.is_open()) {
            return 1;
        }

        file >> num_lines;

        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        lines = new std::string[num_lines];

        if(!lines){
            fprintf(stderr, "Memory allocation failed for lines array\n");
            return 1;
        }

        for (int i = 0; i < num_lines; ++i) {
            std::getline(file, lines[i]);
        }
        file.close();
    }
    catch(const std::exception& e){
        fprintf(stderr, "Error reading input file: %s\n", e.what());
        return 1;
    }

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

    int final_result[25] = {0};

    for (int i = 0; i < NUM_THREADS; ++i) {
        for (int j = 0; j < 25; ++j) {
            final_result[j] += results[i][j];
        }
    }

    try{
        std::ofstream output(output_file);
        output << "{\n";
        for(int i = 0; i < 25; ++i) {
            output << "  \"" << i << "\": " << final_result[i];
            if(i != 24){
                output << ",";
            }
            output << "\n";
        }
        output << "}\n";
        output.close();
    }
    catch(const std::exception& e){
        fprintf(stderr, "Error writing to output file: %s\n", e.what());
        return 1;
    }

    return 0;
}