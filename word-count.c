#include <mpi.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>


#define FILE_PATH_SIZE 200
#define MINIMUM_SIZE_FOR_REMAINING_AMOUNT 20
#define SIZE_FOR_REALLOC_NUM_TASKS 5
#define SIZE_FOR_REALLOC_ARRAY_CHAR 100
#define MAX_WORD_LENGTH 100
#define TABLE_SIZE 2000
#define PATH_TIMES_FILE "risultati.txt"
#define PATH_DEFAULT_CSV_FILE "file_csv.csv"

typedef struct {
    char file_path[FILE_PATH_SIZE];
    off_t size_of_file_in_bytes;
} file_info;

typedef struct {
    char file_path[FILE_PATH_SIZE];
    off_t start_offset;
    off_t end_offset;
    int process_rank;
} process_file_task;

typedef struct struct_word_elem{
    char word[MAX_WORD_LENGTH];
    int num_occurrences;
    struct struct_word_elem * next;
} word_elem;

typedef struct {
    int num_of_words;
    word_elem * words;
} table_entry;


file_info * get_array_file_info(char * dir_path, int * sum, int * count_files){
    file_info * arr;
    DIR * dir;
    struct dirent* file_in_dir;

    if((dir = opendir(dir_path)) == NULL) {
        fprintf(stderr, "error open dir\n");
        return NULL;
    }
    *count_files=0;
    while ((file_in_dir = readdir(dir)) != NULL){
        if(!(strcmp (file_in_dir->d_name, ".")))
            continue;
        if(!(strcmp (file_in_dir->d_name, "..")))
            continue; 
        *count_files = *count_files + 1;
    }
    closedir(dir);
    
    arr = malloc(sizeof(file_info) * (*count_files));
    if((dir = opendir(dir_path)) == NULL) {
        fprintf(stderr, "error opening input dir\n");
        return NULL;
    }
    struct stat stat_file;
    int i=0;
    *sum = 0;
    while ((file_in_dir = readdir(dir)) != NULL){
        if(!(strcmp (file_in_dir->d_name, ".")))
            continue;
        if(!(strcmp (file_in_dir->d_name, "..")))
            continue;    
        strcpy(arr[i].file_path, dir_path);
        strcat(arr[i].file_path, "/");
        strcat(arr[i].file_path, file_in_dir->d_name);
        
        if(stat(arr[i].file_path, &stat_file) != 0)
            fprintf(stderr, "stat error on file %s\n", arr[i].file_path);
            
        arr[i].size_of_file_in_bytes = stat_file.st_size;
        *sum = *sum + arr[i].size_of_file_in_bytes; 
        i++;
    }
    closedir(dir);
    return arr;
}

