// Copyright (c) 2013 Primecoin developers
// Copyright (c) 2017 Ahmad A Kazi (Empinel/Plaxton)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <prime/prime.h>
#include <prime/bignum.h>

#include <boost/foreach.hpp>

// Prime Table
std::vector<unsigned int> vPrimes;
static const unsigned int nPrimeTableLimit = nMaxSieveSize;

void GeneratePrimeTable()
{
    vPrimes.clear();
    // Generate prime table using sieve of Eratosthenes
    std::vector<bool> vfComposite (nPrimeTableLimit, false);
    for (unsigned int nFactor = 2; nFactor * nFactor < nPrimeTableLimit; nFactor++)
    {
        if (vfComposite[nFactor])
            continue;
        for (unsigned int nComposite = nFactor * nFactor; nComposite < nPrimeTableLimit; nComposite += nFactor)
            vfComposite[nComposite] = true;
    }
    for (unsigned int n = 2; n < nPrimeTableLimit; n++)
        if (!vfComposite[n])
            vPrimes.push_back(n);
    LogPrint(BCLog::PRIME, "GeneratePrimeTable() : prime table [1, %u] generated with %u primes\n", nPrimeTableLimit, (unsigned int) vPrimes.size());
}

// Get next prime number of p
bool PrimeTableGetNextPrime(unsigned int& p)
{
    BOOST_FOREACH(unsigned int nPrime, vPrimes)
    {
        if (nPrime > p)
        {
            p = nPrime;
            return true;
        }
    }
    return false;
}

// Get previous prime number of p
bool PrimeTableGetPreviousPrime(unsigned int& p)
{
    unsigned int nPrevPrime = 0;
    BOOST_FOREACH(unsigned int nPrime, vPrimes)
    {
        if (nPrime >= p)
            break;
        nPrevPrime = nPrime;
    }
    if (nPrevPrime)
    {
        p = nPrevPrime;
        return true;
    }
    return false;
}

// Compute Primorial number p#
void Primorial(unsigned int p, CBigNum& bnPrimorial)
{
    bnPrimorial = 1;
    BOOST_FOREACH(unsigned int nPrime, vPrimes)
    {
        if (nPrime > p) break;
        bnPrimorial *= nPrime;
    }
}

// Compute first primorial number greater than or equal to pn
void PrimorialAt(CBigNum& bn, CBigNum& bnPrimorial)
{
    bnPrimorial = 1;
    BOOST_FOREACH(unsigned int nPrime, vPrimes)
    {
        bnPrimorial *= nPrime;
        if (bnPrimorial >= bn)
            return;
    }
}

// Check Fermat probable primality test (2-PRP): 2 ** (n-1) = 1 (mod n)
// true: n is probable prime
// false: n is composite; set fractional length in the nLength output
static bool FermatProbablePrimalityTest(const CBigNum& n, unsigned int& nLength)
{
    CAutoBN_CTX pctx;
    CBigNum a = 2; // base; Fermat witness
    CBigNum e = n - 1;
    CBigNum r;
    BN_mod_exp(&r, &a, &e, &n, pctx);
    if (r == 1)
        return true;
    // Failed Fermat test, calculate fractional length
    unsigned int nFractionalLength = (((n-r) << nFractionalBits) / n).getuint();
    if (nFractionalLength >= (1 << nFractionalBits)) {
        LogPrint(BCLog::PRIME, "FermatProbablePrimalityTest() : fractional assert");
		return false;
	}
    nLength = (nLength & TARGET_LENGTH_MASK) | nFractionalLength;
    return false;
}

// Test probable primality of n = 2p +/- 1 based on Euler, Lagrange and Lifchitz
// fSophieGermain:
//   true:  n = 2p+1, p prime, aka Cunningham Chain of first kind
//   false: n = 2p-1, p prime, aka Cunningham Chain of second kind
// Return values
//   true: n is probable prime
//   false: n is composite; set fractional length in the nLength output
static bool EulerLagrangeLifchitzPrimalityTest(const CBigNum& n, bool fSophieGermain, unsigned int& nLength)
{
    CAutoBN_CTX pctx;
    CBigNum a = 2;
    CBigNum e = (n - 1) >> 1;
    CBigNum r;
    BN_mod_exp(&r, &a, &e, &n, pctx);
    CBigNum nMod8 = n % 8;
    bool fPassedTest = false;
    if (fSophieGermain && (nMod8 == 7)) // Euler & Lagrange
        fPassedTest = (r == 1);
    else if (fSophieGermain && (nMod8 == 3)) // Lifchitz
        fPassedTest = ((r+1) == n);
    else if ((!fSophieGermain) && (nMod8 == 5)) // Lifchitz
        fPassedTest = ((r+1) == n);
    else if ((!fSophieGermain) && (nMod8 == 1)) // LifChitz
        fPassedTest = (r == 1);
    else {
        LogPrint(BCLog::PRIME, "EulerLagrangeLifchitzPrimalityTest() : invalid n %% 8 = %d, %s", nMod8.getint(), (fSophieGermain? "first kind" : "second kind"));
		return false;
	}
	
    if (fPassedTest)
        return true;
    // Failed test, calculate fractional length
    r = (r * r) % n; // derive Fermat test remainder
    unsigned int nFractionalLength = (((n-r) << nFractionalBits) / n).getuint();
    if (nFractionalLength >= (1 << nFractionalBits)) {
        LogPrint(BCLog::PRIME, "EulerLagrangeLifchitzPrimalityTest() : fractional assert");
        return false;
	}
    nLength = (nLength & TARGET_LENGTH_MASK) | nFractionalLength;
    return false;
}

