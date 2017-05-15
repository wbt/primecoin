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
	// void static PrimeMiner(CWallet *pwallet);
	// void GenerateBitcoins(bool fGenerate, CWallet* pwallet, int nThreads);

	std::string getCurrencyName() { return currencyName; }
	std::string getSmallCurrencyName() { return currencyName; }
	std::string getLargeCurrencyName() { return currencyName; }
	std::string getTickerName() { return tickerName; }
	int64_t forkTime() { return forkFromPrimechain; }

};

class PrimeBlock {

public:
	uint256 hashBlock; // Primecoin: Persist block hash as well

	// Primecoin: proof-of-work certificate
 	// Multiplier to block hash to derive the probable prime chain (k=0, 1, ...)
 	// Cunningham Chain of first kind:  hash * multiplier * 2**k - 1
 	// Cunningham Chain of second kind: hash * multiplier * 2**k + 1
 	// BiTwin Chain:                    hash * multiplier * 2**k +/- 1
 	
 	CBigNum bnPrimeChainMultiplier;
 	int64_t nMoneySupply;

	uint32_t nPrimeChainType;
    uint32_t nPrimeChainLength;
    uint32_t nWorkTransition;
};

#endif // PRIME_PARAMETERS_H