process_file_task * get_array_process_file_task(file_info * arr_file_info, int sum_of_file_sizes, int number_of_files, int number_of_processes, int * number_of_process_file_task){
    *number_of_process_file_task = 0; 
    process_file_task * arr_process_file_task;
    int allocated_mem_for_arr_process_file_task = SIZE_FOR_REALLOC_NUM_TASKS;
    arr_process_file_task = malloc(sizeof(process_file_task) * allocated_mem_for_arr_process_file_task);
    int process_task_size = sum_of_file_sizes / number_of_processes, file_index=0, task_index=0, start_offset_index=0;

    for(int index_process=0; index_process<number_of_processes; index_process++){
		int remaining_task_size = process_task_size;
		while(remaining_task_size > 0){
			arr_process_file_task[task_index].start_offset = start_offset_index;
            strcpy(arr_process_file_task[task_index].file_path, arr_file_info[file_index].file_path);
            arr_process_file_task[task_index].process_rank = index_process;
			if(remaining_task_size > (arr_file_info[file_index].size_of_file_in_bytes - start_offset_index)){
                arr_process_file_task[task_index].end_offset = arr_file_info[file_index].size_of_file_in_bytes;
                
                remaining_task_size = remaining_task_size - (arr_process_file_task[task_index].end_offset - arr_process_file_task[task_index].start_offset);
                file_index++;
                start_offset_index = 0;
            }
            else{
                arr_process_file_task[task_index].end_offset = start_offset_index + remaining_task_size;
                remaining_task_size = 0;
                start_offset_index = arr_process_file_task[task_index].end_offset;
                
            }
            task_index++;
            if((task_index + 1) > allocated_mem_for_arr_process_file_task){
                allocated_mem_for_arr_process_file_task = allocated_mem_for_arr_process_file_task + SIZE_FOR_REALLOC_NUM_TASKS;
                arr_process_file_task = (process_file_task *) realloc(arr_process_file_task, sizeof(process_file_task) * allocated_mem_for_arr_process_file_task);
            }
		}
    }

    //codice per il resto delle porzioni di file da assegnare
    int assigned_amount = number_of_processes * process_task_size, remaining_amount_to_assign = sum_of_file_sizes - assigned_amount, remaining_amount_for_process = remaining_amount_to_assign / number_of_processes;

    if(remaining_amount_for_process < MINIMUM_SIZE_FOR_REMAINING_AMOUNT)
        remaining_amount_for_process = MINIMUM_SIZE_FOR_REMAINING_AMOUNT;

    int index_process = 0;
    while(remaining_amount_to_assign > 0){
        arr_process_file_task[task_index].start_offset = start_offset_index;
        strcpy(arr_process_file_task[task_index].file_path, arr_file_info[file_index].file_path);
        arr_process_file_task[task_index].process_rank = index_process;
        if(remaining_amount_for_process > (arr_file_info[file_index].size_of_file_in_bytes - start_offset_index)){
            arr_process_file_task[task_index].end_offset = arr_file_info[file_index].size_of_file_in_bytes;
            
            remaining_amount_for_process = remaining_amount_for_process - (arr_process_file_task[task_index].end_offset - arr_process_file_task[task_index].start_offset);
            file_index++;
            start_offset_index = 0;
        }
        else{
            arr_process_file_task[task_index].end_offset = start_offset_index + remaining_amount_for_process;
            remaining_amount_for_process = 0;
            start_offset_index = arr_process_file_task[task_index].end_offset;
        }
        task_index++;
        if((task_index + 1) > allocated_mem_for_arr_process_file_task){
            allocated_mem_for_arr_process_file_task = allocated_mem_for_arr_process_file_task + SIZE_FOR_REALLOC_NUM_TASKS;
            arr_process_file_task = (process_file_task *) realloc(arr_process_file_task, sizeof(process_file_task) * allocated_mem_for_arr_process_file_task);
        }
        remaining_amount_to_assign = remaining_amount_to_assign - (arr_process_file_task[task_index-1].end_offset - arr_process_file_task[task_index-1].start_offset);
        index_process++;
        if(index_process >= number_of_processes)
            index_process = 0;
    }


    *number_of_process_file_task = task_index; 
    return arr_process_file_task;
}

void sort_process_file_task_arr_by_rank(process_file_task * arr, int num_of_process_file_task, int number_of_processes){
    int l_index; 
    for(int i=0; i< number_of_processes; i++){
        l_index = -1;
        for(int j=0; j < num_of_process_file_task; j++){
            if((arr[j].process_rank > i) && (l_index == -1))
                l_index = j;
            if((arr[j].process_rank == i) && (l_index != -1)){
                //swap tra arr[l_index] e arr[j]
                process_file_task temp;
                strcpy(temp.file_path, arr[l_index].file_path);
                temp.start_offset = arr[l_index].start_offset;
                temp.end_offset = arr[l_index].end_offset;
                temp.process_rank = arr[l_index].process_rank;

                strcpy(arr[l_index].file_path, arr[j].file_path);
                arr[l_index].start_offset = arr[j].start_offset;
                arr[l_index].end_offset = arr[j].end_offset;
                arr[l_index].process_rank = arr[j].process_rank;

                strcpy(arr[j].file_path, temp.file_path);
                arr[j].start_offset = temp.start_offset;
                arr[j].end_offset = temp.end_offset;
                arr[j].process_rank = temp.process_rank;
                l_index++;
            }
        }
    }
}