// Proof-of-work Target (prime chain target):
//   format - 32 bit, 8 length bits, 24 fractional length bits

unsigned int TargetGetLimit(const Consensus::Params& consensus_params)
{
    return (consensus_params.nTargetMinLength << nFractionalBits);
}

unsigned int TargetGetInitial(const Consensus::Params& consensus_params)
{
    return (consensus_params.nTargetInitialLength << nFractionalBits);
}

unsigned int TargetGetLength(unsigned int nBits)
{
    return ((nBits & TARGET_LENGTH_MASK) >> nFractionalBits);
}

bool TargetSetLength(unsigned int nLength, unsigned int& nBits)
{
    if (nLength >= 0xff) {
		LogPrint(BCLog::PRIME, "TargetSetLength() : invalid length=%u", nLength);
        return false;
    }
    nBits &= TARGET_FRACTIONAL_MASK;
    nBits |= (nLength << nFractionalBits);
    return true;
}

void TargetIncrementLength(unsigned int& nBits)
{
    nBits += (1 << nFractionalBits);
}

static void TargetDecrementLength(unsigned int& nBits, const Consensus::Params& consensus_params)
{
    if (TargetGetLength(nBits) > consensus_params.nTargetMinLength)
        nBits -= (1 << nFractionalBits);
}

unsigned int TargetGetFractional(unsigned int nBits)
{
    return (nBits & TARGET_FRACTIONAL_MASK);
}

uint64_t TargetGetFractionalDifficulty(unsigned int nBits)
{
    return (nFractionalDifficultyMax / (uint64_t) ((1llu<<nFractionalBits) - TargetGetFractional(nBits)));
}

bool TargetSetFractionalDifficulty(uint64_t nFractionalDifficulty, unsigned int& nBits)
{
    if (nFractionalDifficulty < nFractionalDifficultyMin) {
        LogPrint(BCLog::PRIME, "TargetSetFractionalDifficulty() : difficulty below min");
        return false;
	}
    uint64_t nFractional = nFractionalDifficultyMax / nFractionalDifficulty;
    if (nFractional > (1u<<nFractionalBits)) {
        LogPrint(BCLog::PRIME, "TargetSetFractionalDifficulty() : fractional overflow: nFractionalDifficulty=%ld\n", (long)nFractionalDifficulty);
		return false;
    }
    nFractional = (1u<<nFractionalBits) - nFractional;
    nBits &= TARGET_LENGTH_MASK;
    nBits |= (unsigned int)nFractional;
    return true;
}

std::string TargetToString(unsigned int nBits)
{
    return strprintf("%02x.%06x", TargetGetLength(nBits), TargetGetFractional(nBits));
}

unsigned int TargetFromInt(unsigned int nLength)
{
    return (nLength << nFractionalBits);
}

// Get mint value from target
// Primecoin mint rate is determined by target
//   mint = 999 / (target length ** 2)
// Inflation is controlled via Moore's Law
bool TargetGetMint(unsigned int nBits, uint64_t& nMint, const Consensus::Params& consensus_params)
{
    nMint = 0;
    static uint64_t nMintLimit = 999llu * COIN;
    CBigNum bnMint = nMintLimit;
    if (TargetGetLength(nBits) < consensus_params.nTargetMinLength) {
        LogPrint(BCLog::PRIME, "TargetGetMint() : length below minimum required, nBits=%08x", nBits);
        return false;
	}
    bnMint = (bnMint << nFractionalBits) / nBits;
    bnMint = (bnMint << nFractionalBits) / nBits;
    bnMint = (bnMint / CENT) * CENT;  // mint value rounded to cent
    nMint = UintToArith256(bnMint.getuint256()).GetLow64();
    if (nMint > nMintLimit)
    {
        nMint = 0;
        LogPrint(BCLog::PRIME, "TargetGetMint() : mint value over limit, nBits=%08x", nBits);
        
        return false;
    }
    return true;
}

