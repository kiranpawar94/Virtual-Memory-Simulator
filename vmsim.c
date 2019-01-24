/*Project3: Virtual Memory Simulator
*
*Author: Kiran Pawar
*
*CS1550 Operating Systems
*
*Simulates page replacement algorithms
*****************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

//Debugs for each algorithm
#define DEBUG 0		//Fifo debug
#define NRUDEBUG 0	//NRU debug
#define CLKDEBUG 0	//Clock debug
#define OPTDEBUG 0	//Optimal debug

//Global Variables
FILE *trace;
int page_faults;
int frames;
char file;
int node_count = 0;

#define LINE_SZ 11	//Size per line in bytes

struct Node{
	int val;
	struct Node* next;
};

struct trace_file_node{		//Node that holds what is read from the trace file
	unsigned int address;
	char mode;	
	unsigned int ranking;	//Only used in optimal, used to determine page with longest wait until reference
};

//Node for opt rank determination
struct rank_node{
	int address;
	int page;
};

//Linked list that comprises the page table entry
struct page_table_entry_node{		//Page table entry
	unsigned int node_number;	//Position on linked list
	unsigned int page;		//Page that holds the instructions
	unsigned int bit_op;		//Bitwise operator: first bit = dirty bit, second bit = reference bit
	unsigned int ranking;		//Ranking pages for eviction in opt
	struct page_table_entry_node *next;	//Points to next node
	struct page_table_entry_node *prev;	//Ponits to the previous node
};

struct page_table_entry_node *head;	//Head of pte
struct page_table_entry_node *tail;	//Tail of pte


//-------------------------------------------Helper functions

//NRU eviction function
//Evicts based on the bit_op(class) value
void NRU_evict(struct page_table_entry_node *top, struct page_table_entry_node *end, int page_num, int bit_op, int *found_flag){
	struct page_table_entry_node *temp;

	for(temp = top; temp < end; temp = temp->next){

                if(temp->bit_op == bit_op){                                                                //Insert page at the evictee's location
		if(NRUDEBUG)printf("Evicting a bit_op: %d	Node number: %d		Page: %d\n", bit_op, temp->node_number, page_num);
                                                temp->page = page_num;
                                                temp->bit_op = 0;                       //Reset bits to 0
						*found_flag = 1;
						page_faults++;
                                                break;
		}
	}
	return;
}

//Locate next occurance function
//Finds when a particular page occurs again in a tracefile(within a certain frame)
void locate_next_occ(struct trace_file_node *start_mem, struct trace_file_node *end_mem, int page, int *offset){
	int j, k;

	//Tracks how far the next node is
	for(j = 0; start_mem < end_mem; start_mem++, j++){
		if(OPTDEBUG)printf("Comparing page %x to trace %x\n", page, start_mem->address >> 12);
		if(page == (start_mem->address >> 12)){			//check for page hit
		//	if(OPTDEBUG)printf("Found page hit %x	at address %d\n", start_mem->address >>12, j);
			break;
		}
		else{
		//	if(OPTDEBUG)printf("Not found page%x	Offset:%d\n", page, j);
		}
	}
	*offset = j;
	return;

}

//Optimal ranking function
void setup_opt_rank(struct trace_file_node *mem, int max_lines, int num_frames){
	int j, k;
	int position;
	int offset = 0;
	int page = 0;

	//Load one set at a time the size of num_frames from trace file
	for(j = 0; j < max_lines; j++){
		//Get page at current address (j) for each frame specified
			//Load here
			locate_next_occ(&mem[j+1], &mem[max_lines-1], (mem[j].address>>12 ), &offset);
			mem[j].ranking = offset;
	}
	if(OPTDEBUG){
		for(j = 0; j < max_lines; j++){
			printf("Trace file node number %d		Page %x		Ranking %d\n", j, mem[j].address >>12, mem[j].ranking);
		}
	}
}
///  12345678  01238012     1[0].
//----------------------------------ALGORITHMS-------------------------------------------//
//------------------------------------------------------Opt algorithm
/*
*The optimal algorithm will determine through foresight how soon a page will be referenced and, using this knowledge, determine whether a page should be evicted or not
*This was accomplished by slicing the tracefile into framebuffer sized chunks and ranking each page on how soon it will be referenced
*Using this, the algorithm will then evict the page with the lowest rank (higher rank number = longer it will take to be referenced)
*/
void opt_algorithm(struct trace_file_node *mem, struct page_table_entry_node *top, struct page_table_entry_node *end, int max_lines, int frames){

	int page = 0;
	int page_hit = 0;
	int dirty_pages = 0;
	int num_frames = frames;
	int mem_acc = 0;
	int flag = 0;

	struct page_table_entry_node *temp;
	struct page_table_entry_node *mal_ptr;
	struct page_table_entry_node *max_rank;

	int j, k;

	setup_opt_rank(mem, max_lines, num_frames);

	for(j = 0; j < max_lines; j++, mem++, mem_acc++){
		//Scan through the file, store each important part of the memory address into mem
		page = mem->address >> 12;
		flag = 0;			//Flag that determines whether the appropriate action has been taken or not
		if(mem->mode == 'W'){
			dirty_pages++;		//If the mode of the address is 'W' (write to disk), increment a counter for it
		}

		for(temp = top; temp < end; temp = temp->next){
			if(temp->page == page){				//Loop through frame buffer
				/*if(OPTDEBUG)*/printf("PAGE HIT %x		Node: %d\n", page, temp->node_number);
				page_hit++;
				flag = 1;				//Set flag so no other action is taken for this address
				break;		
			}
			if(temp->page == 0 && flag == 0){
				/*if(OPTDEBUG)*/printf("Page fault no eviction %x\n", page);
				temp->page = page;		//Since the frame buffer is currently empty at this position, we simply need to chage the address and rank values
				temp->ranking = mem->ranking;
				page_faults++;
				flag = 1;
				break;
			}
		}	//End linkedlist for loop

		//Opt algorithm

		if(flag == 0){
			max_rank = top;
			for(temp = top; temp < end; temp = temp->next){		//Search for page with max ranking and evict it
				if(temp->ranking > max_rank->ranking){
					max_rank = temp;
				}
			}
                        for(temp = top; temp < end; temp = temp->next){         //Search for$
				temp->ranking--;
                        }

			//Evict whichever page max rank is pointing to
			printf("Page fault with eviction\n");
		//	/*if(OPTDEBUG)*/printf("Evicting page %x of rank %d	with page %x Node number %d\n", max_rank->page, max_rank->ranking, page, max_rank->node_number);
			max_rank->page = page;
			max_rank->ranking = mem->ranking;
			page_faults++;
			flag = 1;
		}
	}	//End j loop

	//Return algorithm stats
	printf("\n");
	printf("Algorithm: Optimal\n");
	if(OPTDEBUG)printf("Page hit: %d\n",page_hit);
	printf("Number of frames %d\n", num_frames);
	printf("Total mem accesses: %d\n", mem_acc);
	printf("Total page faults: %d\n", page_faults);
	printf("Total writes to disk: %d\n", dirty_pages);
	printf("\n");
}