int * create_array_num_file_task_for_process(process_file_task * arr, int num_of_process_file_task, int number_of_processes){
    int * count_file_task_for_process_arr;
    count_file_task_for_process_arr = malloc(sizeof(int) * number_of_processes);
    for(int i=0; i<number_of_processes; i++){
        int count=0;
        for(int j=0; j<num_of_process_file_task; j++){
            if(arr[j].process_rank == i)
                count++;
            if(arr[j].process_rank > i){
                count_file_task_for_process_arr[i] = count;
                break;
            }
            count_file_task_for_process_arr[i] = count;
        }
    }
    return count_file_task_for_process_arr;
}

int * create_array_disps(process_file_task * arr, int num_of_process_file_task, int number_of_processes){
	int * displs;
	displs = malloc(sizeof(int)*number_of_processes);
	
	for(int i=0; i < number_of_processes; i++){
		for(int j=0; j < num_of_process_file_task; j++){
			if(arr[j].process_rank == i){
				displs[i] = j;
				break;
			}
	    }
    }
    return displs;
}

unsigned calc_hash_key(char * key){
    int len = strlen(key);
    unsigned char *p = key;
    unsigned h = 0x811c9dc5;
    int i;

    for ( i = 0; i < len; i++ )
      h = ( h ^ p[i] ) * 0x01000193;

   return h;
}

void increase_occurrence_of_word(table_entry * ht, char * word, int increase){
    int word_index = calc_hash_key(word) % TABLE_SIZE;
    if(ht[word_index].num_of_words == 0){
        ht[word_index].num_of_words = 1;
        ht[word_index].words = malloc(sizeof(word_elem) * 1);
        strcpy(ht[word_index].words->word, word);
        ht[word_index].words->num_occurrences = increase;
        ht[word_index].words->next = NULL;
    }
    else{
        word_elem * pwe;
        pwe = ht[word_index].words;
        while((strcmp(pwe->word, word)) && (pwe->next != NULL)){
            pwe = pwe->next;     
        }
        if((strcmp(pwe->word, word)) == 0)
            pwe->num_occurrences = pwe->num_occurrences + increase;
        else{
            pwe->next = malloc(sizeof(word_elem) * 1);
            pwe = pwe->next;
            strcpy(pwe->word, word);
            pwe->num_occurrences = increase;
            pwe->next = NULL;
			ht[word_index].num_of_words += 1;
        }
    } 
}

void initialize_hash_table(table_entry * ht){
    for(int i=0;i < TABLE_SIZE; i++){
        ht[i].num_of_words = 0;
        ht[i].words = NULL;
    }
}

int get_occurrence_of_word(table_entry * ht, char * word){
    int word_index = calc_hash_key(word) % TABLE_SIZE;
    if(ht[word_index].num_of_words == 0){
        return 0;
    }
    else{
        word_elem * pwe;
        pwe = ht[word_index].words;
        while((strcmp(pwe->word, word)) && (pwe->next != NULL)){
            pwe = pwe->next;     
        }
        if((strcmp(pwe->word, word)) == 0)
            return pwe->num_occurrences;
        else{
            return 0;
        }
    } 
}

