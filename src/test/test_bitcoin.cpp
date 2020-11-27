// Copyright (c) 2011-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/test_bitcoin.h>

#include <chainparams.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <crypto/sha256.h>
#include <validation.h>
#include <miner.h>
#include <net_processing.h>
#include <ui_interface.h>
#include <streams.h>
#include <rpc/server.h>
#include <rpc/register.h>
#include <script/sigcache.h>

#include <memory>

void CConnmanTest::AddNode(CNode& node)
{
    LOCK(g_connman->cs_vNodes);
    g_connman->vNodes.push_back(&node);
}

void CConnmanTest::ClearNodes()
{
    LOCK(g_connman->cs_vNodes);
    g_connman->vNodes.clear();
}

uint256 insecure_rand_seed = GetRandHash();
FastRandomContext insecure_rand_ctx(insecure_rand_seed);

extern bool fPrintToConsole;
extern void noui_connect();

BasicTestingSetup::BasicTestingSetup(const std::string& chainName)
{
        SHA256AutoDetect();
        RandomInit();
        ECC_Start();
        SetupEnvironment();
        SetupNetworking();
        InitSignatureCache();
        InitScriptExecutionCache();
        fPrintToDebugLog = false; // don't want to write to debug.log file
        fCheckBlockIndex = true;
        SelectParams(chainName);
        noui_connect();
}

BasicTestingSetup::~BasicTestingSetup()
{
        ECC_Stop();
}

TestingSetup::TestingSetup(const std::string& chainName) : BasicTestingSetup(chainName)
{
    const CChainParams& chainparams = Params();
        // Ideally we'd move all the RPC tests to the functional testing framework
        // instead of unit tests, but for now we need these here.

        RegisterAllCoreRPCCommands(tableRPC);
        ClearDatadirCache();
        pathTemp = fs::temp_directory_path() / strprintf("test_bitcoin_%lu_%i", (unsigned long)GetTime(), (int)(InsecureRandRange(100000)));
        fs::create_directories(pathTemp);
        gArgs.ForceSetArg("-datadir", pathTemp.string());

        // We have to run a scheduler thread to prevent ActivateBestChain
        // from blocking due to queue overrun.
        threadGroup.create_thread(boost::bind(&CScheduler::serviceQueue, &scheduler));
        GetMainSignals().RegisterBackgroundSignalScheduler(scheduler);

        mempool.setSanityCheck(1.0);
        pblocktree.reset(new CBlockTreeDB(1 << 20, true));
        pcoinsdbview.reset(new CCoinsViewDB(1 << 23, true));
        pcoinsTip.reset(new CCoinsViewCache(pcoinsdbview.get()));
        if (!LoadGenesisBlock(chainparams)) {
            throw std::runtime_error("LoadGenesisBlock failed.");
        }
        {
            CValidationState state;
            if (!ActivateBestChain(state, chainparams)) {
                throw std::runtime_error("ActivateBestChain failed.");
            }
        }
        nScriptCheckThreads = 3;
        for (int i=0; i < nScriptCheckThreads-1; i++)
            threadGroup.create_thread(&ThreadScriptCheck);
        g_connman = std::unique_ptr<CConnman>(new CConnman(0x1337, 0x1337)); // Deterministic randomness for tests.
        connman = g_connman.get();
        peerLogic.reset(new PeerLogicValidation(connman, scheduler));
}

TestingSetup::~TestingSetup()
{
        threadGroup.interrupt_all();
        threadGroup.join_all();
        GetMainSignals().FlushBackgroundCallbacks();
        GetMainSignals().UnregisterBackgroundSignalScheduler();
        g_connman.reset();
        peerLogic.reset();
        UnloadBlockIndex();
        pcoinsTip.reset();
        pcoinsdbview.reset();
        pblocktree.reset();
        fs::remove_all(pathTemp);
}

TestChain100Setup::TestChain100Setup() : TestingSetup(CBaseChainParams::REGTEST)
{
    // CreateAndProcessBlock() does not support building SegWit blocks, so don't activate in these tests.
    // TODO: fix the code to support SegWit blocks.
    UpdateVersionBitsParameters(Consensus::DEPLOYMENT_SEGWIT, 0, Consensus::BIP9Deployment::NO_TIMEOUT);
    // Generate a 100-block chain:
    coinbaseKey.MakeNewKey(true);
    CScript scriptPubKey = CScript() <<  ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    for (int i = 0; i < COINBASE_MATURITY; i++)
    {
        std::vector<CMutableTransaction> noTxns;
        CBlock b = CreateAndProcessBlock(noTxns, scriptPubKey);
        coinbaseTxns.push_back(*b.vtx[0]);
    }
}