//-------------------------------------------------------------Clock algorithm
/*
*Clock will have a pointer that runs through the frame buffer in a circular fashion
*Each page is also has a reference bit determining whether or not it is eligible for eviction when the clock pointer passes over it
*If the reference bit for the page is 0, it is evicted and the clock pointer stops moving
*If the reference bit for the page is 1, the clock pointer sets it to 0 and it continues searching for a page with a reference bit of 0
*Each page starts with a reference bit when it is inserted, and the only way that reference bit can be set to 1 is if it is referenced
*/
void clock_algorithm(struct trace_file_node *mem, struct page_table_entry_node *top, struct page_table_entry_node *end, int max_lines, int frames){
	int page = 0;
	page_faults = 0;
	int num_frames = frames;
	int mem_acc = 0;
	int dirty_pages = 0;
	int flag = 0;

	struct page_table_entry_node *temp;
	struct page_table_entry_node *clock_ptr;
	struct page_table_entry_node *mal_ptr;

	int j;

	end->next = top;			//Make linkedlist circular
	clock_ptr = top;			//Start clock pointer at head

	for(j = 0; j < max_lines; j++, mem++, mem_acc++){
		
		flag = 0;	//Initialize/reset flag
		page = mem->address >> 12;

		if(mem->mode == 'W'){
			dirty_pages++;
		}
		for(temp = top; temp < end; temp = temp->next){
			if(temp->page == page){
				/*if(CLKDEBUG)*/printf("PAGE HIT %x\n", page);
				temp->bit_op = 1;	//Set ref bit to 1
				flag = 1;		//Page found
				break;
			}
			if(temp->page == 0 && flag == 0){
				flag = 1;
				/*if(CLKDEBUG)*/printf("Page fault no eviction %x\n", page);
				temp->page = page;
				page_faults++;
				break;
			}
		}

		while(flag != 1){			//No page hit, no page fault without eviction
			if(clock_ptr->bit_op == 0){
				//Evict page
				printf("Page fault with eviction\n");
				if(CLKDEBUG)printf("Evicting page %x	Node %d\n", clock_ptr->page, clock_ptr->node_number);
				clock_ptr->page = page;
				clock_ptr = clock_ptr->next;
				page_faults++;
				break;
			}	//End bit_op == 0 if statement
	
			else if(clock_ptr->bit_op == 1){
				clock_ptr->bit_op = 0;
				clock_ptr = clock_ptr->next;
			}
			else{
				//This should never happen
				printf("Error: invalid bit_op in clock : %d\n", clock_ptr->bit_op);
				printf("Aborting program\n");
				exit(0);
			}
		}	//End of while loop


	}	//End of j loop
	//Return algorithm stats
	printf("\n");
	printf("Algorithm: Clock\n");
	printf("Number of frames %d\n", num_frames);
	printf("Total mem accesses: %d\n", mem_acc);
	printf("Total page faults: %d\n", page_faults);
	printf("Total writes to disk: %d\n", dirty_pages);
	printf("\n");
}

