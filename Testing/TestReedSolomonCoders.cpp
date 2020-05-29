// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: GPL-3.0-or-later

#include "TestUtility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>

#include <turbofec/turbo.h>

#include <iostream>
#include <string>
#include <vector>

using namespace FECTests;

// TODO: test getters/setters

// See: https://github.com/quiet/libfec/blob/master/rstest.c#L15
struct TestParams
{
    unsigned int symSize;
    unsigned int gfPoly;
    unsigned int fcr;
    unsigned int prim;
    unsigned int nroots;
};
static const std::vector<TestParams> AllTestParams =
{
    {2, 0x7,     1,   1, 1},
    {3, 0xb,     1,   1, 2},
    {4, 0x13,    1,   1, 4},
    {5, 0x25,    1,   1, 6},
    {6, 0x43,    1,   1, 8},
    {7, 0x89,    1,   1, 10},
    {8, 0x11d,   1,   1, 32},
    {8, 0x187,   112,11, 32}, /* Duplicates CCSDS codec */
    {9, 0x211,   1,   1, 32},
    {10,0x409,   1,   1, 32},
    {11,0x805,   1,   1, 32},
    {12,0x1053,  1,   1, 32},
    {13,0x201b,  1,   1, 32},
    {14,0x4443,  1,   1, 32},
    {15,0x8003,  1,   1, 32},
    {16,0x1100b, 1,   1, 32},
};

static void testReedSolomonCoderSymmetry(const TestParams& testParams)
{
    const auto nn = (1U << testParams.symSize) - 1;
    const auto kk = nn - testParams.nroots;
    constexpr size_t numBlocks = 256;
    static const std::string startID = "START BLOCK";
    const auto numElems = kk * numBlocks;

    std::cout << "Testing (" << nn << "," << kk << ") code" << std::endl;

    auto feederSource = Pothos::BlockRegistry::make("/blocks/feeder_source", "uint8");
    auto rsEncoder = Pothos::BlockRegistry::make(
                         "/fec/rs_encoder",
                         "uint8",
                         testParams.symSize,
                         testParams.gfPoly,
                         testParams.fcr,
                         testParams.prim,
                         testParams.nroots);
    auto rsDecoder = Pothos::BlockRegistry::make(
                         "/fec/rs_decoder",
                         "uint8",
                         testParams.symSize,
                         testParams.gfPoly,
                         testParams.fcr,
                         testParams.prim,
                         testParams.nroots);
    auto collectorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", "uint8");

    auto randomInput = getRandomInput(numElems, false /*asBits*/);
    for(size_t i = 0; i < randomInput.elements(); ++i)
    {
        randomInput.as<std::uint8_t*>()[i] &= nn;
    }

    feederSource.call("feedBuffer", randomInput);
    feederSource.call("feedLabel", Pothos::Label(startID, 0, 0));

    // TODO: add stop ID
    rsEncoder.call("setStartID", startID);
    rsDecoder.call("setStartID", startID);

    {
        Pothos::Topology topology;

        topology.connect(feederSource, 0, rsEncoder, 0);
        topology.connect(rsEncoder, 0, rsDecoder, 0);
        topology.connect(rsDecoder, 0, collectorSink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.05));
    }

    const auto outputBuffer = collectorSink.call<Pothos::BufferChunk>("getBuffer");
    POTHOS_TEST_EQUAL(numElems, outputBuffer.elements());

    const auto expectedLabel = Pothos::Label(startID, Pothos::NullObject(), 0);
    const auto outputLabels = collectorSink.call<std::vector<Pothos::Label>>("getLabels");
    POTHOS_TEST_EQUAL(1, outputLabels.size());
    testLabelsEqual(expectedLabel, outputLabels[0]);

    // With no noise added between encoding and decoding, the decoded output should be identical
    // to the input.
    POTHOS_TEST_EQUALA(
        randomInput.as<const std::uint8_t*>(),
        outputBuffer.as<const std::uint8_t*>(),
        randomInput.elements());
}