// Get next target value
bool TargetGetNext(unsigned int nBits, int64_t nInterval, int64_t nTargetSpacing, int64_t nActualSpacing, unsigned int& nBitsNext, const Consensus::Params& consensus_params)
{
    nBitsNext = nBits;
    // Convert length into fractional difficulty
    uint64_t nFractionalDifficulty = TargetGetFractionalDifficulty(nBits);
    // Compute new difficulty via exponential moving toward target spacing
    CBigNum bnFractionalDifficulty = nFractionalDifficulty;
    bnFractionalDifficulty *= ((nInterval + 1) * nTargetSpacing);
    bnFractionalDifficulty /= ((nInterval - 1) * nTargetSpacing + nActualSpacing + nActualSpacing);
    if (bnFractionalDifficulty > nFractionalDifficultyMax)
        bnFractionalDifficulty = nFractionalDifficultyMax;
    if (bnFractionalDifficulty < nFractionalDifficultyMin)
        bnFractionalDifficulty = nFractionalDifficultyMin;

    uint64_t nFractionalDifficultyNew = UintToArith256(bnFractionalDifficulty.getuint256()).GetLow64();

    LogPrint(BCLog::PRIME, "TargetGetNext() : nActualSpacing=%d nFractionDiff=%ld nFractionDiffNew=%ld\n", (int)nActualSpacing, (long)nFractionalDifficulty, (long)nFractionalDifficultyNew);
    // Step up length if fractional past threshold
    if (nFractionalDifficultyNew > nFractionalDifficultyThreshold)
    {
        nFractionalDifficultyNew = nFractionalDifficultyMin;
        TargetIncrementLength(nBitsNext);
    }
    // Step down length if fractional at minimum
    else if (nFractionalDifficultyNew == nFractionalDifficultyMin && TargetGetLength(nBitsNext) > consensus_params.nTargetMinLength)
    {
        nFractionalDifficultyNew = nFractionalDifficultyThreshold;
        TargetDecrementLength(nBitsNext, consensus_params);
    }
    // Convert fractional difficulty back to length
    if (!TargetSetFractionalDifficulty(nFractionalDifficultyNew, nBitsNext)) {
        LogPrint(BCLog::PRIME, "TargetGetNext() : unable to set fractional difficulty prev=%ld new=%ld\n", (long)nFractionalDifficulty, (long)nFractionalDifficultyNew);
        return false;
	}
    return true;
}

// prime chain type and length value
std::string GetPrimeChainName(unsigned int nChainType, unsigned int nChainLength)
{
    const std::string strLabels[5] = {"NUL", "1CC", "2CC", "TWN", "UNK"};
    return strprintf("%s%s", strLabels[std::min(nChainType, 4u)].c_str(), TargetToString(nChainLength).c_str());
}

// primorial form of prime chain origin
std::string GetPrimeOriginPrimorialForm(CBigNum& bnPrimeChainOrigin)
{
    CBigNum bnNonPrimorialFactor = bnPrimeChainOrigin;
    unsigned int nPrimeSeq = 0;
    while (nPrimeSeq < vPrimes.size() && bnNonPrimorialFactor % vPrimes[nPrimeSeq] == 0)
    {
        bnNonPrimorialFactor /= vPrimes[nPrimeSeq];
        nPrimeSeq++;
    }
    return strprintf("%s*%u#", bnNonPrimorialFactor.ToString().c_str(), (nPrimeSeq > 0)? vPrimes[nPrimeSeq-1] : 0);
}

// Test Probable Cunningham Chain for: n
// fSophieGermain:
//   true - Test for Cunningham Chain of first kind (n, 2n+1, 4n+3, ...)
//   false - Test for Cunningham Chain of second kind (n, 2n-1, 4n-3, ...)
// Return value:
//   true - Probable Cunningham Chain found (length at least 2)
//   false - Not Cunningham Chain
static bool ProbableCunninghamChainTest(const CBigNum& n, bool fSophieGermain, bool fFermatTest, unsigned int& nProbableChainLength)
{
    nProbableChainLength = 0;
    CBigNum N = n;

    // Fermat test for n first
    if (!FermatProbablePrimalityTest(N, nProbableChainLength))
        return false;

    // Euler-Lagrange-Lifchitz test for the following numbers in chain
    while (true)
    {
        TargetIncrementLength(nProbableChainLength);
        N = N + N + (fSophieGermain? 1 : (-1));
        if (fFermatTest)
        {
            if (!FermatProbablePrimalityTest(N, nProbableChainLength))
                break;
        }
        else
        {
            if (!EulerLagrangeLifchitzPrimalityTest(N, fSophieGermain, nProbableChainLength))
                break;
        }
    }

    return (TargetGetLength(nProbableChainLength) >= 2);
}

