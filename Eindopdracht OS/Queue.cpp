#include "Queue.h"

using namespace std;

Queue::Queue()
{
	// Hier worden ze goed geinitialiseerd
	InitializeCriticalSection(&busy);
	canTreble = CreateEvent(0, 0, 0, 0);
	canBass = CreateEvent(0, 0, 0, 0);
	canOutput = CreateEvent(0, 0, 0, 0);
	sample = 0;
	inputPos = 0;
	treblePos = 0;
	bassPos = 0;
	outputPos= 0;
	count = 0;
	//for (int i = 0; i < BUFLEN; i++) buffer[i] = 0;
}

Block* Queue::input()
{
	EnterCriticalSection(&busy);
	while (count == 1) {
		LeaveCriticalSection(&busy);
		WaitForSingleObject(canInput, INFINITE);
		EnterCriticalSection(&busy);
	}
	//audio.inputFile();
	Block* block = new Block;
	int i = 0;
	std::ifstream file("you_and_i.pcm", ios::binary);
	while (file.good() && i < 1024) {
		file.read((char*)sample, sizeof(sample));
		block->sample[i] = sample;
		i++;
	}
	block->orderNr = inputPos;
	buffer[inputPos] = new Block;
	buffer[inputPos] = block;
	treblePos = (inputPos + 1) % BUFLEN;
	//count--;
	SetEvent(canTreble);	// set next step
	ResetEvent(canInput);	// reset current step
	LeaveCriticalSection(&busy);

	return block;
}

Block* Queue::treble(Block* block)
{
	EnterCriticalSection(&busy);
	while (count == 1) {
		LeaveCriticalSection(&busy);
		WaitForSingleObject(canTreble, INFINITE);
		EnterCriticalSection(&busy);
	}

	block = buffer[treblePos];

	bassPos = (treblePos + 1) % BUFLEN;
	count--;
	SetEvent(canBass);	// set next step
	ResetEvent(canTreble);	// reset current step
	LeaveCriticalSection(&busy);

	return block;
}

Block* Queue::bass(Block* block)
{
	EnterCriticalSection(&busy);
	while (count == 1) {
		LeaveCriticalSection(&busy);
		WaitForSingleObject(canBass, INFINITE);
		EnterCriticalSection(&busy);
	}

	block = buffer[bassPos];
	outputPos = (bassPos + 1) % BUFLEN;
	count--;
	SetEvent(canOutput);	// set next step	
	ResetEvent(canBass);	// reset current step
	LeaveCriticalSection(&busy);

	return block;
}

Block* Queue::output(Block* block)
{
	EnterCriticalSection(&busy);
	while (count == 1) {
		LeaveCriticalSection(&busy);
		WaitForSingleObject(canOutput, INFINITE);
		EnterCriticalSection(&busy);
	}

	block = buffer[outputPos];
	inputPos = (outputPos + 1) % BUFLEN;
	count--;
	SetEvent(canInput);	// set next step (input, because the queue is smaller
	ResetEvent(canOutput);	// reset current step
	LeaveCriticalSection(&busy);

	return block;
}

unsigned long __stdcall Queue::input(void* pVoid)
{
	EnterCriticalSection(&busy);
	while (count == 1) {
		LeaveCriticalSection(&busy);
		WaitForSingleObject(canInput, INFINITE);
		EnterCriticalSection(&busy);
	}
	//audio.inputFile();
	Block* block = new Block;
	int i = 0;
	std::ifstream file("you_and_i.pcm", ios::binary);
	while (file.good() && i < 1024) {
		file.read((char*)sample, sizeof(sample));
		block->sample[i] = sample;
		i++;
	}
	block->orderNr = inputPos;
	buffer[inputPos] = new Block;
	buffer[inputPos] = block;
	treblePos = (inputPos + 1) % BUFLEN;
	//count--;
	SetEvent(canTreble);	// set next step
	ResetEvent(canInput);	// reset current step
	LeaveCriticalSection(&busy);

	return 0;
}

unsigned long __stdcall Queue::treble(void* pVoid)
{
	return 0;
}

unsigned long __stdcall Queue::bass(void* pVoid)
{
	return 0;
}

unsigned long __stdcall Queue::output(void* pVoid)
{
	return 0;
}