POTHOS_TEST_BLOCK("/fec/tests", test_reedsolomon_coder_symmetry_uint8)
{
    for(const auto& testParams: AllTestParams)
    {
        if(testParams.symSize > 8) break;

        testReedSolomonCoderSymmetry(testParams);
    }
}

/*
POTHOS_TEST_BLOCK("/fec/tests", test_lte_encoder_output_length)
{
    constexpr size_t numElems = TURBO_MAX_K;
    constexpr size_t numOutputElems = numElems * 3 + 4 * 3;
    constexpr size_t numJunkElems = 512;
    constexpr unsigned rgen = 013;
    constexpr unsigned gen = 015;
    const std::string blockStartID = "START";

    auto feederSource = Pothos::BlockRegistry::make("/blocks/feeder_source", "uint8");
    auto lteEncoder = Pothos::BlockRegistry::make("/fec/lte_turbo_encoder", rgen, gen);
    std::vector<Pothos::Proxy> collectorSinks;
    for(size_t port = 0; port < 3; ++port)
    {
        collectorSinks.emplace_back(Pothos::BlockRegistry::make("/blocks/collector_sink", "uint8"));
    }

    auto randomInputPlusJunk = getRandomInput(numJunkElems);
    randomInputPlusJunk.append(getRandomInput(numElems));
    randomInputPlusJunk.append(getRandomInput(numJunkElems));

    feederSource.call("feedBuffer", randomInputPlusJunk);
    feederSource.call("feedLabel", Pothos::Label(blockStartID, numElems, numJunkElems));

    lteEncoder.call("setBlockStartID", blockStartID);

    {
        Pothos::Topology topology;

        topology.connect(feederSource, 0, lteEncoder, 0);
        topology.connect(lteEncoder, 0, collectorSinks[0], 0);
        topology.connect(lteEncoder, 1, collectorSinks[1], 0);
        topology.connect(lteEncoder, 2, collectorSinks[2], 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.05));
    }

    // This will make sure that only the intended data is encoded.
    POTHOS_TEST_EQUAL(numOutputElems, collectorSinks[0].call("getBuffer").get<size_t>("length"));
    POTHOS_TEST_EQUAL(numOutputElems, collectorSinks[1].call("getBuffer").get<size_t>("length"));
    POTHOS_TEST_EQUAL(numOutputElems, collectorSinks[2].call("getBuffer").get<size_t>("length"));

    const auto expectedLabel = Pothos::Label(blockStartID, numOutputElems, 0);
    const auto actualLabels = collectorSinks[0].call<std::vector<Pothos::Label>>("getLabels");
    POTHOS_TEST_EQUAL(1, actualLabels.size());
    testLabelsEqual(expectedLabel, actualLabels[0]);
}

POTHOS_TEST_BLOCK("/fec/tests", test_lte_decoder_output_length)
{
    constexpr size_t numOutputElems = TURBO_MAX_K;
    constexpr size_t numElems = numOutputElems * 3 + 4 * 3;
    constexpr size_t numJunkElems = 512;
    constexpr size_t numIterations = 4;
    const std::string blockStartID = "START";

    auto lteDecoder = Pothos::BlockRegistry::make("/fec/lte_turbo_decoder", numIterations, false);
    auto lteDecoderUnpack = Pothos::BlockRegistry::make("/fec/lte_turbo_decoder", numIterations, true);

    std::vector<Pothos::Proxy> feederSources;
    for(size_t port = 0; port < 3; ++port)
    {
        feederSources.emplace_back(Pothos::BlockRegistry::make("/blocks/feeder_source", "uint8"));
        auto randomInputPlusJunk = getRandomInput(numJunkElems);
        randomInputPlusJunk.append(getRandomInput(numElems));
        randomInputPlusJunk.append(getRandomInput(numJunkElems));

        feederSources.back().call("feedBuffer", randomInputPlusJunk);
        feederSources.back().call("feedLabel", Pothos::Label(blockStartID, numElems, numJunkElems));
    }

    auto collectorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", "uint8");
    auto collectorSinkUnpack = Pothos::BlockRegistry::make("/blocks/collector_sink", "uint8");

    lteDecoder.call("setBlockStartID", blockStartID);
    lteDecoderUnpack.call("setBlockStartID", blockStartID);

    {
        Pothos::Topology topology;

        for(size_t port = 0; port < 3; ++port)
        {
            topology.connect(feederSources[port], 0, lteDecoder, port);
            topology.connect(feederSources[port], 0, lteDecoderUnpack, port);
        }

        topology.connect(lteDecoder, 0, collectorSink, 0);
        topology.connect(lteDecoderUnpack, 0, collectorSinkUnpack, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.05));
    }

    // This will make sure that only the intended data is decoded.
    POTHOS_TEST_EQUAL(numOutputElems / 8, collectorSink.call("getBuffer").get<size_t>("length"));
    POTHOS_TEST_EQUAL(numOutputElems, collectorSinkUnpack.call("getBuffer").get<size_t>("length"));

    const auto expectedLabel = Pothos::Label(blockStartID, numOutputElems, 0);

    const auto actualLabels = collectorSink.call<std::vector<Pothos::Label>>("getLabels");
    POTHOS_TEST_EQUAL(1, actualLabels.size());
    testLabelsEqual(expectedLabel, actualLabels[0]);

    const auto actualUnpackLabels = collectorSinkUnpack.call<std::vector<Pothos::Label>>("getLabels");
    POTHOS_TEST_EQUAL(1, actualUnpackLabels.size());
    testLabelsEqual(expectedLabel, actualUnpackLabels[0]);
}

POTHOS_TEST_BLOCK("/fec/tests", test_lte_coder_symmetry)
{
    constexpr size_t numElems = TURBO_MAX_K;
    constexpr unsigned rgen = 013;
    constexpr unsigned gen = 015;
    constexpr size_t numIterations = 4;
    const std::string blockStartID = "START";

    const auto randomInput = getRandomInput(numElems);

    auto feederSource = Pothos::BlockRegistry::make("/blocks/feeder_source", "uint8");
    feederSource.call("feedBuffer", randomInput);
    feederSource.call("feedLabel", Pothos::Label(blockStartID, numElems, 0));

    auto lteEncoder = Pothos::BlockRegistry::make("/fec/lte_turbo_encoder", rgen, gen);
    auto lteDecoder = Pothos::BlockRegistry::make("/fec/lte_turbo_decoder", numIterations, true);

    lteEncoder.call("setBlockStartID", blockStartID);
    lteDecoder.call("setBlockStartID", blockStartID);

    auto collectorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", "uint8");

    {
        Pothos::Topology topology;

        topology.connect(feederSource, 0, lteEncoder, 0);

        topology.connect(lteEncoder, 0, lteDecoder, 0);
        topology.connect(lteEncoder, 1, lteDecoder, 1);
        topology.connect(lteEncoder, 2, lteDecoder, 2);

        topology.connect(lteDecoder, 0, collectorSink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.05));
    }

    const auto outputBuffer = collectorSink.call<Pothos::BufferChunk>("getBuffer");
    POTHOS_TEST_EQUAL(numElems, outputBuffer.elements());

    const auto expectedLabel = Pothos::Label(blockStartID, numElems, 0);
    const auto outputLabels = collectorSink.call<std::vector<Pothos::Label>>("getLabels");
    POTHOS_TEST_EQUAL(1, outputLabels.size());
    testLabelsEqual(expectedLabel, outputLabels[0]);

    // With no noise added between encoding and decoding, the decoded output should be identical
    // to the input.
    POTHOS_TEST_EQUALA(
        randomInput.as<const std::uint8_t*>(),
        outputBuffer.as<const std::uint8_t*>(),
        randomInput.elements());
}

// TODO: add noise to encoded values, decode, check BER

*/
