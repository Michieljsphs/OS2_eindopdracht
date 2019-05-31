#include <iostream>

#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <time.h>
#include <thread>
#include "Queue.h"
#include "Block.h"
//#include "Audio.h"

using namespace std;
void bassCoefficients(int intensity, double* b0, double* b1, double* b2, double* a1, double* a2);
void trebleCoefficients(int intensity, double* b0, double* b1, double* b2, double* a1, double* a2);
void inputFile();


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

