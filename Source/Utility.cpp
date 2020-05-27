// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: GPL-3.0-or-later

extern "C"
{
#include "errnoname.h"
#include "reed-solomon/rs.h"
}

#include "Utility.hpp"

#include <Pothos/Exception.hpp>

#include <cstring>

ReedSolomonUPtr initRSChar(
    unsigned int symsize,
    unsigned int gfpoly,
    unsigned int fcr,
    unsigned int prim,
    unsigned int nroots)
{
    return ReedSolomonUPtr(init_rs_char(symsize, gfpoly, fcr, prim, nroots), &free_rs_char);
}

ReedSolomonUPtr initRSInt(
    unsigned int symsize,
    unsigned int gfpoly,
    unsigned int fcr,
    unsigned int prim,
    unsigned int nroots)
{
    return ReedSolomonUPtr(init_rs_int(symsize, gfpoly, fcr, prim, nroots), &free_rs_int);
}

// TODO: move this into PothosCore(?)
static std::string errnoName(int errCode)
{
    return errCode ? std::string(::errnoname(-errCode)) : 0;
}

void throwOnErrCode(int errCode)
{
    if(errCode < 0)
    {
        throw Pothos::RuntimeException(errnoName(errCode)+": "+std::strerror(-errCode));
    }
}
