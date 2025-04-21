/**
 * @file arena.c
 * @brief Implementation of the arena allocator
 */

#include "arena.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

/**
 * @struct Block
 * @brief Represents a block of memory in the arena
 */
typedef struct Block {
	void* memory;         /**< Pointer to the allocated memory */
	size_t size;          /**< Size of the block in bytes */
	size_t used;          /**< Amount of memory used in this block */
	struct Block* next;   /**< Pointer to the next block */
} Block;

/**
 * @struct Arena
 * @brief Arena allocator structure
 */
struct Arena {
	Block* first_block;   /**< Pointer to the first block in the arena */
	Block* current_block; /**< Pointer to the current block for allocations */
	size_t total_size;    /**< Total size of all blocks in the arena */
	size_t total_used;    /**< Total amount of memory used in the arena */
	size_t min_block_size; /**< Minimum size for new blocks */
	size_t max_size;      /**< Maximum size the arena can grow to (0 for unlimited) */
};

/**
 * @brief Create a new memory block for the arena
 *
 * @param size Size of the block to create
 * @return Pointer to the new block or NULL on failure
 */
static Block* create_block(size_t size) {
	Block* block = (Block*)malloc(sizeof(Block));
	if (!block) return NULL;

	block->memory = malloc(size);
	if (!block->memory) {
		free(block);
		return NULL;
	}

	block->size = size;
	block->used = 0;
	block->next = NULL;

	return block;
}

/**
 * @brief Free a memory block
 *
 * @param block Pointer to the block to free
 */
static void free_block(Block* block) {
	if (block) {
		free(block->memory);
		free(block);
	}
}

Arena* arena_init(size_t initial_size, size_t max_size) {
	// Minimum block size is 4KB
	const size_t min_block_size = 4096;
	
	// Ensure initial_size is at least the minimum
	if (initial_size < min_block_size) {
		initial_size = min_block_size;
	}
	
	// Validate max_size if specified
	if (max_size > 0 && max_size < initial_size) {
		// Max size must be at least as large as initial size
		return NULL;
	}

	Arena* arena = (Arena*)malloc(sizeof(Arena));
	if (!arena) return NULL;

	Block* block = create_block(initial_size);
	if (!block) {
		free(arena);
		return NULL;
	}

	arena->first_block = block;
	arena->current_block = block;
	arena->total_size = initial_size;
	arena->total_used = 0;
	arena->min_block_size = min_block_size;
	arena->max_size = max_size; // 0 means unlimited

	return arena;
}

void arena_destroy(Arena* arena) {
	if (!arena) return;

	Block* block = arena->first_block;
	while (block) {
		Block* next = block->next;
		free_block(block);
		block = next;
	}

	free(arena);
}

/**
 * @brief Align a value to the specified alignment
 *
 * @param value Value to align
 * @param alignment Alignment (must be a power of 2)
 * @return Aligned value
 */
static size_t align_up(size_t value, size_t alignment) {
	assert((alignment & (alignment - 1)) == 0 && "Alignment must be a power of 2");
	return (value + alignment - 1) & ~(alignment - 1);
}

/**
 * @brief Add a new block to the arena
 *
 * @param arena Pointer to the arena
 * @param min_size Minimum size required for the new block
 * @return 1 if successful, 0 on failure
 */
static int add_block(Arena* arena, size_t min_size) {
	// Determine the size of the new block
	// Double the current block size, but ensure it's at least min_size
	size_t new_size = arena->current_block->size * 2;
	if (new_size < min_size) {
		new_size = min_size;
	}
	if (new_size < arena->min_block_size) {
		new_size = arena->min_block_size;
	}
	
	// Check against max_size if it's set
	if (arena->max_size > 0) {
		size_t new_total_size = arena->total_size + new_size;
		
		// Check if adding this block would exceed max_size
		if (new_total_size > arena->max_size) {
			// If we can't fit the doubled size, try the minimum required size
			if (arena->total_size + min_size > arena->max_size) {
				// Can't even fit the minimum size needed
				return 0;
			}
			
			// Use the maximum size we can allocate without exceeding max_size
			new_size = arena->max_size - arena->total_size;
			
			// Ensure it's at least min_size
			if (new_size < min_size) {
				return 0;
			}
		}
	}

	Block* block = create_block(new_size);
	if (!block) return 0;

	// Add the new block to the end of the list
	arena->current_block->next = block;
	arena->current_block = block;
	arena->total_size += new_size;

	return 1;
}

void* arena_alloc(Arena* arena, size_t size, size_t alignment) {
	if (!arena || size == 0) return NULL;

	// Ensure alignment is at least 1
	if (alignment < 1) alignment = 1;

	// Check if the size is reasonable (not too large to cause integer overflow)
	if (size > SIZE_MAX - alignment) return NULL;

	Block* block = arena->current_block;

	// Align the current block's used pointer
	size_t offset = align_up(block->used, alignment);

	// Check if we have enough space in the current block
	if (offset + size > block->size) {
		// Not enough space, try to add a new block
		if (!add_block(arena, offset + size)) {
			return NULL;
		}
		
		// Update block to the new current block
		block = arena->current_block;
		offset = 0; // Start at the beginning of the new block
	}

	// Allocate from the current block
	void* result = (char*)block->memory + offset;
	block->used = offset + size;
	arena->total_used += size;

	return result;
}

void* arena_alloc_default(Arena* arena, size_t size) {
	// Default to sizeof(max_align_t) alignment, which is usually suitable
	// for any data type without specific alignment requirements
	return arena_alloc(arena, size, sizeof(max_align_t));
}

void arena_reset(Arena* arena) {
	if (!arena) return;

	// Reset all blocks
	Block* block = arena->first_block;
	while (block) {
		block->used = 0;
		block = block->next;
	}

	// Reset the current block to the first block
	arena->current_block = arena->first_block;
	arena->total_used = 0;
}

size_t arena_capacity(const Arena* arena) {
	return arena ? arena->total_size : 0;
}

size_t arena_used(const Arena* arena) {
	return arena ? arena->total_used : 0;
}

size_t arena_max_size(const Arena* arena) {
	return arena ? arena->max_size : 0;
}

void* arena_push(Arena* arena, const void* data, size_t size, size_t alignment) {
	if (!arena || !data || size == 0) return NULL;
	
	// Allocate memory for the object
	void* dest = arena_alloc(arena, size, alignment);
	if (!dest) return NULL;
	
	// Copy the data into the allocated memory
	memcpy(dest, data, size);
	
	return dest;
}

void* arena_push_default(Arena* arena, const void* data, size_t size) {
	return arena_push(arena, data, size, sizeof(max_align_t));
}