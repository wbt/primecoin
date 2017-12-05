// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/block.h>

#include <hash.h>
#include <streams.h>
#include <tinyformat.h>
#include <utilstrencodings.h>
#include <crypto/common.h>
#include <chain.h> // why?

#define BEGIN(a)            ((char*)&(a))
#define END(a)              ((char*)&((&(a))[1]))

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

std::string CBlock::ToString() const
{
    std::stringstream s;
    s <<    strprintf("CBlock(hash=%s, hashBlockHeader=%s, ver=%d, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, bnPrimeChainMultiplier=%s, vtx=%u)\n",
            GetHash().ToString().c_str(),
            GetHeaderHash().ToString().c_str(),
            nVersion,
            hashPrevBlock.ToString().c_str(),
            hashMerkleRoot.ToString().c_str(),
            nTime, nBits, nNonce,
            bnPrimeChainMultiplier.GetHex().c_str(),
            vtx.size());
            
    for (unsigned int i = 0; i < vtx.size(); i++)
    {
        s << "  " << vtx[i]->ToString() << "\n";
    }
    return s.str();
}

std::string CBlockHeader::HeaderToString() const
{
    std::stringstream s;
    s <<    strprintf("CBlockHeader(hash=%s, hashBlockHeader=%s, ver=%d, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, bnPrimeChainMultiplier=%s)\n",
            GetHash().ToString().c_str(),
            GetHeaderHash().ToString().c_str(),
            nVersion,
            hashPrevBlock.ToString().c_str(),
            hashMerkleRoot.ToString().c_str(),
            nTime, nBits, nNonce,
            bnPrimeChainMultiplier.GetHex().c_str());

    return s.str();
}