char * insert_element_in_array_char(char * arr_char, char * word, int occurrences, int * allocated_mem_arr_char, int * current_length_arr_char){
    int num_of_char_required = snprintf(NULL, 0,"%s:%d", word, occurrences);
    if((*current_length_arr_char) + num_of_char_required + 1 > (*allocated_mem_arr_char)){
        *allocated_mem_arr_char = *allocated_mem_arr_char + SIZE_FOR_REALLOC_ARRAY_CHAR;
        arr_char = realloc(arr_char, sizeof(char) * (*allocated_mem_arr_char));
    }
    sprintf(&arr_char[(*current_length_arr_char)], "%s:%d", word, occurrences);
    *current_length_arr_char = *current_length_arr_char + num_of_char_required + 1;
    return arr_char;
}

char * convert_ht_in_array_char(table_entry * ht, int * length_arr_char){
    char * arr_char;
    int allocated_mem_arr_char = SIZE_FOR_REALLOC_ARRAY_CHAR;
    *length_arr_char = 0;
    arr_char = malloc(sizeof(char) * allocated_mem_arr_char);
    for(int i=0; i < TABLE_SIZE; i++){
        word_elem * pwe;
        pwe = ht[i].words;
        while(pwe != NULL){
            arr_char = insert_element_in_array_char(arr_char, pwe->word, pwe->num_occurrences, &allocated_mem_arr_char, length_arr_char);
            pwe = pwe->next;
        }
    }
    return arr_char;
}
 
int calculate_effective_initial_seek(FILE* file_ptr, int start_offset_of_task){
    long curr_seek = ftell(file_ptr);
    if(start_offset_of_task == 0)
        return 0;
    fseek(file_ptr, start_offset_of_task, SEEK_SET);
    char line[MAX_WORD_LENGTH];
    int num_of_read_char = fread(line, 1, MAX_WORD_LENGTH, file_ptr);
    if(isalpha(line[0]) == 0){
        fseek(file_ptr, curr_seek, SEEK_SET);
        return start_offset_of_task;
    }
    else{
        for(int i=0; i < num_of_read_char; i++){
            if(isalpha(line[i]) == 0){
                fseek(file_ptr, curr_seek, SEEK_SET);
                return start_offset_of_task + i;
            }   
        }
    }
    fseek(file_ptr, curr_seek, SEEK_SET);
    //caso in cui la parola a cavallo tra due processi è l'ultima parola nel file
    return start_offset_of_task + num_of_read_char + 1;
}

int calculate_effective_final_seek(FILE* file_ptr, int end_offset_of_task){
    long curr_seek = ftell(file_ptr);
    fseek(file_ptr, end_offset_of_task, SEEK_SET);
    char line[MAX_WORD_LENGTH];
    int num_of_read_char = fread(line, 1, MAX_WORD_LENGTH, file_ptr);
    if(num_of_read_char == 0){
        fseek(file_ptr, curr_seek, SEEK_SET);
        return end_offset_of_task;
    }
    if(isalpha(line[0]) == 0){
        fseek(file_ptr, curr_seek, SEEK_SET);
        return end_offset_of_task;
    }   
    else{
        for(int i=0; i < num_of_read_char; i++){
            if(isalpha(line[i]) == 0){
                fseek(file_ptr, curr_seek, SEEK_SET);
                return end_offset_of_task + i;
            }
                
        }
    }
    fseek(file_ptr, curr_seek, SEEK_SET);
    return end_offset_of_task + num_of_read_char + 1;
}

void count_word_in_buff(table_entry * ht, char * read_buff, long unsigned num_char_in_buff){
    int i=0, word_len;
    char * word;
    word = malloc(sizeof(char) * MAX_WORD_LENGTH);
    while(i < num_char_in_buff){
        strcpy(word, "");
        word_len = 0;
        while((isalpha(read_buff[i]) == 0) && (i < num_char_in_buff))
            i++;
        while((isalpha(read_buff[i]) != 0) && (i < num_char_in_buff)){
            word[word_len] = read_buff[i];
            word_len++;
            i++;
        }
        if((word_len > 0) ){
            word[word_len] = '\0';
            for(int j = 0; word[j] != '\0'; j++){
                word[j] = tolower(word[j]);
            }
            increase_occurrence_of_word(ht, word, 1);
        }
    }
}

