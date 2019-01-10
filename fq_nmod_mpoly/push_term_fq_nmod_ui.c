/*
    Copyright (C) 2019 Daniel Schultz

    This file is part of FLINT.

    FLINT is free software: you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License (LGPL) as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.  See <http://www.gnu.org/licenses/>.
*/

#include "fq_nmod_mpoly.h"

void _fq_nmod_mpoly_push_exp_ui(fq_nmod_mpoly_t A,
                              const ulong * exp, const fq_nmod_mpoly_ctx_t ctx)
{
    slong N;
    slong old_length = A->length;
    mp_bitcnt_t exp_bits;

    exp_bits = mpoly_exp_bits_required_ui(exp, ctx->minfo);
    exp_bits = mpoly_fix_bits(exp_bits, ctx->minfo);
    fq_nmod_mpoly_fit_bits(A, exp_bits, ctx);
    fq_nmod_mpoly_fit_length(A, old_length + 1, ctx);

    A->length = old_length + 1;
    N = mpoly_words_per_exp(A->bits, ctx->minfo);
    mpoly_set_monomial_ui(A->exps + N*old_length, exp, A->bits, ctx->minfo);
}


void fq_nmod_mpoly_push_term_fq_nmod_ui(fq_nmod_mpoly_t A, const fq_nmod_t c,
                              const ulong * exp, const fq_nmod_mpoly_ctx_t ctx)
{
    _fq_nmod_mpoly_push_exp_ui(A, exp, ctx);
    FLINT_ASSERT(A->length > 0);
    fq_nmod_set(A->coeffs + A->length - 1, c, ctx->fqctx);
}
