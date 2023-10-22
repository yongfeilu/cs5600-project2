/*
 * file:        part-2.c
 * description: Part 2, CS5600 Project 2, Fall 2023
 */

/* NO OTHER INCLUDE FILES */
#include "elf64.h"
#include "sysdefs.h"
#include <stdlib.h>

extern void *vector[];

/* ---------- */

/* write these functions 
*/
int read(int fd, void *ptr, int len);           //done in part1
int write(int fd, void *ptr, int len);          //done in part1
void exit(int err);                             //done in part1
int open(char *path, int flags);
int close(int fd);
int lseek(int fd, int offset, int flag);
void *mmap(void *addr, int len, int prot, int flags, int fd, int offset);
int munmap(void *addr, int len);


int read(int fd, void *ptr, int len) {
	if (len < 0) {
		return -1;
	}

	return syscall(__NR_read, fd, ptr, len);
} /*https://man7.org/linux/man-pages/man2/read.2.html */


int write(int fd, void *ptr, int len){
	if (len < 0) {
        return -1;
    }

    return syscall(__NR_write, fd, ptr, len);
} /* https://man7.org/linux/man-pages/man2/write.2.html */



void exit(int err){
	syscall(__NR_exit, err);
} /* https://man7.org/linux/man-pages/man2/exit.2.html */


int open(char *path, int flags) {
    int fd = syscall(__NR_open, path, flags);
    if (fd < 0)
        return -1;
    return fd;
}

int close(int fd) {
    return syscall(__NR_close, fd);
}

int lseek(int fd, int offset, int flag) {
    return syscall(__NR_lseek, fd, offset, flag);
}

void *mmap(void *addr, int len, int prot, int flags, int fd, int offset) {
    return (void *) syscall(__NR_mmap, addr, len, prot, flags, fd, offset);
}

int munmap(void *addr, int len) {
    return syscall(__NR_munmap, addr, len);
}

/* ---------- */

/* Write the three 'system call' functions - do_readline, do_print, do_getarg 
 * Adjust the functions readline and print-and-clean functions written in part 1, to obtain
 * the 'system call' functions do_readline and do_print
 * hints: 
 *  - read() or write() one byte at a time. It's OK to be slow.
 *  - stdin is file desc. 0, stdout is file descriptor 1
 *  - use global variables for getarg
 */

/* your code here */
void do_readline(char *buf, int len);
void do_print(char *buf);
char *do_getarg(int i);         

/* Global variables */
int g_argc = 0;
char* g_argv[10];  // assume max of 10 arguments

void do_readline(char *buf, int n) {
    char c = 0;
    int i = 0;
    while (i < n - 1) {
        int byte_read = read(0, &c, 1);
        if (byte_read <= 0) {
            break;
        }
        buf[i] = c;
        if (c == '\n') {
            break;
        }
        i++;
    }
    buf[i] = '\0';
    // g_argc = split(g_argv, 10, buf);
}

void do_print(char *buf) {
    int i = 0;
    while (buf[i] != '\0') {
        write(1, &buf[i], 1);
        i++;
    }
    write(1, "\n", 1);
}

char *do_getarg(int i) {
    if (i >= 0 && i < g_argc) {
        return g_argv[i];
    }
    return 0;
}

/* ---------- */



/* simple function to split a line:
 *   char buffer[200];
 *   <read line into 'buffer'>
 *   char *argv[10];
 *   int argc = split(argv, 10, buffer);
 *   ... pointers to words are in argv[0], ... argv[argc-1]
 */
int split(char **argv, int max_argc, char *line)
{
	int i = 0;
	char *p = line;

	while (i < max_argc) {
		while (*p != 0 && (*p == ' ' || *p == '\t' || *p == '\n'))
			*p++ = 0;
		if (*p == 0)
			return i;
		argv[i++] = p;
		while (*p != 0 && *p != ' ' && *p != '\t' && *p != '\n')
			p++;
	}
	return i;
}

/* ---------- */

