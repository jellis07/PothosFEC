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
class ReedSolomonEncoder: public ReedSolomonCoderBase<T>
{
    public:
        ReedSolomonEncoder(
            size_t dtypeDimension,
            unsigned int symSize,
            unsigned int gfPoly,
            unsigned int fcr,
            unsigned int prim,
            unsigned int nroots
        ): ReedSolomonCoderBase<T>(dtypeDimension, symSize, gfPoly, fcr, prim, nroots)
        {
        }

        ~ReedSolomonEncoder(){}

        void work() override;

    private:
        void _getSingleIterationElems(size_t* pInputElems, size_t* pOutputElems) override;
};

template <>
void ReedSolomonEncoder<unsigned char>::work()
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
    const auto singleIterationElems = std::min(numInputIterations, numOutputIterations);

    const auto numIterations = elems / singleIterationElems;

    auto input = this->input(0);
    auto output = this->output(0);

    auto buffIn = input->buffer().as<const unsigned char*>();
    auto buffOut = output->buffer().as<unsigned char*>();

    output->postLabel(this->_startID, Pothos::NullObject(), 0);

    for(size_t i = 0; i < numIterations; ++i)
    {
        std::memcpy(buffOut, buffIn, inputIterationElems);
        encode_rs_char(
            _rsUPtr.get(),
            buffOut,
            (buffOut + inputIterationElems));

        buffIn += inputIterationElems;
        buffOut += outputIterationElems;

        input->consume(inputIterationElems);
        output->produce(outputIterationElems);
    }
}

template <>
void ReedSolomonEncoder<int>::work()
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
    const auto singleIterationElems = std::min(numInputIterations, numOutputIterations);

    const auto numIterations = elems / singleIterationElems;

    auto input = this->input(0);
    auto output = this->output(0);

    auto buffIn = input->buffer().as<const int*>();
    auto buffOut = output->buffer().as<int*>();

    output->postLabel(this->_startID, Pothos::NullObject(), 0);

    for(size_t i = 0; i < numIterations; ++i)
    {
        std::memcpy(buffOut, buffIn, inputIterationElems);
        encode_rs_int(
            _rsUPtr.get(),
            buffOut,
            (buffOut + inputIterationElems));

        buffIn += inputIterationElems;
        buffOut += outputIterationElems;

        input->consume(inputIterationElems);
        output->produce(outputIterationElems);
    }
}

template <typename T>
void ReedSolomonEncoder<T>::_getSingleIterationElems(
    size_t* pInputElems,
    size_t* pOutputElems)
{
    *pOutputElems = (1U << this->_symSize) - 1;
    *pInputElems = (*pOutputElems) - this->_nroots;
}

//
// Factory/registration
//

static Pothos::Block* makeReedSolomonEncoder(
    const Pothos::DType& dtype,
    unsigned int symSize,
    unsigned int gfPoly,
    unsigned int fcr,
    unsigned int prim,
    unsigned int nroots)
{
    #define IfTypeThenReturn(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new ReedSolomonEncoder<T>(dtype.dimension(), symSize, gfPoly, fcr, prim, nroots);

    IfTypeThenReturn(unsigned char)
    IfTypeThenReturn(int)

    throw Pothos::InvalidArgumentException(
              __FUNCTION__,
              "Unsupported type: "+dtype.name());
}

static Pothos::BlockRegistry registerReedSolomonEncoder(
    "/fec/rs_encoder",
    Pothos::Callable(&makeReedSolomonEncoder));