void count_words(table_entry * ht, process_file_task * arr_process_file_task, int number_of_process_file_task){
    char * read_buff;
    read_buff = malloc(sizeof(char) * 1);
    for(int i=0; i < number_of_process_file_task; i++){
        FILE* file_ptr;
        file_ptr = fopen(arr_process_file_task[i].file_path, "r");
        if(file_ptr == NULL){
            fprintf(stderr, "error opening file %s", arr_process_file_task[i].file_path);
            return;
        }
        arr_process_file_task[i].start_offset = calculate_effective_initial_seek(file_ptr, arr_process_file_task[i].start_offset);
		arr_process_file_task[i].end_offset = calculate_effective_final_seek(file_ptr, arr_process_file_task[i].end_offset);
        fseek(file_ptr, arr_process_file_task[i].start_offset, SEEK_SET);
        long unsigned num_of_char_to_read = arr_process_file_task[i].end_offset - arr_process_file_task[i].start_offset;
        
        read_buff = realloc(read_buff, sizeof(char) * num_of_char_to_read);
        
        long unsigned num_char_in_buff = fread(read_buff, 1, num_of_char_to_read, file_ptr);
        count_word_in_buff(ht, read_buff, num_char_in_buff);
        
        fclose(file_ptr);
    }
}

void merge_arr_char_in_ht(table_entry * local_ht, char * arr_char, int num_of_char_in_arr_char){
    int read_char=0, read_char_in_word, num_occur_of_word;
    char word_and_num_occur[MAX_WORD_LENGTH + 10];
    char * word, * num_occ_str;
    while(read_char < num_of_char_in_arr_char){
        sscanf(&(arr_char[read_char]), "%s%n", word_and_num_occur, &read_char_in_word);
        read_char += read_char_in_word + 1;
        word = strtok(word_and_num_occur, ":");
        num_occ_str = strtok(NULL, ":");
        increase_occurrence_of_word(local_ht, word, atoi(num_occ_str));
    }
}

