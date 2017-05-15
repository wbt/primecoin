// Copyright (c) 2017 Ahmad A Kazi (Empinel/Plaxton) 
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PRIME_PARAMETERS_H
#define PRIME_PARAMETERS_H

#include "prime.h"
#include "bignum.h"

#include <string>

class PrimeCoin {

private:
	int64_t forkFromPrime = 0;
	int64_t nTargetSpacing = 60;
	int64_t nTargetTimespan = 604800;  // 7 * 24 * 60 * 60
	std::string currencyName = "Primecoin";
	std::string tickerName = "XPM";

public:
	uint256 GetPrimeBlockProof(const CBlockIndex& block);
	unsigned int GetPrimeWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock);
	bool CheckPrimeProofs(uint256 hashBlockHeader, unsigned int nBits, const CBigNum& bnProbablePrime, unsigned int& nChainType, unsigned int& nChainLength);
	CAmount GetPrimeBlockValue(int nBits, const CAmount& nFees);
	// void static PrimeMiner(CWallet *pwallet);
	// void GenerateBitcoins(bool fGenerate, CWallet* pwallet, int nThreads);

	std::string getCurrencyName() { return currencyName; }
	std::string getSmallCurrencyName() { return currencyName; }
	std::string getLargeCurrencyName() { return currencyName; }
	std::string getTickerName() { return tickerName; }
	int64_t forkTime() { return forkFromPrime; }

};

#endif // PRIME_PARAMETERS_H
