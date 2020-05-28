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
        ): ReedSolomonCoderBase<T>(dtypeDimension, symSize, gfPoly, fcr, prim, nroots, true /*forwardBuffer*/)
        {
        }

        ~ReedSolomonEncoder(){}

        void work() override;

    private:
        std::string _startID;

        bool _prepForData();
};

template <>
void ReedSolomonEncoder<unsigned char>::work()
{
    auto elems = this->workInfo().minElements;
    if(0 == elems) return;

    auto input = this->input(0);
    auto output = this->output(0);
    if(!_prepForData()) return;

    const auto singleIterationElems = this->numElemsSingleIteration();
    const auto numIterations = elems / singleIterationElems;

    auto buff = input->takeBuffer();
    auto buffPtr = buff.as<unsigned char*>();

    std::vector<Pothos::Label> labels;

    for(size_t i = 0; i < numIterations; ++i)
    {
        std::vector<unsigned char> parity(this->_nroots);
        encode_rs_char(_rsUPtr.get(), buffPtr, parity.data());

        labels.emplace_back(
            this->_parityLabelID,
            std::move(parity),
            (i*singleIterationElems),
            singleIterationElems);

        buffPtr += singleIterationElems;
    }

    input->consume(singleIterationElems * numIterations);
    output->postBuffer(std::move(buff));

    for(auto& label: labels) output->postLabel(std::move(label));
}

template <>
void ReedSolomonEncoder<int>::work()
{
    auto elems = this->workInfo().minElements;
    if(0 == elems) return;
    if(!_prepForData()) return;

    const auto singleIterationElems = this->numElemsSingleIteration();
    const auto numIterations = elems / singleIterationElems;

    auto input = this->input(0);
    auto output = this->output(0);

    auto buff = input->takeBuffer();
    auto buffPtr = buff.as<int*>();

    std::vector<Pothos::Label> labels;

    for(size_t i = 0; i < numIterations; ++i)
    {
        std::vector<int> parity(this->_nroots);
        encode_rs_int(_rsUPtr.get(), buffPtr, parity.data());

        labels.emplace_back(
            this->_parityLabelID,
            std::move(parity),
            (i*singleIterationElems),
            singleIterationElems);

        buffPtr += singleIterationElems;
    }

    input->consume(singleIterationElems * numIterations);
    output->postBuffer(std::move(buff));

    for(auto& label: labels) output->postLabel(std::move(label));
}

template <typename T>
bool ReedSolomonEncoder<T>::_prepForData()
{
    if(_startID.empty()) return true;
    else
    {
        auto input = this->input(0);

        // See if this input has a start ID.
        auto labels = input->labels();

        auto labelWithIDIter = std::find_if(
                                   labels.begin(),
                                   labels.end(),
                                   [this](const Pothos::Label& label)
                                   {
                                       return (label.id == _startID);
                                   });
        if(labels.end() != labelWithIDIter)
        {
            // Skip all data before the buffer starts.
            if(0 != labelWithIDIter->index)
            {
                input->consume(labelWithIDIter->index);
                input->setReserve(this->numElemsSingleIteration());
                return false;
            }
            else return true;
        }
        else return false;
    }
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
