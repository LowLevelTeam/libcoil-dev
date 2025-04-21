/**
 * Implementation of the arena allocator
 */

#include <coil/arena.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

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

static void free_block(Block* block) {
  if (block) {
    free(block->memory);
    free(block);
  }
}

coil_arena_t* arena_init(size_t initial_size, size_t max_size) {
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

  coil_arena_t* arena = (coil_arena_t*)malloc(sizeof(coil_arena_t));
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

void arena_destroy(coil_arena_t* arena) {
  if (!arena) return;

  Block* block = arena->first_block;
  while (block) {
    Block* next = block->next;
    free_block(block);
    block = next;
  }

  free(arena);
}

static size_t align_up(size_t value, size_t alignment) {
  assert((alignment & (alignment - 1)) == 0 && "Alignment must be a power of 2");
  return (value + alignment - 1) & ~(alignment - 1);
}

static int add_block(coil_arena_t* arena, size_t min_size) {
  // Determine new block size based on growth strategy
  size_t new_size;
  
  // For small blocks, ensure growth to at least min_block_size
  if (arena->current_block->size < arena->min_block_size) {
    new_size = arena->min_block_size;
  } else {
    // For larger blocks, double the size
    new_size = arena->current_block->size * 2;
  }
  
  // Ensure new_size is at least min_size
  if (new_size < min_size) {
    new_size = min_size;
  }
  
  // Check against max_size if it's set
  if (arena->max_size > 0) {
    if (arena->total_size + new_size > arena->max_size) {
      // Adjust to fit within max_size
      if (arena->total_size < arena->max_size) {
        new_size = arena->max_size - arena->total_size;
        if (new_size < min_size) {
          // Not enough space for even the minimum size
          return 0;
        }
      } else {
        // Already at max_size
        return 0;
      }
    }
  }

  // Create new block
  Block* block = create_block(new_size);
  if (!block) return 0;

  // Add the new block to the chain
  arena->current_block->next = block;
  arena->current_block = block;
  arena->total_size += new_size;

  return 1;
}

void* arena_alloc(coil_arena_t* arena, size_t size, size_t alignment) {
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
    
    // Need to align again in case the new block has an alignment requirement
    offset = align_up(offset, alignment);
    
    // Check if the new block is big enough (unlikely to fail, but let's check anyway)
    if (offset + size > block->size) {
      return NULL;
    }
  }

  // Allocate from the current block
  void* result = (char*)block->memory + offset;
  block->used = offset + size;
  arena->total_used += size;

  return result;
}

void* arena_alloc_default(coil_arena_t* arena, size_t size) {
  // Default to sizeof(max_align_t) alignment, which is usually suitable
  // for any data type without specific alignment requirements
  return arena_alloc(arena, size, sizeof(max_align_t));
}

void arena_reset(coil_arena_t* arena) {
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

size_t arena_capacity(const coil_arena_t* arena) {
  return arena ? arena->total_size : 0;
}

size_t arena_used(const coil_arena_t* arena) {
  return arena ? arena->total_used : 0;
}

size_t arena_max_size(const coil_arena_t* arena) {
  return arena ? arena->max_size : 0;
}

void* arena_push(coil_arena_t* arena, const void* data, size_t size, size_t alignment) {
  if (!arena || !data || size == 0) return NULL;
  
  // Allocate memory for the object
  void* dest = arena_alloc(arena, size, alignment);
  if (!dest) return NULL;
  
  // Copy the data into the allocated memory
  memcpy(dest, data, size);
  
  return dest;
}

void* arena_push_default(coil_arena_t* arena, const void* data, size_t size) {
  return arena_push(arena, data, size, sizeof(max_align_t));
}