// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>

using ReedSolomonUPtr = std::unique_ptr<void, void(*)(void*)>;

ReedSolomonUPtr initRSChar(
    unsigned int symsize,
    unsigned int gfpoly,
    unsigned int fcr,
    unsigned int prim,
    unsigned int nroots);

ReedSolomonUPtr initRSInt(
    unsigned int symsize,
    unsigned int gfpoly,
    unsigned int fcr,
    unsigned int prim,
    unsigned int nroots);

void throwOnErrCode(int errCode);