void reduce_tables(int my_rank, int number_of_processes, table_entry * local_ht){
    //codice per recv
    int left_child_tree = (2 * (my_rank + 1)) - 1, right_child_tree = 2 * (my_rank + 1), parent_tree = (my_rank - 1) / 2;
    if(number_of_processes-1 >= right_child_tree){
        int num_of_char_in_arr_char;
        char * arr_char;
        //figlio sinistro
        MPI_Recv(&num_of_char_in_arr_char, 1, MPI_INT, left_child_tree, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        arr_char = malloc(sizeof(char) * num_of_char_in_arr_char);
        MPI_Recv(arr_char, num_of_char_in_arr_char, MPI_CHAR, left_child_tree, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        merge_arr_char_in_ht(local_ht, arr_char, num_of_char_in_arr_char);
        free(arr_char);
        //figlio destro
        MPI_Recv(&num_of_char_in_arr_char, 1, MPI_INT, right_child_tree, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        arr_char = malloc(sizeof(char) * num_of_char_in_arr_char);
        MPI_Recv(arr_char, num_of_char_in_arr_char, MPI_CHAR, right_child_tree, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        merge_arr_char_in_ht(local_ht, arr_char, num_of_char_in_arr_char);
        free(arr_char);
    }
    else if(number_of_processes-1 >= left_child_tree){
        int num_of_char_in_arr_char;
        char * arr_char;
        //figlio sinistro
        MPI_Recv(&num_of_char_in_arr_char, 1, MPI_INT, left_child_tree, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        arr_char = malloc(sizeof(char) * num_of_char_in_arr_char);
        MPI_Recv(arr_char, num_of_char_in_arr_char, MPI_CHAR, left_child_tree, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        merge_arr_char_in_ht(local_ht, arr_char, num_of_char_in_arr_char);
        free(arr_char);
    }
    
    //codice per le send
    if(my_rank != 0){
        int num_of_char_in_arr_char;
        char * arr_char;
        arr_char = convert_ht_in_array_char(local_ht, &num_of_char_in_arr_char);
        MPI_Send(&num_of_char_in_arr_char, 1, MPI_INT, parent_tree, 0, MPI_COMM_WORLD);
        MPI_Send(arr_char, num_of_char_in_arr_char, MPI_CHAR, parent_tree, 0, MPI_COMM_WORLD);
        free(arr_char);
    }
}

word_elem * insert_elem_in_sorted_list_of_words(word_elem * sorted_list, char * word, int num_occ){
    if(sorted_list == NULL){
        sorted_list = malloc(sizeof(word_elem));
        strcpy(sorted_list->word, word);
        sorted_list->num_occurrences = num_occ;
        sorted_list->next = NULL;
        return sorted_list;
    }
    if((sorted_list->num_occurrences < num_occ) || ((sorted_list->num_occurrences == num_occ) && (strcmp(sorted_list->word, word) > 0))){
        word_elem * ptr_new_elem;
        ptr_new_elem = malloc(sizeof(word_elem));
        strcpy(ptr_new_elem->word, word);
        ptr_new_elem->num_occurrences = num_occ;
        ptr_new_elem->next = sorted_list;
        sorted_list = ptr_new_elem;
        return sorted_list;
    }

    word_elem * ptr;
    ptr = sorted_list;
    while((ptr->next != NULL) && ((ptr->next->num_occurrences > num_occ)  || ((ptr->next->num_occurrences == num_occ) && (strcmp(ptr->next->word, word) < 0))))
        ptr = ptr->next;
    if(ptr->next == NULL){
        ptr->next = malloc(sizeof(word_elem));
        strcpy(ptr->next->word, word);
        ptr->next->num_occurrences = num_occ;
        ptr->next->next = NULL;
    }
    else{
        word_elem * ptr_new_elem;
        ptr_new_elem = malloc(sizeof(word_elem));
        strcpy(ptr_new_elem->word, word);
        ptr_new_elem->num_occurrences = num_occ;
        ptr_new_elem->next = ptr->next;
        ptr->next = ptr_new_elem;
    }
    return sorted_list;
}

word_elem * create_sorted_list_of_words(table_entry * ht){
    word_elem * sorted_list;
    sorted_list = NULL;
    for(int i=0;i < TABLE_SIZE; i++){
        if(ht[i].num_of_words == 0)
            continue;
        word_elem * pwe;
        pwe = ht[i].words;
        while(pwe != NULL){ 
            sorted_list = insert_elem_in_sorted_list_of_words(sorted_list, pwe->word, pwe->num_occurrences);
            pwe = pwe->next;   
        }
    }
    return sorted_list;
}

void print_csv_file(char * path_scv_file, word_elem * sorted_list_of_words){
    FILE* file_ptr;
    file_ptr = fopen(path_scv_file, "w");
    if(file_ptr == NULL)
        fprintf(stderr, "error opening file %s\n", path_scv_file);
    word_elem * ptr;
    ptr = sorted_list_of_words;
    fprintf(file_ptr, "word,number of occurrences\n");
    while(ptr != NULL){
        fprintf(file_ptr, "%s,%d\n", ptr->word, ptr->num_occurrences);
        ptr = ptr->next;
    }
    fclose(file_ptr);
}

int main(int argc, char** argv) {
    if(argc < 2){//deve essere dato come parametro il path della directory con i file su cui bisogna fare il word count ed opzionalmente il path del file csv
        fprintf(stderr, "numero insufficiente di parametri\n");
        return 0;
    }

    MPI_Init(&argc, &argv);

    int number_of_processes;
    MPI_Comm_size(MPI_COMM_WORLD, &number_of_processes);

    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);

    if((my_rank == 0) && (number_of_processes == 1)){//codice sequenziale
        double start_time, end_time;
        start_time = MPI_Wtime();
        DIR * dir;
        struct dirent * file_in_dir;
        table_entry * ht;
        ht = malloc(sizeof(table_entry) * TABLE_SIZE);
        initialize_hash_table(ht);
        char path_file[FILE_PATH_SIZE];
        long unsigned file_size;
        struct stat stat_file;
        FILE* file_ptr;
        char * read_buff;
        read_buff = malloc(sizeof(char) * 1);

        if((dir = opendir(argv[1])) == NULL) {
            fprintf(stderr, "error opening input dir\n");
            return 0;
        }
        while ((file_in_dir = readdir(dir)) != NULL){
            if(!(strcmp (file_in_dir->d_name, ".")))
                continue;
            if(!(strcmp (file_in_dir->d_name, "..")))
                continue; 
            strcpy(path_file, argv[1]);
            strcat(path_file, "/");
            strcat(path_file, file_in_dir->d_name);
            
            if(stat(path_file, &stat_file) != 0){
                fprintf(stderr, "stat error on file %s\n", path_file);
                return 0;
            } 
            file_size = stat_file.st_size;
            file_ptr = fopen(path_file, "r");
            if(file_ptr == NULL){
                fprintf(stderr, "error opening file %s", path_file);
                return 0;
            }
            read_buff = realloc(read_buff, sizeof(char) * file_size);
            long unsigned num_of_read_char = fread(read_buff, 1, file_size, file_ptr);
            count_word_in_buff(ht, read_buff, num_of_read_char);
            fclose(file_ptr);
        }
        closedir(dir);

        word_elem * sorted_list_of_words;
        sorted_list_of_words = create_sorted_list_of_words(ht);
        if(argc >= 3)
            print_csv_file(argv[2], sorted_list_of_words);
        else
            print_csv_file(PATH_DEFAULT_CSV_FILE, sorted_list_of_words);
        end_time = MPI_Wtime();
        fprintf(stderr, "\n--------------tempo con codice sequenziale: %f\n", end_time - start_time);

        FILE * file_times_results;
        file_times_results = fopen(PATH_TIMES_FILE, "a");
        if(file_times_results == NULL){
            fprintf(stderr, "error opening file %s", PATH_TIMES_FILE);
        }
        fprintf(file_times_results, "time with %d processes: %f\n", number_of_processes, end_time - start_time);
        fclose(file_times_results);
        
        MPI_Finalize();
        return 0;
    }

    //codice per commit struct process_file_task
    MPI_Aint displacements_process_file_task[4], base_address_process_file_task;
    process_file_task process_file_task_per_commit;
    MPI_Get_address(&process_file_task_per_commit, &base_address_process_file_task);
    MPI_Get_address(&process_file_task_per_commit.file_path[0], &displacements_process_file_task[0]);
    MPI_Get_address(&process_file_task_per_commit.start_offset, &displacements_process_file_task[1]);
    MPI_Get_address(&process_file_task_per_commit.end_offset, &displacements_process_file_task[2]);
    MPI_Get_address(&process_file_task_per_commit.process_rank, &displacements_process_file_task[3]);
    displacements_process_file_task[0] = MPI_Aint_diff(displacements_process_file_task[0], base_address_process_file_task);
    displacements_process_file_task[1] = MPI_Aint_diff(displacements_process_file_task[1], base_address_process_file_task);
    displacements_process_file_task[2] = MPI_Aint_diff(displacements_process_file_task[2], base_address_process_file_task);
    displacements_process_file_task[3] = MPI_Aint_diff(displacements_process_file_task[3], base_address_process_file_task);

    int lengths_process_file_task[4] = {200, 1, 1, 1};
    MPI_Datatype types_process_file_task[4] = {MPI_CHAR, MPI_INT64_T, MPI_INT64_T, MPI_INT}, process_file_task_type;
    MPI_Type_create_struct(4, lengths_process_file_task, displacements_process_file_task, types_process_file_task, &process_file_task_type);
    MPI_Type_commit(&process_file_task_type);

    
    int num_of_task_to_perform;
    process_file_task * file_task_to_perform;
    table_entry * ht;
    
    
    if(my_rank == 0){
        double start_time, end_time;
        start_time = MPI_Wtime();

        int somma_sizes, num_of_files;
        file_info * arr_file_info;
        arr_file_info = get_array_file_info(argv[1], &somma_sizes, &num_of_files);

        fprintf(stderr, "\n\n");
        int number_of_process_file_task;
        process_file_task * arr_process_file_task;
        arr_process_file_task = get_array_process_file_task(arr_file_info, somma_sizes, num_of_files, number_of_processes, &number_of_process_file_task);

        free(arr_file_info);

        sort_process_file_task_arr_by_rank(arr_process_file_task, number_of_process_file_task, number_of_processes);

        int * num_file_task_for_process_arr;
        num_file_task_for_process_arr = create_array_num_file_task_for_process(arr_process_file_task, number_of_process_file_task, number_of_processes);

        int * arr_disps;
        arr_disps = create_array_disps(arr_process_file_task, number_of_process_file_task, number_of_processes);

        MPI_Scatter(num_file_task_for_process_arr, 1, MPI_INT, &num_of_task_to_perform, 1, MPI_INT, 0, MPI_COMM_WORLD);

        file_task_to_perform = malloc(sizeof(process_file_task) * num_of_task_to_perform);
        MPI_Scatterv(arr_process_file_task, num_file_task_for_process_arr, arr_disps, process_file_task_type, file_task_to_perform, num_of_task_to_perform, process_file_task_type, 0, MPI_COMM_WORLD);
        free(arr_process_file_task);
        free(arr_disps);
        free(num_file_task_for_process_arr);      
        ht = malloc(sizeof(table_entry) * TABLE_SIZE);
        initialize_hash_table(ht);
        count_words(ht, file_task_to_perform, num_of_task_to_perform);      
        free(file_task_to_perform);

        reduce_tables(my_rank, number_of_processes, ht);
        
        word_elem * sorted_list_of_words;
        sorted_list_of_words = create_sorted_list_of_words(ht);
        if(argc >= 3)
            print_csv_file(argv[2], sorted_list_of_words);
        else
            print_csv_file(PATH_DEFAULT_CSV_FILE, sorted_list_of_words);
        end_time = MPI_Wtime();
        fprintf(stderr, "\n--------------tempo con %d processi: %f\n", number_of_processes, end_time - start_time);

        FILE * file_times_results;
        file_times_results = fopen(PATH_TIMES_FILE, "a");
        if(file_times_results == NULL){
            fprintf(stderr, "error opening file %s", PATH_TIMES_FILE);
        }
        fprintf(file_times_results, "time with %d processes: %f\n", number_of_processes, end_time - start_time);
        fclose(file_times_results);

    }
    else{
        MPI_Scatter(NULL, 1, MPI_INT, &num_of_task_to_perform, 1, MPI_INT, 0, MPI_COMM_WORLD);
        file_task_to_perform = malloc(sizeof(process_file_task) * num_of_task_to_perform);
        MPI_Scatterv(NULL, NULL, NULL, process_file_task_type, file_task_to_perform, num_of_task_to_perform, process_file_task_type, 0, MPI_COMM_WORLD);

        ht = malloc(sizeof(table_entry) * TABLE_SIZE);
        initialize_hash_table(ht);
        count_words(ht, file_task_to_perform, num_of_task_to_perform);

        free(file_task_to_perform);

        reduce_tables(my_rank, number_of_processes, ht);

    }

    MPI_Finalize();
    
    return 0;
}