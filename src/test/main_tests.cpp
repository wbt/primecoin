// Copyright (c) 2014-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <validation.h>
#include <net.h>

#include <test/test_bitcoin.h>

#include <boost/signals2/signal.hpp>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(main_tests, TestingSetup)

static void TestBlockSubsidy(const Consensus::Params& consensusParams)
{
    std::map<int, CAmount> subsidy = { { 0x6000000, 2775000000l},
                                       { 0x7000000, 2038000000l},
                                       { 0x8000000, 1560000000l},
                                       { 0x9000000, 1233000000l},
                                       { 0xa000000,  999000000l},
                                       { 0xb000000,  825000000l},
                                       { 0xc000000,  693000000l},
                                       { 0xd000000,  591000000l},
                                       { 0xe000000,  509000000l},
                                       { 0xf000000,  444000000l},
                                       { 0x10000000, 390000000l},
                                       { 0x11000000, 345000000l},
                                       { 0x12000000, 308000000l},
                                       { 0x13000000, 276000000l},
                                       { 0x14000000, 249000000l},
                                       { 0x7f000000,   6000000l}
                                     };
    for (std::map<int, CAmount>::iterator it=subsidy.begin(); it!=subsidy.end(); ++it) {
        CAmount nSubsidy = GetBlockSubsidy(it->first, consensusParams);
        BOOST_CHECK_EQUAL(nSubsidy, it->second);
        printf("0x%ld, %ld\n", nSubsidy, it->second);
    }
}

BOOST_AUTO_TEST_CASE(block_subsidy_test)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    TestBlockSubsidy(chainParams->GetConsensus()); // As in main
}

BOOST_AUTO_TEST_CASE(subsidy_limit_test)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    CAmount nSum = 0;
    for (int nBits = 0; nBits < 0x6000000; nBits += 0x0200000) {
        CAmount nSubsidy = GetBlockSubsidy(nBits, chainParams->GetConsensus());
        BOOST_CHECK_EQUAL(nSubsidy, 0);
    }
}

bool ReturnFalse() { return false; }
bool ReturnTrue() { return true; }

BOOST_AUTO_TEST_CASE(test_combiner_all)
{
    boost::signals2::signal<bool (), CombinerAll> Test;
    BOOST_CHECK(Test());
    Test.connect(&ReturnFalse);
    BOOST_CHECK(!Test());
    Test.connect(&ReturnTrue);
    BOOST_CHECK(!Test());
    Test.disconnect(&ReturnFalse);
    BOOST_CHECK(Test());
    Test.disconnect(&ReturnTrue);
    BOOST_CHECK(Test());
}
BOOST_AUTO_TEST_SUITE_END()
