// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ReedSolomonCoderBase.hpp"

#include "Utility.hpp"

extern "C"
{
#include "reed-solomon/rs.h"
}

#include <Poco/Format.h>
#include <Poco/NumberFormatter.h>

#include <Pothos/Exception.hpp>

#include <algorithm>
#include <cstring>
#include <vector>

template <typename T>
class ReedSolomonDecoder: public ReedSolomonCoderBase<T>
{
    public:
        ReedSolomonDecoder(
            size_t dtypeDimension,
            unsigned int symSize,
            unsigned int gfPoly,
            unsigned int fcr,
            unsigned int prim,
            unsigned int nroots
        ): ReedSolomonCoderBase<T>(dtypeDimension, symSize, gfPoly, fcr, prim, nroots)
        {
        }

        ~ReedSolomonDecoder(){}

        void work() override;

    private:
        void _getSingleIterationElems(size_t* pInputElems, size_t* pOutputElems) override;
};

template <>
void ReedSolomonDecoder<unsigned char>::work()
{
    auto elems = this->workInfo().minElements;
    if(0 == elems) return;
    if(!_prepForData()) return;

    size_t inputIterationElems = 0;
    size_t outputIterationElems = 0;
    this->_getSingleIterationElems(
        &inputIterationElems,
        &outputIterationElems);

    auto input = this->input(0);
    auto output = this->output(0);

    const auto maxInputIterations = input->elements() / inputIterationElems;
    const auto maxOutputIterations = output->elements() / outputIterationElems;
    const auto maxIterations = std::min(maxInputIterations, maxOutputIterations);

    const auto idealNumIterations = elems / inputIterationElems;
    const auto numIterations = std::min(maxIterations, idealNumIterations);

    auto buffIn = input->buffer().as<const unsigned char*>();
    auto buffOut = output->buffer().as<unsigned char*>();

    output->postLabel(this->_startID, Pothos::NullObject(), 0);

    std::vector<unsigned char> intermediate(inputIterationElems);
    for(size_t i = 0; i < numIterations; ++i)
    {
        std::memcpy(intermediate.data(), buffIn, inputIterationElems);
        decode_rs_char(_rsUPtr.get(), intermediate.data(), nullptr, 0);
        std::memcpy(buffOut, intermediate.data(), outputIterationElems);

        buffIn += inputIterationElems;
        buffOut += outputIterationElems;

        input->consume(inputIterationElems);
        output->produce(outputIterationElems);
    }
}

template <>
void ReedSolomonDecoder<int>::work()
{
    auto elems = this->workInfo().minElements;
    if(0 == elems) return;
    if(!_prepForData()) return;

    size_t inputIterationElems = 0;
    size_t outputIterationElems = 0;
    this->_getSingleIterationElems(
        &inputIterationElems,
        &outputIterationElems);

    const auto numInputIterations = elems / inputIterationElems;
    const auto numOutputIterations = elems / outputIterationElems;
    const auto numIterations = std::min(numInputIterations, numOutputIterations);

    auto input = this->input(0);
    auto output = this->output(0);

    auto buffIn = input->buffer().as<const int*>();
    auto buffOut = output->buffer().as<int*>();

    output->postLabel(this->_startID, Pothos::NullObject(), 0);

    std::vector<int> intermediate(inputIterationElems);
    for(size_t i = 0; i < numIterations; ++i)
    {
        std::memcpy(intermediate.data(), buffIn, inputIterationElems*sizeof(int));
        decode_rs_int(_rsUPtr.get(), intermediate.data(), nullptr, 0);
        std::memcpy(buffOut, intermediate.data(), outputIterationElems*sizeof(int));

        buffIn += inputIterationElems;
        buffOut += outputIterationElems;

        input->consume(inputIterationElems);
        output->produce(outputIterationElems);
    }
}

template <typename T>
void ReedSolomonDecoder<T>::_getSingleIterationElems(
    size_t* pInputElems,
    size_t* pOutputElems)
{
    *pInputElems = (1U << this->_symSize) - 1;
    *pOutputElems = (*pInputElems) - this->_nroots;
}

//
// Factory/registration
//

static Pothos::Block* makeReedSolomonDecoder(
    const Pothos::DType& dtype,
    unsigned int symSize,
    unsigned int gfPoly,
    unsigned int fcr,
    unsigned int prim,
    unsigned int nroots)
{
    #define IfTypeThenReturn(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new ReedSolomonDecoder<T>(dtype.dimension(), symSize, gfPoly, fcr, prim, nroots);

    IfTypeThenReturn(unsigned char)
    IfTypeThenReturn(int)

    throw Pothos::InvalidArgumentException(
              __FUNCTION__,
              "Unsupported type: "+dtype.name());
}

static Pothos::BlockRegistry registerReedSolomonDecoder(
    "/fec/rs_decoder",
    Pothos::Callable(&makeReedSolomonDecoder));
