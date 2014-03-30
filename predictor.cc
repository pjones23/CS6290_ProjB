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

INT32 histPerceptronTbl[NUM_PERCEPTRONS/2][NUM_WEIGHTS]; // table of history-based perceptrons
INT32 addrPerceptronTbl[NUM_PERCEPTRONS/2][NUM_WEIGHTS]; // table of address-based perceptrons

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
	pt = new list<UINT32>();

	// Initialize perceptron table
	// Hardware budget ( in bits ) = Number of perceptrons x Number of weights per perceptron x Number of bits per weight
	// Hardware budget ( in bits ) = 32 KB = 32768 B = 262144 b
	// Number of weights per perceptron = history length + 1
	// use the last weight as w0
	// Number of bits per weight = 8 bits
	// Threshold value: the best threshold, phi, for a given history length h is always exactly phi = 1.93*h + 14
	numWeights = NUM_WEIGHTS;
	numPerceptrons = NUM_PERCEPTRONS/2;

	threshold = (1.93 * historyLength) + 14;

	// history-based perceptron table initialization
	for (INT32 i = 0; i < numPerceptrons; i++) {
		histPerceptronTbl[i][numWeights - 1] = WEIGHT_INIT; // w0 = 0

		for (INT32 j = 0; j < numWeights; j++) {
			histPerceptronTbl[i][j] = WEIGHT_INIT;
		}
	}

	// address-based perceptron table initialization
	for (INT32 i = 0; i < numPerceptrons; i++) {
			addrPerceptronTbl[i][numWeights - 1] = WEIGHT_INIT; // w0 = 0

			for (INT32 j = 0; j < numWeights; j++) {
				addrPerceptronTbl[i][j] = WEIGHT_INIT;
			}
		}

	cout << "history length: " << historyLength << endl;
	cout << "# weights/perceptron: " << WEIGHT_SIZE << endl;
	cout << "# weight: " << numWeights << endl;
	cout << "# perceptrons: " << (numPerceptrons *2) << endl;
	cout << "threshold: " << threshold << endl;

}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

bool PREDICTOR::GetPrediction(UINT32 PC) {

	// Perceptron branch prediction code
	// PC = branch address
	UINT32 perceptronIndex = getPerceptronIndex(PC);
	INT32 prediction = getPerceptronPrediction(perceptronIndex, PC);

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
	INT32 prediction = getPerceptronPrediction(perceptronIndex, PC);

	//cout << "pred: " << prediction << endl;
	//cout << "pred dir: " << predictionSign << endl;
	//cout << "resolve dir: " << resolveDir << endl;

	if ((predDir != resolveDir) || (abs(prediction) <= threshold)) {
		//cout << "UPDATING!!!" << endl;
		INT32 x;
		UINT32 pathPerceptronIndex;
		for (INT32 i = 0; i < historyLength; ++i) {
			x = getBitOfGHR(i);
			pathPerceptronIndex = getPathPerceptronIndex(i);
			if (x == t) {
				histPerceptronTbl[pathPerceptronIndex][i] = saturatedWeightInc(histPerceptronTbl[pathPerceptronIndex][i]);
			} else {
				histPerceptronTbl[pathPerceptronIndex][i] = saturatedWeightDec(histPerceptronTbl[pathPerceptronIndex][i]);
			}

			//cout << "Saturated Weight: " << perceptronTbl[perceptronIndex][i] << endl;
		}
		if(t == 1){
			histPerceptronTbl[perceptronIndex][numWeights - 1] = saturatedWeightInc(histPerceptronTbl[perceptronIndex][numWeights - 1]); //w0
		}
		else{
			histPerceptronTbl[perceptronIndex][numWeights - 1] = saturatedWeightDec(histPerceptronTbl[perceptronIndex][numWeights - 1]); //w0
		}
		// -------------------------------------------------------------
		INT32 y;
		for (INT32 j = 0; j < historyLength; ++j) {
			y = getBitOfPC(j, PC);
			pathPerceptronIndex = getPathPerceptronIndex(j);
			if (y == t) {
				addrPerceptronTbl[pathPerceptronIndex][j] = saturatedWeightInc(addrPerceptronTbl[pathPerceptronIndex][j]);
			} else {
				addrPerceptronTbl[pathPerceptronIndex][j] = saturatedWeightDec(addrPerceptronTbl[pathPerceptronIndex][j]);
			}
				//cout << "Saturated Weight: " << perceptronTbl[perceptronIndex][i] << endl;
		}
		if(t == 1){
			addrPerceptronTbl[perceptronIndex][numWeights - 1] = saturatedWeightInc(addrPerceptronTbl[perceptronIndex][numWeights - 1]); //w0
		}
		else{
			addrPerceptronTbl[perceptronIndex][numWeights - 1] = saturatedWeightDec(addrPerceptronTbl[perceptronIndex][numWeights - 1]); //w0
		}

	}
	// -------------------------------------------------------------
	// update the GHR
	ghr = (ghr << 1);
	if (resolveDir == TAKEN) {
		ghr++;
	}

	// update path table
	if(pt->size() > (UINT32)(historyLength))
	{
		pt->pop_front();
	}
	pt->push_back(PC);

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

UINT32 PREDICTOR::getPathPerceptronIndex(INT32 index)
{
	list<UINT32>::iterator cii;
	cii = pt->begin();
	advance(cii,index);
	UINT32 pastPC = *cii;
	return (pastPC % numPerceptrons);
}

INT32 PREDICTOR::getPerceptronPrediction(UINT32 perceptronIndex, UINT32 PC) {
	INT32 histPred = 0;
	// y = w0 + summation from i=1 to n(xi*wi)
	// xi from branch history(i) where 1 = taken, -1 = not taken

	// summation
	// xi = ghr[i] (branch history)
	// wi = perceptronTbl[perceptronIndex][i] (weights)
	INT32 x;
	UINT32 pathPerceptronIndex;
	for (INT32 i = 0; i < historyLength; ++i) {
		x = getBitOfGHR(i);
		pathPerceptronIndex = getPathPerceptronIndex(i);
		histPred += (x * histPerceptronTbl[pathPerceptronIndex][i]);
	}

	// w0 + summation
	// summation = prediction_guess
	// w0 = perceptronTbl[perceptronIndex][numWeights - 1]
	histPred += histPerceptronTbl[perceptronIndex][numWeights - 1];
	// -------------------------------------------------------------
	INT32 addrPred = 0;
	// y = w0 + summation from i=1 to n(xi*wi)
	// xi from branch history(i) where 1 = taken, -1 = not taken

	// summation
	// xi = PC[i] (address bits)
	// wi = perceptronTbl[perceptronIndex][i] (weights)
	INT32 y;
	for (INT32 j = 0; j < historyLength; ++j) {
		y = getBitOfPC(j, PC);
		pathPerceptronIndex = getPathPerceptronIndex(j);
		addrPred += (y * addrPerceptronTbl[pathPerceptronIndex][j]);
	}

	// w0 + summation
	// summation = prediction_guess
	// w0 = perceptronTbl[perceptronIndex][numWeights - 1]
	addrPred += addrPerceptronTbl[perceptronIndex][numWeights - 1];
	// -------------------------------------------------------------
	return histPred + addrPred;
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

INT32 PREDICTOR::getBitOfPC(INT32 bitIndex, UINT32 PC) {
	INT32 bit = PC >> bitIndex;
	bit = bit % 2;
	if (bit == 0) {
		bit = -1;
	}
	else{
		bit = 1;
	}
	return bit;
}
