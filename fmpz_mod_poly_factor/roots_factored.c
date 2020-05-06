/*
    Copyright (C) 2020 Daniel Schultz

    This file is part of FLINT.

    FLINT is free software: you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License (LGPL) as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.  See <http://www.gnu.org/licenses/>.
*/

#include "fmpz_mod_poly.h"
#include "fmpz_mod_poly_factor.h"
#include "fmpz.h"
#include "fmpq.h"

typedef fmpz_poly_t fmpz_list_t;

/* lets not generate solutions lists with length longer than LENGTH_LIMIT */
#if FLINT64
#define LENGTH_LIMIT (WORD(1) << 35)
#else
#define LENGTH_LIMIT (WORD(1) << 25)
#endif

/*
    The modulus of b is divisible by the modulus of a.
    Map b via the projection.
*/
static void map_down(fmpz_mod_poly_t a, const fmpz_mod_poly_t b)
{
    slong i;
    FLINT_ASSERT(fmpz_divisible(&b->p, &a->p));
    fmpz_mod_poly_fit_length(a, b->length);
    for (i = 0; i < b->length; i++)
        fmpz_mod(a->coeffs + i, b->coeffs + i, &a->p);
    a->length = b->length;
    _fmpz_mod_poly_normalise(a);
}

/*
    Every lifter needs a Diophantine equation solver:
    Given a, b, c with a >= 0, b > 0, try to push all solutions for x to
        a*x + b*y = c
    with 0 <= x < b.
*/
static int disolve(fmpz_list_t v,
                                const fmpz_t a, const fmpz_t b, const fmpz_t c)
{
    int success = 1;
    slong k;
    fmpz_t d, x, q, r, bbar;
    FLINT_ASSERT(fmpz_sgn(a) >= 0);
    FLINT_ASSERT(fmpz_sgn(b) > 0);

    fmpz_init(d);
    fmpz_init(x);
    fmpz_init(q);
    fmpz_init(r);
    fmpz_init(bbar);

    fmpz_gcdinv(d, x, a, b);
    fmpz_fdiv_qr(q, r, c, d);
    if (fmpz_is_zero(r))
    {
        fmpz_divexact(bbar, b, d);
        fmpz_mul(x, x, q);
        fmpz_fdiv_q(r, x, bbar);

        k = *d;
        if ((!COEFF_IS_MPZ(k)) && (k + v->length < LENGTH_LIMIT))
        {
            fmpz_poly_fit_length(v, k + v->length);
            for (k--; k >= 0; k--)
            {
                fmpz * v_back = v->coeffs + v->length;
                fmpz_sub_si(q, r, k);
                fmpz_set(v_back, x);
                fmpz_submul(v_back, bbar, q);
                FLINT_ASSERT(fmpz_sgn(v_back) >= 0);
                FLINT_ASSERT(fmpz_cmp(v_back, b) < 0);
                v->length++;
            }
        }
        else
        {
            /* too many solutions */
            success = 0;
        }
    }

    fmpz_clear(d);
    fmpz_clear(x);
    fmpz_clear(q);
    fmpz_clear(r);
    fmpz_clear(bbar);

    return success;
}

/* Fill x1 with the roots of f, where f->p is p^k */
static int roots_mod_prime_power(fmpz_list_t x1, fmpz_mod_poly_t fpk,
                                                       const fmpz_t p, slong k)
{
    int success = 1;
    slong e1, e2;
    slong i, j, old_length;
    fmpz_list_t x2;
    fmpz_mod_poly_t f, dfpk;
    fmpz_t pe1, pe2e1, fprime, mfpe1, t;

    FLINT_ASSERT(k >= 1);
    FLINT_ASSERT(fmpz_is_probabprime(p));

    fmpz_mod_poly_init(dfpk, &fpk->p);
    fmpz_mod_poly_derivative(dfpk, fpk);

    fmpz_poly_init(x2);
    fmpz_mod_poly_init(f, p);

    map_down(f, fpk);

    fmpz_init(pe1);
    fmpz_init(pe2e1);
    fmpz_init(fprime);
    fmpz_init(mfpe1);
    fmpz_init(t);

    /* try to fill x1 with solution mod p */
    x1->length = 0;
    if (f->length > 0)
    {
        fmpz_mod_poly_factor_t r;
        fmpz_mod_poly_factor_init(r);

        fmpz_mod_poly_roots(r, f, 0);
        fmpz_poly_fit_length(x1, r->num);
        for (i = 0; i < r->num; i++)
            fmpz_negmod(x1->coeffs + i, r->poly[i].coeffs + 0, p);
        x1->length = r->num;
        fmpz_mod_poly_factor_clear(r);
    }
    else
    {
        if (fmpz_cmp_si(p, LENGTH_LIMIT) >= 0)
        {
            /* too many solution mod p */
            success = 0;
            goto cleanup;
        }

        fmpz_poly_fit_length(x1, fmpz_get_si(p));
        for (i = 0; i < fmpz_get_si(p); i++)
            fmpz_set_si(x1->coeffs + i, i);
        x1->length = fmpz_get_si(p);
    }

    /* lift roots mod p^e1 to roots mod p^e2 */
    for (e1 = 1; e1 < k; e1 = e2)
    {
        e2 = FLINT_MIN(k, 2*e1);

        fmpz_pow_ui(pe1, p, e1);
        fmpz_pow_ui(pe2e1, p, e2 - e1);
        x2->length = 0;
        for (i = 0; i < x1->length; i++)
        {
            fmpz_mod_poly_evaluate_fmpz(mfpe1, fpk, x1->coeffs + i);
            fmpz_neg(mfpe1, mfpe1);
            FLINT_ASSERT(fmpz_divisible(mfpe1, pe1));
            fmpz_divexact(mfpe1, mfpe1, pe1);
            fmpz_mod_poly_evaluate_fmpz(fprime, dfpk, x1->coeffs + i);
            fmpz_mod(fprime, fprime, pe2e1);

            old_length = x2->length;
            if (!disolve(x2, fprime, pe2e1, mfpe1))
            {
                success = 0;
                goto cleanup;
            }

            for (j = old_length; j < x2->length; j++)
            {
                fmpz_mul(t, x2->coeffs + j, pe1);
                fmpz_add(x2->coeffs + j, x1->coeffs + i, t);
            }
        }
        fmpz_poly_swap(x1, x2);
    }

cleanup:

    fmpz_poly_clear(x2);
    fmpz_mod_poly_clear(f);
    fmpz_clear(pe1);
    fmpz_clear(pe2e1);
    fmpz_clear(fprime);
    fmpz_clear(mfpe1);
    fmpz_clear(t);

    fmpz_mod_poly_clear(dfpk);

    return success;
}