// Test probable prime chain for: nOrigin
// Return value:
//   true - Probable prime chain found (one of nChainLength meeting target)
//   false - prime chain too short (none of nChainLength meeting target)
bool ProbablePrimeChainTest(const CBigNum& bnPrimeChainOrigin, unsigned int nBits, bool fFermatTest, unsigned int& nChainLengthCunningham1, unsigned int& nChainLengthCunningham2, unsigned int& nChainLengthBiTwin)
{
    nChainLengthCunningham1 = 0;
    nChainLengthCunningham2 = 0;
    nChainLengthBiTwin = 0;

    // Test for Cunningham Chain of first kind
    ProbableCunninghamChainTest(bnPrimeChainOrigin-1, true, fFermatTest, nChainLengthCunningham1);
    // Test for Cunningham Chain of second kind
    ProbableCunninghamChainTest(bnPrimeChainOrigin+1, false, fFermatTest, nChainLengthCunningham2);
    // Figure out BiTwin Chain length
    // BiTwin Chain allows a single prime at the end for odd length chain
    nChainLengthBiTwin =
        (TargetGetLength(nChainLengthCunningham1) > TargetGetLength(nChainLengthCunningham2))?
            (nChainLengthCunningham2 + TargetFromInt(TargetGetLength(nChainLengthCunningham2)+1)) :
            (nChainLengthCunningham1 + TargetFromInt(TargetGetLength(nChainLengthCunningham1)));

    return (nChainLengthCunningham1 >= nBits || nChainLengthCunningham2 >= nBits || nChainLengthBiTwin >= nBits);
}

// Fast check block header integrity
bool CheckBlockHeaderIntegrity(uint256 hashBlockHeader, unsigned int nBits, const CBigNum& bnPrimeChainMultiplier, const Consensus::Params& consensus_params)
{
    // Check target
    if (TargetGetLength(nBits) < consensus_params.nTargetMinLength || TargetGetLength(nBits) > 99) {
        LogPrint(BCLog::PRIME, "CheckBlockHeaderIntegrity() : invalid chain length target %s", TargetToString(nBits).c_str());
        return false;
    }

    // Check header hash limit
    if (UintToArith256(hashBlockHeader) < hashBlockHeaderLimit) {
        LogPrint(BCLog::PRIME, "CheckBlockHeaderIntegrity() : block header hash under limit: %s", hashBlockHeader.ToString().c_str());
        return false;
    }
    // Check target for prime proof-of-work
    CBigNum bnPrimeChainOrigin = CBigNum(hashBlockHeader) * bnPrimeChainMultiplier;
    if (bnPrimeChainOrigin < bnPrimeMin) {
        LogPrint(BCLog::PRIME, "CheckBlockHeaderIntegrity() : prime too small");
        return false;
    }
    // First prime in chain must not exceed cap
    if (bnPrimeChainOrigin > bnPrimeMax) {
        LogPrint(BCLog::PRIME, "CheckBlockHeaderIntegrity() : prime too big");
        return false;
    }

    // Proof-of-work check contains Fermat test of prime chain origin, it takes a lot of time,
    // typical CPU can do ~60-70k Fermat tests per second in single thread, current block
    // height at moment near 2.7M. So, we can't use Fermat test at startup for checking block headers
    // in index database. Also, we can't use trial division test for preliminary check, because
    // prime chain origin can be Carmichael number than can pass CheckPrimeProofOfWork function,
    // but not pass another primality tests.

    // For adding any check here, we need add it to CheckPrimeProofOfWork before
    return true;
}

