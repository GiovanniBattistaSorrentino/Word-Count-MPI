# Word Count MPI
# Giovanni Battista Sorrentino

# Indice

1. Breve descrizione del problema
2. Soluzione proposta
    1. Divisione del carico di lavoro
    2. Conteggio delle frequenze
    3. Raccolta dei dati
4. Note sulla soluzione
5. Correttezza della soluzione proposta
6. Istruzioni per l'esecuzione
7. Benchmark
    1. Scalabilità forte
    2. Scalabilità debole
    3. Analisi dei risultati
8. Conclusioni

# 1. Breve descrizione del problema
Il problema di word count consiste nel conteggio delle parole presenti in uno o più file di testo. Questo è un problema che trova applicazione in molti contesti come quello del giornalismo, l'ambito pubblicitario o nell'ambito delle traduzioni di testi per determinare il costo del lavoro.


Il seguente progetto propone di risolvere il problema del conteggio delle occorrenze delle parole contenute nei file presenti in una directory utilizzando un approccio distribuito mediante l'uso di <strong>MPI</strong>.

# 2. Soluzione proposta
La soluzione comprende tre step principali: divisione del carico di lavoro, conteggio delle frequenze e raccolta dei dati con stampa dei risultati in un file CSV.

## 2.1 Divisione del carico di lavoro
Per la divisione del carico è stato deciso di utilizzare la dimensione dei file in input, dato che una suddivisione per file avrebbe comportato una distribuzione non omogenea del carico dato che nella cartella i file possono avere dimensioni diverse. Sono state utilizzate due struct per effettuare la suddivisione del carico di lavoro: <em>file_info</em> e <em>process_file_task</em>.

```
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

```
Il nodo master quindi tramite la funzione <em>get_array_file_info</em> crea l'array contenente le informazioni relative ai file presenti nella cartella e restituisce la somma delle dimensioni (in byte) dei file, tramite questa somma viene calcolata la quantità di byte che dovranno elaborare i processi, dividendo la somma delle dimensioni dei file per il numero di processi in esecuzione. Successivamente viene invocata la funzione <em>get_array_process_file_task</em> che restituisce un array di <em>process_file_task</em>, ogni elemento rappresenta un intero file o una porzione di file che dovrà essere elaborata da un processo. Per la quantità di byte rimanente dalla distribuzione del carico è stato deciso di assegnare ulteriori <em>process_file_task</em> con una quantità di byte equivalente al numero rimanente di byte da assegnare diviso il numero di processi, con una soglia minima dato che non avrebbe avuto senso assegare uno o due byte ad un processo. 

```
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
```


Quindi il nodo master effettua due comunicazioni: una <strong>MPI_Scatter</strong> per inviare ai processi il numero di <em>process_file_task</em> che riceveranno ed una una <strong>MPI_Scatterv</strong> per inviare i <em>process_file_task</em> che i processi dovranno elaborare.



```
MPI_Scatter(num_file_task_for_process_arr, 1, MPI_INT, &num_of_task_to_perform, 1, MPI_INT, 0, MPI_COMM_WORLD);
file_task_to_perform = malloc(sizeof(process_file_task) * num_of_task_to_perform);
MPI_Scatterv(arr_process_file_task, num_file_task_for_process_arr, arr_disps, process_file_task_type, file_task_to_perform, num_of_task_to_perform, process_file_task_type, 0, MPI_COMM_WORLD);
```

## 2.2 Conteggio delle frequenze
Dopo che sono state eseguite le scatter ogni processo possiederà le informazioni relative alle porzioni di file su cui dovrà effettuare il conteggio delle frequenze delle parole. Una delle problematiche riscontrate in questa fase è la presenza di parole che potrebbero stare a cavallo tra due assegnazioni del carico di lavoro, è stato quindi deciso che queste parole vengano contate solo dal primo processo. Per fare ciò sono state implementate le funzioni <em>calculate_effective_initial_seek</em> e <em>calculate_effective_final_seek</em> che calcolano le posizione effettive, iniziali e finali, del puntatore del file dentro cui il processo dovrà effettuare il conteggio delle parole.


