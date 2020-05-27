// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ReedSolomonCoderBase.hpp"

#include "Utility.hpp"

#include <Poco/Format.h>
#include <Poco/NumberFormatter.h>

#include <Pothos/Exception.hpp>

template <typename T>
ReedSolomonCoderBase<T>::ReedSolomonCoderBase(
    size_t dtypeDimension,
    unsigned int symSize,
    unsigned int gfPoly,
    unsigned int fcr,
    unsigned int prim,
    unsigned int nroots): Pothos::Block()
{
    validateParameters(symSize, gfPoly, fcr, prim, nroots);

    _symSize = symSize;
    _gfPoly = gfPoly;
    _fcr = fcr;
    _prim = prim;
    _nroots = nroots;

    const Pothos::DType dtype(typeid(T), dtypeDimension);
    this->setupInput(0, dtype);
    this->setupOutput(0, dtype);

    using Class = ReedSolomonCoderBase<T>;

    this->registerCall(this, POTHOS_FCN_TUPLE(Class, symbolSize));
    this->registerCall(this, POTHOS_FCN_TUPLE(Class, setSymbolSize));
    this->registerProbe("symbolSize");
    this->registerSignal("symbolSizeChanged");

    this->registerCall(this, POTHOS_FCN_TUPLE(Class, gfPoly));
    this->registerCall(this, POTHOS_FCN_TUPLE(Class, setGFPoly));
    this->registerProbe("gfPoly");
    this->registerSignal("gfPolyChanged");

    this->registerCall(this, POTHOS_FCN_TUPLE(Class, symbolSize));
    this->registerCall(this, POTHOS_FCN_TUPLE(Class, setSymbolSize));
    this->registerProbe("symbolSize");
    this->registerSignal("symbolSizeChanged");

    this->registerCall(this, POTHOS_FCN_TUPLE(Class, fcr));
    this->registerCall(this, POTHOS_FCN_TUPLE(Class, setFCR));
    this->registerProbe("fcr");
    this->registerSignal("fcrChanged");

    this->registerCall(this, POTHOS_FCN_TUPLE(Class, numRoots));
    this->registerCall(this, POTHOS_FCN_TUPLE(Class, setNumRoots));
    this->registerProbe("numRoots");
    this->registerSignal("numRootsChanged");
}

template <typename T>
unsigned int ReedSolomonCoderBase<T>::symbolSize() const
{
    return _symSize;
}

template <typename T>
void ReedSolomonCoderBase<T>::setSymbolSize(unsigned int symSize)
{
    validateParameters(symSize, _gfPoly, _fcr, _prim, _nroots);
    _symSize = symSize;

    this->emitSignal("symbolSizeChanged", symSize);
}

template <typename T>
unsigned int ReedSolomonCoderBase<T>::gfPoly() const
{
    return _gfPoly;
}

template <typename T>
void ReedSolomonCoderBase<T>::setGFPoly(unsigned int gfPoly)
{
    validateParameters(_symSize, gfPoly, _fcr, _prim, _nroots);
    _gfPoly = gfPoly;

    this->emitSignal("gfPolyChanged", gfPoly);
}

template <typename T>
unsigned int ReedSolomonCoderBase<T>::fcr() const
{
    return _fcr;
}

template <typename T>
void ReedSolomonCoderBase<T>::setFCR(unsigned int fcr)
{
    validateParameters(_symSize, _gfPoly, fcr, _prim, _nroots);
    _fcr = fcr;

    this->emitSignal("fcrChanged", fcr);
}

template <typename T>
unsigned int ReedSolomonCoderBase<T>::primElement() const
{
    return _prim;
}

template <typename T>
void ReedSolomonCoderBase<T>::setPrimElement(unsigned int prim)
{
    validateParameters(_symSize, _gfPoly, _fcr, prim, _nroots);
    _prim = prim;

    this->emitSignal("primElementChanged", prim);
}

template <typename T>
unsigned int ReedSolomonCoderBase<T>::numRoots() const
{
    return _nroots;
}

template <typename T>
void ReedSolomonCoderBase<T>::setNumRoots(unsigned int nroots)
{
    validateParameters(_symSize, _gfPoly, _fcr, _prim, nroots);
    _nroots = nroots;

    this->emitSignal("numRootsChanged", nroots);
}

template <typename T>
void ReedSolomonCoderBase<T>::validateParameters(
    unsigned int symSize,
    unsigned int /*gfPoly*/,
    unsigned int fcr,
    unsigned int prim,
    unsigned int nroots)
{
    const auto numSymbolValues = (1U << symSize);

    if(symSize > TSizeBits)
    {
        throw Pothos::RangeException(
                  "Symbol size cannot be larger than the element type",
                  Poco::format(
                      "%s > %s",
                      Poco::NumberFormatter::format(symSize),
                      Poco::NumberFormatter::format(TSizeBits)));
    }
    if(fcr >= numSymbolValues)
    {
        throw Pothos::RangeException(
                  "FCR cannot be greater or equal to the number of symbol values",
                  Poco::format(
                      "%s >= %s",
                      Poco::NumberFormatter::format(fcr),
                      Poco::NumberFormatter::format(numSymbolValues)));
    }
    if((0 == prim) || (prim >= numSymbolValues))
    {
        throw Pothos::RangeException(
                  "Primitive element cannot be greater or equal to the number of symbol values",
                  Poco::format(
                      "%s outside range [0, %s]",
                      Poco::NumberFormatter::format(prim),
                      Poco::NumberFormatter::format(numSymbolValues)));
    }
    if(nroots >= numSymbolValues)
    {
        throw Pothos::RangeException(
                  "Number of roots cannot be greater or equal to the number of symbol values",
                  Poco::format(
                      "%s >= %s",
                      Poco::NumberFormatter::format(nroots),
                      Poco::NumberFormatter::format(numSymbolValues)));
    }
}

//
// Valid types
//
template class ReedSolomonCoderBase<unsigned char>;
template class ReedSolomonCoderBase<int>;