//-----------------------------------------------------------FIFO algorithm
/*
*Fifo simply evicts the page at the beginning of the frame buffer and inserts at the end of it
*/
void fifo_algorithm(struct trace_file_node *mem, struct page_table_entry_node *top, struct page_table_entry_node *end, int max_lines, int frames){
	int page = 0;
	int write_to_disk = 0;
	page_faults = 0;	//Total number of page faults
	int num_frames = frames;		//Total number of frames
	int mem_acc = 0;		//Total number of memory accesses
	int dirty_pages = 0;	//Total number of dirty pages
	int flag = 0;		//Flag that indicates when a page is found

	struct page_table_entry_node *temp;
	struct page_table_entry_node *mal_ptr;

	int j;

	for(j = 0; j < max_lines; j++, mem++, mem_acc++){		//Trace file
		flag = 0;
		page = mem->address >> 12;	//Shift through mem to desired 20 bits

		if(mem->mode == 'W'){
			dirty_pages++;
		}
                if(DEBUG)printf("Head = %d	Tail = %d\n", top, end);
		for(temp = top; temp < end; temp = temp->next){
			//Check if page field is zero

			if(temp->page == page){
				//Check if page already exists within frame buffer
			//	/*if(DEBUG)*/printf("PAGE HIT\n");
				flag = 1;
				break;		//Return without page fault
			}

			else if(temp->page == 0 && flag == 0){
				//First check if the page exists anywhere in the buffer
			//	/*if(DEBUG)printf*/("Page fault with no eviction\n");
				//Print inserted page
				if(DEBUG)printf("Update node # %d\n", temp->node_number);
				temp->page = page;
				flag = 1;
				page_faults++;
				break;
			} 
		} /* end of for linklsit */

		if(flag == 0){
		//Do fifo algorithm		
			temp = top;
			top = top->next;
		//	/*if(DEBUG)*/printf("Page fault with eviction\n");
			if(DEBUG)printf("Deleting top node_number: %d\n", temp->node_number);
			free(temp);
			//Insert page at tail
			mal_ptr = malloc(sizeof(struct page_table_entry_node));
			end->next = mal_ptr;
			end = end->next;
			end->next = NULL;

			end->page = page;
			node_count++;
			temp->node_number = node_count;
			if(DEBUG)printf("Inserted into node # %d\n", temp->node_number);
			//printf("Inserting node at end: %d\n", end->page);
			page_faults++;
			//printf("Page %d\n", temp->page);
		}

	} /* end of for trace file loop*/

	//Return algorithm stats
	printf("\n");
	printf("Algorithm: FIFO\n");
	printf("Number of frames %d\n", num_frames);
	printf("Total mem accesses: %d\n", mem_acc);
	printf("Total page faults: %d\n", page_faults);
	printf("Total writes to disk: %d\n", dirty_pages);
	printf("\n");
}