È stato deciso di utilizzare come stuttura dati per la memorizzazione del numero di occorrenze delle parole l'hash table dato che questa struttura permette l'accesso e l'aggiornamento dei dati in maniera molto rapida, inoltre i conflitti sulle chiavi sono stati risolti utilizzando le liste. Inoltre nel conteggio delle parole non sono state fatte differenze tra parole scritte in maiuscolo o in minuscolo. 


```
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
```



## 2.3 Raccolta dei dati
Una volta effettuato il conteggio delle parole ogni processo avrà nella propria hash table le informazioni relative alle parole e al numero di occorrenze di queste, il nodo master dovrà quindi ricevere queste informazioni e calcolare il numero di occorrenze totali di ogni parola. Per eseguire questo calcolo è stato deciso di non utilizzare la <strong>MPI_Reduce</strong> dato che nessuna delle operazioni offerte da MPI per questa funzione era adatta al problema in questione; è stato quindi deciso di creare una propria funzione che effettuasse le somme delle frequenze delle parole. Al fine di rendere il codice più parallelo possibile la funzione effettua delle <strong>MPI_Send</strong> e <strong>MPI_Recv</strong> tra i processi seguendo la struttura di un'albero binario con radice il nodo master.


Alla invocazione di questa funzione (chiamata <em>reduce_tables</em>) ogni nodo calcola il rank dei processi che sono figli nell'albero binario e se questi processi sono presenti viene eseguita una <strong>MPI_Recv</strong> dal figlio sinistro per ricevere il numero di elementi presenti nell'array di caratteri che riceverà e successivamente un'ulteriore <strong>MPI_Recv</strong> per ricevere l'array di caratteri che contiene le informazioni sulle parole e sulle frequenze contate dal nodo, vengono quindi sommate le frequenze presenti nell'hash table locale e successivamente queste operazioni vengono effettuate sul figlio destro (se presente). Dopo che sono state effettuate queste operazioni il nodo (se non è il master) calcola il rank del nodo padre e invia a questo tramite una <strong>MPI_Send</strong> il numero di elementi che sono presenti nell'array di char da inviare e successivamente tramite un'altra <strong>MPI_Send</strong> l'array di char contiene le informazioni sulle parole e sulle frequenze presenti nella hash table locale.

Le informazioni presenti nell'hash table sono state inviate inserendo queste informazioni in un array di char, questa scelta è dettata dal fatto che non sarebbe stato possibile inviare l'hash table dato che questa struttura dati non è allocata in maniera contigua in memoria. L'array di char è formato da coppie "parola-frequenza" separate dal carattere '\0', mentre le parole sono separate dalle frequenze usando il carattere ':'. È stato deciso di implementare la comunicazione in questo modo e di non utilizzare struct perchè questo avrebbe richiesto che la dimensione relativa al numero di caratteri della parola fosse definito a priori, il che avrebbe comportato uno spreco di dati nella comunicazione dato che parole piccole come "il" o "con" avrebbero comunque occupato la dimensione definita a priori per tutte le parole.

```
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
```

Concusa la esecuzione della funzione <em>reduce_tables</em> il nodo master avrà nella propria hash table le occorrenze di tutte le parole presenti nella directory in input, manca quindi solo l'ordinamento delle parole e la stampa nel file CSV. Per effettuare l'ordinamento delle parole è stato deciso di utilizzare una lista in cui vengono effettuati inserimenti in maniera ordinata per il numero di frequenze e come secondo criterio l'ordine alfabetico delle parole. Dopo l'ordinamento il master scrivere il file CSV con i risultati.

# 3. Note sulla soluzione
L'implementazione prevede la semplificazione per cui nei file non possono essere presenti parole con caratteri non appartenenti alla codifica ASCII, pena il non corretto conteggio di queste parole. 


# 4. Correttezza
La corretteza della soluzione è stata testata mediante l'esecuzione di diversi test case semplici in cui è stato variato il numero di processi, verificando che il risultato era sempre lo stesso indipendentemente dal numero di processi. La cartella "correttezza" contenente il materiale per eseguire un semplice test.

