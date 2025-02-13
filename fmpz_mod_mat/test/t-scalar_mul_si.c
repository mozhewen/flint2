/*
    Copyright (C) 2011 Fredrik Johansson

    This file is part of FLINT.

    FLINT is free software: you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License (LGPL) as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.  See <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <gmp.h>
#include "flint.h"
#include "fmpz_mod_mat.h"
#include "ulong_extras.h"

int
main(void)
{
    slong m, n, rep;
    FLINT_TEST_INIT(state);
    

    flint_printf("scalar_mul_si....");
    fflush(stdout);

    for (rep = 0; rep < 1000 * flint_test_multiplier(); rep++)
    {
        fmpz_mod_mat_t A, B, C, D;
        slong c;
        fmpz_t mod, c1;

        m = n_randint(state, 20);
        n = n_randint(state, 20);

	fmpz_init(c1);
	fmpz_init(mod);
	fmpz_randtest_not_zero(mod, state, 200);
        fmpz_abs(mod, mod);
        
	c = (slong) n_randbits(state, n_randint(state, FLINT_BITS));
        if (n_randint(state, 2) == 0) c = -c;
	
        fmpz_mod_mat_init(A, m, n, mod);
        fmpz_mod_mat_init(B, m, n, mod);
        fmpz_mod_mat_init(C, m, n, mod);
        fmpz_mod_mat_init(D, m, n, mod);

        fmpz_mod_mat_randtest(A, state);
        fmpz_mod_mat_randtest(B, state);

        fmpz_mod_mat_scalar_mul_si(C, A, c);
	fmpz_set_si(c1, c);
	fmpz_sub_ui(c1, c1, 1);
	fmpz_mod(c1, c1, A->mod);
        fmpz_mod_mat_scalar_mul_fmpz(D, A, c1);

        /* c*A - (c-1)*A == A */
        fmpz_mod_mat_sub(D, C, D);

        if (!fmpz_mod_mat_equal(A, D))
        {
            flint_printf("FAIL\n");
            fflush(stdout);
            flint_abort();
        }

        /* Aliasing */
        fmpz_mod_mat_scalar_mul_si(C, A, c);
        fmpz_mod_mat_scalar_mul_si(A, A, c);

        if (!fmpz_mod_mat_equal(A, C))
        {
            flint_printf("FAIL\n");
            fflush(stdout);
            flint_abort();
        }

        fmpz_mod_mat_clear(A);
        fmpz_mod_mat_clear(B);
        fmpz_mod_mat_clear(C);
        fmpz_mod_mat_clear(D);
	fmpz_clear(mod);
	fmpz_clear(c1);
    }

    FLINT_TEST_CLEANUP(state);
    
    flint_printf("PASS\n");
    return 0;
}