/* This is where you write the details of the function exec(char* filename) called by main()
* Follow instructions listed in project description.
* the guts of part 2
*   read the ELF header
*   for each segment, if b_type == PT_LOAD:
*     create mmap region
*     read from file into region
*   function call to hdr.e_entry
*   munmap each mmap'ed region so we don't crash the 2nd time
*
*   don't forget to define offset, and add it to virtual addresses read from ELF file
*
*               your code here
*/

// #define OFFSET 0x80000000

// void exec(char* filename) {
//     int fd;
//     struct elf64_ehdr hdr;
//     struct elf64_phdr *phdrs;

//     fd = open(filename, O_RDONLY);
//     if (fd < 0) {
//         do_print("Failed to open the file");
//         return;
//     }

//     // Read the ELF header at the beginning of the file
//     if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
//         do_print("Failed to read ELF header");
//         close(fd);
//         return;
//     }

//     // Allocate space for and read the program headers
//     phdrs = (struct elf64_phdr *)malloc(hdr.e_phnum * sizeof(struct elf64_phdr));
//     lseek(fd, hdr.e_phoff, SEEK_SET);
//     if (read(fd, phdrs, hdr.e_phnum * sizeof(struct elf64_phdr)) != hdr.e_phnum * sizeof(struct elf64_phdr)) {
//         do_print("Failed to read program headers");
//         free(phdrs);
//         close(fd);
//         return;
//     }

//     for (int i = 0; i < hdr.e_phnum; i++) {
//         if (phdrs[i].p_type == PT_LOAD) {
//             // Calculate size to be mmaped, rounding up to the nearest page boundary
//             int len = ROUND_UP(phdrs[i].p_memsz, 4096);
//             void* addr = mmap((void*)(phdrs[i].p_vaddr + OFFSET), len, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
//             if (addr == MAP_FAILED) {
//                 do_print("Memory mapping failed");
//                 free(phdrs);
//                 close(fd);
//                 return;
//             }

//             // Seek to the segment's position in the file and read its content into the mapped memory region
//             lseek(fd, phdrs[i].p_offset, SEEK_SET);
//             if (read(fd, addr, phdrs[i].p_filesz) != phdrs[i].p_filesz) {
//                 do_print("Failed to read segment data");
//                 munmap(addr, len);
//                 free(phdrs);
//                 close(fd);
//                 return;
//             }
//         }
//     }

//     close(fd);

//     // Jump to the entry point of the loaded executable
//     void (*entry_point)(void) = (void (*)(void))(hdr.e_entry + OFFSET);
//     entry_point();

//     // Cleanup
//     for (int i = 0; i < hdr.e_phnum; i++) {
//         if (phdrs[i].p_type == PT_LOAD) {
//             int len = ROUND_UP(phdrs[i].p_memsz, 4096);
//             munmap((void*)(phdrs[i].p_vaddr + OFFSET), len);
//         }
//     }
//     free(phdrs);
// }



int str_equals(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return (*str1 - *str2) == 0;
}


/* ---------- */
void main(void)
{   // The vector array is defined as a global array. It plays the role of a system call vector table 
	// (similar to the interrupt vector table seen in class). Each entry in this array/table holds the address
	// of the corresponding system function. Check out call-vector.S and Makefile to see how the vector table is built.
	char buffer[200];
	vector[0] = do_readline;
	vector[1] = do_print;
	vector[2] = do_getarg;

	/* YOUR CODE HERE AS DESCRIBED IN THE FILE DESCRIPTION*/
	/* When the user enters an executable_file, the main function should call exec(executable_file) */
	while (1) {
        do_print("Enter command:");
        do_readline(buffer, sizeof(buffer));
        g_argc = split(g_argv, 10, buffer);

        if (g_argc == 0) continue; // Empty command

        
        if (str_equals(g_argv[0], "quit")) {
            exit(0);
        } else {
            do_print(g_argv[0]);
            // exec(g_argv[0]);
        }
        
    }
}