# 5. Istruzioni per l'esecuzione
Per una generica esecuzione nel docker fornito basterà eseguire i seguenti comandi:
```
mpicc word-count.c -o word-count.out
mpirun --allow-run-as-root -np num_pro word-count.out path_dir_input path_risultati.csv
```

Dove "path_dir_input" è il path della cartella contenente i file su cui si vuole effettuare il conteggio della parole, "num_pro" è il numero di processi con cui si vuole eseguire il codice mentre "path_risultati.csv" è il path del file in cui verranno scritte le parole con le relative frequenze.


Per eseguire il codice relativo alla corretteza spostarsi nella relativa cartella tramite il comando:
```
cd correttezza
```
ed eseguire l'apposito script bash tramite il comando
```
sh correttezza.sh
```

# 6. Benchmark
Nella fase di benchmark è stato testato il comportamento della soluzione sia in termini di scalabilità debole sia in termini di scalabilità forte. Per la scalabilità debole viene testata la soluzione dando input di dimensione costante per ogni processore mentre per la scalabilità forte viene fissata la dimensione dell'input e viene variato il numero di processi.

Nel nostro problema la dimensione dell'input è relativa alla quantità di byte presenti nei file nella cartella di input. Per eseguire i benchmark sono stati utilizzati dei libri in formato .txt ripetuti più volte nelle cartelle di input.

Dimensioni dei file ripetuti:

Libro 1: 1.2MB</br>
Libro 2: 2.0MB</br>
Libro 3: 861KB</br>
Libro 4: 389KB</br>
Libro 5: 1012KB</br>
Libro 6: 1.0MB</br>
Libro 7: 1.3MB</br>
Libro 8: 2.0MB</br>
Libro 9: 2.6MB</br>
Libro 10: 1.1MB</br>
Libro 11: 4.3MB</br>
Libro 12: 694KB</br>
Libro 13: 1.2MB</br>
Libro 14: 356KB</br>
Libro 15: 3.2MB</br>

Per la creazione del cluster sono state utilizzate 6 istanze di nodi e2-standard-4(Google cloud, serie E2) aventi ognuna 4vCPU e 16GB di RAM. Per la scalabilità forte sono stati effettuati i benchmark per due diverse taglie di input: 680MB e 1.4GB, mentre per la scalabilità debole il workload per processore è stato fissato a 23MB.


## 6.1 Scalabilità forte


<h3>Taglia input: 680MB</h3>


| # Processi | Tempo(s) | Speedup |
|-------------|-----------|---------|
| 1           | 40.001326 | -       |
| 2           | 27.823263 | 1.43    |
| 3           | 28.603063 | 1.39    |
| 4           | 26.578741 | 1.50    |
| 5           | 23.325608 | 1.71    |
| 6           | 24.311971 | 1.64    |
| 7           | 22.723440 | 1.76    |
| 8           | 22.179606 | 1.80    |
| 9           | 20.749958 | 1.92    |
| 10          | 21.075522 | 1.89    |
| 11          | 22.483402 | 1.77    |
| 12          | 20.612830 | 1.94    |
| 13          | 21.070671 | 1.89    |
| 14          | 21.416847 | 1.86    |
| 15          | 19.918035 | 2.00    |
| 16          | 20.220172 | 1.97    |
| 17          | 19.433683 | 2.05    |
| 18          | 20.052202 | 1.99    |
| 19          | 19.750273 | 2.02    |
| 20          | 19.234885 | 2.07    |
| 21          | 20.547704 | 1.94    |
| 22          | 19.904991 | 2.00    |
| 23          | 19.451942 | 2.05    |
| 24          | 18.689850 | 2.14    |

<img src="img/chart_scal_forte30.png" align="center">

<h3>Taglia input: 1.4GB</h3>


