#pragma once


class Audio {
public:
	Audio();
	void inputFile();
	void bassCoefficients(int intensity, double* b0, double* b1, double* b2, double* a1, double* a2);
	void trebleCoefficients(int intensity, double* b0, double* b1, double* b2, double* a1, double* a2);


private:
	signed short sample;
};