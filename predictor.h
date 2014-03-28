#ifndef _PREDICTOR_H_
#define _PREDICTOR_H_

#include "utils.h"
#include "tracer.h"

#define LOG2(x) ( (x == 1 ) ? 0  : 		                  	\
                 ((x == (0x1 << 1 )) ? 1  :	              			\
		  ((x == (0x1 << 2 )) ? 2  :					\
		   ((x == (0x1 << 3 )) ? 3  :					\
		    ((x == (0x1 << 4 )) ? 4  :					\
		     ((x == (0x1 << 5 )) ? 5  :					\
		      ((x == (0x1 << 6 )) ? 6  :				\
		       ((x == (0x1 << 7 )) ? 7  :				\
			((x == (0x1 << 8 )) ? 8  :				\
			 ((x == (0x1 << 9 )) ? 9  :				\
			  ((x == (0x1 << 10)) ? 10 :				\
			   ((x == (0x1 << 11)) ? 11 :				\
			    ((x == (0x1 << 12)) ? 12 :				\
			     ((x == (0x1 << 13)) ? 13 :				\
			      ((x == (0x1 << 14)) ? 14 :			\
			       ((x == (0x1 << 15)) ? 15 :			\
				((x == (0x1 << 16)) ? 16 :			\
				 ((x == (0x1 << 17)) ? 17 :			\
				  ((x == (0x1 << 18)) ? 18 :			\
				   ((x == (0x1 << 19)) ? 19 :			\
				    ((x == (0x1 << 20)) ? 20 :			\
				     ((x == (0x1 << 21)) ? 21 :			\
				      ((x == (0x1 << 22)) ? 22 :		\
				       ((x == (0x1 << 23)) ? 23 :		\
					((x == (0x1 << 24)) ? 24 :		\
					 ((x == (0x1 << 25)) ? 25 :		\
					  ((x == (0x1 << 26)) ? 26 :		\
					   ((x == (0x1 << 27)) ? 27 :		\
					    ((x == (0x1 << 28)) ? 28 :		\
					     ((x == (0x1 << 29)) ? 29 :		\
					      ((x == (0x1 << 30)) ? 30 :        \
					       ((x == (0x1 << 31)) ? 31 : 1))))))))))))))))))))))))))))))))


/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

class PREDICTOR {

	// The state is defined for Gshare, change for your design

private:
	UINT64 ghr;          // global history register
	// UINT32 *pht;          // pattern history table
	INT32 historyLength; // history length
	// UINT32 numPhtEntries; // entries in pht

public:

	// The interface to the four functions below CAN NOT be changed

	PREDICTOR(void);
	bool GetPrediction(UINT32 PC);
	void UpdatePredictor(UINT32 PC, bool resolveDir, bool predDir,
			UINT32 branchTarget);
	void TrackOtherInst(UINT32 PC, OpType opType, UINT32 branchTarget);

	// Contestants can define their own functions below
	INT32 getPerceptronPrediction(UINT32 perceptronIndex);
	UINT32 getPerceptronIndex(UINT32 PC);
	INT32 saturatedWeightInc(INT32 originalWeight);
	INT32 saturatedWeightDec(INT32 originalWeight);
	INT32 getBitOfGHR(INT32 bitIndex);

	INT32 numPerceptrons; // number of perceptrons
	INT32 numWeights; 	   // number of weights in each perceptron
	INT32 threshold;

};

/***********************************************************/
#endif

