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
	static const int64_t forkFromPrimechain;
	static const std::string currencyName;
	static const std::string tickerName;
	
public:
	uint256 GetPrimeBlockProof(const CBlockIndex& block);
	unsigned int GetPrimeWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock);
	bool CheckPrimeProofs(uint256 hashBlockHeader, unsigned int nBits, const CBigNum& bnProbablePrime, unsigned int& nChainType, unsigned int& nChainLength);
	CAmount GetPrimeBlockValue(int nBits, const CAmount& nFees);

	std::string getCurrencyName() { return currencyName; }
	std::string getSmallCurrencyName() { return currencyName; }
	std::string getLargeCurrencyName() { return currencyName; }
	std::string getTickerName() { return tickerName; }
	int64_t forkTime() { return forkFromPrimechain; }
}

#endif // PRIME_PARAMETERS_H