// Check prime proof-of-work
bool CheckPrimeProofOfWork(uint256 hashBlockHeader, unsigned int nBits, const CBigNum& bnPrimeChainMultiplier, unsigned int& nChainType, unsigned int& nChainLength, const Consensus::Params& consensus_params)
{
    nChainType = 0;   // clear output chain type
    nChainLength = 0; // clear output chain length

    // Check target
    if (TargetGetLength(nBits) < consensus_params.nTargetMinLength || TargetGetLength(nBits) > 99) {
        LogPrintf("CheckPrimeProofOfWork() : invalid chain length target %s", TargetToString(nBits).c_str());
        return false;
	}
	
    // Check header hash limit
    if (UintToArith256(hashBlockHeader) < hashBlockHeaderLimit) {
        LogPrintf("CheckPrimeProofOfWork() : block header hash under limit: %s", hashBlockHeader.ToString().c_str());
        return false;
	}
    // Check target for prime proof-of-work
    CBigNum bnPrimeChainOrigin = CBigNum(hashBlockHeader) * bnPrimeChainMultiplier;
    if (bnPrimeChainOrigin < bnPrimeMin) {
        LogPrintf("CheckPrimeProofOfWork() : prime too small");
        return false;
	}
    // First prime in chain must not exceed cap
    if (bnPrimeChainOrigin > bnPrimeMax) {
        LogPrintf("CheckPrimeProofOfWork() : prime too big");
        return false;
	}

    // Check prime chain
    unsigned int nChainLengthCunningham1 = 0;
    unsigned int nChainLengthCunningham2 = 0;
    unsigned int nChainLengthBiTwin = 0;
    if (!ProbablePrimeChainTest(bnPrimeChainOrigin, nBits, false, nChainLengthCunningham1, nChainLengthCunningham2, nChainLengthBiTwin)) {
        // Despite failing the check, still return info of longest primechain from the three chain types
        nChainLength = nChainLengthCunningham1;
        nChainType = PRIME_CHAIN_CUNNINGHAM1;
        if (nChainLengthCunningham2 > nChainLength)
        {
            nChainLength = nChainLengthCunningham2;
            nChainType = PRIME_CHAIN_CUNNINGHAM2;
        }
        if (nChainLengthBiTwin > nChainLength)
        {
            nChainLength = nChainLengthBiTwin;
            nChainType = PRIME_CHAIN_BI_TWIN;
        }
        LogPrint(BCLog::PRIME, "CheckPrimeProofOfWork() : failed prime chain test target=%s length=(%s %s %s)", TargetToString(nBits).c_str(), TargetToString(nChainLengthCunningham1).c_str(), TargetToString(nChainLengthCunningham2).c_str(), TargetToString(nChainLengthBiTwin).c_str());
        return false;
    }
    if (nChainLengthCunningham1 < nBits && nChainLengthCunningham2 < nBits && nChainLengthBiTwin < nBits) {
        LogPrint(BCLog::PRIME, "CheckPrimeProofOfWork() : prime chain length assert target=%s length=(%s %s %s)", TargetToString(nBits).c_str(),
				TargetToString(nChainLengthCunningham1).c_str(), TargetToString(nChainLengthCunningham2).c_str(), TargetToString(nChainLengthBiTwin).c_str());
        return false; 
	}
    // Double check prime chain with Fermat tests only
    unsigned int nChainLengthCunningham1FermatTest = 0;
    unsigned int nChainLengthCunningham2FermatTest = 0;
    unsigned int nChainLengthBiTwinFermatTest = 0;
    if (!ProbablePrimeChainTest(bnPrimeChainOrigin, nBits, true, nChainLengthCunningham1FermatTest, nChainLengthCunningham2FermatTest, nChainLengthBiTwinFermatTest)) {
        LogPrint(BCLog::PRIME, "CheckPrimeProofOfWork() : failed Fermat test target=%s length=(%s %s %s) lengthFermat=(%s %s %s)", TargetToString(nBits).c_str(),
            TargetToString(nChainLengthCunningham1).c_str(), TargetToString(nChainLengthCunningham2).c_str(), TargetToString(nChainLengthBiTwin).c_str(),
            TargetToString(nChainLengthCunningham1FermatTest).c_str(), TargetToString(nChainLengthCunningham2FermatTest).c_str(), TargetToString(nChainLengthBiTwinFermatTest).c_str());
        return false;
	}
    if (nChainLengthCunningham1 != nChainLengthCunningham1FermatTest ||
        nChainLengthCunningham2 != nChainLengthCunningham2FermatTest ||
        nChainLengthBiTwin != nChainLengthBiTwinFermatTest) {
        LogPrint(BCLog::PRIME, "CheckPrimeProofOfWork() : failed Fermat-only double check target=%s length=(%s %s %s) lengthFermat=(%s %s %s)", TargetToString(nBits).c_str(), 
            TargetToString(nChainLengthCunningham1).c_str(), TargetToString(nChainLengthCunningham2).c_str(), TargetToString(nChainLengthBiTwin).c_str(),
            TargetToString(nChainLengthCunningham1FermatTest).c_str(), TargetToString(nChainLengthCunningham2FermatTest).c_str(), TargetToString(nChainLengthBiTwinFermatTest).c_str());
        return false; 
	}
    // Select the longest primechain from the three chain types
    nChainLength = nChainLengthCunningham1;
    nChainType = PRIME_CHAIN_CUNNINGHAM1;
    if (nChainLengthCunningham2 > nChainLength)
    {
        nChainLength = nChainLengthCunningham2;
        nChainType = PRIME_CHAIN_CUNNINGHAM2;
    }
    if (nChainLengthBiTwin > nChainLength)
    {
        nChainLength = nChainLengthBiTwin;
        nChainType = PRIME_CHAIN_BI_TWIN;
    }

    // Check that the certificate (bnPrimeChainMultiplier) is normalized
    if (bnPrimeChainMultiplier % 2 == 0 && bnPrimeChainOrigin % 4 == 0)
    {
        unsigned int nChainLengthCunningham1Extended = 0;
        unsigned int nChainLengthCunningham2Extended = 0;
        unsigned int nChainLengthBiTwinExtended = 0;
        if (ProbablePrimeChainTest(bnPrimeChainOrigin / 2, nBits, false, nChainLengthCunningham1Extended, nChainLengthCunningham2Extended, nChainLengthBiTwinExtended))
        { // try extending down the primechain with a halved multiplier
            if (nChainLengthCunningham1Extended > nChainLength || nChainLengthCunningham2Extended > nChainLength || nChainLengthBiTwinExtended > nChainLength) {
                LogPrint(BCLog::PRIME, "CheckPrimeProofOfWork() : prime certificate not normalzied target=%s length=(%s %s %s) extend=(%s %s %s)",
                    TargetToString(nBits).c_str(),
                    TargetToString(nChainLengthCunningham1).c_str(), TargetToString(nChainLengthCunningham2).c_str(), TargetToString(nChainLengthBiTwin).c_str(),
                    TargetToString(nChainLengthCunningham1Extended).c_str(), TargetToString(nChainLengthCunningham2Extended).c_str(), TargetToString(nChainLengthBiTwinExtended).c_str());
                return false; 
			}
        }
    }

    return true;
}