//----------------------------------------------------NRU algorithm 
/*
*NRU(not recently used) will set values for each page depending on whether or not it's referenced and whether or not it's dirty
*This is accomplished by assigning each page with two bits, one for if it's referenced and one for if it's dirty using the bit_op variable
*Depeding on the bit_op value, a page's likelihood for eviction is determined
*0 - not referenced, not dirty
*1 - not referenced, dirty
*2 - referenced, not dirty
*3 - referenced, dirty
*The reference bit for all pages in the frame buffer is reset to 0 periodically depending on what the refresh value is set to when the program is run
*The dirty bit will remain unchanged until the page is evicted and a new page is set
*/
void nru_algorithm(struct trace_file_node *mem, struct page_table_entry_node *top, struct page_table_entry_node *end, int max_lines, int frames, int refresh){
	int page = 0;
	int write_to_disk = 0;
	page_faults = 0;
	int num_frames = frames;
	int mem_acc = 0;
	int dirty_pages = 0;
	int found_flag = 0;
	int refresh_counter = 0;			//Resets ref bits after a specified period

	struct page_table_entry_node *temp;
	struct page_table_entry_node *mal_ptr;
	
	int j, k;

	for(j = 0; j < max_lines; j++, mem++, mem_acc++){
		found_flag = 0;
		page = mem->address >> 12;

		if(mem->mode == 'W'){
			dirty_pages++;
		}

		//Check refresh and reset all reference bits when the period is met

//           if(NRUDEBUG)printf("Top: %d		End: %d\n", top, end);
	
		//Refreshing bits after specified references to memory
		if(mem_acc%refresh == 0){
			if(NRUDEBUG)printf(">>>>>>>>>>>>>>>>>>>>>>>Refreshing bit at %d\n", mem_acc%refresh);
			for(temp = top; temp < end; temp = temp->next){
				temp->bit_op = temp->bit_op & 1;
			}
		}

	   for(temp = top ;  temp < end; temp = temp->next){
//		if(NRUDEBUG)printf("Temp: %d	temp->next:%d	top: %d		top->next: %d end :%d \n", temp, temp->next, top, top->next, end);
		//Check if page already exists
		if(temp->page == page){
			/*if(NRUDEBUG)*/printf("PAGE HIT	Node number: %d\n", temp->node_number);
			if(mem->mode == 'W'){
				temp->bit_op = temp->bit_op | (1<<0); //Set dirty $
				if(NRUDEBUG)printf("Bit op after write to disk: %d	Node number: %d		Page: %d\n", temp->bit_op, temp->node_number, page);
                         }

                         temp->bit_op = temp->bit_op | (1<<1);           //Set ref bit
			if(NRUDEBUG)printf("Bit op after page hit: %d	Node number: %d		Page:  %d\n", temp->bit_op, temp->node_number, page);
			found_flag = 1;
                         break;
		}
		else if(temp->page == 0){
			/*if(NRUDEBUG)*/printf("Page fault no eviction	Node number: %d		Page: %d\n", temp->node_number, page);
			temp->page = page;
			if(mem->mode == 'W'){
				temp->bit_op = temp->bit_op | (1<<0);   //Set dirty $
				if(NRUDEBUG)printf("Bit op after write to disk: %d	Node number: %d\n", temp->bit_op, temp->node_number);
			}

			found_flag = 1;
			page_faults++;
			break;
		}
           }		//End of linked list loop
	//Run NRU

	//Replace bit op 0 pages
	if(found_flag == 0){
		NRU_evict(top, end, page, 0, &found_flag);
		printf("Page fault, not referenced, clean\n");
	}

		//Replace bit op 1 pages
	if(found_flag == 0){
		NRU_evict(top, end, page, 1, &found_flag);
		printf("Page fault, not referenced, dirty\n");
	}

		//Replace bit op 2 pages
	if(found_flag == 0){
		NRU_evict(top, end, page, 2, &found_flag);
		printf("Page fault, referenced, clean\n");
	}

		//Replace bit op 3 pages
	if(found_flag == 0){
		NRU_evict(top, end, page, 3, &found_flag);
		printf("Page fault, referenced, dirty\n");
	}

       }		//End of j loop
	//Return algorithm stats
	printf("\n");
	printf("Algorithm: NRU\n");
	printf("Number of frames %d\n", num_frames);
	printf("Total mem accesses: %d\n", mem_acc);
	printf("Total page faults: %d\n", page_faults);
	printf("Total writes to disk: %d\n", dirty_pages);
	printf("\n");
}		//End of NRU function

