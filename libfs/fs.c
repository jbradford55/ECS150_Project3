#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"



/* TODO: Phase 1 */
char* read_bytes_to_string(char* buffer, int num_bytes){
	char* output_string = malloc(num_bytes * sizeof(char));
	for (int index = 0; index < num_bytes; index++){
		output_string[index] = buffer[index];
	}
	return output_string;
}


int read_bytes_to_int(char* buffer, int start, int num_bytes){

	char binary_string[50] = "";
	//loop through each byte
	for (int index = start; index < start + num_bytes; index++){
		//get byte
		int8_t byte = buffer[index];

		//convert byte to binary
		for (int byte_index = 0; byte_index < 8; byte_index++)
		{
			if ((byte & 1)==1){
				char temp_str[50] = "1";
				strcat(temp_str, binary_string);
				strcpy(binary_string, temp_str);
			} else {
				char temp_str[50] = "0";
				strcat(temp_str, binary_string);
				strcpy(binary_string, temp_str);
			}
			byte = byte >> 1;
		}
	}


	//convert binary to int
	int output = 0;
	for (int binary_string_index = 0; binary_string_index < strlen(binary_string); binary_string_index++){
		if (binary_string[binary_string_index] == '1'){
			int power = strlen(binary_string) - binary_string_index - 1;
			int sum = 1;
			for (int s = 0; s < power; s++){
				sum *= 2;
			}
			output += sum;
		}
	}
	
	return output;
}

int count_number_of_occupied(int8_t *buffer, int block_index, int step_size){
	int count = 0;
	block_read(block_index, buffer);
	for (int i = 0; i < BLOCK_SIZE; i+= step_size){
		if (buffer[i] != NULL){
			count++;
		}
	}
	return count;
}

struct sb{
	int16_t data_blocks;
	int16_t root_directory_block_index;
	int16_t data_block_start_index;
	int16_t amount_of_data_blocks;
	int8_t amount_of_fat_blocks;

	int8_t free_fat_spaces;
	int free_root_spaces;
};

static struct sb* SB;

int fs_mount(const char *diskname)
{
	//read in super block
	block_disk_open(diskname);
	char *super_block = malloc(BLOCK_SIZE*sizeof(uint8_t));
	block_read(0, super_block);

	SB = malloc(sizeof(struct sb));

	//read in first 8 bytes
	char* first_8_bytes = malloc(8 * sizeof(char));
	first_8_bytes = read_bytes_to_string(super_block, 8);
	if (strcmp(first_8_bytes, "ECS150FS")){
		return -1;
	}

	//read in bytes 8-9(data blocks)
	SB->data_blocks = (int16_t)read_bytes_to_int(super_block, 8, 2);

	//read in bytes 10-11(root dir block)
	SB->root_directory_block_index = (int16_t)read_bytes_to_int(super_block, 10, 2);

	//read in bytes 12-13(data block start)
	SB->data_block_start_index = (int16_t)read_bytes_to_int(super_block, 12, 2);

	//read in bytes 14-15(amount of data blocks)
	SB->amount_of_data_blocks = (int16_t)read_bytes_to_int(super_block, 14, 2);	

		//read in bytes 16(number of FAT blocks)
	SB->amount_of_fat_blocks = (int8_t)read_bytes_to_int(super_block, 16, 1);	


	//occupied fat blocks
	int8_t *fat_block = malloc(BLOCK_SIZE * sizeof(int8_t));
	int occupied_fat_spaces = count_number_of_occupied(fat_block, 1, 16);
	// printf("%d\n", occupied_fat_spaces);
	// printf("%d\n", SB->amount_of_fat_blocks);
	SB->free_fat_spaces = (SB->amount_of_data_blocks) - occupied_fat_spaces;


	int8_t *root_block = malloc(BLOCK_SIZE * sizeof(int8_t));
	int8_t occupied_root_spaces = count_number_of_occupied(root_block, SB->root_directory_block_index, 32);
	SB->free_root_spaces = 128 - occupied_root_spaces;	

	return 0;
}

int fs_umount(void)
{
	block_disk_close();
	/* TODO: Phase 1 */
}

int fs_info(void)
{
	printf("FS Info:\n");
	printf("total_blk_count=%d\n", SB->data_blocks);
	printf("fat_blk_count=%d\n", SB->amount_of_fat_blocks);
	printf("rdir_blk=%d\n", SB->root_directory_block_index);
	printf("data_blk=%d\n", SB->data_block_start_index);
	printf("data_blk_count=%d\n", SB->amount_of_data_blocks);
	printf("fat_free_ratio=%d/%d\n", SB->free_fat_spaces, SB->amount_of_data_blocks);
	printf("rdir_free_ratio=%d/%d\n", SB->free_root_spaces, 128);
	/* TODO: Phase 1 */
}

void update_string(int8_t *buffer, char* filename){
	for (int str_index = 0; str_index < strlen(filename); str_index++){
		buffer[str_index] = filename[str_index];
	}
}

int fs_create(const char *filename)
{
	/* TODO: Phase 2 */
	int8_t *root_block = malloc(BLOCK_SIZE*sizeof(int8_t));
	block_read(SB->root_directory_block_index, root_block);

	//iterate through 32 bit sized entries
	for (int entry = 0; entry < BLOCK_SIZE; entry++){
		
		//update string
		if (root_block[entry] == NULL){
			update_string(root_block[entry], filename);
		}
	}

	block_write(SB->root_directory_block_index, root_block);	



}

int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */
}

int fs_ls(void)
{
	/* TODO: Phase 2 */
}

int fs_open(const char *filename)
{
	/* TODO: Phase 3 */
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */
}

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
}

