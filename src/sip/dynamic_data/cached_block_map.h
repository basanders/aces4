/*
 * cached_block_map.h
 *
 *  Created on: Jul 5, 2014
 *      Author: njindal
 */

#ifndef CACHED_BLOCK_MAP_H_
#define CACHED_BLOCK_MAP_H_

#include "id_block_map.h"
#include "lru_array_policy.h"
#include "block.h"

namespace sip {

class BlockId;
/**
 * Block Map that caches blocks with a LRU array block replacement policy.
 * Delegates block operations to IdBlockMap<Block>
 */
class CachedBlockMap {
public:
	CachedBlockMap(int num_arrays);
	~CachedBlockMap();

	/** Obtains requested block
	 *
	 * If present only in cache, removes from cache and
	 * puts into block map before returning block
	 *
	 * @param block_id  BlockId of of desired block
	 * @return pointer to the requested block, or NULL if it is not in the map.
	 */
	Block* block(const BlockId& block_id);

	/**
	 * Inserts given block in the IdBlockMap.
	 * It is a fatal error to insert a block that already exists.
	 * @param id BlockId of block to insert
	 * @param block_ptr pointer to Block to insert
	 */
	void insert_block(const BlockId& block_id, Block* block_ptr);

	/**
	 * Removes given block from the map and caches it if possible
	 * Require that the block exist.
	 * @param block_id
	 */
	void cached_delete_block(const BlockId& block_id);

	/**
	 * Removes given block from block map.
	 * @param block_id
	 */
	void delete_block(const BlockId& block_id);

	/**
	 * Inserts the given PerArrayMap, updating the array_id in each block's ID to the given array_id.
	 * This is used to restore persistent arrays.  The array_id is not consistent across sial programs
	 * so must be updated here.
	 * @param array_id
	 * @param PerArrayMap*
	 */
	void insert_per_array_map(int array_id, IdBlockMap<Block>::PerArrayMap* map_ptr);

	/**
	 * Deletes the map containing blocks of the
	 * indicated array along with the blocks in the map.
	 * Also deletes them from the cached blocks.
	 * @param array_id
	 */
	void delete_per_array_map_and_blocks(int array_id);


	/**
	 * Returns a pointer to the PerArrayMap of the given array,
	 * and removes it from the map.  Contents of the map are preserved.
	 * If no PerArrayMap exists, an empty one is created and returned.
	 * This routine never returns NULL.
	 * @param array_id
	 * @return
	 */
	IdBlockMap<Block>::PerArrayMap* get_and_remove_per_array_map(int array_id);

	friend std::ostream& operator<<(std::ostream&, const CachedBlockMap&);

	/** Sets max_allocatable_bytes_ */
    void set_max_allocatable_bytes(std::size_t size);
	void free_up_bytes_in_cache(std::size_t block_size);

	/**
	 * Returns total number of blocks in the active map + cached mape.  Used for testing.
	 * @return
	 */
	std::size_t total_blocks(){
		return block_map_.total_blocks() + cache_.total_blocks();
	}

private:

	/* A block must be in only one of the two maps : block_map_ or cache_ */

	IdBlockMap<Block> block_map_; 	/*! Backing IdBlockMap */
	IdBlockMap<Block> cache_;		/*! Backing Cache */
	LRUArrayPolicy<Block> policy_;	/*! The block replacement Policy */

	/** Maximum number of bytes before deleting blocks from the cache_ */
	std::size_t max_allocatable_bytes_;

	/** Allocated bytes in blocks */
	std::size_t allocated_bytes_;
};

} /* namespace sip */

#endif /* CACHED_BLOCK_MAP_H_ */