//
// Create a new block with just given transactions, coinbase paying to
// scriptPubKey, and try to add it to the current chain.
//
CBlock
TestChain100Setup::CreateAndProcessBlock(const std::vector<CMutableTransaction>& txns, const CScript& scriptPubKey)
{
    const CChainParams& chainparams = Params();
    std::unique_ptr<CBlockTemplate> pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey);
    CBlock& block = pblocktemplate->block;

    // Replace mempool-selected txns with just coinbase plus passed-in txns:
    block.vtx.resize(1);
    for (const CMutableTransaction& tx : txns)
        block.vtx.push_back(MakeTransactionRef(tx));
    // IncrementExtraNonce creates a valid coinbase and merkleRoot
    unsigned int extraNonce = 0;
    {
        LOCK(cs_main);
        IncrementExtraNonce(&block, chainActive.Tip(), extraNonce);
    }

    while (!CheckProofOfWork(block.GetHash(), block.nBits, block.bnPrimeChainMultiplier, chainparams.GetConsensus())) ++block.nNonce;

    std::shared_ptr<const CBlock> shared_pblock = std::make_shared<const CBlock>(block);
    ProcessNewBlock(chainparams, shared_pblock, true, nullptr);

    CBlock result = block;
    return result;
}

TestChain100Setup::~TestChain100Setup()
{
}


CTxMemPoolEntry TestMemPoolEntryHelper::FromTx(const CMutableTransaction &tx) {
    CTransaction txn(tx);
    return FromTx(txn);
}

CTxMemPoolEntry TestMemPoolEntryHelper::FromTx(const CTransaction &txn) {
    return CTxMemPoolEntry(MakeTransactionRef(txn), nFee, nTime, nHeight,
                           spendsCoinbase, sigOpCost, lp);
}

/**
 * @returns a real block (bdf80a4705f77712811d869e6acf4b78fba18b65ae5bbfdddd93ea51dec3c8df)
 *      with 10 txs.
 */