| # Processi | Tempo(s) | Speedup |
|----------|-----------|---------|
| 1        | 70.368636 | -       |
| 2        | 40.279933 | 1.74    |
| 3        | 36.564464 | 1.92    |
| 4        | 36.586580 | 1.92    |
| 5        | 31.192589 | 2.25    |
| 6        | 29.643566 | 2.37    |
| 7        | 30.191607 | 2.33    |
| 8        | 27.810646 | 2.53    |
| 9        | 25.409182 | 2.76    |
| 10       | 25.902050 | 2.71    |
| 11       | 24.153868 | 2.91    |
| 12       | 23.547325 | 2.98    |
| 13       | 23.723869 | 2.96    |
| 14       | 23.672927 | 2.97    |
| 15       | 22.664826 | 3.10    |
| 16       | 26.685242 | 2.63    |
| 17       | 23.292631 | 3.02    |
| 18       | 22.197088 | 3.17    |
| 19       | 21.393068 | 3.28    |
| 20       | 20.855140 | 3.37    |
| 21       | 20.881139 | 3.36    |
| 22       | 22.348183 | 3.14    |
| 23       | 21.861920 | 3.21    |
| 24       | 21.063792 | 3.34    |

<img src="img/chart_scal_forte60.png" align="center">

## 6.2 Scalabilità debole



| # Processi | Dimensione cartella | Tempo(s) |
|----------|-------|-----------|
| 1        | 23MB  | 17.766565 |
| 2        | 46MB  | 17.387945 |
| 3        | 68MB  | 18.413572 |
| 4        | 91MB  | 20.682183 |
| 5        | 114MB | 18.325722 |
| 6        | 136MB | 19.888343 |
| 7        | 159MB | 18.291017 |
| 8        | 182MB | 18.165009 |
| 9        | 204MB | 18.542102 |
| 10       | 227MB | 18.345816 |
| 11       | 250MB | 18.469738 |
| 12       | 272MB | 18.328443 |
| 13       | 295MB | 18.884554 |
| 14       | 317MB | 19.331179 |
| 15       | 340MB | 19.100347 |
| 16       | 363MB | 21.526364 |
| 17       | 385MB | 17.935599 |
| 18       | 408MB | 20.076173 |
| 19       | 431MB | 19.155655 |
| 20       | 453MB | 19.267358 |
| 21       | 476MB | 20.899231 |
| 22       | 499MB | 20.532396 |
| 23       | 521MB | 19.678615 |
| 24       | 544MB | 19.071691 |


<img src="img/chart_scal_deb.png" align="center">

## 6.3 Analisi dei risultati
Dai risultati raccolti è possibile notare che l'algoritmo trae vantaggio della parallelizzazione, tuttavia la crescita dello speedup è limitata da diverse criticità, prima delle quali l'ordinamento delle parole che si sarebbe potuto eseguire, in parte o completamente, in parallelo. Un'ulteriore criticità che ha compromesso la crescita dello speedup è la scelta implementativa di inviare ai nodi worker la lista di sezioni di file da computare, dato che la lunghezza di questa lista cresce al crescere del numero di file presenti nella cartella, questa potrebbe essere molto lunga facendo costare molto la comunicazione in termini prestazionali. Per evitare questa criticità sarebbe bastato inviare solo un riferimento indicante dove iniziare il conteggio delle parole e un riferimento indicate dove terminare il conteggio delle parole, computando i file in ordine alfabetico; in questo modo non sarebbe stata necessaria la esecuzione della <strong>MPI_Scatter</strong> inviante il numero di task successivamente ricevuti tramite la <strong>MPI_Scatterv</strong>. Nonostante queste criticità l'analisi della scalabilità debole non ha mostrato importati incrementi dei tempi di esecuzione al crescere del numero di processi in esecuzione.


# 7. Conclusioni
L'analisi del problema e dei risultati dei benchmark ci fanno concludere che il problema di word count giova di un approccio distribuito soprattutto per input di grandi dimensioni, tuttavia non è possibile ottenere speedup molto elevati data la presenza di porzioni di codice non completamente parallelizzabili come quella relativa alla raccolta delle frequenze delle parole.