static int mul_si_checked(slong * a, slong b, slong c)
{
	ulong ahi, alo;
	smul_ppmm(ahi, alo, b, c);
	*a = alo;
	return FLINT_SIGN_EXT(alo) != ahi;
}



int fmpz_mod_poly_roots_factored(fmpz_mod_poly_factor_t r,
       const fmpz_mod_poly_t f, int with_multiplicity, const fmpz_factor_t fac)
{
    int success = 1;
    slong i, j, k, new_length;
    fmpz_t m;
    fmpz_list_t x0, x1, x2;
    fmpz_mod_poly_t fpe, tq, tr, tf;

    if (f->length <= 0)
    {
        flint_throw(FLINT_ERROR, "Exception in fmpz_mod_poly_roots_factored: "
                                                  "input polynomial is zero.");
        return 0;
    }

    fmpz_mod_poly_init(fpe, fac->p + 0);
    fmpz_mod_poly_init(tq, &f->p);
    fmpz_mod_poly_init(tr, &f->p);
    fmpz_mod_poly_init(tf, &f->p);

    fmpz_init_set_ui(m, 1);

    fmpz_poly_init(x0);
    fmpz_poly_init(x1);
    fmpz_poly_init(x2);

    i = 0;
    fmpz_pow_ui(&fpe->p, fac->p + i, fac->exp[i]);
    map_down(fpe, f);
    if (!roots_mod_prime_power(x0, fpe, fac->p + i, fac->exp[i]))
    {
        goto almost_failed;
    }

    for (i = 1; x0->length > 0 && i < fac->num; i++)
    {
        fmpz_mul(m, m, &fpe->p);

        fmpz_pow_ui(&fpe->p, fac->p + i, fac->exp[i]);
        map_down(fpe, f);
        if (!roots_mod_prime_power(x1, fpe, fac->p + i, fac->exp[i]))
        {
            goto almost_failed;
        }

        if (mul_si_checked(&new_length, x0->length, x1->length) ||
            new_length >= LENGTH_LIMIT)
        {
            goto almost_failed;
        }

        x2->length = 0;
        fmpz_poly_fit_length(x2, new_length);
        for (j = 0; j < x0->length; j++)
        for (k = 0; k < x1->length; k++)
        {
            fmpz_CRT(x2->coeffs + x2->length, x1->coeffs + k, &fpe->p,
                                              x0->coeffs + j, m, 0);
            x2->length++;
        }
        fmpz_poly_swap(x0, x2);
    }

    _fmpz_vec_sort(x0->coeffs, x0->length);

    fmpz_mod_poly_factor_fit_length(r, x0->length);
    for (i = 0; i < x0->length; i++)
    {
        fmpz_mod_poly_fit_length(r->poly + i, 2);
        fmpz_set(&r->poly[i].p, &f->p);
        fmpz_one(r->poly[i].coeffs + 1);
        fmpz_negmod(r->poly[i].coeffs + 0, x0->coeffs + i, &f->p);
        _fmpz_mod_poly_set_length(r->poly + i, 2);
        r->exp[i] = 1;
        if (with_multiplicity)
        {
            fmpz_mod_poly_divrem(tf, tr, f, r->poly + i);
            FLINT_ASSERT(fmpz_mod_poly_is_zero(tr));
            while (fmpz_mod_poly_divrem(tq, tr, tf, r->poly + i),
                   fmpz_mod_poly_is_zero(tr))
            {
                r->exp[i]++;
                fmpz_mod_poly_swap(tq, tf);
            }
        }
    }
    r->num = x0->length;

cleanup:

    fmpz_poly_clear(x0);
    fmpz_poly_clear(x1);
    fmpz_poly_clear(x2);

    fmpz_clear(m);

    fmpz_mod_poly_clear(fpe);

    fmpz_mod_poly_clear(tq);
    fmpz_mod_poly_clear(tr);
    fmpz_mod_poly_clear(tf);

    return success;

almost_failed:

    /* if any prime power is lacking roots, we can still succeed */

    for (i++; i < fac->num; i++)
    {
        fmpz_pow_ui(&fpe->p, fac->p + i, fac->exp[i]);
        map_down(fpe, f);
        if (roots_mod_prime_power(x1, fpe, fac->p + i, fac->exp[i]) &&
            x1->length == 0)
        {
            r->num = 0;
            goto cleanup;
        }
    }

    success = 0;

    goto cleanup;
}
