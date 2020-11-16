// Copyright (c) 2015-2017 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <pow.h>
#include <random.h>
#include <util.h>
#include <test/test_bitcoin.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(pow_tests, BasicTestingSetup)

/* Test calculation of next difficulty target with no constraints applying */
BOOST_AUTO_TEST_CASE(get_next_work)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    CBlockIndex pindexLast1;
    CBlockIndex pindexLast2;
    CBlockIndex pindexLast3;
    pindexLast1.nHeight = 32255;
    pindexLast1.nTime = 1373768017;
    pindexLast1.nBits = 0x07fd75e4;
    pindexLast2.nHeight = 32256;
    pindexLast2.nTime = 1373768021;
    pindexLast2.pprev = &pindexLast1;
    pindexLast2.nBits = 0x07fd7605;
    pindexLast3.nHeight = 32257;
    pindexLast3.nTime = 1373768035;
    pindexLast3.nBits = 0x07fd7624;
    CBlockHeader pblock = pindexLast3.GetBlockHeader();
    pindexLast3.pprev = &pindexLast2;
    BOOST_CHECK_EQUAL(GetNextWorkRequired(&pindexLast3, &pblock, chainParams->GetConsensus()), 0x07fd763e);
}

BOOST_AUTO_TEST_CASE(GetBlockProofEquivalentTime_test)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    std::vector<CBlockIndex> blocks(10000);
    for (int i = 0; i < 10000; i++) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        blocks[i].nTime = 1269211443 + i * chainParams->GetConsensus().nPowTargetSpacing;
        blocks[i].nBits = 0x207fffff; /* target 0x7fffff000... */
        blocks[i].nChainWork = i ? blocks[i - 1].nChainWork + GetBlockProof(blocks[i - 1], chainParams->GetConsensus()) : arith_uint256(0);
    }

    for (int j = 0; j < 1000; j++) {
        CBlockIndex *p1 = &blocks[InsecureRandRange(10000)];
        CBlockIndex *p2 = &blocks[InsecureRandRange(10000)];
        CBlockIndex *p3 = &blocks[InsecureRandRange(10000)];

        int64_t tdiff = GetBlockProofEquivalentTime(*p1, *p2, *p3, chainParams->GetConsensus());
        BOOST_CHECK_EQUAL(tdiff, p1->GetBlockTime() - p2->GetBlockTime());
    }
}

BOOST_AUTO_TEST_SUITE_END()
