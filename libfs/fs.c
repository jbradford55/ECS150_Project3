#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define SUPERBLOCK_SIZE 512
#define FAT_ENTRY_SIZE 2
#define ROOT_DIR_ENTRY_SIZE 32
#define BLOCK_SIZE2 512


// Define file system data structures
struct SuperBlock {
	char signature[8];
  uint16_t amount_block_VirDisk;
  uint16_t root_directory;
  uint16_t data_block_startIdx;
  uint16_t amount_dataBlocks;
  uint8_t blocks_Fat;  // uint8_t because it is 1 byte or 8 bits
	char padding[4080];
};
// struct for the RootDirectory
struct RootDirectory { 
	char filename[FS_FILENAME_LEN]; // given in the fs.h
  uint32_t file_size;  // uint32_t because it is 4 bytes or 32 bits
  uint16_t idx_first_data_block; //uint16_t because it is 2 bytes or 16 bits
	char padding[10];
}; 

// delcaring global varialbes
static int check_fsmount = 0; // check if the FS is mounted
static uint16_t *flat_array_table = NULL; 
static struct SuperBlock *superblock; 
static struct RootDirectory *root_direct = NULL; // used to dynamically allocate memory for the root directory of the fs
static int open_file; // check if there is any open file


/* TODO: Phase 1 */

int fs_mount(const char *diskname) {
	// if not able to open the disk
	if (block_disk_open(diskname) == -1) { 
		return -1;
	}
	// allocate memory for the superblock
	superblock = (struct SuperBlock *)malloc(SUPERBLOCK_SIZE);
	if (block_read(0, &superblock) == -1) { // read block 0 from disk 0 = superblock
		block_disk_close();
		return -1;
	}
	// checking the file system signature is = "ECS150FS"
	if (strncmp(superblock->signature, "ECS150FS", 8) != 0) { // 8 bcz signature is 8 charecters
		block_disk_close(); // close the disk
		return -1;
	}
	// check amount_block_VirDisk is = to the amount of block in the superblock
	if (superblock->amount_block_VirDisk != block_disk_count()){ 
		block_disk_close();        // block_disk_count() =  Get block count
		return -1;
	}
	// check root_directory is = to the amount of block in the superblock
	// Check if the root_directory field is not equal to the amount_block_VirDisk field
	// If it is not equal that means that the file system is not valid
	if (superblock->root_directory != superblock->amount_block_VirDisk) {
		block_disk_close();
		return -1;
	}
	
	// check data_block_startIdx field is not equal to the amount_block_VirDisk field
	if (superblock->data_block_startIdx != superblock->amount_block_VirDisk) {
		block_disk_close();
		return -1;
	}
	// check amount_dataBlocks field is not equal to the amount_block_VirDisk field
	if (superblock->amount_dataBlocks != superblock->amount_block_VirDisk) {
		block_disk_close();
		return -1;
	}
	// check blocks_Fat field is not equal to the amount_block_VirDisk field
	if (superblock->blocks_Fat != superblock->amount_block_VirDisk) {
		block_disk_close();
		return -1;
	}
	// now load the flat array table  from VD (virtual disk) into memeory
	/* FAT is located on one or more blocks, and keeps track of both the free data blocks and the mapping between files and the data blocks holding their content. From PDF */
	// allocate memory for the flat array table (FAT)
	flat_array_table = malloc(superblock->blocks_Fat * FAT_ENTRY_SIZE);
	if (flat_array_table == NULL) {
		block_disk_close();
		return -1;
	}
	for(int i = 0; i < superblock->blocks_Fat; i++) {
		// i+1 because 0 is the superblock 
		if (block_read(i+1, &flat_array_table + (i * BLOCK_SIZE2/2)) == -1) { 
			free(flat_array_table); 
			block_disk_close();
			return -1;
		}
	}
	
// Superblock is now loaded into memory, FAT is loaded into memory, and Now it's the time for RootDirectory to load into memory
/* The Root directory is in the following block and contains an entry for each file of the file system, defining its name, size and the location of the first data block for this file. PDF */

// allocate memory for the root directory
	root_direct = malloc(superblock->root_directory * ROOT_DIR_ENTRY_SIZE);
	if (root_direct == NULL) {
		free(flat_array_table);
		block_disk_close();
		return -1;
	}
	
	
	// file system mouted successfully
	check_fsmount = 1; 
	return 0;
	
}


