/*
 * Skeleton code for CSC 360, Spring 2024,  Assignment #4
 *
 * Prepared by: Michael Zastre (University of Victoria) 2024
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/*
 * Some compile-time constants.
 */

#define REPLACE_NONE 0
#define REPLACE_FIFO 1
#define REPLACE_LRU  2
#define REPLACE_CLOCK 3
#define REPLACE_OPTIMAL 4


#define TRUE 1
#define FALSE 0
#define PROGRESS_BAR_WIDTH 60
#define MAX_LINE_LEN 100


/*
 * Some function prototypes to keep the compiler happy.
 */
int setup(void);
int teardown(void);
int output_report(void);
long resolve_address(long, int);
void error_resolve_address(long, int);


/*
 * Variables used to keep track of the number of memory-system events
 * that are simulated.
 */
int page_faults = 0;
int mem_refs    = 0;
int swap_outs   = 0;
int swap_ins    = 0;


/*
 * Page-table information. You are permitted to modify this in order to
 * implement schemes such as CLOCK. However, you are not required
 * to do so.
 */
struct page_table_entry *page_table = NULL;
struct page_table_entry {
    long page_num;
    int dirty;
    int free;
    int last_access_time; // counter to keep track of last acces time for a page
    int use_bit; // recently used indicator bit for clock algorithm
};


/*
 * These global variables will be set in the main() function. The default
 * values here are non-sensical, but it is safer to zero out a variable
 * rather than trust to random data that might be stored in it -- this
 * helps with debugging (i.e., eliminates a possible source of randomness
 * in misbehaving programs).
 */

int size_of_frame = 0;  /* power of 2 */
int size_of_memory = 0; /* number of frames */
int page_replacement_scheme = REPLACE_NONE;


// FIFO variables for tracking pages in page table memory
int fifo_front = 0;
int fifo_rear = -1;

// Keep track of clock hand position for clock algorithm
int clock_hand = 0;

/*
 * Function to convert a logical address into its corresponding 
 * physical address. The value returned by this function is the
 * physical address (or -1 if no physical address can exist for
 * the logical address given the current page-allocation state.
 */

long resolve_address(long logical, int memwrite)
{
    int i;
    long page, frame;
    long offset;
    long mask = 0;
    long effective;

    /* Get the page and offset */
    page = (logical >> size_of_frame);

    for (i=0; i < size_of_frame; i++) {
        mask = mask << 1;
        mask |= 1;
    }
    offset = logical & mask;

    /* Find page in the inverted page table. */
    frame = -1;
    for ( i = 0; i < size_of_memory; i++ ) {
        if (!page_table[i].free && page_table[i].page_num == page) {
            frame = i;
            break;
        }
    }

    /* If frame is not -1, then we can successfully resolve the
     * address and return the result. */
    if (frame != -1) {
        effective = (frame << size_of_frame) | offset;
        return effective;
    }


    /* If we reach this point, there was a page fault. Find
     * a free frame. */
    page_faults++;

    // Basic iteration to fill empty page_table before needing a replacement algorithm
    for ( i = 0; i < size_of_memory; i++) {
        if (page_table[i].free) {
            frame = i;
            page_table[frame].page_num = page;
            page_table[frame].free = FALSE;
            page_table[frame].dirty = memwrite;
            swap_ins++;
            effective = (frame << size_of_frame) | offset;
            return effective;
        }
    }

    /* If we got here that means the page table is full
     * and the page was not found in the table
     * so we must swap something out of the page 
     * table to swap in the current page
     */
    
    // Handle page fault with FIFO algorithm
    if (page_replacement_scheme == REPLACE_FIFO) {
        // Swap out the front page if table full
        frame = fifo_front;
        fifo_front = (fifo_front + 1) % size_of_memory;
        page_table[frame].free = TRUE;
        // Write to memory if page is dirty
        if (page_table[frame].dirty) {
            swap_outs++;
        }

        // Load the new page into frame
        fifo_rear = (fifo_rear + 1) % size_of_memory; // update rear fifo value
        frame = fifo_rear; 
        page_table[frame].page_num = page; 
        page_table[frame].free = FALSE;
        page_table[frame].dirty = memwrite;
        swap_ins++;
        effective = (frame << size_of_frame) | offset; // Return effective address
        return effective;
    }
    // handle page fault with LRU algorithm
    else if (page_replacement_scheme == REPLACE_LRU) {
        int lru_index = -1;
        int oldest_time = MAX_LINE_LEN;
        
        // Find the least recently accessed page in the table
        for (i = 0; i < size_of_memory; i++) {
            if (!page_table[i].free && (page_table[i].last_access_time < oldest_time)) {
                oldest_time = page_table[i].last_access_time; 
                lru_index = i;
            }
        }

        if (lru_index != -1) { // make sure an index was found to swap out
            // Swap out LRU page
            page_table[lru_index].free = TRUE;

            // Write to memory if page is dirty
            if (page_table[lru_index].dirty) {
                swap_outs++;
            }
            
            // Load the new page into frame
            page_table[lru_index].page_num = page;
            page_table[lru_index].free = FALSE;
            page_table[lru_index].last_access_time = 0;
            page_table[lru_index].dirty = memwrite;
            swap_ins++;

            // Increment last access time for all pages but the LRU selected
            for (i = 0; i < size_of_memory; i++) {
                if (i != lru_index) {
                    page_table[i].last_access_time++;
                }
            }

            // Return effective address
            effective = (frame << size_of_frame) | offset;
            return effective;
        }
    }
    // handle page fault with clock algorithm
    else if (page_replacement_scheme == REPLACE_CLOCK) {
        int replaced = FALSE;
        while (!replaced) { // iterate until replacement page is found
            if (!page_table[clock_hand].use_bit) { // find first page according to clock iteration where use_bit is 0
                // Write page to memory if dirty
                if (page_table[clock_hand].dirty) {
                    swap_outs++;
                }
            
                // Load the new page into frame
                page_table[clock_hand].page_num = page;
                page_table[clock_hand].free = FALSE;
                page_table[clock_hand].dirty = memwrite;
                page_table[clock_hand].use_bit = 1; 
                swap_ins++;
                replaced = TRUE;
    
                // Return effective address
                effective = (frame << size_of_frame) | offset;
                return effective;
                
            } else {
                page_table[clock_hand].use_bit = 0; // Clear the use bit if iterating over a set bit
            }

            // Move the clock hand to the next frame
            clock_hand = (clock_hand + 1) % size_of_memory;
        }
    }
    return -1; // error occured
}



