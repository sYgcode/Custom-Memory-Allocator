#include "MemAllocator.h"
#include <iostream>

int main()
{

	MemAllocator mem(1025, 9);
	int* testing = (int*)mem.Alloc(3);
	*testing = 4;
	std::cout << *testing << std::endl; 
	mem.Dealloc(testing);
	char* x = (char*)mem.Alloc(3);
	*x = '3';
	std::cout << *testing << std::endl;

	int* testing1 = (int*)malloc(3);
	*testing1 = 4;
	std::cout << *testing1 << std::endl;
	free(testing);
	char* x1 = (char*)malloc(3);
	*x1 = '3';
	std::cout << *testing1 << std::endl;

	free(x1);
	mem.Dealloc(x);
}