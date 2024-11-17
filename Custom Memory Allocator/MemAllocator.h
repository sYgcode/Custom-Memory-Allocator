#pragma once

class MemAllocator
{
public:
	// Construct our Allocator. memamount & chunksize in bytes. 
	MemAllocator(size_t memAmount, size_t chunkSize);
	// Destruct our allocator and return the memory we borrowed to the OS
	~MemAllocator();
	// Allocate memory to fit the user's request. size in bytes.
	void* Alloc(size_t size);
	// Free the memory at the pointer
	void Dealloc(void* mem);

private:
	// struct to represent esch memory chunk
	struct Chunk
	{
		size_t size = 0;
		Chunk* next = nullptr;
		inline void Reset() {
			size = 0;
		}
		inline void setSize(size_t size) {
			this->size = size;
		}
		inline bool isFree() { return !(size > 0); }
	};

	// bitmap of our struct, saved as a linked list
	Chunk* headChunk;
	// pointer to the beginning of our memory
	void* basePointer;
	// size of each chunk
	size_t chunkSize;
	// amount of chunks (length of bitmap)
	size_t chunkAmount;
	// dynamic memory size
	size_t memSize;

	void* firstFit(size_t requiredChunks);
};