/*
 * Super-simple progress bar.
 */
void display_progress(int percent)
{
    int to_date = PROGRESS_BAR_WIDTH * percent / 100;
    static int last_to_date = 0;
    int i;

    if (last_to_date < to_date) {
        last_to_date = to_date;
    } else {
        return;
    }

    printf("Progress [");
    for (i=0; i<to_date; i++) {
        printf(".");
    }
    for (; i<PROGRESS_BAR_WIDTH; i++) {
        printf(" ");
    }
    printf("] %3d%%", percent);
    printf("\r");
    fflush(stdout);
}


int setup()
{
    int i;

    page_table = (struct page_table_entry *)malloc(
        sizeof(struct page_table_entry) * size_of_memory
    );

    if (page_table == NULL) {
        fprintf(stderr,
            "Simulator error: cannot allocate memory for page table.\n");
        exit(1);
    }

    for (i=0; i<size_of_memory; i++) {
        page_table[i].free = TRUE;
    }

    return -1;
}


int teardown()
{
    free(page_table);
    return -1;
}


void error_resolve_address(long a, int l)
{
    fprintf(stderr, "\n");
    fprintf(stderr, 
        "Simulator error: cannot resolve address 0x%lx at line %d\n",
        a, l
    );
    exit(1);
}


int output_report()
{
    printf("\n");
    printf("Memory references: %d\n", mem_refs);
    printf("Page faults: %d\n", page_faults);
    printf("Swap ins: %d\n", swap_ins);
    printf("Swap outs: %d\n", swap_outs);

    return -1;
}


int main(int argc, char **argv)
{
    /* For working with command-line arguments. */
    int i;
    char *s;

    /* For working with input file. */
    FILE *infile = NULL;
    char *infile_name = NULL;
    struct stat infile_stat;
    int  line_num = 0;
    int infile_size = 0;

    /* For processing each individual line in the input file. */
    char buffer[MAX_LINE_LEN];
    long addr;
    char addr_type;
    int  is_write;

    /* For making visible the work being done by the simulator. */
    int show_progress = FALSE;

    /* Process the command-line parameters. Note that the
     * REPLACE_OPTIMAL scheme is not required for A#3.
     */
    for (i=1; i < argc; i++) {
        if (strncmp(argv[i], "--replace=", 9) == 0) {
            s = strstr(argv[i], "=") + 1;
            if (strcmp(s, "fifo") == 0) {
                page_replacement_scheme = REPLACE_FIFO;
            } else if (strcmp(s, "lru") == 0) {
                page_replacement_scheme = REPLACE_LRU;
            } else if (strcmp(s, "clock") == 0) {
                page_replacement_scheme = REPLACE_CLOCK;
            } else if (strcmp(s, "optimal") == 0) {
                page_replacement_scheme = REPLACE_OPTIMAL;
            } else {
                page_replacement_scheme = REPLACE_NONE;
            }
        } else if (strncmp(argv[i], "--file=", 7) == 0) {
            infile_name = strstr(argv[i], "=") + 1;
        } else if (strncmp(argv[i], "--framesize=", 12) == 0) {
            s = strstr(argv[i], "=") + 1;
            size_of_frame = atoi(s);
        } else if (strncmp(argv[i], "--numframes=", 12) == 0) {
            s = strstr(argv[i], "=") + 1;
            size_of_memory = atoi(s);
        } else if (strcmp(argv[i], "--progress") == 0) {
            show_progress = TRUE;
        }
    }

    if (infile_name == NULL) {
        infile = stdin;
    } else if (stat(infile_name, &infile_stat) == 0) {
        infile_size = (int)(infile_stat.st_size);
        /* If this fails, infile will be null */
        infile = fopen(infile_name, "r"); 
    }

    if (page_replacement_scheme == REPLACE_NONE ||
        size_of_frame <= 0 ||
        size_of_memory <= 0 ||
        infile == NULL)
    {
        fprintf(stderr, 
            "usage: %s --framesize=<m> --numframes=<n>", argv[0]);
        fprintf(stderr, 
            " --replace={fifo|lru|optimal} [--file=<filename>]\n");
        exit(1);
    }


    setup();

    while (fgets(buffer, MAX_LINE_LEN-1, infile)) {
        line_num++;
        if (strstr(buffer, ":")) {
            sscanf(buffer, "%c: %lx", &addr_type, &addr);
            if (addr_type == 'W') {
                is_write = TRUE;
            } else {
                is_write = FALSE;
            }

            if (resolve_address(addr, is_write) == -1) {
                error_resolve_address(addr, line_num);
            }
            mem_refs++;
        } 

        if (show_progress) {
            display_progress(ftell(infile) * 100 / infile_size);
        }
    }
    

    teardown();
    output_report();

    fclose(infile);

    exit(0);
}