//----------------------------------------------MAIN
int main(int argc, char *argv[]){

	unsigned long addr = 0;
	char mode;

	struct trace_file_node *tracef_ptr;		//Points to the beginning of the memory that stores the trace file
	struct page_table_entry_node *pte_ptr;		//Page table entry pointer

	int i = 0;
	int j = 0;
	int numframes = 0;
	int refresh = 0;
	char output[50];
	char algorithm[5];
	char tracef_name[20];
	FILE *fptr;
	int file_size;
	int max_num_lines;	//Total number of lines in trace file
	int max_num_addr;	//Total number of addresses in the trace file

	struct stat file_stat;	//Info about file size
	struct page_table_entry_node *temp;

	for(i = 0; i < argc; i++){
		//Command line arguments
		if (argc > 8 || argc < 6){
			printf("Invalid number of args (%d), usage: ./vmsim –n <numframes> -a <opt|clock|fifo|nru> [-r <refresh>] <tracefile>\n", argc);
			exit(0);
		}

		if(strcmp(argv[i], "-n") == 0){
			numframes = atoi(argv[i + 1]);	//Convert string to int for number of frames
			i++;

			if(numframes < 1){
				printf("Error, invalid number of frames, should be 1 or more\n");
				exit(0);
			}
		}

		else if(strcmp(argv[i], "-a") == 0){
			//Pick algorithm
			strcpy(algorithm, argv[i+1]);	//Copy algorithm into buffer

		}

		else if(strcmp(argv[i], "-r") == 0){
			//Refresh
			refresh = atoi(argv[i + 1]);
			i++;
		}
		else{
			//Get trace file
			strcpy(tracef_name, argv[i]);
		}
	}

	fptr = fopen(tracef_name, "r");	//Open file for reading

	if(fptr == NULL){
		printf("Unable to open file specified: %s\n", tracef_name);
		exit(1);
	}

	//Get size of file/lines/line size = number of struct trace_file_node
	stat(tracef_name, &file_stat);

	file_size = file_stat.st_size;	//Got size of file in bytes

	max_num_lines = file_size/LINE_SZ;

	//Malloc a chunk of memory the size of the file

	tracef_ptr = malloc(file_size * sizeof(struct trace_file_node));

	printf("Numframes = %d, Algorithm = %s, Refresh = %d, Trace File = %s\n",
					 numframes, algorithm, refresh, tracef_name);

	//Create page table based on the number of frames
        head = malloc(sizeof(struct page_table_entry_node));	//First node of the linked list
	tail = head;
	tail->page = 0;
	tail->bit_op = 0;

	head->next = NULL;

	//Data structure that holds frames
	for(i = 0; i < numframes; i++){
		pte_ptr = malloc(sizeof(struct page_table_entry_node));
		pte_ptr->page = 0;
		pte_ptr->node_number = i + 1;
		tail->next = pte_ptr;
		tail = tail->next;
		tail->next = NULL;

	}

	node_count = numframes;

	if(DEBUG)printf("Node_count = %d	numframes = %d\n", node_count, numframes);


	//Read from file and store into mem to prevent thrashing

	j = 0;
	while(fscanf(fptr, "%x %c", &addr, &mode) != EOF && j < max_num_lines){
		tracef_ptr[j].address = addr;
		tracef_ptr[j].mode = mode;
		tracef_ptr[j].ranking = max_num_lines;			//For opt
		j++;
		//Read through the file and store in buffer
	}

//	for(j = 0; j < max_num_lines; j++){
//	 	printf("%0x %c\n", tracef_ptr[j].address, tracef_ptr[j].mode); //Print each line of the file
//	}

	if(strcmp(algorithm, "fifo") == 0){
		//Call fifo
		fifo_algorithm(tracef_ptr, head, tail, max_num_lines, numframes);
		temp = head;
//		printf("Final linkedlist\n");
//		for(temp; temp->next <= tail; temp = temp->next){
//			printf("%x\n", temp->page);
//		}
	}
	else if(strcmp(algorithm, "nru") == 0){
		nru_algorithm(tracef_ptr, head, tail, max_num_lines, numframes, refresh);
	}

	else if(strcmp(algorithm, "clock") == 0){
		clock_algorithm(tracef_ptr, head, tail, max_num_lines, numframes);
	}
	else if(strcmp(algorithm, "opt") == 0){
		opt_algorithm(tracef_ptr, head, tail, max_num_lines, numframes);
	}
	
/*	temp = head;
	for(temp; temp->next <= tail; temp = temp->next){
		printf("%x\n", temp->page);
	}*/
	return 0;
}