bool CheckPrimeProofOfWorkV02Compatibility(uint256 hashBlockHeader)
{
    unsigned int nLength = 0;
    return (FermatProbablePrimalityTest(CBigNum(hashBlockHeader), nLength));
}

// prime target difficulty value for visualization
double GetPrimeDifficulty(unsigned int nBits)
{
    return ((double) nBits / (double) (1 << nFractionalBits));
}

// Estimate work transition target to longer prime chain
unsigned int EstimateWorkTransition(unsigned int nPrevWorkTransition, unsigned int nBits, unsigned int nChainLength)
{
    int64_t nInterval = 500;
    int64_t nWorkTransition = nPrevWorkTransition;
    unsigned int nBitsCeiling = 0;
    TargetSetLength(TargetGetLength(nBits)+1, nBitsCeiling);
    unsigned int nBitsFloor = 0;
    TargetSetLength(TargetGetLength(nBits), nBitsFloor);
    uint64_t nFractionalDifficulty = TargetGetFractionalDifficulty(nBits);
    bool fLonger = (TargetGetLength(nChainLength) > TargetGetLength(nBits));
    if (fLonger)
        nWorkTransition = (nWorkTransition * (((nInterval - 1) * nFractionalDifficulty) >> 32) + 2 * ((uint64_t) nBitsFloor)) / ((((nInterval - 1) * nFractionalDifficulty) >> 32) + 2);
    else
        nWorkTransition = ((nInterval - 1) * nWorkTransition + 2 * ((uint64_t) nBitsCeiling)) / (nInterval + 1);
    return nWorkTransition;
}

// Test probable prime chain for: nOrigin (miner version - for miner use only)
// Return value:
//   true - Probable prime chain found (nChainLength meeting target)
//   false - prime chain too short (nChainLength not meeting target)
static bool ProbablePrimeChainTestForMiner(const CBigNum& bnPrimeChainOrigin, unsigned int nBits, unsigned nCandidateType, unsigned int& nChainLength)
{
    nChainLength = 0;

    // Test for Cunningham Chain of first kind
    if (nCandidateType == PRIME_CHAIN_CUNNINGHAM1)
        ProbableCunninghamChainTest(bnPrimeChainOrigin-1, true, false, nChainLength);
    // Test for Cunningham Chain of second kind
    else if (nCandidateType == PRIME_CHAIN_CUNNINGHAM2)
        ProbableCunninghamChainTest(bnPrimeChainOrigin+1, false, false, nChainLength);
    else
    {
        unsigned int nChainLengthCunningham1 = 0;
        unsigned int nChainLengthCunningham2 = 0;
        if (ProbableCunninghamChainTest(bnPrimeChainOrigin-1, true, false, nChainLengthCunningham1))
        {
            ProbableCunninghamChainTest(bnPrimeChainOrigin+1, false, false, nChainLengthCunningham2);
            // Figure out BiTwin Chain length
            // BiTwin Chain allows a single prime at the end for odd length chain
            nChainLength =
                (TargetGetLength(nChainLengthCunningham1) > TargetGetLength(nChainLengthCunningham2))?
                    (nChainLengthCunningham2 + TargetFromInt(TargetGetLength(nChainLengthCunningham2)+1)) :
                    (nChainLengthCunningham1 + TargetFromInt(TargetGetLength(nChainLengthCunningham1)));
        }
    }

    return (nChainLength >= nBits);
}

// Perform Fermat test with trial division
// Return values:
//   true  - passes trial division test and Fermat test; probable prime
//   false - failed either trial division or Fermat test; composite
bool ProbablePrimalityTestWithTrialDivision(const CBigNum& bnCandidate, unsigned int nTrialDivisionLimit)
{
    // Trial division
    BOOST_FOREACH(unsigned int nPrime, vPrimes)
    {
        if (nPrime >= nTrialDivisionLimit)
            break;
        if (bnCandidate % nPrime == 0)
            return false; // failed trial division test
    }
    unsigned int nLength = 0;
    return (FermatProbablePrimalityTest(bnCandidate, nLength));
}

// Sieve for mining
boost::thread_specific_ptr<CSieveOfEratosthenes> psieve;
boost::thread_specific_ptr<CPrimeMiner> pminer;