int fs_umount(void)	{
	//check if the file is not mounted
	if (check_fsmount == 0) {
		return -1;
	}
	// check for any open file
	if (open_file > 0) {
		return -1;
	}
//---------------------------//
	// writing FAT to DISK (NOT SURE IF IMPLEMENTED CORRECTLY)
	for (int i = 0; i < superblock->blocks_Fat; i++) {
		if (block_write(i+1, &flat_array_table + (i * BLOCK_SIZE2/sizeof(uint16_t))) == -1) {
				block_disk_close();
			  return -1;
		}
	}
	// Writing the Root Directory to Disk (NOT SURE IF IMPLEMENTED CORRECTLY)
	for (int i = 0; i < superblock->root_directory; i++) {
		if (block_write(i+1, &root_direct + (i * BLOCK_SIZE2 / sizeof(struct RootDirectory))) == -1) {
				block_disk_close();
			  return -1;
		}
	}
	// Writing the Superblock to Disk
	if (block_write(0, &superblock) == -1) { // superblock is the first block, block 0
		block_disk_close();
		return -1;
	}
 
	
	// free memory for superblock
	if (superblock != NULL) {
		free(superblock);
		superblock = NULL;
	}
	// free memory for flat_array_table 
	if (flat_array_table != NULL) {
		free(flat_array_table);
		flat_array_table = NULL;
	}
	// free memory root_direct
	if (root_direct != NULL) {
		free(root_direct);
		root_direct = NULL;
	}
	
	if (block_disk_close() == -1) { 
		return -1;
	} 
	// file system is unmounted
	check_fsmount = 0; 
	return 0;
}


int fs_info(void)
{
	/* TODO: Phase 1 */
	if(check_fsmount == 0) {
		// no FS is mounted then print error message
		printf("No file is mounted.\n");
		return -1;
	}
	
	// counting # of files en root directory
	int total_fileCount_rootDirectory = 0;
	for (int i = 0; i < superblock->root_directory; i++) {
		if (root_direct[i].filename[0] != '\0') {
			total_fileCount_rootDirectory++;
		}
	}
	
	// find and count # of free blocks in the FS
	int total_free_blockCount = 0; // holds the total count of free blocks, 
	for (int i = 0; i < superblock->blocks_Fat; i++) {
		if (flat_array_table[i] == 0) { // if it's to 0 then it's a free block
			total_free_blockCount++;
		}
	}
	// print all the information about the file
	printf("Display information about file system :\n");
	printf("Total blocks count: %d\n", superblock->amount_block_VirDisk);
	printf("Fat blocks count: %d\n", superblock->blocks_Fat);
	printf("Free block count: %d\n", total_free_blockCount); // implemented this few lines above
	printf("Number of files in Root Directory: %d\n", total_fileCount_rootDirectory); // implemented this few lines above
	printf("Data blocks count: %d\n", superblock->data_block_startIdx);
	printf("Amout of data block count: %d\n", superblock->amount_dataBlocks);
	return 0;
}

// creating a new file in the root directory
int fs_create(const char *filename)
{
	/* TODO: Phase 2 */
	if (check_fsmount == 0) {
		// no FS is mounted then print error message
		printf("No file is mounted.\n");
	}
	// check the file  
	if (filename == NULL || strlen(filename) >= FS_FILENAME_LEN|| strlen(filename) == 0) {
			return -1;
	}
	// another check to see if there is an existing file with the same name
		// go thru root directory and check for filenames 
		for (int i =0; i< superblock->root_directory; i++) {
			if (strcmp(root_direct[i].filename, filename) == 0) {
				// if file with same name existed then retuen -1
				return -1;
			}
		}
	
		// if no file with same name existed then create a new file
		// find empty spot in the root directory, if there is free space then create a new file.
		for (int i = 0; i < superblock->root_directory; i++) {
			if (root_direct[i].filename[0] == '\0') {
				// copy the filename into the root directory
				strncpy(root_direct[i].filename, filename, FS_FILENAME_LEN); // should work ***
				root_direct[i].idx_first_data_block = 0xFFFF; //0xFFF = end of chain
				root_direct[i].file_size = 0;
			}
		}

		// find the first free block in the FAR
		int first_free_block = 0;
		for (int i = 0; i < superblock->blocks_Fat; i++) {
			if (flat_array_table[i] == 0) {
				first_free_block = i;
				break;
			}
		}
		// set the first free block in the FAT to 1
		flat_array_table[first_free_block] = 1;
		// write the FAT to disk
		if (block_write(first_free_block+1, &flat_array_table) == -1) {
			return -1;
		}
		// update the superblock
		superblock->amount_dataBlocks++;
		// write the superblock to disk
		if (block_write(0, &superblock) == -1) {
			return -1;
		}

	return 0;
	
}
// * Delete the file from the root directory of the mounted file
int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */
	if (check_fsmount == 0) {
		printf("No file is mounted.\n");
	}
	// check the file information
	if (filename == NULL || strlen(filename) == 0) {
		return -1;
	}

	//  Find the file in the root directory
	int idx = -1;
	for (int i = 0; i < superblock->root_directory; i++) {
		if (strcmp(root_direct[i].filename, filename) == 0) {
				idx = i;
			break;
		}
	}
	// if file not found then return -1
	if (idx == -1) {
		return -1;
	}
	// if file is found then delete the file
	memset(&root_direct[idx], 0, sizeof(struct RootDirectory));

	// write the root directory to disk
	if (block_write(idx+1, &root_direct[idx]) == -1){
		return -1;
	}
	// Write back to disk
	if (block_write(0, &superblock) == -1) {
		return -1;
	}
	// update the FAT
	flat_array_table[root_direct[idx].idx_first_data_block] = 0;
	// update the superblock
	superblock->amount_dataBlocks--;
	// write the superblock to disk
	if (block_write(0, &superblock) == -1) {
		return -1;
	}
	// write the FAT to disk
	if (block_write(root_direct[idx].idx_first_data_block+1, &flat_array_table) == -1) {
		return -1;
	}
	return 0;
}