CBlock getBlockbdf80()
{
    CBlock block;
    CDataStream stream(ParseHex("020000003d547c7cc3b6b5d49a75255f30e07bfb6f488441618d928324d985e786defd364d0861c88b4d6e3de3fb4fb912609167e7db7223866acfbf82d1eee0541b4557b840dc5103415a070a0200000bf33d07efc600116901af190a01000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0d026b13012400062f503253482fffffffff010050a06e00000000232102845206b0f4882cd28daa5fd0ca927d2dd417196bf1b73cd8aeb1c81d356e6cd8ac0000000001000000032a6e2a0bb9c25d675d459bafe5befe07a3d7c7b94adb17f380d77c183c9169d6000000004847304402202f18fcbacb8dafbde0f790b84da71327a174c1aa86cad9a1daf40239de892f3b022022f691976ebb94445dd8f08d5ca458e916b8f3d6198f4c914bbcf8bf2797ed2601fffffffff2fbe544d57e7e76811580461705c8e8457a535582b1cd9baf93de61cab950fa0000000048473044022042be3ffa1f4142c71572c65762f169890ce65ed58e2ccca7a732493dfb8059b502207af493e64886beb68ff1166785e972f3f0d3a970e27afa233dd62d1f7618058901ffffffff876c166cd6898515f736b07af468cee263ac4750573ae228c7697c55a89223980000000049483045022100c020fbb75c232ba813d4d3b39032c7107acf6bb446cc7323453f83b09c236484022003fb432983f237d6b34766db6dc2ff525e38826d356abb17b2b2c53b170ee1fd01ffffffff01c0480d6b010000001976a914e05b25470c2db969ae737a4b1950f111f49d1b6988ac000000000100000001d01ec8a3c5b9ae7719b7892a4b2ea3138a8024a5a1720fcf199647ad6b06f2950000000049483045022100bca24d10cd37e038a784412fa8eef30f366d23dd266c6fe8182dadab9ac49c8502201d32f0eafd1a4bde10b76b56c7c5847ca0a5953153a47517e4c316f642340d3e01ffffffff01401e2d79000000001976a914e05b25470c2db969ae737a4b1950f111f49d1b6988ac00000000010000000137242881d48ca34b68b998fc18d3fa043e68023eb9d200b865771cedc30fcfe100000000494830450221009e47ea7c2efd5acd6b420698cda38f649c0923660449a5c5a70a3deb4c9d096d02205397bbc2a30d615fb8df56df36d4bb0a87ee580ebe1d84d4fdc96bcae3ffc9e701ffffffff0100dc1d79000000001976a914e05b25470c2db969ae737a4b1950f111f49d1b6988ac0000000001000000019f2542922fb24a41fed965039dca77604f13cd0dd2199839e89e7bd09c9ec3c5000000004847304402202cd8185ffc436ad063b44e6c8e347c106c4e324ec35ea58004c24c1ece9ae626022077c823c74ad2fdca1e4a36698c0cd179f2a1075d8db9651ae6b30bc71bbc99c101ffffffff0100dc1d79000000001976a914e05b25470c2db969ae737a4b1950f111f49d1b6988ac000000000100000003c112c05d42e574da98a29e90612ec6dd5de463bb111953005de58f9c895190200000000048473044022070c83c258835f64a03cfc1ffd0140a6d0572a6b31f2066848cf289e5a8426a6002204b8c9f141477da54d0aeb9789d6f53d726885a38d4543794cbc42df2bd99463401ffffffff2b8e4e68d71f1dd1f66b4ebc8bc984501358077443f73e5f01099796f9e13ebf000000004948304502200be4b48c29efeebd7b75b23343a746957e5e7bab1b233ddc21be531899abc130022100e538307865a3268b1cb643f50b5607e652cedc795de845cddc1b328324f1500c01ffffffffef325a9622b5441afaf24852fee2fa417a2864e125fc4d8c9c1758585bee58d8000000004847304402205c19be5587ce5fad9d5c58164f9e1bb1f24f3c29b3f29b5891d4e0d0e27bc57902204b82dc94597c27859e70d1020d4f1430c9a2976c3809ff2007737dc9fa63d9c801ffffffff0100c5dd65010000001976a914e05b25470c2db969ae737a4b1950f111f49d1b6988ac0000000001000000025e92ab25a1494adc83957b39a9f9237aa75378f3ab807c3fdc0646d73f27720c0000000049483045022100cada680780695c400b045be416a0af5fbb3d6c7231ca26b2a715599e867e5a8402203c308137ab5213993570d11435e495ef9eed6bd36aefc3401f40d03c4f71116b01ffffffff606109b6351a9858df016be21db2368c8b4de5b7bb85d5beb9276e802ed6130b000000004847304402206918020a90888b24e993bebb2221442210da7e601fd08d9671aeb0d8e046aaf902207d375e02d5ffc9aad1c36e14cfa8e3b83e4cfe4abec8594194a4702d2dce11cb01ffffffff0180d07def000000001976a914e05b25470c2db969ae737a4b1950f111f49d1b6988ac000000000100000001367e5e55f7bcba187759331c88603fa53e4dcdd5383a68d82ebf753b0060c339000000004948304502202c09b0e1ec239d9d29a165f6463a769462d4aedec66fe78995d9e06aa940c78f022100e5c3f3564a25ea375ea60b3ab1b78c44baad94e04ab75ccce4e451f695b1901401ffffffff014015f078000000001976a914e05b25470c2db969ae737a4b1950f111f49d1b6988ac000000000100000001bac2b95043b7bdac55ba5e55b601449a4acd8a9eeb687487cadc40d50ccfa1ae000000004948304502210096270db340c8d9c03f3867e64246a68214ba191170431fc9b08690d9d6cd741e022067ef490984613b2c141a76db538edb775a91acd0461b146a2330b59bb43f06fc01ffffffff0100d3e078000000001976a914e05b25470c2db969ae737a4b1950f111f49d1b6988ac0000000001000000015fc54b2e3ea094321b47ca3b5b8a40f1fd91dfd5e8e202e5a1254d372585b111000000004948304502206524cf322b9d26e60a8320ae6dba2982af9606d7d3876f901ec1aabe18e432de022100fe693ec300a75aa2bafcf9f46d055c6313801dc1a9ed72777c147f13e29091ec01ffffffff010082bb76000000001976a914e05b25470c2db969ae737a4b1950f111f49d1b6988ac00000000"), SER_NETWORK, PROTOCOL_VERSION);
    stream >> block;
    return block;
}
