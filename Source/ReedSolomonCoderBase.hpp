// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "Utility.hpp"

#include <Pothos/Framework.hpp>

#include <string>

//
// Block class (valid types defined in source file)
//

template <typename T>
class ReedSolomonCoderBase: public Pothos::Block
{
    public:
        ReedSolomonCoderBase(
            size_t dtypeDimension,
            unsigned int symSize,
            unsigned int gfPoly,
            unsigned int fcr,
            unsigned int prim,
            unsigned int nroots);
        virtual ~ReedSolomonCoderBase();

        unsigned int symbolSize() const;
        void setSymbolSize(unsigned int symSize);

        unsigned int gfPoly() const;
        void setGFPoly(unsigned int gfpoly);

        unsigned int fcr() const;
        void setFCR(unsigned int fcr);

        unsigned int primElement() const;
        void setPrimElement(unsigned int primElement);

        unsigned int numRoots() const;
        void setNumRoots(unsigned int nroots);

        std::string startID() const;
        void setStartID(const std::string& startID);

    protected:
        static void validateParameters(
            unsigned int symSize,
            unsigned int gfPoly,
            unsigned int fcr,
            unsigned int prim,
            unsigned int nroots);

        void resetRSUPtr();

        virtual void _getSingleIterationElems(size_t* pInputElems, size_t* pOutputElems) = 0;
        bool _prepForData();

        static constexpr unsigned int TSizeBits = sizeof(T)*8;

        unsigned int _symSize;
        unsigned int _gfPoly;
        unsigned int _fcr;
        unsigned int _prim;
        unsigned int _nroots;
        std::string _startID;

        ReedSolomonUPtr _rsUPtr;
};
