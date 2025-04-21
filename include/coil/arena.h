/**
 * @file arena.h
 * @brief Simple arena allocator for fast memory management
 * 
 * This arena allocator provides a way to allocate memory quickly from 
 * pre-allocated blocks. It handles dynamic resizing while maintaining
 * pointer validity.
 */

#ifndef __COIL_INCLUDE_GUARD_ARENA_H
#define __COIL_INCLUDE_GUARD_ARENA_H

#include <stddef.h>

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
*
* The arena allocator manages memory in blocks, allowing for
* fast allocation and deallocation of memory.
*/
typedef struct coil_arena {
  Block* first_block;   /**< Pointer to the first block in the arena */
  Block* current_block; /**< Pointer to the current block for allocations */
  size_t total_size;    /**< Total size of all blocks in the arena */
  size_t total_used;    /**< Total amount of memory used in the arena */
  size_t min_block_size; /**< Minimum size for new blocks */
  size_t max_size;      /**< Maximum size the arena can grow to (0 for unlimited) */
} coil_arena_t;

/**
 * @brief Initialize a new arena with the specified initial capacity and maximum size
 *
 * @param initial_size Initial size of the arena in bytes
 * @param max_size Maximum size the arena can grow to (0 for unlimited)
 * @return Pointer to the initialized arena or NULL on failure
 */
coil_arena_t *arena_init(size_t initial_size, size_t max_size);

/**
 * @brief Destroy an arena and free all associated memory
 *
 * @param arena Pointer to the arena to destroy
 */
void arena_destroy(coil_arena_t *arena);

/**
 * @brief Allocate memory from the arena
 *
 * @param arena Pointer to the arena
 * @param size Size of the memory block to allocate in bytes
 * @param alignment Memory alignment (must be a power of 2)
 * @return Pointer to the allocated memory or NULL on failure
 */
void* arena_alloc(coil_arena_t *arena, size_t size, size_t alignment);

/**
 * @brief Allocate memory from the arena with default alignment
 *
 * @param arena Pointer to the arena
 * @param size Size of the memory block to allocate in bytes
 * @return Pointer to the allocated memory or NULL on failure
 */
void* arena_alloc_default(coil_arena_t *arena, size_t size);

/**
 * @brief Reset the arena, making all previously allocated memory available again
 *
 * This does not actually free any memory, it just resets the arena's internal
 * pointers so that future allocations will reuse the existing memory.
 *
 * @param arena Pointer to the arena to reset
 */
void arena_reset(coil_arena_t *arena);

/**
 * @brief Get the total capacity of the arena
 *
 * @param arena Pointer to the arena
 * @return Total capacity of the arena in bytes
 */
size_t arena_capacity(const coil_arena_t *arena);

/**
 * @brief Get the amount of memory currently allocated from the arena
 *
 * @param arena Pointer to the arena
 * @return Amount of memory currently allocated in bytes
 */
size_t arena_used(const coil_arena_t *arena);

/**
 * @brief Get the maximum size the arena can grow to
 *
 * @param arena Pointer to the arena
 * @return Maximum size in bytes (0 means unlimited)
 */
size_t arena_max_size(const coil_arena_t *arena);

/**
 * @brief Push an object into the arena, automatically resizing if needed
 *
 * This function allocates memory for an object of the specified size in the arena,
 * then copies the data from the provided pointer to the allocated memory.
 *
 * @param arena Pointer to the arena
 * @param data Pointer to the data to be copied into the arena
 * @param size Size of the data in bytes
 * @param alignment Memory alignment (must be a power of 2)
 * @return Pointer to the allocated memory in the arena, or NULL on failure
 */
void* arena_push(coil_arena_t *arena, const void* data, size_t size, size_t alignment);

/**
 * @brief Push an object into the arena with default alignment
 *
 * @param arena Pointer to the arena
 * @param data Pointer to the data to be copied into the arena
 * @param size Size of the data in bytes
 * @return Pointer to the allocated memory in the arena, or NULL on failure
 */
void* arena_push_default(coil_arena_t *arena, const void* data, size_t size);

#endif /* __COIL_INCLUDE_GUARD_ARENA_H */