// Mine probable prime chain of form: n = h * p# +/- 1
bool MineProbablePrimeChain(CBlock& block, CBigNum& bnFixedMultiplier, bool& fNewBlock, unsigned int& nTriedMultiplier, unsigned int& nProbableChainLength, unsigned int& nTests, unsigned int& nPrimesHit)
{
    nProbableChainLength = 0;
    nTests = 0;
    nPrimesHit = 0;

    if (fNewBlock && psieve.get() != NULL)
    {
        // Must rebuild the sieve
        psieve.reset();
    }
    fNewBlock = false;

    int64_t nStart, nCurrent; // microsecond timer
    CBlockIndex* pindexPrev = chainActive.Tip();
    if (psieve.get() == NULL)
    {
        // Build sieve
        psieve.reset(new CSieveOfEratosthenes(nMaxSieveSize, block.nBits, block.GetHeaderHash(), bnFixedMultiplier));
        int64_t nSieveRoundLimit = (int)gArgs.GetArg("-gensieveroundlimitms", 1000);
        nStart = GetTimeMicros();
        unsigned int nWeaveTimes = 0;
        while (psieve->Weave() && pindexPrev == chainActive.Tip() && (GetTimeMicros() - nStart < 1000 * nSieveRoundLimit) && (++nWeaveTimes < pminer->nSieveWeaveOptimal));
        nCurrent = GetTimeMicros();
        int64_t nSieveWeaveCost = (nCurrent - nStart) / std::max(nWeaveTimes, 1u); // average weave cost in us
        unsigned int nCandidateCount = psieve->GetCandidateCount();
        psieve->Weave(); // weave once more to find out about weave efficiency
        unsigned int nSieveWeaveComposites = nCandidateCount;
        nCandidateCount = psieve->GetCandidateCount();
        nSieveWeaveComposites = nCandidateCount - nSieveWeaveComposites; // number of composite chains found in last weave
        LogPrint(BCLog::PRIME, "MineProbablePrimeChain() : new sieve (%u/%u@%u/%u) ready in %uus test cost=%uus\n",
                nCandidateCount, nMaxSieveSize,
                (nWeaveTimes < vPrimes.size())? vPrimes[nWeaveTimes] : nPrimeTableLimit, pminer->GetSieveWeaveOptimalPrime(),
                (unsigned int) (nCurrent - nStart), (unsigned int)pminer->GetPrimalityTestCost());
        pminer->TimerSetSieveReady(nCandidateCount, nCurrent);
        pminer->SetSieveWeaveCount(nWeaveTimes);
        pminer->SetSieveWeaveCost(nSieveWeaveCost, nSieveWeaveComposites);
        pminer->AdjustSieveWeaveOptimal();
    }

    CBigNum bnChainOrigin;

    nStart = GetTimeMicros();
    nCurrent = nStart;

    while (nCurrent - nStart < 10000 && nCurrent >= nStart && pindexPrev == chainActive.Tip())
    {
        nTests++;
        unsigned int nCandidateType;
        if (!psieve->GetNextCandidateMultiplier(nTriedMultiplier, nCandidateType))
        {
            // power tests completed for the sieve
            pminer->TimerSetPrimalityDone(nCurrent);
            psieve.reset();
            fNewBlock = true; // notify caller to change nonce
            return false;
        }
        bnChainOrigin = CBigNum(block.GetHeaderHash()) * bnFixedMultiplier * nTriedMultiplier;
        unsigned int nChainLength = 0;
        if (ProbablePrimeChainTestForMiner(bnChainOrigin, block.nBits, nCandidateType, nChainLength))
        {
            block.bnPrimeChainMultiplier = bnFixedMultiplier * nTriedMultiplier;
            LogPrint(BCLog::PRIME, "Probable prime chain found for block=%s!!\n  Target: %s\n  Chain: %s\n", block.GetHash().GetHex().c_str(),
                TargetToString(block.nBits).c_str(), GetPrimeChainName(nCandidateType, nChainLength).c_str());
            nProbableChainLength = nChainLength;
            return true;
        }
        nProbableChainLength = nChainLength;
        if(TargetGetLength(nProbableChainLength) >= 1)
            nPrimesHit++;

        nCurrent = GetTimeMicros();
    }
    return false; // stop as timed out
}

