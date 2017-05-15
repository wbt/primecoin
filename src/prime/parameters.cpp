// Copyright (c) 2017 Ahmad A Kazi (Empinel/Plaxton) 
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "parameters.h"

static const std::string PrimeCoin::currencyName 			= "Primecoin";
static const std::string PrimeCoin::tickerName 				= "XPM";
static const int64_t 	 PrimeCoin::forkFromPrimechain 	= 0;
static const int64_t 	 PrimeCoin::nTargetSpacing 			= 60; // one minute block spacing
static const int64_t 	 PrimeCoin::nTargetTimespan			= 604800;  // 7 * 24 * 60 * 60 (one week)

uint256 PrimeCoin::GetPrimeBlockProof(const CBlockIndex& block)
{
    uint64_t nFractionalDifficulty = TargetGetFractionalDifficulty(nBits);
    CBigNum bnWork = 256;
    for (unsigned int nCount = nTargetMinLength; nCount < TargetGetLength(nBits); nCount++)
        bnWork *= nWorkTransitionRatio;

    bnWork *= ((uint64_t) nWorkTransitionRatio) * nFractionalDifficulty;
    bnWork /= (((uint64_t) nWorkTransitionRatio - 1) * nFractionalDifficultyMin + nFractionalDifficulty);

    return bnWork;
}

unsigned int PrimeCoin::GetPrimeWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock)
{
    unsigned int nBits = TargetGetLimit();

    // Genesis block
    if (pindexLast == NULL)
        return nBits;

    const CBlockIndex* pindexPrev = pindexLast;

    if (pindexPrev->pprev == NULL)
        return TargetGetInitial(); // first block

    const CBlockIndex* pindexPrevPrev = pindexPrev->pprev;

    if (pindexPrevPrev->pprev == NULL)
        return TargetGetInitial(); // second block

    // Bitcoin: continuous target adjustment on every block
    int64_t nInterval = nTargetTimespan / nTargetSpacing;
    int64_t nActualSpacing = pindexPrev->GetBlockTime() - pindexPrevPrev->GetBlockTime();
    if (!TargetGetNext(pindexPrev->nBits, nInterval, nTargetSpacing, nActualSpacing, nBits))
        return error("GetNextWorkRequired() : failed to get next target");

    return nBits;
}

bool PrimeCoin::CheckPrimeProofs(uint256 hashBlockHeader, unsigned int nBits, const CBigNum& bnProbablePrime, unsigned int& nChainType, unsigned int& nChainLength)
{
    if (!CheckPrimeProofOfWork(hashBlockHeader, nBits, bnProbablePrime, nChainType, nChainLength))
        return error("CheckProofOfWork() : check failed for prime proof-of-work");

    return true;
}

CAmount PrimeCoin::GetPrimeBlockValue(int nBits, const CAmount& nFees)
{
    uint64_t nSubsidy = 0;

    if (!TargetGetMint(nBits, nSubsidy))
        error("GetBlockValue() : invalid mint value");

    return ((CAmount)nSubsidy) + nFees;
}

// Block hash includes prime certificate
uint256 CBlockHeader::GetHash() const
{
    CDataStream ss(SER_GETHASH, 0);
    ss << nVersion << hashPrevBlock << hashMerkleRoot << nTime << nBits << nNonce << bnPrimeChainMultiplier;
    return Hash(ss.begin(), ss.end());
}

// Header hash does not include prime certificate
uint256 CBlockHeader::GetHeaderHash() const
{
    return Hash(BEGIN(nVersion), END(nNonce));
}
