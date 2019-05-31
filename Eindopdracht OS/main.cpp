#include <iostream>

#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <time.h>
#include <thread>
//#include "Queue.h"
//#include "Block.h"
//#include "Audio.h"

using namespace std;
void bassCoefficients(int intensity, double* b0, double* b1, double* b2, double* a1, double* a2);
void trebleCoefficients(int intensity, double* b0, double* b1, double* b2, double* a1, double* a2);
const int BUFLEN = 5; // aantal in de queue
//void inputFile();

class Block
{
public:
	Block() {
		orderNr = 0;
		for (int i = 0; i < 1024; i++) sample[i] = 0;
	}

	signed int orderNr;
	signed short sample[1024];
};

class Queue
{
private:
	Block* buffer[BUFLEN];
	// Audio audio;
	// Twee indexen en de lengte bijhouden.
	// Redundant, maar lekker makkelijk!
	int treblePos, bassPos, outputPos, inputPos;
	int count;
	signed short sample;
	// Deze sync-dingen moet je gaan gebruiken in get() en put()
	CRITICAL_SECTION busy;
	HANDLE canTreble, canBass, canOutput, canInput;

public:
	Queue() {
		// Hier worden ze goed geinitialiseerd
		InitializeCriticalSection(&busy);
		canTreble = CreateEvent(0, 0, 0, 0);
		canBass = CreateEvent(0, 0, 0, 0);
		canOutput = CreateEvent(0, 0, 0, 0);
		sample = 0;
		inputPos = 0;
		treblePos = 0;
		bassPos = 0;
		outputPos = 0;
		count = 0;
		//for (int i = 0; i < BUFLEN; i++) buffer[i] = 0;
	}

	Block* input() {
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

	Block* treble(Block* block) {
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
	Block* bass(Block* block) {
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
	Block* output(Block* block) {
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

	static unsigned long __stdcall input(void* pVoid);
	static unsigned long __stdcall treble(void* pVoid);
	static unsigned long __stdcall bass(void* pVoid);
	static unsigned long __stdcall output(void* pVoid);
};

Queue queue;

unsigned long __stdcall function(void*)
{}

int _tmain(int argc, _TCHAR* argv[]){
	int p = int(argv[0]);	// number of threads
	int b = int(argv[1]);	// bass intensity
	int t = int(argv[2]);	// treble intensity
	int inputFile = int(argv[3]);
	int outputFile = int(argv[4]);

	CreateThread(0, 0, /*Queue::input*/ Queue::function, LPVOID("Input1"), 0, 0);
	//	CreateThread(0, 0, queue.treble, LPVOID("Treble1"), 0, 0);
	//	CreateThread(0, 0, queue.bass, LPVOID("Bass1"), 0, 0);
	//	CreateThread(0, 0, queue.output, LPVOID("Output1"), 0, 0);

	std::thread *thread[4];
	thread[0] = new std::thread(&Queue::input);
	thread[1] = new std::thread(&Queue::treble);
	thread[2] = new std::thread(&Queue::bass);
	thread[3] = new std::thread(&Queue::output);
	
	
	// Druk op een toets om af te breken...
	cin.get();
	return 0;
}

void inputFile()
{
	
}

void bassCoefficients(int intensity, double* b0, double* b1, double* b2, double* a1, double* a2)
{
	double frequency = 330;
	double qFactor = 0.5;
	double gain = intensity;
	double sampleRate = 44100;

	double pi = 4.0 * atan(1);
	double a = pow(10.0, gain / 40);
	double w0 = 2 * pi * frequency / sampleRate;
	double alpha = sin(w0) / (2.0 * qFactor);
	double a0 = (a + 1) + (a - 1) * cos(w0) + 2.0 * sqrt(a) * alpha;

	*a1 = -(-2.0 * ((a - 1) + (a + 1) * cos(w0))) / a0;
	*a2 = -((a + 1) + (a - 1) * cos(w0) - 2.0 * sqrt(a) * alpha) / a0;

	*b0 = (a * ((a + 1) - (a - 1) * cos(w0) + 2.0 * sqrt(a) * alpha)) / a0;
	*b1 = (2 * a * ((a - 1) - (a + 1) * cos(w0))) / a0;
	*b2 = (a * ((a + 1) - (a - 1) * cos(w0) - 2.0 * sqrt(a) * alpha)) / a0;
}

void trebleCoefficients(int intensity, double* b0, double* b1, double* b2, double* a1, double* a2)
{
	double frequency = 3300;
	double qFactor = 0.5;
	double gain = intensity;
	double sampleRate = 44100;

	double pi = 4.0 * atan(1);
	double a = pow(10.0, gain / 40);
	double w0 = 2 * pi * frequency / sampleRate;
	double alpha = sin(w0) / (2.0 * qFactor);
	double a0 = (a + 1) - (a - 1) * cos(w0) + 2.0 * sqrt(a) * alpha;

	*a1 = -(2.0 * ((a - 1) - (a + 1) * cos(w0))) / a0;
	*a2 = -((a + 1) - (a - 1) * cos(w0) - 2.0 * sqrt(a) * alpha) / a0;

	*b0 = (a * ((a + 1) + (a - 1) * cos(w0) + 2.0 * sqrt(a) * alpha)) / a0;
	*b1 = (-2.0 * a * ((a - 1) + (a + 1) * cos(w0))) / a0;
	*b2 = (a * ((a + 1) + (a - 1) * cos(w0) - 2.0 * sqrt(a) * alpha)) / a0;
}

