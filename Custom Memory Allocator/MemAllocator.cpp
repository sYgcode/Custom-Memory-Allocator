#include "MemAllocator.h"
#include <stdexcept>
#include <Windows.h>
#define MAX_SIZE 1024*1024*10
#define MULTIPLIER 8


/* Constructor for our Memory Allocator. Recieve an amount of memory to divide into chunks, and chunk size.
* request memory to set up our heap and metadata and initiate the metadata while accounting for exceptions.
*/
MemAllocator::MemAllocator(size_t memAmount = 1024, size_t chunkSize = 8) {
	// Check to make sure we don't use too much memory in our heap
	this->memSize = (memAmount > MAX_SIZE) ? MAX_SIZE : memAmount;
	// Align our chunk size to be divisble by 8, while rounding up.
	this->chunkSize = chunkSize + (MULTIPLIER - (chunkSize % MULTIPLIER));
	// Make sure our chunk size is not bigger then our heap
	if (chunkSize > this->memSize)
		this->chunkSize = this->memSize;

	// Allign our heap size to be divisible by our chunk size, align to the upper bound.
	this->memSize = memAmount + (chunkSize - (memAmount % this->chunkSize));
	// Get our chunk amount
	this->chunkAmount = this->memSize / this->chunkSize;

	// Make a system call to the kernel to retrieve memory to allocate for our metadata and for our heap. MEM_RESERVE - reserves a series of memory addresses virtually. 
	// MEM_COMMIT - actively assign physical memory to be used. PAGE_READWRITE - permission to read and write to the memory.
	this->headChunk = (Chunk*)VirtualAlloc(NULL, sizeof(Chunk) * chunkAmount, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	this->basePointer = VirtualAlloc(NULL ,memSize,MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	// Make sure that we stop any problems and de allocate memory in case some part of the allocation failed.
	if (this->basePointer == nullptr || this->headChunk == nullptr) {
		if (this->headChunk)
			VirtualFree(headChunk, 0, MEM_RELEASE);
		if (this->basePointer)
			VirtualFree(basePointer, 0, MEM_RELEASE);
		throw std::runtime_error("failed to allocate memory for heap\n");
	}
	
	// Initialize chunks
	Chunk* tempChunk = headChunk;
	for (size_t i = 0; i < chunkAmount - 1; ++i) {
		tempChunk->Reset();
		tempChunk->next = tempChunk + 1;
		tempChunk++;
	}
	// last chunk
	tempChunk->Reset();
	tempChunk->next = nullptr;
}

// Destructor to return the memory we took form the operating system
MemAllocator::~MemAllocator()
{
	// Free the heap memory. MEM_RELEASE - releases commited/reserved memory
	bool x = VirtualFree(basePointer, 0, MEM_RELEASE);
	// free the metadata memory
	bool y = VirtualFree(headChunk, 0, MEM_RELEASE);
	// Account for rare failures in freeing memory
	if (!(x) || !(y))
		throw std::runtime_error("could not free up the heap");
}


// Memory allocatior to assign a pointer space on the heap, while handling metadata
void* MemAllocator::Alloc(size_t size) {
	// Calculate the amount of chunks we will use. round up.
	size_t requiredChunks = (size + this->chunkSize - 1) / this->chunkSize;

	// Calculate the head chunk of the series of chunks we will be using
	Chunk* newMem = (Chunk*)firstFit(requiredChunks);
	// If there was no valid space throw a bad alloc error
	if (newMem == nullptr) {
		throw std::bad_alloc(); 
	}
	// update our head chunk to keep track of how many chunks were assigned to the pointer
	newMem->setSize(requiredChunks);

	// calculate the memory offset from the start of our chunks so that we can match the chunk with the appropriate spot in memory.
	size_t offset = (reinterpret_cast<uint8_t*>(newMem) - reinterpret_cast<uint8_t*>(headChunk));

	// Loop through all of the chunks we will use and update their size so that we know they are occupied
	for (int i = 0; i < requiredChunks; ++i) {
		newMem = newMem->next;
		newMem->setSize(--requiredChunks);
	}

	// Return the address in memory that corresponds to our head chunk.
	return static_cast<uint8_t*>(basePointer) + offset;
}


// Function to free the memory we assigned with alloc. While handling metadata.
void MemAllocator::Dealloc(void* mem) {
	// calculate the offset between the base pointer and our pointer so that we can find the appropriate chunk in the metadata.
	size_t offset = (reinterpret_cast<uint8_t*>(mem) - reinterpret_cast<uint8_t*>(basePointer));

	// Find the chunk corresponding to our memory
	Chunk* chunkToAccess = ((this->headChunk) + offset);

	// Find the amount of chunks we will need to free
	size_t memSize = chunkToAccess->size;

	// Update our metadata to refer to these chunks as free to use.
	for (int i = 0; i < memSize; ++i) {
		chunkToAccess->Reset();
		chunkToAccess = chunkToAccess->next;
	}

}

// Helper function implementing the first fit principle. This means it finds the first spot in memory that can allocate the memory we will need.
void* MemAllocator::firstFit(size_t requiredChunks) {
	size_t tracker = 0;
	Chunk* chunkTracker = this->headChunk;

	for (int i = 0; i < this->chunkAmount; ++i) {
		if (chunkTracker == nullptr) {
			break;
		}
		if (chunkTracker->isFree()) {
			++tracker;
			if (tracker == requiredChunks)
				return chunkTracker;
		}
		else {
			tracker = 0;
			chunkTracker = chunkTracker->next;
		}
		chunkTracker = chunkTracker->next;
	}

	return nullptr;
}