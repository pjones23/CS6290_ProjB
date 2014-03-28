#include "predictor.h"
#include <cmath>        // std::abs

#define PHT_CTR_MAX  3
#define PHT_CTR_INIT 2

#define WEIGHT_MAX 127 // 8 bit weight
#define WEIGHT_MIN -128 // 8 bit weight
#define WEIGHT_INIT 0

// Whenever changing settings, change HIST_LEN, NUM_WEIGHTS, NUM_Perceptions 2^x = (1<<x)
// 16 = (1<<4)
// 32 = (1<<5)
// 64 = (1<<6)
// 128 = (1<<7)
// 256 = (1<<8)
// 512 = (1<<9)
// 1024 = (1<<10)
// 2048  = (1<<11)
// 32KB = 262144 = (1<<18)
#define HIST_LEN 15
#define WEIGHT_SIZE (1<<3) // number of bits per weight
#define HW_BUGDGET (1<<18) // Hardware budget ( in bits ) = 32 KB = 32768 B = 262144 b
#define NUM_WEIGHTS (1<<4) // HIST_LEN+1 = 15 + 1
//#define NUM_PERCEPTRONS (HW_BUGDGET / (NUM_WEIGHTS * WEIGHT_SIZE)) //(hardwareBudget / (numWeights * numBitsPerWeight))
#define NUM_PERCEPTRONS (1<<11)

INT32 perceptronTbl[NUM_PERCEPTRONS][NUM_WEIGHTS]; // table of perceptrons

/////////////// STORAGE BUDGET JUSTIFICATION ////////////////
// Total storage budget: 32KB + 17 bits
// Total PHT counters: 2^17 
// Total PHT size = 2^17 * 2 bits/counter = 2^18 bits = 32KB
// GHR size: 17 bits
// Total Size = PHT size + GHR size
/////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

PREDICTOR::PREDICTOR(void) {

	historyLength = HIST_LEN;
	ghr = 0;

	// Initialize perceptron table
	// Hardware budget ( in bits ) = Number of perceptrons x Number of weights per perceptron x Number of bits per weight
	// Hardware budget ( in bits ) = 32 KB = 32768 B = 262144 b
	// Number of weights per perceptron = history length + 1
	// use the last weight as w0
	// Number of bits per weight = 8 bits
	// Threshold value: the best threshold, phi, for a given history length h is always exactly phi = 1.93*h + 14
	numWeights = NUM_WEIGHTS;
	numPerceptrons = NUM_PERCEPTRONS;

	threshold = (1.93 * historyLength) + 14;

	for (INT32 i = 0; i < numPerceptrons; i++) {
		perceptronTbl[i][numWeights - 1] = WEIGHT_INIT; // w0 = 0

		for (INT32 j = 0; j < numWeights; j++) {
			perceptronTbl[i][j] = WEIGHT_INIT;
		}
	}

	//cout << "history length: " << historyLength << endl;
	//cout << "# weights/perceptron: " << WEIGHT_SIZE << endl;
	//cout << "# weight: " << numWeights << endl;
	//cout << "# perceptrons: " << numPerceptrons << endl;
	//cout << "threshold: " << threshold << endl;

}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

bool PREDICTOR::GetPrediction(UINT32 PC) {

	// Perceptron branch prediction code
	// PC = branch address
	UINT32 perceptronIndex = getPerceptronIndex(PC);
	INT32 prediction = getPerceptronPrediction(perceptronIndex);

	//cout << "pred: " << prediction << endl;

	if (prediction >= 0) {
		return TAKEN;
	} else {
		return NOT_TAKEN;
	}

}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

void PREDICTOR::UpdatePredictor(UINT32 PC, bool resolveDir, bool predDir,
UINT32 branchTarget) {

	// resolveDir = 0 or 1
	// train
	INT32 t;
	if (resolveDir == TAKEN) {
		t = 1;
	} else {
		t = -1;
	}

	UINT32 perceptronIndex = getPerceptronIndex(PC);
	INT32 prediction = getPerceptronPrediction(perceptronIndex);

	//cout << "pred: " << prediction << endl;
	//cout << "pred dir: " << predictionSign << endl;
	//cout << "resolve dir: " << resolveDir << endl;

	if ((predDir != resolveDir) || (abs(prediction) <= threshold)) {
		//cout << "UPDATING!!!" << endl;
		INT32 x;
		for (INT32 i = 0; i < historyLength; ++i) {
			x = getBitOfGHR(i);
			if (x == t) {
				perceptronTbl[perceptronIndex][i] = saturatedWeightInc(perceptronTbl[perceptronIndex][i]);
			} else {
				perceptronTbl[perceptronIndex][i] = saturatedWeightDec(perceptronTbl[perceptronIndex][i]);
			}

			//cout << "Saturated Weight: " << perceptronTbl[perceptronIndex][i] << endl;
		}
		if(t == 1){
			perceptronTbl[perceptronIndex][numWeights - 1] = saturatedWeightInc(perceptronTbl[perceptronIndex][numWeights - 1]); //w0
		}
		else{
			perceptronTbl[perceptronIndex][numWeights - 1] = saturatedWeightDec(perceptronTbl[perceptronIndex][numWeights - 1]); //w0
		}

	}

	// update the GHR
	ghr = (ghr << 1);
	if (resolveDir == TAKEN) {
		ghr++;
	}

}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

void PREDICTOR::TrackOtherInst(UINT32 PC, OpType opType, UINT32 branchTarget) {

	// This function is called for instructions which are not
	// conditional branches, just in case someone decides to design
	// a predictor that uses information from such instructions.
	// We expect most contestants to leave this function untouched.

	return;
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

UINT32 PREDICTOR::getPerceptronIndex(UINT32 PC) {
	return (PC % numPerceptrons);
}

INT32 PREDICTOR::getPerceptronPrediction(UINT32 perceptronIndex) {
	INT32 pred = 0;
	// y = w0 + summation from i=1 to n(xi*wi)
	// xi from branch history(i) where 1 = taken, -1 = not taken

	// summation
	// xi = ghr[i] (branch history)
	// wi = perceptronTbl[perceptronIndex][i] (weights)
	INT32 x;
	for (INT32 i = 0; i < historyLength; ++i) {
		x = getBitOfGHR(i);
		pred += (x * perceptronTbl[perceptronIndex][i]);
	}

	// w0 + summation
	// summation = prediction_guess
	// w0 = perceptronTbl[perceptronIndex][numWeights - 1]
	pred += perceptronTbl[perceptronIndex][numWeights - 1];

	return pred;
}

INT32 PREDICTOR::saturatedWeightInc(INT32 originalWeight) {
	if (originalWeight < WEIGHT_MAX) {
		return originalWeight + 1;
	} else {
		return originalWeight;
	}
}

INT32 PREDICTOR::saturatedWeightDec(INT32 originalWeight) {
	if (originalWeight > WEIGHT_MIN) {
		return originalWeight - 1;
	} else {
		return originalWeight;
	}
}

INT32 PREDICTOR::getBitOfGHR(INT32 bitIndex) {
	INT32 bit = ghr >> bitIndex;
	bit = bit % 2;
	if (bit == 0) {
		bit = -1;
	}
	else{
		bit = 1;
	}
	return bit;
}