int fs_ls(void)
{
	/* TODO: Phase 2 */
	if (check_fsmount == 0){
		printf("No file is mounted.\n");
	}
	// check if there is any file in the root directory
	if (root_direct == NULL) {
		return -1;
	}
	for (int i = 0; i < superblock->root_directory; i++){
		if (root_direct[i].filename[0] != '\0'){
			printf("File name: %s, File size: %d\n", root_direct[i].filename, root_direct[i].file_size);
		}
	}
	return 0;
}


#define MAX_FILE_DESCRIPTORS 32

struct fileDescriptor {
    int lseek;
    int rootIndex;
    int size;
    int numBlocks;
    int *dataIndices;
    const char *filename;
};

struct fileDescriptor *fileDescriptors[32];

int fs_open(const char *filename) {
    // first we check file system is mounted
    if (check_fsmount == 0) {
        return -1; 
    }

    // now we need empty slot in array to open fd
    int fd;
    for (fd = 0; fd < MAX_FILE_DESCRIPTORS; fd++) {
        if (fileDescriptors[fd] == NULL) {
            break; // empty slot == found
        }
    }

    if (fd == MAX_FILE_DESCRIPTORS) {
        return -1;
    }

    // search for the file in the root directory
    int file_index = -1;
    for (int i = 0; i < superblock->root_directory; i++) {
        if (strcmp(root_direct[i].filename, filename) == 0) {
            file_index = i;
            break; //equality found
        }
    }

    if (file_index == -1) {
        return -1; // indicating file not found, as -1 is the val we initialized to file_index
    }

    //create new file descriptor object from struct
    struct fileDescriptor *newDescriptor = malloc(sizeof(struct fileDescriptor));
    if (newDescriptor == NULL) {
        return -1; // indicates that th memory allocation failed
    }

    // Initialize file descriptor values
    newDescriptor->lseek = 0;
    newDescriptor->rootIndex = file_index;
    newDescriptor->size = root_direct[file_index].file_size;
    newDescriptor->filename = filename;

    // now, finally store the file descriptor in the file descriptor array, and return the index
    fileDescriptors[fd] = newDescriptor;


    return fd;
}


int fs_close(int fd)
{

    // check if file descriptor open
    if (fileDescriptors[fd] == NULL) {
        // File descriptor is already closed
        return -1;
    }

    // Free memory associated with the file descriptor
    free(fileDescriptors[fd]);
    fileDescriptors[fd] = NULL;

    return 0;
}

int fs_stat(int fd)
{
    // check valid fd

    if (fileDescriptors[fd] == NULL) {
        // File descriptor is not open
        return -1;
    }

	return fileDescriptors[fd]->size; //return file size
}


int fs_lseek(int fd, size_t offset)
{
    // check if the file descriptor is valid

    // make sure file is open
    if (fileDescriptors[fd] == NULL) {
        return -1;
    }

    // set offset of file descriptor
    fileDescriptors[fd]->lseek = offset;

    // Return the new file offset
    return (int)offset;
}


int fs_write(int fd, void *buf, size_t count)
{
    /* TODO: Phase 4 */
    (void)fd;
    (void)buf;
    (void)count;
    return 0;
}

int fs_read(int fd, void *buf, size_t count)
{
    /* TODO: Phase 4 */
    (void)fd;
    (void)buf;
    (void)count;
    return 0;
}

/* 
REFERENCES/CITATION
http://elmchan.org/docs/fat_e.html#:~:text=The%20FAT%20entry%20with%20last,0xFF8%20%2D%200xFFF%20(typically%200xFFF)
https://stackoverflow.com/questions/1864103/reading-superblock-into-a-c-structure
https://people.cs.rutgers.edu/~pxk/416/notes/11-filesystems.html


*/