// Weave sieve for the next prime in table
// Return values:
//   True  - weaved another prime
//   False - sieve already completed
bool CSieveOfEratosthenes::Weave()
{
    if (nPrimeSeq >= vPrimes.size() || vPrimes[nPrimeSeq] >= nSieveSize)
        return false;  // sieve has been completed
    CBigNum p = vPrimes[nPrimeSeq];
    if (bnFixedFactor % p == 0)
    {
        // Nothing in the sieve is divisible by this prime
        nPrimeSeq++;
        return true;
    }
    // Find the modulo inverse of fixed factor
    CAutoBN_CTX pctx;
    CBigNum bnFixedInverse;
    if (!BN_mod_inverse(&bnFixedInverse, &bnFixedFactor, &p, pctx)) {
		LogPrint(BCLog::PRIME, "CSieveOfEratosthenes::Weave(): BN_mod_inverse of fixed factor failed for prime #%u=%u", nPrimeSeq, vPrimes[nPrimeSeq]);
        return false; 
    }
    CBigNum bnTwo = 2;
    CBigNum bnTwoInverse;
    if (!BN_mod_inverse(&bnTwoInverse, &bnTwo, &p, pctx)) {
        LogPrint(BCLog::PRIME, "CSieveOfEratosthenes::Weave(): BN_mod_inverse of 2 failed for prime #%u=%u", nPrimeSeq, vPrimes[nPrimeSeq]);
        return false; 
	}
    // Weave the sieve for the prime
    unsigned int nChainLength = TargetGetLength(nBits);
    for (unsigned int nBiTwinSeq = 0; nBiTwinSeq < 2 * nChainLength; nBiTwinSeq++)
    {
        // Find the first number that's divisible by this prime
        int nDelta = ((nBiTwinSeq % 2 == 0)? (-1) : 1);
        unsigned int nSolvedMultiplier = ((bnFixedInverse * (p - nDelta)) % p).getuint();
        if (nBiTwinSeq % 2 == 1)
            bnFixedInverse *= bnTwoInverse; // for next number in chain

        unsigned int nPrime = vPrimes[nPrimeSeq];
        if (nBiTwinSeq < nChainLength)
            for (unsigned int nVariableMultiplier = nSolvedMultiplier; nVariableMultiplier < nSieveSize; nVariableMultiplier += nPrime)
                vfCompositeBiTwin[nVariableMultiplier] = true;
        if (((nBiTwinSeq & 1u) == 0))
            for (unsigned int nVariableMultiplier = nSolvedMultiplier; nVariableMultiplier < nSieveSize; nVariableMultiplier += nPrime)
                vfCompositeCunningham1[nVariableMultiplier] = true;
        if (((nBiTwinSeq & 1u) == 1u))
            for (unsigned int nVariableMultiplier = nSolvedMultiplier; nVariableMultiplier < nSieveSize; nVariableMultiplier += nPrime)
                vfCompositeCunningham2[nVariableMultiplier] = true;
    }
    nPrimeSeq++;
    return true;
}

// Estimate the probability of primality for a number in a candidate chain
double EstimateCandidatePrimeProbability()
{
    // h * q# / r# * s is prime with probability 1/log(h * q# / r# * s),
    //   (prime number theorem)
    //   here s ~ max sieve size / 2,
    //   h ~ 2^255 * 1.5,
    //   r = 7 (primorial multiplier embedded in the hash)
    // Euler product to p ~ 1.781072 * log(p)   (Mertens theorem)
    // If sieve is weaved up to p, a number in a candidate chain is a prime
    // with probability
    //     (1/log(h * q# / r# * s)) / (1/(1.781072 * log(p)))
    //   = 1.781072 * log(p) / (255 * log(2) + log(1.5) + log(q# / r#) + log(s))
    //
    // This model assumes that the numbers on a chain being primes are
    // statistically independent after running the sieve, which might not be
    // true, but nontheless it's a reasonable model of the chances of finding
    // prime chains.
    unsigned int nSieveWeaveOptimalPrime = pminer->GetSieveWeaveOptimalPrime();
    unsigned int nAverageCandidateMultiplier = nMaxSieveSize / 2;
    unsigned int nPrimorialMultiplier = pminer->nPrimorialMultiplier;
    double dFixedMultiplier = 1.0;
    for (unsigned int i = 0; vPrimes[i] <= nPrimorialMultiplier; i++)
        dFixedMultiplier *= vPrimes[i];
    return (1.781072 * log((double)std::max(1u, nSieveWeaveOptimalPrime)) / (255.0 * log(2.0) + log(1.5) + log(dFixedMultiplier) + log(nAverageCandidateMultiplier)));
}

unsigned int CPrimeMiner::GetSieveWeaveOptimalPrime()
{
    return (nSieveWeaveOptimal < vPrimes.size())? vPrimes[nSieveWeaveOptimal] : nPrimeTableLimit;
}

void CPrimeMiner::SetSieveWeaveCount(unsigned int nSieveWeaveCount)
{
    if (nSieveWeaveCount < nSieveWeaveOptimal)
        nSieveWeaveOptimal = std::max(nSieveWeaveCount, nSieveWeaveInitial);
}

void CPrimeMiner::AdjustSieveWeaveOptimal()
{
    if (fSieveRoundShrink)
        nSieveWeaveOptimal = std::max(nSieveWeaveOptimal * 95 / 100 + 1, nSieveWeaveInitial);
    else
        nSieveWeaveOptimal = std::min(nSieveWeaveOptimal * 100 / 95, (unsigned int) vPrimes.size());
}
